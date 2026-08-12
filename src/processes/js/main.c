#include "content/handlers/javascript/quickjs/qjs_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wisp/utils/ipc.h>
#include <wisp/utils/log.h>
#include <wisp/utils/corestrings.h>
#include <wisp/utils/nsurl.h>
#include "quickjs.h"
#include "content/handlers/javascript/quickjs/qjs_internal.h"
#include "content/handlers/javascript/quickjs/dom_bridge.h"

extern JSValue js_eval_with_aot_cache(JSContext *ctx, const uint8_t *txt, size_t txtlen, const char *name, int eval_flags);
extern JSModuleDef *wisp_module_loader(JSContext *ctx, const char *module_name, void *opaque);
extern char *wisp_module_normalize(JSContext *ctx, const char *base_name, const char *name, void *opaque);

static JSRuntime *rt;
static char *js_process_origin = NULL;

struct js_context_node {
    uint32_t id;
    JSContext *ctx;
    struct jsthread *thread;
    struct js_context_node *next;
};

static struct js_context_node *contexts = NULL;

extern bool wisp_is_js_process;
extern shm_dom_t *wisp_shm_dom;
extern uint32_t wisp_shm_capacity;

extern bool wisp_in_microtask;
extern wisp_ipc_handle *ipc_main;

extern int qjs_init_dom_bridge(JSContext *ctx);
extern int qjs_init_eventtarget(JSContext *ctx);
extern int qjs_init_event(JSContext *ctx);
extern int qjs_init_node(JSContext *ctx);
extern int qjs_init_element(JSContext *ctx);
extern int qjs_init_document(JSContext *ctx);
extern int qjs_init_window(JSContext *ctx);
extern int qjs_init_console(JSContext *ctx);
extern int qjs_init_timers(JSContext *ctx);
extern int qjs_init_crypto(JSContext *ctx);
extern int qjs_init_navigator(JSContext *ctx);
extern int qjs_init_location(JSContext *ctx);
extern int qjs_init_storage(JSContext *ctx);
extern int qjs_init_xmlhttprequest(JSContext *ctx);
extern int qjs_init_mutationobserver(JSContext *ctx);
extern int qjs_init_intersectionobserver(JSContext *ctx);
extern int qjs_init_imagedata(JSContext *ctx);
extern int qjs_init_canvas(JSContext *ctx);
extern int qjs_init_trusted_types(JSContext *ctx);

static JSValue global_document_get(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (t) {
        /* If doc_priv is not set, try to find it in the shm dom */
        if (!t->doc_priv && wisp_shm_dom) {
            uint64_t doc_node_id = 0;
            WispCompactNode *nodes_arr = shm_dom_get_nodes(wisp_shm_dom);
            for (uint32_t i = 0; i < wisp_shm_dom->node_count; i++) {
                if (nodes_arr[i].node_type == 9) { /* DOM_DOCUMENT_NODE is 9 */
                    doc_node_id = i;
                    break;
                }
            }
            if (doc_node_id != 0) {
                t->doc_priv = (void*)(uintptr_t)doc_node_id;
            }
        }
        struct dom_document *doc_node = qjs_thread_get_document(t);
        if (doc_node) {
            return qjs_wrap_node(ctx, (dom_node *)doc_node);
        }
    }
    return JS_UNDEFINED;
}

