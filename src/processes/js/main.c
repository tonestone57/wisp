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
#include "content/handlers/javascript/quickjs/dom_bridge.h"
#include "js_process.h"

extern JSValue js_eval_with_aot_cache(JSContext *ctx, const uint8_t *txt, size_t txtlen, const char *name, int eval_flags);
extern JSModuleDef *wisp_module_loader(JSContext *ctx, const char *module_name, void *opaque);
extern char *wisp_module_normalize(JSContext *ctx, const char *base_name, const char *name, void *opaque);

JSRuntime *rt;
char *js_process_origin = NULL;

struct js_context_node *contexts = NULL;

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

static void url_decode_inplace(char *str) {
    if (!str) return;
    char *src = str, *dst = str;
    while (*src) {
        if (*src == '%' && src[1] && src[2]) {
            int h1 = src[1], h2 = src[2];
            int v1 = (h1 >= '0' && h1 <= '9') ? h1 - '0' : (h1 >= 'a' && h1 <= 'f') ? h1 - 'a' + 10 : (h1 >= 'A' && h1 <= 'F') ? h1 - 'A' + 10 : -1;
            int v2 = (h2 >= '0' && h2 <= '9') ? h2 - '0' : (h2 >= 'a' && h2 <= 'f') ? h2 - 'a' + 10 : (h2 >= 'A' && h2 <= 'F') ? h2 - 'A' + 10 : -1;
            if (v1 != -1 && v2 != -1) {
                *dst++ = (char)((v1 << 4) | v2);
                src += 3;
                continue;
            }
        }
        *dst++ = *src++;
    }
    *dst = '\0';
}

static uint64_t find_shm_doc_node_id(void)
{
    if (!wisp_shm_dom) return 0;
    WispCompactNode *nodes_arr = shm_dom_get_nodes(wisp_shm_dom);
    if (!nodes_arr) return 0;
    for (uint32_t i = 0; i < wisp_shm_dom->node_count; i++) {
        if (nodes_arr[i].node_type == 9) { /* DOM_DOCUMENT_NODE is 9 */
            return (uint64_t)i;
        }
    }
    return 0;
}

JSValue global_document_get(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (t) {
        /* If doc_priv is not set, try to find it in the shm dom */
        if (!t->doc_priv && wisp_shm_dom) {
            uint64_t doc_node_id = find_shm_doc_node_id();
            if (doc_node_id != 0) {
                t->doc_priv = (void*)(uintptr_t)doc_node_id;
                t->win_priv = (void*)(uintptr_t)doc_node_id;
                t->global_window_priv.node = (void*)(uintptr_t)doc_node_id;
            }
        }
        struct dom_document *doc_node = qjs_thread_get_document(t);
        if (doc_node) {
            return qjs_wrap_node(ctx, (dom_node *)doc_node);
        }
    }
    return JS_NULL;
}

#define JS_CTX_HASH_SIZE 64

static struct js_context_node *ctx_hash_table[JS_CTX_HASH_SIZE];
static struct js_context_node *last_accessed_ctx = NULL;

