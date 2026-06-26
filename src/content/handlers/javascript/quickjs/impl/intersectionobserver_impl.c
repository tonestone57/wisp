#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSIntersectionObserver.gen.h"
#include "JSIntersectionObserverEntry.gen.h"
#include "wisp/content/handlers/html/box.h"
#include "wisp/content/handlers/html/box_inspect.h"
#include "wisp/content/handlers/html/private.h"

typedef struct {
    struct dom_node *target;
    bool is_intersecting;
    double ratio;
} WispObservedTarget;

typedef struct WispIntersectionObserver {
    JSValue callback;
    JSValue observer_val;
    struct jsthread *thread;
    WispObservedTarget *targets;
    uint32_t target_count;
    JSValue records;
    bool microtask_scheduled;
    struct WispIntersectionObserver *next;
} WispIntersectionObserver;

static void intersectionobserver_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_intersectionobserver_class_id);
    if (priv) {
        WispIntersectionObserver *observer = priv->node;
        if (observer) {
            /* Remove from thread list */
            WispIntersectionObserver **curr = &observer->thread->intersection_observers;
            while (*curr) {
                if (*curr == observer) {
                    *curr = observer->next;
                    break;
                }
                curr = &(*curr)->next;
            }

            for (uint32_t i = 0; i < observer->target_count; i++) {
                dom_node_unref(observer->targets[i].target);
            }
            free(observer->targets);
            JS_FreeValueRT(rt, observer->callback);
            JS_FreeValueRT(rt, observer->records);
            free(observer);
        }
        free(priv);
    }
}

static void intersectionobserver_gc_mark(JSRuntime *rt, JSValueConst val, JS_MarkFunc *mark_func)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_intersectionobserver_class_id);
    if (priv) {
        WispIntersectionObserver *observer = priv->node;
        if (observer) {
            JS_MarkValue(rt, observer->callback, mark_func);
            JS_MarkValue(rt, observer->records, mark_func);
            JS_MarkValue(rt, observer->observer_val, mark_func);
        }
    }
}

static JSClassDef wisp_intersectionobserver_class = {
    "IntersectionObserver",
    .finalizer = intersectionobserver_finalizer,
    .gc_mark = intersectionobserver_gc_mark,
};

static JSValue intersection_observer_microtask(JSContext *ctx, int argc, JSValueConst *argv)
{
    WispIntersectionObserver *observer = JS_GetOpaque(argv[0], qjs_intersectionobserver_class_id);
    if (!observer || observer->thread->closed) return JS_UNDEFINED;

    observer->microtask_scheduled = false;

    JSValue records = observer->records;
    observer->records = JS_NewArray(ctx);

    JSValueConst args[2];
    args[0] = records;
    args[1] = argv[0];

    JSValue ret = JS_Call(ctx, observer->callback, JS_UNDEFINED, 2, args);
    JS_FreeValue(ctx, ret);
    JS_FreeValue(ctx, records);

    return JS_UNDEFINED;
}

JSValue wisp_intersectionobserver_observe_impl(JSContext *ctx, QJSNodePrivate *priv, void * target)
{
    if (!priv || !priv->node || !target) return JS_EXCEPTION;
    WispIntersectionObserver *observer = priv->node;
    struct dom_node *node = target;

    for (uint32_t i = 0; i < observer->target_count; i++) {
        if (observer->targets[i].target == node) return JS_UNDEFINED;
    }

    observer->targets = realloc(observer->targets, (observer->target_count + 1) * sizeof(WispObservedTarget));
    observer->targets[observer->target_count].target = dom_node_ref(node);
    observer->targets[observer->target_count].is_intersecting = false;
    observer->targets[observer->target_count].ratio = 0.0;
    observer->target_count++;

    return JS_UNDEFINED;
}

JSValue wisp_intersectionobserver_unobserve_impl(JSContext *ctx, QJSNodePrivate *priv, void * target)
{
    if (!priv || !priv->node || !target) return JS_EXCEPTION;
    WispIntersectionObserver *observer = priv->node;
    struct dom_node *node = target;

    for (uint32_t i = 0; i < observer->target_count; i++) {
        if (observer->targets[i].target == node) {
            dom_node_unref(observer->targets[i].target);
            memmove(&observer->targets[i], &observer->targets[i+1], (observer->target_count - i - 1) * sizeof(WispObservedTarget));
            observer->target_count--;
            break;
        }
    }
    return JS_UNDEFINED;
}

JSValue wisp_intersectionobserver_disconnect_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_EXCEPTION;
    WispIntersectionObserver *observer = priv->node;
    for (uint32_t i = 0; i < observer->target_count; i++) {
        dom_node_unref(observer->targets[i].target);
    }
    free(observer->targets);
    observer->targets = NULL;
    observer->target_count = 0;
    return JS_UNDEFINED;
}

JSValue wisp_intersectionobserver_takeRecords_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_EXCEPTION;
    WispIntersectionObserver *observer = priv->node;
    JSValue records = observer->records;
    observer->records = JS_NewArray(observer->thread->ctx);
    return records;
}

