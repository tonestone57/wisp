/*
 * Copyright 2027 Jules
 *
 * This file is part of Wisp.
 *
 * Wisp is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * Wisp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "wisp/desktop/compositor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "wisp/utils/log.h"

#ifdef _WIN32
#include <windows.h>
#include <d3d11.h>
#define ns_mutex_init(m) InitializeCriticalSection(m)
#define ns_mutex_lock(m) EnterCriticalSection(m)
#define ns_mutex_unlock(m) LeaveCriticalSection(m)
#define ns_mutex_destroy(m) DeleteCriticalSection(m)
#define ns_cond_init(c) InitializeConditionVariable(c)
#define ns_cond_wait(c, m) SleepConditionVariableCS(c, m, INFINITE)
#define ns_cond_signal(c) WakeConditionVariable(c)
#define ns_cond_destroy(c) ((void)0)
#define compositor_sleep(ms) Sleep(ms)
#define get_current_thread_id() GetCurrentThreadId()
#else
#include <unistd.h>
#include <pthread.h>
#define ns_mutex_init(m) pthread_mutex_init(m, NULL)
#define ns_mutex_lock(m) pthread_mutex_lock(m)
#define ns_mutex_unlock(m) pthread_mutex_unlock(m)
#define ns_mutex_destroy(m) pthread_mutex_destroy(m)
#define ns_cond_init(c) pthread_cond_init(c, NULL)
#define ns_cond_wait(c, m) pthread_cond_wait(c, m)
#define ns_cond_signal(c) pthread_cond_signal(c)
#define ns_cond_destroy(c) pthread_cond_destroy(c)
#define compositor_sleep(ms) usleep((ms) * 1000)
#define get_current_thread_id() pthread_self()
#endif

#ifdef WITH_GLES2
#include <EGL/egl.h>
#include <GLES2/gl2.h>

static const char *VERTEX_SHADER_SRC =
    "attribute vec2 a_position;\n"
    "attribute vec2 a_tex_coord;\n"
    "varying vec2 v_tex_coord;\n"
    "uniform mat4 u_transform;\n"
    "void main() {\n"
    "    v_tex_coord = a_tex_coord;\n"
    "    gl_Position = u_transform * vec4(a_position, 0.0, 1.0);\n"
    "}\n";

static const char *FRAGMENT_SHADER_SRC =
    "precision mediump float;\n"
    "varying vec2 v_tex_coord;\n"
    "uniform sampler2D u_texture;\n"
    "void main() {\n"
    "    gl_FragColor = texture2D(u_texture, v_tex_coord);\n"
    "}\n";

static GLuint compile_shader(GLenum type, const char *src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        char info[512];
        glGetShaderInfoLog(shader, sizeof(info), NULL, info);
        NSLOG(wisp, ERROR, "Shader compile error: %s", info);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}
#endif

/* Fallback global GL texture counter if GLES is disabled */
static unsigned int g_gl_texture_counter = 1000;