JSContext* get_context(uint32_t id) {
    if (!contexts) {
        last_accessed_ctx = NULL;
        memset(ctx_hash_table, 0, sizeof(ctx_hash_table));
    } else if (last_accessed_ctx && last_accessed_ctx->id == id && last_accessed_ctx->ctx) {
        return last_accessed_ctx->ctx;
    }

    uint32_t slot = id % JS_CTX_HASH_SIZE;
    struct js_context_node *curr = ctx_hash_table[slot];
    while (curr) {
        if (curr->id == id) {
            last_accessed_ctx = curr;
            return curr->ctx;
        }
        curr = curr->hash_next;
    }

    struct js_context_node *node = calloc(1, sizeof(*node));
    if (!node) return NULL;
    struct jsthread *t = calloc(1, sizeof(*t));
    if (!t) { free(node); return NULL; }
    qjs_thread_init_wrapper_cache(t);
    node->id = id;
    node->ctx = JS_NewContext(rt);
    if (!node->ctx) { free(t); free(node); return NULL; }
    node->thread = t;

    /* Set thread context and opaque early so callbacks during binding/polyfill setup have opaque access */
    t->ctx = node->ctx;
    JS_SetContextOpaque(node->ctx, t);

    /* Find document node ID in shm_dom to set up as the root */
    uint64_t doc_node_id = find_shm_doc_node_id();

    t->doc_priv = (void*)(uintptr_t)doc_node_id;
    t->win_priv = (void*)(uintptr_t)doc_node_id;
    t->global_window_priv.magic = QJS_DOM_MAGIC;
    t->global_window_priv.node = (void*)(uintptr_t)doc_node_id;
    t->global_window_priv.ctx = node->ctx;
    t->global_window_priv.is_dom_node = false;
    t->shm_dom = wisp_shm_dom;
    t->shm_capacity = wisp_shm_capacity;

    if (js_process_origin) {
        t->origin = strdup(js_process_origin);
        if (!t->origin) {
            JS_SetContextOpaque(node->ctx, NULL);
            JS_FreeContext(node->ctx);
            free(t);
            free(node);
            return NULL;
        }
    }

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

    qjs_inject_dom_polyfills(node->ctx);
    wisp_qjs_register_core_polyfills(node->ctx);

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

    /* Link into contexts list and hash table slot */
    node->next = contexts;
    contexts = node;
    node->hash_next = ctx_hash_table[slot];
    ctx_hash_table[slot] = node;
    last_accessed_ctx = node;

    return node->ctx;
}