void wisp_handle_intersection_check(struct jsthread *thread, struct box *layout, int viewport_width, int viewport_height)
{
    if (!thread || thread->closed) return;
    JSContext *ctx = thread->ctx;

    for (WispIntersectionObserver *observer = thread->intersection_observers; observer; observer = observer->next) {
        bool changed = false;
        for (uint32_t i = 0; i < observer->target_count; i++) {
            WispObservedTarget *target = &observer->targets[i];
            struct box *target_box = box_find_by_node(layout, target->target);
            bool is_intersecting = false;
            double ratio = 0.0;

            if (target_box) {
                int x, y;
                box_coords(target_box, &x, &y);
                int x0 = x, y0 = y;
                int x1 = x + target_box->width;
                int y1 = y + target_box->height;

                int ix0 = (x0 > 0) ? x0 : 0;
                int iy0 = (y0 > 0) ? y0 : 0;
                int ix1 = (x1 < viewport_width) ? x1 : viewport_width;
                int iy1 = (y1 < viewport_height) ? y1 : viewport_height;

                if (ix1 > ix0 && iy1 > iy0) {
                    is_intersecting = true;
                    double intersect_area = (double)(ix1 - ix0) * (iy1 - iy0);
                    double target_area = (double)(x1 - x0) * (y1 - y0);
                    if (target_area > 0) ratio = intersect_area / target_area;
                    else ratio = 1.0;
                }
            }

            if (is_intersecting != target->is_intersecting || fabs(ratio - target->ratio) > 0.001) {
                target->is_intersecting = is_intersecting;
                target->ratio = ratio;
                changed = true;

                /* Create IntersectionObserverEntry */
                JSValue entry = JS_NewObjectClass(ctx, qjs_intersectionobserverentry_class_id);
                JS_SetPropertyStr(ctx, entry, "time", JS_NewFloat64(ctx, 0.0)); /* TODO: timestamp */
                JS_SetPropertyStr(ctx, entry, "rootBounds", JS_NULL);
                JS_SetPropertyStr(ctx, entry, "boundingClientRect", JS_NULL);
                JS_SetPropertyStr(ctx, entry, "intersectionRect", JS_NULL);
                JS_SetPropertyStr(ctx, entry, "isIntersecting", JS_NewBool(ctx, is_intersecting));
                JS_SetPropertyStr(ctx, entry, "intersectionRatio", JS_NewFloat64(ctx, ratio));
                JS_SetPropertyStr(ctx, entry, "target", qjs_wrap_node(ctx, target->target));

                uint32_t len;
                JSValue len_val = JS_GetPropertyStr(ctx, observer->records, "length");
                JS_ToUint32(ctx, &len, len_val);
                JS_FreeValue(ctx, len_val);
                JS_SetPropertyUint32(ctx, observer->records, len, entry);
            }
        }

        if (changed && !observer->microtask_scheduled) {
            observer->microtask_scheduled = true;
            JS_EnqueueJob(ctx, intersection_observer_microtask, 1, &observer->observer_val);
        }
    }
}

JSValue wisp_intersectionobserver_root_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NULL; }
JSValue wisp_intersectionobserver_rootMargin_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, "0px"); }
JSValue wisp_intersectionobserver_thresholds_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewArray(ctx); }

static JSValue js_intersectionobserver_constructor(JSContext *ctx, JSValueConst new_target,
                                                 int argc, JSValueConst *argv)
{
    if (argc < 1 || !JS_IsFunction(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "IntersectionObserver constructor requires a callback function");
    }

    WispIntersectionObserver *observer = calloc(1, sizeof(WispIntersectionObserver));
    if (!observer) return JS_ThrowOutOfMemory(ctx);

    observer->callback = JS_DupValue(ctx, argv[0]);
    observer->thread = JS_GetContextOpaque(ctx);
    observer->records = JS_NewArray(ctx);

    JSValue obj = JS_NewObjectClass(ctx, qjs_intersectionobserver_class_id);
    if (JS_IsException(obj)) {
        JS_FreeValue(ctx, observer->callback);
        JS_FreeValue(ctx, observer->records);
        free(observer);
        return obj;
    }
    observer->observer_val = JS_DupValue(ctx, obj);

    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));
    if (!priv) {
        JS_FreeValue(ctx, obj);
        JS_FreeValue(ctx, observer->callback);
        JS_FreeValue(ctx, observer->records);
        free(observer);
        return JS_ThrowOutOfMemory(ctx);
    }

    priv->magic = QJS_DOM_MAGIC;
    priv->node = observer;
    priv->is_dom_node = false;
    priv->ctx = ctx;

    JS_SetOpaque(obj, priv);

    /* Add to thread list */
    observer->next = observer->thread->intersection_observers;
    observer->thread->intersection_observers = observer;

    return obj;
}

int qjs_init_intersectionobserver(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_intersectionobserver_class_id == 0) JS_NewClassID(rt, &qjs_intersectionobserver_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_intersectionobserver_class_id)) {
        JS_NewClass(rt, qjs_intersectionobserver_class_id, &wisp_intersectionobserver_class);
    }

    qjs_init_intersectionobserver_gen(ctx);

    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue proto = JS_GetClassProto(ctx, qjs_intersectionobserver_class_id);
    JSValue ctor = JS_NewCFunction2(ctx, js_intersectionobserver_constructor, "IntersectionObserver", 1, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, ctor, proto);
    JS_SetPropertyStr(ctx, global_obj, "IntersectionObserver", ctor);

    JS_FreeValue(ctx, proto);
    JS_FreeValue(ctx, global_obj);
    return 0;
}