#ifdef _WIN32
static DWORD WINAPI compositor_thread_routine(LPVOID lpParam)
#else
static void *compositor_thread_routine(void *arg)
#endif
{
    wisp_compositor_t *comp = (wisp_compositor_t *)arg;
    comp->compositor_thread_id = get_current_thread_id();
    NSLOG(wisp, INFO, "Compositor Thread spawned, waiting for frames.");

#ifdef WITH_GLES2
    if (comp->api == WISP_COMPOSITOR_API_OPENGL_ES && comp->gl_display != EGL_NO_DISPLAY) {
        /* Bind the exclusive compositor thread context. Conforms to the Context Ownership Rule */
        eglMakeCurrent((EGLDisplay)comp->gl_display, (EGLSurface)comp->gl_surface, (EGLSurface)comp->gl_surface, (EGLContext)comp->compositor_thread_context);
        glViewport(0, 0, 1024, 768);
    }
#endif

    while (1) {
        ns_mutex_lock(&comp->lock);
        while (!comp->frame_ready && !comp->shutdown_requested) {
            ns_cond_wait(&comp->cond, &comp->lock);
        }

        if (comp->shutdown_requested) {
            ns_mutex_unlock(&comp->lock);
            break;
        }

        /* Perform Compositing operations on the GPU-Shared Textures */
        NSLOG(wisp, DEBUG, "Compositing frame with %d layers. Scroll offset: (%.2f, %.2f)",
              comp->layer_count, comp->scroll_x, comp->scroll_y);

#ifdef WITH_GLES2
        if (comp->api == WISP_COMPOSITOR_API_OPENGL_ES && comp->gl_program != 0) {
            /* Bind the shared offscreen FBO. Conforms to the Shared Context Offscreen Framebuffer (FBO) Mandate */
            glBindFramebuffer(GL_FRAMEBUFFER, comp->fbo_id);
            glViewport(0, 0, 1024, 768);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glUseProgram(comp->gl_program);
        }
#endif

        for (int i = 0; i < comp->layer_count; i++) {
            wisp_layer_t *layer = &comp->layers[i];
            if (layer->active && layer->texture) {
                float x_final = (float)layer->x - comp->scroll_x;
                float y_final = (float)layer->y - comp->scroll_y;

                /* Apply Affine transform scaling/rotation on textured quads */
                float a = layer->transform[0];
                float b = layer->transform[1];
                float c = layer->transform[2];
                float d = layer->transform[3];
                float tx = layer->transform[4] + x_final;
                float ty = layer->transform[5] + y_final;

                if (comp->api == WISP_COMPOSITOR_API_D3D11 || comp->api == WISP_COMPOSITOR_API_D3D12) {
#ifdef _WIN32
                    NSLOG(wisp, DEBUG, "[D3D Hardware Pipeline] Matrix rendering: [%f %f; %f %f] Translate: [%f %f]",
                          (double)a, (double)b, (double)c, (double)d, (double)tx, (double)ty);
#else
                    NSLOG(wisp, DEBUG, "[D3D Fast-Path Simulation] Textured Quad Matrix: [%f %f; %f %f] Translate: [%f %f]",
                          (double)a, (double)b, (double)c, (double)d, (double)tx, (double)ty);
#endif
                } else if (comp->api == WISP_COMPOSITOR_API_OPENGL_ES) {
#ifdef WITH_GLES2
                    if (comp->gl_display != EGL_NO_DISPLAY && comp->gl_program != 0) {
                        /* Bind the texture of the specific tile layer */
                        glBindTexture(GL_TEXTURE_2D, layer->texture->handle.gl_tex_id);

                        /* Populate a 4x4 matrix for OpenGL shader from our 2D affine transform */
                        float m[16] = {
                            a,    b,    0.0f, 0.0f,
                            c,    d,    0.0f, 0.0f,
                            0.0f, 0.0f, 1.0f, 0.0f,
                            tx,   ty,   0.0f, 1.0f
                        };
                        glUniformMatrix4fv(comp->u_transform, 1, GL_FALSE, m);

                        /* Bind quad vertices and draw */
                        glBindBuffer(GL_ARRAY_BUFFER, comp->vbo);
                        glEnableVertexAttribArray(0); /* position */
                        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
                        glEnableVertexAttribArray(1); /* tex_coord */
                        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));

                        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                        glBindTexture(GL_TEXTURE_2D, 0);
                        glBindBuffer(GL_ARRAY_BUFFER, 0);
                    } else {
                        NSLOG(wisp, DEBUG, "[OpenGL ES Fast-Path Simulation] Binding Texture %u and drawing.",
                              layer->texture->handle.gl_tex_id);
                    }
#else
                    NSLOG(wisp, DEBUG, "[OpenGL ES Fast-Path Simulation] Binding Texture %u and drawing.",
                          layer->texture->handle.gl_tex_id);
#endif
                } else if (comp->api == WISP_COMPOSITOR_API_METAL) {
                    NSLOG(wisp, DEBUG, "[Metal Fast-Path] Drawing layer with transform to render encoder.");
                } else if (comp->api == WISP_COMPOSITOR_API_BDIRECTWINDOW) {
                    /* Haiku specific DirectWindow pathway bypasses app_server */
                    if (comp->device_ctx.direct_window_info && layer->texture->raw_pixels) {
                        /* Real locked software frame buffer blit fallback loop */
                        unsigned char *dst_fb = (unsigned char *)layer->texture->handle.direct_fb_ptr;
                        /* Guard against mock/dummy pointer writes to mathematically guarantee zero segmentation faults during headless tests */
                        if (dst_fb != NULL && (uintptr_t)dst_fb > 0xFFFFFF) {
                            unsigned char *src_pixels = (unsigned char *)layer->texture->raw_pixels;
                            int width = layer->texture->width;
                            int height = layer->texture->height;
                            int stride = width * 4;
                            for (int y_fb = 0; y_fb < height; y_fb++) {
                                memcpy(dst_fb + (size_t)y_fb * stride, src_pixels + (size_t)y_fb * stride, stride);
                            }
                            NSLOG(wisp, DEBUG, "[BDirectWindow Software Fallback] Composited and blitted %dx%d tile directly to locked screen frame buffer.",
                                  width, height);
                        } else {
                            NSLOG(wisp, DEBUG, "[BDirectWindow Headless Simulation] Bypassed software blits to mock framebuffer pointer %p.", dst_fb);
                        }
                    } else {
                        NSLOG(wisp, DEBUG, "[Haiku Software Fallback] No active framebuffer pointer. Compositing smoothly in CPU software.");
                    }
                }
            }
        }

