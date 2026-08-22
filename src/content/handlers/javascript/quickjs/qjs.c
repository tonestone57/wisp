#include <quickjs.h>
#include "qjs_css.h"

#include <stdlib.h>

static JSValue wisp_qjs_noop(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    return JS_UNDEFINED;
}

/* 1. crypto.randomUUID implementation */
static JSValue js_crypto_randomUUID(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    uint8_t bytes[16];
    for (int i = 0; i < 16; i++) {
        bytes[i] = (uint8_t)(rand() & 0xFF);
    }
    // Set UUID version 4 and variant bits
    bytes[6] = (bytes[6] & 0x0F) | 0x40;
    bytes[8] = (bytes[8] & 0x3F) | 0x80;

    char uuid[37];
    snprintf(uuid, sizeof(uuid),
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             bytes[0], bytes[1], bytes[2], bytes[3],
             bytes[4], bytes[5], bytes[6], bytes[7],
             bytes[8], bytes[9], bytes[10], bytes[11],
             bytes[12], bytes[13], bytes[14], bytes[15]);

    return JS_NewString(ctx, uuid);
}

/* 2. window.matchMedia stub */
static JSValue js_window_matchMedia(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    const char *query = "";
    if (argc > 0 && !JS_IsUndefined(argv[0])) {
        query = JS_ToCString(ctx, argv[0]);
    }

    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "matches", JS_NewBool(ctx, 0));
    JS_SetPropertyStr(ctx, obj, "media", JS_NewString(ctx, query ? query : ""));
    JS_SetPropertyStr(ctx, obj, "onchange", JS_NULL);

    JSValue noop_func = JS_NewCFunction(ctx, wisp_qjs_noop, "noop", 0);
    JS_SetPropertyStr(ctx, obj, "addListener", JS_DupValue(ctx, noop_func));
    JS_SetPropertyStr(ctx, obj, "removeListener", JS_DupValue(ctx, noop_func));
    JS_SetPropertyStr(ctx, obj, "addEventListener", JS_DupValue(ctx, noop_func));
    JS_SetPropertyStr(ctx, obj, "removeEventListener", JS_DupValue(ctx, noop_func));
    JS_SetPropertyStr(ctx, obj, "dispatchEvent", JS_NewCFunction(ctx, wisp_qjs_noop, "dispatchEvent", 1));
    JS_FreeValue(ctx, noop_func);

    if (argc > 0 && !JS_IsUndefined(argv[0])) JS_FreeCString(ctx, query);
    return obj;
}

/* 3. ResizeObserver stub */
static JSValue js_ResizeObserver_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
    JSValue obj = JS_NewObject(ctx);
    JSValue noop_func = JS_NewCFunction(ctx, wisp_qjs_noop, "noop", 0);
    JS_SetPropertyStr(ctx, obj, "observe", JS_DupValue(ctx, noop_func));
    JS_SetPropertyStr(ctx, obj, "unobserve", JS_DupValue(ctx, noop_func));
    JS_SetPropertyStr(ctx, obj, "disconnect", JS_DupValue(ctx, noop_func));
    JS_FreeValue(ctx, noop_func);
    return obj;
}

/* 4. Register polyfills on the document's JSContext */
void wisp_qjs_register_core_polyfills(JSContext *ctx) {
    JSValue global = JS_GetGlobalObject(ctx);

    // Bind matchMedia
    JS_SetPropertyStr(ctx, global, "matchMedia",
                      JS_NewCFunction(ctx, js_window_matchMedia, "matchMedia", 1));

    // Bind ResizeObserver
    JSValue ro_ctor = JS_NewCFunction2(ctx, js_ResizeObserver_constructor, "ResizeObserver", 1, JS_CFUNC_constructor, 0);
    JS_SetPropertyStr(ctx, global, "ResizeObserver", ro_ctor);

    // Bind crypto.randomUUID
    JSValue crypto = JS_GetPropertyStr(ctx, global, "crypto");
    if (JS_IsUndefined(crypto) || JS_IsNull(crypto)) {
        crypto = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, global, "crypto", JS_DupValue(ctx, crypto));
    }
    JS_SetPropertyStr(ctx, crypto, "randomUUID",
                      JS_NewCFunction(ctx, js_crypto_randomUUID, "randomUUID", 0));
    JS_FreeValue(ctx, crypto);

    // Bind CSS.escape
    JSValue css_obj = JS_GetPropertyStr(ctx, global, "CSS");
    if (JS_IsUndefined(css_obj) || JS_IsNull(css_obj)) {
        css_obj = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, global, "CSS", JS_DupValue(ctx, css_obj));
    }
    JS_SetPropertyStr(ctx, css_obj, "escape",
                      JS_NewCFunction(ctx, js_css_escape, "escape", 1));
    JS_FreeValue(ctx, css_obj);

    // Try to bind window/self aliases if missing, but be careful not to overwrite the real window if it's an exotic object.
    JSValue window_val = JS_GetPropertyStr(ctx, global, "window");
    if (JS_IsUndefined(window_val)) {
        JS_SetPropertyStr(ctx, global, "window", JS_DupValue(ctx, global));
    } else {
        // If window already exists, mirror properties to it.
        JS_SetPropertyStr(ctx, window_val, "matchMedia", JS_NewCFunction(ctx, js_window_matchMedia, "matchMedia", 1));
        JSValue ro_ctor_window = JS_NewCFunction2(ctx, js_ResizeObserver_constructor, "ResizeObserver", 1, JS_CFUNC_constructor, 0);
        JS_SetPropertyStr(ctx, window_val, "ResizeObserver", ro_ctor_window);

        JSValue window_crypto = JS_GetPropertyStr(ctx, window_val, "crypto");
        if (JS_IsUndefined(window_crypto) || JS_IsNull(window_crypto)) {
            window_crypto = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, window_val, "crypto", JS_DupValue(ctx, window_crypto));
        }
        JS_SetPropertyStr(ctx, window_crypto, "randomUUID", JS_NewCFunction(ctx, js_crypto_randomUUID, "randomUUID", 0));
        JS_FreeValue(ctx, window_crypto);

        JSValue window_css = JS_GetPropertyStr(ctx, window_val, "CSS");
        if (JS_IsUndefined(window_css) || JS_IsNull(window_css)) {
            window_css = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, window_val, "CSS", JS_DupValue(ctx, window_css));
        }
        JS_SetPropertyStr(ctx, window_css, "escape", JS_NewCFunction(ctx, js_css_escape, "escape", 1));
        JS_FreeValue(ctx, window_css);

    }
    JS_FreeValue(ctx, window_val);

    JSValue globalThis_val = JS_GetPropertyStr(ctx, global, "globalThis");
    if (JS_IsUndefined(globalThis_val)) {
        JS_SetPropertyStr(ctx, global, "globalThis", JS_DupValue(ctx, global));
    }
    JS_FreeValue(ctx, globalThis_val);

    JS_FreeValue(ctx, global);


}

/*
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * Copyright 2026 Wisp
 *
 * NeoSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wisp/utils/utf8proc_wrapper.h>
#include <wisp/content/csp.h>

#include <wisp/utils/errors.h>
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "utils/corestrings.h"
#include "quickjs.h"
#include "utils/hashmap.h"
#include "content/handlers/javascript/js.h"
#include "qjs_internal.h"
#include "JSEvent.gen.h"
#include "wisp_subsystem.h"
#include "crypto.h"
#include "dom_bridge.h"

dom_string *g_qjs_node_key = NULL;

#include <nsutils/time.h>
#include <wisp/misc.h>
#include <wisp/content/handlers/html/box_inspect.h>
#include <wisp/content/handlers/html/box.h>
#include <wisp/content/handlers/html/private.h>
#include <wisp/content/handlers/html/html.h>
#include <wisp/content.h>
#include <wisp/content/hlcache.h>
#include <wisp/utils/nsoption.h>
#include <math.h>
#include "impl/observer_internal.h"
#include <wisp/desktop/gui_table.h>
#include <wisp/utils/ipc.h>
#include <wisp/utils/nsurl.h>
#include <wisp/browser_window.h>
#include "desktop/browser_private.h"

#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <openssl/evp.h>
#include <curl/curl.h>
#include "quickjs-libc.h"

#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#endif

static void compute_sha256(const uint8_t *data, size_t len, char *hex_out, size_t hex_out_len);
static char *wisp_read_local_file(const char *filename, size_t *out_len);

struct wisp_curl_buffer {
    char *data;
    size_t size;
};

extern void (*wisp_gui_pump_events_hook)(void);

static size_t wisp_curl_write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct wisp_curl_buffer *mem = (struct wisp_curl_buffer *)userp;

    char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if (!ptr) {
        return 0; /* out of memory! */
    }

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;

    return realsize;
}

static char *wisp_sync_fetch(const char *url, size_t *out_len)
{
    if (!url)
        return NULL;

    char hex[65];
    compute_sha256((const uint8_t *)url, strlen(url), hex, sizeof(hex));

    char cache_dir[] = "/tmp/wisp-module-cache";
    char cache_path[256];
    char tmp_path[256];
    snprintf(cache_path, sizeof(cache_path), "%s/%s.js", cache_dir, hex);
    snprintf(tmp_path, sizeof(tmp_path), "%s/%s.js.tmp", cache_dir, hex);

#ifdef _WIN32
    _mkdir(cache_dir);
#else
    mkdir(cache_dir, 0700);
#endif

    char *cached_buf = wisp_read_local_file(cache_path, out_len);
    if (cached_buf) {
        return cached_buf;
    }

    CURL *curl_handle;
    CURLcode res = CURLE_OK;
    struct wisp_curl_buffer chunk;
    long response_code = 0;
    int max_retries = 5;
    int attempt;
    bool success = false;
    int delay_ms = 150;

    for (attempt = 1; attempt <= max_retries; attempt++) {
        chunk.data = malloc(1); /* will be grown as needed by the realloc above */
        if (!chunk.data)
            return NULL;
        chunk.size = 0; /* no data at this point */

        curl_handle = curl_easy_init();
        if (!curl_handle) {
            free(chunk.data);
            return NULL;
        }

        curl_easy_setopt(curl_handle, CURLOPT_URL, url);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, wisp_curl_write_callback);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 2L);
        // Extend timeouts for synchronous fetches under debug or heavy loads
        curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 15L);
        curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 45L);

        CURLM *multi_handle = curl_multi_init();
        if (multi_handle) {
            curl_multi_add_handle(multi_handle, curl_handle);
            int still_running = 1;

            while (still_running) {
                CURLMcode mres = curl_multi_perform(multi_handle, &still_running);
                if (mres != CURLM_OK) {
                    res = CURLE_RECV_ERROR;
                    break;
                }

                if (still_running) {
                    // Pump host UI/IPC events to avoid starvation
                    if (wisp_gui_pump_events_hook) {
                        wisp_gui_pump_events_hook();
                    }
                    int numfds;
#if LIBCURL_VERSION_NUM >= 0x074400 // 7.68.0
                    curl_multi_poll(multi_handle, NULL, 0, 10, &numfds);
#else
                    curl_multi_wait(multi_handle, NULL, 0, 10, &numfds);
#endif
                }
            }

            // Extract actual easy transfer result
            CURLMsg *msg;
            int msgs_left;
            while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
                if (msg->msg == CURLMSG_DONE && msg->easy_handle == curl_handle) {
                    res = msg->data.result; // Updates 'res' with CURLE_OK or error code
                }
            }

            curl_multi_remove_handle(multi_handle, curl_handle);
            curl_multi_cleanup(multi_handle);
        } else {
            res = curl_easy_perform(curl_handle);
        }

        bool should_retry = false;

        if (res == CURLE_OK) {
            curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
            if (response_code >= 200 && response_code < 300) {
                success = true;
                curl_easy_cleanup(curl_handle);
                break;
            } else {
                fprintf(stderr, "WISP_SYNC_FETCH: HTTP error %ld for %s (attempt %d/%d)\n", response_code, url, attempt,
                    max_retries);
                if (response_code == 429 || response_code >= 500) {
                    should_retry = true;
                } else {
                    // 404, 403, 401 etc. are non-retryable
                    should_retry = false;
                }
            }
        } else {
            fprintf(stderr, "WISP_SYNC_FETCH: Curl error %s for %s (attempt %d/%d)\n", curl_easy_strerror(res), url,
                attempt, max_retries);
            should_retry = true;
        }

        curl_easy_cleanup(curl_handle);
        free(chunk.data);
        chunk.data = NULL;

        if (!should_retry || attempt == max_retries) {
            break;
        }

        usleep(delay_ms * 1000);
        delay_ms *= 2; // Exponential backoff
    }

    if (!success) {
        return NULL;
    }

    FILE *f = fopen(tmp_path, "wb");
    if (f) {
        fwrite(chunk.data, 1, chunk.size, f);
        fclose(f);
        if (rename(tmp_path, cache_path) != 0) {
            fprintf(
                stderr, "WISP_SYNC_FETCH: Failed to rename atomic cache file from %s to %s\n", tmp_path, cache_path);
            unlink(tmp_path);
            free(chunk.data);
            return NULL;
        }
    } else {
        free(chunk.data);
        return NULL;
    }

    if (out_len) {
        *out_len = chunk.size;
    }
    return chunk.data;
}

static char *wisp_read_local_file(const char *filename, size_t *out_len)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) {
        fclose(f);
        return NULL;
    }
    char *buf = malloc(sz + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    size_t read_bytes = fread(buf, 1, sz, f);
    buf[read_bytes] = '\0';
    fclose(f);
    if (out_len) {
        *out_len = read_bytes;
    }
    return buf;
}

static char *wisp_load_module_source(const char *module_name, size_t *out_len)
{
    if (strncmp(module_name, "http://", 7) == 0 || strncmp(module_name, "https://", 8) == 0) {
        return wisp_sync_fetch(module_name, out_len);
    } else if (strncmp(module_name, "//", 2) == 0) {
        char url_buf[1024];
        snprintf(url_buf, sizeof(url_buf), "https:%s", module_name);
        return wisp_sync_fetch(url_buf, out_len);
    } else if (strncmp(module_name, "file://", 7) == 0) {
        return wisp_read_local_file(module_name + 7, out_len);
    } else {
        return wisp_read_local_file(module_name, out_len);
    }
}

char *wisp_module_normalize(JSContext *ctx, const char *base_name, const char *name, void *opaque)
{
    if (!name)
        return NULL;

    // If name is an absolute URL or path, just return it
    if (strncmp(name, "http://", 7) == 0 || strncmp(name, "https://", 8) == 0 || strncmp(name, "file://", 7) == 0) {
        return js_strdup(ctx, name);
    }

    // Check if base_name is a URL
    const char *scheme_sep = strstr(base_name, "://");
    if (scheme_sep) {
        size_t scheme_len = (scheme_sep - base_name) + 3; // e.g. "https://"
        const char *host_start = base_name + scheme_len;
        const char *path_start = strchr(host_start, '/');

        char origin[512];
        if (path_start) {
            size_t origin_len = path_start - base_name;
            if (origin_len >= sizeof(origin))
                origin_len = sizeof(origin) - 1;
            memcpy(origin, base_name, origin_len);
            origin[origin_len] = '\0';
        } else {
            strncpy(origin, base_name, sizeof(origin) - 1);
            origin[sizeof(origin) - 1] = '\0';
            path_start = "/";
        }

        // If name is absolute path relative to host
        if (name[0] == '/') {
            char buf[1024];
            snprintf(buf, sizeof(buf), "%s%s", origin, name);
            return js_strdup(ctx, buf);
        }

        // It's a relative path. Resolve against base_name's path
        char path_buf[1024];
        strncpy(path_buf, path_start, sizeof(path_buf) - 1);
        path_buf[sizeof(path_buf) - 1] = '\0';

        // Get directory of path_buf (up to last '/')
        char *last_slash = strrchr(path_buf, '/');
        if (last_slash) {
            *(last_slash + 1) = '\0';
        } else {
            path_buf[0] = '/';
            path_buf[1] = '\0';
        }

        // Process leading ./ and ../
        const char *r = name;
        while (1) {
            if (strncmp(r, "./", 2) == 0) {
                r += 2;
            } else if (strncmp(r, "../", 3) == 0) {
                // Pop last directory
                size_t len = strlen(path_buf);
                if (len > 1) {
                    if (path_buf[len - 1] == '/') {
                        path_buf[len - 1] = '\0';
                        len--;
                    }
                    char *prev_slash = strrchr(path_buf, '/');
                    if (prev_slash) {
                        *(prev_slash + 1) = '\0';
                    } else {
                        path_buf[0] = '/';
                        path_buf[1] = '\0';
                    }
                }
                r += 3;
            } else {
                break;
            }
        }

        char buf[2048];
        size_t path_len = strlen(path_buf);
        if (path_len > 0 && path_buf[path_len - 1] != '/' && r[0] != '/') {
            snprintf(buf, sizeof(buf), "%s%s/%s", origin, path_buf, r);
        } else {
            snprintf(buf, sizeof(buf), "%s%s%s", origin, path_buf, r);
        }
        return js_strdup(ctx, buf);
    }

    // Default path normalizer fallback
    char *filename, *p;
    const char *r;
    int cap;
    int len;

    if (name[0] != '.') {
        return js_strdup(ctx, name);
    }

    r = strrchr(base_name, '/');
    if (r)
        len = r - base_name;
    else
        len = 0;

    cap = len + strlen(name) + 1 + 1;
    filename = js_malloc(ctx, cap);
    if (!filename)
        return NULL;
    memcpy(filename, base_name, len);
    filename[len] = '\0';

    r = name;
    for (;;) {
        if (r[0] == '.' && r[1] == '/') {
            r += 2;
        } else if (r[0] == '.' && r[1] == '.' && r[2] == '/') {
            if (filename[0] == '\0')
                break;
            p = strrchr(filename, '/');
            if (!p)
                p = filename;
            else
                p++;
            if (!strcmp(p, ".") || !strcmp(p, ".."))
                break;
            if (p > filename)
                p--;
            *p = '\0';
            r += 3;
        } else {
            break;
        }
    }
    if (filename[0] != '\0') {
        size_t flen = strlen(filename);
        if (flen + 2 <= cap) {
            filename[flen] = '/';
            filename[flen + 1] = '\0';
        }
    }
    size_t flen = strlen(filename);
    if (flen < (size_t)cap) {
        snprintf(filename + flen, cap - flen, "%s", r);
    }
    return filename;
}

JSModuleDef *wisp_module_loader(JSContext *ctx, const char *module_name, void *opaque)
{
    fprintf(stderr, "WISP_MODULE_LOADER: Loading module '%s'\n", module_name);
    size_t buf_len = 0;
    char *buf = wisp_load_module_source(module_name, &buf_len);
    if (!buf) {
        fprintf(stderr, "WISP_MODULE_LOADER: Failed to load module source for '%s'\n", module_name);
        JS_ThrowReferenceError(ctx, "could not load module '%s'", module_name);
        return NULL;
    }
    fprintf(stderr, "WISP_MODULE_LOADER: Compiling module '%s' (%zu bytes)\n", module_name, buf_len);
    JSValue val = JS_Eval(ctx, buf, buf_len, module_name, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    free(buf);

    if (JS_IsException(val)) {
        JSValue exc = JS_GetException(ctx);
        const char *exc_str = JS_ToCString(ctx, exc);
        fprintf(stderr, "WISP_MODULE_LOADER: Compilation failed for '%s': %s\n", module_name,
            exc_str ? exc_str : "unknown");
        if (exc_str)
            JS_FreeCString(ctx, exc_str);
        JS_FreeValue(ctx, exc);
        return NULL;
    }

    JSModuleDef *m = JS_VALUE_GET_PTR(val);
    js_module_set_import_meta(ctx, val, false, false);
    JS_FreeValue(ctx, val);
    fprintf(stderr, "WISP_MODULE_LOADER: Successfully loaded module '%s'\n", module_name);
    return m;
}

bool wisp_is_js_process = false;
shm_dom_t *wisp_shm_dom = NULL;
uint32_t wisp_shm_capacity = 0;

extern void (*wisp_node_destroy_cb)(void *node);
static jsheap *global_heaps_list = NULL;
static pthread_mutex_t global_heaps_mutex = PTHREAD_MUTEX_INITIALIZER;

static __thread shm_dom_t *current_thread_shm = NULL;
static __thread bool thread_shm_locked = false;

void qjs_on_node_destroy(void *node) {
    if (!node) return;
    dom_node_type type = 0;
    dom_node_get_node_type((dom_node *)node, &type);
    bool is_doc = (type == DOM_DOCUMENT_NODE);

    pthread_mutex_lock(&global_heaps_mutex);
    jsheap *heap = global_heaps_list;
    while (heap) {
        struct jsthread *t = heap->threads;
        while (t) {
            shm_dom_t *shm = t->shm_dom;
            if (shm) {
                bool already_locked = (shm == current_thread_shm && thread_shm_locked);
                if (!already_locked) {
                    shm_dom_lock_write(shm);
                }
                for (uint32_t i = 1; i < shm->node_count; i++) {
                    uintptr_t entry_ptr = (uintptr_t)shm_dom_get_dom_ptrs(shm)[i];
                    if (entry_ptr == (uintptr_t)node) {
                        shm_dom_get_dom_ptrs(shm)[i] = 0;
                    } else if (is_doc && entry_ptr != 0 && (entry_ptr % sizeof(void *)) == 0) {
                        dom_node *e = (dom_node *)entry_ptr;
                        if (!e->vtable) {
                            shm_dom_get_dom_ptrs(shm)[i] = 0;
                            continue;
                        }
                        struct dom_document *owner = NULL;
                        dom_node_get_owner_document(e, &owner);
                        if (owner == (struct dom_document *)node) {
                            shm_dom_get_dom_ptrs(shm)[i] = 0;
                            /* Decrement refcnt directly to balance the ref added by
                             * dom_node_get_owner_document, bypassing the destructor call
                             * to avoid recursive infinite destruction loops. */
                            ((struct dom_node *)owner)->refcnt--;
                        } else if (owner != NULL) {
                            dom_node_unref((dom_node *)owner);
                        }
                    }
                }
                if (!already_locked) {
                    shm_dom_unlock_write(shm);
                }
            }
            t = t->next_in_heap;
        }
        heap = heap->next_in_global;
    }
    pthread_mutex_unlock(&global_heaps_mutex);
}

static void compute_sha256(const uint8_t *data, size_t len, char *hex_out, size_t hex_out_len) {
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    const EVP_MD *md = EVP_sha256();
    uint8_t hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;

    if (mdctx && md) {
        EVP_DigestInit_ex(mdctx, md, NULL);
        EVP_DigestUpdate(mdctx, data, len);
        EVP_DigestFinal_ex(mdctx, hash, &hash_len);
    }
    if (mdctx) {
        EVP_MD_CTX_free(mdctx);
    }

    for (unsigned int i = 0; i < hash_len; i++) {
        if ((i * 2) < hex_out_len) {
            snprintf(hex_out + (i * 2), hex_out_len - (i * 2), "%02x", hash[i]);
        }
    }
    if (hex_out_len > 0) {
        hex_out[(hash_len * 2) < hex_out_len ? (hash_len * 2) : (hex_out_len - 1)] = '\0';
    }
}

JSValue js_eval_with_aot_cache(JSContext *ctx, const uint8_t *txt, size_t txtlen, const char *name, int eval_flags)
{
    if (!txt || txtlen == 0) {
        return JS_Eval(ctx, (const char *)txt, txtlen, name, eval_flags);
    }

    char hex[65];
    compute_sha256(txt, txtlen, hex, sizeof(hex));

    char cache_dir[] = "/tmp/wisp-bytecode-cache";
    char cache_path[256];
    bool is_module = (eval_flags & JS_EVAL_TYPE_MASK) == JS_EVAL_TYPE_MODULE;
    snprintf(cache_path, sizeof(cache_path), "%s/%s%s.bin", cache_dir, hex, is_module ? "_module" : "");

#ifdef _WIN32
    _mkdir(cache_dir);
#else
    mkdir(cache_dir, 0700);
#endif

    FILE *f = fopen(cache_path, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);

        if (sz > 0) {
            uint8_t *buf = malloc(sz);
            if (buf) {
                if (fread(buf, 1, sz, f) == (size_t)sz) {
                    fclose(f);
                    f = NULL;

                    JSValue obj = JS_ReadObject(ctx, buf, sz, JS_READ_OBJ_BYTECODE);
                    free(buf);

                    if (!JS_IsException(obj)) {
                        if (JS_IsModule(obj)) {
                            if (JS_ResolveModule(ctx, obj) < 0) {
                                JS_FreeValue(ctx, obj);
                                unlink(cache_path);
                                return JS_EXCEPTION;
                            }
                        }
                        JSValue res = JS_EvalFunction(ctx, obj);
                        return res;
                    } else {
                        unlink(cache_path);
                    }
                } else {
                    free(buf);
                }
            }
        }
        if (f)
            fclose(f);
    }

    char *txt_null_term = malloc(txtlen + 1);
    if (!txt_null_term)
        return JS_ThrowOutOfMemory(ctx);
    memcpy(txt_null_term, txt, txtlen);
    txt_null_term[txtlen] = '\0';

    JSValue compiled = JS_Eval(ctx, (const char *)txt_null_term, txtlen, name, eval_flags | JS_EVAL_FLAG_COMPILE_ONLY);
    free(txt_null_term);

    if (JS_IsException(compiled)) {
        return compiled;
    }

    size_t bytecode_size = 0;
    uint8_t *bytecode = JS_WriteObject(ctx, &bytecode_size, compiled, JS_WRITE_OBJ_BYTECODE);
    if (bytecode && bytecode_size > 0) {
        f = fopen(cache_path, "wb");
        if (f) {
            fwrite(bytecode, 1, bytecode_size, f);
            fclose(f);
        }
    }
    if (bytecode) {
        js_free(ctx, bytecode);
    }

    JSValue res = JS_EvalFunction(ctx, compiled);
    return res;
}

struct precompile_arg {
    uint8_t *txt;
    size_t txtlen;
};

static void do_precompile(void *arg)
{
    struct precompile_arg *pa = arg;
    if (!pa)
        return;

    if (pa->txt && pa->txtlen > 0) {
        char hex[65];
        compute_sha256(pa->txt, pa->txtlen, hex, sizeof(hex));

        char cache_dir[] = "/tmp/wisp-bytecode-cache";
        char cache_path[256];
        snprintf(cache_path, sizeof(cache_path), "%s/%s.bin", cache_dir, hex);

        struct stat st;
        if (stat(cache_path, &st) != 0) {
            // File does not exist, compile it!
            JSRuntime *rt = JS_NewRuntime();
            if (rt) {
                JSContext *ctx = JS_NewContext(rt);
                if (ctx) {
                    char *txt_null_term = malloc(pa->txtlen + 1);
                    if (txt_null_term) {
                        memcpy(txt_null_term, pa->txt, pa->txtlen);
                        txt_null_term[pa->txtlen] = '\0';

                        JSValue compiled = JS_Eval(ctx, txt_null_term, pa->txtlen, "<precompile>",
                            JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);
                        free(txt_null_term);

                        if (!JS_IsException(compiled)) {
                            size_t bytecode_size = 0;
                            uint8_t *bytecode = JS_WriteObject(ctx, &bytecode_size, compiled, JS_WRITE_OBJ_BYTECODE);
                            if (bytecode && bytecode_size > 0) {
#ifdef _WIN32
                                _mkdir(cache_dir);
#else
                                mkdir(cache_dir, 0700);
#endif
                                FILE *f = fopen(cache_path, "wb");
                                if (f) {
                                    fwrite(bytecode, 1, bytecode_size, f);
                                    fclose(f);
                                }
                            }
                            if (bytecode) {
                                js_free(ctx, bytecode);
                            }
                            JS_FreeValue(ctx, compiled);
                        }
                    }
                    JS_FreeContext(ctx);
                }
                JS_FreeRuntime(rt);
            }
        }
    }

    if (pa->txt)
        free(pa->txt);
    free(pa);
}

void wisp_queue_precompile(const uint8_t *txt, size_t txtlen)
{
    if (!txt || txtlen == 0)
        return;

    struct precompile_arg *pa = malloc(sizeof(*pa));
    if (!pa)
        return;
    pa->txt = malloc(txtlen);
    if (!pa->txt) {
        free(pa);
        return;
    }
    memcpy(pa->txt, txt, txtlen);
    pa->txtlen = txtlen;

    if (!wisp_dispatch_js(NULL, do_precompile, pa, 0.5f)) {
        do_precompile(pa);
    }
}

extern struct wisp_table *guit;

struct origin_js_process {
    char origin[256];
    wisp_ipc_handle *ipc_handle;
    pid_t pid;
    char ipc_dir[256];
    unsigned int ref_count;
    struct origin_js_process *next;
};

static struct origin_js_process *js_processes = NULL;
static pthread_mutex_t js_processes_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint32_t js_process_counter = 0;
static uint32_t null_origin_counter = 0;

struct content;
struct nsurl *content_get_url(struct content *c);

static bool create_secure_ipc_path(char *ipc_path_buf, size_t path_len, char *dir_buf, size_t dir_len)
{
    char template[] = "/tmp/wisp-js-XXXXXX";
    char *dir_name = mkdtemp(template);
    if (!dir_name) {
        return false;
    }
    strncpy(dir_buf, dir_name, dir_len - 1);
    dir_buf[dir_len - 1] = '\0';

    snprintf(ipc_path_buf, path_len, "%s/ipc", dir_name);
    return true;
}

static void resolve_origin_from_content(void *win_priv, void *doc_priv, char *origin_buf, size_t buf_len)
{
    if (!doc_priv || win_priv == doc_priv) {
        pthread_mutex_lock(&js_processes_mutex);
        uint32_t val = ++null_origin_counter;
        pthread_mutex_unlock(&js_processes_mutex);
        snprintf(origin_buf, buf_len, "null-origin-%u", val);
        return;
    }

    struct nsurl *url = content_get_url((struct content *)doc_priv);
    if (!url) {
        pthread_mutex_lock(&js_processes_mutex);
        uint32_t val = ++null_origin_counter;
        pthread_mutex_unlock(&js_processes_mutex);
        snprintf(origin_buf, buf_len, "null-origin-%u", val);
        return;
    }

    lwc_string *scheme = nsurl_get_component(url, NSURL_SCHEME);
    lwc_string *host = nsurl_get_component(url, NSURL_HOST);
    lwc_string *port = nsurl_get_component(url, NSURL_PORT);

    if (scheme && host) {
        if (port) {
            snprintf(origin_buf, buf_len, "%s://%s:%s", lwc_string_data(scheme), lwc_string_data(host),
                lwc_string_data(port));
        } else {
            snprintf(origin_buf, buf_len, "%s://%s", lwc_string_data(scheme), lwc_string_data(host));
        }
    } else {
        pthread_mutex_lock(&js_processes_mutex);
        uint32_t val = ++null_origin_counter;
        pthread_mutex_unlock(&js_processes_mutex);
        snprintf(origin_buf, buf_len, "null-origin-%u", val);
    }

    if (scheme)
        lwc_string_unref(scheme);
    if (host)
        lwc_string_unref(host);
    if (port)
        lwc_string_unref(port);

    /* Enforce COOP isolation if option is enabled and same-origin COOP is declared */
    if (nsoption_bool(enable_coop)) {
        struct html_content *htmlc = (struct html_content *)doc_priv;
        if (htmlc && htmlc->coop && (strcasecmp(htmlc->coop, "same-origin") == 0)) {
            strncat(origin_buf, "-coop-isolated", buf_len - strlen(origin_buf) - 1);
        }
    }
}

static wisp_ipc_handle *ensure_js_process_for_origin(const char *origin)
{
    if (!origin)
        return NULL;
    if (strncmp(origin, "null-origin-", 12) == 0 || strncmp(origin, "null-worker-", 12) == 0) {
        return NULL;
    }
    pthread_mutex_lock(&js_processes_mutex);
    struct origin_js_process *curr = js_processes;
    while (curr) {
        if (wisp_simd_streq(curr->origin, origin)) {
            curr->ref_count++;
            wisp_ipc_handle *h = curr->ipc_handle;
            pthread_mutex_unlock(&js_processes_mutex);
            return h;
        }
        curr = curr->next;
    }

    char exec_path[256];
    if (!wisp_ipc_find_executable("wisp-js", exec_path, sizeof(exec_path))) {
        pthread_mutex_unlock(&js_processes_mutex);
        return NULL;
    }

    char ipc_path[256];
    char ipc_dir[256];
    if (!create_secure_ipc_path(ipc_path, sizeof(ipc_path), ipc_dir, sizeof(ipc_dir))) {
        pthread_mutex_unlock(&js_processes_mutex);
        return NULL;
    }

    wisp_ipc_handle *server = wisp_ipc_create_server(ipc_path);
    if (!server) {
        rmdir(ipc_dir);
        pthread_mutex_unlock(&js_processes_mutex);
        return NULL;
    }

    pid_t pid = wisp_ipc_spawn(exec_path, ipc_path);
    if (pid < 0) {
        wisp_ipc_destroy(server);
        unlink(ipc_path);
        rmdir(ipc_dir);
        pthread_mutex_unlock(&js_processes_mutex);
        return NULL;
    }

    wisp_ipc_handle *client = wisp_ipc_accept(server);
    wisp_ipc_destroy(server);

    if (!client) {
        unlink(ipc_path);
        rmdir(ipc_dir);
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        pthread_mutex_unlock(&js_processes_mutex);
        return NULL;
    }

    struct origin_js_process *node = malloc(sizeof(*node));
    if (!node) {
        wisp_ipc_destroy(client);
        unlink(ipc_path);
        rmdir(ipc_dir);
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        pthread_mutex_unlock(&js_processes_mutex);
        return NULL;
    }

    strncpy(node->origin, origin, sizeof(node->origin) - 1);
    node->origin[sizeof(node->origin) - 1] = '\0';
    node->ipc_handle = client;
    node->pid = pid;
    strncpy(node->ipc_dir, ipc_dir, sizeof(node->ipc_dir) - 1);
    node->ipc_dir[sizeof(node->ipc_dir) - 1] = '\0';
    node->ref_count = 1;
    node->next = js_processes;
    js_processes = node;

    pthread_mutex_unlock(&js_processes_mutex);
    return client;
}

