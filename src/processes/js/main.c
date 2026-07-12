#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wisp/utils/ipc.h>
#include <wisp/utils/log.h>
#include <wisp/utils/corestrings.h>
#include "quickjs.h"

extern JSValue js_eval_with_aot_cache(JSContext *ctx, const uint8_t *txt, size_t txtlen, const char *name, int eval_flags);

static wisp_ipc_handle *ipc_main;
static JSRuntime *rt;

struct js_context_node {
    uint32_t id;
    JSContext *ctx;
    struct js_context_node *next;
};

static struct js_context_node *contexts = NULL;

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
    return node->ctx;
}

int main(int argc, char **argv) {
    if (argc < 2) return 1;
    const char *ipc_name = argv[1];

    ipc_main = wisp_ipc_connect(ipc_name);
    if (!ipc_main) return 1;

    corestrings_init();
    rt = JS_NewRuntime();

    while (1) {
        wisp_ipc_msg msg;
        if (wisp_ipc_recv(ipc_main, &msg) != NSERROR_OK) break;

        if (msg.type == WISP_IPC_MSG_JS_EXEC) {
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
        JS_FreeContext(curr->ctx);
        free(curr);
        curr = next;
    }
    JS_FreeRuntime(rt);
    wisp_ipc_destroy(ipc_main);
    return 0;
}
