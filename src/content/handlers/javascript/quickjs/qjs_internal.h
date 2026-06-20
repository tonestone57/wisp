#ifndef WISP_QUICKJS_INTERNAL_H
#define WISP_QUICKJS_INTERNAL_H

#include "quickjs.h"
#include "utils/libdom.h"
#include "content/handlers/javascript/js.h"

struct jsheap {
    JSRuntime *rt;
    int timeout;
    uint64_t deadline_ms;
};

struct qjs_event_listener_ctx {
    struct qjs_event_listener_ctx *next;
    struct jsthread *thread;
    JSValue func;
    struct dom_event_target *target;
    struct dom_string *type;
    struct dom_event_listener *listener;
};

struct qjs_event_map {
    struct qjs_event_map *next;
    struct dom_event *evt;
    JSValue js_evt;
};

struct qjs_timer {
    JSContext *ctx;
    JSValue func;
    bool repeat;
    int interval;
    int id;
    bool cancelled;
    struct qjs_timer *next;
};

struct jsthread {
    JSContext *ctx;
    struct jsheap *heap;
    void *win_priv;
    void *doc_priv;
    bool closed;
    struct qjs_event_listener_ctx *listeners;
    struct qjs_event_map *events;
    struct qjs_timer *timers;
};

void *qjs_get_window_priv(JSContext *ctx);
void *qjs_get_document_priv(JSContext *ctx);

#endif /* WISP_QUICKJS_INTERNAL_H */
