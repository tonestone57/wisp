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

JSContext* get_context(uint32_t id);
JSValue global_document_get(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

#endif