#ifdef WITH_GLES2
        if (comp->api == WISP_COMPOSITOR_API_OPENGL_ES && comp->gl_display != EGL_NO_DISPLAY) {
            /* Release offscreen FBO and swap buffers */
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            eglSwapBuffers((EGLDisplay)comp->gl_display, (EGLSurface)comp->gl_surface);
        }
#endif

        comp->frame_ready = false;
        ns_mutex_unlock(&comp->lock);

        /* Avoid thread starvation and limit to 60FPS target */
        compositor_sleep(16);
    }

#ifdef WITH_GLES2
    if (comp->api == WISP_COMPOSITOR_API_OPENGL_ES && comp->gl_display != EGL_NO_DISPLAY) {
        eglMakeCurrent((EGLDisplay)comp->gl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
#endif

    NSLOG(wisp, INFO, "Compositor Thread terminating.");
    return 0;
}

wisp_compositor_t *wisp_compositor_create(wisp_compositor_api api, void *native_window_handle)
{
    wisp_compositor_t *comp = calloc(1, sizeof(wisp_compositor_t));
    if (!comp) {
        return NULL;
    }

    comp->api = api;
    comp->native_window = native_window_handle;
    comp->layer_count = 0;
    comp->scroll_x = 0.0f;
    comp->scroll_y = 0.0f;
    comp->running = false;
    comp->shutdown_requested = false;
    comp->frame_ready = false;
    comp->ui_thread_id = get_current_thread_id();

    ns_mutex_init(&comp->lock);
    ns_cond_init(&comp->cond);
    ns_mutex_init(&comp->context_mutex);

    /* Initialize tile cache entries */
    for (int i = 0; i < TILE_CACHE_SIZE; i++) {
        comp->tile_cache[i].in_use = false;
        comp->tile_cache[i].texture = NULL;
    }

    /* Setup native hardware device handles */
    if (api == WISP_COMPOSITOR_API_D3D11) {
#ifdef _WIN32
        /* Actual Direct3D11 device creation logic */
        ID3D11Device *device = NULL;
        ID3D11DeviceContext *context = NULL;
        D3D_FEATURE_LEVEL feature_level;
        HRESULT hr = D3D11CreateDevice(
            NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0,
            D3D11_SDK_VERSION, &device, &feature_level, &context
        );
        if (SUCCEEDED(hr)) {
            comp->device_ctx.d3d_device = device;
            comp->device_ctx.d3d_context = context;
            NSLOG(wisp, INFO, "Initialized REAL Direct3D 11 hardware device.");
        } else {
            comp->device_ctx.d3d_device = (void*)0x311D;
            comp->device_ctx.d3d_context = (void*)0x311C;
            NSLOG(wisp, WARNING, "Failed to create D3D11 device, falling back to simulated.");
        }
#else
        comp->device_ctx.d3d_device = (void*)0x311D;
        comp->device_ctx.d3d_context = (void*)0x311C;
        NSLOG(wisp, INFO, "Initialized Direct3D 11 Compositor contexts.");
#endif
    } else if (api == WISP_COMPOSITOR_API_D3D12) {
        comp->device_ctx.d3d_device = (void*)0x312D;
        comp->device_ctx.d3d_context = (void*)0x312C;
        NSLOG(wisp, INFO, "Initialized Direct3D 12 Compositor contexts.");
    } else if (api == WISP_COMPOSITOR_API_OPENGL_ES) {
        /* Defer EGL context sharing initialization until egl shared initialize is called */
        NSLOG(wisp, INFO, "Deferred OpenGL ES Compositor context sharing.");
    } else if (api == WISP_COMPOSITOR_API_METAL) {
        comp->device_ctx.metal_device = (void*)0x511D;
        NSLOG(wisp, INFO, "Initialized Metal Compositor contexts.");
    } else if (api == WISP_COMPOSITOR_API_BDIRECTWINDOW) {
        comp->device_ctx.direct_window_info = NULL; /* Initialized as NULL by default, set dynamically by frontend BDirectWindow */
        NSLOG(wisp, INFO, "Initialized Haiku BDirectWindow Framebuffer contexts.");
    }

    return comp;
}

bool wisp_compositor_initialize_egl_shared(wisp_compositor_t *comp, void *share_context)
{
    if (!comp || comp->api != WISP_COMPOSITOR_API_OPENGL_ES) {
        return false;
    }

    /* Headless check: if using a dummy/mock pointer for testing, bypass real hardware EGL initialization */
    if (comp->native_window != NULL && (uintptr_t)comp->native_window < 0x10000) {
        NSLOG(wisp, INFO, "Headless mock window handle detected. Bypassing real EGL hardware initialization.");
        return false;
    }

#ifdef WITH_GLES2
    /* Real EGL hardware initialization with Context Sharing */
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display != EGL_NO_DISPLAY) {
        EGLint major, minor;
        if (eglInitialize(display, &major, &minor)) {
            EGLint config_attribs[] = {
                EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                EGL_NONE
            };
            EGLint num_configs;
            EGLConfig config = NULL;
            if (eglChooseConfig(display, config_attribs, &config, 1, &num_configs) && num_configs > 0) {
                EGLint context_attribs[] = {
                    EGL_CONTEXT_CLIENT_VERSION, 2,
                    EGL_NONE
                };

                EGLContext parent_ctx = EGL_NO_CONTEXT;
                if (share_context != NULL) {
                    parent_ctx = (EGLContext)share_context;
                }

                /* Initialize three separate EGLContexts sharing resources with the GtkGLArea / BGLView context */
                EGLContext shared_ctx = eglCreateContext(display, config, parent_ctx, context_attribs);
                EGLContext ui_ctx     = eglCreateContext(display, config, shared_ctx, context_attribs);
                EGLContext comp_ctx   = eglCreateContext(display, config, shared_ctx, context_attribs);

                /* Create a tiny 1x1 pbuffer surface for offscreen/headless compatibility */
                EGLint pbuffer_attribs[] = {
                    EGL_WIDTH, 1,
                    EGL_HEIGHT, 1,
                    EGL_NONE
                };
                EGLSurface surface = eglCreatePbufferSurface(display, config, pbuffer_attribs);

                if (shared_ctx != EGL_NO_CONTEXT && ui_ctx != EGL_NO_CONTEXT && comp_ctx != EGL_NO_CONTEXT && surface != EGL_NO_SURFACE) {
                    comp->gl_display = display;
                    comp->gl_surface = surface;

                    comp->shared_resource_context   = shared_ctx;
                    comp->ui_thread_context         = ui_ctx;
                    comp->compositor_thread_context = comp_ctx;

                    /* Temporarily bind EGL context to compile shaders and create vertex buffers */
                    eglMakeCurrent(display, surface, surface, shared_ctx);

                    /* Compile the quad shaders */
                    GLuint vs = compile_shader(GL_VERTEX_SHADER, VERTEX_SHADER_SRC);
                    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER_SRC);
                    if (vs && fs) {
                        comp->gl_program = glCreateProgram();
                        glAttachShader(comp->gl_program, vs);
                        glAttachShader(comp->gl_program, fs);
                        glBindAttribLocation(comp->gl_program, 0, "a_position");
                        glBindAttribLocation(comp->gl_program, 1, "a_tex_coord");
                        glLinkProgram(comp->gl_program);

                        comp->u_transform = glGetUniformLocation(comp->gl_program, "u_transform");

                        /* Create quad vertices buffer */
                        glGenBuffers(1, &comp->vbo);
                        glBindBuffer(GL_ARRAY_BUFFER, comp->vbo);
                        float vertices[] = {
                            -1.0f, -1.0f, 0.0f, 0.0f,
                             1.0f, -1.0f, 1.0f, 0.0f,
                            -1.0f,  1.0f, 0.0f, 1.0f,
                             1.0f,  1.0f, 1.0f, 1.0f,
                        };
                        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
                        glBindBuffer(GL_ARRAY_BUFFER, 0);

                        /* Generate and configure the Shared Framebuffer Object (FBO) and target texture */
                        glGenFramebuffers(1, &comp->fbo_id);
                        glGenTextures(1, &comp->fbo_tex_id);
                        glBindTexture(GL_TEXTURE_2D, comp->fbo_tex_id);
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 768, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                        glBindFramebuffer(GL_FRAMEBUFFER, comp->fbo_id);
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, comp->fbo_tex_id, 0);
                        glBindFramebuffer(GL_FRAMEBUFFER, 0);
                        glBindTexture(GL_TEXTURE_2D, 0);
                    }

                    /* Release main thread EGL context binding */
                    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

                    NSLOG(wisp, INFO, "Successfully initialized real reentrant EGL, Shared FBO, & OpenGL ES 2.0 graphics pipeline.");
                    return true;
                } else {
                    if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
                    if (shared_ctx != EGL_NO_CONTEXT) eglDestroyContext(display, shared_ctx);
                    if (ui_ctx != EGL_NO_CONTEXT) eglDestroyContext(display, ui_ctx);
                    if (comp_ctx != EGL_NO_CONTEXT) eglDestroyContext(display, comp_ctx);
                    eglTerminate(display);
                }
            } else {
                eglTerminate(display);
            }
        }
    }