static void release_js_process_for_origin(const char *origin)
{
    if (!origin)
        return;
    pthread_mutex_lock(&js_processes_mutex);
    struct origin_js_process **prev = &js_processes;
    struct origin_js_process *curr = js_processes;
    while (curr) {
        if (wisp_simd_streq(curr->origin, origin)) {
            curr->ref_count--;
            if (curr->ref_count == 0) {
                *prev = curr->next;
                wisp_ipc_destroy(curr->ipc_handle);

                char ipc_path[512];
                snprintf(ipc_path, sizeof(ipc_path), "%s/ipc", curr->ipc_dir);
                unlink(ipc_path);
                rmdir(curr->ipc_dir);

                kill(curr->pid, SIGTERM);
                int status;
                int retries = 50;
                while (retries-- > 0) {
                    if (waitpid(curr->pid, &status, WNOHANG) > 0) {
                        break;
                    }
                    usleep(10000);
                }
                if (retries <= 0) {
                    kill(curr->pid, SIGKILL);
                    waitpid(curr->pid, NULL, 0);
                }

                free(curr);
            }
            break;
        }
        prev = &curr->next;
        curr = curr->next;
    }
    pthread_mutex_unlock(&js_processes_mutex);
}

static wisp_ipc_handle *get_js_process_handle(const char *origin)
{
    if (!origin)
        return NULL;
    pthread_mutex_lock(&js_processes_mutex);
    struct origin_js_process *curr = js_processes;
    while (curr) {
        if (wisp_simd_streq(curr->origin, origin)) {
            wisp_ipc_handle *h = curr->ipc_handle;
            pthread_mutex_unlock(&js_processes_mutex);
            return h;
        }
        curr = curr->next;
    }
    pthread_mutex_unlock(&js_processes_mutex);
    return NULL;
}

