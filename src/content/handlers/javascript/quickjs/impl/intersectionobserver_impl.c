#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSIntersectionObserver.gen.h"

typedef struct {
    JSValue callback;
} WispIntersectionObserver;

static void intersectionobserver_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_intersectionobserver_class_id);
    if (priv) {
        WispIntersectionObserver *observer = priv->node;
        if (observer) {
            JS_FreeValueRT(rt, observer->callback);
            free(observer);
        }
        free(priv);
    }
}

static JSClassDef wisp_intersectionobserver_class = {
    "IntersectionObserver",
    .finalizer = intersectionobserver_finalizer,
};

JSValue wisp_intersectionobserver_observe_impl(JSContext *ctx, QJSNodePrivate *priv, void * target)
{
    if (!priv || !priv->node || !target) return JS_EXCEPTION;
    NSLOG(wisp, INFO, "IntersectionObserver.observe() called (stub)");
    return JS_UNDEFINED;
}

JSValue wisp_intersectionobserver_unobserve_impl(JSContext *ctx, QJSNodePrivate *priv, void * target)
{
    if (!priv || !priv->node || !target) return JS_EXCEPTION;
    NSLOG(wisp, INFO, "IntersectionObserver.unobserve() called");
    return JS_UNDEFINED;
}

JSValue wisp_intersectionobserver_disconnect_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_EXCEPTION;
    NSLOG(wisp, INFO, "IntersectionObserver.disconnect() called");
    return JS_UNDEFINED;
}

JSValue wisp_intersectionobserver_takeRecords_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_EXCEPTION;
    return JS_NewArray(ctx);
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

    JSValue obj = JS_NewObjectClass(ctx, qjs_intersectionobserver_class_id);
    if (JS_IsException(obj)) {
        JS_FreeValue(ctx, observer->callback);
        free(observer);
        return obj;
    }

    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));
    if (!priv) {
        JS_FreeValue(ctx, obj);
        JS_FreeValue(ctx, observer->callback);
        free(observer);
        return JS_ThrowOutOfMemory(ctx);
    }

    priv->magic = QJS_DOM_MAGIC;
    priv->node = observer;
    priv->is_dom_node = false;
    priv->ctx = ctx;

    JS_SetOpaque(obj, priv);
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
