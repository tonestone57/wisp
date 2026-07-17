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

#ifndef WISP_DESKTOP_COMPOSITOR_H_
#define WISP_DESKTOP_COMPOSITOR_H_

#include <stdbool.h>
#include <stddef.h>

#ifdef _WIN32
#include <windows.h>
#define COMPOSITOR_MUTEX CRITICAL_SECTION
#define COMPOSITOR_THREAD HANDLE
#define COMPOSITOR_COND CONDITION_VARIABLE
#define COMPOSITOR_THREAD_ID DWORD
#else
#include <pthread.h>
#define COMPOSITOR_MUTEX pthread_mutex_t
#define COMPOSITOR_THREAD pthread_t
#define COMPOSITOR_COND pthread_cond_t
#define COMPOSITOR_THREAD_ID pthread_t
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct wisp_compositor;

/**
 * GPU native API selections for accelerated compositing.
 */
typedef enum {
    WISP_COMPOSITOR_API_D3D11,
    WISP_COMPOSITOR_API_D3D12,
    WISP_COMPOSITOR_API_OPENGL_ES,
    WISP_COMPOSITOR_API_METAL,
    WISP_COMPOSITOR_API_BDIRECTWINDOW, /* Zero-latency direct frame buffer blits on Haiku */
} wisp_compositor_api;

/**
 * GPU Shared Texture containing the native handle of the graphics API.
 */
typedef struct wisp_texture {
    int width;
    int height;
    wisp_compositor_api api;
    struct wisp_compositor *compositor; /* The parent compositor instance */

    union {
        void *d3d11_texture;       /* ID3D11Texture2D* on Windows */
        void *d3d12_resource;      /* ID3D12Resource* on Windows */
        unsigned int gl_tex_id;    /* GLuint texture on Linux/Haiku */
        void *metal_texture;       /* id<MTLTexture> on macOS */
        void *direct_fb_ptr;       /* Direct pointer to screen frame buffer for Haiku software fallback */
    } handle;

    void *raw_pixels;              /* Backing store fallback (only allocated if direct GPU uploads are unavailable) */
    size_t size;
} wisp_texture_t;

/**
 * Compositor Layer representing a textured quad positioned on the screen.
 */
typedef struct wisp_layer {
    wisp_texture_t *texture;
    int x;
    int y;
    float transform[6];            /* 2D affine transform matrix */
    bool active;
} wisp_layer_t;

#define MAX_COMPOSITOR_LAYERS 64
#define TILE_CACHE_SIZE 128

/**
 * Cached Tile entry to map redraw coordinates to reusable GPU textures.
 */
typedef struct wisp_cached_tile {
    int tx;
    int ty;
    wisp_texture_t *texture;
    bool in_use;
} wisp_cached_tile_t;

/**
 * Global or per-window GPU-Accelerated Compositor context.
 */
typedef struct wisp_compositor {
    wisp_compositor_api api;

    /* Platform-neutral window/display references */
    void *native_display;
    void *native_window;

    /* Thread state for dedicated Compositor Thread */
    COMPOSITOR_THREAD thread;
    COMPOSITOR_MUTEX lock;
    COMPOSITOR_COND cond;
    bool running;
    bool shutdown_requested;
    bool frame_ready;

    /* Viewport / Scrolling transforms handled fully on GPU */
    float scroll_x;
    float scroll_y;

    /* Registered compositor layers */
    wisp_layer_t layers[MAX_COMPOSITOR_LAYERS];
    int layer_count;

    /* Tile texture cache to prevent redundant uploads/allocations and eliminate Use-After-Free bugs */
    wisp_cached_tile_t tile_cache[TILE_CACHE_SIZE];

    /* Separate context handles for safe concurrent reentrant access */
    void *ui_thread_context;          /* Owned by GTK Main Thread / BWindow (e.g. EGLContext) */
    void *compositor_thread_context;  /* Owned strictly by Wisp's Compositor Thread (e.g. EGLContext) */
    void *shared_resource_context;    /* Context used to upload/manage shared textures (e.g. EGLContext) */

    /* Dedicated Thread Identification */
    COMPOSITOR_THREAD_ID ui_thread_id;
    COMPOSITOR_THREAD_ID compositor_thread_id;

    /* Sync primitives for context/frame presentation */
    COMPOSITOR_MUTEX context_mutex;

    /* GLES2 specific fields */
    unsigned int gl_program;
    int u_transform;
    unsigned int vbo;
    void *gl_display;              /* EGLDisplay handle */
    void *gl_surface;              /* EGLSurface handle */

    /* Platform native hardware context handles (mapped dynamically) */
    struct {
        void *d3d_device;          /* ID3D11Device* / ID3D12Device* */
        void *d3d_context;         /* ID3D11DeviceContext* / ID3D12CommandQueue* */
        void *metal_device;        /* id<MTLDevice> */
        void *direct_window_info;  /* direct_buffer_info* on Haiku */
    } device_ctx;

} wisp_compositor_t;