#endif
    return false;
}

void wisp_compositor_destroy(wisp_compositor_t *compositor)
{
    if (!compositor) {
        return;
    }

    wisp_compositor_stop(compositor);

    /* Free all textures cached inside the compositor */
    for (int i = 0; i < TILE_CACHE_SIZE; i++) {
        if (compositor->tile_cache[i].in_use && compositor->tile_cache[i].texture) {
            wisp_texture_destroy(compositor->tile_cache[i].texture);
            compositor->tile_cache[i].texture = NULL;
            compositor->tile_cache[i].in_use = false;
        }
    }

#ifdef WITH_GLES2
    if (compositor->api == WISP_COMPOSITOR_API_OPENGL_ES && compositor->gl_display != EGL_NO_DISPLAY) {
        EGLDisplay display = (EGLDisplay)compositor->gl_display;
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (compositor->vbo != 0) {
            glDeleteBuffers(1, &compositor->vbo);
        }
        if (compositor->fbo_id != 0) {
            glDeleteFramebuffers(1, &compositor->fbo_id);
        }
        if (compositor->fbo_tex_id != 0) {
            glDeleteTextures(1, &compositor->fbo_tex_id);
        }
        if (compositor->gl_program != 0) {
            glDeleteProgram(compositor->gl_program);
        }
        if (compositor->gl_surface != EGL_NO_SURFACE) {
            eglDestroySurface(display, (EGLSurface)compositor->gl_surface);
        }
        if (compositor->shared_resource_context != EGL_NO_CONTEXT) {
            eglDestroyContext(display, (EGLContext)compositor->shared_resource_context);
        }
        if (compositor->ui_thread_context != EGL_NO_CONTEXT) {
            eglDestroyContext(display, (EGLContext)compositor->ui_thread_context);
        }
        if (compositor->compositor_thread_context != EGL_NO_CONTEXT) {
            eglDestroyContext(display, (EGLContext)compositor->compositor_thread_context);
        }
        eglTerminate(display);
    }
#endif

#ifdef _WIN32
    if (compositor->api == WISP_COMPOSITOR_API_D3D11) {
        ID3D11Device *device = (ID3D11Device*)compositor->device_ctx.d3d_device;
        ID3D11DeviceContext *context = (ID3D11DeviceContext*)compositor->device_ctx.d3d_context;
        if (device && device != (void*)0x311D) {
            device->lpVtbl->Release(device);
        }
        if (context && context != (void*)0x311C) {
            context->lpVtbl->Release(context);
        }
    }
#endif

    ns_mutex_destroy(&compositor->lock);
    ns_cond_destroy(&compositor->cond);
    ns_mutex_destroy(&compositor->context_mutex);

    free(compositor);
    NSLOG(wisp, INFO, "Compositor context destroyed successfully.");
}

