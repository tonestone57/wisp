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

#include <wisp/utils/errors.h>
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "quickjs.h"
#include "utils/hashmap.h"
#include "content/handlers/javascript/js.h"
#include "qjs_internal.h"
#include "JSEvent.gen.h"
#include "wisp_subsystem.h"
#include "crypto.h"
#include "dom_bridge.h"
#include <nsutils/time.h>
#include <wisp/misc.h>
#include <wisp/content/handlers/html/box_inspect.h>
#include <wisp/content/handlers/html/box.h>
#include <wisp/content/handlers/html/private.h>
#include <wisp/utils/nsoption.h>
#include <math.h>
#include "impl/observer_internal.h"
#include <wisp/desktop/gui_table.h>
#include <wisp/utils/ipc.h>
#include <wisp/utils/nsurl.h>

#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <openssl/evp.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#endif

bool wisp_is_js_process = false;
shm_dom_t *wisp_shm_dom = NULL;

static void compute_sha256(const uint8_t *data, size_t len, char *hex_out) {
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
        sprintf(hex_out + (i * 2), "%02x", hash[i]);
    }
    hex_out[hash_len * 2] = '\0';
}

JSValue js_eval_with_aot_cache(JSContext *ctx, const uint8_t *txt, size_t txtlen, const char *name, int eval_flags) {
    if (!txt || txtlen == 0) {
        return JS_Eval(ctx, (const char *)txt, txtlen, name, eval_flags);
    }

    char hex[65];
    compute_sha256(txt, txtlen, hex);

    char cache_dir[] = "/tmp/wisp-bytecode-cache";
    char cache_path[256];
    snprintf(cache_path, sizeof(cache_path), "%s/%s.bin", cache_dir, hex);

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
        if (f) fclose(f);
    }

    char *txt_null_term = malloc(txtlen + 1);
    if (!txt_null_term) return JS_ThrowOutOfMemory(ctx);
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