/**
 * Create a new GPU-Accelerated Compositor context.
 *
 * \param api                  The graphics API to use for compositing.
 * \param native_window_handle Native window viewport pointer (HDC, GdkWindow, BWindow, etc.).
 * \return Pointer to initialized compositor context, or NULL on failure.
 */
wisp_compositor_t *wisp_compositor_create(wisp_compositor_api api, void *native_window_handle);

/**
 * Initialize EGL shared contexts with GTK's or Haiku's native GUI context.
 *
 * \param comp          The compositor context.
 * \param share_context Native GdkGLContext / EGLContext.
 * \return true on success, false on failure.
 */
bool wisp_compositor_initialize_egl_shared(wisp_compositor_t *comp, void *share_context);

/**
 * Destroy the compositor context and free all platform device resources.
 *
 * \param compositor The compositor context to destroy.
 */
void wisp_compositor_destroy(wisp_compositor_t *compositor);

/**
 * Spawn the dedicated Compositor Thread and start polling for frame updates.
 *
 * \param compositor The compositor context.
 * \return true on success, false on failure.
 */
bool wisp_compositor_start(wisp_compositor_t *compositor);

/**
 * Signal and join the dedicated Compositor Thread.
 *
 * \param compositor The compositor context.
 */
void wisp_compositor_stop(wisp_compositor_t *compositor);

/**
 * Create a new GPU-Shared Texture using platform-native APIs.
 *
 * \param compositor The compositor context.
 * \param width      Width of texture in pixels.
 * \param height     Height of texture in pixels.
 * \return Pointer to the allocated GPU-Shared texture, or NULL on failure.
 */
wisp_texture_t *wisp_texture_create(wisp_compositor_t *compositor, int width, int height);

/**
 * Destroy the GPU-Shared Texture and release native graphics hardware resources.
 *
 * \param tex The texture to destroy.
 */
void wisp_texture_destroy(wisp_texture_t *tex);

/**
 * Upload raw CPU-rasterized tile bitmap pixels directly to the GPU shared texture.
 *
 * \param tex    The GPU-Shared texture.
 * \param pixels Pointer to raw RGBA32 bitmap buffer.
 * \param size   Size of pixel buffer in bytes.
 * \return true on success, false on failure.
 */
bool wisp_texture_upload(wisp_texture_t *tex, const void *pixels, size_t size);

/**
 * Submit a textured layer to the compositor list to be processed on the GPU.
 *
 * \param compositor The compositor context.
 * \param tex        The GPU-Shared texture containing tile contents.
 * \param x          Target x offset in viewport.
 * \param y          Target y offset in viewport.
 * \param transform  6-element affine transform matrix to apply (scrolling, scaling, rotations).
 * \return true on success, false on failure.
 */
bool wisp_compositor_submit_texture(wisp_compositor_t *compositor, wisp_texture_t *tex, int x, int y, const float transform[6]);

/**
 * Trigger rendering of a composite frame (uploads matrix transforms and renders layers).
 *
 * \param compositor The compositor context.
 * \param scroll_x   The current scroll x offset.
 * \param scroll_y   The current scroll y offset.
 * \return true on success, false on failure.
 */
bool wisp_compositor_draw_frame(wisp_compositor_t *compositor, float scroll_x, float scroll_y);

/**
 * Obtain a cached or newly uploaded GPU shared texture for a tile.
 *
 * \param compositor The compositor context.
 * \param tx         Tile coordinate x.
 * \param ty         Tile coordinate y.
 * \param tile_size  Tile size.
 * \param pixels     Tile pixel data.
 * \return Reusable GPU shared texture pointer.
 */
wisp_texture_t *wisp_compositor_get_tile_texture(wisp_compositor_t *compositor, int tx, int ty, int tile_size, const void *pixels);

#ifdef __cplusplus
}
#endif

#endif /* WISP_DESKTOP_COMPOSITOR_H_ */
