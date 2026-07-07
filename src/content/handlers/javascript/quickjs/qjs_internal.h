#ifndef WISP_QUICKJS_INTERNAL_H
#define WISP_QUICKJS_INTERNAL_H

#include "quickjs.h"
#include <stdbool.h>
#include <stdint.h>
#include "utils/libdom.h"
#include "content/handlers/javascript/js.h"

/* Private data for JS DOM objects */
typedef struct QJSNodePrivate {
    uint32_t magic;         /* Magic number for type safety */
    void *node;             /* Underlying LibDOM node/object */
    JSContext *ctx;         /* Associated context */
    bool is_dom_node;       /* True if node is dom_node* (needs unref) */
} QJSNodePrivate;

#define QJS_DOM_MAGIC 0x57495350

struct jsheap {
    JSRuntime *rt;
    int timeout;
    uint64_t deadline_ms;
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

struct qjs_event_listener_ctx {
    struct qjs_event_listener_ctx *next;
    struct jsthread *thread;
    JSValue func;
    struct dom_event_target *target;
    struct dom_string *type;
    struct dom_event_listener *listener;
    bool is_dom_node;
};

struct qjs_event_map {
    struct qjs_event_map *next;
    struct dom_event *evt;
    JSValue js_evt;
};

struct jsthread {
    JSContext *ctx;
    struct jsheap *heap;
    void *win_priv;
    void *doc_priv;
    QJSNodePrivate global_window_priv;
    void *worker_handle; /* WispWorkerHandle* if this is a worker thread */
    bool is_worker;
    bool closed;
    struct qjs_event_listener_ctx *listeners;
    struct qjs_event_map *events;
    struct qjs_timer *timers;

    struct WispMutationObserver *mutation_observers;
    struct WispIntersectionObserver *intersection_observers;
    void *mutation_callback_registered_doc;
};

static inline QJSNodePrivate *qjs_get_dom_priv(JSContext *ctx, JSValueConst val) {
    if (JS_VALUE_GET_TAG(val) != JS_TAG_OBJECT) return NULL;

    JSClassID class_id;
    void *opaque = JS_GetAnyOpaque(val, &class_id);
    if (opaque) {
        QJSNodePrivate *priv = (QJSNodePrivate *)opaque;
        if (priv->magic == QJS_DOM_MAGIC) return priv;
    }

    /* Fallback for global object (Window) */
    JSValue global_obj = JS_GetGlobalObject(ctx);
    bool is_global = JS_IsSameValue(ctx, val, global_obj);
    JS_FreeValue(ctx, global_obj);
    if (is_global) {
        struct jsthread *t = (struct jsthread *)JS_GetContextOpaque(ctx);
        if (t) return &t->global_window_priv;
    }

    return NULL;
}

void qjs_finalise_dom_bridge(JSContext *ctx);
void *qjs_get_window_priv(JSContext *ctx);
void *qjs_get_document_priv(JSContext *ctx);

/* From generated code */
void wisp_js_register_all_bindings(JSContext *ctx);

#endif /* WISP_QUICKJS_INTERNAL_H */
