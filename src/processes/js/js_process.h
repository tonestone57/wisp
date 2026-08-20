#ifndef WISP_JS_PROCESS_H
#define WISP_JS_PROCESS_H

#include <stdint.h>
#include "quickjs.h"

struct js_context_node {
    uint32_t id;
    JSContext *ctx;
    struct jsthread *thread;
    struct js_context_node *next;
    struct js_context_node *hash_next;
};

extern struct js_context_node *contexts;
extern JSRuntime *rt;
extern char *js_process_origin;

#include <wisp/utils/ipc.h>

JSContext* get_context(uint32_t id);
JSValue global_document_get(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

void js_process_handle_ipc_msg(const wisp_ipc_msg *msg);
int js_process_main(int argc, char **argv);

#endif