static JSContext* get_context(uint32_t id) {
    struct js_context_node *curr = contexts;
    while (curr) {
        if (curr->id == id) return curr->ctx;
        curr = curr->next;
    }
    struct js_context_node *node = malloc(sizeof(*node));
    if (!node) return NULL;
    struct jsthread *t = calloc(1, sizeof(*t));
    if (!t) { free(node); return NULL; }
    node->id = id;
    node->ctx = JS_NewContext(rt);
    if (!node->ctx) { free(t); free(node); return NULL; }
    node->thread = NULL;
    node->next = contexts;
    contexts = node;

    /* Initialize bindings */
    qjs_init_dom_bridge(node->ctx);
    wisp_js_register_all_bindings(node->ctx);

    qjs_init_eventtarget(node->ctx);
    qjs_init_event(node->ctx);
    qjs_init_node(node->ctx);
    qjs_init_element(node->ctx);
    qjs_init_document(node->ctx);
    qjs_init_window(node->ctx);
    qjs_init_console(node->ctx);
    qjs_init_timers(node->ctx);
    qjs_init_crypto(node->ctx);
    qjs_init_navigator(node->ctx);
    qjs_init_location(node->ctx);
    qjs_init_storage(node->ctx);
    qjs_init_xmlhttprequest(node->ctx);
    qjs_init_mutationobserver(node->ctx);
    qjs_init_intersectionobserver(node->ctx);
    qjs_init_imagedata(node->ctx);
    qjs_init_canvas(node->ctx);
    qjs_init_trusted_types(node->ctx);

    qjs_inject_fetch_polyfill(node->ctx);

    /* Setup dummy jsthread for the remote context so opaque callbacks match */
    t->ctx = node->ctx;
    if (js_process_origin) {
        t->origin = strdup(js_process_origin);
    }

    /* Find document node ID in shm_dom to set up as the root */
    uint64_t doc_node_id = 0;
    if (wisp_shm_dom) {
        WispCompactNode *nodes_arr = shm_dom_get_nodes(wisp_shm_dom);
        for (uint32_t i = 0; i < wisp_shm_dom->node_count; i++) {
            if (nodes_arr[i].node_type == 9) { /* DOM_DOCUMENT_NODE is 9 */
                doc_node_id = i;
                break;
            }
        }
    }

    t->doc_priv = (void*)(uintptr_t)doc_node_id;
    t->win_priv = (void*)(uintptr_t)doc_node_id;
    t->global_window_priv.magic = QJS_DOM_MAGIC;
    t->global_window_priv.node = (void*)(uintptr_t)doc_node_id;
    t->global_window_priv.ctx = node->ctx;
    t->global_window_priv.is_dom_node = false;

    JS_SetContextOpaque(node->ctx, t);
    node->thread = t;

    /* Setup window/document on global object */
    JSValue global_obj = JS_GetGlobalObject(node->ctx);
    JSValue window_proto = JS_GetClassProto(node->ctx, qjs_window_class_id);
    if (JS_IsObject(window_proto)) JS_SetPrototype(node->ctx, global_obj, window_proto);
    JS_FreeValue(node->ctx, window_proto);

    JS_DefinePropertyValueStr(node->ctx, global_obj, "__wisp_is_js_process", JS_TRUE, 0);
    JS_DefinePropertyValueStr(node->ctx, global_obj, "window", JS_DupValue(node->ctx, global_obj), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(node->ctx, global_obj, "self", JS_DupValue(node->ctx, global_obj), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(node->ctx, global_obj, "parent", JS_DupValue(node->ctx, global_obj), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(node->ctx, global_obj, "top", JS_DupValue(node->ctx, global_obj), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(node->ctx, global_obj, "frames", JS_DupValue(node->ctx, global_obj), JS_PROP_C_W_E);

    /* Define 'document' accessor on the global object */
    JSAtom doc_atom = JS_NewAtom(node->ctx, "document");
    JSValue doc_getter = JS_NewCFunction(node->ctx, global_document_get, "get_document", 0);
    JS_DefinePropertyGetSet(node->ctx, global_obj, doc_atom, doc_getter, JS_UNDEFINED, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(node->ctx, doc_atom);

    JS_FreeValue(node->ctx, global_obj);

    return node->ctx;
}

int main(int argc, char **argv) {
    if (argc < 2) return 1;
    const char *ipc_name = argv[1];

    wisp_is_js_process = true;

    ipc_main = wisp_ipc_connect(ipc_name);
    if (!ipc_main) return 1;

    corestrings_init();
    rt = JS_NewRuntime();
    JS_SetMemoryLimit(rt, 128 * 1024 * 1024); // Increased to 128MB
    JS_SetMaxStackSize(rt, 16384 * 1024);     // Increased to 16MB
    JS_SetModuleLoaderFunc(rt, wisp_module_normalize, wisp_module_loader, NULL);

    while (1) {
        wisp_ipc_msg msg;
        nserror err = wisp_ipc_recv(ipc_main, &msg);
        if (err == NSERROR_NOT_FOUND) {
            struct js_context_node *curr_c = contexts;
            uint64_t wait_time = 1000;
            while (curr_c) {
                if (curr_c->ctx) {
                    JSContext *ctx1;
                    int job_ret;
                    while ((job_ret = JS_ExecutePendingJob(JS_GetRuntime(curr_c->ctx), &ctx1)) != 0) {
                        wait_time = 0;
                        if (job_ret < 0) {
                            JSValue exc = JS_GetException(ctx1);
                            JS_FreeValue(ctx1, exc);
                        }
                    }
                    uint64_t ctx_wait = qjs_execute_timers(curr_c->ctx);
                    if (ctx_wait < wait_time) {
                        wait_time = ctx_wait;
                    }
                }
                curr_c = curr_c->next;
            }
            if (wait_time > 0) {
                usleep(wait_time * 1000);
            }
            continue;
        } else if (err == NSERROR_SHUTDOWN) {
            break;
        } else if (err != NSERROR_OK) {
            fprintf(stderr, "\n=== JS PROCESS RECEIVE FAILED: error %d ===\n", (int)err);
            break;
        }

        if (msg.type == WISP_IPC_MSG_SHM_INIT) {
            char *payload = malloc(msg.length + 1);
            if (payload) {
                memcpy(payload, msg.data, msg.length);
                payload[msg.length] = '\0';

                char *delim = strchr(payload, '|');
                if (delim) {
                    *delim = '\0';
                    char *shm_name = payload;
                    char *origin = delim + 1;

                    if (wisp_shm_dom) {
                        shm_dom_destroy(wisp_shm_dom, NULL, false);
                    }
                    wisp_shm_dom = shm_dom_create(shm_name, 0, false);
                    wisp_shm_capacity = wisp_shm_dom ? wisp_shm_dom->node_capacity : 0;

                    if (js_process_origin) free(js_process_origin);
                    js_process_origin = strdup(origin);

                    /* Update any already-created contexts */
                    struct js_context_node *curr_c = contexts;
                    while (curr_c) {
                        if (curr_c->thread) {
                            if (curr_c->thread->origin) free(curr_c->thread->origin);
                            curr_c->thread->origin = strdup(origin);
                            if (curr_c->thread->location_url) {
                                nsurl_unref(curr_c->thread->location_url);
                                curr_c->thread->location_url = NULL;
                            }
                        }
                        curr_c = curr_c->next;
                    }
                } else {
                    if (wisp_shm_dom) {
                        shm_dom_destroy(wisp_shm_dom, NULL, false);
                    }
                    wisp_shm_dom = shm_dom_create(payload, 0, false);
                    wisp_shm_capacity = wisp_shm_dom ? wisp_shm_dom->node_capacity : 0;
                }
                free(payload);
            }
        } else if (msg.type == WISP_IPC_MSG_JS_EXEC) {
            /* Format: [ctx_id(4)][eval_flags(4)][name_len(4)][name...][script...] */
            if (msg.length >= 12) {
                uint32_t ctx_id;
                uint32_t eval_flags;
                uint32_t name_len;
                memcpy(&ctx_id, msg.data, 4);
                memcpy(&eval_flags, msg.data + 4, 4);
                memcpy(&name_len, msg.data + 8, 4);

                char *script_name = NULL;
                if (name_len > 0 && name_len <= msg.length - 12) {
                    script_name = malloc(name_len + 1);
                    if (script_name) {
                        memcpy(script_name, msg.data + 12, name_len);
                        script_name[name_len] = '\0';
                    }
                }
                if (!script_name) {
                    script_name = strdup("<ipc>");
                }

                JSContext *ctx = get_context(ctx_id);
                if (!ctx) {
                    free(script_name);
                    wisp_ipc_msg_free(&msg);
                    continue;
                }

                if (name_len > msg.length - 12) name_len = 0;
                size_t offset = 12 + name_len;
                size_t script_len = msg.length - offset;
                char *script = NULL;
                if (script_len >= 7 && strncmp((const char *)(msg.data + offset), "file://", 7) == 0) {
                    char file_path[512];
                    size_t path_len = script_len - 7;
                    if (path_len < sizeof(file_path)) {
                        memcpy(file_path, msg.data + offset + 7, path_len);
                        file_path[path_len] = '\0';
                        FILE *f = fopen(file_path, "rb");
                        if (f) {
                            fseek(f, 0, SEEK_END);
                            long sz = ftell(f);
                            fseek(f, 0, SEEK_SET);
                            if (sz >= 0) {
                                script = malloc(sz + 1);
                                if (script) {
                                    if (fread(script, 1, sz, f) == (size_t)sz) {
                                        script[sz] = '\0';
                                        script_len = sz;
                                    } else {
                                        free(script);
                                        script = NULL;
                                    }
                                }
                            }
                            fclose(f);
                        }
                    }
                }

                if (!script) {
                    script = malloc(script_len + 1);
                    if (script) {
                        memcpy(script, msg.data + offset, script_len);
                        script[script_len] = '\0';
                    }
                }

                if (script) {
                    if (wisp_shm_dom) {
                        // Query the capacity safely, remapping if necessary, but under write lock or fine-grained lock.
                        // Actually, wisp_shm_dom has fine-grained locks. We should not hold a read lock across
                        // the entire script execution. Instead, if capacity has grown, let's remap under a write lock.
                        shm_dom_lock_write(wisp_shm_dom);
                        if (wisp_shm_capacity < wisp_shm_dom->node_capacity) {
                            uint32_t new_cap = wisp_shm_dom->node_capacity;
                            wisp_shm_dom = shm_dom_remap(wisp_shm_dom, wisp_shm_capacity, new_cap);
                            if (wisp_shm_dom) {
                                wisp_shm_capacity = new_cap;
                            } else {
                                wisp_shm_capacity = 0;
                            }
                        }
                        shm_dom_unlock_write(wisp_shm_dom);
                    }

                    JSValue val = JS_UNDEFINED;
                    struct jsthread *t = JS_GetContextOpaque(ctx);
                    if (t) {
                        t->current_script_name = script_name;
                    }
                    if (wisp_shm_dom) {
                        val = js_eval_with_aot_cache(ctx, (const uint8_t *)script, script_len, script_name, eval_flags);
                    }
                    if (t) {
                        t->current_script_name = NULL;
                    }

                    /* Execute any pending microtasks (microtask-tick serialization) */
                    wisp_in_microtask = true;
                    JSContext *ctx1;
                    int job_ret;
                    while ((job_ret = JS_ExecutePendingJob(rt, &ctx1)) != 0) {
                        if (job_ret < 0) {
                            JSValue exc = JS_GetException(ctx1);
                            const char *exc_str = JS_ToCString(ctx1, exc);
                            fprintf(stderr, "\n=== MICROTASK JS Error: %s ===\n", exc_str ? exc_str : "unknown");
                            if (exc_str) JS_FreeCString(ctx1, exc_str);
                            JS_FreeValue(ctx1, exc);
                        }
                    }
                    wisp_in_microtask = false;

                    /* Flush the Batch-Buffered Mutation Queue (BBMQ) */
                    extern void bbmq_flush(void);
                    bbmq_flush();

                    wisp_ipc_msg response;
                    response.type = WISP_IPC_MSG_JS_EXEC;
                    if (JS_IsException(val)) {
                        JSValue exc = JS_GetException(ctx);
                        const char *exc_str = JS_ToCString(ctx, exc);
                        fprintf(stderr, "\n=== JS PROCESS EXCEPTION: %s ===\n", exc_str ? exc_str : "unknown");
                        JSValue stack = JS_UNDEFINED;
                        if (JS_IsObject(exc)) {
                            stack = JS_GetPropertyStr(ctx, exc, "stack");
                        }
                        const char *stack_str = JS_ToCString(ctx, stack);
                        if (stack_str) {
                            fprintf(stderr, "Stack Trace:\n%s\n", stack_str);
                            JS_FreeCString(ctx, stack_str);
                        }
                        JS_FreeValue(ctx, stack);
                        if (exc_str) JS_FreeCString(ctx, exc_str);
                        JS_FreeValue(ctx, exc);
                        response.length = 0;
                        response.data = NULL;
                    } else {
                        const char *res_str = JS_ToCString(ctx, val);
                        if (res_str) {
                            response.length = strlen(res_str);
                            response.data = (uint8_t*)strdup(res_str);
                            if (res_str) JS_FreeCString(ctx, res_str);
                        } else {
                            response.length = 0;
                            response.data = NULL;
                        }
                    }
                    wisp_ipc_send(ipc_main, &response);
                    wisp_ipc_msg_free(&response);
                    JS_FreeValue(ctx, val);
                    free(script);
                    free(script_name);
                } else {
                    free(script_name);
                    wisp_ipc_msg response;
                    response.type = WISP_IPC_MSG_JS_EXEC;
                    response.length = 0;
                    response.data = NULL;
                    wisp_ipc_send(ipc_main, &response);
                }
            }
        }
        wisp_ipc_msg_free(&msg);
    }

    /* Cleanup */
    struct js_context_node *curr = contexts;
    while (curr) {
        if (curr->ctx) {
            qjs_finalise_dom_bridge(rt, curr->ctx);
            JS_SetContextOpaque(curr->ctx, NULL);
            JS_FreeContext(curr->ctx);
        }
        curr = curr->next;
    }

    qjs_bridge_cleanup(rt);
    JS_FreeRuntime(rt);

    curr = contexts;
    while (curr) {
        struct js_context_node *next = curr->next;
        if (curr->thread) {
            if (curr->thread->location_url) {
                nsurl_unref(curr->thread->location_url);
            }
            if (curr->thread->origin) free(curr->thread->origin);
            free(curr->thread);
        }
        free(curr);
        curr = next;
    }
    if (wisp_shm_dom) {
        shm_dom_destroy(wisp_shm_dom, NULL, false);
    }
    if (ipc_main) {
        wisp_ipc_handle *to_destroy = ipc_main;
        ipc_main = NULL;
        wisp_ipc_destroy(to_destroy);
    }
    if (js_process_origin) free(js_process_origin);
    corestrings_fini();
    fprintf(stderr, "\n=== JS PROCESS EXITING NORMALLY ===\n");
    return 0;
}