void js_process_handle_ipc_msg(const wisp_ipc_msg *msg) {
    if (!msg) return;

    if (msg->type == WISP_IPC_MSG_SHM_INIT) {
        if (!msg->data && msg->length > 0) return;
        char *payload = malloc(msg->length + 1);
        if (payload) {
            if (msg->length > 0) {
                memcpy(payload, msg->data, msg->length);
            }
            payload[msg->length] = '\0';

            if (wisp_shm_dom) {
                shm_dom_destroy(wisp_shm_dom, NULL, false);
            }

            char *shm_name = payload;
            char *origin = NULL;
            char *delim = strchr(payload, '|');
            if (delim) {
                *delim = '\0';
                origin = delim + 1;
            }

            wisp_shm_dom = shm_dom_create(shm_name, 0, false);
            wisp_shm_capacity = wisp_shm_dom ? wisp_shm_dom->node_capacity : 0;

            if (origin) {
                char *new_orig = strdup(origin);
                if (new_orig) {
                    if (js_process_origin) free(js_process_origin);
                    js_process_origin = new_orig;
                }
            } else {
                if (js_process_origin) {
                    free(js_process_origin);
                    js_process_origin = NULL;
                }
            }

            uint64_t new_doc_id = find_shm_doc_node_id();

            /* Update any already-created contexts */
            struct js_context_node *curr_c = contexts;
            while (curr_c) {
                if (curr_c->thread) {
                    if (origin) {
                        char *new_thread_orig = strdup(origin);
                        if (new_thread_orig) {
                            if (curr_c->thread->origin) free(curr_c->thread->origin);
                            curr_c->thread->origin = new_thread_orig;
                        } else {
                            if (curr_c->thread->origin) {
                                free(curr_c->thread->origin);
                                curr_c->thread->origin = NULL;
                            }
                        }
                        if (curr_c->thread->location_url) {
                            nsurl_unref(curr_c->thread->location_url);
                            curr_c->thread->location_url = NULL;
                        }
                    } else {
                        if (curr_c->thread->origin) {
                            free(curr_c->thread->origin);
                            curr_c->thread->origin = NULL;
                        }
                        if (curr_c->thread->location_url) {
                            nsurl_unref(curr_c->thread->location_url);
                            curr_c->thread->location_url = NULL;
                        }
                    }
                    curr_c->thread->doc_priv = (void*)(uintptr_t)new_doc_id;
                    curr_c->thread->win_priv = (void*)(uintptr_t)new_doc_id;
                    curr_c->thread->global_window_priv.node = (void*)(uintptr_t)new_doc_id;
                    curr_c->thread->shm_dom = wisp_shm_dom;
                    curr_c->thread->shm_capacity = wisp_shm_capacity;
                }
                curr_c = curr_c->next;
            }
            free(payload);
        }
    } else if (msg->type == WISP_IPC_MSG_JS_EXEC) {
        /* Format: [ctx_id(4)][eval_flags(4)][name_len(4)][name...][script...] */
        if (msg->length >= 12 && msg->data) {
            uint32_t ctx_id;
            uint32_t eval_flags;
            uint32_t name_len;
            memcpy(&ctx_id, msg->data, 4);
            memcpy(&eval_flags, msg->data + 4, 4);
            memcpy(&name_len, msg->data + 8, 4);

            if (name_len > msg->length - 12) {
                wisp_ipc_msg response;
                memset(&response, 0, sizeof(response));
                response.type = WISP_IPC_MSG_JS_EXEC;
                response.length = 0;
                response.data = NULL;
                wisp_ipc_send(ipc_main, &response);
                return;
            }

            char *script_name = NULL;
            if (name_len > 0) {
                script_name = malloc(name_len + 1);
                if (script_name) {
                    memcpy(script_name, msg->data + 12, name_len);
                    script_name[name_len] = '\0';
                }
            }
            if (!script_name) {
                script_name = strdup("<ipc>");
            }

            JSContext *ctx = get_context(ctx_id);
            if (!ctx) {
                free(script_name);
                wisp_ipc_msg response;
                memset(&response, 0, sizeof(response));
                response.type = WISP_IPC_MSG_JS_EXEC;
                response.length = 0;
                response.data = NULL;
                wisp_ipc_send(ipc_main, &response);
                return;
            }

            size_t offset = 12 + name_len;
            size_t script_len = msg->length - offset;
            char *script = NULL;
            bool is_file_payload = false;

            if (script_len >= 7 && strncmp((const char *)(msg->data + offset), "file://", 7) == 0) {
                is_file_payload = true;
                size_t path_len = script_len - 7;
                char *file_path = malloc(path_len + 1);
                if (file_path) {
                    memcpy(file_path, msg->data + offset + 7, path_len);
                    file_path[path_len] = '\0';
                    FILE *f = fopen(file_path, "rb");
                    if (!f && strchr(file_path, '%')) {
                        /* Fallback to URL decoding if direct path opening failed */
                        url_decode_inplace(file_path);
                        f = fopen(file_path, "rb");
                    }
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
                        /* Clean up temporary script file from disk */
                        unlink(file_path);
                    }
                    free(file_path);
                }
            }

            if (!script && !is_file_payload) {
                script = malloc(script_len + 1);
                if (script) {
                    memcpy(script, msg->data + offset, script_len);
                    script[script_len] = '\0';
                }
            }

            if (script) {
                if (wisp_shm_dom) {
                    // Query capacity safely; avoid write lock if capacity has not changed.
                    if (wisp_shm_capacity < wisp_shm_dom->node_capacity) {
                        shm_dom_lock_write(wisp_shm_dom);
                        if (wisp_shm_capacity < wisp_shm_dom->node_capacity) {
                            uint32_t new_cap = wisp_shm_dom->node_capacity;
                            shm_dom_t *old_shm = wisp_shm_dom;
                            wisp_shm_dom = shm_dom_remap(wisp_shm_dom, wisp_shm_capacity, new_cap);
                            if (wisp_shm_dom) {
                                wisp_shm_capacity = new_cap;
                                shm_dom_unlock_write(wisp_shm_dom);
                            } else {
                                wisp_shm_capacity = 0;
                            }
                            struct js_context_node *curr_c = contexts;
                            while (curr_c) {
                                if (curr_c->thread) {
                                    curr_c->thread->shm_dom = wisp_shm_dom;
                                    curr_c->thread->shm_capacity = wisp_shm_capacity;
                                }
                                curr_c = curr_c->next;
                            }
                        } else {
                            shm_dom_unlock_write(wisp_shm_dom);
                        }
                    }
                }

                JSValue val = JS_UNDEFINED;
                struct jsthread *t = JS_GetContextOpaque(ctx);
                if (t) {
                    t->current_script_name = script_name;
                }
                val = js_eval_with_aot_cache(ctx, (const uint8_t *)script, script_len, script_name, eval_flags);
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
                        JSValue stack = JS_UNDEFINED;
                        if (JS_IsObject(exc)) {
                            stack = JS_GetPropertyStr(ctx1, exc, "stack");
                        }
                        if (!JS_IsUndefined(stack) && !JS_IsNull(stack)) {
                            const char *stack_str = JS_ToCString(ctx1, stack);
                            if (stack_str) {
                                fprintf(stderr, "Stack Trace:\n%s\n", stack_str);
                                JS_FreeCString(ctx1, stack_str);
                            }
                        }
                        JS_FreeValue(ctx1, stack);
                        if (exc_str) JS_FreeCString(ctx1, exc_str);
                        JS_FreeValue(ctx1, exc);
                    }
                }
                wisp_in_microtask = false;

                /* Flush the Batch-Buffered Mutation Queue (BBMQ) */
                extern void bbmq_flush(void);
                bbmq_flush();

                wisp_ipc_msg response;
                memset(&response, 0, sizeof(response));
                response.type = WISP_IPC_MSG_JS_EXEC;
                if (JS_IsException(val)) {
                    JSValue exc = JS_GetException(ctx);
                    const char *exc_str = JS_ToCString(ctx, exc);
                    fprintf(stderr, "\n=== JS PROCESS EXCEPTION in script [%s]: %s ===\n", script_name ? script_name : "<unknown>", exc_str ? exc_str : "unknown");
                    JSValue stack = JS_UNDEFINED;
                    if (JS_IsObject(exc)) {
                        stack = JS_GetPropertyStr(ctx, exc, "stack");
                    }
                    if (!JS_IsUndefined(stack) && !JS_IsNull(stack)) {
                        const char *stack_str = JS_ToCString(ctx, stack);
                        if (stack_str) {
                            fprintf(stderr, "Stack Trace:\n%s\n", stack_str);
                            JS_FreeCString(ctx, stack_str);
                        }
                    }
                    JS_FreeValue(ctx, stack);
                    if (exc_str) JS_FreeCString(ctx, exc_str);
                    JS_FreeValue(ctx, exc);
                    response.length = 0;
                    response.data = NULL;
                } else {
                    size_t res_len = 0;
                    const char *res_str = JS_ToCStringLen(ctx, &res_len, val);
                    if (res_str) {
                        if (res_len > 0) {
                            response.data = (uint8_t *)malloc(res_len);
                            if (response.data) {
                                memcpy(response.data, res_str, res_len);
                                response.length = res_len;
                            } else {
                                response.length = 0;
                                response.data = NULL;
                            }
                        } else {
                            response.length = 0;
                            response.data = NULL;
                        }
                        JS_FreeCString(ctx, res_str);
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
                memset(&response, 0, sizeof(response));
                response.type = WISP_IPC_MSG_JS_EXEC;
                response.length = 0;
                response.data = NULL;
                wisp_ipc_send(ipc_main, &response);
            }
        } else {
            wisp_ipc_msg response;
            memset(&response, 0, sizeof(response));
            response.type = WISP_IPC_MSG_JS_EXEC;
            response.length = 0;
            response.data = NULL;
            wisp_ipc_send(ipc_main, &response);
        }
    }
}