static void handle_process_crash(const char *origin)
{
    if (!origin)
        return;
    pthread_mutex_lock(&js_processes_mutex);
    struct origin_js_process **prev = &js_processes;
    struct origin_js_process *curr = js_processes;
    while (curr) {
        if (wisp_simd_streq(curr->origin, origin)) {
            *prev = curr->next;
            wisp_ipc_destroy(curr->ipc_handle);

            char ipc_path[512];
            snprintf(ipc_path, sizeof(ipc_path), "%s/ipc", curr->ipc_dir);
            unlink(ipc_path);
            rmdir(curr->ipc_dir);

            int status = 0;
            if (waitpid(curr->pid, &status, WNOHANG) == 0) {
                kill(curr->pid, SIGKILL);
                waitpid(curr->pid, &status, 0);
            }

            if (WIFEXITED(status)) {
                NSLOG(wisp, WARNING, "JS process exited with code %d", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                NSLOG(wisp, WARNING, "JS process terminated by signal %d", WTERMSIG(status));
            }

            free(curr);
            break;
        }
        prev = &curr->next;
        curr = curr->next;
    }
    pthread_mutex_unlock(&js_processes_mutex);
}
void qjs_timer_callback(void *p);

#ifdef _WIN32
#include <windows.h>
#endif

void *qjs_get_window_priv(JSContext *ctx)
{
    struct jsthread *t = JS_GetContextOpaque(ctx);
    return t ? t->win_priv : NULL;
}

void *qjs_get_document_priv(JSContext *ctx)
{
    struct jsthread *t = JS_GetContextOpaque(ctx);
    return t ? t->doc_priv : NULL;
}

struct dom_document *qjs_thread_get_document(struct jsthread *t)
{
    if (!t || !t->doc_priv)
        return NULL;
    if (wisp_is_js_process) {
        return (struct dom_document *)t->doc_priv;
    }
    if (t->win_priv && t->win_priv != t->doc_priv) {
        struct html_content *htmlc = (struct html_content *)t->doc_priv;
        return (struct dom_document *)htmlc->document;
    }
    return (struct dom_document *)t->doc_priv;
}

extern void (*wisp_dom_node_destroy_hook)(void *node);

static void on_dom_node_destroy(void *node) {
    if (!node) return;
    shm_dom_t *shm = current_thread_shm;
    if (shm) {
        bool already_locked = thread_shm_locked;
        if (!already_locked) {
            shm_dom_lock_write(shm);
        }
        for (uint32_t j = 1; j < shm->node_count; j++) {
            if (shm_dom_get_dom_ptrs(shm)[j] == (uint64_t)(uintptr_t)node) {
                shm_dom_get_dom_ptrs(shm)[j] = 0;
            }
        }
        if (!already_locked) {
            shm_dom_unlock_write(shm);
        }
    }
}

void js_initialise(void)
{
    if (!g_qjs_node_key) dom_string_create((const uint8_t *)"__qjs_node", 10, &g_qjs_node_key);

    wisp_dom_node_destroy_hook = on_dom_node_destroy;
    wisp_node_destroy_cb = qjs_on_node_destroy;
}

void (*wisp_gui_pump_events_hook)(void) = NULL;

static int qjs_interrupt_handler(JSRuntime *rt, void *opaque)
{
    struct jsheap *heap = opaque;
    uint64_t now;
    nsu_getmonotonic_ms(&now);

    // 1. Hard Timeout Guard (e.g., 3 seconds max execution)
    if (heap->deadline_ms > 0 && (now > heap->deadline_ms)) {
        NSLOG(wisp, WARNING, "JS execution timed out, aborting script.");
        return 1; // Terminates QuickJS execution
    }

    // 2. Time-Slicing: Yield to OS/UI event loop every 16ms
    if (heap->last_yield_ms > 0 && (now > heap->last_yield_ms + 16)) {
        heap->last_yield_ms = now;
        if (wisp_gui_pump_events_hook) {
            wisp_gui_pump_events_hook();
        }
    }

    return 0; // Continue execution
}

void js_finalise(void)
{
    wisp_dom_node_destroy_hook = NULL;
    wisp_node_destroy_cb = NULL;
    pthread_mutex_lock(&js_processes_mutex);
    struct origin_js_process *curr = js_processes;
    while (curr) {
        struct origin_js_process *next = curr->next;
        wisp_ipc_destroy(curr->ipc_handle);
        char ipc_path[512];
        snprintf(ipc_path, sizeof(ipc_path), "%s/ipc", curr->ipc_dir);
        unlink(ipc_path);
        rmdir(curr->ipc_dir);
        kill(curr->pid, SIGKILL);
        waitpid(curr->pid, NULL, 0);
        free(curr);
        curr = next;
    }
    js_processes = NULL;
    pthread_mutex_unlock(&js_processes_mutex);
}

nserror js_newheap(int timeout, jsheap **heap)
{
    jsheap *h = calloc(1, sizeof(*h));
    if (!h)
        return NSERROR_NOMEM;
    h->rt = JS_NewRuntime();
    if (!h->rt) {
        free(h);
        return NSERROR_NOMEM;
    }
    h->timeout = timeout;
    JS_SetMemoryLimit(h->rt, 128 * 1024 * 1024); // Increased to 128MB
    JS_SetMaxStackSize(h->rt, 16384 * 1024); // Increased to 16MB
    JS_SetModuleLoaderFunc(h->rt, wisp_module_normalize, wisp_module_loader, NULL);
    JS_SetInterruptHandler(h->rt, qjs_interrupt_handler, h);

    pthread_mutex_lock(&global_heaps_mutex);
    h->next_in_global = global_heaps_list;
    global_heaps_list = h;
    pthread_mutex_unlock(&global_heaps_mutex);

    *heap = h;
    return NSERROR_OK;
}

void js_destroyheap(jsheap *heap)
{
    if (!heap)
        return;

    pthread_mutex_lock(&global_heaps_mutex);
    jsheap **curr_h = &global_heaps_list;
    while (*curr_h) {
        if (*curr_h == heap) {
            *curr_h = heap->next_in_global;
            break;
        }
        curr_h = &((*curr_h)->next_in_global);
    }
    pthread_mutex_unlock(&global_heaps_mutex);

    /* Safely destroy remaining active threads associated with this heap */
    while (heap->threads != NULL) {
        struct jsthread *t = heap->threads;
        js_destroythread(t);
    }

    if (heap->rt) {
        /* Clean up the DOM bridge first while the runtime opaque is still valid.
         * qjs_bridge_cleanup will set the opaque to NULL when finished. */
        qjs_bridge_cleanup(heap->rt);
        JS_RunGC(heap->rt);
        JS_RunGC(heap->rt);
        JS_SetRuntimeOpaque(heap->rt, NULL);
        JS_FreeRuntime(heap->rt);
    }
    free(heap);
}

static void qjs_apply_csp_eval_restrictions(JSContext *ctx)
{
    /* Block eval / Function if CSP blocks 'unsafe-eval' */
    struct jsthread *t = (struct jsthread *)JS_GetContextOpaque(ctx);
    if (t) {
        struct html_content *htmlc = (t->win_priv && t->win_priv != t->doc_priv) ? (struct html_content *)t->doc_priv
                                                                                 : NULL;
        if (htmlc && htmlc->csp) {
            if (!csp_check_eval(htmlc->csp)) {
                const char *csp_eval_block =
                    "globalThis.eval = function() { throw new EvalError('CSP blocks eval'); };\n"
                    "globalThis.Function = function() { throw new EvalError('CSP blocks Function'); };\n";
                JSValue val = JS_Eval(ctx, csp_eval_block, strlen(csp_eval_block), "<csp-eval>", JS_EVAL_TYPE_GLOBAL);
                JS_FreeValue(ctx, val);
            }
        }
    }
}

#include "polyfill_intl_c.h"
#include "dataset_polyfill.h"
#include "polyfill_cssom_c.h"
#include "new_polyfills.h"

void qjs_inject_dom_polyfills(JSContext *ctx)
{

    JSValue val_intl = JS_Eval(ctx, intl_polyfill, strlen(intl_polyfill), "<intl-polyfill>", JS_EVAL_TYPE_GLOBAL);
    JSValue val_dataset = JS_Eval(ctx, dataset_polyfill, strlen(dataset_polyfill), "<dataset-polyfill>", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(val_dataset)) {
        JSValue exc = JS_GetException(ctx);
        const char *exc_str = JS_ToCString(ctx, exc);
        NSLOG(wisp, WARNING, "Error evaluating dataset polyfill: %s", exc_str ? exc_str : "unknown");
        if (exc_str) JS_FreeCString(ctx, exc_str);
        JS_FreeValue(ctx, exc);
    }
    JS_FreeValue(ctx, val_dataset);
    JS_FreeValue(ctx, val_intl);

    JSValue val_cssom = JS_Eval(ctx, cssom_polyfill, strlen(cssom_polyfill), "<cssom-polyfill>", JS_EVAL_TYPE_GLOBAL);
    JS_FreeValue(ctx, val_cssom);

    JSValue val_new = JS_Eval(ctx, new_polyfills_js, strlen(new_polyfills_js), "<new-polyfills>", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(val_new)) {
        JSValue exc = JS_GetException(ctx);
        const char *exc_str = JS_ToCString(ctx, exc);
        NSLOG(wisp, WARNING, "Error evaluating new polyfill: %s", exc_str ? exc_str : "unknown");
        if (exc_str) JS_FreeCString(ctx, exc_str);
        JS_FreeValue(ctx, exc);
    }
    JS_FreeValue(ctx, val_new);

    const char *fetch_polyfill =
        "if (typeof globalThis.Headers === 'undefined') {\n"
        "    globalThis.Headers = class Headers {\n"
        "        constructor(init) {\n"
        "            this._map = new Map();\n"
        "            if (init) {\n"
        "                if (init instanceof Headers) {\n"
        "                    init.forEach((value, name) => { this.append(name, value); });\n"
        "                } else if (Array.isArray(init)) {\n"
        "                    init.forEach(([name, value]) => { this.append(name, value); });\n"
        "                } else if (typeof init === 'object') {\n"
        "                    Object.keys(init).forEach(name => { this.append(name, init[name]); });\n"
        "                }\n"
        "            }\n"
        "        }\n"
        "        append(name, value) {\n"
        "            name = name.toLowerCase();\n"
        "            if (this._map.has(name)) {\n"
        "                this._map.set(name, this._map.get(name) + ', ' + value);\n"
        "            } else {\n"
        "                this._map.set(name, value);\n"
        "            }\n"
        "        }\n"
        "        delete(name) { this._map.delete(name.toLowerCase()); }\n"
        "        get(name) { return this._map.get(name.toLowerCase()) || null; }\n"
        "        has(name) { return this._map.has(name.toLowerCase()); }\n"
        "        set(name, value) { this._map.set(name.toLowerCase(), value); }\n"
        "        forEach(callback, thisArg) {\n"
        "            this._map.forEach((value, key) => { callback.call(thisArg, value, key, this); });\n"
        "        }\n"
        "        entries() { return this._map.entries(); }\n"
        "        keys() { return this._map.keys(); }\n"
        "        values() { return this._map.values(); }\n"
        "        [Symbol.iterator]() { return this._map.entries(); }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.ReadableStream === 'undefined') {\n"
        "    globalThis.ReadableStream = class ReadableStream {\n"
        "        constructor(underlyingSource = {}) {\n"
        "            this._state = 'readable';\n"
        "            this._reader = null;\n"
        "            this._storedChunks = [];\n"
        "            this._readRequests = [];\n"
        "            const controller = {\n"
        "                enqueue: (chunk) => {\n"
        "                    if (this._state !== 'readable') return;\n"
        "                    if (this._reader && this._readRequests.length > 0) {\n"
        "                        const request = this._readRequests.shift();\n"
        "                        request.resolve({ value: chunk, done: false });\n"
        "                    } else {\n"
        "                        this._storedChunks.push(chunk);\n"
        "                    }\n"
        "                },\n"
        "                close: () => {\n"
        "                    if (this._state !== 'readable') return;\n"
        "                    this._state = 'closed';\n"
        "                    while (this._readRequests.length > 0) {\n"
        "                        const request = this._readRequests.shift();\n"
        "                        request.resolve({ value: undefined, done: true });\n"
        "                    }\n"
        "                },\n"
        "                error: (err) => {\n"
        "                    if (this._state !== 'readable') return;\n"
        "                    this._state = 'errored';\n"
        "                    this._error = err;\n"
        "                    while (this._readRequests.length > 0) {\n"
        "                        const request = this._readRequests.shift();\n"
        "                        request.reject(err);\n"
        "                    }\n"
        "                }\n"
        "            };\n"
        "            if (underlyingSource.start) {\n"
        "                try {\n"
        "                    underlyingSource.start(controller);\n"
        "                } catch (e) {\n"
        "                    controller.error(e);\n"
        "                }\n"
        "            }\n"
        "        }\n"
        "        get locked() { return this._reader !== null; }\n"
        "        getReader() {\n"
        "            if (this._reader) {\n"
        "                throw new TypeError('Stream is already locked by another reader');\n"
        "            }\n"
        "            this._reader = new globalThis.ReadableStreamDefaultReader(this);\n"
        "            return this._reader;\n"
        "        }\n"
        "        cancel(reason) {\n"
        "            if (this._state === 'errored') return Promise.reject(this._error);\n"
        "            if (this._state === 'closed') return Promise.resolve();\n"
        "            this._state = 'closed';\n"
        "            this._storedChunks = [];\n"
        "            while (this._readRequests.length > 0) {\n"
        "                const request = this._readRequests.shift();\n"
        "                request.resolve({ value: undefined, done: true });\n"
        "            }\n"
        "            return Promise.resolve();\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.ReadableStreamDefaultReader === 'undefined') {\n"
        "    globalThis.ReadableStreamDefaultReader = class ReadableStreamDefaultReader {\n"
        "        constructor(stream) { this._stream = stream; }\n"
        "        read() {\n"
        "            if (!this._stream) return Promise.reject(new TypeError('Reader is released'));\n"
        "            if (this._stream._storedChunks.length > 0) {\n"
        "                const chunk = this._stream._storedChunks.shift();\n"
        "                return Promise.resolve({ value: chunk, done: false });\n"
        "            }\n"
        "            if (this._stream._state === 'closed') {\n"
        "                return Promise.resolve({ value: undefined, done: true });\n"
        "            }\n"
        "            if (this._stream._state === 'errored') {\n"
        "                return Promise.reject(this._stream._error);\n"
        "            }\n"
        "            return new Promise((resolve, reject) => {\n"
        "                this._stream._readRequests.push({ resolve, reject });\n"
        "            });\n"
        "        }\n"
        "        cancel(reason) {\n"
        "            if (!this._stream) return Promise.reject(new TypeError('Reader is released'));\n"
        "            return this._stream.cancel(reason);\n"
        "        }\n"
        "        releaseLock() {\n"
        "            if (!this._stream) return;\n"
        "            if (this._stream._readRequests.length > 0) {\n"
        "                throw new TypeError('Cannot release lock with pending read requests');\n"
        "            }\n"
        "            this._stream._reader = null;\n"
        "            this._stream = null;\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.WritableStream === 'undefined') {\n"
        "    globalThis.WritableStream = class WritableStream {\n"
        "        constructor(underlyingSink = {}) {\n"
        "            this._state = 'writable';\n"
        "            this._writer = null;\n"
        "            this._underlyingSink = underlyingSink;\n"
        "        }\n"
        "        get locked() { return this._writer !== null; }\n"
        "        getWriter() {\n"
        "            if (this._writer) throw new TypeError('WritableStream is already locked');\n"
        "            this._writer = new globalThis.WritableStreamDefaultWriter(this);\n"
        "            return this._writer;\n"
        "        }\n"
        "        abort(reason) {\n"
        "            if (this._state === 'errored') return Promise.reject(this._error);\n"
        "            if (this._state === 'closed') return Promise.resolve();\n"
        "            this._state = 'errored'; this._error = reason;\n"
        "            if (this._underlyingSink.abort) {\n"
        "                try {\n"
        "                    return Promise.resolve(this._underlyingSink.abort(reason));\n"
        "                } catch (e) {\n"
        "                    return Promise.reject(e);\n"
        "                }\n"
        "            }\n"
        "            return Promise.resolve();\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.WritableStreamDefaultWriter === 'undefined') {\n"
        "    globalThis.WritableStreamDefaultWriter = class WritableStreamDefaultWriter {\n"
        "        constructor(stream) { this._stream = stream; }\n"
        "        write(chunk) {\n"
        "            if (!this._stream) return Promise.reject(new TypeError('Writer is released'));\n"
        "            if (this._stream._state !== 'writable') return Promise.reject(new TypeError('Stream is not writable'));\n"
        "            if (this._stream._underlyingSink.write) {\n"
        "                try {\n"
        "                    return Promise.resolve(this._stream._underlyingSink.write(chunk));\n"
        "                } catch (e) { return Promise.reject(e); }\n"
        "            }\n"
        "            return Promise.resolve();\n"
        "        }\n"
        "        close() {\n"
        "            if (!this._stream) return Promise.reject(new TypeError('Writer is released'));\n"
        "            if (this._stream._state !== 'writable') return Promise.reject(new TypeError('Stream is not writable'));\n"
        "            this._stream._state = 'closed';\n"
        "            if (this._stream._underlyingSink.close) {\n"
        "                try {\n"
        "                    return Promise.resolve(this._stream._underlyingSink.close());\n"
        "                } catch (e) { return Promise.reject(e); }\n"
        "            }\n"
        "            return Promise.resolve();\n"
        "        }\n"
        "        abort(reason) {\n"
        "            if (!this._stream) return Promise.reject(new TypeError('Writer is released'));\n"
        "            return this._stream.abort(reason);\n"
        "        }\n"
        "        releaseLock() {\n"
        "            if (!this._stream) return;\n"
        "            this._stream._writer = null;\n"
        "            this._stream = null;\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.Request === 'undefined') {\n"
        "    globalThis.Request = class Request {\n"
        "        constructor(input, options = {}) {\n"
        "            if (typeof input === 'string') {\n"
        "                this.url = input;\n"
        "            } else if (input && typeof input === 'object') {\n"
        "                this.url = input.url;\n"
        "                this.method = input.method;\n"
        "                this.headers = new globalThis.Headers(input.headers);\n"
        "            }\n"
        "            this.method = options.method || this.method || 'GET';\n"
        "            this.headers = new globalThis.Headers(options.headers || this.headers);\n"
        "            this.body = options.body || null;\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.Response === 'undefined') {\n"
        "    globalThis.Response = class Response {\n"
        "        constructor(body, init = {}) {\n"
        "            this.body = body;\n"
        "            this.status = init.status !== undefined ? init.status : 200;\n"
        "            this.statusText = init.statusText || '';\n"
        "            this.headers = new globalThis.Headers(init.headers);\n"
        "            this.ok = this.status >= 200 && this.status < 300;\n"
        "            this.bodyUsed = false;\n"
        "        }\n"
        "        text() {\n"
        "            if (this.bodyUsed) return Promise.reject(new TypeError('Body has already been consumed'));\n"
        "            this.bodyUsed = true;\n"
        "            if (!this.body) return Promise.resolve('');\n"
        "            if (typeof this.body === 'string') return Promise.resolve(this.body);\n"
        "            if (this.body instanceof globalThis.ReadableStream) {\n"
        "                const reader = this.body.getReader();\n"
        "                const decoder = globalThis.TextDecoder ? new TextDecoder('utf-8') : null;\n"
        "                let result = '';\n"
        "                const uint8ToString = (bytes) => {\n"
        "                    const CHUNK_SIZE = 0x8000;\n"
        "                    let strResult = '';\n"
        "                    for (let i = 0; i < bytes.length; i += CHUNK_SIZE) {\n"
        "                        strResult += String.fromCharCode.apply(\n"
        "                            null,\n"
        "                            bytes.subarray ? bytes.subarray(i, i + CHUNK_SIZE) : bytes.slice(i, i + CHUNK_SIZE)\n"
        "                        );\n"
        "                    }\n"
        "                    return strResult;\n"
        "                };\n"
        "                return new Promise((resolve, reject) => {\n"
        "                    function pump() {\n"
        "                        reader.read().then(({ value, done }) => {\n"
        "                            if (done) { resolve(result); return; }\n"
        "                            if (typeof value === 'string') {\n"
        "                                result += value;\n"
        "                            } else if (value instanceof Uint8Array) {\n"
        "                                result += decoder ? decoder.decode(value, { stream: true }) : uint8ToString(value);\n"
        "                            }\n"
        "                            pump();\n"
        "                        }).catch(reject);\n"
        "                    }\n"
        "                    pump();\n"
        "                });\n"
        "            }\n"
        "            return Promise.resolve(String(this.body));\n"
        "        }\n"
        "        json() {\n"
        "            return this.text().then(text => JSON.parse(text));\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.ShadowRoot === 'undefined') {\n"
        "    globalThis.ShadowRoot = class ShadowRoot {\n"
        "        constructor(host, mode) {\n"
        "            this.host = host;\n"
        "            this.mode = mode;\n"
        "            this._childNodes = [];\n"
        "            this.nodeType = 11;\n"
        "            this.nodeName = '#document-fragment';\n"
        "        }\n"
        "        get children() {\n"
        "            return this._childNodes.filter(node => node && node.nodeType === 1);\n"
        "        }\n"
        "        get firstElementChild() {\n"
        "            const elements = this.children;\n"
        "            return elements.length > 0 ? elements[0] : null;\n"
        "        }\n"
        "        get lastElementChild() {\n"
        "            const elements = this.children;\n"
        "            return elements.length > 0 ? elements[elements.length - 1] : null;\n"
        "        }\n"
        "        get childElementCount() {\n"
        "            return this.children.length;\n"
        "        }\n"
        "        get firstChild() {\n"
        "            return this._childNodes.length > 0 ? this._childNodes[0] : null;\n"
        "        }\n"
        "        get lastChild() {\n"
        "            return this._childNodes.length > 0 ? this._childNodes[this._childNodes.length - 1] : null;\n"
        "        }\n"
        "        appendChild(node) {\n"
        "            if (!node) return null;\n"
        "            if (node.parentNode) {\n"
        "                node.parentNode.removeChild(node);\n"
        "            }\n"
        "            node.parentNode = this;\n"
        "            this._childNodes.push(node);\n"
        "            return node;\n"
        "        }\n"
        "        removeChild(node) {\n"
        "            const idx = this._childNodes.indexOf(node);\n"
        "            if (idx === -1) throw new Error('Node not found');\n"
        "            this._childNodes.splice(idx, 1);\n"
        "            node.parentNode = null;\n"
        "            return node;\n"
        "        }\n"
        "        insertBefore(newNode, referenceNode) {\n"
        "            if (!newNode) return null;\n"
        "            if (newNode.parentNode) {\n"
        "                newNode.parentNode.removeChild(newNode);\n"
        "            }\n"
        "            if (!referenceNode) {\n"
        "                return this.appendChild(newNode);\n"
        "            }\n"
        "            const idx = this._childNodes.indexOf(referenceNode);\n"
        "            if (idx === -1) throw new Error('Reference node not found');\n"
        "            newNode.parentNode = this;\n"
        "            this._childNodes.splice(idx, 0, newNode);\n"
        "            return newNode;\n"
        "        }\n"
        "        replaceChild(newChild, oldChild) {\n"
        "            if (!newChild || !oldChild) return null;\n"
        "            const idx = this._childNodes.indexOf(oldChild);\n"
        "            if (idx === -1) throw new Error('Old child not found');\n"
        "            if (newChild.parentNode) {\n"
        "                newChild.parentNode.removeChild(newChild);\n"
        "            }\n"
        "            oldChild.parentNode = null;\n"
        "            newChild.parentNode = this;\n"
        "            this._childNodes[idx] = newChild;\n"
        "            return oldChild;\n"
        "        }\n"
        "        querySelector(selectors) {\n"
        "            for (const child of this._childNodes) {\n"
        "                if (child.nodeType === 1) {\n"
        "                    if (child.matches && child.matches(selectors)) return child;\n"
        "                    const res = child.querySelector(selectors);\n"
        "                    if (res) return res;\n"
        "                }\n"
        "            }\n"
        "            return null;\n"
        "        }\n"
        "        querySelectorAll(selectors) {\n"
        "            let results = [];\n"
        "            function traverse(node) {\n"
        "                if (node.nodeType === 1) {\n"
        "                    if (node.matches && node.matches(selectors)) results.push(node);\n"
        "                    for (const c of (node.childNodes || [])) {\n"
        "                        traverse(c);\n"
        "                    }\n"
        "                }\n"
        "            }\n"
        "            for (const child of this._childNodes) {\n"
        "                traverse(child);\n"
        "            }\n"
        "            return results;\n"
        "        }\n"
        "        get innerHTML() {\n"
        "            let html = '';\n"
        "            for (const child of this._childNodes) {\n"
        "                if (child.nodeType === 1) {\n"
        "                    html += child.outerHTML || '';\n"
        "                } else if (child.nodeType === 3) {\n"
        "                    html += child.nodeValue || '';\n"
        "                }\n"
        "            }\n"
        "            return html;\n"
        "        }\n"
        "        set innerHTML(htmlText) {\n"
        "            while (this._childNodes.length > 0) {\n"
        "                this.removeChild(this._childNodes[0]);\n"
        "            }\n"
        "            if (!htmlText) return;\n"
        "            const parser = new DOMParser();\n"
        "            const doc = parser.parseFromString(htmlText, 'text/html');\n"
        "            if (doc && doc.body) {\n"
        "                const childNodes = Array.from(doc.body.childNodes);\n"
        "                for (const child of childNodes) {\n"
        "                    this.appendChild(child);\n"
        "                }\n"
        "            }\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof Element.prototype.attachShadow === 'undefined') {\n"
        "    Element.prototype.attachShadow = function(init) {\n"
        "        if (!init || (init.mode !== 'open' && init.mode !== 'closed')) {\n"
        "            throw new TypeError('Failed to execute attachShadow on Element: member mode is required and must be open or closed');\n"
        "        }\n"
        "        const shadowRoot = new globalThis.ShadowRoot(this, init.mode);\n"
        "        if (init.mode === 'open') {\n"
        "            this.shadowRoot = shadowRoot;\n"
        "        } else {\n"
        "            this.shadowRoot = null;\n"
        "        }\n"
        "        this._shadowRoot = shadowRoot;\n"
        "        return shadowRoot;\n"
        "    };\n"
        "}\n"
        "\n"
        "if (globalThis.History && globalThis.History.prototype) {\n"
        "    let historyState = null;\n"
        "    let historyLength = 1;\n"
        "    Object.defineProperty(globalThis.History.prototype, 'state', {\n"
        "        get: function() { return historyState; },\n"
        "        configurable: true\n"
        "    });\n"
        "    Object.defineProperty(globalThis.History.prototype, 'length', {\n"
        "        get: function() { return historyLength; },\n"
        "        configurable: true\n"
        "    });\n"
        "    globalThis.History.prototype.pushState = function(state, title, url) {\n"
        "        historyState = state;\n"
        "        historyLength++;\n"
        "    };\n"
        "    globalThis.History.prototype.replaceState = function(state, title, url) {\n"
        "        historyState = state;\n"
        "    };\n"
        "}\n"
        "\n"
        "globalThis.fetch = function(url, options) {\n"
        "    return new Promise(function(resolve, reject) {\n"
        "        var xhr = new XMLHttpRequest();\n"
        "        options = options || {};\n"
        "        var method = options.method || 'GET';\n"
        "        xhr.open(method, url, true);\n"
        "        if (options.headers) {\n"
        "            if (typeof options.headers.forEach === 'function') {\n"
        "                options.headers.forEach(function(value, name) {\n"
        "                    xhr.setRequestHeader(name, value);\n"
        "                });\n"
        "            } else {\n"
        "                for (var header in options.headers) {\n"
        "                    if (options.headers.hasOwnProperty(header)) {\n"
        "                        xhr.setRequestHeader(header, options.headers[header]);\n"
        "                    }\n"
        "                }\n"
        "            }\n"
        "        }\n"
        "        var resolved = false;\n"
        "        var streamController = null;\n"
        "        var lastLength = 0;\n"
        "        var stream = new globalThis.ReadableStream({\n"
        "            start: function(controller) {\n"
        "                streamController = controller;\n"
        "            }\n"
        "        });\n"
        "        const encoder = globalThis.TextEncoder ? new TextEncoder() : {\n"
        "            encode: str => {\n"
        "                const arr = new Uint8Array(str.length);\n"
        "                for (let i = 0; i < str.length; i++) arr[i] = str.charCodeAt(i) & 0xff;\n"
        "                return arr;\n"
        "            }\n"
        "        };\n"
        "        function onStateChange() {\n"
        "            if (xhr.readyState === 2 && !resolved) {\n"
        "                var headers = new globalThis.Headers();\n"
        "                var rawHeaders = xhr.getAllResponseHeaders() || '';\n"
        "                var lines = rawHeaders.split('\\n');\n"
        "                for (var i = 0; i < lines.length; i++) {\n"
        "                    var line = lines[i];\n"
        "                    var parts = line.split(':');\n"
        "                    if (parts.length >= 2) {\n"
        "                        headers.append(parts[0].trim(), parts.slice(1).join(':').trim());\n"
        "                    }\n"
        "                }\n"
        "                var response = new globalThis.Response(stream, {\n"
        "                    status: xhr.status,\n"
        "                    statusText: xhr.statusText,\n"
        "                    headers: headers\n"
        "                });\n"
        "                resolved = true;\n"
        "                resolve(response);\n"
        "            }\n"
        "            if (xhr.readyState >= 3 && streamController) {\n"
        "                var currentText = xhr.responseText || '';\n"
        "                if (currentText.length > lastLength) {\n"
        "                    var newText = currentText.slice(lastLength);\n"
        "                    lastLength = currentText.length;\n"
        "                    streamController.enqueue(encoder.encode(newText));\n"
        "                }\n"
        "            }\n"
        "            if (xhr.readyState === 4) {\n"
        "                if (!resolved) {\n"
        "                    var headers = new globalThis.Headers();\n"
        "                    var rawHeaders = xhr.getAllResponseHeaders() || '';\n"
        "                    var lines = rawHeaders.split('\\n');\n"
        "                    for (var i = 0; i < lines.length; i++) {\n"
        "                        var line = lines[i];\n"
        "                        var parts = line.split(':');\n"
        "                        if (parts.length >= 2) {\n"
        "                            headers.append(parts[0].trim(), parts.slice(1).join(':').trim());\n"
        "                        }\n"
        "                    }\n"
        "                    var currentText = xhr.responseText || '';\n"
        "                    var newText = currentText.slice(lastLength);\n"
        "                    var payloadStream = stream;\n"
        "                    if (newText.length > 0) {\n"
        "                        payloadStream = new globalThis.ReadableStream({\n"
        "                            start: function(controller) {\n"
        "                                controller.enqueue(encoder.encode(newText));\n"
        "                                controller.close();\n"
        "                            }\n"
        "                        });\n"
        "                    } else if (streamController) {\n"
        "                        streamController.close();\n"
        "                    }\n"
        "                    var response = new globalThis.Response(payloadStream, {\n"
        "                        status: xhr.status,\n"
        "                        statusText: xhr.statusText,\n"
        "                        headers: headers\n"
        "                    });\n"
        "                    resolved = true;\n"
        "                    resolve(response);\n"
        "                } else if (streamController) {\n"
        "                    streamController.close();\n"
        "                }\n"
        "            }\n"
        "        }\n"
        "        xhr.onreadystatechange = onStateChange;\n"
        "        xhr.onerror = function() {\n"
        "            if (!resolved) {\n"
        "                reject(new TypeError('Network request failed'));\n"
        "            } else if (streamController) {\n"
        "                streamController.error(new TypeError('Network request failed'));\n"
        "            }\n"
        "        };\n"
        "        xhr.send(options.body || null);\n"
        "    });\n"
        "};\n"
        "(function() {\n"
        "    if (globalThis.XMLHttpRequest && XMLHttpRequest.prototype) {\n"
        "        const origXhrSend = XMLHttpRequest.prototype.send;\n"
        "        XMLHttpRequest.prototype.send = function(body) {\n"
        "            this.__body = body;\n"
        "            return origXhrSend.call(this);\n"
        "        };\n"
        "    }\n"
        "})();\n"
        "(function() {\n"
        "    if (typeof globalThis.PerformanceEntry === 'undefined') {\n"
        "        globalThis.PerformanceEntry = class PerformanceEntry {\n"
        "            constructor(name, entryType, startTime, duration) {\n"
        "                this.name = String(name);\n"
        "                this.entryType = String(entryType);\n"
        "                this.startTime = Number(startTime);\n"
        "                this.duration = Number(duration);\n"
        "            }\n"
        "            toJSON() {\n"
        "                return {\n"
        "                    name: this.name,\n"
        "                    entryType: this.entryType,\n"
        "                    startTime: this.startTime,\n"
        "                    duration: this.duration\n"
        "                };\n"
        "            }\n"
        "        };\n"
        "    }\n"
        "\n"
        "    if (typeof globalThis.PerformanceMark === 'undefined') {\n"
        "        globalThis.PerformanceMark = class PerformanceMark extends globalThis.PerformanceEntry {\n"
        "            constructor(name, options = {}) {\n"
        "                const startTime = options.startTime !== undefined ? Number(options.startTime) : (globalThis.performance ? globalThis.performance.now() : Date.now());\n"
        "                super(name, 'mark', startTime, 0);\n"
        "                this.detail = options.detail !== undefined ? options.detail : null;\n"
        "            }\n"
        "            toJSON() {\n"
        "                const json = super.toJSON();\n"
        "                json.detail = this.detail;\n"
        "                return json;\n"
        "            }\n"
        "        };\n"
        "    }\n"
        "\n"
        "    if (typeof globalThis.PerformanceMeasure === 'undefined') {\n"
        "        globalThis.PerformanceMeasure = class PerformanceMeasure extends globalThis.PerformanceEntry {\n"
        "            constructor(name, startTime, duration, detail = null) {\n"
        "                super(name, 'measure', startTime, duration);\n"
        "                this.detail = detail;\n"
        "            }\n"
        "            toJSON() {\n"
        "                const json = super.toJSON();\n"
        "                json.detail = this.detail;\n"
        "                return json;\n"
        "            }\n"
        "        };\n"
        "    }\n"
        "\n"
        "    if (typeof globalThis.PerformancePaintTiming === 'undefined') {\n"
        "        globalThis.PerformancePaintTiming = class PerformancePaintTiming extends globalThis.PerformanceEntry {\n"
        "            constructor(name, startTime) {\n"
        "                super(name, 'paint', startTime, 0);\n"
        "            }\n"
        "        };\n"
        "    }\n"
        "\n"
        "    if (typeof globalThis.PerformanceNavigationTiming === 'undefined') {\n"
        "        globalThis.PerformanceNavigationTiming = class PerformanceNavigationTiming extends globalThis.PerformanceEntry {\n"
        "            constructor(name, startTime, duration, timingData) {\n"
        "                super(name, 'navigation', startTime, duration);\n"
        "                this.type = 'navigate';\n"
        "                this.redirectCount = 0;\n"
        "                if (timingData) {\n"
        "                    Object.keys(timingData).forEach(key => {\n"
        "                        this[key] = timingData[key];\n"
        "                    });\n"
        "                }\n"
        "            }\n"
        "        };\n"
        "    }\n"
        "\n"
        "    const _entries = [];\n"
        "    const _marksByName = new Map();\n"
        "    const _observers = new Set();\n"
        "    const _timeOrigin = Date.now() - 100;\n"
        "\n"
        "    _entries.push(new globalThis.PerformanceNavigationTiming('document', 0, 150, {\n"
        "        domInteractive: 50,\n"
        "        domContentLoadedEventStart: 80,\n"
        "        domContentLoadedEventEnd: 90,\n"
        "        domComplete: 150,\n"
        "        loadEventStart: 150,\n"
        "        loadEventEnd: 160\n"
        "    }));\n"
        "    _entries.push(new globalThis.PerformancePaintTiming('first-paint', 100));\n"
        "    _entries.push(new globalThis.PerformancePaintTiming('first-contentful-paint', 110));\n"
        "\n"
        "    function _addEntry(entry) {\n"
        "        _entries.push(entry);\n"
        "        if (entry.entryType === 'mark') {\n"
        "            _marksByName.set(entry.name, entry);\n"
        "        }\n"
        "        _observers.forEach(obs => {\n"
        "            obs._queueEntry(entry);\n"
        "        });\n"
        "    }\n"
        "\n"
        "    if (typeof globalThis.PerformanceObserver === 'undefined') {\n"
        "        globalThis.PerformanceObserver = class PerformanceObserver {\n"
        "            static get supportedEntryTypes() {\n"
        "                return ['mark', 'measure', 'navigation', 'paint', 'resource', 'longtask'];\n"
        "            }\n"
        "\n"
        "            constructor(callback) {\n"
        "                if (typeof callback !== 'function') {\n"
        "                    throw new TypeError(\"Failed to construct 'PerformanceObserver': callback must be a function\");\n"
        "                }\n"
        "                this._callback = callback;\n"
        "                this._queuedEntries = [];\n"
        "                this._observingTypes = new Set();\n"
        "                this._microtaskQueued = false;\n"
        "            }\n"
        "\n"
        "            observe(options = {}) {\n"
        "                if (!options || (options.entryTypes === undefined && options.type === undefined)) {\n"
        "                    throw new TypeError(\"Failed to execute 'observe' on 'PerformanceObserver': Must provide either entryTypes or type options\");\n"
        "                }\n"
        "\n"
        "                let typesToObserve = [];\n"
        "                let buffered = !!options.buffered;\n"
        "\n"
        "                if (options.entryTypes !== undefined) {\n"
        "                    if (options.type !== undefined) {\n"
        "                        throw new TypeError(\"Failed to execute 'observe' on 'PerformanceObserver': Cannot specify both entryTypes and type\");\n"
        "                    }\n"
        "                    if (!Array.isArray(options.entryTypes)) {\n"
        "                        throw new TypeError(\"Failed to execute 'observe' on 'PerformanceObserver': entryTypes must be a sequence\");\n"
        "                    }\n"
        "                    typesToObserve = options.entryTypes;\n"
        "                } else if (options.type !== undefined) {\n"
        "                    typesToObserve = [String(options.type)];\n"
        "                }\n"
        "\n"
        "                const supported = PerformanceObserver.supportedEntryTypes;\n"
        "                const validatedTypes = typesToObserve.filter(t => supported.includes(t));\n"
        "\n"
        "                if (validatedTypes.length === 0) {\n"
        "                    return;\n"
        "                }\n"
        "\n"
        "                validatedTypes.forEach(t => {\n"
        "                    this._observingTypes.add(t);\n"
        "                    if (buffered) {\n"
        "                        _entries.forEach(entry => {\n"
        "                            if (entry.entryType === t && !this._queuedEntries.includes(entry)) {\n"
        "                                this._queuedEntries.push(entry);\n"
        "                            }\n"
        "                        });\n"
        "                    }\n"
        "                });\n"
        "\n"
        "                _observers.add(this);\n"
        "\n"
        "                if (this._queuedEntries.length > 0) {\n"
        "                    this._triggerCallback();\n"
        "                }\n"
        "            }\n"
        "\n"
        "            disconnect() {\n"
        "                this._observingTypes.clear();\n"
        "                this._queuedEntries = [];\n"
        "                _observers.delete(this);\n"
        "            }\n"
        "\n"
        "            takeRecords() {\n"
        "                const records = this._queuedEntries;\n"
        "                this._queuedEntries = [];\n"
        "                return records;\n"
        "            }\n"
        "\n"
        "            _queueEntry(entry) {\n"
        "                if (this._observingTypes.has(entry.entryType)) {\n"
        "                    this._queuedEntries.push(entry);\n"
        "                    this._triggerCallback();\n"
        "                }\n"
        "            }\n"
        "\n"
        "            _triggerCallback() {\n"
        "                if (this._microtaskQueued) return;\n"
        "                this._microtaskQueued = true;\n"
        "                queueMicrotask(() => {\n"
        "                    this._microtaskQueued = false;\n"
        "                    const records = this.takeRecords();\n"
        "                    if (records.length === 0) return;\n"
        "                    const list = {\n"
        "                        getEntries: () => [...records],\n"
        "                        getEntriesByType: (type) => records.filter(e => e.entryType === type),\n"
        "                        getEntriesByName: (name, type) => records.filter(e => e.name === name && (type === undefined || e.entryType === type))\n"
        "                    };\n"
        "                    try {\n"
        "                        this._callback(list, this);\n"
        "                    } catch (e) {\n"
        "                        console.error(\"Error in PerformanceObserver callback:\", e);\n"
        "                    }\n"
        "                });\n"
        "            }\n"
        "        };\n"
        "    }\n"
        "\n"
        "    const perf = globalThis.performance || {};\n"
        "    perf.timeOrigin = perf.timeOrigin || _timeOrigin;\n"
        "    perf.now = perf.now || function() { return Date.now() - _timeOrigin; };\n"
        "    perf.timing = perf.timing || {\n"
        "        navigationStart: _timeOrigin,\n"
        "        unloadEventStart: 0,\n"
        "        unloadEventEnd: 0,\n"
        "        redirectStart: 0,\n"
        "        redirectEnd: 0,\n"
        "        fetchStart: _timeOrigin + 20,\n"
        "        domainLookupStart: _timeOrigin + 20,\n"
        "        domainLookupEnd: _timeOrigin + 20,\n"
        "        connectStart: _timeOrigin + 20,\n"
        "        connectEnd: _timeOrigin + 20,\n"
        "        secureConnectionStart: 0,\n"
        "        requestStart: _timeOrigin + 50,\n"
        "        responseStart: _timeOrigin + 70,\n"
        "        responseEnd: _timeOrigin + 80,\n"
        "        domLoading: _timeOrigin + 90,\n"
        "        domInteractive: _timeOrigin + 150,\n"
        "        domContentLoadedEventStart: _timeOrigin + 180,\n"
        "        domContentLoadedEventEnd: _timeOrigin + 190,\n"
        "        domComplete: _timeOrigin + 250,\n"
        "        loadEventStart: _timeOrigin + 250,\n"
        "        loadEventEnd: _timeOrigin + 260\n"
        "    };\n"
        "    perf.navigation = perf.navigation || { type: 0, redirectCount: 0 };\n"
        "\n"
        "    perf.getEntries = function() {\n"
        "        return [..._entries];\n"
        "    };\n"
        "\n"
        "    perf.getEntriesByType = function(type) {\n"
        "        type = String(type);\n"
        "        return _entries.filter(e => e.entryType === type);\n"
        "    };\n"
        "\n"
        "    perf.getEntriesByName = function(name, type) {\n"
        "        name = String(name);\n"
        "        if (type !== undefined) {\n"
        "            type = String(type);\n"
        "            return _entries.filter(e => e.name === name && e.entryType === type);\n"
        "        }\n"
        "        return _entries.filter(e => e.name === name);\n"
        "    };\n"
        "\n"
        "    perf.mark = function(name, options = {}) {\n"
        "        const mark = new globalThis.PerformanceMark(name, options);\n"
        "        _addEntry(mark);\n"
        "        return mark;\n"
        "    };\n"
        "\n"
        "    perf.measure = function(name, startMarkOrOptions, endMark) {\n"
        "        let startTime = 0;\n"
        "        let endTime = perf.now();\n"
        "        let detail = null;\n"
        "\n"
        "        if (typeof startMarkOrOptions === 'object' && startMarkOrOptions !== null) {\n"
        "            if (startMarkOrOptions.start !== undefined) {\n"
        "                if (typeof startMarkOrOptions.start === 'string') {\n"
        "                    const m = _marksByName.get(startMarkOrOptions.start);\n"
        "                    startTime = m ? m.startTime : 0;\n"
        "                } else {\n"
        "                    startTime = Number(startMarkOrOptions.start);\n"
        "                }\n"
        "            }\n"
        "            if (startMarkOrOptions.end !== undefined) {\n"
        "                if (typeof startMarkOrOptions.end === 'string') {\n"
        "                    const m = _marksByName.get(startMarkOrOptions.end);\n"
        "                    endTime = m ? m.startTime : perf.now();\n"
        "                } else {\n"
        "                    endTime = Number(startMarkOrOptions.end);\n"
        "                }\n"
        "            }\n"
        "            if (startMarkOrOptions.detail !== undefined) {\n"
        "                detail = startMarkOrOptions.detail;\n"
        "            }\n"
        "        } else {\n"
        "            if (typeof startMarkOrOptions === 'string') {\n"
        "                const m = _marksByName.get(startMarkOrOptions);\n"
        "                startTime = m ? m.startTime : 0;\n"
        "            } else if (startMarkOrOptions !== undefined) {\n"
        "                startTime = Number(startMarkOrOptions);\n"
        "            }\n"
        "\n"
        "            if (typeof endMark === 'string') {\n"
        "                const m = _marksByName.get(endMark);\n"
        "                endTime = m ? m.startTime : perf.now();\n"
        "            } else if (endMark !== undefined) {\n"
        "                endTime = Number(endMark);\n"
        "            }\n"
        "        }\n"
        "\n"
        "        const duration = endTime - startTime;\n"
        "        const measure = new globalThis.PerformanceMeasure(name, startTime, duration, detail);\n"
        "        _addEntry(measure);\n"
        "        return measure;\n"
        "    };\n"
        "\n"
        "    perf.clearMarks = function(name) {\n"
        "        if (name === undefined) {\n"
        "            for (let i = _entries.length - 1; i >= 0; i--) {\n"
        "                if (_entries[i].entryType === 'mark') {\n"
        "                    _entries.splice(i, 1);\n"
        "                }\n"
        "            }\n"
        "            _marksByName.clear();\n"
        "        } else {\n"
        "            name = String(name);\n"
        "            for (let i = _entries.length - 1; i >= 0; i--) {\n"
        "                if (_entries[i].entryType === 'mark' && _entries[i].name === name) {\n"
        "                    _entries.splice(i, 1);\n"
        "                }\n"
        "            }\n"
        "            _marksByName.delete(name);\n"
        "        }\n"
        "    };\n"
        "\n"
        "    perf.clearMeasures = function(name) {\n"
        "        if (name === undefined) {\n"
        "            for (let i = _entries.length - 1; i >= 0; i--) {\n"
        "                if (_entries[i].entryType === 'measure') {\n"
        "                    _entries.splice(i, 1);\n"
        "                }\n"
        "            }\n"
        "        } else {\n"
        "            name = String(name);\n"
        "            for (let i = _entries.length - 1; i >= 0; i--) {\n"
        "                if (_entries[i].entryType === 'measure' && _entries[i].name === name) {\n"
        "                    _entries.splice(i, 1);\n"
        "                }\n"
        "            }\n"
        "        }\n"
        "    };\n"
        "\n"
        "    globalThis.performance = perf;\n"
        "})();\n"
        "globalThis.screen = globalThis.screen || {\n"
        "    width: 1920,\n"
        "    height: 1080,\n"
        "    availWidth: 1920,\n"
        "    availHeight: 1040,\n"
        "    colorDepth: 24,\n"
        "    pixelDepth: 24\n"
        "};\n"
        "globalThis.NodeFilter = globalThis.NodeFilter || {\n"
        "    FILTER_ACCEPT: 1,\n"
        "    FILTER_REJECT: 2,\n"
        "    FILTER_SKIP: 3,\n"
        "    SHOW_ALL: 0xFFFFFFFF,\n"
        "    SHOW_ELEMENT: 0x1,\n"
        "    SHOW_ATTRIBUTE: 0x2,\n"
        "    SHOW_TEXT: 0x4,\n"
        "    SHOW_CDATA_SECTION: 0x8,\n"
        "    SHOW_ENTITY_REFERENCE: 0x10,\n"
        "    SHOW_ENTITY: 0x20,\n"
        "    SHOW_PROCESSING_INSTRUCTION: 0x40,\n"
        "    SHOW_COMMENT: 0x80,\n"
        "    SHOW_DOCUMENT: 0x100,\n"
        "    SHOW_DOCUMENT_TYPE: 0x200,\n"
        "    SHOW_DOCUMENT_FRAGMENT: 0x400,\n"
        "    SHOW_NOTATION: 0x800\n"
        "};\n"
        "globalThis.devicePixelRatio = globalThis.devicePixelRatio || 1.0;\n"
        "globalThis.MessagePort = class MessagePort extends globalThis.EventTarget {\n"
        "    constructor() {\n"
        "        super();\n"
        "        this.onmessage = null;\n"
        "        this._other = null;\n"
        "    }\n"
        "    postMessage(message, transfer) {\n"
        "        var self = this;\n"
        "        if (self._other) {\n"
        "            setTimeout(function() {\n"
        "                if (self._other) {\n"
        "                    var event = null;\n"
        "                    if (globalThis.MessageEvent) {\n"
        "                        try {\n"
        "                            event = new globalThis.MessageEvent('message', { data: message });\n"
        "                        } catch(e) {\n"
        "                            event = new globalThis.Event('message');\n"
        "                            event.data = message;\n"
        "                        }\n"
        "                    } else {\n"
        "                        event = new globalThis.Event('message');\n"
        "                        event.data = message;\n"
        "                    }\n"
        "                    if (self._other.onmessage) {\n"
        "                        try { self._other.onmessage(event); } catch(e) { console.error(e); }\n"
        "                    }\n"
        "                    self._other.dispatchEvent(event);\n"
        "                }\n"
        "            }, 0);\n"
        "        }\n"
        "    }\n"
        "    start() {}\n"
        "    close() {}\n"
        "};\n"
        "globalThis.MessageChannel = class MessageChannel {\n"
        "    constructor() {\n"
        "        this.port1 = new globalThis.MessagePort();\n"
        "        this.port2 = new globalThis.MessagePort();\n"
        "        this.port1._other = this.port2;\n"
        "        this.port2._other = this.port1;\n"
        "    }\n"
        "};\n"
        "globalThis.RTCDataChannel = class RTCDataChannel extends globalThis.EventTarget {\n"
        "    constructor() {\n"
        "        super();\n"
        "        this.label = '';\n"
        "        this.ordered = true;\n"
        "        this.readyState = 'connecting';\n"
        "        this.bufferedAmount = 0;\n"
        "        this.binaryType = 'blob';\n"
        "        this.onopen = null;\n"
        "        this.onmessage = null;\n"
        "        this.onerror = null;\n"
        "        this.onclose = null;\n"
        "    }\n"
        "    send(data) {}\n"
        "    close() { this.readyState = 'closed'; }\n"
        "};\n"
        "globalThis.RTCSessionDescription = class RTCSessionDescription {\n"
        "    constructor(init) {\n"
        "        this.type = (init && init.type) ? init.type : '';\n"
        "        this.sdp = (init && init.sdp) ? init.sdp : '';\n"
        "    }\n"
        "    toJSON() { return { type: this.type, sdp: this.sdp }; }\n"
        "};\n"
        "globalThis.RTCIceCandidate = class RTCIceCandidate {\n"
        "    constructor(init) {\n"
        "        this.candidate = (init && init.candidate) ? init.candidate : '';\n"
        "        this.sdpMid = (init && init.sdpMid) ? init.sdpMid : null;\n"
        "        this.sdpMLineIndex = (init && init.sdpMLineIndex !== undefined) ? init.sdpMLineIndex : null;\n"
        "    }\n"
        "    toJSON() { return { candidate: this.candidate, sdpMid: this.sdpMid, sdpMLineIndex: this.sdpMLineIndex }; }\n"
        "};\n"
        "globalThis.RTCPeerConnection = class RTCPeerConnection extends globalThis.EventTarget {\n"
        "    constructor(configuration) {\n"
        "        super();\n"
        "        this.localDescription = null;\n"
        "        this.remoteDescription = null;\n"
        "        this.signalingState = 'stable';\n"
        "        this.iceConnectionState = 'new';\n"
        "        this.connectionState = 'new';\n"
        "        this.onicecandidate = null;\n"
        "        this.ondatachannel = null;\n"
        "        this.ontrack = null;\n"
        "    }\n"
        "    createOffer(options) { return Promise.resolve(new globalThis.RTCSessionDescription({ type: 'offer', sdp: '' })); }\n"
        "    createAnswer(options) { return Promise.resolve(new globalThis.RTCSessionDescription({ type: 'answer', sdp: '' })); }\n"
        "    setLocalDescription(desc) {\n"
        "        this.localDescription = desc;\n"
        "        return Promise.resolve();\n"
        "    }\n"
        "    setRemoteDescription(desc) {\n"
        "        this.remoteDescription = desc;\n"
        "        return Promise.resolve();\n"
        "    }\n"
        "    addIceCandidate(candidate) { return Promise.resolve(); }\n"
        "    createDataChannel(label, options) {\n"
        "        var dc = new globalThis.RTCDataChannel();\n"
        "        if (label) dc.label = label;\n"
        "        return dc;\n"
        "    }\n"
        "    close() {\n"
        "        this.signalingState = 'closed';\n"
        "        this.iceConnectionState = 'closed';\n"
        "        this.connectionState = 'closed';\n"
        "    }\n"
        "};\n"
        "globalThis.webkitRTCPeerConnection = globalThis.RTCPeerConnection;\n"
        "globalThis.mozRTCPeerConnection = globalThis.RTCPeerConnection;\n"
        "globalThis.msRTCPeerConnection = globalThis.RTCPeerConnection;\n"
        "globalThis.__wisp_get_computed_style_internal = function(elt, pseudoElt) {\n"
        "    if (!elt) return null;\n"
        "    const styleObj = globalThis.__wisp_new_cssstyledeclaration ? globalThis.__wisp_new_cssstyledeclaration(elt) : null;\n"
        "    if (!styleObj) {\n"
        "        const dummyStyle = {\n"
        "            getPropertyValue: function(prop) { return ''; },\n"
        "            getPropertyPriority: function() { return ''; },\n"
        "            setProperty: function() {},\n"
        "            removeProperty: function() {},\n"
        "            length: 0,\n"
        "            cssText: ''\n"
        "        };\n"
        "        if (globalThis.CSSStyleDeclaration && globalThis.CSSStyleDeclaration.prototype) {\n"
        "            Object.setPrototypeOf(dummyStyle, globalThis.CSSStyleDeclaration.prototype);\n"
        "        }\n"
        "        return dummyStyle;\n"
        "    }\n"
        "    const jsBuiltIns = new Set([\n"
        "        'constructor', 'toString', 'toLocaleString', 'valueOf', 'hasOwnProperty',\n"
        "        'isPrototypeOf', 'propertyIsEnumerable', '__proto__', '__defineGetter__',\n"
        "        '__defineSetter__', '__lookupGetter__', '__lookupSetter__'\n"
        "    ]);\n"
        "    return new Proxy(styleObj, {\n"
        "        get(target, prop) {\n"
        "            if (prop === 'getPropertyValue') {\n"
        "                return function(p) {\n"
        "                    return target.getPropertyValue(p);\n"
        "                };\n"
        "            }\n"
        "            if (prop === 'getPropertyPriority') {\n"
        "                return function(p) {\n"
        "                    return target.getPropertyPriority(p);\n"
        "                };\n"
        "            }\n"
        "            if (prop === 'setProperty') {\n"
        "                return function() {};\n"
        "            }\n"
        "            if (prop === 'removeProperty') {\n"
        "                return function() { return ''; };\n"
        "            }\n"
        "            if (prop === 'cssText') {\n"
        "                return target.cssText;\n"
        "            }\n"
        "            if (prop === 'length') {\n"
        "                return target.length;\n"
        "            }\n"
        "            if (prop === 'item') {\n"
        "                return function(idx) {\n"
        "                    return target.item(idx);\n"
        "                };\n"
        "            }\n"
        "            if (prop in target) {\n"
        "                return target[prop];\n"
        "            }\n"
        "            if (typeof prop === 'string') {\n"
        "                let kebab = prop.replace(/([A-Z])/g, '-$1').toLowerCase();\n"
        "                const val = target.getPropertyValue(kebab);\n"
        "                if (val !== undefined && val !== '') return val;\n"
        "                if (prop === 'display') return 'block';\n"
        "                if (prop === 'width') return '1024px';\n"
        "                if (prop === 'height') return '768px';\n"
        "                if (prop === 'opacity') return '1';\n"
        "                return '';\n"
        "            }\n"
        "            return undefined;\n"
        "        },\n"
        "        has(target, prop) {\n"
        "            if (typeof prop !== 'string') {\n"
        "                return Reflect.has(target, prop);\n"
        "            }\n"
        "            if (prop in target) {\n"
        "                return true;\n"
        "            }\n"
        "            if (jsBuiltIns.has(prop)) {\n"
        "                return false;\n"
        "            }\n"
        "            return /^[a-zA-Z0-9-]+$/.test(prop) && /^[a-zA-Z-]/.test(prop);\n"
        "        }\n"
        "    });\n"
        "};\n"
        "globalThis.getComputedStyle = globalThis.__wisp_get_computed_style_internal;\n"
        "\n"
        "if (typeof globalThis.TextEncoder === 'undefined') {\n"
        "    globalThis.TextEncoder = class TextEncoder {\n"
        "        get encoding() { return 'utf-8'; }\n"
        "        encode(str = '') {\n"
        "            str = String(str);\n"
        "            const len = str.length;\n"
        "            const bytes = [];\n"
        "            for (let i = 0; i < len; i++) {\n"
        "                let code = str.charCodeAt(i);\n"
        "                if (code >= 0xd800 && code <= 0xdbff && i + 1 < len) {\n"
        "                    const next = str.charCodeAt(i + 1);\n"
        "                    if (next >= 0xdc00 && next <= 0xdfff) {\n"
        "                        code = 0x10000 + ((code - 0xd800) << 10) + (next - 0xdc00);\n"
        "                        i++;\n"
        "                    }\n"
        "                }\n"
        "                if (code < 128) {\n"
        "                    bytes.push(code);\n"
        "                } else if (code < 2048) {\n"
        "                    bytes.push(0xc0 | (code >> 6), 0x80 | (code & 0x3f));\n"
        "                } else if (code < 65536) {\n"
        "                    bytes.push(0xe0 | (code >> 12), 0x80 | ((code >> 6) & 0x3f), 0x80 | (code & 0x3f));\n"
        "                } else {\n"
        "                    bytes.push(0xf0 | (code >> 18), 0x80 | ((code >> 12) & 0x3f), 0x80 | ((code >> 6) & 0x3f), 0x80 | (code & 0x3f));\n"
        "                }\n"
        "            }\n"
        "            return new Uint8Array(bytes);\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.TextDecoder === 'undefined') {\n"
        "    globalThis.TextDecoder = class TextDecoder {\n"
        "        constructor(label = 'utf-8', options = {}) {\n"
        "            this.encoding = 'utf-8';\n"
        "            this.fatal = !!options.fatal;\n"
        "            this.ignoreBOM = !!options.ignoreBOM;\n"
        "        }\n"
        "        decode(input, options = {}) {\n"
        "            if (!input) return '';\n"
        "            let bytes;\n"
        "            if (input instanceof Uint8Array) {\n"
        "                bytes = input;\n"
        "            } else if (input instanceof ArrayBuffer) {\n"
        "                bytes = new Uint8Array(input);\n"
        "            } else if (ArrayBuffer.isView(input)) {\n"
        "                bytes = new Uint8Array(input.buffer, input.byteOffset, input.byteLength);\n"
        "            } else {\n"
        "                throw new TypeError('Failed to execute \"decode\" on \"TextDecoder\": Invalid input');\n"
        "            }\n"
        "            let str = '';\n"
        "            let i = 0;\n"
        "            const len = bytes.length;\n"
        "            while (i < len) {\n"
        "                const b1 = bytes[i++];\n"
        "                if (b1 < 128) {\n"
        "                    str += String.fromCharCode(b1);\n"
        "                } else if (b1 >= 192 && b1 < 224 && i < len) {\n"
        "                    const b2 = bytes[i++];\n"
        "                    str += String.fromCharCode(((b1 & 31) << 6) | (b2 & 63));\n"
        "                } else if (b1 >= 224 && b1 < 240 && i + 1 < len) {\n"
        "                    const b2 = bytes[i++];\n"
        "                    const b3 = bytes[i++];\n"
        "                    str += String.fromCharCode(((b1 & 15) << 12) | ((b2 & 63) << 6) | (b3 & 63));\n"
        "                } else if (b1 >= 240 && b1 < 248 && i + 2 < len) {\n"
        "                    const b2 = bytes[i++];\n"
        "                    const b3 = bytes[i++];\n"
        "                    const b4 = bytes[i++];\n"
        "                    const cp = ((b1 & 7) << 18) | ((b2 & 63) << 12) | ((b3 & 63) << 6) | (b4 & 63);\n"
        "                    if (cp >= 0x10000) {\n"
        "                        const h = 0xd800 + ((cp - 0x10000) >> 10);\n"
        "                        const l = 0xdc00 + ((cp - 0x10000) & 0x3ff);\n"
        "                        str += String.fromCharCode(h, l);\n"
        "                    } else {\n"
        "                        str += String.fromCharCode(cp);\n"
        "                    }\n"
        "                } else {\n"
        "                    str += String.fromCharCode(0xfffd);\n"
        "                }\n"
        "            }\n"
        "            return str;\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.TextEncoderStream === 'undefined') {\n"
        "    globalThis.TextEncoderStream = class TextEncoderStream {\n"
        "        constructor() {\n"
        "            const encoder = new globalThis.TextEncoder();\n"
        "            let controller;\n"
        "            this.readable = new globalThis.ReadableStream({\n"
        "                start(c) {\n"
        "                    controller = c;\n"
        "                }\n"
        "            });\n"
        "            this.writable = new globalThis.WritableStream({\n"
        "                write(chunk) {\n"
        "                    if (controller) {\n"
        "                        controller.enqueue(encoder.encode(chunk));\n"
        "                    }\n"
        "                },\n"
        "                close() {\n"
        "                    if (controller) {\n"
        "                        controller.close();\n"
        "                    }\n"
        "                }\n"
        "            });\n"
        "        }\n"
        "        get encoding() { return 'utf-8'; }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.TextDecoderStream === 'undefined') {\n"
        "    globalThis.TextDecoderStream = class TextDecoderStream {\n"
        "        constructor(label = 'utf-8', options = {}) {\n"
        "            const decoder = new globalThis.TextDecoder(label, options);\n"
        "            let controller;\n"
        "            this.readable = new globalThis.ReadableStream({\n"
        "                start(c) {\n"
        "                    controller = c;\n"
        "                }\n"
        "            });\n"
        "            this.writable = new globalThis.WritableStream({\n"
        "                write(chunk) {\n"
        "                    if (controller) {\n"
        "                        controller.enqueue(decoder.decode(chunk, { stream: true }));\n"
        "                    }\n"
        "                },\n"
        "                close() {\n"
        "                    if (controller) {\n"
        "                        controller.close();\n"
        "                    }\n"
        "                }\n"
        "            });\n"
        "        }\n"
        "        get encoding() { return 'utf-8'; }\n"
        "        get fatal() { return false; }\n"
        "        get ignoreBOM() { return false; }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.URL === 'undefined') {\n"
        "    globalThis.URL = class {\n"
        "        constructor(url, base) {\n"
        "            url = String(url).trim();\n"
        "            if (base !== undefined) {\n"
        "                base = String(base).trim();\n"
        "            }\n"
        "            let parsedBase = null;\n"
        "            if (base) {\n"
        "                parsedBase = this._parse(base, null);\n"
        "                if (!parsedBase) {\n"
        "                    throw new TypeError(\"Failed to construct 'URL': Invalid base URL\");\n"
        "                }\n"
        "            }\n"
        "            const parsed = this._parse(url, parsedBase);\n"
        "            if (!parsed) {\n"
        "                throw new TypeError(\"Failed to construct 'URL': Invalid URL\");\n"
        "            }\n"
        "            this._protocol = parsed.protocol;\n"
        "            this._username = parsed.username;\n"
        "            this._password = parsed.password;\n"
        "            this._hostname = parsed.hostname;\n"
        "            this._port = parsed.port;\n"
        "            this._pathname = parsed.pathname;\n"
        "            this._search = parsed.search;\n"
        "            this._hash = parsed.hash;\n"
        "        }\n"
        "\n"
        "        _parse(urlStr, baseObj) {\n"
        "            let schemeMatch = urlStr.match(/^([a-zA-Z][a-zA-Z0-9+.-]*):/);\n"
        "            let scheme = schemeMatch ? schemeMatch[1].toLowerCase() : null;\n"
        "            let rest = schemeMatch ? urlStr.slice(schemeMatch[0].length) : urlStr;\n"
        "\n"
        "            if (!scheme && baseObj) {\n"
        "                scheme = baseObj._protocol.replace(':', '');\n"
        "            }\n"
        "\n"
        "            if (!scheme) {\n"
        "                return null;\n"
        "            }\n"
        "\n"
        "            let protocol = scheme + ':';\n"
        "            let username = '';\n"
        "            let password = '';\n"
        "            let hostname = '';\n"
        "            let port = '';\n"
        "            let pathname = '';\n"
        "            let search = '';\n"
        "            let hash = '';\n"
        "\n"
        "            if (rest.startsWith('//')) {\n"
        "                rest = rest.slice(2);\n"
        "                let authorityEnd = rest.indexOf('/');\n"
        "                if (authorityEnd === -1) {\n"
        "                    authorityEnd = rest.indexOf('?');\n"
        "                    if (authorityEnd === -1) {\n"
        "                        authorityEnd = rest.indexOf('#');\n"
        "                        if (authorityEnd === -1) {\n"
        "                            authorityEnd = rest.length;\n"
        "                        }\n"
        "                    }\n"
        "                }\n"
        "                let authority = rest.slice(0, authorityEnd);\n"
        "                rest = rest.slice(authorityEnd);\n"
        "\n"
        "                let atIdx = authority.indexOf('@');\n"
        "                if (atIdx !== -1) {\n"
        "                    let userinfo = authority.slice(0, atIdx);\n"
        "                    authority = authority.slice(atIdx + 1);\n"
        "                    let colonIdx = userinfo.indexOf(':');\n"
        "                    if (colonIdx !== -1) {\n"
        "                        username = userinfo.slice(0, colonIdx);\n"
        "                        password = userinfo.slice(colonIdx + 1);\n"
        "                    } else {\n"
        "                        username = userinfo;\n"
        "                    }\n"
        "                }\n"
        "\n"
        "                let colonIdx = authority.lastIndexOf(':');\n"
        "                let bracketIdx = authority.indexOf(']');\n"
        "                if (colonIdx !== -1 && colonIdx > bracketIdx) {\n"
        "                    hostname = authority.slice(0, colonIdx);\n"
        "                    port = authority.slice(colonIdx + 1);\n"
        "                } else {\n"
        "                    hostname = authority;\n"
        "                }\n"
        "            } else if (baseObj) {\n"
        "                username = baseObj._username;\n"
        "                password = baseObj._password;\n"
        "                hostname = baseObj._hostname;\n"
        "                port = baseObj._port;\n"
        "                pathname = baseObj._pathname;\n"
        "            }\n"
        "\n"
        "            let hashIdx = rest.indexOf('#');\n"
        "            if (hashIdx !== -1) {\n"
        "                hash = rest.slice(hashIdx);\n"
        "                rest = rest.slice(0, hashIdx);\n"
        "            }\n"
        "            let searchIdx = rest.indexOf('?');\n"
        "            if (searchIdx !== -1) {\n"
        "                search = rest.slice(searchIdx);\n"
        "                rest = rest.slice(0, searchIdx);\n"
        "            }\n"
        "\n"
        "            if (rest) {\n"
        "                if (rest.startsWith('/')) {\n"
        "                    pathname = rest;\n"
        "                } else if (baseObj) {\n"
        "                    let basePath = baseObj._pathname;\n"
        "                    let lastSlash = basePath.lastIndexOf('/');\n"
        "                    pathname = basePath.slice(0, lastSlash + 1) + rest;\n"
        "                } else {\n"
        "                    pathname = '/' + rest;\n"
        "                }\n"
        "            } else if (!pathname) {\n"
        "                pathname = '/';\n"
        "            }\n"
        "\n"
        "            let segments = pathname.split('/');\n"
        "            let resolved = [];\n"
        "            for (let segment of segments) {\n"
        "                if (segment === '.' || segment === '') {\n"
        "                    if (resolved.length === 0) resolved.push('');\n"
        "                } else if (segment === '..') {\n"
        "                    if (resolved.length > 1) resolved.pop();\n"
        "                } else {\n"
        "                    resolved.push(segment);\n"
        "                }\n"
        "            }\n"
        "            pathname = resolved.join('/') || '/';\n"
        "\n"
        "            return { protocol, username, password, hostname, port, pathname, search, hash };\n"
        "        }\n"
        "\n"
        "        get href() {\n"
        "            let auth = '';\n"
        "            if (this._username || this._password) {\n"
        "                auth = this._username + (this._password ? ':' + this._password : '') + '@';\n"
        "            }\n"
        "            let host = this.host;\n"
        "            return this._protocol + (host ? '//' + auth + host : '') + this._pathname + this._search + this._hash;\n"
        "        }\n"
        "        set href(value) {\n"
        "            const parsed = this._parse(String(value), null);\n"
        "            if (!parsed) throw new TypeError(\"Invalid URL\");\n"
        "            this._protocol = parsed.protocol;\n"
        "            this._username = parsed.username;\n"
        "            this._password = parsed.password;\n"
        "            this._hostname = parsed.hostname;\n"
        "            this._port = parsed.port;\n"
        "            this._pathname = parsed.pathname;\n"
        "            this._search = parsed.search;\n"
        "            this._hash = parsed.hash;\n"
        "        }\n"
        "\n"
        "        get origin() {\n"
        "            return this._protocol + '//' + this.host;\n"
        "        }\n"
        "\n"
        "        get protocol() { return this._protocol; }\n"
        "        set protocol(val) {\n"
        "            val = String(val).trim().toLowerCase();\n"
        "            if (val.match(/^[a-z][a-z0-9+.-]*:?$/)) {\n"
        "                this._protocol = val.endsWith(':') ? val : val + ':';\n"
        "            }\n"
        "        }\n"
        "\n"
        "        get username() { return this._username; }\n"
        "        set username(val) { this._username = String(val); }\n"
        "\n"
        "        get password() { return this._password; }\n"
        "        set password(val) { this._password = String(val); }\n"
        "\n"
        "        get host() {\n"
        "            return this._hostname + (this._port ? ':' + this._port : '');\n"
        "        }\n"
        "        set host(val) {\n"
        "            val = String(val).trim();\n"
        "            let colonIdx = val.lastIndexOf(':');\n"
        "            let bracketIdx = val.indexOf(']');\n"
        "            if (colonIdx !== -1 && colonIdx > bracketIdx) {\n"
        "                this._hostname = val.slice(0, colonIdx);\n"
        "                this._port = val.slice(colonIdx + 1);\n"
        "            } else {\n"
        "                this._hostname = val;\n"
        "                this._port = '';\n"
        "            }\n"
        "        }\n"
        "\n"
        "        get hostname() { return this._hostname; }\n"
        "        set hostname(val) { this._hostname = String(val).trim(); }\n"
        "\n"
        "        get port() { return this._port; }\n"
        "        set port(val) { this._port = String(val).trim(); }\n"
        "\n"
        "        get pathname() { return this._pathname; }\n"
        "        set pathname(val) {\n"
        "            val = String(val);\n"
        "            this._pathname = val.startsWith('/') ? val : '/' + val;\n"
        "        }\n"
        "\n"
        "        get search() { return this._search; }\n"
        "        set search(val) {\n"
        "            val = String(val);\n"
        "            if (!val) {\n"
        "                this._search = '';\n"
        "            } else {\n"
        "                this._search = val.startsWith('?') ? val : '?' + val;\n"
        "            }\n"
        "        }\n"
        "\n"
        "        get hash() { return this._hash; }\n"
        "        set hash(val) {\n"
        "            val = String(val);\n"
        "            if (!val) {\n"
        "                this._hash = '';\n"
        "            } else {\n"
        "                this._hash = val.startsWith('#') ? val : '#' + val;\n"
        "            }\n"
        "        }\n"
        "\n"
        "        get searchParams() {\n"
        "            return new globalThis.URLSearchParams(this._search);\n"
        "        }\n"
        "\n"
        "        toString() {\n"
        "            return this.href;\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.AbortController === 'undefined') {\n"
        "    globalThis.AbortSignal = class AbortSignal extends globalThis.EventTarget {\n"
        "        constructor() {\n"
        "            super();\n"
        "            this._aborted = false;\n"
        "            this._reason = undefined;\n"
        "            this.onabort = null;\n"
        "        }\n"
        "        get aborted() { return this._aborted; }\n"
        "        get reason() { return this._reason; }\n"
        "        throwIfAborted() {\n"
        "            if (this._aborted) {\n"
        "                throw this._reason !== undefined ? this._reason : new DOMException('The user aborted a request.', 'AbortError');\n"
        "            }\n"
        "        }\n"
        "    };\n"
        "\n"
        "    globalThis.AbortController = class AbortController {\n"
        "        constructor() {\n"
        "            this._signal = new globalThis.AbortSignal();\n"
        "        }\n"
        "        get signal() { return this._signal; }\n"
        "        abort(reason) {\n"
        "            if (this._signal._aborted) return;\n"
        "            this._signal._aborted = true;\n"
        "            this._signal._reason = reason !== undefined ? reason : new DOMException('The user aborted a request.', 'AbortError');\n"
        "            const event = new globalThis.Event('abort');\n"
        "            if (typeof this._signal.onabort === 'function') {\n"
        "                try { this._signal.onabort(event); } catch (e) {}\n"
        "            }\n"
        "            this._signal.dispatchEvent(event);\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.MediaStreamTrack === 'undefined') {\n"
        "    globalThis.MediaStreamTrack = class MediaStreamTrack extends globalThis.EventTarget {\n"
        "        constructor(kind, label) {\n"
        "            super();\n"
        "            this.id = 'track-' + Math.random().toString(36).slice(2, 11);\n"
        "            this.kind = kind || 'audio';\n"
        "            this.label = label || (kind === 'video' ? 'Camera' : 'Microphone');\n"
        "            this.enabled = true;\n"
        "            this.readyState = 'live';\n"
        "            this.onended = null;\n"
        "        }\n"
        "        stop() {\n"
        "            if (this.readyState === 'ended') return;\n"
        "            this.readyState = 'ended';\n"
        "            const event = new globalThis.Event('ended');\n"
        "            if (typeof this.onended === 'function') {\n"
        "                try { this.onended(event); } catch(e) {}\n"
        "            }\n"
        "            this.dispatchEvent(event);\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.MediaStream === 'undefined') {\n"
        "    globalThis.MediaStream = class MediaStream extends globalThis.EventTarget {\n"
        "        constructor(arg) {\n"
        "            super();\n"
        "            this.id = 'stream-' + Math.random().toString(36).slice(2, 11);\n"
        "            this._tracks = [];\n"
        "            if (arg instanceof globalThis.MediaStream) {\n"
        "                this._tracks = [...arg._tracks];\n"
        "            } else if (Array.isArray(arg)) {\n"
        "                this._tracks = [...arg];\n"
        "            }\n"
        "            this.onaddtrack = null;\n"
        "            this.onremovetrack = null;\n"
        "        }\n"
        "        getTracks() { return this._tracks; }\n"
        "        getAudioTracks() { return this._tracks.filter(t => t.kind === 'audio'); }\n"
        "        getVideoTracks() { return this._tracks.filter(t => t.kind === 'video'); }\n"
        "        addTrack(track) {\n"
        "            if (this._tracks.indexOf(track) === -1) {\n"
        "                this._tracks.push(track);\n"
        "                const event = new globalThis.Event('addtrack');\n"
        "                event.track = track;\n"
        "                if (typeof this.onaddtrack === 'function') {\n"
        "                    try { this.onaddtrack(event); } catch(e) {}\n"
        "                }\n"
        "                this.dispatchEvent(event);\n"
        "            }\n"
        "        }\n"
        "        removeTrack(track) {\n"
        "            const idx = this._tracks.indexOf(track);\n"
        "            if (idx !== -1) {\n"
        "                this._tracks.splice(idx, 1);\n"
        "                const event = new globalThis.Event('removetrack');\n"
        "                event.track = track;\n"
        "                if (typeof this.onremovetrack === 'function') {\n"
        "                    try { this.onremovetrack(event); } catch(e) {}\n"
        "                }\n"
        "                this.dispatchEvent(event);\n"
        "            }\n"
        "        }\n"
        "        clone() {\n"
        "            return new globalThis.MediaStream(this._tracks.map(t => new globalThis.MediaStreamTrack(t.kind, t.label)));\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (globalThis.HTMLMediaElement) {\n"
        "    Object.defineProperty(globalThis.HTMLMediaElement.prototype, 'srcObject', {\n"
        "        get() { return this._srcObject || null; },\n"
        "        set(val) {\n"
        "            this._srcObject = val;\n"
        "            if (val) {\n"
        "                const self = this;\n"
        "                setTimeout(function() {\n"
        "                    const evt = new globalThis.Event('loadedmetadata');\n"
        "                    self.dispatchEvent(evt);\n"
        "                    const evt2 = new globalThis.Event('canplay');\n"
        "                    self.dispatchEvent(evt2);\n"
        "                }, 50);\n"
        "            }\n"
        "        },\n"
        "        configurable: true, enumerable: true\n"
        "    });\n"
        "}\n"
        "\n"
        "if (typeof globalThis.MediaDevices === 'undefined') {\n"
        "    globalThis.MediaDevices = class MediaDevices extends globalThis.EventTarget {\n"
        "        constructor() {\n"
        "            super();\n"
        "            this.ondevicechange = null;\n"
        "        }\n"
        "        enumerateDevices() {\n"
        "            return Promise.resolve([\n"
        "                { deviceId: 'default', kind: 'audioinput', label: 'Default Microphone', groupId: 'group1' },\n"
        "                { deviceId: 'default', kind: 'videoinput', label: 'Default Camera', groupId: 'group2' },\n"
        "                { deviceId: 'default', kind: 'audiooutput', label: 'Default Speaker', groupId: 'group1' }\n"
        "            ]);\n"
        "        }\n"
        "        getSupportedConstraints() {\n"
        "            return {\n"
        "                aspectRatio: true,\n"
        "                channelCount: true,\n"
        "                depth: false,\n"
        "                deviceId: true,\n"
        "                echoCancellation: true,\n"
        "                facingMode: true,\n"
        "                frameRate: true,\n"
        "                groupId: true,\n"
        "                height: true,\n"
        "                latency: true,\n"
        "                noiseSuppression: true,\n"
        "                sampleRate: true,\n"
        "                sampleSize: true,\n"
        "                width: true\n"
        "            };\n"
        "        }\n"
        "        getUserMedia(constraints) {\n"
        "            const tracks = [];\n"
        "            const audio_requested = !!(constraints && constraints.audio);\n"
        "            const video_requested = !!(constraints && constraints.video);\n"
        "            const probed = (globalThis.navigator && globalThis.navigator.__probe_devices) ? globalThis.navigator.__probe_devices(audio_requested, video_requested) : { audio: true, video: true };\n"
        "            if (audio_requested) {\n"
        "                if (probed.audio) {\n"
        "                    tracks.push(new globalThis.MediaStreamTrack('audio', 'Microphone'));\n"
        "                } else {\n"
        "                    return Promise.reject(new DOMException('Requested audio capture device not found', 'NotFoundError'));\n"
        "                }\n"
        "            }\n"
        "            if (video_requested) {\n"
        "                if (probed.video) {\n"
        "                    tracks.push(new globalThis.MediaStreamTrack('video', 'Camera'));\n"
        "                } else {\n"
        "                    return Promise.reject(new DOMException('Requested video capture device not found', 'NotFoundError'));\n"
        "                }\n"
        "            }\n"
        "            if (tracks.length === 0 && !audio_requested && !video_requested) {\n"
        "                if (probed.audio) tracks.push(new globalThis.MediaStreamTrack('audio', 'Microphone'));\n"
        "                if (probed.video) tracks.push(new globalThis.MediaStreamTrack('video', 'Camera'));\n"
        "            }\n"
        "            return Promise.resolve(new globalThis.MediaStream(tracks));\n"
        "        }\n"
        "        getDisplayMedia(constraints) {\n"
        "            const track = new globalThis.MediaStreamTrack('video', 'Screen Share');\n"
        "            return Promise.resolve(new globalThis.MediaStream([track]));\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (globalThis.navigator) {\n"
        "    globalThis.navigator.mediaDevices = globalThis.navigator.mediaDevices || new globalThis.MediaDevices();\n"
        "}\n"
        "\n"
        "if (typeof globalThis.SpeechSynthesisVoice === 'undefined') {\n"
        "    globalThis.SpeechSynthesisVoice = class SpeechSynthesisVoice {\n"
        "        constructor(voiceURI, name, lang, localService, isDefault) {\n"
        "            this.voiceURI = voiceURI || 'urn:wisp:voice:default';\n"
        "            this.name = name || 'System Voice';\n"
        "            this.lang = lang || 'en-US';\n"
        "            this.localService = localService !== undefined ? !!localService : true;\n"
        "            this.default = isDefault !== undefined ? !!isDefault : true;\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.SpeechSynthesisUtterance === 'undefined') {\n"
        "    globalThis.SpeechSynthesisUtterance = class SpeechSynthesisUtterance extends globalThis.EventTarget {\n"
        "        constructor(text) {\n"
        "            super();\n"
        "            this.text = text !== undefined ? String(text) : '';\n"
        "            this.lang = '';\n"
        "            this.voice = null;\n"
        "            this.volume = 1.0;\n"
        "            this.rate = 1.0;\n"
        "            this.pitch = 1.0;\n"
        "            this.onstart = null;\n"
        "            this.onend = null;\n"
        "            this.onerror = null;\n"
        "            this.onpause = null;\n"
        "            this.onresume = null;\n"
        "            this.onmark = null;\n"
        "            this.onboundary = null;\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.SpeechSynthesisEvent === 'undefined') {\n"
        "    globalThis.SpeechSynthesisEvent = class SpeechSynthesisEvent extends globalThis.Event {\n"
        "        constructor(type, eventInitDict = {}) {\n"
        "            super(type, eventInitDict);\n"
        "            this.utterance = eventInitDict.utterance || null;\n"
        "            this.charIndex = eventInitDict.charIndex || 0;\n"
        "            this.charLength = eventInitDict.charLength || 0;\n"
        "            this.elapsedTime = eventInitDict.elapsedTime || 0;\n"
        "            this.name = eventInitDict.name || '';\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.SpeechSynthesisErrorEvent === 'undefined') {\n"
        "    globalThis.SpeechSynthesisErrorEvent = class SpeechSynthesisErrorEvent extends globalThis.SpeechSynthesisEvent {\n"
        "        constructor(type, eventInitDict = {}) {\n"
        "            super(type, eventInitDict);\n"
        "            this.error = eventInitDict.error || 'unknown';\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.SpeechSynthesis === 'undefined') {\n"
        "    const _defaultVoice = new globalThis.SpeechSynthesisVoice('urn:wisp:voice:default', 'System Voice', 'en-US', true, true);\n"
        "    const _voicesList = [_defaultVoice];\n"
        "\n"
        "    globalThis.SpeechSynthesis = class SpeechSynthesis extends globalThis.EventTarget {\n"
        "        constructor() {\n"
        "            super();\n"
        "            this.pending = false;\n"
        "            this.speaking = false;\n"
        "            this.paused = false;\n"
        "            this.onvoiceschanged = null;\n"
        "        }\n"
        "        getVoices() {\n"
        "            return _voicesList;\n"
        "        }\n"
        "        speak(utterance) {\n"
        "            if (!(utterance instanceof globalThis.SpeechSynthesisUtterance)) {\n"
        "                throw new TypeError(\"Failed to execute 'speak' on 'SpeechSynthesis': parameter 1 is not of type 'SpeechSynthesisUtterance'.\");\n"
        "            }\n"
        "            this.speaking = true;\n"
        "            const self = this;\n"
        "            setTimeout(() => {\n"
        "                const startEvt = new globalThis.SpeechSynthesisEvent('start', { utterance: utterance });\n"
        "                if (typeof utterance.onstart === 'function') {\n"
        "                    try { utterance.onstart(startEvt); } catch(e) {}\n"
        "                }\n"
        "                utterance.dispatchEvent(startEvt);\n"
        "\n"
        "                setTimeout(() => {\n"
        "                    self.speaking = false;\n"
        "                    const endEvt = new globalThis.SpeechSynthesisEvent('end', { utterance: utterance });\n"
        "                    if (typeof utterance.onend === 'function') {\n"
        "                        try { utterance.onend(endEvt); } catch(e) {}\n"
        "                    }\n"
        "                    utterance.dispatchEvent(endEvt);\n"
        "                }, 10);\n"
        "            }, 0);\n"
        "        }\n"
        "        cancel() {\n"
        "            this.speaking = false;\n"
        "            this.pending = false;\n"
        "            this.paused = false;\n"
        "        }\n"
        "        pause() {\n"
        "            this.paused = true;\n"
        "        }\n"
        "        resume() {\n"
        "            this.paused = false;\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (!(globalThis.speechSynthesis instanceof globalThis.SpeechSynthesis)) {\n"
        "    globalThis.speechSynthesis = new globalThis.SpeechSynthesis();\n"
        "}\n"
        "if (typeof Window !== 'undefined' && Window.prototype) {\n"
        "    Object.defineProperty(Window.prototype, 'speechSynthesis', {\n"
        "        get() { return globalThis.speechSynthesis; },\n"
        "        configurable: true, enumerable: true\n"
        "    });\n"
        "}\n"
        "if (typeof window !== 'undefined') {\n"
        "    window.speechSynthesis = globalThis.speechSynthesis;\n"
        "}\n"
        "\n"
        "if (typeof globalThis.GeolocationPositionError === 'undefined') {\n"
        "    globalThis.GeolocationPositionError = class GeolocationPositionError {\n"
        "        constructor(code, message) {\n"
        "            this.code = code || 0;\n"
        "            this.message = message || '';\n"
        "        }\n"
        "    };\n"
        "    globalThis.GeolocationPositionError.PERMISSION_DENIED = 1;\n"
        "    globalThis.GeolocationPositionError.POSITION_UNAVAILABLE = 2;\n"
        "    globalThis.GeolocationPositionError.TIMEOUT = 3;\n"
        "    globalThis.PositionError = globalThis.GeolocationPositionError;\n"
        "}\n"
        "\n"
        "if (typeof globalThis.GeolocationCoordinates === 'undefined') {\n"
        "    globalThis.GeolocationCoordinates = class GeolocationCoordinates {\n"
        "        constructor(latitude, longitude, altitude, accuracy, altitudeAccuracy, heading, speed) {\n"
        "            this.latitude = latitude !== undefined ? latitude : 37.7749;\n"
        "            this.longitude = longitude !== undefined ? longitude : -122.4194;\n"
        "            this.altitude = altitude !== undefined ? altitude : null;\n"
        "            this.accuracy = accuracy !== undefined ? accuracy : 10;\n"
        "            this.altitudeAccuracy = altitudeAccuracy !== undefined ? altitudeAccuracy : null;\n"
        "            this.heading = heading !== undefined ? heading : null;\n"
        "            this.speed = speed !== undefined ? speed : null;\n"
        "        }\n"
        "    };\n"
        "    globalThis.Coordinates = globalThis.GeolocationCoordinates;\n"
        "}\n"
        "\n"
        "if (typeof globalThis.GeolocationPosition === 'undefined') {\n"
        "    globalThis.GeolocationPosition = class GeolocationPosition {\n"
        "        constructor(coords, timestamp) {\n"
        "            this.coords = coords || new globalThis.GeolocationCoordinates();\n"
        "            this.timestamp = timestamp !== undefined ? timestamp : Date.now();\n"
        "        }\n"
        "    };\n"
        "    globalThis.Position = globalThis.GeolocationPosition;\n"
        "}\n"
        "\n"
        "if (typeof globalThis.Geolocation === 'undefined') {\n"
        "    globalThis.Geolocation = class Geolocation {\n"
        "        constructor() {\n"
        "            this._watchId = 0;\n"
        "            this._watches = new Map();\n"
        "        }\n"
        "        getCurrentPosition(successCallback, errorCallback, options) {\n"
        "            if (typeof successCallback !== 'function') {\n"
        "                throw new TypeError(\"Failed to execute 'getCurrentPosition' on 'Geolocation': 1 argument required, but only 0 present.\");\n"
        "            }\n"
        "            const pos = new globalThis.GeolocationPosition();\n"
        "            setTimeout(() => {\n"
        "                try { successCallback(pos); } catch (e) {}\n"
        "            }, 0);\n"
        "        }\n"
        "        watchPosition(successCallback, errorCallback, options) {\n"
        "            if (typeof successCallback !== 'function') {\n"
        "                throw new TypeError(\"Failed to execute 'watchPosition' on 'Geolocation': 1 argument required, but only 0 present.\");\n"
        "            }\n"
        "            const id = ++this._watchId;\n"
        "            const pos = new globalThis.GeolocationPosition();\n"
        "            this._watches.set(id, successCallback);\n"
        "            setTimeout(() => {\n"
        "                if (this._watches.has(id)) {\n"
        "                    try { successCallback(pos); } catch (e) {}\n"
        "                }\n"
        "            }, 0);\n"
        "            return id;\n"
        "        }\n"
        "        clearWatch(watchId) {\n"
        "            this._watches.delete(watchId);\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (globalThis.navigator && !globalThis.navigator.geolocation) {\n"
        "    globalThis.navigator.geolocation = new globalThis.Geolocation();\n"
        "}\n"
        "\n"
        "if (typeof globalThis.DeviceOrientationEvent === 'undefined') {\n"
        "    globalThis.DeviceOrientationEvent = class DeviceOrientationEvent extends globalThis.Event {\n"
        "        constructor(type, eventInitDict = {}) {\n"
        "            super(type, eventInitDict);\n"
        "            this.alpha = eventInitDict.alpha !== undefined ? eventInitDict.alpha : 0;\n"
        "            this.beta = eventInitDict.beta !== undefined ? eventInitDict.beta : 0;\n"
        "            this.gamma = eventInitDict.gamma !== undefined ? eventInitDict.gamma : 0;\n"
        "            this.absolute = eventInitDict.absolute !== undefined ? !!eventInitDict.absolute : false;\n"
        "        }\n"
        "        static requestPermission() {\n"
        "            return Promise.resolve('granted');\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.DeviceMotionEvent === 'undefined') {\n"
        "    globalThis.DeviceMotionEvent = class DeviceMotionEvent extends globalThis.Event {\n"
        "        constructor(type, eventInitDict = {}) {\n"
        "            super(type, eventInitDict);\n"
        "            this.acceleration = eventInitDict.acceleration !== undefined ? eventInitDict.acceleration : { x: 0, y: 0, z: 0 };\n"
        "            this.accelerationIncludingGravity = eventInitDict.accelerationIncludingGravity !== undefined ? eventInitDict.accelerationIncludingGravity : { x: 0, y: 0, z: 9.81 };\n"
        "            this.rotationRate = eventInitDict.rotationRate !== undefined ? eventInitDict.rotationRate : { alpha: 0, beta: 0, gamma: 0 };\n"
        "            this.interval = eventInitDict.interval !== undefined ? eventInitDict.interval : 16;\n"
        "        }\n"
        "        static requestPermission() {\n"
        "            return Promise.resolve('granted');\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.GamepadButton === 'undefined') {\n"
        "    globalThis.GamepadButton = class GamepadButton {\n"
        "        constructor(pressed = false, touched = false, value = 0) {\n"
        "            this.pressed = !!pressed;\n"
        "            this.touched = !!touched;\n"
        "            this.value = Number(value);\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.Gamepad === 'undefined') {\n"
        "    globalThis.Gamepad = class Gamepad {\n"
        "        constructor(id = '', index = -1, connected = false, timestamp = 0, mapping = '', axes = [], buttons = [], vibrationActuator = null) {\n"
        "            this.id = String(id);\n"
        "            this.index = Number(index);\n"
        "            this.connected = !!connected;\n"
        "            this.timestamp = Number(timestamp);\n"
        "            this.mapping = String(mapping);\n"
        "            this.axes = Array.isArray(axes) ? axes : [];\n"
        "            this.buttons = Array.isArray(buttons) ? buttons : [];\n"
        "            this.vibrationActuator = vibrationActuator || null;\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.GamepadEvent === 'undefined') {\n"
        "    globalThis.GamepadEvent = class GamepadEvent extends globalThis.Event {\n"
        "        constructor(type, eventInitDict = {}) {\n"
        "            super(type, eventInitDict);\n"
        "            this.gamepad = eventInitDict.gamepad !== undefined ? eventInitDict.gamepad : null;\n"
        "        }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (globalThis.navigator && typeof globalThis.navigator.getGamepads !== 'function') {\n"
        "    globalThis.navigator.getGamepads = function getGamepads() {\n"
        "        return [];\n"
        "    };\n"
        "}\n"
        "\n"
        "if (globalThis.navigator && typeof globalThis.navigator.vibrate !== 'function') {\n"
        "    globalThis.navigator.vibrate = function vibrate(pattern) {\n"
        "        return true;\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.BatteryManager === 'undefined') {\n"
        "    globalThis.BatteryManager = class BatteryManager extends globalThis.EventTarget {\n"
        "        constructor() {\n"
        "            super();\n"
        "            this.charging = true;\n"
        "            this.chargingTime = 0;\n"
        "            this.dischargingTime = Infinity;\n"
        "            this.level = 1.0;\n"
        "            this._onchargingchange = null;\n"
        "            this._onchargingtimechange = null;\n"
        "            this._ondischargingtimechange = null;\n"
        "            this._onlevelchange = null;\n"
        "        }\n"
        "        get onchargingchange() { return this._onchargingchange; }\n"
        "        set onchargingchange(fn) { this._onchargingchange = typeof fn === 'function' ? fn : null; }\n"
        "        get onchargingtimechange() { return this._onchargingtimechange; }\n"
        "        set onchargingtimechange(fn) { this._onchargingtimechange = typeof fn === 'function' ? fn : null; }\n"
        "        get ondischargingtimechange() { return this._ondischargingtimechange; }\n"
        "        set ondischargingtimechange(fn) { this._ondischargingtimechange = typeof fn === 'function' ? fn : null; }\n"
        "        get onlevelchange() { return this._onlevelchange; }\n"
        "        set onlevelchange(fn) { this._onlevelchange = typeof fn === 'function' ? fn : null; }\n"
        "    };\n"
        "}\n"
        "\n"
        "if (globalThis.navigator && typeof globalThis.navigator.getBattery !== 'function') {\n"
        "    let batteryInstance = null;\n"
        "    globalThis.navigator.getBattery = function getBattery() {\n"
        "        if (!batteryInstance) {\n"
        "            batteryInstance = new globalThis.BatteryManager();\n"
        "        }\n"
        "        return Promise.resolve(batteryInstance);\n"
        "    };\n"
        "}\n"
        "\n"
        "(function() {\n"
        "    function getRetargetedTarget(target, currentTarget) {\n"
        "        let t = target;\n"
        "        while (t) {\n"
        "            let isAncestor = false;\n"
        "            let p = t;\n"
        "            while (p) {\n"
        "                if (p === currentTarget) {\n"
        "                    isAncestor = true;\n"
        "                    break;\n"
        "                }\n"
        "                p = p.parentNode || p.host;\n"
        "            }\n"
        "            if (isAncestor) return t;\n"
        "            let nextT = null;\n"
        "            let p2 = t;\n"
        "            while (p2) {\n"
        "                if (p2.host) {\n"
        "                    nextT = p2.host;\n"
        "                    break;\n"
        "                }\n"
        "                p2 = p2.parentNode;\n"
        "            }\n"
        "            if (!nextT) break;\n"
        "            t = nextT;\n"
        "        }\n"
        "        return target;\n"
        "    }\n"
        "\n"
        "    Object.defineProperty(Event.prototype, 'target', {\n"
        "        get() {\n"
        "            const t = this._target !== undefined ? this._target : null;\n"
        "            const ct = this.currentTarget;\n"
        "            if (t && ct) {\n"
        "                return getRetargetedTarget(t, ct);\n"
        "            }\n"
        "            return t;\n"
        "        },\n"
        "        configurable: true, enumerable: true\n"
        "    });\n"
        "\n"
        "    Object.defineProperty(Event.prototype, 'currentTarget', {\n"
        "        get() {\n"
        "            return this._currentTarget !== undefined ? this._currentTarget : null;\n"
        "        },\n"
        "        configurable: true, enumerable: true\n"
        "    });\n"
        "\n"
        "    Object.defineProperty(Event.prototype, 'eventPhase', {\n"
        "        get() {\n"
        "            return this._eventPhase !== undefined ? this._eventPhase : 0;\n"
        "        },\n"
        "        configurable: true, enumerable: true\n"
        "    });\n"
        "\n"
        "    Object.defineProperty(Event.prototype, 'composed', {\n"
        "        get() {\n"
        "            return this._composed !== undefined ? this._composed : false;\n"
        "        },\n"
        "        set(v) {\n"
        "            this._composed = !!v;\n"
        "        },\n"
        "        configurable: true, enumerable: true\n"
        "    });\n"
        "\n"
        "    Event.prototype.composedPath = function() {\n"
        "        if (!this._composedPath) {\n"
        "            const path = [];\n"
        "            let curr = this.target || this.currentTarget;\n"
        "            if (curr) {\n"
        "                while (curr) {\n"
        "                    path.push(curr);\n"
        "                    if (curr.parentNode) {\n"
        "                        curr = curr.parentNode;\n"
        "                    } else if (curr.host) {\n"
        "                        if (this.composed) {\n"
        "                            curr = curr.host;\n"
        "                        } else {\n"
        "                            curr = null;\n"
        "                        }\n"
        "                    } else if (curr === globalThis.document) {\n"
        "                        curr = globalThis;\n"
        "                    } else {\n"
        "                        curr = null;\n"
        "                    }\n"
        "                }\n"
        "                if (path.length > 0 && path[path.length - 1] !== globalThis) {\n"
        "                    path.push(globalThis);\n"
        "                }\n"
        "            }\n"
        "            this._composedPath = path;\n"
        "        }\n"
        "\n"
        "        const currentTarget = this.currentTarget;\n"
        "        if (!currentTarget) return this._composedPath;\n"
        "\n"
        "        const closedRootsToHide = [];\n"
        "        let p = this.target;\n"
        "        while (p) {\n"
        "            if (p.host) {\n"
        "                if (p.mode === 'closed') {\n"
        "                    let hasAsAncestor = false;\n"
        "                    let cp = currentTarget;\n"
        "                    while (cp) {\n"
        "                        if (cp === p) {\n"
        "                            hasAsAncestor = true;\n"
        "                            break;\n"
        "                        }\n"
        "                        cp = cp.parentNode || cp.host;\n"
        "                    }\n"
        "                    if (!hasAsAncestor) {\n"
        "                        closedRootsToHide.push(p);\n"
        "                    }\n"
        "                }\n"
        "                p = p.host;\n"
        "            } else {\n"
        "                p = p.parentNode;\n"
        "            }\n"
        "        }\n"
        "\n"
        "        return this._composedPath.filter(node => {\n"
        "            let p = node;\n"
        "            while (p) {\n"
        "                if (closedRootsToHide.includes(p)) {\n"
        "                    return false;\n"
        "                }\n"
        "                p = p.parentNode || p.host;\n"
        "            }\n"
        "            return true;\n"
        "        });\n"
        "    };\n"
        "\n"
        "    const OriginalEvent = globalThis.Event;\n"
        "    globalThis.Event = function(type, options = {}) {\n"
        "        const evt = new OriginalEvent(type, options);\n"
        "        evt._composed = !!options.composed;\n"
        "        Object.defineProperty(evt, 'bubbles', { value: !!options.bubbles, configurable: true, enumerable: true });\n"
        "        Object.defineProperty(evt, 'cancelable', { value: !!options.cancelable, configurable: true, enumerable: true });\n"
        "        return evt;\n"
        "    };\n"
        "    globalThis.Event.prototype = OriginalEvent.prototype;\n"
        "\n"
        "    const originalDispatchEvent = EventTarget.prototype.dispatchEvent;\n"
        "    EventTarget.prototype.dispatchEvent = function(event) {\n"
        "        if (!event) return false;\n"
        "        \n"
        "        const self = (this === undefined || this === null) ? globalThis : this;\n"
        "        if (!(self instanceof EventTarget || self === globalThis || (self && typeof self.addEventListener === 'function'))) {\n"
        "            throw new TypeError('Invalid this');\n"
        "        }\n"
        "        event._target = self;\n"
        "        \n"
        "        let insideShadow = false;\n"
        "        let curr_shadow = self;\n"
        "        while (curr_shadow) {\n"
        "            if (globalThis.ShadowRoot && curr_shadow instanceof globalThis.ShadowRoot) {\n"
        "                insideShadow = true;\n"
        "                break;\n"
        "            }\n"
        "            curr_shadow = curr_shadow.parentNode || curr_shadow.host;\n"
        "        }\n"
        "        const isNative = (self instanceof Node || (globalThis.Window && self instanceof globalThis.Window)) && (typeof __wisp_is_js_process === 'undefined' || !__wisp_is_js_process);\n"
        "        if (isNative && !insideShadow && originalDispatchEvent) {\n"
        "            return originalDispatchEvent.call(self, event);\n"
        "        }\n"
        "        \n"
        "        const fullPath = [];\n"
        "        let curr = self;\n"
        "        while (curr) {\n"
        "            fullPath.push(curr);\n"
        "            if (curr.parentNode) {\n"
        "                curr = curr.parentNode;\n"
        "            } else if (curr.host) {\n"
        "                if (event.composed) {\n"
        "                    curr = curr.host;\n"
        "                } else {\n"
        "                    curr = null;\n"
        "                }\n"
        "            } else if (curr === globalThis.document) {\n"
        "                curr = globalThis;\n"
        "            } else {\n"
        "                curr = null;\n"
        "            }\n"
        "        }\n"
        "        if (fullPath.length > 0 && fullPath[fullPath.length - 1] !== globalThis) {\n"
        "            fullPath.push(globalThis);\n"
        "        }\n"
        "        \n"
        "        event._composedPath = fullPath;\n"
        "\n"
        "        let stopPropagation = false;\n"
        "        let stopImmediatePropagation = false;\n"
        "\n"
        "        const origStop = event.stopPropagation;\n"
        "        const origStopImm = event.stopImmediatePropagation;\n"
        "\n"
        "        event.stopPropagation = function() {\n"
        "            stopPropagation = true;\n"
        "            if (origStop) {\n"
        "                try { origStop.call(this); } catch(e) {}\n"
        "            }\n"
        "        };\n"
        "        event.stopImmediatePropagation = function() {\n"
        "            stopImmediatePropagation = true;\n"
        "            stopPropagation = true;\n"
        "            if (origStopImm) {\n"
        "                try { origStopImm.call(this); } catch(e) {}\n"
        "            }\n"
        "        };\n"
        "\n"
        "        // Phase 1: Capture Phase\n"
        "        event._eventPhase = 1; // CAPTURING_PHASE\n"
        "        for (let i = fullPath.length - 1; i > 0; i--) {\n"
        "            const node = fullPath[i];\n"
        "            if (node === self) continue;\n"
        "            \n"
        "            event._target = getRetargetedTarget(self, node);\n"
        "            event._currentTarget = node;\n"
        "            \n"
        "            const listeners = node.__wisp_listeners ? node.__wisp_listeners[event.type] : null;\n"
        "            if (listeners) {\n"
        "                for (const record of listeners) {\n"
        "                    if (record.capture) {\n"
        "                        try {\n"
        "                            const cb = record.callback;\n"
        "                            if (typeof cb === 'function') {\n"
        "                                cb.call(node, event);\n"
        "                            } else if (cb && typeof cb.handleEvent === 'function') {\n"
        "                                cb.handleEvent(event);\n"
        "                            }\n"
        "                        } catch (e) {\n"
        "                            console.error('Error in event listener:', e);\n"
        "                        }\n"
        "                        if (stopImmediatePropagation) break;\n"
        "                    }\n"
        "                }\n"
        "            }\n"
        "            if (stopPropagation) break;\n"
        "        }\n"
        "\n"
        "        // Phase 2: Target Phase\n"
        "        if (!stopPropagation) {\n"
        "            event._eventPhase = 2; // AT_TARGET\n"
        "            event._target = self;\n"
        "            event._currentTarget = self;\n"
        "            \n"
        "            const listeners = self.__wisp_listeners ? self.__wisp_listeners[event.type] : null;\n"
        "            if (listeners) {\n"
        "                for (const record of listeners) {\n"
        "                    try {\n"
        "                        const cb = record.callback;\n"
        "                        if (typeof cb === 'function') {\n"
        "                            cb.call(self, event);\n"
        "                        } else if (cb && typeof cb.handleEvent === 'function') {\n"
        "                            cb.handleEvent(event);\n"
        "                        }\n"
        "                    } catch (e) { \n"
        "                        console.error('Error in event listener:', e);\n"
        "                    }\n"
        "                    if (stopImmediatePropagation) break;\n"
        "                }\n"
        "            }\n"
        "        }\n"
        "\n"
        "        // Phase 3: Bubble Phase\n"
        "        if (!stopPropagation && event.bubbles) {\n"
        "            event._eventPhase = 3; // BUBBLING_PHASE\n"
        "            for (let i = 1; i < fullPath.length; i++) {\n"
        "                const node = fullPath[i];\n"
        "                if (node === self) continue;\n"
        "                \n"
        "                event._target = getRetargetedTarget(self, node);\n"
        "                event._currentTarget = node;\n"
        "                \n"
        "                const listeners = node.__wisp_listeners ? node.__wisp_listeners[event.type] : null;\n"
        "                if (listeners) {\n"
        "                    for (const record of listeners) {\n"
        "                        if (!record.capture) {\n"
        "                            try {\n"
        "                                const cb = record.callback;\n"
        "                                if (typeof cb === 'function') {\n"
        "                                    cb.call(node, event);\n"
        "                                } else if (cb && typeof cb.handleEvent === 'function') {\n"
        "                                    cb.handleEvent(event);\n"
        "                                }\n"
        "                            } catch (e) {\n"
        "                                console.error('Error in event listener:', e);\n"
        "                            }\n"
        "                            if (stopImmediatePropagation) break;\n"
        "                        }\n"
        "                    }\n"
        "                }\n"
        "                if (stopPropagation) break;\n"
        "            }\n"
        "        }\n"
        "\n"
        "        event._currentTarget = null;\n"
        "        return !event.defaultPrevented;\n"
        "    };\n"
        "})();\n"
        "\n"
        "class CustomElementRegistry {\n"
        "    constructor() {\n"
        "        this._registry = new Map();\n"
        "        this._pendingCallbacks = [];\n"
        "        this._insideCallback = false;\n"
        "        this._whenDefinedPromises = new Map();\n"
        "    }\n"
        "\n"
        "    _queueCallback(fn) {\n"
        "        this._pendingCallbacks.push(fn);\n"
        "        if (this._insideCallback) return;\n"
        "\n"
        "        this._insideCallback = true;\n"
        "        try {\n"
        "            while (this._pendingCallbacks.length > 0) {\n"
        "                const next = this._pendingCallbacks.shift();\n"
        "                try {\n"
        "                    next();\n"
        "                } catch (e) {\n"
        "                    console.error('Error in queued custom elements callback:', e);\n"
        "                }\n"
        "            }\n"
        "        } finally {\n"
        "            this._insideCallback = false;\n"
        "        }\n"
        "    }\n"
        "\n"
        "    define(name, constructor, options) {\n"
        "        name = name.toLowerCase();\n"
        "        if (this._registry.has(name)) {\n"
        "            throw new DOMException('Registration failed: already defined', 'NotSupportedError');\n"
        "        }\n"
        "        const observed = constructor.observedAttributes || [];\n"
        "        this._registry.set(name, {\n"
        "            constructor: constructor,\n"
        "            observedAttributes: new Set(observed.map(a => a.toLowerCase())),\n"
        "            connectedCallback: constructor.prototype.connectedCallback,\n"
        "            disconnectedCallback: constructor.prototype.disconnectedCallback,\n"
        "            adoptedCallback: constructor.prototype.adoptedCallback,\n"
        "            attributeChangedCallback: constructor.prototype.attributeChangedCallback,\n"
        "            extends: options && options.extends ? options.extends.toLowerCase() : null\n"
        "        });\n"
        "        const pending = this._whenDefinedPromises.get(name);\n"
        "        if (pending) {\n"
        "            pending.resolve(constructor);\n"
        "            this._whenDefinedPromises.delete(name);\n"
        "        }\n"
        "        if (globalThis.document && globalThis.document.documentElement) {\n"
        "            this._upgradeAll(globalThis.document.documentElement, name);\n"
        "        }\n"
        "    }\n"
        "\n"
        "    get(name) {\n"
        "        return this._registry.get(name.toLowerCase())?.constructor;\n"
        "    }\n"
        "\n"
        "    whenDefined(name) {\n"
        "        name = name.toLowerCase();\n"
        "        if (this._registry.has(name)) {\n"
        "            return Promise.resolve(this._registry.get(name).constructor);\n"
        "        }\n"
        "        let pending = this._whenDefinedPromises.get(name);\n"
        "        if (!pending) {\n"
        "            let resolveFn;\n"
        "            const promise = new Promise(resolve => { resolveFn = resolve; });\n"
        "            pending = { promise, resolve: resolveFn };\n"
        "            this._whenDefinedPromises.set(name, pending);\n"
        "        }\n"
        "        return pending.promise;\n"
        "    }\n"
        "\n"
        "    upgrade(root) {\n"
        "        this._upgradeAll(root);\n"
        "    }\n"
        "\n"
        "    _upgradeNode(node) {\n"
        "        if (!node || node.nodeType !== 1) return;\n"
        "        if (!node.tagName) return;\n"
        "        const name = node.tagName.toLowerCase();\n"
        "        const definition = this._registry.get(name);\n"
        "        if (definition && !node._upgraded) {\n"
        "            Object.setPrototypeOf(node, definition.constructor.prototype);\n"
        "            node._upgraded = true;\n"
        "            try {\n"
        "                definition.constructor.call(node);\n"
        "            } catch (e) {\n"
        "                console.error('Error in Custom Element constructor:', e);\n"
        "            }\n"
        "            if (this._isConnected(node)) {\n"
        "                this._triggerConnect(node);\n"
        "            }\n"
        "            if (definition.attributeChangedCallback) {\n"
        "                const attrs = node.attributes || [];\n"
        "                for (let i = 0; i < attrs.length; i++) {\n"
        "                    const attr = attrs[i];\n"
        "                    const attrName = attr.name.toLowerCase();\n"
        "                    if (definition.observedAttributes.has(attrName)) {\n"
        "                        try {\n"
        "                            definition.attributeChangedCallback.call(node, attr.name, null, attr.value);\n"
        "                        } catch (e) {\n"
        "                            console.error('Error in attributeChangedCallback:', e);\n"
        "                        }\n"
        "                    }\n"
        "                }\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "\n"
        "    _upgradeAll(root, specificName) {\n"
        "        if (!root) return;\n"
        "        const self = this;\n"
        "        function traverse(node) {\n"
        "            if (node.nodeType === 1) {\n"
        "                const name = node.tagName.toLowerCase();\n"
        "                if (!specificName || name === specificName) {\n"
        "                    self._upgradeNode(node);\n"
        "                }\n"
        "                const children = Array.from(node.childNodes || []);\n"
        "                for (const child of children) {\n"
        "                    traverse(child);\n"
        "                }\n"
        "            }\n"
        "        }\n"
        "        traverse(root);\n"
        "    }\n"
        "\n"
        "    _isConnected(node) {\n"
        "        let curr = node;\n"
        "        while (curr) {\n"
        "            if (curr === document.documentElement) return true;\n"
        "            curr = curr.parentNode || curr.host;\n"
        "        }\n"
        "        return false;\n"
        "    }\n"
        "\n"
        "    _triggerConnect(node) {\n"
        "        const self = this;\n"
        "        function run(curr) {\n"
        "            if (curr.nodeType === 1) {\n"
        "                const name = curr.tagName.toLowerCase();\n"
        "                const definition = self._registry.get(name);\n"
        "                if (definition && definition.connectedCallback) {\n"
        "                    try {\n"
        "                        definition.connectedCallback.call(curr);\n"
        "                    } catch (e) {\n"
        "                        console.error('Error in connectedCallback:', e);\n"
        "                    }\n"
        "                }\n"
        "                const children = Array.from(curr.childNodes || []);\n"
        "                for (const child of children) {\n"
        "                    run(child);\n"
        "                }\n"
        "            }\n"
        "        }\n"
        "        run(node);\n"
        "    }\n"
        "\n"
        "    _triggerDisconnect(node) {\n"
        "        const self = this;\n"
        "        function run(curr) {\n"
        "            if (curr.nodeType === 1) {\n"
        "                const name = curr.tagName.toLowerCase();\n"
        "                const definition = self._registry.get(name);\n"
        "                if (definition && definition.disconnectedCallback) {\n"
        "                    try {\n"
        "                        definition.disconnectedCallback.call(curr);\n"
        "                    } catch (e) {\n"
        "                        console.error('Error in disconnectedCallback:', e);\n"
        "                    }\n"
        "                }\n"
        "                const children = Array.from(curr.childNodes || []);\n"
        "                for (const child of children) {\n"
        "                    run(child);\n"
        "                }\n"
        "            }\n"
        "        }\n"
        "        run(node);\n"
        "    }\n"
        "\n"
        "    __on_connect(node) {\n"
        "        this._queueCallback(() => {\n"
        "            this._upgradeAll(node);\n"
        "            if (this._isConnected(node)) {\n"
        "                this._triggerConnect(node);\n"
        "            }\n"
        "        });\n"
        "    }\n"
        "\n"
        "    __on_disconnect(node) {\n"
        "        this._queueCallback(() => {\n"
        "            this._triggerDisconnect(node);\n"
        "        });\n"
        "    }\n"
        "\n"
        "    __on_adopt(node, oldDoc, newDoc) {\n"
        "        this._queueCallback(() => {\n"
        "            const self = this;\n"
        "            function run(curr) {\n"
        "                if (curr.nodeType === 1) {\n"
        "                    const name = curr.tagName.toLowerCase();\n"
        "                    const definition = self._registry.get(name);\n"
        "                    if (definition && definition.adoptedCallback) {\n"
        "                        try {\n"
        "                            definition.adoptedCallback.call(curr, oldDoc, newDoc);\n"
        "                        } catch (e) {\n"
        "                            console.error('Error in adoptedCallback:', e);\n"
        "                        }\n"
        "                    }\n"
        "                    const children = Array.from(curr.childNodes || []);\n"
        "                    for (const child of children) {\n"
        "                        run(child);\n"
        "                    }\n"
        "                }\n"
        "            }\n"
        "            run(node);\n"
        "        });\n"
        "    }\n"
        "\n"
        "    __on_attr_change(node, attrName, oldValue, newValue) {\n"
        "        this._queueCallback(() => {\n"
        "            attrName = attrName.toLowerCase();\n"
        "            const name = node.tagName.toLowerCase();\n"
        "            const definition = this._registry.get(name);\n"
        "            if (definition && definition.attributeChangedCallback) {\n"
        "                if (definition.observedAttributes.has(attrName)) {\n"
        "                    try {\n"
        "                        definition.attributeChangedCallback.call(node, attrName, oldValue, newValue);\n"
        "                    } catch (e) {\n"
        "                        console.error('Error in attributeChangedCallback:', e);\n"
        "                    }\n"
        "                }\n"
        "            }\n"
        "        });\n"
        "    }\n"
        "}\n"
        "\n"
        "globalThis.CustomElementRegistry = CustomElementRegistry;\n"
        "globalThis.customElements = new CustomElementRegistry();\n"
        "globalThis.__wisp_custom_elements_registry = globalThis.customElements;\n"
        "\n"
        "if (typeof Window !== 'undefined' && Window.prototype) {\n"
        "    Object.defineProperty(Window.prototype, 'customElements', {\n"
        "        value: globalThis.customElements,\n"
        "        writable: true,\n"
        "        configurable: true,\n"
        "        enumerable: true\n"
        "    });\n"
        "}\n"
        "if (typeof window !== 'undefined') {\n"
        "    window.customElements = globalThis.customElements;\n"
        "    window.CustomElementRegistry = CustomElementRegistry;\n"
        "}\n"
        "\n"
        "if (globalThis.Document && globalThis.Document.prototype) {\n"
        "    const origCreateElement = globalThis.Document.prototype.createElement;\n"
        "    globalThis.Document.prototype.createElement = function(tagName, options) {\n"
        "        const el = origCreateElement.call(this, tagName, options);\n"
        "        if (globalThis.customElements) {\n"
        "            globalThis.customElements._upgradeNode(el);\n"
        "        }\n"
        "        return el;\n"
        "    };\n"
        "\n"
        "    const origAdoptNode = globalThis.Document.prototype.adoptNode;\n"
        "    globalThis.Document.prototype.adoptNode = function(node) {\n"
        "        const oldDoc = node.ownerDocument;\n"
        "        const res = origAdoptNode.call(this, node);\n"
        "        if (globalThis.customElements && res) {\n"
        "            globalThis.customElements.__on_adopt(res, oldDoc, this);\n"
        "        }\n"
        "        return res;\n"
        "    };\n"
        "}\n"
        "\n"
        "globalThis.__dispatchNativeDragEvent = function(targetNode, typeStr, mimeTypes, data, allowedEffects) {\n"
        "    if (!targetNode) targetNode = document.body || document.documentElement;\n"
        "    if (!targetNode) return false;\n"
        "\n"
        "    let mode = 'protected';\n"
        "    if (typeStr === 'dragstart') mode = 'readwrite';\n"
        "    else if (typeStr === 'drop') mode = 'readonly';\n"
        "\n"
        "    const evt = new globalThis.DragEvent(typeStr);\n"
        "    const dt = new globalThis.DataTransfer();\n"
        "    globalThis.__initDataTransferInstance(dt, mode);\n"
        "    evt._dataTransfer = dt;\n"
        "\n"
        "    if (allowedEffects === 1) dt.effectAllowed = 'copy';\n"
        "    else if (allowedEffects === 2) dt.effectAllowed = 'move';\n"
        "    else if (allowedEffects === 4) dt.effectAllowed = 'link';\n"
        "    else dt.effectAllowed = 'all';\n"
        "\n"
        "    if (Array.isArray(mimeTypes)) {\n"
        "        for (const type of mimeTypes) {\n"
        "            const payloadData = (typeStr === 'drop') ? data : '';\n"
        "            dt.setData(type, payloadData);\n"
        "        }\n"
        "    }\n"
        "\n"
        "    return targetNode.dispatchEvent(evt);\n"
        "};\n"
        "\n"
        "if (typeof globalThis.matchMedia === 'undefined') {\n"
        "    globalThis.matchMedia = function(query) {\n"
        "        return {\n"
        "            matches: false,\n"
        "            media: query,\n"
        "            onchange: null,\n"
        "            addListener: function() {},\n"
        "            removeListener: function() {},\n"
        "            addEventListener: function() {},\n"
        "            removeEventListener: function() {},\n"
        "            dispatchEvent: function() { return true; }\n"
        "        };\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.ResizeObserver === 'undefined') {\n"
        "    globalThis.ResizeObserver = class ResizeObserver {\n"
        "        constructor(callback) {\n"
        "            this.callback = callback;\n"
        "        }\n"
        "        observe(target, options) {}\n"
        "        unobserve(target) {}\n"
        "        disconnect() {}\n"
        "    };\n"
        "}\n"
        "\n"
        "if (typeof globalThis.scrollTo === 'undefined') {\n"
        "    globalThis.scrollTo = function() {};\n"
        "}\n"
        "if (typeof globalThis.scroll === 'undefined') {\n"
        "    globalThis.scroll = function() {};\n"
        "}\n"
        "if (typeof globalThis.scrollBy === 'undefined') {\n"
        "    globalThis.scrollBy = function() {};\n"
        "}\n"
        "\n"
        "globalThis.CSS = globalThis.CSS || {};\n"
        "globalThis.CSS.supports = function(a, b) {\n"
        "    if (b !== undefined) {\n"
        "        let prop = String(a).trim();\n"
        "        let val = String(b).trim();\n"
        "        let dummy = globalThis.__wisp_dummy_supports_el;\n"
        "        if (!dummy) {\n"
        "            dummy = document.createElement('_');\n"
        "            globalThis.__wisp_dummy_supports_el = dummy;\n"
        "        }\n"
        "        let style = dummy.style;\n"
        "        style.cssText = '';\n"
        "        style[prop] = '';\n"
        "        style[prop] = val;\n"
        "        return style.length > 0;\n"
        "    } else {\n"
        "        let cond = String(a).trim();\n"
        "        function evaluateCondition(text) {\n"
        "            text = text.trim();\n"
        "            if (!text) return false;\n"
        "            if (text.includes(' or ')) {\n"
        "                return text.split(' or ').some(evaluateCondition);\n"
        "            }\n"
        "            if (text.includes(' and ')) {\n"
        "                return text.split(' and ').every(evaluateCondition);\n"
        "            }\n"
        "            if (text.startsWith('not ')) {\n"
        "                return !evaluateCondition(text.substring(4));\n"
        "            }\n"
        "            if (text.startsWith('(') && text.endsWith(')')) {\n"
        "                return evaluateCondition(text.slice(1, -1));\n"
        "            }\n"
        "            if (text.startsWith('selector(')) {\n"
        "                return true;\n"
        "            }\n"
        "            let colonIdx = text.indexOf(':');\n"
        "            if (colonIdx !== -1) {\n"
        "                let p = text.substring(0, colonIdx).trim();\n"
        "                let v = text.substring(colonIdx + 1).trim();\n"
        "                return globalThis.CSS.supports(p, v);\n"
        "            }\n"
        "            return false;\n"
        "        }\n"
        "        return evaluateCondition(cond);\n"
        "    }\n"
        "};\n"
        "\n"
        "(function() {\n"
        "    function setupHandler(proto, prop, eventName) {\n"
        "        if (!proto) return;\n"
        "        try {\n"
        "            Object.defineProperty(proto, prop, {\n"
        "                get() {\n"
        "                    return this['__' + prop + '_func'] || null;\n"
        "                },\n"
        "                set(val) {\n"
        "                    let oldVal = this['__' + prop + '_func'];\n"
        "                    if (oldVal) {\n"
        "                        this.removeEventListener(eventName, oldVal);\n"
        "                    }\n"
        "                    this['__' + prop + '_func'] = val;\n"
        "                    if (typeof val === 'function') {\n"
        "                        this.addEventListener(eventName, val);\n"
        "                    }\n"
        "                },\n"
        "                configurable: true,\n"
        "                enumerable: true\n"
        "            });\n"
        "        } catch (e) {\n"
        "            // ignore\n"
        "        }\n"
        "    }\n"
        "\n"
        "    const events = {\n"
        "        onload: 'load',\n"
        "        onerror: 'error',\n"
        "        onmessage: 'message',\n"
        "        onclick: 'click',\n"
        "        onchange: 'change',\n"
        "        oninput: 'input',\n"
        "        onkeydown: 'keydown',\n"
        "        onkeyup: 'keyup',\n"
        "        onkeypress: 'keypress',\n"
        "        onmousedown: 'mousedown',\n"
        "        onmouseup: 'mouseup',\n"
        "        onmousemove: 'mousemove',\n"
        "        onmouseover: 'mouseover',\n"
        "        onmouseout: 'mouseout',\n"
        "        onsubmit: 'submit',\n"
        "        onreset: 'reset',\n"
        "        onscroll: 'scroll'\n"
        "    };\n"
        "\n"
        "    const protos = [];\n"
        "    if (typeof Window !== 'undefined' && Window.prototype) protos.push(Window.prototype);\n"
        "    if (typeof Document !== 'undefined' && Document.prototype) protos.push(Document.prototype);\n"
        "    if (typeof HTMLElement !== 'undefined' && HTMLElement.prototype) protos.push(HTMLElement.prototype);\n"
        "    if (typeof Element !== 'undefined' && Element.prototype) protos.push(Element.prototype);\n"
        "    protos.push(globalThis);\n"
        "\n"
        "    for (let proto of protos) {\n"
        "        for (let prop in events) {\n"
        "            setupHandler(proto, prop, events[prop]);\n"
        "        }\n"
        "    }\n"
        "    if (globalThis.CustomEvent) {\n"
        "        const OriginalCustomEvent = globalThis.CustomEvent;\n"
        "        globalThis.CustomEvent = function(type, options = {}) {\n"
        "            const evt = new OriginalCustomEvent(type, options);\n"
        "            const detail = options.detail !== undefined ? options.detail : null;\n"
        "            Object.defineProperty(evt, 'detail', { value: detail, configurable: true, enumerable: true });\n"
        "            return evt;\n"
        "        };\n"
        "        globalThis.CustomEvent.prototype = OriginalCustomEvent.prototype;\n"
        "        globalThis.CustomEvent.prototype.initCustomEvent = function(type, bubbles, cancelable, detail) {\n"
        "            this.initEvent(type, bubbles, cancelable);\n"
        "            Object.defineProperty(this, 'detail', { value: detail !== undefined ? detail : null, configurable: true, enumerable: true });\n"
        "        };\n"
        "    }\n"
        "    if (globalThis.Document && globalThis.Document.prototype) {\n"
        "        if (!('readyState' in globalThis.Document.prototype)) {\n"
        "            Object.defineProperty(globalThis.Document.prototype, 'readyState', {\n"
        "                get: function() { return this._readyState || 'complete'; },\n"
        "                set: function(v) { this._readyState = v; },\n"
        "                configurable: true, enumerable: true\n"
        "            });\n"
        "        }\n"
        "        if (!('head' in globalThis.Document.prototype)) {\n"
        "            Object.defineProperty(globalThis.Document.prototype, 'head', {\n"
        "                get: function() { return this.getElementsByTagName('head')[0] || this.documentElement || null; },\n"
        "                configurable: true, enumerable: true\n"
        "            });\n"
        "        }\n"
        "        if (!('body' in globalThis.Document.prototype)) {\n"
        "            Object.defineProperty(globalThis.Document.prototype, 'body', {\n"
        "                get: function() { return this.getElementsByTagName('body')[0] || null; },\n"
        "                configurable: true, enumerable: true\n"
        "            });\n"
        "        }\n"
        "    }\n"
        "    let _jQuery = undefined;\n"
        "    let _dollar = undefined;\n"
        "    if (!('jQuery' in globalThis)) {\n"
        "        Object.defineProperty(globalThis, 'jQuery', {\n"
        "            get() { return _jQuery; },\n"
        "            set(v) { _jQuery = v; if (v && !_dollar) _dollar = v; },\n"
        "            configurable: true, enumerable: true\n"
        "        });\n"
        "    }\n"
        "    if (!('$' in globalThis)) {\n"
        "        Object.defineProperty(globalThis, '$', {\n"
        "            get() { return _dollar || _jQuery; },\n"
        "            set(v) { _dollar = v; if (v && !_jQuery) _jQuery = v; },\n"
        "            configurable: true, enumerable: true\n"
        "        });\n"
        "    }\n"
        "})();\n"
        "";
    JSValue val = JS_Eval(ctx, fetch_polyfill, strlen(fetch_polyfill), "<polyfill>", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(val)) {
        JSValue exc = JS_GetException(ctx);
        const char *exc_str = JS_ToCString(ctx, exc);
        NSLOG(wisp, WARNING, "Error evaluating fetch polyfill: %s", exc_str ? exc_str : "unknown");
        if (exc_str) JS_FreeCString(ctx, exc_str);
        JS_FreeValue(ctx, exc);
    }
    JS_FreeValue(ctx, val);

    const char *forms_polyfill =
        "(() => {\n"
        "    class HTMLOptionsCollectionImpl {\n"
        "        constructor(selectElement) {\n"
        "            this._select = selectElement;\n"
        "        }\n"
        "        _getOptions() {\n"
        "            if (!this._select) return [];\n"
        "            let list = this._select.getElementsByTagName('option');\n"
        "            if (!list) list = [];\n"
        "            const options = [];\n"
        "            for (let i = 0; i < list.length; i++) {\n"
        "                options.push(list[i]);\n"
        "            }\n"
        "            return options;\n"
        "        }\n"
        "        get length() {\n"
        "            let opts = this._getOptions();\n"
        "            return opts ? opts.length : 0;\n"
        "        }\n"
        "        set length(val) {\n"
        "            val = Number(val);\n"
        "            if (isNaN(val) || val < 0) return;\n"
        "            const current = this.length;\n"
        "            if (val < current) {\n"
        "                for (let i = current - 1; i >= val; i--) {\n"
        "                    this.remove(i);\n"
        "                }\n"
        "            }\n"
        "        }\n"
        "        item(index) {\n"
        "            const options = this._getOptions();\n"
        "            if (!options) return null;\n"
        "            return (index >= 0 && index < options.length) ? options[index] : null;\n"
        "        }\n"
        "        namedItem(name) {\n"
        "            if (!name) return null;\n"
        "            name = String(name);\n"
        "            const options = this._getOptions();\n"
        "            if (!options) return null;\n"
        "            for (let opt of options) {\n"
        "                if (opt.getAttribute('id') === name || opt.getAttribute('name') === name) {\n"
        "                    return opt;\n"
        "                }\n"
        "            }\n"
        "            return null;\n"
        "        }\n"
        "        add(element, before = null) {\n"
        "            if (!element || !(element instanceof globalThis.HTMLElement)) {\n"
        "                throw new TypeError(\"Failed to execute 'add' on 'HTMLOptionsCollection': The element provided is not of type 'HTMLElement'.\");\n"
        "            }\n"
        "            if (before === null || before === undefined) {\n"
        "                this._select.appendChild(element);\n"
        "            } else if (typeof before === 'number') {\n"
        "                const target = this.item(before);\n"
        "                if (target) this._select.insertBefore(element, target);\n"
        "                else this._select.appendChild(element);\n"
        "            } else if (before instanceof globalThis.HTMLElement) {\n"
        "                this._select.insertBefore(element, before);\n"
        "            }\n"
        "        }\n"
        "        remove(index) {\n"
        "            const target = this.item(index);\n"
        "            if (target && target.parentNode) {\n"
        "                target.parentNode.removeChild(target);\n"
        "            }\n"
        "        }\n"
        "        get selectedIndex() {\n"
        "            const opts = this._getOptions();\n"
        "            if (!opts) return -1;\n"
        "            for (let i = 0; i < opts.length; i++) {\n"
        "                if (opts[i].selected) return i;\n"
        "            }\n"
        "            return opts.length > 0 ? 0 : -1;\n"
        "        }\n"
        "        set selectedIndex(idx) {\n"
        "            const opts = this._getOptions();\n"
        "            if (!opts) return;\n"
        "            for (let i = 0; i < opts.length; i++) {\n"
        "                opts[i].selected = (i === idx);\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "    const HTMLOptionsCollection = new Proxy(function() {}, {\n"
        "        construct(target, args) {\n"
        "            const impl = new HTMLOptionsCollectionImpl(args[0]);\n"
        "            return new Proxy(impl, {\n"
        "                get(target, prop) {\n"
        "                    if (prop === 'options') return impl._getOptions();\n"
        "                    if (prop === 'selectedIndex') return target.selectedIndex;\n"
        "                    if (prop === 'length') return target.length;\n"
        "                    if (prop === 'add') return target.add.bind(target);\n"
        "                    if (prop === 'remove') return target.remove.bind(target);\n"
        "                    if (prop === 'item') return target.item.bind(target);\n"
        "                    if (prop === 'namedItem') return target.namedItem.bind(target);\n"
        "                    if (prop in target) return typeof target[prop] === 'function' ? target[prop].bind(target) : target[prop];\n"
        "                    if (typeof prop === 'string' && !isNaN(prop)) return target.item(Number(prop));\n"
        "                    return target.namedItem(prop);\n"
        "                },\n"
        "                set(target, prop, value) {\n"
        "                    if (prop === 'selectedIndex') { target.selectedIndex = value; return true; }\n"
        "                    if (prop === 'length') { target.length = value; return true; }\n"
        "                    if (prop in target) { target[prop] = value; return true; }\n"
        "                    if (typeof prop === 'string' && !isNaN(prop)) {\n"
        "                        const index = Number(prop);\n"
        "                        const current = target._getOptions();\n"
        "                        if (value === null || value === undefined) { target.remove(index); return true; }\n"
        "                        if (current && index < current.length) {\n"
        "                            target._select.replaceChild(value, current[index]);\n"
        "                        } else {\n"
        "                            target._select.appendChild(value);\n"
        "                        }\n"
        "                        return true;\n"
        "                    }\n"
        "                    return false;\n"
        "                }\n"
        "            });\n"
        "        }\n"
        "    });\n"
        "    if (globalThis.HTMLSelectElement && HTMLSelectElement.prototype) {\n"
        "        const proto = HTMLSelectElement.prototype;\n"
        "        Object.defineProperty(proto, 'options', {\n"
        "            get() {\n"
        "                if (!this._optionsColl) {\n"
        "                    this._optionsColl = new HTMLOptionsCollection(this);\n"
        "                }\n"
        "                return this._optionsColl;\n"
        "            },\n"
        "            configurable: true, enumerable: true\n"
        "        });\n"
        "        Object.defineProperty(proto, 'selectedIndex', {\n"
        "            get() {\n"
        "                if (!this.options) return -1;\n"
        "                return this.options.selectedIndex;\n"
        "            },\n"
        "            set(val) {\n"
        "                if (this.options) this.options.selectedIndex = val;\n"
        "            },\n"
        "            configurable: true, enumerable: true\n"
        "        });\n"
        "        Object.defineProperty(proto, 'value', {\n"
        "            get() {\n"
        "                const idx = this.selectedIndex;\n"
        "                const opt = this.options ? this.options.item(idx) : null;\n"
        "                return opt ? (opt.value !== undefined ? opt.value : opt.text) : '';\n"
        "            },\n"
        "            set(val) {\n"
        "                val = String(val);\n"
        "                const opts = this.options && this.options.options ? this.options.options : (this.options && this.options._getOptions ? this.options._getOptions() : []);\n"
        "                if (!opts) return;\n"
        "                for (let i=0; i<opts.length; i++) {\n"
        "                    let opt = opts[i];\n"
        "                    if (opt.value === val || opt.text === val) {\n"
        "                        opt.selected = true;\n"
        "                        return;\n"
        "                    }\n"
        "                }\n"
        "            },\n"
        "            configurable: true, enumerable: true\n"
        "        });\n"
        "    }\n"
        "    if (globalThis.HTMLOptionElement && HTMLOptionElement.prototype) {\n"
        "        const proto = HTMLOptionElement.prototype;\n"
        "        Object.defineProperty(proto, 'text', {\n"
        "            get() { return this.textContent || ''; },\n"
        "            set(val) { this.textContent = String(val); },\n"
        "            configurable: true, enumerable: true\n"
        "        });\n"
        "        Object.defineProperty(proto, 'value', {\n"
        "            get() {\n"
        "                return this.hasAttribute('value') ? this.getAttribute('value') : this.text;\n"
        "            },\n"
        "            set(val) { this.setAttribute('value', String(val)); },\n"
        "            configurable: true, enumerable: true\n"
        "        });\n"
        "        Object.defineProperty(proto, 'selected', {\n"
        "            get() { return this._selected !== undefined ? this._selected : this.hasAttribute('selected'); },\n"
        "            set(val) { this._selected = !!val; },\n"
        "            configurable: true, enumerable: true\n"
        "        });\n"
        "    }\n"
        "    globalThis.HTMLOptionsCollection = HTMLOptionsCollection;\n"
        "    globalThis.Option = function(text = '', value = '', defaultSelected = false, selected = false) {\n"
        "        const opt = document.createElement('option');\n"
        "        if (text) opt.text = String(text);\n"
        "        if (value) opt.value = String(value);\n"
        "        if (defaultSelected) opt.defaultSelected = true;\n"
        "        if (selected) opt.selected = true;\n"
        "        return opt;\n"
        "    };\n"
        "})();\n";
    JSValue forms_val = JS_Eval(ctx, forms_polyfill, strlen(forms_polyfill), "forms_polyfill.js", JS_EVAL_TYPE_GLOBAL);
    JS_FreeValue(ctx, forms_val);

    const char *html5_polyfills =
        "/* Web Audio API */\n"
        "if (typeof globalThis.AudioContext === 'undefined') {\n"
        "    globalThis.AudioContext = class AudioContext extends globalThis.EventTarget {\n"
        "        constructor() { super(); this.state = 'running'; this.sampleRate = 44100; this.destination = {}; }\n"
        "        createGain() { return { gain: { value: 1 }, connect() {}, disconnect() {} }; }\n"
        "        createOscillator() { return { frequency: { value: 440 }, start() {}, stop() {}, connect() {}, disconnect() {} }; }\n"
        "        createBufferSource() { return { buffer: null, start() {}, stop() {}, connect() {}, disconnect() {} }; }\n"
        "        createBuffer(numChannels, length, sampleRate) { return { numberOfChannels: numChannels, length: length, sampleRate: sampleRate, getChannelData() { return new Float32Array(length); } }; }\n"
        "        decodeAudioData(data, success, error) { const buf = this.createBuffer(2, 100, 44100); if (success) success(buf); return Promise.resolve(buf); }\n"
        "        close() { this.state = 'closed'; return Promise.resolve(); }\n"
        "        suspend() { this.state = 'suspended'; return Promise.resolve(); }\n"
        "        resume() { this.state = 'running'; return Promise.resolve(); }\n"
        "    };\n"
        "    globalThis.webkitAudioContext = globalThis.AudioContext;\n"
        "}\n"
        "if (typeof globalThis.OfflineAudioContext === 'undefined') {\n"
        "    globalThis.OfflineAudioContext = class OfflineAudioContext extends globalThis.AudioContext {};\n"
        "}\n"
        "\n"
        "/* Generic Sensor API */\n"
        "if (typeof globalThis.Sensor === 'undefined') {\n"
        "    globalThis.Sensor = class Sensor extends globalThis.EventTarget {};\n"
        "    globalThis.Accelerometer = class Accelerometer extends globalThis.Sensor {};\n"
        "    globalThis.Gyroscope = class Gyroscope extends globalThis.Sensor {};\n"
        "    globalThis.Magnetometer = class Magnetometer extends globalThis.Sensor {};\n"
        "    globalThis.AmbientLightSensor = class AmbientLightSensor extends globalThis.Sensor {};\n"
        "    globalThis.LinearAccelerationSensor = class LinearAccelerationSensor extends globalThis.Sensor {};\n"
        "    globalThis.AbsoluteOrientationSensor = class AbsoluteOrientationSensor extends globalThis.Sensor {};\n"
        "    globalThis.RelativeOrientationSensor = class RelativeOrientationSensor extends globalThis.Sensor {};\n"
        "}\n"
        "\n"
        "/* Web Bluetooth & Web USB */\n"
        "if (globalThis.navigator && !globalThis.navigator.bluetooth) {\n"
        "    globalThis.navigator.bluetooth = { getAvailability: () => Promise.resolve(false), requestDevice: () => Promise.reject(new Error('Bluetooth unsupported')) };\n"
        "}\n"
        "if (typeof globalThis.BluetoothDevice === 'undefined') {\n"
        "    globalThis.BluetoothDevice = class BluetoothDevice extends globalThis.EventTarget {};\n"
        "}\n"
        "if (globalThis.navigator && !globalThis.navigator.usb) {\n"
        "    globalThis.navigator.usb = { getDevices: () => Promise.resolve([]), requestDevice: () => Promise.reject(new Error('USB unsupported')) };\n"
        "}\n"
        "if (typeof globalThis.USB === 'undefined') {\n"
        "    globalThis.USB = class USB extends globalThis.EventTarget {};\n"
        "}\n"
        "if (typeof globalThis.USBDevice === 'undefined') {\n"
        "    globalThis.USBDevice = class USBDevice {};\n"
        "}\n"
        "\n"
        "/* Fullscreen API */\n"
        "if (typeof Element !== 'undefined' && Element.prototype) {\n"
        "    if (!Element.prototype.requestFullscreen) Element.prototype.requestFullscreen = function() { return Promise.resolve(); };\n"
        "    if (!Element.prototype.webkitRequestFullscreen) Element.prototype.webkitRequestFullscreen = Element.prototype.requestFullscreen;\n"
        "    if (!Element.prototype.mozRequestFullScreen) Element.prototype.mozRequestFullScreen = Element.prototype.requestFullscreen;\n"
        "    if (!Element.prototype.msRequestFullscreen) Element.prototype.msRequestFullscreen = Element.prototype.requestFullscreen;\n"
        "}\n"
        "if (typeof Document !== 'undefined' && Document.prototype) {\n"
        "    if (!('fullscreenElement' in Document.prototype)) Object.defineProperty(Document.prototype, 'fullscreenElement', { get() { return null; }, configurable: true });\n"
        "    if (!Document.prototype.exitFullscreen) Document.prototype.exitFullscreen = function() { return Promise.resolve(); };\n"
        "    if (!Document.prototype.webkitExitFullscreen) Document.prototype.webkitExitFullscreen = Document.prototype.exitFullscreen;\n"
        "    if (!Document.prototype.mozCancelFullScreen) Document.prototype.mozCancelFullScreen = Document.prototype.exitFullscreen;\n"
        "}\n"
        "\n"
        "/* Web Notifications */\n"
        "if (typeof globalThis.Notification === 'undefined') {\n"
        "    globalThis.Notification = class Notification extends globalThis.EventTarget {\n"
        "        static requestPermission(cb) { if (cb) cb('granted'); return Promise.resolve('granted'); }\n"
        "        static get permission() { return 'granted'; }\n"
        "    };\n"
        "}\n"
        "\n"
        "/* Pointer Events & Pointer Lock */\n"
        "if (typeof globalThis.PointerEvent === 'undefined') {\n"
        "    globalThis.PointerEvent = class PointerEvent extends globalThis.MouseEvent {};\n"
        "}\n"
        "if (typeof Element !== 'undefined' && Element.prototype && !Element.prototype.requestPointerLock) {\n"
        "    Element.prototype.requestPointerLock = function() {};\n"
        "}\n"
        "if (typeof Document !== 'undefined' && Document.prototype && !('pointerLockElement' in Document.prototype)) {\n"
        "    Object.defineProperty(Document.prototype, 'pointerLockElement', { get() { return null; }, configurable: true });\n"
        "}\n"
        "\n"
        "/* Media Source Extensions (MSE) */\n"
        "if (typeof globalThis.MediaSource === 'undefined') {\n"
        "    globalThis.MediaSource = class MediaSource extends globalThis.EventTarget {\n"
        "        static isTypeSupported(type) { return true; }\n"
        "        addSourceBuffer(type) { return { appendBuffer() {}, remove() {}, addEventListener() {}, removeEventListener() {} }; }\n"
        "        endOfStream() {}\n"
        "    };\n"
        "}\n"
        "\n"
        "/* Web Animations API */\n"
        "if (typeof Element !== 'undefined' && Element.prototype && !Element.prototype.animate) {\n"
        "    Element.prototype.animate = function(keyframes, options) {\n"
        "        return { play() {}, cancel() {}, finish() {}, pause() {}, finished: Promise.resolve(), ready: Promise.resolve() };\n"
        "    };\n"
        "}\n"
        "\n"
        "/* Beacon API */\n"
        "if (globalThis.navigator && typeof globalThis.navigator.sendBeacon !== 'function') {\n"
        "    globalThis.navigator.sendBeacon = function(url, data) { return true; };\n"
        "}\n"
        "\n"
        "/* ObjectRTC & MediaRecorder */\n"
        "if (typeof globalThis.RTCIceTransport === 'undefined') {\n"
        "    globalThis.RTCIceTransport = class RTCIceTransport extends globalThis.EventTarget {};\n"
        "}\n"
        "if (typeof globalThis.MediaRecorder === 'undefined') {\n"
        "    globalThis.MediaRecorder = class MediaRecorder extends globalThis.EventTarget {\n"
        "        static isTypeSupported(type) { return true; }\n"
        "        start() {}\n"
        "        stop() {}\n"
        "        pause() {}\n"
        "        resume() {}\n"
        "    };\n"
        "}\n"
        "\n"
        "/* Clipboard API */\n"
        "if (globalThis.navigator && !globalThis.navigator.clipboard) {\n"
        "    globalThis.navigator.clipboard = { readText: () => Promise.resolve(''), writeText: (t) => Promise.resolve() };\n"
        "}\n"
        "if (typeof globalThis.ClipboardEvent === 'undefined') {\n"
        "    globalThis.ClipboardEvent = class ClipboardEvent extends globalThis.Event {};\n"
        "}\n"
        "\n"
        "/* WebAssembly */\n"
        "if (typeof globalThis.WebAssembly === 'undefined') {\n"
        "    globalThis.WebAssembly = {\n"
        "        compile: () => Promise.resolve({}),\n"
        "        instantiate: () => Promise.resolve({ module: {}, instance: { exports: {} } }),\n"
        "        validate: () => true,\n"
        "        Module: class Module {},\n"
        "        Instance: class Instance {},\n"
        "        Memory: class Memory { constructor(init) { this.buffer = new ArrayBuffer((init && init.initial) ? init.initial * 65536 : 65536); } },\n"
        "        Table: class Table { constructor(init) { this.length = (init && init.initial) || 0; } }\n"
        "    };\n"
        "}\n"
        "\n"
        "/* Font Loader API */\n"
        "if (typeof globalThis.FontFace === 'undefined') {\n"
        "    globalThis.FontFace = class FontFace {\n"
        "        constructor(family, source, descriptors) { this.family = family; this.status = 'loaded'; }\n"
        "        load() { return Promise.resolve(this); }\n"
        "    };\n"
        "}\n"
        "if (typeof document !== 'undefined' && !document.fonts) {\n"
        "    document.fonts = {\n"
        "        add() {}, delete() {}, clear() {}, check() { return true; }, load() { return Promise.resolve([]); }, ready: Promise.resolve(),\n"
        "        addEventListener() {}, removeEventListener() {}\n"
        "    };\n"
        "}\n"
        "\n"
        "/* ServiceWorkers & Push Messages */\n"
        "if (globalThis.navigator && !globalThis.navigator.serviceWorker) {\n"
        "    globalThis.navigator.serviceWorker = new (class ServiceWorkerContainer extends globalThis.EventTarget {\n"
        "        register() { return Promise.resolve({ active: {}, installing: null, waiting: null }); }\n"
        "        getRegistration() { return Promise.resolve(null); }\n"
        "        getRegistrations() { return Promise.resolve([]); }\n"
        "    })();\n"
        "}\n"
        "if (typeof globalThis.PushManager === 'undefined') {\n"
        "    globalThis.PushManager = class PushManager {\n"
        "        subscribe() { return Promise.resolve({}); }\n"
        "        getSubscription() { return Promise.resolve(null); }\n"
        "        permissionState() { return Promise.resolve('granted'); }\n"
        "    };\n"
        "}\n"
        "\n"
        "/* Web SQL Database */\n"
        "if (typeof globalThis.openDatabase === 'undefined') {\n"
        "    globalThis.openDatabase = function(name, version, displayName, estimatedSize) {\n"
        "        return {\n"
        "            transaction(callback) {\n"
        "                if (callback) callback({ executeSql(sql, args, success) { if (success) success(null, { rows: [] }); } });\n"
        "            }\n"
        "        };\n"
        "    };\n"
        "}\n"
        "\n"
        "/* Page Visibility API */\n"
        "if (typeof document !== 'undefined') {\n"
        "    if (!('visibilityState' in document)) Object.defineProperty(document, 'visibilityState', { get() { return 'visible'; }, configurable: true });\n"
        "    if (!('hidden' in document)) Object.defineProperty(document, 'hidden', { get() { return false; }, configurable: true });\n"
        "}\n"
        "\n"
        "/* Selection API */\n"
        "if (typeof window !== 'undefined' && !window.getSelection) {\n"
        "    window.getSelection = function() {\n"
        "        return {\n"
        "            toString() { return ''; },\n"
        "            removeAllRanges() {},\n"
        "            addRange() {},\n"
        "            getRangeAt() { return { startOffset: 0, endOffset: 0, collapse() {} }; },\n"
        "            rangeCount: 0\n"
        "        };\n"
        "    };\n"
        "}\n"
        "\n"
        "/* scrollIntoView */\n"
        "if (typeof Element !== 'undefined' && Element.prototype && !Element.prototype.scrollIntoView) {\n"
        "    Element.prototype.scrollIntoView = function() {};\n"
        "}\n"
        "\n"
        "/* OffscreenCanvas & WebGL / WebGL2 Context Classes */\n"
        "if (typeof globalThis.OffscreenCanvas === 'undefined') {\n"
        "    globalThis.OffscreenCanvas = class OffscreenCanvas extends globalThis.EventTarget {\n"
        "        constructor(w, h) { super(); this.width = w || 300; this.height = h || 150; }\n"
        "        getContext(type) {\n"
        "            return {\n"
        "                drawImage() {}, fillRect() {}, clearRect() {}, convertToBlob() { return Promise.resolve(new Blob()); }\n"
        "            };\n"
        "        }\n"
        "    };\n"
        "}\n"
        "if (typeof globalThis.WebGLRenderingContext === 'undefined') {\n"
        "    globalThis.WebGLRenderingContext = class WebGLRenderingContext {};\n"
        "}\n"
        "if (typeof globalThis.WebGL2RenderingContext === 'undefined') {\n"
        "    globalThis.WebGL2RenderingContext = class WebGL2RenderingContext {};\n"
        "}\n"
        "if (globalThis.navigator && !globalThis.navigator.xr) {\n"
        "    globalThis.navigator.xr = { isSessionSupported: () => Promise.resolve(false), requestSession: () => Promise.reject() };\n"
        "}\n"
        "if (typeof globalThis.XRSession === 'undefined') {\n"
        "    globalThis.XRSession = class XRSession {};\n"
        "}\n"
        "\n"
        "/* Speech Recognition */\n"
        "if (typeof globalThis.SpeechRecognition === 'undefined') {\n"
        "    globalThis.SpeechRecognition = class SpeechRecognition extends globalThis.EventTarget {\n"
        "        start() {}\n"
        "        stop() {}\n"
        "        abort() {}\n"
        "    };\n"
        "    globalThis.webkitSpeechRecognition = globalThis.SpeechRecognition;\n"
        "}\n"
        "\n"
        "/* Payment Request API */\n"
        "if (typeof globalThis.PaymentRequest === 'undefined') {\n"
        "    globalThis.PaymentRequest = class PaymentRequest extends globalThis.EventTarget {\n"
        "        constructor(methods, details) { super(); }\n"
        "        show() { return Promise.resolve({ complete() { return Promise.resolve(); } }); }\n"
        "        canMakePayment() { return Promise.resolve(true); }\n"
        "    };\n"
        "}\n"
        "\n"
        "/* Security CSP / CORS / Subresource Integrity / Authentication / Credentials */\n"
        "if (typeof globalThis.SecurityPolicyViolationEvent === 'undefined') {\n"
        "    globalThis.SecurityPolicyViolationEvent = class SecurityPolicyViolationEvent extends globalThis.Event {};\n"
        "}\n"
        "if (typeof globalThis.PublicKeyCredential === 'undefined') {\n"
        "    globalThis.PublicKeyCredential = class PublicKeyCredential extends globalThis.EventTarget {};\n"
        "}\n"
        "if (typeof globalThis.CredentialsContainer === 'undefined') {\n"
        "    globalThis.CredentialsContainer = class CredentialsContainer {};\n"
        "}\n"
        "if (globalThis.navigator && !globalThis.navigator.credentials) {\n"
        "    globalThis.navigator.credentials = new globalThis.CredentialsContainer();\n"
        "}\n"
        "if (typeof globalThis.ImageBitmap === 'undefined') {\n"
        "    globalThis.ImageBitmap = class ImageBitmap {};\n"
        "}\n"
        "if (globalThis.XMLHttpRequest && XMLHttpRequest.prototype) {\n"
        "    if (!('upload' in XMLHttpRequest.prototype)) {\n"
        "        Object.defineProperty(XMLHttpRequest.prototype, 'upload', {\n"
        "            get() { if (!this._upload) this._upload = new globalThis.EventTarget(); return this._upload; },\n"
        "            configurable: true\n"
        "        });\n"
        "    }\n"
        "    if (!('withCredentials' in XMLHttpRequest.prototype)) {\n"
        "        Object.defineProperty(XMLHttpRequest.prototype, 'withCredentials', {\n"
        "            get() { return this._withCredentials || false; },\n"
        "            set(v) { this._withCredentials = !!v; },\n"
        "            configurable: true\n"
        "        });\n"
        "    }\n"
        "}\n"
        "/* Subresource Integrity & relList */\n"
        "if (typeof DOMTokenList !== 'undefined' && DOMTokenList.prototype) {\n"
        "    DOMTokenList.prototype.supports = function(type) { return ['preload', 'prefetch', 'dns-prefetch', 'preconnect'].includes(String(type).toLowerCase()); };\n"
        "}\n"
        "function makeRelList() {\n"
        "    let list = new globalThis.DOMTokenList();\n"
        "    list.supports = function(type) { return ['preload', 'prefetch', 'dns-prefetch', 'preconnect'].includes(String(type).toLowerCase()); };\n"
        "    return list;\n"
        "}\n"
        "if (typeof HTMLLinkElement !== 'undefined' && HTMLLinkElement.prototype) {\n"
        "    if (!('integrity' in HTMLLinkElement.prototype)) Object.defineProperty(HTMLLinkElement.prototype, 'integrity', { get() { return this.getAttribute('integrity') || ''; }, set(v) { this.setAttribute('integrity', v); }, configurable: true });\n"
        "    if (!('relList' in HTMLLinkElement.prototype)) Object.defineProperty(HTMLLinkElement.prototype, 'relList', { get() { return makeRelList(); }, configurable: true });\n"
        "}\n"
        "if (typeof HTMLScriptElement !== 'undefined' && HTMLScriptElement.prototype) {\n"
        "    if (!('integrity' in HTMLScriptElement.prototype)) Object.defineProperty(HTMLScriptElement.prototype, 'integrity', { get() { return this.getAttribute('integrity') || ''; }, set(v) { this.setAttribute('integrity', v); }, configurable: true });\n"
        "}\n"
        "if (typeof HTMLAnchorElement !== 'undefined' && HTMLAnchorElement.prototype) {\n"
        "    if (!('relList' in HTMLAnchorElement.prototype)) Object.defineProperty(HTMLAnchorElement.prototype, 'relList', { get() { return makeRelList(); }, configurable: true });\n"
        "}\n";
    JSValue h5_val = JS_Eval(ctx, html5_polyfills, strlen(html5_polyfills), "html5_polyfills.js", JS_EVAL_TYPE_GLOBAL);
    JS_FreeValue(ctx, h5_val);

}

static void qjs_lifecycle_mutation_hook(dom_mutation_hook_category category, dom_mutation_type type,
    struct dom_node *target, struct dom_node *related, struct dom_string *prev_value, struct dom_string *new_value,
    struct dom_string *attr_name, struct dom_string *attr_ns, void *pw)
{
    jsthread *t = pw;
    if (!t || t->closed || !t->ctx)
        return;
    JSContext *ctx = t->ctx;

    JSValue global = JS_GetGlobalObject(ctx);
    JSValue registry = JS_GetPropertyStr(ctx, global, "__wisp_custom_elements_registry");
    if (JS_IsUndefined(registry) || JS_IsNull(registry)) {
        JS_FreeValue(ctx, global);
        JS_FreeValue(ctx, registry);
        return;
    }

    if (category == DOM_MUTATION_HOOK_CHILD_LIST) {
        if (related) {
            JSValue js_node = qjs_wrap_node(ctx, related);
            if (JS_IsObject(js_node)) {
                if (type == DOM_MUTATION_ADDITION) {
                    JSValue on_connect = JS_GetPropertyStr(ctx, registry, "__on_connect");
                    if (JS_IsFunction(ctx, on_connect)) {
                        JSValue ret = JS_Call(ctx, on_connect, JS_UNDEFINED, 1, &js_node);
                        JS_FreeValue(ctx, ret);
                    }
                    JS_FreeValue(ctx, on_connect);
                } else if (type == DOM_MUTATION_REMOVAL) {
                    JSValue on_disconnect = JS_GetPropertyStr(ctx, registry, "__on_disconnect");
                    if (JS_IsFunction(ctx, on_disconnect)) {
                        JSValue ret = JS_Call(ctx, on_disconnect, JS_UNDEFINED, 1, &js_node);
                        JS_FreeValue(ctx, ret);
                    }
                    JS_FreeValue(ctx, on_disconnect);
                }
            }
            JS_FreeValue(ctx, js_node);
        }
    } else if (category == DOM_MUTATION_HOOK_ATTRIBUTES) {
        if (target && attr_name) {
            JSValue js_target = qjs_wrap_node(ctx, target);
            if (JS_IsObject(js_target)) {
                JSValue on_attr_change = JS_GetPropertyStr(ctx, registry, "__on_attr_change");
                if (JS_IsFunction(ctx, on_attr_change)) {
                    const char *attr_name_cstr = (const char *)dom_string_data(attr_name);
                    size_t attr_name_len = dom_string_byte_length(attr_name);
                    JSValue js_attr_name = JS_NewStringLen(ctx, attr_name_cstr, attr_name_len);

                    JSValue js_prev = JS_NULL;
                    if (prev_value) {
                        js_prev = JS_NewStringLen(
                            ctx, (const char *)dom_string_data(prev_value), dom_string_byte_length(prev_value));
                    }
                    JSValue js_new = JS_NULL;
                    if (new_value) {
                        js_new = JS_NewStringLen(
                            ctx, (const char *)dom_string_data(new_value), dom_string_byte_length(new_value));
                    }

                    JSValue args[4] = {js_target, js_attr_name, js_prev, js_new};
                    JSValue ret = JS_Call(ctx, on_attr_change, JS_UNDEFINED, 4, args);
                    JS_FreeValue(ctx, ret);

                    JS_FreeValue(ctx, js_attr_name);
                    JS_FreeValue(ctx, js_prev);
                    JS_FreeValue(ctx, js_new);
                }
                JS_FreeValue(ctx, on_attr_change);
            }
            JS_FreeValue(ctx, js_target);
        }
    }

    JS_FreeValue(ctx, registry);
    JS_FreeValue(ctx, global);
}

static JSValue global_document_get(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (t) {
        struct dom_document *doc_node = qjs_thread_get_document(t);
        if (doc_node) {
            return qjs_wrap_node(ctx, (dom_node *)doc_node);
        }
    }
    return JS_NULL;
}

nserror js_newthread(jsheap *heap, void *win_priv, void *doc_priv, jsthread **thread)
{
    JS_UpdateStackTop(heap->rt);
    jsthread *t = calloc(1, sizeof(*t));
    if (!t)
        return NSERROR_NOMEM;
    JS_UpdateStackTop(heap->rt);
    t->ctx = JS_NewContext(heap->rt);
    if (!t->ctx) {
        free(t);
        return NSERROR_NOMEM;
    }
    t->heap = heap;
    t->win_priv = win_priv;
    JS_SetContextOpaque(t->ctx, t);

    char origin_buf[256];
    resolve_origin_from_content(win_priv, doc_priv, origin_buf, sizeof(origin_buf));
    t->origin = strdup(origin_buf);
    if (!t->origin) {
        JS_FreeContext(t->ctx);
        free(t);
        return NSERROR_NOMEM;
    }
    ensure_js_process_for_origin(t->origin);

    /* Map shared memory segment for the thread context */
    snprintf(t->shm_dom_name, sizeof(t->shm_dom_name), "/wisp_shm_dom_%u", (unsigned int)(uintptr_t)t);
    uint32_t cap = SHM_DOM_MAX_NODES;
    if (win_priv && win_priv != doc_priv) {
        struct browser_window *bw = (struct browser_window *)win_priv;
        if (bw->browser_window_type == BROWSER_WINDOW_IFRAME || bw->browser_window_type == BROWSER_WINDOW_FRAME) {
            cap = 512;
        } else {
            cap = 1024;
        }
    } else {
        cap = 1024;
    }
    t->shm_capacity = cap;
    t->shm_dom = shm_dom_create(t->shm_dom_name, t->shm_capacity, true);
    current_thread_shm = t->shm_dom;

    /* Bridge must be initialized first */
    if (qjs_init_dom_bridge(t->ctx) != 0) {
        js_destroythread(t);
        return NSERROR_NOMEM;
    }

    /* Core registration handles skeleton creation and prototype inheritance */
    wisp_js_register_all_bindings(t->ctx);

    /* Manual refinements to prototypes must come after registration */
    if (qjs_init_eventtarget(t->ctx) != 0 || qjs_init_event(t->ctx) != 0 || qjs_init_node(t->ctx) != 0 ||
        qjs_init_element(t->ctx) != 0 || qjs_init_document(t->ctx) != 0 || qjs_init_window(t->ctx) != 0 ||
        qjs_init_console(t->ctx) != 0 || qjs_init_timers(t->ctx) != 0 || qjs_init_crypto(t->ctx) != 0 ||
        qjs_init_navigator(t->ctx) != 0 || qjs_init_location(t->ctx) != 0 || qjs_init_storage(t->ctx) != 0 ||
        qjs_init_xmlhttprequest(t->ctx) != 0 || qjs_init_mutationobserver(t->ctx) != 0 ||
        qjs_init_intersectionobserver(t->ctx) != 0 || qjs_init_imagedata(t->ctx) != 0 || qjs_init_canvas(t->ctx) != 0 ||
        qjs_init_trusted_types(t->ctx) != 0) {
        js_destroythread(t);
        return NSERROR_NOMEM;
    }

    JSValue global_obj = JS_GetGlobalObject(t->ctx);
    t->global_window_priv.magic = QJS_DOM_MAGIC;
    t->global_window_priv.node = win_priv;
    t->global_window_priv.ctx = t->ctx;
    t->global_window_priv.is_dom_node = false;

    JSValue window_proto = JS_GetClassProto(t->ctx, qjs_window_class_id);
    if (JS_IsObject(window_proto))
        JS_SetPrototype(t->ctx, global_obj, window_proto);
    JS_FreeValue(t->ctx, window_proto);

    JS_DefinePropertyValueStr(t->ctx, global_obj, "window", JS_DupValue(t->ctx, global_obj), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(t->ctx, global_obj, "self", JS_DupValue(t->ctx, global_obj), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(t->ctx, global_obj, "parent", JS_DupValue(t->ctx, global_obj), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(t->ctx, global_obj, "top", JS_DupValue(t->ctx, global_obj), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(t->ctx, global_obj, "frames", JS_DupValue(t->ctx, global_obj), JS_PROP_C_W_E);

    /* Define 'document' accessor on the global object */
    JSAtom doc_atom = JS_NewAtom(t->ctx, "document");
    JSValue doc_getter = JS_NewCFunction(t->ctx, global_document_get, "get_document", 0);
    JS_DefinePropertyGetSet(
        t->ctx, global_obj, doc_atom, doc_getter, JS_UNDEFINED, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(t->ctx, doc_atom);

    if (doc_priv) {
        t->doc_priv = doc_priv;
        struct dom_document *doc_node = qjs_thread_get_document(t);
        if (doc_node) {
            dom_node_ref((dom_node *)doc_node);
            dom_document_set_mutation_hook(doc_node, qjs_lifecycle_mutation_hook, t);
        }
    }

    JS_FreeValue(t->ctx, global_obj);
    // Register C native polyfills before JS environment is populated
    wisp_qjs_register_core_polyfills(t->ctx);
    qjs_inject_dom_polyfills(t->ctx);
    qjs_apply_csp_eval_restrictions(t->ctx);

    /* Link thread into the heap's active threads list */
    t->next_in_heap = heap->threads;
    heap->threads = t;

    *thread = t;

    return NSERROR_OK;
}

extern int qjs_init_dedicatedworkerglobalscope(JSContext *ctx);

nserror qjs_init_worker_thread(WispWorkerHandle *h, jsthread **thread_out)
{
    jsthread *t = calloc(1, sizeof(*t));
    if (!t)
        return NSERROR_NOMEM;

    JSRuntime *rt = JS_NewRuntime();
    if (!rt) {
        free(t);
        return NSERROR_NOMEM;
    }
    JS_SetMemoryLimit(rt, 128 * 1024 * 1024); // Increased to 128MB
    JS_SetMaxStackSize(rt, 16384 * 1024); // Increased to 16MB
    JS_SetModuleLoaderFunc(rt, wisp_module_normalize, wisp_module_loader, NULL);

    t->ctx = JS_NewContext(rt);
    if (!t->ctx) {
        JS_FreeRuntime(rt);
        free(t);
        return NSERROR_NOMEM;
    }

    t->is_worker = true;
    t->worker_handle = h;
    JS_SetContextOpaque(t->ctx, t);

    char origin_buf[256];
    pthread_mutex_lock(&js_processes_mutex);
    uint32_t val = ++null_origin_counter;
    pthread_mutex_unlock(&js_processes_mutex);
    snprintf(origin_buf, sizeof(origin_buf), "null-worker-%u", val);
    t->origin = strdup(origin_buf);
    if (!t->origin) {
        JS_FreeContext(t->ctx);
        JS_FreeRuntime(rt);
        free(t);
        return NSERROR_NOMEM;
    }
    ensure_js_process_for_origin(t->origin);

    JS_SetRuntimeOpaque(rt, t);

    /* DedicatedWorkerGlobalScope doesn't need the full DOM bridge,
     * but it needs basic infrastructure. */
    wisp_js_register_all_bindings(t->ctx);

    qjs_init_dedicatedworkerglobalscope(t->ctx);
    qjs_init_console(t->ctx);
    qjs_init_crypto(t->ctx);

    /* Self-reference in worker global */
    JSValue global = JS_GetGlobalObject(t->ctx);
    JS_DefinePropertyValueStr(t->ctx, global, "self", JS_DupValue(t->ctx, global), JS_PROP_C_W_E);
    JS_FreeValue(t->ctx, global);

    qjs_inject_dom_polyfills(t->ctx);

    *thread_out = t;
    return NSERROR_OK;
}

nserror js_closethread(jsthread *thread)
{
    if (thread)
        thread->closed = true;
    return NSERROR_OK;
}

void js_destroythread(jsthread *thread)
{
    if (!thread)
        return;
    thread->closed = true;

    /* Unlink thread from heap's active threads list if heap is still valid */
    if (thread->heap != NULL) {
        struct jsthread **curr = &thread->heap->threads;
        while (*curr != NULL) {
            if (*curr == thread) {
                *curr = thread->next_in_heap;
                break;
            }
            curr = &(*curr)->next_in_heap;
        }
        thread->heap = NULL;
    }

    if (thread->ctx) {
        JSRuntime *rt = JS_GetRuntime(thread->ctx);
        JSContext *ctx1;
        int job_ret;
        while ((job_ret = JS_ExecutePendingJob(rt, &ctx1)) != 0) {
            if (job_ret < 0) {
                JSValue exc = JS_GetException(ctx1);
                const char *exc_str = JS_ToCString(ctx1, exc);
                NSLOG(wisp, WARNING, "JS Error in microtask during teardown: %s", exc_str ? exc_str : "unknown");
                if (exc_str)
                    JS_FreeCString(ctx1, exc_str);
                JS_FreeValue(ctx1, exc);
            }
        }
    }

    struct qjs_timer *tim = thread->timers;
    thread->timers = NULL;
    while (tim) {
        struct qjs_timer *next = tim->next;
        if (!tim->cancelled && guit && guit->misc && guit->misc->schedule) {
            /* Unscheduling uses -1 as the signal to the scheduler to drop the task.
             * Note: qjs_timer_callback checks tim->cancelled to perform cleanup if fired. */
            guit->misc->schedule(-1, qjs_timer_callback, tim);
        }
        if (thread->ctx) {
            JS_FreeValue(thread->ctx, tim->func);
            JS_FreeValue(thread->ctx, tim->arguments);
        }
        free(tim);
        tim = next;
    }

    struct qjs_raf_callback *raf = thread->raf_callbacks;
    thread->raf_callbacks = NULL;
    while (raf) {
        struct qjs_raf_callback *next = raf->next;
        if (guit && guit->misc && guit->misc->schedule) {
            guit->misc->schedule(-1, qjs_raf_callback_fn, raf);
        }
        if (thread->ctx) {
            JS_FreeValue(thread->ctx, raf->func);
        }
        free(raf);
        raf = next;
    }

    struct qjs_idle_callback *idle = thread->idle_callbacks;
    thread->idle_callbacks = NULL;
    while (idle) {
        struct qjs_idle_callback *next = idle->next;
        if (guit && guit->misc && guit->misc->schedule) {
            guit->misc->schedule(-1, qjs_idle_callback_fn, idle);
        }
        if (thread->ctx) {
            JS_FreeValue(thread->ctx, idle->func);
        }
        free(idle);
        idle = next;
    }

    struct qjs_event_listener_ctx *l = thread->listeners;
    thread->listeners = NULL;
    while (l) {
        struct qjs_event_listener_ctx *next = l->next;
        if (!wisp_is_js_process && l->target) {
            dom_event_target_remove_event_listener(l->target, l->type, l->listener, false);
            dom_node_unref((struct dom_node *)l->target);
            dom_string_unref(l->type);
            dom_event_listener_unref(l->listener);
        }
        if (thread->ctx) {
            JS_FreeValue(thread->ctx, l->func);
        }
        free(l);
        l = next;
    }

    struct qjs_event_map *e = thread->events;
    thread->events = NULL;
    while (e) {
        struct qjs_event_map *next = e->next;
        if (thread->ctx) {
            JS_FreeValue(thread->ctx, e->js_evt);
        }
        if (!wisp_is_js_process && e->evt) {
            dom_event_unref(e->evt);
        }
        free(e);
        e = next;
    }

    /* Break XMLHttpRequest cycles and orphan them */
    struct WispXHR *xhr_list = thread->xmlhttprequests;
    thread->xmlhttprequests = NULL;
    while (xhr_list) {
        struct WispXHR *xhr = xhr_list;
        xhr_list = xhr->next;
        JSValue self = xhr->self;
        xhr->self = JS_UNDEFINED;
        xhr->next = NULL;
        if (thread->ctx) {
            JS_FreeValue(thread->ctx, self);
        }
    }

    /* Break MutationObserver cycles and orphan them */
    struct WispMutationObserver *mo_list = (struct WispMutationObserver *)thread->mutation_observers;
    thread->mutation_observers = NULL;
    while (mo_list) {
        struct WispMutationObserver *mo = mo_list;
        mo_list = mo->next;
        JSValue self = mo->self;
        mo->self = JS_UNDEFINED;
        mo->next = NULL;
        if (thread->ctx) {
            JS_FreeValue(thread->ctx, self);
        }
    }

    /* Break IntersectionObserver cycles and orphan them */
    struct WispIntersectionObserver *io_list = (struct WispIntersectionObserver *)thread->intersection_observers;
    thread->intersection_observers = NULL;
    while (io_list) {
        struct WispIntersectionObserver *io = io_list;
        io_list = io->next;
        JSValue self = io->self;
        io->self = JS_UNDEFINED;
        io->next = NULL;
        if (thread->ctx) {
            JS_FreeValue(thread->ctx, self);
        }
    }

    qjs_cleanup_mutation_observer(thread);

    if (thread->ctx) {
        JSRuntime *rt = JS_GetRuntime(thread->ctx);
        JSContext *ctx = thread->ctx;

        /* 1. Set opaque to NULL so no more callbacks are made */
        JS_SetContextOpaque(ctx, NULL);

        /* 2. Run DOM bridge cleanup first while context is fully alive. */
        qjs_finalise_dom_bridge(rt, ctx);

        /* 3. Run GC to collect and finalize objects in the context */
        JS_RunGC(rt);
        JS_RunGC(rt);

        /* 4. Finally, free the context. */
        JS_FreeContext(ctx);
    }

    struct dom_document *doc_node = qjs_thread_get_document(thread);
    if (doc_node) {
        if (!wisp_is_js_process) {
            dom_document_set_mutation_hook(doc_node, NULL, NULL);
            dom_node_unref((dom_node *)doc_node);
        }
    }
    if (thread->location_url) {
        nsurl_unref(thread->location_url);
    }
    if (thread->origin) {
        if (!wisp_is_js_process) {
            release_js_process_for_origin(thread->origin);
        }
        free(thread->origin);
    }
    if (thread->shm_dom) {
        if (current_thread_shm == thread->shm_dom) {
            current_thread_shm = NULL;
        }
        if (!wisp_is_js_process) {
            shm_dom_destroy(thread->shm_dom, thread->shm_dom_name, true);
        }
    }
    free(thread);
}

static uint16_t get_tag_atom_from_name(const char *tag_name)
{
    if (!tag_name)
        return 0;
    if (strcasecmp(tag_name, "html") == 0)
        return 1;
    if (strcasecmp(tag_name, "head") == 0)
        return 2;
    if (strcasecmp(tag_name, "body") == 0)
        return 3;
    if (strcasecmp(tag_name, "title") == 0)
        return 4;
    if (strcasecmp(tag_name, "div") == 0)
        return 5;
    if (strcasecmp(tag_name, "span") == 0)
        return 6;
    if (strcasecmp(tag_name, "p") == 0)
        return 7;
    if (strcasecmp(tag_name, "a") == 0)
        return 8;
    if (strcasecmp(tag_name, "script") == 0)
        return 9;
    if (strcasecmp(tag_name, "style") == 0)
        return 10;
    if (strcasecmp(tag_name, "link") == 0)
        return 11;
    if (strcasecmp(tag_name, "img") == 0)
        return 12;
    if (strcasecmp(tag_name, "iframe") == 0)
        return 13;
    return 14; // Other tag
}

static uint32_t compute_class_hash(const char *class_str)
{
    if (!class_str)
        return 0;
    uint32_t hash = 5381;
    int c;
    while ((c = (unsigned char)*class_str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

void shm_dom_ensure_capacity(struct jsthread *thread, uint32_t required_count)
{
    shm_dom_t *shm = thread->shm_dom;
    if (!shm)
        return;
    if (required_count < shm->node_capacity)
        return;
    if (required_count > 10000000) {
        if (current_thread_shm == shm) {
            current_thread_shm = NULL;
        }
        shm_dom_destroy(shm, thread->shm_dom_name, true);
        thread->shm_dom = NULL;
        thread->shm_capacity = 0;
        return;
    }

    uint32_t old_cap = shm->node_capacity;
    if (required_count >= old_cap) {
        uint32_t new_cap = old_cap;
        while (required_count >= new_cap) {
            new_cap *= 2;
        }
        shm_dom_t *new_shm = shm_dom_remap(shm, thread->shm_capacity, new_cap);
        if (new_shm) {
            new_shm->node_capacity = new_cap;
            if (current_thread_shm == shm) {
                current_thread_shm = new_shm;
            }
            thread->shm_dom = new_shm;
            thread->shm_capacity = new_cap;
        } else {
            if (current_thread_shm == shm) {
                current_thread_shm = NULL;
            }
            thread->shm_dom = NULL;
            thread->shm_capacity = 0;
        }
    }
}

static uint32_t assign_node_index(shm_dom_t *shm, struct jsthread *thread, dom_node *node)
{
    if (!node || !shm)
        return 0;
    if (shm->node_count == 0) {
        shm->node_count = 1;
    }
    for (uint32_t i = 1; i < shm->node_count; i++) {
        if ((dom_node *)(uintptr_t)shm_dom_get_dom_ptrs(shm)[i] == node) {
            return i;
        }
    }
    uint32_t idx = shm->node_count++;
    if (idx >= shm->node_capacity) {
        if (thread) {
            shm_dom_ensure_capacity(thread, idx + 1);
            shm = thread->shm_dom;
            if (!shm)
                return 0;
        } else {
            return 0;
        }
    }
    shm_dom_get_dom_ptrs(shm)[idx] = (uint64_t)(uintptr_t)node;
    return idx;
}

static void
serialize_dom_node(shm_dom_t *shm, struct jsthread *thread, dom_node *node, uint32_t idx, WispNodeID parent_idx)
{
    if (!node || !shm)
        return;

    extern int peak_nodes_used;
    if ((int)shm->node_count > peak_nodes_used) {
        peak_nodes_used = (int)shm->node_count;
        NSLOG(wisp, INFO, "[SHM_DOM] New record peak reached: %d nodes", peak_nodes_used);
    }
    WispCompactNode *nodes_array = shm_dom_get_nodes(shm);
    WispCompactNode *sn = &nodes_array[idx];
    memset(sn, 0, sizeof(*sn));

    WispNodeStrings *node_strings_array = shm_dom_get_node_strings(shm);
    WispNodeStrings *sns = &node_strings_array[idx];
    memset(sns, 0, sizeof(*sns));

    shm_dom_get_dom_ptrs(shm)[idx] = (uint64_t)(uintptr_t)node;
    sn->parent_id = parent_idx;

    dom_node_type type;
    dom_node_get_node_type(node, &type);
    sn->node_type = (uint16_t)type;

    dom_string *name = NULL;
    dom_node_get_node_name(node, &name);
    if (name) {
        size_t len = dom_string_byte_length(name);
        char stack_buf[256];
        char *name_cstr = len < sizeof(stack_buf) ? stack_buf : malloc(len + 1);
        if (name_cstr) {
            memcpy(name_cstr, dom_string_data(name), len);
            name_cstr[len] = '\0';
            sns->name = wisp_shm_alloc_string(shm, name_cstr);
            if (name_cstr != stack_buf) free(name_cstr);
        }
        dom_string_unref(name);
    }

    dom_string *value = NULL;
    dom_node_get_node_value(node, &value);
    if (value) {
        char *value_cstr = malloc(dom_string_byte_length(value) + 1);
        if (value_cstr) {
            memcpy(value_cstr, dom_string_data(value), dom_string_byte_length(value));
            value_cstr[dom_string_byte_length(value)] = '\0';
            sns->value = wisp_shm_alloc_string(shm, value_cstr);
            free(value_cstr);
        }
        dom_string_unref(value);
    }

    if (type == DOM_ELEMENT_NODE) {
        dom_string *tag_name = NULL;
        dom_element_get_tag_name((dom_element *)node, &tag_name);
        if (tag_name) {
            char *tag_cstr = malloc(dom_string_byte_length(tag_name) + 1);
            if (tag_cstr) {
                memcpy(tag_cstr, dom_string_data(tag_name), dom_string_byte_length(tag_name));
                tag_cstr[dom_string_byte_length(tag_name)] = '\0';
                sns->tag_name = wisp_shm_alloc_string(shm, tag_cstr);
                sn->tag_atom = get_tag_atom_from_name(tag_cstr);
                free(tag_cstr);
            }
            dom_string_unref(tag_name);
        }

        dom_namednodemap *attrs = NULL;
        dom_node_get_attributes(node, &attrs);
        if (attrs) {
            uint32_t attr_len = 0;
            dom_namednodemap_get_length(attrs, &attr_len);
            if (attr_len > WISP_SHM_MAX_ATTRIBUTES) {
                NSLOG(wisp, WARNING, "serialize: Element attributes truncated from %u to %d", attr_len,
                    WISP_SHM_MAX_ATTRIBUTES);
                attr_len = WISP_SHM_MAX_ATTRIBUTES;
            }
            sns->attr_count = attr_len;
            for (uint32_t i = 0; i < attr_len; i++) {
                dom_node *attr_node = NULL;
                dom_namednodemap_item(attrs, i, &attr_node);
                if (attr_node) {
                    dom_string *attr_name = NULL;
                    dom_node_get_node_name(attr_node, &attr_name);
                    dom_string *attr_val = NULL;
                    dom_node_get_node_value(attr_node, &attr_val);

                    if (attr_name) {
                        char *an_cstr = malloc(dom_string_byte_length(attr_name) + 1);
                        if (an_cstr) {
                            memcpy(an_cstr, dom_string_data(attr_name), dom_string_byte_length(attr_name));
                            an_cstr[dom_string_byte_length(attr_name)] = '\0';
                            sns->attrs[i].name = wisp_shm_alloc_string(shm, an_cstr);

                            if (strcasecmp(an_cstr, "class") == 0 && attr_val) {
                                char *av_cstr = malloc(dom_string_byte_length(attr_val) + 1);
                                if (av_cstr) {
                                    memcpy(av_cstr, dom_string_data(attr_val), dom_string_byte_length(attr_val));
                                    av_cstr[dom_string_byte_length(attr_val)] = '\0';
                                    sn->class_hash = compute_class_hash(av_cstr);
                                    free(av_cstr);
                                }
                            }
                            free(an_cstr);
                        }
                        dom_string_unref(attr_name);
                    }
                    if (attr_val) {
                        char *av_cstr = malloc(dom_string_byte_length(attr_val) + 1);
                        if (av_cstr) {
                            memcpy(av_cstr, dom_string_data(attr_val), dom_string_byte_length(attr_val));
                            av_cstr[dom_string_byte_length(attr_val)] = '\0';
                            sns->attrs[i].value = wisp_shm_alloc_string(shm, av_cstr);
                            free(av_cstr);
                        }
                        dom_string_unref(attr_val);
                    }
                    dom_node_unref(attr_node);
                }
            }
            dom_namednodemap_unref(attrs);
        }
    }

    dom_node *child = NULL;
    dom_node_get_first_child(node, &child);
    uint32_t prev_child_idx = 0;
    if (child) {
        uint32_t first_child_idx = assign_node_index(shm, thread, child);
        if (thread) {
            shm = thread->shm_dom;
            if (!shm) {
                dom_node_unref(child);
                return;
            }
        }
        nodes_array = shm_dom_get_nodes(shm);
        sn = &nodes_array[idx];
        sn->first_child_id = first_child_idx;

        while (child) {
            uint32_t child_idx = assign_node_index(shm, thread, child);
            if (thread) {
                shm = thread->shm_dom;
                if (!shm) {
                    while (child) {
                        dom_node *next = NULL;
                        dom_node_get_next_sibling(child, &next);
                        dom_node_unref(child);
                        child = next;
                    }
                    break;
                }
            }
            if (child_idx != 0) {
                serialize_dom_node(shm, thread, child, child_idx, idx);
            }

            if (thread) {
                shm = thread->shm_dom;
                if (!shm) {
                    while (child) {
                        dom_node *next = NULL;
                        dom_node_get_next_sibling(child, &next);
                        dom_node_unref(child);
                        child = next;
                    }
                    break;
                }
            }
            nodes_array = shm_dom_get_nodes(shm);
            sn = &nodes_array[idx]; // Re-acquire parent node pointer as the address space might have changed

            nodes_array[child_idx].prev_sibling_id = prev_child_idx;
            if (prev_child_idx != 0) {
                nodes_array[prev_child_idx].next_sibling_id = child_idx;
            }

            prev_child_idx = child_idx;

            dom_node *next = NULL;
            dom_node_get_next_sibling(child, &next);
            dom_node_unref(child);
            child = next;
        }
    }
}

static inline void host_ensure_shm_capacity(struct jsthread *thread)
{
    if (thread && thread->shm_dom && thread->shm_dom->node_capacity > thread->shm_capacity) {
        uint32_t new_cap = thread->shm_dom->node_capacity;
        shm_dom_t *old_shm = thread->shm_dom;
        shm_dom_t *new_shm = shm_dom_remap(old_shm, thread->shm_capacity, new_cap);
        if (new_shm) {
            if (current_thread_shm == old_shm) {
                current_thread_shm = new_shm;
            }
            thread->shm_dom = new_shm;
            thread->shm_capacity = new_cap;
        } else {
            if (current_thread_shm == old_shm) {
                current_thread_shm = NULL;
            }
            thread->shm_dom = NULL;
            thread->shm_capacity = 0;
        }
    }
}

void serialize_dom_tree(shm_dom_t *shm, struct jsthread *thread, struct dom_document *doc)
{
    if (thread) {
        host_ensure_shm_capacity(thread);
        shm = thread->shm_dom;
    }
    if (!shm || !doc) return;

    shm_dom_t *prev_shm = current_thread_shm;
    current_thread_shm = shm;
    thread_shm_locked = true;

    shm_dom_lock_write(shm);

    if (shm->node_count == 0) {
        shm->node_count = 1;
    }

    // Set all existing nodes' node_type to 0 to mark them unvisited
    for (uint32_t i = 1; i < shm->node_count; i++) {
        shm_dom_get_nodes(shm)[i].node_type = 0;
    }

    shm->layout_cache_count = 0;
    memset(shm_dom_get_layout_cache(shm), 0, sizeof(WispShmLayoutCache) * shm->node_capacity);
    memset(shm->string_hash_table, 0, sizeof(shm->string_hash_table));
    shm->string_heap_top = 1; // Initialize top to 1 to reserve 0 as NULL/empty

    uint32_t doc_idx = assign_node_index(shm, thread, (dom_node *)doc);
    if (thread) {
        shm = thread->shm_dom;
        if (!shm) {
            current_thread_shm = prev_shm;
            thread_shm_locked = false;
            return;
        }
    }
    if (doc_idx != 0) {
        serialize_dom_node(shm, thread, (dom_node *)doc, doc_idx, 0);
    }

    if (thread) {
        shm = thread->shm_dom;
    }

    // Scan for unvisited detached nodes and serialize them
    if (shm) {
        for (uint32_t i = 1; i < shm->node_count; i++) {
            uintptr_t raw_ptr = (uintptr_t)shm_dom_get_dom_ptrs(shm)[i];
            if (!raw_ptr || (raw_ptr % sizeof(void *)) != 0) {
                continue;
            }
            dom_node *node = (dom_node *)raw_ptr;
            if (!node->vtable) {
                shm_dom_get_dom_ptrs(shm)[i] = 0;
                continue;
            }
            bool is_valid = (node == (dom_node *)doc);
            if (!is_valid && !wisp_is_js_process) {
                /* In multiprocess main browser process (wisp-gtk),
                 * JS wrappers exist in wisp-js process, not thread->ctx in wisp-gtk.
                 * Always serialize valid non-null raw_ptrs in SHM DOM ptrs array. */
                is_valid = true;
            } else if (!is_valid && thread && thread->ctx) {
                /* Detached node. Serialize it if it has an active JS wrapper in the thread context,
                 * which guarantees it is alive and valid. This prevents use-after-free crashes. */
                is_valid = qjs_bridge_has_node(thread->ctx, node);
            }
            if (!is_valid) {
                continue;
            }
            if (shm_dom_get_nodes(shm)[i].node_type == 0) {
                dom_node *parent = NULL;
                dom_node_get_parent_node(node, &parent);
                uint32_t parent_idx = 0;
                if (parent) {
                    for (uint32_t j = 1; j < shm->node_count; j++) {
                        if ((dom_node *)(uintptr_t)shm_dom_get_dom_ptrs(shm)[j] == parent) {
                            parent_idx = j;
                            break;
                        }
                    }
                    dom_node_unref(parent);
                }
                serialize_dom_node(shm, thread, node, i, parent_idx);
                if (thread) {
                    shm = thread->shm_dom;
                    if (!shm)
                        break;
                }
            }
        }
    }

    shm_dom_t *final_shm = thread ? thread->shm_dom : shm;
    if (final_shm) {
        shm_dom_unlock_write(final_shm);
    } else if (shm) {
        shm_dom_unlock_write(shm);
    }

    current_thread_shm = prev_shm;
    thread_shm_locked = false;
}

static dom_node *get_dom_node_from_id(shm_dom_t *shm, uint64_t id, struct dom_document *doc)
{
    if (!shm || id == 0 || id == 0xFFFFFFFF)
        return NULL;
    uint32_t idx = (uint32_t)id;
    if (idx >= shm->node_count)
        return NULL;

    dom_node *node = (dom_node *)(uintptr_t)shm_dom_get_dom_ptrs(shm)[idx];
    if (!node && doc) {
        WispCompactNode *sn = &shm_dom_get_nodes(shm)[idx];
        WispNodeStrings *sns = &shm_dom_get_node_strings(shm)[idx];
        if (sn->node_type == 1) { // ELEMENT_NODE
            const char *tag = wisp_string_ref_data(shm, sns->tag_name);
            if (tag) {
                dom_string *tag_dom = NULL;
                dom_string_create((const uint8_t *)tag, strlen(tag), &tag_dom);
                struct dom_element *elem = NULL;
                dom_document_create_element(doc, tag_dom, &elem);
                dom_string_unref(tag_dom);
                if (elem) {
                    node = (dom_node *)elem;
                    shm_dom_get_dom_ptrs(shm)[idx] = (uint64_t)(uintptr_t)node;

                    // Copy attributes from SHM to the newly created host element!
                    uint32_t limit = sns->attr_count < WISP_SHM_MAX_ATTRIBUTES ? sns->attr_count
                                                                               : WISP_SHM_MAX_ATTRIBUTES;
                    for (uint32_t i = 0; i < limit; i++) {
                        const char *attr_name = wisp_string_ref_data(shm, sns->attrs[i].name);
                        const char *attr_val = wisp_string_ref_data(shm, sns->attrs[i].value);
                        if (attr_name && attr_val) {
                            dom_string *name_dom = NULL;
                            dom_string_create((const uint8_t *)attr_name, strlen(attr_name), &name_dom);
                            dom_string *val_dom = NULL;
                            dom_string_create((const uint8_t *)attr_val, strlen(attr_val), &val_dom);
                            if (name_dom && val_dom) {
                                dom_element_set_attribute(elem, name_dom, val_dom);
                            }
                            if (name_dom)
                                dom_string_unref(name_dom);
                            if (val_dom)
                                dom_string_unref(val_dom);
                        }
                    }
                }
            }
        } else if (sn->node_type == 3) { // TEXT_NODE
            const char *val_cstr = wisp_string_ref_data(shm, sns->value);
            dom_string *val_dom = NULL;
            dom_string_create((const uint8_t *)(val_cstr ? val_cstr : ""), val_cstr ? strlen(val_cstr) : 0, &val_dom);
            struct dom_text *text_node = NULL;
            dom_document_create_text_node(doc, val_dom, &text_node);
            dom_string_unref(val_dom);
            if (text_node) {
                node = (dom_node *)text_node;
                shm_dom_get_dom_ptrs(shm)[idx] = (uint64_t)(uintptr_t)node;
            }
        } else if (sn->node_type == 8) { // COMMENT_NODE
            const char *val_cstr = wisp_string_ref_data(shm, sns->value);
            dom_string *val_dom = NULL;
            dom_string_create((const uint8_t *)(val_cstr ? val_cstr : ""), val_cstr ? strlen(val_cstr) : 0, &val_dom);
            struct dom_comment *comment_node = NULL;
            dom_document_create_comment(doc, val_dom, &comment_node);
            dom_string_unref(val_dom);
            if (comment_node) {
                node = (dom_node *)comment_node;
                shm_dom_get_dom_ptrs(shm)[idx] = (uint64_t)(uintptr_t)node;
            }
        } else if (sn->node_type == 11) { // DOCUMENT_FRAGMENT_NODE
            struct dom_document_fragment *frag = NULL;
            dom_document_create_document_fragment(doc, &frag);
            if (frag) {
                node = (dom_node *)frag;
                shm_dom_get_dom_ptrs(shm)[idx] = (uint64_t)(uintptr_t)node;
            }
        }
    }
    return node;
}

static dom_node *ensure_host_node(shm_dom_t *shm, uint64_t id, dom_document *doc)
{
    if (!shm || !doc || id == 0 || id == 0xFFFFFFFF)
        return NULL;
    uint32_t idx = (uint32_t)id;
    if (idx >= shm->node_count)
        return NULL;

    dom_node *existing = (dom_node *)(uintptr_t)shm_dom_get_dom_ptrs(shm)[idx];
    if (existing)
        return existing;

    WispCompactNode *nodes = shm_dom_get_nodes(shm);
    WispCompactNode *sn = &nodes[idx];
    WispNodeStrings *node_strings = shm_dom_get_node_strings(shm);
    WispNodeStrings *sns = &node_strings[idx];

    dom_node *new_node = NULL;
    if (sn->node_type == DOM_ELEMENT_NODE) {
        const char *tag = wisp_string_ref_data(shm, sns->tag_name);
        if (tag) {
            dom_string *tag_dom = NULL;
            dom_string_create((const uint8_t *)tag, strlen(tag), &tag_dom);
            if (tag_dom) {
                dom_element *elem = NULL;
                dom_document_create_element(doc, tag_dom, &elem);
                new_node = (dom_node *)elem;
                dom_string_unref(tag_dom);
            }
        }
        if (new_node) {
            uint32_t attr_limit = sns->attr_count < WISP_SHM_MAX_ATTRIBUTES ? sns->attr_count : WISP_SHM_MAX_ATTRIBUTES;
            for (uint32_t i = 0; i < attr_limit; i++) {
                const char *attr_name = wisp_string_ref_data(shm, sns->attrs[i].name);
                const char *attr_val = wisp_string_ref_data(shm, sns->attrs[i].value);
                if (attr_name && attr_val) {
                    dom_string *attr_name_dom = NULL;
                    dom_string_create((const uint8_t *)attr_name, strlen(attr_name), &attr_name_dom);
                    dom_string *attr_val_dom = NULL;
                    dom_string_create((const uint8_t *)attr_val, strlen(attr_val), &attr_val_dom);
                    if (attr_name_dom && attr_val_dom) {
                        dom_element_set_attribute((dom_element *)new_node, attr_name_dom, attr_val_dom);
                    }
                    if (attr_name_dom)
                        dom_string_unref(attr_name_dom);
                    if (attr_val_dom)
                        dom_string_unref(attr_val_dom);
                }
            }
        }
    } else if (sn->node_type == DOM_TEXT_NODE) {
        const char *val = wisp_string_ref_data(shm, sns->value);
        dom_string *val_dom = NULL;
        dom_string_create((const uint8_t *)(val ? val : ""), val ? strlen(val) : 0, &val_dom);
        if (val_dom) {
            dom_text *text = NULL;
            dom_document_create_text_node(doc, val_dom, &text);
            new_node = (dom_node *)text;
            dom_string_unref(val_dom);
        }
    } else if (sn->node_type == DOM_COMMENT_NODE) {
        const char *val = wisp_string_ref_data(shm, sns->value);
        dom_string *val_dom = NULL;
        dom_string_create((const uint8_t *)(val ? val : ""), val ? strlen(val) : 0, &val_dom);
        if (val_dom) {
            dom_comment *comment = NULL;
            dom_document_create_comment(doc, val_dom, &comment);
            new_node = (dom_node *)comment;
            dom_string_unref(val_dom);
        }
    } else if (sn->node_type == DOM_DOCUMENT_FRAGMENT_NODE) {
        dom_document_fragment *frag = NULL;
        dom_document_create_document_fragment(doc, &frag);
        new_node = (dom_node *)frag;
    }

    if (new_node) {
        shm_dom_get_dom_ptrs(shm)[idx] = (uint64_t)(uintptr_t)new_node;

        // Recursively create and append child nodes from SHM
        uint64_t child_id = sn->first_child_id;
        while (child_id != 0) {
            dom_node *child_node = ensure_host_node(shm, child_id, doc);
            if (child_node) {
                dom_node *appended = NULL;
                dom_node_append_child(new_node, child_node, &appended);
                if (appended)
                    dom_node_unref(appended);
            }
            WispCompactNode *child_sn = &shm_dom_get_nodes(shm)[child_id];
            child_id = child_sn->next_sibling_id;
        }
    }
    return new_node;
}

static void apply_shm_mutation(shm_dom_t *shm, shm_mutation_t *m, struct dom_document *doc)
{
    if (!doc)
        return;

    dom_node *target = ensure_host_node(shm, m->target_id, doc);
    dom_node *param1 = ensure_host_node(shm, m->param1_id, doc);
    dom_node *param2 = ensure_host_node(shm, m->param2_id, doc);

    const char *m_name_cstr = wisp_string_ref_data(shm, m->name);
    const char *m_value_cstr = wisp_string_ref_data(shm, m->value);

    switch (m->type) {
    case SHM_MUTATION_SET_ATTRIBUTE: {
        dom_string *name_dom = NULL;
        dom_string_create((const uint8_t *)m_name_cstr, strlen(m_name_cstr), &name_dom);
        dom_string *value_dom = NULL;
        dom_string_create((const uint8_t *)m_value_cstr, strlen(m_value_cstr), &value_dom);
        if (target && name_dom && value_dom) {
            dom_element_set_attribute((dom_element *)target, name_dom, value_dom);
        }
        if (name_dom)
            dom_string_unref(name_dom);
        if (value_dom)
            dom_string_unref(value_dom);
        break;
    }
    case SHM_MUTATION_REMOVE_ATTRIBUTE: {
        dom_string *name_dom = NULL;
        dom_string_create((const uint8_t *)m_name_cstr, strlen(m_name_cstr), &name_dom);
        if (target && name_dom) {
            dom_element_remove_attribute((dom_element *)target, name_dom);
        }
        if (name_dom)
            dom_string_unref(name_dom);
        break;
    }
    case SHM_MUTATION_APPEND_CHILD: {
        if (target && param1) {
            dom_node_append_child(target, param1, NULL);
        }
        break;
    }
    case SHM_MUTATION_REMOVE_CHILD: {
        if (target && param1) {
            dom_node_remove_child(target, param1, NULL);
        }
        break;
    }
    case SHM_MUTATION_INSERT_BEFORE: {
        if (target && param1) {
            dom_node_insert_before(target, param1, param2, NULL);
        }
        break;
    }
    case SHM_MUTATION_REPLACE_CHILD: {
        if (target && param1 && param2) {
            dom_node_replace_child(target, param1, param2, NULL);
        }
        break;
    }
    case SHM_MUTATION_SET_NODE_VALUE: {
        dom_string *ds = NULL;
        dom_string_create((const uint8_t *)m_value_cstr, strlen(m_value_cstr), &ds);
        if (target && ds) {
            dom_node_set_node_value(target, ds);
        }
        if (ds)
            dom_string_unref(ds);
        break;
    }
    case SHM_MUTATION_SET_TEXT_CONTENT: {
        dom_string *ds = NULL;
        dom_string_create((const uint8_t *)m_value_cstr, strlen(m_value_cstr), &ds);
        if (target && ds) {
            dom_node_set_text_content(target, ds);
        }
        if (ds)
            dom_string_unref(ds);
        break;
    }
    case SHM_MUTATION_SET_INNER_HTML: {
        if (target && m_value_cstr) {
            // Clear existing children
            dom_node *child = NULL;
            while (dom_node_get_first_child(target, &child) == DOM_NO_ERR && child != NULL) {
                dom_node_remove_child(target, child, NULL);
                dom_node_unref(child);
                child = NULL;
            }

            // Parse and insert new HTML using Hubbub
            dom_hubbub_parser_params params;
            memset(&params, 0, sizeof(params));
            params.enc = "UTF-8";
            params.idname = corestring_dom_id;

            dom_hubbub_parser *parser = NULL;
            dom_document_fragment *fragment = NULL;
            dom_hubbub_error err = dom_hubbub_fragment_parser_create(&params, doc, &parser, &fragment);
            if (err == DOM_HUBBUB_OK) {
                err = dom_hubbub_parser_parse_chunk(parser, (const uint8_t *)m_value_cstr, strlen(m_value_cstr));
                if (err == DOM_HUBBUB_OK) {
                    err = dom_hubbub_parser_completed(parser);
                }
                if (err == DOM_HUBBUB_OK && fragment != NULL) {
                    dom_node *result = NULL;
                    dom_node_append_child(target, (dom_node *)fragment, &result);
                    if (result)
                        dom_node_unref(result);
                }
                if (fragment)
                    dom_node_unref((dom_node *)fragment);
                dom_hubbub_parser_destroy(parser);
            }
        }
        break;
    }
    }
}

static void update_shm_box_bounds_recursive(struct jsthread *thread, struct box *box)
{
    if (!box)
        return;
    shm_dom_t *shm = thread->shm_dom;
    if (!shm)
        return;
    if (box->node) {
        for (uint32_t i = 1; i < shm->node_count; i++) {
            if (shm_dom_get_dom_ptrs(shm)[i] == (uint64_t)(uintptr_t)box->node) {
                WispCompactNode *nodes_array = shm_dom_get_nodes(shm);
                WispCompactNode *node = &nodes_array[i];
                if (node->layout_index == 0) {
                    uint32_t l_idx = ++shm->layout_cache_count;
                    if (l_idx >= shm->node_capacity) {
                        shm_dom_ensure_capacity(thread, l_idx + 1);
                        shm = thread->shm_dom;
                        if (!shm)
                            return;
                        nodes_array = shm_dom_get_nodes(shm);
                        node = &nodes_array[i];
                    }
                    node->layout_index = l_idx;
                }
                uint32_t l_idx = node->layout_index;
                WispShmLayoutCache *lc = &shm_dom_get_layout_cache(shm)[l_idx];

                struct rect r;
                box_bounds(box, &r);

                uint32_t seq = lc->seq_version;
                __atomic_store_n(&lc->seq_version, seq + 1, __ATOMIC_RELEASE);

                lc->x = r.x0;
                lc->y = r.y0;
                lc->width = r.x1 - r.x0;
                lc->height = r.y1 - r.y0;
                lc->layout_dirty = 0;

                __atomic_store_n(&lc->seq_version, seq + 2, __ATOMIC_RELEASE);
                break;
            }
        }
    }
    for (struct box *child = box->children; child; child = child->next) {
        update_shm_box_bounds_recursive(thread, child);
    }
}

void qjs_update_shm_box_bounds(struct jsthread *thread, struct box *doc_box)
{
    if (!thread || !thread->shm_dom || !doc_box)
        return;
    host_ensure_shm_capacity(thread);
    shm_dom_t *shm = thread->shm_dom;
    shm_dom_lock_write(shm);
    update_shm_box_bounds_recursive(thread, doc_box);
    if (thread->shm_dom == shm) {
        thread->shm_dom->layout_dirty = false;
        shm_dom_unlock_write(thread->shm_dom);
    } else if (shm) {
        shm_dom_unlock_write(shm);
    }
}

extern bool layout_document(struct html_content *content, int width, int height);

static void force_synchronous_layout(struct jsthread *thread)
{
    struct html_content *htmlc = (thread->win_priv && thread->win_priv != thread->doc_priv)
        ? (struct html_content *)thread->doc_priv
        : NULL;
    if (htmlc && htmlc->layout) {
        int width = htmlc->last_layout_width > 0 ? htmlc->last_layout_width : 1024;
        int height = htmlc->last_layout_height > 0 ? htmlc->last_layout_height : 768;

        doc_rwlock_rdlock(&htmlc->doc_mutex);
        layout_document(htmlc, width, height);
        doc_rwlock_rdunlock(&htmlc->doc_mutex);

        qjs_update_shm_box_bounds(thread, htmlc->layout);
    }
}

void drain_mutation_queue(shm_dom_t *shm, struct dom_document *doc) {
    if (!shm) return;
    shm_dom_t *prev_shm = current_thread_shm;
    current_thread_shm = shm;
    shm_mutation_queue_t *mq = &shm->mutation_queue;
    while (mq->tail != mq->head) {
        uint32_t idx = mq->tail % SHM_MUTATION_QUEUE_SIZE;
        shm_mutation_t *m = &mq->queue[idx];
        apply_shm_mutation(shm, m, doc);
        mq->tail++;
    }
    current_thread_shm = prev_shm;
}

bool js_exec(jsthread *thread, const uint8_t *txt, size_t txtlen, const char *name)
{
    if (!thread || thread->closed)
        return false;
    JS_UpdateStackTop(JS_GetRuntime(thread->ctx));

    /* In-process Content Security Policy (CSP) validation */
    struct html_content *htmlc = (thread->win_priv && thread->win_priv != thread->doc_priv)
        ? (struct html_content *)thread->doc_priv
        : NULL;

    bool is_module = false;
    struct html_script *found_s = NULL;
    if (htmlc && name) {
        if (strcmp(name, "?inline script?") == 0) {
            for (unsigned int idx = 0; idx < htmlc->scripts_count; idx++) {
                struct html_script *s = &htmlc->scripts[idx];
                if (s->type == HTML_SCRIPT_INLINE && s->data.string != NULL) {
                    const char *str_data = dom_string_data(s->data.string);
                    size_t str_len = dom_string_byte_length(s->data.string);
                    if (str_len == txtlen && memcmp(str_data, txt, txtlen) == 0) {
                        found_s = s;
                        break;
                    }
                }
            }
        } else {
            for (unsigned int idx = 0; idx < htmlc->scripts_count; idx++) {
                struct html_script *s = &htmlc->scripts[idx];
                if (s->type != HTML_SCRIPT_INLINE && s->data.handle != NULL) {
                    const char *url_str = nsurl_access(hlcache_handle_get_url(s->data.handle));
                    if (url_str && strcmp(url_str, name) == 0) {
                        found_s = s;
                        break;
                    }
                }
            }
        }
    }
    if (found_s && found_s->mimetype) {
        const char *mime_data = dom_string_data(found_s->mimetype);
        if (mime_data && strcasecmp(mime_data, "module") == 0) {
            is_module = true;
        }
    }
    int eval_flags = is_module ? JS_EVAL_TYPE_MODULE : JS_EVAL_TYPE_GLOBAL;
    if (htmlc && htmlc->csp) {
        bool is_url = false;
        nsurl *url = NULL;

        if (name) {
            /* Check if name has a scheme/URL prefix, including protocol-relative, data, or blob URLs */
            if (strstr(name, "://") || strncmp(name, "//", 2) == 0 || strncmp(name, "data:", 5) == 0 ||
                strncmp(name, "blob:", 5) == 0 || strncmp(name, "http", 4) == 0 || strncmp(name, "file", 4) == 0) {

                if (nsurl_create(name, &url) != NSERROR_OK) {
                    if (htmlc->base_url) {
                        nsurl_join(htmlc->base_url, name, &url);
                    }
                }
                if (url != NULL) {
                    is_url = true;
                }
            }
        }

        if (is_url && url != NULL) {
            /* Check scheme to exclude internal trusted resources */
            lwc_string *scheme = nsurl_get_component(url, NSURL_SCHEME);
            bool is_internal = false;
            if (scheme) {
                const char *scheme_str = lwc_string_data(scheme);
                if (strcasecmp(scheme_str, "resource") == 0 || strcasecmp(scheme_str, "about") == 0) {
                    is_internal = true;
                }
                lwc_string_unref(scheme);
            }

            if (!is_internal) {
                if (!csp_check_url(htmlc->csp, CSP_SCRIPT_SRC, url)) {
                    NSLOG(wisp, WARNING, "CSP blocked script execution from URL: %s", name);
                    nsurl_unref(url);
                    return false;
                }
            }
            nsurl_unref(url);
        } else {
            /* This is an inline or dynamic script evaluation.
             * If name is "?inline script?", it was already validated in exec_inline_script using nonce or inline
             * checks. Otherwise, check if inline script execution is permitted by CSP.
             */
            if (!name || strcmp(name, "?inline script?") != 0) {
                bool is_dynamic_eval = false;
                if (!name || strcmp(name, "<ipc>") == 0 || strcmp(name, "<eval>") == 0 || strncmp(name, "<", 1) == 0) {
                    is_dynamic_eval = true;
                }

                if (is_dynamic_eval) {
                    if (!csp_check_eval(htmlc->csp)) {
                        NSLOG(wisp, WARNING, "CSP blocked dynamic script evaluation (unsafe-eval) under QuickJS: %s",
                            name ? name : "unnamed");
                        return false;
                    }
                } else {
                    if (!csp_check_inline(htmlc->csp, CSP_SCRIPT_SRC)) {
                        NSLOG(wisp, WARNING, "CSP blocked inline script execution under QuickJS: %s",
                            name ? name : "unnamed");
                        return false;
                    }
                }
            }
        }
    }

    char *old_script_name = thread->current_script_name;
    thread->current_script_name = (char *)name;

    wisp_ipc_handle *ipc_js = get_js_process_handle(thread->origin);
    if (ipc_js) {
        if (!thread->shm_initialized) {
            const char *orig = thread->origin ? thread->origin : "";
            size_t len = strlen(thread->shm_dom_name) + 1 + strlen(orig);
            char *payload = malloc(len + 1);
            if (payload) {
                snprintf(payload, len + 1, "%s|%s", thread->shm_dom_name, orig);
                wisp_ipc_msg init_msg;
                init_msg.type = WISP_IPC_MSG_SHM_INIT;
                init_msg.length = len;
                init_msg.data = (uint8_t *)payload;
                wisp_ipc_send(ipc_js, &init_msg);
                free(payload);
                thread->shm_initialized = true;
            }
        }

        struct dom_document *doc = qjs_thread_get_document(thread);
        if (doc) {
            dom_node_ref((dom_node *)doc);
            serialize_dom_tree(thread->shm_dom, thread, doc);
        }

        /* Use thread pointer as a unique context ID for the remote process */
        uint32_t ctx_id = (uint32_t)(uintptr_t)thread;
        uint32_t name_len = name ? strlen(name) : 0;

        wisp_ipc_msg msg;
        msg.type = WISP_IPC_MSG_JS_EXEC;

        bool is_file = false;
        char temp_file_path[256];
        if (txtlen > 65536) {
            // Write to a temporary file to avoid IPC buffer saturation
            int fd;
            snprintf(temp_file_path, sizeof(temp_file_path), "/tmp/wisp-script-XXXXXX");
            fd = mkstemp(temp_file_path);
            if (fd >= 0) {
                if (write(fd, txt, txtlen) == (ssize_t)txtlen) {
                    is_file = true;
                }
                close(fd);
            }
        }

        if (is_file) {
            char file_prefix[512];
            snprintf(file_prefix, sizeof(file_prefix), "file://%s", temp_file_path);
            size_t file_prefix_len = strlen(file_prefix);
            msg.length = 12 + name_len + file_prefix_len;
            msg.data = malloc(msg.length);
            if (msg.data) {
                memcpy(msg.data, &ctx_id, 4);
                memcpy(msg.data + 4, &eval_flags, 4);
                memcpy(msg.data + 8, &name_len, 4);
                if (name_len > 0) {
                    memcpy(msg.data + 12, name, name_len);
                }
                memcpy(msg.data + 12 + name_len, file_prefix, file_prefix_len);
            }
        } else {
            msg.length = 12 + name_len + txtlen;
            msg.data = malloc(msg.length);
            if (msg.data) {
                memcpy(msg.data, &ctx_id, 4);
                memcpy(msg.data + 4, &eval_flags, 4);
                memcpy(msg.data + 8, &name_len, 4);
                if (name_len > 0) {
                    memcpy(msg.data + 12, name, name_len);
                }
                memcpy(msg.data + 12 + name_len, txt, txtlen);
            }
        }

        if (msg.data) {
            if (wisp_ipc_send(ipc_js, &msg) == NSERROR_OK) {
                free(msg.data);
                /* Implement timeout for recv to avoid UI hang */
                wisp_ipc_msg response;
                wisp_ipc_set_blocking(ipc_js, false);
                int retries = 30000; // 300 seconds timeout to allow slow connections to fetch 100+ modules
                bool got_response = false;
                bool crashed = false;
                while (retries-- > 0) {
                    nserror recv_err = wisp_ipc_recv(ipc_js, &response);
                    if (recv_err == NSERROR_OK) {
                        if (response.type == WISP_IPC_MSG_JS_EXEC) {
                            got_response = true;
                            break;
                        } else if (response.type == WISP_IPC_MSG_DOM_REQUEST) {
                            if (doc) {
                                host_ensure_shm_capacity(thread);
                                drain_mutation_queue(thread->shm_dom, doc);
                                serialize_dom_tree(thread->shm_dom, thread, doc);
                                force_synchronous_layout(thread);
                            }
                            wisp_ipc_msg resp;
                            resp.type = WISP_IPC_MSG_DOM_RESPONSE;
                            resp.length = 0;
                            resp.data = NULL;
                            wisp_ipc_send(ipc_js, &resp);
                            wisp_ipc_msg_free(&response);
                        } else if (response.type == WISP_IPC_MSG_NAVIGATE) {
                            if (response.data && response.length > 0 && thread->win_priv) {
                                struct browser_window *bw = (struct browser_window *)thread->win_priv;
                                const char *url_str = (const char *)response.data;
                                struct nsurl *base_url = NULL;
                                if (thread->doc_priv && thread->win_priv != thread->doc_priv) {
                                    base_url = content_get_url((struct content *)thread->doc_priv);
                                }
                                struct nsurl *target_url = NULL;
                                nserror err = NSERROR_BAD_URL;
                                if (base_url) {
                                    err = nsurl_join(base_url, url_str, &target_url);
                                }
                                if (err != NSERROR_OK) {
                                    err = nsurl_create(url_str, &target_url);
                                }
                                if (err == NSERROR_OK && target_url) {
                                    browser_window_navigate(bw, target_url, base_url, BW_NAVIGATE_HISTORY, NULL, NULL, NULL);
                                    nsurl_unref(target_url);
                                }
                            }
                            wisp_ipc_msg resp;
                            resp.type = WISP_IPC_MSG_DOM_RESPONSE;
                            resp.length = 0;
                            resp.data = NULL;
                            wisp_ipc_send(ipc_js, &resp);
                            wisp_ipc_msg_free(&response);
                        } else {
                            /* Ignore unexpected/stale message types */
                            wisp_ipc_msg_free(&response);
                        }
                    } else if (recv_err != NSERROR_NOT_FOUND) {
                        /* Socket error or EOF -> crash detected! */
                        NSLOG(wisp, ERROR, "JS process crashed during recv (error %d) for origin %s", (int)recv_err,
                            thread->origin);
                        handle_process_crash(thread->origin);
                        crashed = true;
                        break;
                    }
                    usleep(10000);
                }
                if (!crashed) {
                    wisp_ipc_set_blocking(ipc_js, true);
                }
                if (is_file) {
                    unlink(temp_file_path);
                }
                if (got_response) {
                    if (doc) {
                        host_ensure_shm_capacity(thread);
                        drain_mutation_queue(thread->shm_dom, doc);
                        dom_node_unref((dom_node *)doc);
                    }
                    bool success = (response.length > 0 || response.data != NULL);
                    wisp_ipc_msg_free(&response);
                    return success;
                } else if (!crashed && retries <= 0) {
                    NSLOG(wisp, ERROR, "JS process timed out for origin %s", thread->origin);
                    handle_process_crash(thread->origin);
                    crashed = true;
                }
            } else {
                free(msg.data);
                if (is_file) {
                    unlink(temp_file_path);
                }
                NSLOG(wisp, ERROR, "JS process write failed for origin %s (likely crashed)", thread->origin);
                handle_process_crash(thread->origin);
            }
        } else {
            if (is_file) {
                unlink(temp_file_path);
            }
        }
        if (doc) {
            dom_node_unref((dom_node *)doc);
        }
        /* Fallback to in-process if IPC fails or times out */
        NSLOG(wisp, WARNING, "JS IPC failed for %s, falling back to in-process", name);
    }

    uint64_t old_deadline = 0;
    uint64_t old_last_yield = 0;
    if (thread->heap) {
        old_deadline = thread->heap->deadline_ms;
        old_last_yield = thread->heap->last_yield_ms;
        uint64_t now;
        nsu_getmonotonic_ms(&now);
        thread->heap->deadline_ms = now + 3000; // Absolute deadline 3s in future
        thread->heap->last_yield_ms = now;
    }

    JSValue val = js_eval_with_aot_cache(thread->ctx, txt, txtlen, name, eval_flags);

    if (thread->heap) {
        thread->heap->deadline_ms = old_deadline;
        thread->heap->last_yield_ms = old_last_yield;
    }

    JSContext *ctx1;
    int job_ret;
    while ((job_ret = JS_ExecutePendingJob(JS_GetRuntime(thread->ctx), &ctx1)) != 0) {
        if (job_ret < 0) {
            JSValue exc = JS_GetException(ctx1);
            const char *exc_str = JS_ToCString(ctx1, exc);
            NSLOG(wisp, WARNING, "JS Error in microtask: %s", exc_str ? exc_str : "unknown");
            if (exc_str)
                JS_FreeCString(ctx1, exc_str);
            JS_FreeValue(ctx1, exc);
        }
    }

    bool success = !JS_IsException(val);
    if (!success) {
        JSValue exc = JS_GetException(thread->ctx);
        const char *exc_str = JS_ToCString(thread->ctx, exc);
        fprintf(stderr, "\n=== JS EXEC EXCEPTION: %s ===\n", exc_str ? exc_str : "unknown");
        JSValue stack = JS_UNDEFINED;
        if (JS_IsObject(exc)) {
            stack = JS_GetPropertyStr(thread->ctx, exc, "stack");
        }
        const char *stack_str = JS_ToCString(thread->ctx, stack);
        if (stack_str) {
            fprintf(stderr, "Stack Trace:\n%s\n", stack_str);
            JS_FreeCString(thread->ctx, stack_str);
        }
        JS_FreeValue(thread->ctx, stack);
        NSLOG(wisp, ERROR, "JS execution error in %s: %s", name, exc_str ? exc_str : "unknown error");
        if (exc_str)
            JS_FreeCString(thread->ctx, exc_str);
        JS_FreeValue(thread->ctx, exc);
    }
    JS_FreeValue(thread->ctx, val);
    thread->current_script_name = old_script_name;
    return success;
}

static void qjs_event_handler(struct dom_event *evt, void *pw)
{
    struct qjs_event_listener_ctx *ctx = pw;
    if (!ctx || !ctx->thread || ctx->thread->closed)
        return;
    jsthread *thread = ctx->thread;
    JSContext *jsctx = thread->ctx;
    JSValue global = JS_GetGlobalObject(jsctx);
    JSValue js_evt = JS_UNDEFINED;
    struct qjs_event_map *map = thread->events;
    while (map) {
        if (map->evt == evt) {
            js_evt = JS_DupValue(jsctx, map->js_evt);
            break;
        }
        map = map->next;
    }
    if (JS_IsUndefined(js_evt)) {
        js_evt = qjs_new_event(jsctx, evt, true);
        struct qjs_event_map *new_map = malloc(sizeof(*new_map));
        if (new_map) {
            dom_event_ref(evt);
            new_map->evt = evt;
            new_map->js_evt = JS_DupValue(jsctx, js_evt);
            new_map->next = thread->events;
            thread->events = new_map;
        }
    }
    struct dom_document *doc_node_evt = qjs_thread_get_document(thread);
    JSValue this_obj = (ctx->target == (struct dom_event_target *)thread->win_priv ||
                           ctx->target == (struct dom_event_target *)doc_node_evt)
        ? JS_DupValue(jsctx, global)
        : qjs_wrap_node(jsctx, (dom_node *)ctx->target);

    dom_event_target *evt_target = NULL;
    dom_event_get_target(evt, &evt_target);
    if (evt_target) {
        JSValue js_target = qjs_wrap_node(jsctx, (dom_node *)evt_target);
        JS_SetPropertyStr(jsctx, js_evt, "_target", js_target);
        dom_node_unref((dom_node *)evt_target);
    }

    dom_event_target *evt_curr = NULL;
    dom_event_get_current_target(evt, &evt_curr);
    if (evt_curr) {
        JSValue js_curr = qjs_wrap_node(jsctx, (dom_node *)evt_curr);
        JS_SetPropertyStr(jsctx, js_evt, "_currentTarget", js_curr);
        dom_node_unref((dom_node *)evt_curr);
    } else {
        JS_SetPropertyStr(jsctx, js_evt, "_currentTarget", JS_DupValue(jsctx, this_obj));
    }

    dom_event_flow_phase phase;
    dom_event_get_event_phase(evt, &phase);
    JS_SetPropertyStr(jsctx, js_evt, "_eventPhase", JS_NewInt32(jsctx, phase));

    uint64_t old_deadline = 0;
    uint64_t old_last_yield = 0;
    if (thread && thread->heap) {
        old_deadline = thread->heap->deadline_ms;
        old_last_yield = thread->heap->last_yield_ms;
        uint64_t now;
        nsu_getmonotonic_ms(&now);
        thread->heap->deadline_ms = now + 3000; // Absolute deadline 3s in future
        thread->heap->last_yield_ms = now;
    }

    JSValue ret;
    if (JS_IsFunction(jsctx, ctx->func)) {
        ret = JS_Call(jsctx, ctx->func, this_obj, 1, &js_evt);
    } else if (JS_IsObject(ctx->func)) {
        JSValue handleEvent = JS_GetPropertyStr(jsctx, ctx->func, "handleEvent");
        if (JS_IsFunction(jsctx, handleEvent)) {
            ret = JS_Call(jsctx, handleEvent, ctx->func, 1, &js_evt);
        } else {
            ret = JS_UNDEFINED;
        }
        JS_FreeValue(jsctx, handleEvent);
    } else {
        ret = JS_UNDEFINED;
    }

    if (JS_IsBool(ret) && !JS_ToBool(jsctx, ret)) {
        dom_event_prevent_default(evt);
    }

    if (thread && thread->heap) {
        thread->heap->deadline_ms = old_deadline;
        thread->heap->last_yield_ms = old_last_yield;
    }

    if (JS_IsException(ret)) {
        JSValue exc = JS_GetException(jsctx);
        const char *exc_str = JS_ToCString(jsctx, exc);
        if (exc_str)
            JS_FreeCString(jsctx, exc_str);
        JS_FreeValue(jsctx, exc);
    }

    JSContext *ctx1;
    int job_ret;
    while ((job_ret = JS_ExecutePendingJob(JS_GetRuntime(jsctx), &ctx1)) != 0) {
        if (job_ret < 0) {
            JSValue exc = JS_GetException(ctx1);
            const char *exc_str = JS_ToCString(ctx1, exc);
            NSLOG(wisp, WARNING, "JS Error in microtask: %s", exc_str ? exc_str : "unknown");
            if (exc_str)
                JS_FreeCString(ctx1, exc_str);
            JS_FreeValue(ctx1, exc);
        }
    }

    JS_FreeValue(jsctx, ret);
    JS_FreeValue(jsctx, this_obj);
    JS_FreeValue(jsctx, js_evt);
    JS_FreeValue(jsctx, global);
}

bool js_fire_event_with_cancelable(jsthread *thread, const char *type, struct dom_document *doc, struct dom_node *target, bool cancelable)
{
    if (!thread || !doc)
        return false;
    if (!target)
        target = (dom_node *)doc;
    if (target == (dom_node *)thread->win_priv) {
        target = (dom_node *)qjs_thread_get_document(thread);
        if (!target)
            return false;
    }
    dom_string *type_str = NULL;
    dom_string_create((const uint8_t *)type, strlen(type), &type_str);
    dom_event *evt = NULL;
    dom_event_create(&evt);
    bool success = false;
    if (evt && type_str) {
        dom_event_init(evt, type_str, true, true);
        dom_event_target_dispatch_event((dom_event_target *)target, evt, &success);
        dom_event_unref(evt);
    } else {
        NSLOG(wisp, ERROR, "js_fire_event: Failed to create dom_event");
    }
    if (type_str)
        dom_string_unref(type_str);
    return success;
}

bool js_fire_event(jsthread *thread, const char *type, struct dom_document *doc, struct dom_node *target)
{
    return js_fire_event_with_cancelable(thread, type, doc, target, true);
}

bool js_dom_event_add_listener(jsthread *thread, struct dom_document *document, struct dom_node *node,
    struct dom_string *event_type_dom, JSValue js_funcval)
{
    if (wisp_is_js_process) return false;
    if (!thread || !node)
        return false;
    if (node == (struct dom_node *)thread->win_priv) {
        node = (struct dom_node *)qjs_thread_get_document(thread);
        if (!node)
            return false;
    }
    struct qjs_event_listener_ctx *ctx = malloc(sizeof(*ctx));
    if (!ctx)
        return false;
    ctx->thread = thread;
    ctx->func = JS_DupValue(thread->ctx, js_funcval);
    ctx->target = (struct dom_event_target *)node;
    ctx->type = event_type_dom;
    dom_node_ref(node);
    dom_string_ref(event_type_dom);
    dom_event_listener *listener;
    if (dom_event_listener_create(qjs_event_handler, ctx, &listener) != DOM_NO_ERR) {
        dom_node_unref(node);
        dom_string_unref(event_type_dom);
        JS_FreeValue(thread->ctx, ctx->func);
        free(ctx);
        return false;
    }
    ctx->listener = listener;
    dom_event_target_add_event_listener(ctx->target, ctx->type, listener, false);
    ctx->next = thread->listeners;
    thread->listeners = ctx;
    return true;
}

bool js_dom_event_remove_listener(jsthread *thread, struct dom_document *document, struct dom_node *node,
    struct dom_string *event_type_dom, JSValue js_funcval)
{
    if (wisp_is_js_process) return false;
    if (!thread || !node)
        return false;
    if (node == (struct dom_node *)thread->win_priv) {
        node = (struct dom_node *)qjs_thread_get_document(thread);
        if (!node)
            return false;
    }
    struct qjs_event_listener_ctx **prev = &thread->listeners;
    struct qjs_event_listener_ctx *curr = thread->listeners;
    while (curr) {
        if (curr->target == (struct dom_event_target *)node && dom_string_isequal(curr->type, event_type_dom) &&
            JS_VALUE_GET_PTR(curr->func) == JS_VALUE_GET_PTR(js_funcval)) {
            dom_event_target_remove_event_listener(curr->target, curr->type, curr->listener, false);
            dom_node_unref((struct dom_node *)curr->target);
            dom_string_unref(curr->type);
            JS_FreeValue(thread->ctx, curr->func);
            dom_event_listener_unref(curr->listener);
            *prev = curr->next;
            free(curr);
            return true;
        }
        prev = &curr->next;
        curr = curr->next;
    }
    return false;
}

void js_handle_new_element(jsthread *thread, struct dom_element *node)
{
}

bool js_event_cleanup(jsthread *thread, struct dom_event *evt)
{
    if (!thread || !evt)
        return false;
    struct qjs_event_map **prev = &thread->events, *curr = thread->events;
    while (curr) {
        if (curr->evt == evt) {
            *prev = curr->next;
            JS_FreeValue(thread->ctx, curr->js_evt);
            dom_event_unref(evt);
            free(curr);
            return true;
        }
        prev = &curr->next;
        curr = curr->next;
    }
    return false;
}

JSValue qjs_new_intersectionobserverentry_manual(JSContext *ctx, WispIntersectionObserverEntry *entry);

void js_handle_intersection_check(jsthread *thread, struct box *layout, int viewport_width, int viewport_height)
{
    if (!thread || !thread->intersection_observers || !layout)
        return;
    uint64_t now_ms;
    nsu_getmonotonic_ms(&now_ms);
    WispIntersectionObserver *obs = thread->intersection_observers;
    while (obs) {
        bool changed = false;
        IntersectionObserverTarget *ot = obs->targets;

        int rx0 = 0, ry0 = 0, rx1 = viewport_width, ry1 = viewport_height;
        if (obs->root) {
            struct box *root_box = box_find_by_node(layout, obs->root);
            if (root_box) {
                box_coords(root_box, &rx0, &ry0);
                rx1 = rx0 + root_box->width + root_box->padding[LEFT] + root_box->padding[RIGHT];
                ry1 = ry0 + root_box->height + root_box->padding[TOP] + root_box->padding[BOTTOM];
            }
        }

        while (ot) {
            struct box *target_box = box_find_by_node(layout, ot->node);
            if (target_box) {
                int tx, ty;
                box_coords(target_box, &tx, &ty);
                int tw = target_box->width + target_box->padding[LEFT] + target_box->padding[RIGHT];
                int th = target_box->height + target_box->padding[TOP] + target_box->padding[BOTTOM];

                int ix0 = tx > rx0 ? tx : rx0;
                int iy0 = ty > ry0 ? ty : ry0;
                int ix1 = (tx + tw) < rx1 ? (tx + tw) : rx1;
                int iy1 = (ty + th) < ry1 ? (ty + th) : ry1;

                bool isIntersecting = (ix1 > ix0) && (iy1 > iy0);
                double currentRatio = 0.0;
                if (isIntersecting && tw > 0 && th > 0) {
                    currentRatio = (double)((ix1 - ix0) * (iy1 - iy0)) / (double)(tw * th);
                }

                // Threshold cross detection logic
                bool trigger = false;
                if (ot->lastRatio == -1.0) {
                    // First observation
                    trigger = true;
                } else {
                    // Compare lastRatio and currentRatio against all thresholds
                    for (int i = 0; i < obs->num_thresholds; i++) {
                        double th_val = obs->thresholds[i];
                        if ((ot->lastRatio < th_val && currentRatio >= th_val) ||
                            (ot->lastRatio >= th_val && currentRatio < th_val) ||
                            (ot->lastRatio == th_val && currentRatio != th_val) ||
                            (currentRatio == th_val && ot->lastRatio != th_val)) {
                            trigger = true;
                            break;
                        }
                    }
                }

                if (trigger) {
                    WispIntersectionObserverEntry entry;
                    memset(&entry, 0, sizeof(entry));
                    entry.time = (double)now_ms;
                    entry.target = ot->node;
                    dom_node_ref(ot->node);
                    entry.isIntersecting = isIntersecting;
                    entry.targetX = tx;
                    entry.targetY = ty;
                    entry.targetWidth = tw;
                    entry.targetHeight = th;
                    entry.rootWidth = rx1 - rx0;
                    entry.rootHeight = ry1 - ry0;
                    if (isIntersecting) {
                        entry.intersectX = ix0;
                        entry.intersectY = iy0;
                        entry.intersectWidth = ix1 - ix0;
                        entry.intersectHeight = iy1 - iy0;
                        entry.intersectionRatio = currentRatio;
                    } else {
                        entry.intersectionRatio = 0.0;
                    }
                    uint32_t len = 0;
                    JSValue js_len = JS_GetPropertyStr(obs->ctx, obs->queue, "length");
                    JS_ToUint32(obs->ctx, &len, js_len);
                    JS_FreeValue(obs->ctx, js_len);
                    JS_SetPropertyUint32(
                        obs->ctx, obs->queue, len, qjs_new_intersectionobserverentry_manual(obs->ctx, &entry));
                    ot->wasIntersecting = isIntersecting;
                    ot->lastRatio = currentRatio;
                    changed = true;
                }
            }
            ot = ot->next;
        }
        if (changed) {
            JSValue args[2];
            args[0] = obs->queue;
            args[1] = JS_NULL;
            JSValue ret = JS_Call(obs->ctx, obs->callback, JS_UNDEFINED, 2, args);
            JS_FreeValue(obs->ctx, ret);
            JS_FreeValue(obs->ctx, obs->queue);
            obs->queue = JS_NewArray(obs->ctx);
        }
        obs = obs->next;
    }
}