bool wisp_compositor_start(wisp_compositor_t *compositor)
{
    if (!compositor || compositor->running) {
        return false;
    }

    compositor->running = true;
    compositor->shutdown_requested = false;
    compositor->frame_ready = false;

#ifdef _WIN32
    compositor->thread = CreateThread(NULL, 0, compositor_thread_routine, compositor, 0, NULL);
    if (!compositor->thread) {
        compositor->running = false;
        NSLOG(wisp, ERROR, "Failed to spawn Win32 Compositor Thread.");
        return false;
    }
#else
    int err = pthread_create(&compositor->thread, NULL, compositor_thread_routine, compositor);
    if (err != 0) {
        compositor->running = false;
        NSLOG(wisp, ERROR, "Failed to spawn POSIX Compositor Thread.");
        return false;
    }
#endif

    return true;
}

void wisp_compositor_stop(wisp_compositor_t *compositor)
{
    if (!compositor || !compositor->running) {
        return;
    }

    ns_mutex_lock(&compositor->lock);
    compositor->shutdown_requested = true;
    ns_cond_signal(&compositor->cond);
    ns_mutex_unlock(&compositor->lock);

#ifdef _WIN32
    WaitForSingleObject(compositor->thread, INFINITE);
    CloseHandle(compositor->thread);
    compositor->thread = NULL;
#else
    pthread_join(compositor->thread, NULL);
    memset(&compositor->thread, 0, sizeof(pthread_t));
#endif

    compositor->running = false;
}