int js_process_main(int argc, char **argv) {
    if (argc < 2) return 1;
    const char *ipc_name = argv[1];

    wisp_is_js_process = true;

    ipc_main = wisp_ipc_connect(ipc_name);
    if (!ipc_main) return 1;
    wisp_ipc_set_blocking(ipc_main, false);

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
            bool did_work = false;
            while (curr_c) {
                if (curr_c->ctx) {
                    JSContext *ctx1;
                    int job_ret;
                    wisp_in_microtask = true;
                    while ((job_ret = JS_ExecutePendingJob(JS_GetRuntime(curr_c->ctx), &ctx1)) != 0) {
                        wait_time = 0;
                        did_work = true;
                        if (job_ret < 0) {
                            JSValue exc = JS_GetException(ctx1);
                            const char *exc_str = JS_ToCString(ctx1, exc);
                            fprintf(stderr, "\n=== IDLE MICROTASK JS Error: %s ===\n", exc_str ? exc_str : "unknown");
                            JSValue stack = JS_UNDEFINED;
                            if (JS_IsObject(exc)) {
                                stack = JS_GetPropertyStr(ctx1, exc, "stack");
                            }
                            if (!JS_IsUndefined(stack) && !JS_IsNull(stack)) {
                                const char *stack_str = JS_ToCString(ctx1, stack);
                                if (stack_str) {
                                    fprintf(stderr, "Stack Trace:\n%s\n", stack_str);
                                    JS_FreeCString(ctx1, stack_str);
                                }
                            }
                            JS_FreeValue(ctx1, stack);
                            if (exc_str) JS_FreeCString(ctx1, exc_str);
                            JS_FreeValue(ctx1, exc);
                        }
                    }
                    wisp_in_microtask = false;
                    uint64_t ctx_wait = qjs_execute_timers(curr_c->ctx);
                    if (ctx_wait == 0) did_work = true;
                    if (ctx_wait < wait_time) {
                        wait_time = ctx_wait;
                    }
                }
                curr_c = curr_c->next;
            }
            if (did_work) {
                extern void bbmq_flush(void);
                bbmq_flush();
            }
            if (wait_time > 0) {
                if (wait_time > 5) wait_time = 5;
                usleep(wait_time * 1000);
            }
            continue;
        } else if (err == NSERROR_SHUTDOWN) {
            break;
        } else if (err != NSERROR_OK) {
            fprintf(stderr, "\n=== JS PROCESS RECEIVE FAILED: error %d ===\n", (int)err);
            break;
        }

        js_process_handle_ipc_msg(&msg);
        wisp_ipc_msg_free(&msg);
    }

    /* Cleanup */
    last_accessed_ctx = NULL;
    struct js_context_node *curr = contexts;
    while (curr) {
        struct js_context_node *next = curr->next;
        if (curr->thread) {
            js_destroythread(curr->thread);
            curr->thread = NULL;
            curr->ctx = NULL;
        } else if (curr->ctx) {
            qjs_finalise_dom_bridge(rt, curr->ctx);
            JS_SetContextOpaque(curr->ctx, NULL);
            JS_FreeContext(curr->ctx);
            curr->ctx = NULL;
        }
        free(curr);
        curr = next;
    }
    contexts = NULL;
    memset(ctx_hash_table, 0, sizeof(ctx_hash_table));

    qjs_bridge_cleanup(rt);
    if (rt) {
        JS_RunGC(rt);
        JS_RunGC(rt);
        JS_FreeRuntime(rt);
        rt = NULL;
    }
    if (wisp_shm_dom) {
        shm_dom_destroy(wisp_shm_dom, NULL, false);
        wisp_shm_dom = NULL;
    }
    if (ipc_main) {
        wisp_ipc_handle *to_destroy = ipc_main;
        ipc_main = NULL;
        wisp_ipc_destroy(to_destroy);
    }
    if (js_process_origin) {
        free(js_process_origin);
        js_process_origin = NULL;
    }
    corestrings_fini();
    fprintf(stderr, "\n=== JS PROCESS EXITING NORMALLY ===\n");
    return 0;
}

#ifndef WISP_JS_TESTING
int main(int argc, char **argv) {
    return js_process_main(argc, argv);
}
#endif
