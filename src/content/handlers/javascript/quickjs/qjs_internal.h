#ifndef WISP_QUICKJS_INTERNAL_H
#define WISP_QUICKJS_INTERNAL_H

#include "quickjs.h"
#include <stdbool.h>
#include <stdint.h>
#include "utils/libdom.h"
#include "content/handlers/javascript/js.h"
#include "wisp/utils/shm_dom.h"

/* Forward declarations */
struct nsurl;
struct fetch;
struct fetch_multipart_data;
struct dom_document;

/* Private data for JS DOM objects */
typedef struct QJSNodePrivate {
    uint32_t magic;         /* Magic number for type safety */
    void *node;             /* Underlying LibDOM node/object */
    JSContext *ctx;         /* Associated context */
    bool is_dom_node;       /* True if node is dom_node* (needs unref) */
} QJSNodePrivate;

typedef struct ImageDataPrivate {
    uint32_t width;
    uint32_t height;
    JSValue data; /* Uint8ClampedArray */
} ImageDataPrivate;

#define QJS_DOM_MAGIC 0x57495350

typedef struct WispXHR {
    JSContext *ctx;
    JSValue self;
    int readyState;
    int status;
    char *statusText;
    char *method;
    struct nsurl *url;
    bool async;
    bool withCredentials;
    struct fetch *fetch_handle;
    uint8_t *response_buf;
    size_t response_len;
    size_t response_alloc;
    char *response_headers;
    struct fetch_multipart_data *out_headers;
    struct dom_document *response_xml;
    char *response_type;
    struct WispXHR *next;
} WispXHR;

struct jsheap {
    JSRuntime *rt;
    int timeout;
    uint64_t deadline_ms;
    uint64_t last_yield_ms;
    struct jsthread *threads; /* Head of linked list of active threads on this heap */
    struct jsheap *next_in_global; /* Thread-safe global linked list of active heaps */
};

struct qjs_timer {
    JSContext *ctx;
    JSValue func;
    JSValue arguments;
    bool repeat;
    int interval;
    int id;
    bool cancelled;
    uint64_t scheduled_time;
    struct qjs_timer *next;
};

struct qjs_raf_callback {
    JSContext *ctx;
    JSValue func;
    int id;
    bool cancelled;
    uint64_t scheduled_time;
    struct qjs_raf_callback *next;
};

struct qjs_idle_callback {
    JSContext *ctx;
    JSValue func;
    int id;
    bool cancelled;
    uint32_t timeout;
    uint64_t scheduled_time;
    struct qjs_idle_callback *next;
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
    struct jsthread *next_in_heap; /* Next thread in the same heap's linked list */
    void *win_priv;
    void *doc_priv;
    char *origin;
    struct nsurl *location_url;
    char *current_script_name;
    void *current_script_node;
    QJSNodePrivate global_window_priv;
    void *worker_handle; /* WispWorkerHandle* if this is a worker thread */
    bool is_worker;
    bool closed;
    struct qjs_event_listener_ctx *listeners;
    struct qjs_event_map *events;
    struct qjs_timer *timers;
    struct qjs_raf_callback *raf_callbacks;
    struct qjs_idle_callback *idle_callbacks;
    int next_raf_id;
    int next_idle_id;

    struct WispMutationObserver *mutation_observers;
    struct WispIntersectionObserver *intersection_observers;
    struct WispXHR *xmlhttprequests;
    void *mutation_callback_registered_doc;

    shm_dom_t *shm_dom;
    char shm_dom_name[64];
    bool shm_initialized;
    uint32_t shm_capacity;
    JSValue node_wrapper_cache[SHM_DOM_MAX_NODES];
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

char *wisp_module_normalize(JSContext *ctx, const char *base_name, const char *name, void *opaque);
JSModuleDef *wisp_module_loader(JSContext *ctx, const char *module_name, void *opaque);
JSValue js_eval_with_aot_cache(JSContext *ctx, const uint8_t *txt, size_t txtlen, const char *name, int eval_flags);
void qjs_finalise_dom_bridge(JSRuntime *rt, JSContext *ctx);
void qjs_cleanup_mutation_observer(struct jsthread *thread);
int qjs_init_canvas(JSContext *ctx);
int qjs_init_trusted_types(JSContext *ctx);
void *qjs_get_window_priv(JSContext *ctx);
void *qjs_get_document_priv(JSContext *ctx);
struct dom_document *qjs_thread_get_document(struct jsthread *t);
void qjs_raf_callback_fn(void *p);
void qjs_idle_callback_fn(void *p);
void serialize_dom_tree(shm_dom_t *shm, struct jsthread *thread, struct dom_document *doc);
void drain_mutation_queue(shm_dom_t *shm, struct dom_document *doc);

void qjs_inject_dom_polyfills(JSContext *ctx);
void wisp_qjs_register_core_polyfills(JSContext *ctx);
void check_script_element_execution(JSContext *ctx, void *node);

JSValue wisp_timer_create(JSContext *ctx, JSValue handler, int32_t timeout, JSValue arguments, bool repeat);
JSValue wisp_timer_clear(JSContext *ctx, int32_t handle);
uint64_t qjs_execute_timers(JSContext *ctx);
void force_synchronous_layout(struct jsthread *thread);

/* From generated code */
void wisp_js_register_all_bindings(JSContext *ctx);

#endif /* WISP_QUICKJS_INTERNAL_H */