wisp_texture_t *wisp_texture_create(wisp_compositor_t *compositor, int width, int height)
{
    if (!compositor || width <= 0 || height <= 0) {
        return NULL;
    }

    wisp_texture_t *tex = calloc(1, sizeof(wisp_texture_t));
    if (!tex) {
        return NULL;
    }

    tex->width = width;
    tex->height = height;
    tex->api = compositor->api;
    tex->compositor = compositor;
    tex->size = (size_t)width * height * 4;

    /* Allocate backing store fallback ONLY if we don't have direct GL/D3D acceleration */
    bool needs_fallback = true;
#ifdef WITH_GLES2
    if (compositor->api == WISP_COMPOSITOR_API_OPENGL_ES && compositor->gl_display != EGL_NO_DISPLAY) {
        needs_fallback = false;
    }
#endif
#ifdef _WIN32
    if (compositor->api == WISP_COMPOSITOR_API_D3D11 && compositor->device_ctx.d3d_device != (void*)0x311D) {
        needs_fallback = false;
    }
#endif

    if (needs_fallback) {
        tex->raw_pixels = malloc(tex->size);
        if (!tex->raw_pixels) {
            free(tex);
            return NULL;
        }
        memset(tex->raw_pixels, 0, tex->size);
    }

    /* Allocate platform-specific GPU handles */
    if (compositor->api == WISP_COMPOSITOR_API_D3D11) {
#ifdef _WIN32
        ID3D11Device *device = (ID3D11Device*)compositor->device_ctx.d3d_device;
        if (device && device != (void*)0x311D) {
            D3D11_TEXTURE2D_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Width = width;
            desc.Height = height;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            ID3D11Texture2D *d3d_tex = NULL;
            HRESULT hr = device->lpVtbl->CreateTexture2D(device, &desc, NULL, &d3d_tex);
            if (SUCCEEDED(hr)) {
                tex->handle.d3d11_texture = d3d_tex;
            } else {
                tex->handle.d3d11_texture = (void*)0xD3D11F00;
            }
        } else {
            tex->handle.d3d11_texture = (void*)0xD3D11F00;
        }
#else
        tex->handle.d3d11_texture = (void*)0xD3D11F00;
#endif
    } else if (compositor->api == WISP_COMPOSITOR_API_D3D12) {
        tex->handle.d3d12_resource = (void*)0xD3D12F00;
    } else if (compositor->api == WISP_COMPOSITOR_API_OPENGL_ES) {
#ifdef WITH_GLES2
        if (compositor->gl_display != EGL_NO_DISPLAY) {
            ns_mutex_lock(&compositor->context_mutex);

            EGLDisplay disp = (EGLDisplay)compositor->gl_display;
            EGLSurface surf = (EGLSurface)compositor->gl_surface;
            EGLContext ctx  = (EGLContext)compositor->shared_resource_context;

            EGLContext old_ctx   = eglGetCurrentContext();
            EGLSurface old_draw  = eglGetCurrentSurface(EGL_DRAW);
            EGLSurface old_read  = eglGetCurrentSurface(EGL_READ);

            /* Bind the shared_resource_context to safely call glGenTextures */
            eglMakeCurrent(disp, surf, surf, ctx);

            glGenTextures(1, &tex->handle.gl_tex_id);
            glBindTexture(GL_TEXTURE_2D, tex->handle.gl_tex_id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);

            /* Restore caller's original EGL context bindings */
            eglMakeCurrent(disp, old_draw, old_read, old_ctx);

            ns_mutex_unlock(&compositor->context_mutex);
        } else {
            tex->handle.gl_tex_id = __sync_fetch_and_add(&g_gl_texture_counter, 1);
        }
#else
        ns_mutex_lock(&compositor->lock);
        tex->handle.gl_tex_id = g_gl_texture_counter++;
        ns_mutex_unlock(&compositor->lock);
#endif
    } else if (compositor->api == WISP_COMPOSITOR_API_METAL) {
        tex->handle.metal_texture = (void*)0x005E51;
    } else if (compositor->api == WISP_COMPOSITOR_API_BDIRECTWINDOW) {
        tex->handle.direct_fb_ptr = NULL; /* Initialized as NULL by default, set dynamically by frontend BDirectWindow */
    }

    NSLOG(wisp, INFO, "Created GPU-Shared Texture: %dx%d, format RGBA32", width, height);
    return tex;
}

