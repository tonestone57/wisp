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

            kill(curr->pid, SIGKILL);
            waitpid(curr->pid, NULL, 0);

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

static int qjs_interrupt_handler(JSRuntime *rt, void *opaque)
{
    struct jsheap *heap = opaque;
    uint64_t now;
    if (heap->deadline_ms > 0) {
        nsu_getmonotonic_ms(&now);
        if (now > heap->deadline_ms) return 1;
    }
    return 0;
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
    JS_SetMemoryLimit(h->rt, 64 * 1024 * 1024);
    JS_SetMaxStackSize(h->rt, 8192 * 1024);
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
    JS_SetMaxStackSize(rt, 8192 * 1024);

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
    if (thread->origin) {
        release_js_process_for_origin(thread->origin);
        free(thread->origin);
    }
    free(thread);
}

bool js_exec(jsthread *thread, const uint8_t *txt, size_t txtlen, const char *name)
{
    if (!thread || thread->closed) return false;
    JS_UpdateStackTop(JS_GetRuntime(thread->ctx));

    wisp_ipc_handle *ipc_js = get_js_process_handle(thread->origin);
    if (ipc_js) {
        /* Use thread pointer as a unique context ID for the remote process */
        uint32_t ctx_id = (uint32_t)(uintptr_t)thread;

        wisp_ipc_msg msg;
        msg.type = WISP_IPC_MSG_JS_EXEC;
        msg.length = 4 + txtlen;
        msg.data = malloc(msg.length);
        if (msg.data) {
            memcpy(msg.data, &ctx_id, 4);
            memcpy(msg.data + 4, txt, txtlen);

            if (wisp_ipc_send(ipc_js, &msg) == NSERROR_OK) {
                free(msg.data);
                /* Implement timeout for recv to avoid UI hang */
                wisp_ipc_msg response;
                wisp_ipc_set_blocking(ipc_js, false);
                int retries = 500; // 5 seconds
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
                if (got_response) {
                    bool success = (response.length > 0 || response.data != NULL);
                    wisp_ipc_msg_free(&response);
                    return success;
                } else if (retries <= 0) {
                    NSLOG(wisp, ERROR, "JS process timed out for origin %s", thread->origin);
                    handle_process_crash(thread->origin);
                }
            } else {
                free(msg.data);
                NSLOG(wisp, ERROR, "JS process write failed for origin %s (likely crashed)", thread->origin);
                handle_process_crash(thread->origin);
            }
        }
        /* Fallback to in-process if IPC fails or times out */
        NSLOG(wisp, WARNING, "JS IPC failed for %s, falling back to in-process", name);
    }

    JSValue val = js_eval_with_aot_cache(thread->ctx, txt, txtlen, name, JS_EVAL_TYPE_GLOBAL);
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
    JSValue ret = JS_Call(jsctx, ctx->func, this_obj, 1, &js_evt);
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
