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
        "            return this._childNodes.filter(node => node.nodeType === 1);\n"
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
        "            for (var header in options.headers) {\n"
        "                if (options.headers.hasOwnProperty(header)) {\n"
        "                    xhr.setRequestHeader(header, options.headers[header]);\n"
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
        "                    const m = _entries.find(e => e.entryType === 'mark' && e.name === startMarkOrOptions.start);\n"
        "                    startTime = m ? m.startTime : 0;\n"
        "                } else {\n"
        "                    startTime = Number(startMarkOrOptions.start);\n"
        "                }\n"
        "            }\n"
        "            if (startMarkOrOptions.end !== undefined) {\n"
        "                if (typeof startMarkOrOptions.end === 'string') {\n"
        "                    const m = _entries.find(e => e.entryType === 'mark' && e.name === startMarkOrOptions.end);\n"
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
        "                const m = _entries.find(e => e.entryType === 'mark' && e.name === startMarkOrOptions);\n"
        "                startTime = m ? m.startTime : 0;\n"
        "            } else if (startMarkOrOptions !== undefined) {\n"
        "                startTime = Number(startMarkOrOptions);\n"
        "            }\n"
        "\n"
        "            if (typeof endMark === 'string') {\n"
        "                const m = _entries.find(e => e.entryType === 'mark' && e.name === endMark);\n"
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
        "        } else {\n"
        "            name = String(name);\n"
        "            for (let i = _entries.length - 1; i >= 0; i--) {\n"
        "                if (_entries[i].entryType === 'mark' && _entries[i].name === name) {\n"
        "                    _entries.splice(i, 1);\n"
        "                }\n"
        "            }\n"
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
        "}\n"
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
        "if (typeof globalThis.URL === 'undefined' || !globalThis.URL.prototype.pathname) {\n"
        "    globalThis.URL = class URL {\n"
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
        "if (globalThis.navigator) {\n"
        "    globalThis.navigator.mediaDevices = globalThis.navigator.mediaDevices || new class MediaDevices extends globalThis.EventTarget {\n"
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
        "            const probed = globalThis.navigator.__probe_devices ? globalThis.navigator.__probe_devices(audio_requested, video_requested) : { audio: true, video: true };\n"
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
        "        const evt = new OriginalEvent(type);\n"
        "        evt._composed = !!options.composed;\n"
        "        Object.defineProperty(evt, 'bubbles', { value: !!options.bubbles, configurable: true, enumerable: true });\n"
        "        Object.defineProperty(evt, 'cancelable', { value: !!options.cancelable, configurable: true, enumerable: true });\n"
        "        return evt;\n"
        "    };\n"
        "    globalThis.Event.prototype = OriginalEvent.prototype;\n"
        "\n"
        "    EventTarget.prototype.dispatchEvent = function(event) {\n"
        "        if (!event) return false;\n"
        "        \n"
        "        event._target = this;\n"
        "        \n"
        "        const fullPath = [];\n"
        "        let curr = this;\n"
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
        "            if (node === this) continue;\n"
        "            \n"
        "            event._target = getRetargetedTarget(this, node);\n"
        "            event._currentTarget = node;\n"
        "            \n"
        "            const listeners = node.__wisp_listeners ? node.__wisp_listeners[event.type] : null;\n"
        "            if (listeners) {\n"
        "                for (const record of listeners) {\n"
        "                    if (record.capture) {\n"
        "                        try {\n"
        "                            record.callback.call(node, event);\n"
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
        "            event._target = this;\n"
        "            event._currentTarget = this;\n"
        "            \n"
        "            const listeners = this.__wisp_listeners ? this.__wisp_listeners[event.type] : null;\n"
        "            if (listeners) {\n"
        "                for (const record of listeners) {\n"
        "                    try {\n"
        "                        record.callback.call(this, event);\n"
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
        "                if (node === this) continue;\n"
        "                \n"
        "                event._target = getRetargetedTarget(this, node);\n"
        "                event._currentTarget = node;\n"
        "                \n"
        "                const listeners = node.__wisp_listeners ? node.__wisp_listeners[event.type] : null;\n"
        "                if (listeners) {\n"
        "                    for (const record of listeners) {\n"
        "                        if (!record.capture) {\n"
        "                            try {\n"
        "                                record.callback.call(node, event);\n"
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
        "    define(name, constructor) {\n"
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
        "            attributeChangedCallback: constructor.prototype.attributeChangedCallback\n"
        "        });\n"
        "        this._upgradeAll(document.documentElement, name);\n"
        "    }\n"
        "\n"
        "    get(name) {\n"
        "        return this._registry.get(name.toLowerCase())?.constructor;\n"
        "    }\n"
        "\n"
        "    _upgradeNode(node) {\n"
        "        if (!node || node.nodeType !== 1) return;\n"
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
        "globalThis.customElements = new CustomElementRegistry();\n"
        "globalThis.__wisp_custom_elements_registry = globalThis.customElements;\n"
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
        "";
    JSValue val = JS_Eval(ctx, fetch_polyfill, strlen(fetch_polyfill), "<polyfill>", JS_EVAL_TYPE_GLOBAL);
    JS_FreeValue(ctx, val);
}