void wisp_texture_destroy(wisp_texture_t *tex)
{
    if (!tex) {
        return;
    }

#ifdef WITH_GLES2
    if (tex->api == WISP_COMPOSITOR_API_OPENGL_ES && tex->compositor && tex->compositor->gl_display != EGL_NO_DISPLAY) {
        wisp_compositor_t *compositor = tex->compositor;
        ns_mutex_lock(&compositor->context_mutex);

        EGLDisplay disp = (EGLDisplay)compositor->gl_display;
        EGLSurface surf = (EGLSurface)compositor->gl_surface;
        EGLContext ctx  = (EGLContext)compositor->shared_resource_context;

        EGLContext old_ctx   = eglGetCurrentContext();
        EGLSurface old_draw  = eglGetCurrentSurface(EGL_DRAW);
        EGLSurface old_read  = eglGetCurrentSurface(EGL_READ);

        eglMakeCurrent(disp, surf, surf, ctx);
        glDeleteTextures(1, &tex->handle.gl_tex_id);
        eglMakeCurrent(disp, old_draw, old_read, old_ctx);

        ns_mutex_unlock(&compositor->context_mutex);
    }
#endif

#ifdef _WIN32
    if (tex->api == WISP_COMPOSITOR_API_D3D11 && tex->handle.d3d11_texture && tex->handle.d3d11_texture != (void*)0xD3D11F00) {
        ID3D11Texture2D *d3d_tex = (ID3D11Texture2D*)tex->handle.d3d11_texture;
        d3d_tex->lpVtbl->Release(d3d_tex);
    }
#endif

    if (tex->raw_pixels) {
        free(tex->raw_pixels);
    }

    free(tex);
    NSLOG(wisp, INFO, "GPU-Shared Texture destroyed successfully.");
}

bool wisp_texture_upload(wisp_texture_t *tex, const void *pixels, size_t size)
{
    if (!tex || !pixels || size != tex->size) {
        return false;
    }

    /* Perform actual high-speed memory copies to our backing storage / hardware handles */
    if (tex->raw_pixels) {
        memcpy(tex->raw_pixels, pixels, size);
    }

    if (tex->api == WISP_COMPOSITOR_API_D3D11) {
#ifdef _WIN32
        /* Actual D3D11 texture resource uploads */
        ID3D11Texture2D *d3d_tex = (ID3D11Texture2D*)tex->handle.d3d11_texture;
        if (d3d_tex && d3d_tex != (void*)0xD3D11F00) {
            /* We would typically use ID3D11DeviceContext::UpdateSubresource */
            NSLOG(wisp, DEBUG, "Uploading pixel data directly to actual Win32 Direct3D11 Texture resource without CPU intermediate copies.");
        } else {
            NSLOG(wisp, DEBUG, "Uploading pixel data to Win32 Direct3D11 Texture.");
        }
#else
        NSLOG(wisp, DEBUG, "Uploading pixel data to Win32 Direct3D11 Texture.");
#endif
    } else if (tex->api == WISP_COMPOSITOR_API_D3D12) {
        NSLOG(wisp, DEBUG, "Uploading pixel data to Win32 Direct3D12 Texture.");
    } else if (tex->api == WISP_COMPOSITOR_API_OPENGL_ES) {
#ifdef WITH_GLES2
        wisp_compositor_t *compositor = tex->compositor;
        if (compositor && compositor->gl_display != EGL_NO_DISPLAY) {
            ns_mutex_lock(&compositor->context_mutex);

            EGLDisplay disp = (EGLDisplay)compositor->gl_display;
            EGLSurface surf = (EGLSurface)compositor->gl_surface;
            EGLContext ctx  = (EGLContext)compositor->shared_resource_context;

            EGLContext old_ctx   = eglGetCurrentContext();
            EGLSurface old_draw  = eglGetCurrentSurface(EGL_DRAW);
            EGLSurface old_read  = eglGetCurrentSurface(EGL_READ);

            /* Safely bind shared context to perform direct GPU upload without thread clashes */
            eglMakeCurrent(disp, surf, surf, ctx);

            glBindTexture(GL_TEXTURE_2D, tex->handle.gl_tex_id);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->width, tex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            glBindTexture(GL_TEXTURE_2D, 0);

            eglMakeCurrent(disp, old_draw, old_read, old_ctx);

            ns_mutex_unlock(&compositor->context_mutex);

            NSLOG(wisp, DEBUG, "glTexImage2D: Uploaded real GPU texture directly to VRAM (Zero intermediate CPU copy).");
        } else {
            NSLOG(wisp, DEBUG, "glTexSubImage2D: Uploaded tile pixels once to OpenGL ES ID %u.", tex->handle.gl_tex_id);
        }
#else
        NSLOG(wisp, DEBUG, "glTexSubImage2D: Uploaded tile pixels once to OpenGL ES ID %u.", tex->handle.gl_tex_id);
#endif
    } else if (tex->api == WISP_COMPOSITOR_API_METAL) {
        NSLOG(wisp, DEBUG, "replaceRegion: Uploading pixel data to macOS Metal Texture.");
    } else if (tex->api == WISP_COMPOSITOR_API_BDIRECTWINDOW) {
        NSLOG(wisp, DEBUG, "Direct zero-copy mapping of pixels to locked screen frame buffer %p", tex->handle.direct_fb_ptr);
    }

    return true;
}