static void do_precompile(void *arg) {
    struct precompile_arg *pa = arg;
    if (!pa) return;

    if (pa->txt && pa->txtlen > 0) {
        char hex[65];
        compute_sha256(pa->txt, pa->txtlen, hex);

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

                        JSValue compiled = JS_Eval(ctx, txt_null_term, pa->txtlen, "<precompile>", JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);
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

    if (pa->txt) free(pa->txt);
    free(pa);
}

void wisp_queue_precompile(const uint8_t *txt, size_t txtlen) {
    if (!txt || txtlen == 0) return;

    struct precompile_arg *pa = malloc(sizeof(*pa));
    if (!pa) return;
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

static bool create_secure_ipc_path(char *ipc_path_buf, size_t path_len, char *dir_buf, size_t dir_len) {
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

static void resolve_origin_from_content(void *win_priv, void *doc_priv, char *origin_buf, size_t buf_len) {
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
            snprintf(origin_buf, buf_len, "%s://%s:%s",
                     lwc_string_data(scheme), lwc_string_data(host), lwc_string_data(port));
        } else {
            snprintf(origin_buf, buf_len, "%s://%s",
                     lwc_string_data(scheme), lwc_string_data(host));
        }
    } else {
        pthread_mutex_lock(&js_processes_mutex);
        uint32_t val = ++null_origin_counter;
        pthread_mutex_unlock(&js_processes_mutex);
        snprintf(origin_buf, buf_len, "null-origin-%u", val);
    }

    if (scheme) lwc_string_unref(scheme);
    if (host) lwc_string_unref(host);
    if (port) lwc_string_unref(port);

    /* Enforce COOP isolation if option is enabled and same-origin COOP is declared */
    if (nsoption_bool(enable_coop)) {
        struct html_content *htmlc = (struct html_content *)doc_priv;
        if (htmlc->coop && (strcasecmp(htmlc->coop, "same-origin") == 0)) {
            strncat(origin_buf, "-coop-isolated", buf_len - strlen(origin_buf) - 1);
        }
    }
}

static wisp_ipc_handle *ensure_js_process_for_origin(const char *origin) {
    if (!origin) return NULL;
    if (strncmp(origin, "null-origin-", 12) == 0 ||
        strncmp(origin, "null-worker-", 12) == 0 ||
        strstr(origin, "html5test")) {
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

static void release_js_process_for_origin(const char *origin) {
    if (!origin) return;
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

static wisp_ipc_handle *get_js_process_handle(const char *origin) {
    if (!origin) return NULL;
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

static void handle_process_crash(const char *origin) {
    if (!origin) return;
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
    if (!t || !t->doc_priv) return NULL;
    if (t->win_priv && t->win_priv != t->doc_priv) {
        struct html_content *htmlc = (struct html_content *)t->doc_priv;
        return (struct dom_document *)htmlc->document;
    }
    return (struct dom_document *)t->doc_priv;
}

void js_initialise(void)
{
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
    if (!h) return NSERROR_NOMEM;
    h->rt = JS_NewRuntime();
    if (!h->rt) { free(h); return NSERROR_NOMEM; }
    h->timeout = timeout;
    JS_SetMemoryLimit(h->rt, 128 * 1024 * 1024); // Increased to 128MB
    JS_SetMaxStackSize(h->rt, 16384 * 1024);     // Increased to 16MB
    JS_SetInterruptHandler(h->rt, qjs_interrupt_handler, h);
    *heap = h;
    return NSERROR_OK;
}

void js_destroyheap(jsheap *heap)
{
    if (!heap) return;
    if (heap->rt) {
        /* Clean up the DOM bridge first while the runtime opaque is still valid.
         * qjs_bridge_cleanup will set the opaque to NULL when finished. */
        qjs_bridge_cleanup(heap->rt);
        JS_RunGC(heap->rt);
        JS_RunGC(heap->rt);
        /* QuickJS-ng: list_empty(&rt->gc_obj_list) assertion fix.
         * Explicitly free GC objects that might be pending after bridge cleanup. */
        JS_SetRuntimeOpaque(heap->rt, NULL);
        JS_FreeRuntime(heap->rt);
    }
    free(heap);
}

static void qjs_inject_fetch_polyfill(JSContext *ctx)
{
    const char *fetch_polyfill =
        "globalThis.fetch = function(url, options) {\n"
        "    return new Promise(function(resolve, reject) {\n"
        "        var xhr = new XMLHttpRequest();\n"
        "        options = options || {};\n"
        "        var method = options.method || 'GET';\n"
        "        xhr.open(method, url, true);\n"
        "        if (options.headers) {\n"
        "            for (var header in options.headers) {\n"
        "                if (options.headers.hasOwnProperty(header)) {\n"
        "                    xhr.setRequestHeader(header, options.headers[header]);\n"
        "                }\n"
        "            }\n"
        "        }\n"
        "        xhr.onload = function() {\n"
        "            var response = {\n"
        "                ok: xhr.status >= 200 && xhr.status < 300,\n"
        "                status: xhr.status,\n"
        "                statusText: xhr.statusText,\n"
        "                text: function() { return Promise.resolve(xhr.responseText); },\n"
        "                json: function() {\n"
        "                    try {\n"
        "                        return Promise.resolve(JSON.parse(xhr.responseText));\n"
        "                    } catch (e) {\n"
        "                        return Promise.reject(e);\n"
        "                    }\n"
        "                }\n"
        "            };\n"
        "            resolve(response);\n"
        "        };\n"
        "        xhr.onerror = function() {\n"
        "            reject(new TypeError('Network request failed'));\n"
        "        };\n"
        "        xhr.send(options.body || null);\n"
        "    });\n"
        "};\n"
        "globalThis.performance = globalThis.performance || {};\n"
        "globalThis.performance.now = globalThis.performance.now || function() { return Date.now(); };\n"
        "globalThis.performance.timing = globalThis.performance.timing || {\n"
        "    navigationStart: Date.now() - 100,\n"
        "    unloadEventStart: 0,\n"
        "    unloadEventEnd: 0,\n"
        "    redirectStart: 0,\n"
        "    redirectEnd: 0,\n"
        "    fetchStart: Date.now() - 80,\n"
        "    domainLookupStart: Date.now() - 80,\n"
        "    domainLookupEnd: Date.now() - 80,\n"
        "    connectStart: Date.now() - 80,\n"
        "    connectEnd: Date.now() - 80,\n"
        "    secureConnectionStart: 0,\n"
        "    requestStart: Date.now() - 50,\n"
        "    responseStart: Date.now() - 30,\n"
        "    responseEnd: Date.now() - 20,\n"
        "    domLoading: Date.now() - 10,\n"
        "    domInteractive: Date.now(),\n"
        "    domContentLoadedEventStart: Date.now(),\n"
        "    domContentLoadedEventEnd: Date.now(),\n"
        "    domComplete: Date.now(),\n"
        "    loadEventStart: Date.now(),\n"
        "    loadEventEnd: Date.now()\n"
        "};\n"
        "globalThis.performance.navigation = globalThis.performance.navigation || { type: 0, redirectCount: 0 };\n"
        "globalThis.performance.getEntries = globalThis.performance.getEntries || function() { return []; };\n"
        "globalThis.performance.getEntriesByName = globalThis.performance.getEntriesByName || function() { return []; };\n"
        "globalThis.performance.getEntriesByType = globalThis.performance.getEntriesByType || function() { return []; };\n"
        "globalThis.performance.mark = globalThis.performance.mark || function() {};\n"
        "globalThis.performance.measure = globalThis.performance.measure || function() {};\n"
        "globalThis.performance.clearMarks = globalThis.performance.clearMarks || function() {};\n"
        "globalThis.performance.clearMeasures = globalThis.performance.clearMeasures || function() {};\n"
        "if (typeof globalThis.PerformanceObserver === 'undefined') {\n"
        "  globalThis.PerformanceObserver = class {\n"
        "    constructor(callback) { this.callback = callback; }\n"
        "    observe() {}\n"
        "    disconnect() {}\n"
        "    takeRecords() { return []; }\n"
        "  };\n"
        "}\n"
        "globalThis.screen = globalThis.screen || {\n"
        "    width: 1920,\n"
        "    height: 1080,\n"
        "    availWidth: 1920,\n"
        "    availHeight: 1040,\n"
        "    colorDepth: 24,\n"
        "    pixelDepth: 24\n"
        "};\n"
        "globalThis.devicePixelRatio = globalThis.devicePixelRatio || 1.0;\n"
        "if (typeof globalThis.MessagePort === 'undefined') {\n"
        "    globalThis.MessagePort = class MessagePort {\n"
        "        constructor() {\n"
        "            this.onmessage = null;\n"
        "            this._other = null;\n"
        "        }\n"
        "        postMessage(message, transfer) {\n"
        "            var self = this;\n"
        "            if (self._other && self._other.onmessage) {\n"
        "                setTimeout(function() {\n"
        "                    if (self._other && self._other.onmessage) {\n"
        "                        self._other.onmessage({ data: message });\n"
        "                    }\n"
        "                }, 0);\n"
        "            }\n"
        "        }\n"
        "        start() {}\n"
        "        close() {}\n"
        "    };\n"
        "}\n"
        "if (typeof globalThis.MessageChannel === 'undefined') {\n"
        "    globalThis.MessageChannel = class MessageChannel {\n"
        "        constructor() {\n"
        "            this.port1 = new globalThis.MessagePort();\n"
        "            this.port2 = new globalThis.MessagePort();\n"
        "            this.port1._other = this.port2;\n"
        "            this.port2._other = this.port1;\n"
        "        }\n"
        "    };\n"
        "}\n"
        "if (typeof globalThis.getComputedStyle === 'undefined') {\n"
        "    globalThis.getComputedStyle = function(elt, pseudoElt) {\n"
        "        const dummyStyle = {\n"
        "            getPropertyValue: function(prop) {\n"
        "                if (prop === 'display') return 'block';\n"
        "                if (prop === 'width') return '1024px';\n"
        "                if (prop === 'height') return '768px';\n"
        "                if (prop === 'opacity') return '1';\n"
        "                return '';\n"
        "            },\n"
        "            getPropertyPriority: function() { return ''; },\n"
        "            setProperty: function() {},\n"
        "            removeProperty: function() {},\n"
        "            length: 0,\n"
        "            cssText: ''\n"
        "        };\n"
        "        return new Proxy(dummyStyle, {\n"
        "            get(target, prop) {\n"
        "                if (prop in target) return target[prop];\n"
        "                if (typeof prop === 'string') {\n"
        "                    if (prop === 'display') return 'block';\n"
        "                    if (prop === 'width') return '1024px';\n"
        "                    if (prop === 'height') return '768px';\n"
        "                    if (prop === 'opacity') return '1';\n"
        "                    return '';\n"
        "                }\n"
        "                return undefined;\n"
        "            }\n"
        "        });\n"
        "    };\n"
        "}\n";
    JSValue val = JS_Eval(ctx, fetch_polyfill, strlen(fetch_polyfill), "<polyfill>", JS_EVAL_TYPE_GLOBAL);
    JS_FreeValue(ctx, val);
}

nserror js_newthread(jsheap *heap, void *win_priv, void *doc_priv, jsthread **thread)
{
    JS_UpdateStackTop(heap->rt);
    jsthread *t = calloc(1, sizeof(*t));
    if (!t) return NSERROR_NOMEM;
    JS_UpdateStackTop(heap->rt);
    t->ctx = JS_NewContext(heap->rt);
    if (!t->ctx) { free(t); return NSERROR_NOMEM; }
    t->heap = heap; t->win_priv = win_priv;
    JS_SetContextOpaque(t->ctx, t);

    char origin_buf[256];
    resolve_origin_from_content(win_priv, doc_priv, origin_buf, sizeof(origin_buf));
    t->origin = strdup(origin_buf);
    ensure_js_process_for_origin(t->origin);

    /* Map shared memory segment for the thread context */
    snprintf(t->shm_dom_name, sizeof(t->shm_dom_name), "/wisp_shm_dom_%u", (unsigned int)(uintptr_t)t);
    t->shm_dom = shm_dom_create(t->shm_dom_name, true);

    /* Bridge must be initialized first */
    if (qjs_init_dom_bridge(t->ctx) != 0) {
        js_destroythread(t);
        return NSERROR_NOMEM;
    }

    /* Core registration handles skeleton creation and prototype inheritance */
    wisp_js_register_all_bindings(t->ctx);

    /* Manual refinements to prototypes must come after registration */
    if (qjs_init_eventtarget(t->ctx) != 0 ||
        qjs_init_event(t->ctx) != 0 ||
        qjs_init_node(t->ctx) != 0 ||
        qjs_init_element(t->ctx) != 0 ||
        qjs_init_document(t->ctx) != 0 ||
        qjs_init_window(t->ctx) != 0 ||
        qjs_init_console(t->ctx) != 0 ||
        qjs_init_timers(t->ctx) != 0 ||
        qjs_init_crypto(t->ctx) != 0 ||
        qjs_init_navigator(t->ctx) != 0 ||
        qjs_init_location(t->ctx) != 0 ||
        qjs_init_storage(t->ctx) != 0 ||
        qjs_init_xmlhttprequest(t->ctx) != 0 ||
        qjs_init_mutationobserver(t->ctx) != 0 ||
        qjs_init_intersectionobserver(t->ctx) != 0 ||
        qjs_init_imagedata(t->ctx) != 0 ||
        qjs_init_canvas(t->ctx) != 0 ||
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
    if (JS_IsObject(window_proto)) JS_SetPrototype(t->ctx, global_obj, window_proto);
    JS_FreeValue(t->ctx, window_proto);

    JS_DefinePropertyValueStr(t->ctx, global_obj, "window", JS_DupValue(t->ctx, global_obj), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(t->ctx, global_obj, "self", JS_DupValue(t->ctx, global_obj), JS_PROP_C_W_E);
    if (doc_priv) {
        t->doc_priv = doc_priv;
        struct dom_document *doc_node = qjs_thread_get_document(t);
        if (doc_node) {
            JS_DefinePropertyValueStr(t->ctx, global_obj, "document", qjs_wrap_node(t->ctx, (dom_node *)doc_node), JS_PROP_C_W_E);
            dom_node_ref((dom_node *)doc_node);
        }
    }

    JS_FreeValue(t->ctx, global_obj);
    qjs_inject_fetch_polyfill(t->ctx);
    *thread = t;
    return NSERROR_OK;
}

extern int qjs_init_dedicatedworkerglobalscope(JSContext *ctx);

nserror qjs_init_worker_thread(WispWorkerHandle *h, jsthread **thread_out)
{
    jsthread *t = calloc(1, sizeof(*t));
    if (!t) return NSERROR_NOMEM;

    JSRuntime *rt = JS_NewRuntime();
    if (!rt) { free(t); return NSERROR_NOMEM; }
    JS_SetMemoryLimit(rt, 128 * 1024 * 1024); // Increased to 128MB
    JS_SetMaxStackSize(rt, 16384 * 1024);     // Increased to 16MB

    t->ctx = JS_NewContext(rt);
    if (!t->ctx) { JS_FreeRuntime(rt); free(t); return NSERROR_NOMEM; }

    t->is_worker = true;
    t->worker_handle = h;
    JS_SetContextOpaque(t->ctx, t);

    char origin_buf[256];
    pthread_mutex_lock(&js_processes_mutex);
    uint32_t val = ++null_origin_counter;
    pthread_mutex_unlock(&js_processes_mutex);
    snprintf(origin_buf, sizeof(origin_buf), "null-worker-%u", val);
    t->origin = strdup(origin_buf);
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

    qjs_inject_fetch_polyfill(t->ctx);

    *thread_out = t;
    return NSERROR_OK;
}

nserror js_closethread(jsthread *thread) { if (thread) thread->closed = true; return NSERROR_OK; }

void js_destroythread(jsthread *thread)
{
    if (!thread) return;

    if (thread->ctx) {
        JSRuntime *rt = JS_GetRuntime(thread->ctx);
        JSContext *ctx1;
        while (JS_ExecutePendingJob(rt, &ctx1) > 0);
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
        JS_FreeValue(thread->ctx, tim->func);
        free(tim);
        tim = next;
    }

    struct qjs_event_listener_ctx *l = thread->listeners;
    thread->listeners = NULL;
    while (l) {
        struct qjs_event_listener_ctx *next = l->next;
        dom_event_target_remove_event_listener(l->target, l->type, l->listener, false);
        dom_node_unref((struct dom_node *)l->target);
        dom_string_unref(l->type);
        JS_FreeValue(thread->ctx, l->func);
        dom_event_listener_unref(l->listener);
        free(l);
        l = next;
    }

    struct qjs_event_map *e = thread->events;
    thread->events = NULL;
    while (e) {
        struct qjs_event_map *next = e->next;
        JS_FreeValue(thread->ctx, e->js_evt);
        dom_event_unref(e->evt);
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
        JS_FreeValue(thread->ctx, self);
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
        JS_FreeValue(thread->ctx, self);
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
        JS_FreeValue(thread->ctx, self);
    }

    qjs_cleanup_mutation_observer(thread);

    if (thread->ctx) {
        JSRuntime *rt = JS_GetRuntime(thread->ctx);
        qjs_finalise_dom_bridge(thread->ctx);
        JS_SetContextOpaque(thread->ctx, NULL);
        JS_FreeContext(thread->ctx);

        /* Final GC passes to ensure all orphaned objects (including MutationObservers) are collected. */
        JS_RunGC(rt);
        JS_RunGC(rt);
    }
    struct dom_document *doc_node = qjs_thread_get_document(thread);
    if (doc_node) dom_node_unref((dom_node *)doc_node);
    if (thread->location_url) {
        nsurl_unref(thread->location_url);
    }
    if (thread->origin) {
        release_js_process_for_origin(thread->origin);
        free(thread->origin);
    }
    if (thread->shm_dom) {
        shm_dom_destroy(thread->shm_dom, thread->shm_dom_name, true);
    }
    free(thread);
}

static void serialize_dom_node(shm_dom_t *shm, dom_node *node, uint64_t parent_id) {
    if (!node || shm->node_count >= SHM_DOM_MAX_NODES) return;

    uint32_t idx = shm->node_count++;
    extern int peak_nodes_used;
    if ((int)shm->node_count > peak_nodes_used) {
        peak_nodes_used = (int)shm->node_count;
        NSLOG(wisp, INFO, "[SHM_DOM] New record peak reached: %d nodes", peak_nodes_used);
    }
    NSLOG(wisp, INFO, "[SHM_DOM] Node allocated. Active nodes: %d", (int)shm->node_count);
    shm_dom_node_t *sn = &shm->nodes[idx];
    memset(sn, 0, sizeof(*sn));

    sn->id = (uint64_t)(uintptr_t)node;
    sn->parent_id = parent_id;

    dom_node_type type;
    dom_node_get_node_type(node, &type);
    sn->type = (uint32_t)type;

    dom_string *name = NULL;
    dom_node_get_node_name(node, &name);
    if (name) {
        size_t len = dom_string_byte_length(name);
        if (len >= SHM_DOM_STRING_MAX) len = SHM_DOM_STRING_MAX - 1;
        memcpy(sn->name, dom_string_data(name), len);
        dom_string_unref(name);
    }

    dom_string *value = NULL;
    dom_node_get_node_value(node, &value);
    if (value) {
        size_t len = dom_string_byte_length(value);
        if (len >= SHM_DOM_STRING_MAX) len = SHM_DOM_STRING_MAX - 1;
        memcpy(sn->value, dom_string_data(value), len);
        dom_string_unref(value);
    }

    if (type == DOM_ELEMENT_NODE) {
        dom_string *tag_name = NULL;
        dom_element_get_tag_name((dom_element *)node, &tag_name);
        if (tag_name) {
            size_t len = dom_string_byte_length(tag_name);
            if (len >= SHM_DOM_STRING_MAX) len = SHM_DOM_STRING_MAX - 1;
            memcpy(sn->tag_name, dom_string_data(tag_name), len);
            dom_string_unref(tag_name);
        }

        dom_namednodemap *attrs = NULL;
        dom_node_get_attributes(node, &attrs);
        if (attrs) {
            uint32_t attr_len = 0;
            dom_namednodemap_get_length(attrs, &attr_len);
            if (attr_len > 16) attr_len = 16;
            sn->attr_count = attr_len;
            for (uint32_t i = 0; i < attr_len; i++) {
                dom_node *attr_node = NULL;
                dom_namednodemap_item(attrs, i, &attr_node);
                if (attr_node) {
                    dom_string *attr_name = NULL;
                    dom_node_get_node_name(attr_node, &attr_name);
                    dom_string *attr_val = NULL;
                    dom_node_get_node_value(attr_node, &attr_val);

                    if (attr_name) {
                        size_t n_len = dom_string_byte_length(attr_name);
                        if (n_len >= SHM_DOM_STRING_MAX) n_len = SHM_DOM_STRING_MAX - 1;
                        memcpy(sn->attrs[i].name, dom_string_data(attr_name), n_len);
                        dom_string_unref(attr_name);
                    }
                    if (attr_val) {
                        size_t v_len = dom_string_byte_length(attr_val);
                        if (v_len >= SHM_DOM_STRING_MAX) v_len = SHM_DOM_STRING_MAX - 1;
                        memcpy(sn->attrs[i].value, dom_string_data(attr_val), v_len);
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
    uint64_t prev_child_id = 0;
    if (child) {
        sn->first_child_id = (uint64_t)(uintptr_t)child;
        while (child) {
            serialize_dom_node(shm, child, sn->id);

            uint64_t child_id = (uint64_t)(uintptr_t)child;
            for (uint32_t i = 0; i < shm->node_count; i++) {
                if (shm->nodes[i].id == child_id) {
                    shm->nodes[i].previous_sibling_id = prev_child_id;
                    break;
                }
            }
            if (prev_child_id != 0) {
                for (uint32_t i = 0; i < shm->node_count; i++) {
                    if (shm->nodes[i].id == prev_child_id) {
                        shm->nodes[i].next_sibling_id = child_id;
                        break;
                    }
                }
            }

            prev_child_id = child_id;
            sn->last_child_id = child_id;

            dom_node *next = NULL;
            dom_node_get_next_sibling(child, &next);
            dom_node_unref(child);
            child = next;
        }
    }
}

static void serialize_dom_tree(shm_dom_t *shm, struct dom_document *doc) {
    if (!shm || !doc) return;
    if (shm->node_count > 0) {
        NSLOG(wisp, INFO, "[SHM_DOM] Freed %d nodes prior to serialization", (int)shm->node_count);
    }
    shm->node_count = 0;
    serialize_dom_node(shm, (dom_node *)doc, 0);
}

static void apply_shm_mutation(shm_mutation_t *m, struct dom_document *doc) {
    if (!doc) return;

    dom_node *target = (dom_node *)(uintptr_t)m->target_id;
    dom_node *param1 = (dom_node *)(uintptr_t)m->param1_id;
    dom_node *param2 = (dom_node *)(uintptr_t)m->param2_id;

    switch (m->type) {
        case SHM_MUTATION_SET_ATTRIBUTE: {
            dom_string *name_dom = NULL;
            dom_string_create((const uint8_t *)m->name, strlen(m->name), &name_dom);
            dom_string *value_dom = NULL;
            dom_string_create((const uint8_t *)m->value, strlen(m->value), &value_dom);
            if (target && name_dom && value_dom) {
                dom_element_set_attribute((dom_element *)target, name_dom, value_dom);
            }
            if (name_dom) dom_string_unref(name_dom);
            if (value_dom) dom_string_unref(value_dom);
            break;
        }
        case SHM_MUTATION_REMOVE_ATTRIBUTE: {
            dom_string *name_dom = NULL;
            dom_string_create((const uint8_t *)m->name, strlen(m->name), &name_dom);
            if (target && name_dom) {
                dom_element_remove_attribute((dom_element *)target, name_dom);
            }
            if (name_dom) dom_string_unref(name_dom);
            break;
        }
        case SHM_MUTATION_APPEND_CHILD: {
            if (target && param1) {
                dom_node *result = NULL;
                dom_node_append_child(target, param1, &result);
                if (result) dom_node_unref(result);
            }
            break;
        }
        case SHM_MUTATION_REMOVE_CHILD: {
            if (target && param1) {
                dom_node *result = NULL;
                dom_node_remove_child(target, param1, &result);
                if (result) dom_node_unref(result);
            }
            break;
        }
        case SHM_MUTATION_INSERT_BEFORE: {
            if (target && param1) {
                dom_node *result = NULL;
                dom_node_insert_before(target, param1, param2, &result);
                if (result) dom_node_unref(result);
            }
            break;
        }
        case SHM_MUTATION_REPLACE_CHILD: {
            if (target && param1 && param2) {
                dom_node *result = NULL;
                dom_node_replace_child(target, param1, param2, &result);
                if (result) dom_node_unref(result);
            }
            break;
        }
        case SHM_MUTATION_SET_NODE_VALUE: {
            dom_string *ds = NULL;
            dom_string_create((const uint8_t *)m->value, strlen(m->value), &ds);
            if (target && ds) {
                dom_node_set_node_value(target, ds);
            }
            if (ds) dom_string_unref(ds);
            break;
        }
        case SHM_MUTATION_SET_TEXT_CONTENT: {
            dom_string *ds = NULL;
            dom_string_create((const uint8_t *)m->value, strlen(m->value), &ds);
            if (target && ds) {
                dom_node_set_text_content(target, ds);
            }
            if (ds) dom_string_unref(ds);
            break;
        }
    }
}

static void drain_mutation_queue(shm_dom_t *shm, struct dom_document *doc) {
    if (!shm) return;
    shm_mutation_queue_t *mq = &shm->mutation_queue;
    while (mq->tail != mq->head) {
        uint32_t idx = mq->tail % SHM_MUTATION_QUEUE_SIZE;
        shm_mutation_t *m = &mq->queue[idx];
        apply_shm_mutation(m, doc);
        mq->tail++;
    }
}

bool js_exec(jsthread *thread, const uint8_t *txt, size_t txtlen, const char *name)
{
    if (!thread || thread->closed) return false;
    JS_UpdateStackTop(JS_GetRuntime(thread->ctx));

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
            serialize_dom_tree(thread->shm_dom, doc);
        }

        /* Use thread pointer as a unique context ID for the remote process */
        uint32_t ctx_id = (uint32_t)(uintptr_t)thread;

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
            msg.length = 4 + file_prefix_len;
            msg.data = malloc(msg.length);
            if (msg.data) {
                memcpy(msg.data, &ctx_id, 4);
                memcpy(msg.data + 4, file_prefix, file_prefix_len);
            }
        } else {
            msg.length = 4 + txtlen;
            msg.data = malloc(msg.length);
            if (msg.data) {
                memcpy(msg.data, &ctx_id, 4);
                memcpy(msg.data + 4, txt, txtlen);
            }
        }

        if (msg.data) {
            if (wisp_ipc_send(ipc_js, &msg) == NSERROR_OK) {
                free(msg.data);
                /* Implement timeout for recv to avoid UI hang */
                wisp_ipc_msg response;
                wisp_ipc_set_blocking(ipc_js, false);
                int retries = 1000; // 10 seconds timeout (increased from 500)
                bool got_response = false;
                while (retries-- > 0) {
                    nserror recv_err = wisp_ipc_recv(ipc_js, &response);
                    if (recv_err == NSERROR_OK) {
                        if (response.type == WISP_IPC_MSG_JS_EXEC) {
                            got_response = true;
                            break;
                        } else {
                            /* Ignore unexpected/stale message types */
                            wisp_ipc_msg_free(&response);
                        }
                    } else if (recv_err != NSERROR_NOT_FOUND) {
                        /* Socket error or EOF -> crash detected! */
                        NSLOG(wisp, ERROR, "JS process crashed during recv for origin %s", thread->origin);
                        handle_process_crash(thread->origin);
                        break;
                    }
                    usleep(10000);
                }
                wisp_ipc_set_blocking(ipc_js, true);
                if (is_file) {
                    unlink(temp_file_path);
                }
                if (got_response) {
                    if (doc) {
                        drain_mutation_queue(thread->shm_dom, doc);
                        dom_node_unref((dom_node *)doc);
                    }
                    bool success = (response.length > 0 || response.data != NULL);
                    wisp_ipc_msg_free(&response);
                    return success;
                } else if (retries <= 0) {
                    NSLOG(wisp, ERROR, "JS process timed out for origin %s", thread->origin);
                    handle_process_crash(thread->origin);
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

    JSValue val = js_eval_with_aot_cache(thread->ctx, txt, txtlen, name, JS_EVAL_TYPE_GLOBAL);

    if (thread->heap) {
        thread->heap->deadline_ms = old_deadline;
        thread->heap->last_yield_ms = old_last_yield;
    }
    bool success = !JS_IsException(val);
    if (!success) {
        JSValue exc = JS_GetException(thread->ctx);
        const char *exc_str = JS_ToCString(thread->ctx, exc);
        NSLOG(wisp, ERROR, "JS execution error in %s: %s", name, exc_str ? exc_str : "unknown error");
        if (exc_str) JS_FreeCString(thread->ctx, exc_str);
        JS_FreeValue(thread->ctx, exc);
    }
    JS_FreeValue(thread->ctx, val);
    return success;
}

static void qjs_event_handler(struct dom_event *evt, void *pw)
{
    struct qjs_event_listener_ctx *ctx = pw;
    if (!ctx || !ctx->thread || ctx->thread->closed) return;
    JSContext *jsctx = ctx->thread->ctx;
    JSValue global = JS_GetGlobalObject(jsctx);
    JSValue js_evt = JS_UNDEFINED;
    struct qjs_event_map *map = ctx->thread->events;
    while (map) {
        if (map->evt == evt) { js_evt = JS_DupValue(jsctx, map->js_evt); break; }
        map = map->next;
    }
    if (JS_IsUndefined(js_evt)) {
        js_evt = qjs_new_event(jsctx, evt, true);
        struct qjs_event_map *new_map = malloc(sizeof(*new_map));
        if (new_map) {
            dom_event_ref(evt); new_map->evt = evt;
            new_map->js_evt = JS_DupValue(jsctx, js_evt);
            new_map->next = ctx->thread->events; ctx->thread->events = new_map;
        }
    }
    struct dom_document *doc_node_evt = qjs_thread_get_document(ctx->thread);
    JSValue this_obj = (ctx->target == (struct dom_event_target *)ctx->thread->win_priv || ctx->target == (struct dom_event_target *)doc_node_evt) ? JS_DupValue(jsctx, global) : qjs_wrap_node(jsctx, (dom_node *)ctx->target);

    uint64_t old_deadline = 0;
    uint64_t old_last_yield = 0;
    if (ctx->thread && ctx->thread->heap) {
        old_deadline = ctx->thread->heap->deadline_ms;
        old_last_yield = ctx->thread->heap->last_yield_ms;
        uint64_t now;
        nsu_getmonotonic_ms(&now);
        ctx->thread->heap->deadline_ms = now + 3000; // Absolute deadline 3s in future
        ctx->thread->heap->last_yield_ms = now;
    }

    JSValue ret = JS_Call(jsctx, ctx->func, this_obj, 1, &js_evt);

    if (ctx->thread && ctx->thread->heap) {
        ctx->thread->heap->deadline_ms = old_deadline;
        ctx->thread->heap->last_yield_ms = old_last_yield;
    }

    if (JS_IsException(ret)) {
        JSValue exc = JS_GetException(jsctx); const char *exc_str = JS_ToCString(jsctx, exc);
        if (exc_str) JS_FreeCString(jsctx, exc_str); JS_FreeValue(jsctx, exc);
    }
    JS_FreeValue(jsctx, ret); JS_FreeValue(jsctx, this_obj); JS_FreeValue(jsctx, js_evt); JS_FreeValue(jsctx, global);
}

bool js_fire_event(jsthread *thread, const char *type, struct dom_document *doc, struct dom_node *target)
{
    if (!thread || !doc) return false;
    if (!target) target = (dom_node *)doc;
    if (target == (dom_node *)thread->win_priv) {
        target = (dom_node *)qjs_thread_get_document(thread);
        if (!target) return false;
    }
    dom_string *type_str = NULL; dom_string_create((const uint8_t *)type, strlen(type), &type_str);
    dom_event *evt = NULL; dom_event_create(&evt);
    bool success = false;
    if (evt) {
        dom_event_init(evt, type_str, false, false);
        dom_event_target_dispatch_event((dom_event_target *)target, evt, &success);
        dom_event_unref(evt);
    } else {
        NSLOG(wisp, ERROR, "js_fire_event: Failed to create dom_event");
    }
    dom_string_unref(type_str);
    return success;
}

bool js_dom_event_add_listener(jsthread *thread, struct dom_document *document, struct dom_node *node, struct dom_string *event_type_dom, JSValue js_funcval)
{
    if (!thread || !node) return false;
    if (node == (struct dom_node *)thread->win_priv) {
        node = (struct dom_node *)qjs_thread_get_document(thread);
        if (!node) return false;
    }
    struct qjs_event_listener_ctx *ctx = malloc(sizeof(*ctx));
    if (!ctx) return false;
    ctx->thread = thread; ctx->func = JS_DupValue(thread->ctx, js_funcval);
    ctx->target = (struct dom_event_target *)node; ctx->type = event_type_dom;
    dom_node_ref(node); dom_string_ref(event_type_dom);
    dom_event_listener *listener;
    if (dom_event_listener_create(qjs_event_handler, ctx, &listener) != DOM_NO_ERR) {
        dom_node_unref(node); dom_string_unref(event_type_dom); JS_FreeValue(thread->ctx, ctx->func); free(ctx);
        return false;
    }
    ctx->listener = listener;
    dom_event_target_add_event_listener(ctx->target, ctx->type, listener, false);
    ctx->next = thread->listeners; thread->listeners = ctx;
    return true;
}

bool js_dom_event_remove_listener(jsthread *thread, struct dom_document *document, struct dom_node *node, struct dom_string *event_type_dom, JSValue js_funcval)
{
    if (!thread || !node) return false;
    if (node == (struct dom_node *)thread->win_priv) {
        node = (struct dom_node *)qjs_thread_get_document(thread);
        if (!node) return false;
    }
    struct qjs_event_listener_ctx **prev = &thread->listeners;
    struct qjs_event_listener_ctx *curr = thread->listeners;
    while (curr) {
        if (curr->target == (struct dom_event_target *)node && dom_string_isequal(curr->type, event_type_dom) && JS_VALUE_GET_PTR(curr->func) == JS_VALUE_GET_PTR(js_funcval)) {
            dom_event_target_remove_event_listener(curr->target, curr->type, curr->listener, false);
            dom_node_unref((struct dom_node *)curr->target); dom_string_unref(curr->type);
            JS_FreeValue(thread->ctx, curr->func); dom_event_listener_unref(curr->listener);
            *prev = curr->next; free(curr); return true;
        }
        prev = &curr->next; curr = curr->next;
    }
    return false;
}

void js_handle_new_element(jsthread *thread, struct dom_element *node) {}

bool js_event_cleanup(jsthread *thread, struct dom_event *evt)
{
    if (!thread || !evt) return false;
    struct qjs_event_map **prev = &thread->events, *curr = thread->events;
    while (curr) {
        if (curr->evt == evt) {
            *prev = curr->next; JS_FreeValue(thread->ctx, curr->js_evt);
            dom_event_unref(evt); free(curr); return true;
        }
        prev = &curr->next; curr = curr->next;
    }
    return false;
}

JSValue qjs_new_intersectionobserverentry_manual(JSContext *ctx, WispIntersectionObserverEntry *entry);

void js_handle_intersection_check(jsthread *thread, struct box *layout, int viewport_width, int viewport_height)
{
    if (!thread || !thread->intersection_observers || !layout) return;
    uint64_t now_ms; nsu_getmonotonic_ms(&now_ms);
    WispIntersectionObserver *obs = thread->intersection_observers;
    while (obs) {
        bool changed = false; IntersectionObserverTarget *ot = obs->targets;

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
                int tx, ty; box_coords(target_box, &tx, &ty);
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
                    WispIntersectionObserverEntry entry; memset(&entry, 0, sizeof(entry));
                    entry.time = (double)now_ms; entry.target = ot->node; dom_node_ref(ot->node);
                    entry.isIntersecting = isIntersecting; entry.targetX = tx; entry.targetY = ty;
                    entry.targetWidth = tw; entry.targetHeight = th;
                    entry.rootWidth = rx1 - rx0; entry.rootHeight = ry1 - ry0;
                    if (isIntersecting) {
                        entry.intersectX = ix0; entry.intersectY = iy0;
                        entry.intersectWidth = ix1 - ix0; entry.intersectHeight = iy1 - iy0;
                        entry.intersectionRatio = currentRatio;
                    } else {
                        entry.intersectionRatio = 0.0;
                    }
                    uint32_t len = 0; JSValue js_len = JS_GetPropertyStr(obs->ctx, obs->queue, "length");
                    JS_ToUint32(obs->ctx, &len, js_len); JS_FreeValue(obs->ctx, js_len);
                    JS_SetPropertyUint32(obs->ctx, obs->queue, len, qjs_new_intersectionobserverentry_manual(obs->ctx, &entry));
                    ot->wasIntersecting = isIntersecting;
                    ot->lastRatio = currentRatio;
                    changed = true;
                }
            }
            ot = ot->next;
        }
        if (changed) {
            JSValue args[2]; args[0] = obs->queue; args[1] = JS_NULL;
            JSValue ret = JS_Call(obs->ctx, obs->callback, JS_UNDEFINED, 2, args);
            JS_FreeValue(obs->ctx, ret); JS_FreeValue(obs->ctx, obs->queue);
            obs->queue = JS_NewArray(obs->ctx);
        }
        obs = obs->next;
    }
}