static void qjs_lifecycle_mutation_hook(
    dom_mutation_hook_category category,
    dom_mutation_type type,
    struct dom_node *target,
    struct dom_node *related,
    struct dom_string *prev_value,
    struct dom_string *new_value,
    struct dom_string *attr_name,
    struct dom_string *attr_ns,
    void *pw)
{
    jsthread *t = pw;
    if (!t || t->closed || !t->ctx) return;
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
                        js_prev = JS_NewStringLen(ctx, (const char *)dom_string_data(prev_value), dom_string_byte_length(prev_value));
                    }
                    JSValue js_new = JS_NULL;
                    if (new_value) {
                        js_new = JS_NewStringLen(ctx, (const char *)dom_string_data(new_value), dom_string_byte_length(new_value));
                    }

                    JSValue args[4] = { js_target, js_attr_name, js_prev, js_new };
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
            dom_document_set_mutation_hook(doc_node, qjs_lifecycle_mutation_hook, t);
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
        int job_ret;
        while ((job_ret = JS_ExecutePendingJob(rt, &ctx1)) != 0) {
            if (job_ret < 0) {
                JSValue exc = JS_GetException(ctx1);
                const char *exc_str = JS_ToCString(ctx1, exc);
                NSLOG(wisp, WARNING, "JS Error in microtask during teardown: %s", exc_str ? exc_str : "unknown");
                if (exc_str) JS_FreeCString(ctx1, exc_str);
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
        JS_FreeValue(thread->ctx, tim->func);
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
        JS_FreeValue(thread->ctx, raf->func);
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
        JS_FreeValue(thread->ctx, idle->func);
        free(idle);
        idle = next;
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
    if (doc_node) {
        dom_document_set_mutation_hook(doc_node, NULL, NULL);
        dom_node_unref((dom_node *)doc_node);
    }
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

static uint16_t get_tag_atom_from_name(const char *tag_name) {
    if (!tag_name) return 0;
    if (strcasecmp(tag_name, "html") == 0) return 1;
    if (strcasecmp(tag_name, "head") == 0) return 2;
    if (strcasecmp(tag_name, "body") == 0) return 3;
    if (strcasecmp(tag_name, "title") == 0) return 4;
    if (strcasecmp(tag_name, "div") == 0) return 5;
    if (strcasecmp(tag_name, "span") == 0) return 6;
    if (strcasecmp(tag_name, "p") == 0) return 7;
    if (strcasecmp(tag_name, "a") == 0) return 8;
    if (strcasecmp(tag_name, "script") == 0) return 9;
    if (strcasecmp(tag_name, "style") == 0) return 10;
    if (strcasecmp(tag_name, "link") == 0) return 11;
    if (strcasecmp(tag_name, "img") == 0) return 12;
    if (strcasecmp(tag_name, "iframe") == 0) return 13;
    return 14; // Other tag
}

static uint32_t compute_class_hash(const char *class_str) {
    if (!class_str) return 0;
    uint32_t hash = 5381;
    int c;
    while ((c = (unsigned char)*class_str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static void serialize_dom_node(shm_dom_t *shm, dom_node *node, WispNodeID parent_idx) {
    if (!node || shm->node_count >= SHM_DOM_MAX_NODES) return;

    uint32_t idx = shm->node_count++;
    extern int peak_nodes_used;
    if ((int)shm->node_count > peak_nodes_used) {
        peak_nodes_used = (int)shm->node_count;
        NSLOG(wisp, INFO, "[SHM_DOM] New record peak reached: %d nodes", peak_nodes_used);
    }
    NSLOG(wisp, INFO, "[SHM_DOM] Node allocated. Active nodes: %d", (int)shm->node_count);
    WispCompactNode *sn = &shm->nodes[idx];
    memset(sn, 0, sizeof(*sn));

    WispNodeStrings *sns = &shm->node_strings[idx];
    memset(sns, 0, sizeof(*sns));

    shm->dom_ptrs[idx] = (uint64_t)(uintptr_t)node;
    sn->parent_id = parent_idx;

    dom_node_type type;
    dom_node_get_node_type(node, &type);
    sn->node_type = (uint16_t)type;

    dom_string *name = NULL;
    dom_node_get_node_name(node, &name);
    if (name) {
        size_t len = dom_string_byte_length(name);
        if (len >= SHM_DOM_STRING_MAX) len = SHM_DOM_STRING_MAX - 1;
        memcpy(sns->name, dom_string_data(name), len);
        sns->name[len] = '\0';
        dom_string_unref(name);
    }

    dom_string *value = NULL;
    dom_node_get_node_value(node, &value);
    if (value) {
        size_t len = dom_string_byte_length(value);
        if (len >= SHM_DOM_STRING_MAX) len = SHM_DOM_STRING_MAX - 1;
        memcpy(sns->value, dom_string_data(value), len);
        sns->value[len] = '\0';
        dom_string_unref(value);
    }

    if (type == DOM_ELEMENT_NODE) {
        dom_string *tag_name = NULL;
        dom_element_get_tag_name((dom_element *)node, &tag_name);
        if (tag_name) {
            size_t len = dom_string_byte_length(tag_name);
            if (len >= SHM_DOM_STRING_MAX) len = SHM_DOM_STRING_MAX - 1;
            memcpy(sns->tag_name, dom_string_data(tag_name), len);
            sns->tag_name[len] = '\0';
            sn->tag_atom = get_tag_atom_from_name(sns->tag_name);
            dom_string_unref(tag_name);
        }

        dom_namednodemap *attrs = NULL;
        dom_node_get_attributes(node, &attrs);
        if (attrs) {
            uint32_t attr_len = 0;
            dom_namednodemap_get_length(attrs, &attr_len);
            if (attr_len > 16) attr_len = 16;
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
                        size_t n_len = dom_string_byte_length(attr_name);
                        if (n_len >= SHM_DOM_STRING_MAX) n_len = SHM_DOM_STRING_MAX - 1;
                        memcpy(sns->attrs[i].name, dom_string_data(attr_name), n_len);
                        sns->attrs[i].name[n_len] = '\0';

                        if (strcasecmp((const char *)dom_string_data(attr_name), "class") == 0 && attr_val) {
                            sn->class_hash = compute_class_hash((const char *)dom_string_data(attr_val));
                        }

                        dom_string_unref(attr_name);
                    }
                    if (attr_val) {
                        size_t v_len = dom_string_byte_length(attr_val);
                        if (v_len >= SHM_DOM_STRING_MAX) v_len = SHM_DOM_STRING_MAX - 1;
                        memcpy(sns->attrs[i].value, dom_string_data(attr_val), v_len);
                        sns->attrs[i].value[v_len] = '\0';
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
        sn->first_child_id = shm->node_count;
        while (child) {
            uint32_t child_idx = shm->node_count;
            serialize_dom_node(shm, child, idx);

            shm->nodes[child_idx].prev_sibling_id = prev_child_idx;
            if (prev_child_idx != 0) {
                shm->nodes[prev_child_idx].next_sibling_id = child_idx;
            }

            prev_child_idx = child_idx;

            dom_node *next = NULL;
            dom_node_get_next_sibling(child, &next);
            dom_node_unref(child);
            child = next;
        }
    }
}

void serialize_dom_tree(shm_dom_t *shm, struct dom_document *doc) {
    if (!shm || !doc) return;
    if (shm->node_count > 0) {
        NSLOG(wisp, INFO, "[SHM_DOM] Freed %d nodes prior to serialization", (int)shm->node_count);
    }
    memset(shm->dom_ptrs, 0, sizeof(shm->dom_ptrs));
    shm->node_count = 1; // Start indices at 1, 0 is WISP_NODE_NULL
    shm->layout_cache_count = 0;
    memset(shm->layout_cache, 0, sizeof(shm->layout_cache));
    serialize_dom_node(shm, (dom_node *)doc, 0);
}

static dom_node* get_dom_node_from_id(shm_dom_t *shm, uint64_t id) {
    if (!shm || id == 0 || id == 0xFFFFFFFF) return NULL;
    uint32_t idx = (uint32_t)id;
    if (idx < shm->node_count) {
        return (dom_node *)(uintptr_t)shm->dom_ptrs[idx];
    }
    return NULL;
}

static void apply_shm_mutation(shm_dom_t *shm, shm_mutation_t *m, struct dom_document *doc) {
    if (!doc) return;

    dom_node *target = get_dom_node_from_id(shm, m->target_id);
    dom_node *param1 = get_dom_node_from_id(shm, m->param1_id);
    dom_node *param2 = get_dom_node_from_id(shm, m->param2_id);

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

static void update_shm_box_bounds_recursive(shm_dom_t *shm, struct box *box) {
    if (!box) return;
    if (box->node && shm) {
        for (uint32_t i = 0; i < shm->node_count; i++) {
            if (shm->dom_ptrs[i] == (uint64_t)(uintptr_t)box->node) {
                WispCompactNode *node = &shm->nodes[i];
                if (node->layout_index == 0) {
                    uint32_t l_idx = ++shm->layout_cache_count;
                    if (l_idx < SHM_DOM_MAX_NODES) {
                        node->layout_index = l_idx;
                    } else {
                        node->layout_index = SHM_DOM_MAX_NODES - 1;
                    }
                }
                uint32_t l_idx = node->layout_index;
                WispShmLayoutCache *lc = &shm->layout_cache[l_idx];

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
        update_shm_box_bounds_recursive(shm, child);
    }
}

void qjs_update_shm_box_bounds(struct jsthread *thread, struct box *doc_box) {
    if (!thread || !thread->shm_dom || !doc_box) return;
    shm_dom_t *shm = thread->shm_dom;

    update_shm_box_bounds_recursive(shm, doc_box);

    shm->layout_dirty = false;
}

extern bool layout_document(struct html_content *content, int width, int height);

static void force_synchronous_layout(struct jsthread *thread) {
    struct html_content *htmlc = (thread->win_priv && thread->win_priv != thread->doc_priv) ? (struct html_content *)thread->doc_priv : NULL;
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
    shm_mutation_queue_t *mq = &shm->mutation_queue;
    while (mq->tail != mq->head) {
        uint32_t idx = mq->tail % SHM_MUTATION_QUEUE_SIZE;
        shm_mutation_t *m = &mq->queue[idx];
        apply_shm_mutation(shm, m, doc);
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
                bool crashed = false;
                while (retries-- > 0) {
                    nserror recv_err = wisp_ipc_recv(ipc_js, &response);
                    if (recv_err == NSERROR_OK) {
                        if (response.type == WISP_IPC_MSG_JS_EXEC) {
                            got_response = true;
                            break;
                        } else if (response.type == WISP_IPC_MSG_DOM_REQUEST) {
                            if (doc) {
                                drain_mutation_queue(thread->shm_dom, doc);
                                force_synchronous_layout(thread);
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
                        NSLOG(wisp, ERROR, "JS process crashed during recv for origin %s", thread->origin);
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

    JSValue val = js_eval_with_aot_cache(thread->ctx, txt, txtlen, name, JS_EVAL_TYPE_GLOBAL);

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
            if (exc_str) JS_FreeCString(ctx1, exc_str);
            JS_FreeValue(ctx1, exc);
        }
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

    JSContext *ctx1;
    int job_ret;
    while ((job_ret = JS_ExecutePendingJob(JS_GetRuntime(jsctx), &ctx1)) != 0) {
        if (job_ret < 0) {
            JSValue exc = JS_GetException(ctx1);
            const char *exc_str = JS_ToCString(ctx1, exc);
            NSLOG(wisp, WARNING, "JS Error in microtask: %s", exc_str ? exc_str : "unknown");
            if (exc_str) JS_FreeCString(ctx1, exc_str);
            JS_FreeValue(ctx1, exc);
        }
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
