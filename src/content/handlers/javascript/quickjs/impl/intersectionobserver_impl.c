#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSIntersectionObserver.gen.h"
#include "observer_internal.h"

static void intersectionobserver_mark(JSRuntime *rt, JSValueConst val, JS_MarkFunc *mark_func)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_intersectionobserver_class_id);
    if (priv && priv->node) {
        WispIntersectionObserver *observer = priv->node;
        JS_MarkValue(rt, observer->callback, mark_func);
        JS_MarkValue(rt, observer->queue, mark_func);
        JS_MarkValue(rt, observer->self, mark_func);
    }
}

static void intersectionobserver_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_intersectionobserver_class_id);
    if (priv) {
        WispIntersectionObserver *observer = priv->node;
        if (observer) {
            struct jsthread *t = JS_GetContextOpaque(priv->ctx);
            if (t) {
                WispIntersectionObserver **curr = &t->intersection_observers;
                while (*curr) {
                    if (*curr == observer) {
                        *curr = observer->next;
                        break;
                    }
                    curr = &((*curr)->next);
                }
            }
            JS_FreeValueRT(rt, observer->callback);
            JS_FreeValueRT(rt, observer->queue);
            JS_FreeValueRT(rt, observer->self);
            IntersectionObserverTarget *ot = observer->targets;
            while (ot) {
                IntersectionObserverTarget *next = ot->next;
                dom_node_unref(ot->node); free(ot); ot = next;
            }
            free(observer);
        }
        free(priv);
    }
}

static JSClassDef wisp_intersectionobserver_class = { "IntersectionObserver", .finalizer = intersectionobserver_finalizer, .gc_mark = intersectionobserver_mark };

JSValue wisp_intersectionobserver_observe_impl(JSContext *ctx, QJSNodePrivate *priv, void * target)
{
    WispIntersectionObserver *observer = priv->node;
    IntersectionObserverTarget *ot = calloc(1, sizeof(IntersectionObserverTarget));
    ot->node = target; dom_node_ref(target);
    ot->next = observer->targets; observer->targets = ot;
    return JS_UNDEFINED;
}

JSValue wisp_intersectionobserver_unobserve_impl(JSContext *ctx, QJSNodePrivate *priv, void * target)
{
    WispIntersectionObserver *observer = priv->node;
    IntersectionObserverTarget **curr = &observer->targets;
    while (*curr) {
        if ((*curr)->node == target) {
            IntersectionObserverTarget *to_free = *curr;
            *curr = to_free->next; dom_node_unref(to_free->node); free(to_free);
            return JS_UNDEFINED;
        }
        curr = &((*curr)->next);
    }
    return JS_UNDEFINED;
}

JSValue wisp_intersectionobserver_disconnect_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    WispIntersectionObserver *observer = priv->node;
    IntersectionObserverTarget *ot = observer->targets;
    while (ot) {
        IntersectionObserverTarget *next = ot->next;
        dom_node_unref(ot->node); free(ot); ot = next;
    }
    observer->targets = NULL;
    return JS_UNDEFINED;
}

JSValue wisp_intersectionobserver_takeRecords_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    WispIntersectionObserver *observer = priv->node;
    JSValue queue = observer->queue; observer->queue = JS_NewArray(ctx);
    return queue;
}

static JSValue js_intersectionobserver_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv)
{
    if (argc < 1 || !JS_IsFunction(ctx, argv[0])) return JS_ThrowTypeError(ctx, "Callback required");
    WispIntersectionObserver *observer = calloc(1, sizeof(WispIntersectionObserver));
    if (!observer) return JS_ThrowOutOfMemory(ctx);
    observer->callback = JS_DupValue(ctx, argv[0]);
    observer->ctx = ctx; observer->queue = JS_NewArray(ctx);
    JSValue obj = JS_NewObjectClass(ctx, qjs_intersectionobserver_class_id);
    if (JS_IsException(obj)) { JS_FreeValue(ctx, observer->callback); JS_FreeValue(ctx, observer->queue); free(observer); return obj; }
    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));
    if (!priv) { JS_FreeValue(ctx, observer->callback); JS_FreeValue(ctx, observer->queue); free(observer); JS_FreeValue(ctx, obj); return JS_ThrowOutOfMemory(ctx); }
    priv->magic = QJS_DOM_MAGIC; priv->node = observer; priv->is_dom_node = false; priv->ctx = ctx;
    JS_SetOpaque(obj, priv); observer->self = JS_DupValue(ctx, obj);
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (t) { observer->next = t->intersection_observers; t->intersection_observers = observer; }
    return obj;
}

int qjs_init_intersectionobserver(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_intersectionobserver_class_id == 0) JS_NewClassID(rt, &qjs_intersectionobserver_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_intersectionobserver_class_id)) JS_NewClass(rt, qjs_intersectionobserver_class_id, &wisp_intersectionobserver_class);
    qjs_init_intersectionobserver_gen(ctx);
    JSValue ctor = JS_NewCFunction2(ctx, js_intersectionobserver_constructor, "IntersectionObserver", 1, JS_CFUNC_constructor, 0);
    JSValue global_obj = JS_GetGlobalObject(ctx); JS_DefinePropertyValueStr(ctx, global_obj, "IntersectionObserver", ctor, JS_PROP_C_W_E); JS_FreeValue(ctx, global_obj);
    return 0;
}
