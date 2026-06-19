#ifndef QJS_INTERNAL_H
#define QJS_INTERNAL_H

#include "quickjs.h"
#include <pthread.h>
#include <stdbool.h>

struct jsthread {
    JSRuntime *rt;
    JSContext *ctx;
    pthread_t thread;
    bool closed;
    struct qjs_event_map *events;
    void *win_priv;
    void *doc_priv;
    struct jsheap *heap;
    struct qjs_event_listener_ctx *listeners;
};

void qjs_finalise_dom_bridge(JSContext *ctx);

#endif