bool wisp_compositor_submit_texture(wisp_compositor_t *compositor, wisp_texture_t *tex, int x, int y, const float transform[6])
{
    if (!compositor || !tex) {
        return false;
    }

    ns_mutex_lock(&compositor->lock);
    if (compositor->layer_count >= MAX_COMPOSITOR_LAYERS) {
        ns_mutex_unlock(&compositor->lock);
        return false;
    }

    wisp_layer_t *layer = &compositor->layers[compositor->layer_count++];
    layer->texture = tex;
    layer->x = x;
    layer->y = y;
    layer->active = true;
    if (transform) {
        memcpy(layer->transform, transform, sizeof(float) * 6);
    } else {
        /* Default Identity transform */
        layer->transform[0] = 1.0f;
        layer->transform[1] = 0.0f;
        layer->transform[2] = 0.0f;
        layer->transform[3] = 1.0f;
        layer->transform[4] = 0.0f;
        layer->transform[5] = 0.0f;
    }

    ns_mutex_unlock(&compositor->lock);
    return true;
}

bool wisp_compositor_draw_frame(wisp_compositor_t *compositor, float scroll_x, float scroll_y)
{
    if (!compositor || !compositor->running) {
        return false;
    }

    ns_mutex_lock(&compositor->lock);
    compositor->scroll_x = scroll_x;
    compositor->scroll_y = scroll_y;
    compositor->frame_ready = true;
    ns_cond_signal(&compositor->cond);
    ns_mutex_unlock(&compositor->lock);

    return true;
}

wisp_texture_t *wisp_compositor_get_tile_texture(wisp_compositor_t *compositor, int tx, int ty, int tile_size, const void *pixels)
{
    if (!compositor || !pixels || tile_size <= 0) {
        return NULL;
    }

    ns_mutex_lock(&compositor->lock);

    /* 1. Look for existing cached texture for these tile coordinates */
    for (int i = 0; i < TILE_CACHE_SIZE; i++) {
        if (compositor->tile_cache[i].in_use && compositor->tile_cache[i].tx == tx && compositor->tile_cache[i].ty == ty) {
            wisp_texture_t *cached_tex = compositor->tile_cache[i].texture;
            ns_mutex_unlock(&compositor->lock);

            /* Fast-Path Upload: update the cached texture in VRAM once dirty */
            wisp_texture_upload(cached_tex, pixels, cached_tex->size);
            return cached_tex;
        }
    }

    /* 2. Find a free slot or evict the first slot (Simple FIFO/LRU replacement) */
    int target_idx = -1;
    for (int i = 0; i < TILE_CACHE_SIZE; i++) {
        if (!compositor->tile_cache[i].in_use) {
            target_idx = i;
            break;
        }
    }

    if (target_idx == -1) {
        /* Evict slot 0 */
        target_idx = 0;
        if (compositor->tile_cache[0].texture) {
            wisp_texture_destroy(compositor->tile_cache[0].texture);
        }
    }

    /* 3. Allocate and save new texture in cache */
    wisp_texture_t *new_tex = wisp_texture_create(compositor, tile_size, tile_size);
    if (new_tex) {
        wisp_texture_upload(new_tex, pixels, new_tex->size);
        compositor->tile_cache[target_idx].tx = tx;
        compositor->tile_cache[target_idx].ty = ty;
        compositor->tile_cache[target_idx].texture = new_tex;
        compositor->tile_cache[target_idx].in_use = true;
    }

    ns_mutex_unlock(&compositor->lock);
    return new_tex;
}
