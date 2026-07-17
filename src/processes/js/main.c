#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wisp/utils/ipc.h>
#include <wisp/utils/log.h>
#include <wisp/utils/corestrings.h>
#include "quickjs.h"
#include "content/handlers/javascript/quickjs/qjs_internal.h"
#include "content/handlers/javascript/quickjs/dom_bridge.h"

extern JSValue js_eval_with_aot_cache(JSContext *ctx, const uint8_t *txt, size_t txtlen, const char *name, int eval_flags);

static wisp_ipc_handle *ipc_main;
static JSRuntime *rt;

struct js_context_node {
    uint32_t id;
    JSContext *ctx;
    struct js_context_node *next;
};

static struct js_context_node *contexts = NULL;

extern bool wisp_is_js_process;
extern shm_dom_t *wisp_shm_dom;

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

static JSContext* get_context(uint32_t id) {
    struct js_context_node *curr = contexts;
    while (curr) {
        if (curr->id == id) return curr->ctx;
        curr = curr->next;
    }
    struct js_context_node *node = malloc(sizeof(*node));
    node->id = id;
    node->ctx = JS_NewContext(rt);
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

    /* Setup dummy jsthread for the remote context so opaque callbacks match */
    struct jsthread *t = calloc(1, sizeof(*t));
    t->ctx = node->ctx;

    /* Find document node ID in shm_dom to set up as the root */
    uint64_t doc_node_id = 0;
    if (wisp_shm_dom) {
        for (uint32_t i = 0; i < wisp_shm_dom->node_count; i++) {
            if (wisp_shm_dom->nodes[i].type == 9) { /* DOM_DOCUMENT_NODE is 9 */
                doc_node_id = wisp_shm_dom->nodes[i].id;
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

    /* Setup window/document on global object */
    JSValue global_obj = JS_GetGlobalObject(node->ctx);
    JSValue window_proto = JS_GetClassProto(node->ctx, qjs_window_class_id);
    if (JS_IsObject(window_proto)) JS_SetPrototype(node->ctx, global_obj, window_proto);
    JS_FreeValue(node->ctx, window_proto);

    JS_DefinePropertyValueStr(node->ctx, global_obj, "window", JS_DupValue(node->ctx, global_obj), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(node->ctx, global_obj, "self", JS_DupValue(node->ctx, global_obj), JS_PROP_C_W_E);
    if (doc_node_id != 0) {
        JS_DefinePropertyValueStr(node->ctx, global_obj, "document", qjs_wrap_node(node->ctx, (struct dom_node *)(uintptr_t)doc_node_id), JS_PROP_C_W_E);
    }
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
    JS_SetMaxStackSize(rt, 8192 * 1024);

    while (1) {
        wisp_ipc_msg msg;
        if (wisp_ipc_recv(ipc_main, &msg) != NSERROR_OK) break;

        if (msg.type == WISP_IPC_MSG_SHM_INIT) {
            char *shm_name = malloc(msg.length + 1);
            if (shm_name) {
                memcpy(shm_name, msg.data, msg.length);
                shm_name[msg.length] = '\0';
                if (wisp_shm_dom) {
                    shm_dom_destroy(wisp_shm_dom, NULL, false);
                }
                wisp_shm_dom = shm_dom_create(shm_name, false);
                free(shm_name);
            }
        } else if (msg.type == WISP_IPC_MSG_JS_EXEC) {
            /* Format: [ctx_id(4)][script...] */
            if (msg.length >= 4) {
                uint32_t ctx_id;
                memcpy(&ctx_id, msg.data, 4);
                JSContext *ctx = get_context(ctx_id);

                size_t script_len = msg.length - 4;
                char *script = malloc(script_len + 1);
                if (script) {
                    memcpy(script, msg.data + 4, script_len);
                    script[script_len] = '\0';

                    JSValue val = js_eval_with_aot_cache(ctx, (const uint8_t *)script, script_len, "<ipc>", JS_EVAL_TYPE_GLOBAL);

                    wisp_ipc_msg response;
                    response.type = WISP_IPC_MSG_JS_EXEC;
                    if (JS_IsException(val)) {
                        response.length = 0;
                        response.data = NULL;
                    } else {
                        const char *res_str = JS_ToCString(ctx, val);
                        if (res_str) {
                            response.length = strlen(res_str);
                            response.data = (uint8_t*)strdup(res_str);
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
                } else {
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
        struct js_context_node *next = curr->next;
        struct jsthread *t = JS_GetContextOpaque(curr->ctx);
        if (t) free(t);
        JS_FreeContext(curr->ctx);
        free(curr);
        curr = next;
    }
    JS_FreeRuntime(rt);
    if (wisp_shm_dom) {
        shm_dom_destroy(wisp_shm_dom, NULL, false);
    }
    wisp_ipc_destroy(ipc_main);
    return 0;
}
