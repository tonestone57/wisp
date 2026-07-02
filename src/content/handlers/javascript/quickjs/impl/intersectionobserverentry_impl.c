#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSIntersectionObserverEntry.gen.h"
#include "observer_internal.h"

static void intersectionobserverentry_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_intersectionobserverentry_class_id);
    if (priv) {
        WispIntersectionObserverEntry *entry = priv->node;
        if (entry) { if (entry->target) dom_node_unref(entry->target); free(entry); }
        free(priv);
    }
}

static JSClassDef wisp_intersectionobserverentry_class = { "IntersectionObserverEntry", .finalizer = intersectionobserverentry_finalizer };

JSValue qjs_new_intersectionobserverentry_manual(JSContext *ctx, WispIntersectionObserverEntry *entry_data)
{
    WispIntersectionObserverEntry *entry = malloc(sizeof(WispIntersectionObserverEntry));
    if (!entry) return JS_ThrowOutOfMemory(ctx);
    memcpy(entry, entry_data, sizeof(WispIntersectionObserverEntry));
    JSValue obj = JS_NewObjectClass(ctx, qjs_intersectionobserverentry_class_id);
    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));
    if (!priv) { free(entry); return JS_ThrowOutOfMemory(ctx); }
    priv->magic = QJS_DOM_MAGIC; priv->node = entry; priv->is_dom_node = false; priv->ctx = ctx;
    JS_SetOpaque(obj, priv); return obj;
}

JSValue wisp_intersectionobserverentry_time_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewFloat64(ctx, ((WispIntersectionObserverEntry*)priv->node)->time); }
JSValue wisp_intersectionobserverentry_rootBounds_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    WispIntersectionObserverEntry *e = priv->node; JSValue obj = JS_NewObject(ctx);
    JS_DefinePropertyValueStr(ctx, obj, "x", JS_NewFloat64(ctx, 0), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, obj, "y", JS_NewFloat64(ctx, 0), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, obj, "width", JS_NewFloat64(ctx, e->rootWidth), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, obj, "height", JS_NewFloat64(ctx, e->rootHeight), JS_PROP_C_W_E);
    return obj;
}
JSValue wisp_intersectionobserverentry_boundingClientRect_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    WispIntersectionObserverEntry *e = priv->node; JSValue obj = JS_NewObject(ctx);
    JS_DefinePropertyValueStr(ctx, obj, "x", JS_NewFloat64(ctx, e->targetX), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, obj, "y", JS_NewFloat64(ctx, e->targetY), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, obj, "width", JS_NewFloat64(ctx, e->targetWidth), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, obj, "height", JS_NewFloat64(ctx, e->targetHeight), JS_PROP_C_W_E);
    return obj;
}
JSValue wisp_intersectionobserverentry_intersectionRect_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    WispIntersectionObserverEntry *e = priv->node; JSValue obj = JS_NewObject(ctx);
    JS_DefinePropertyValueStr(ctx, obj, "x", JS_NewFloat64(ctx, e->intersectX), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, obj, "y", JS_NewFloat64(ctx, e->intersectY), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, obj, "width", JS_NewFloat64(ctx, e->intersectWidth), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, obj, "height", JS_NewFloat64(ctx, e->intersectHeight), JS_PROP_C_W_E);
    return obj;
}
JSValue wisp_intersectionobserverentry_isIntersecting_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewBool(ctx, ((WispIntersectionObserverEntry*)priv->node)->isIntersecting); }
JSValue wisp_intersectionobserverentry_intersectionRatio_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewFloat64(ctx, ((WispIntersectionObserverEntry*)priv->node)->intersectionRatio); }
JSValue wisp_intersectionobserverentry_target_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return qjs_wrap_node(ctx, ((WispIntersectionObserverEntry*)priv->node)->target); }

int qjs_init_intersectionobserverentry(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_intersectionobserverentry_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_intersectionobserverentry_class_id == 0) JS_NewClassID(rt, &qjs_intersectionobserverentry_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_intersectionobserverentry_class_id)) JS_NewClass(rt, qjs_intersectionobserverentry_class_id, &wisp_intersectionobserverentry_class);

    qjs_init_intersectionobserverentry_gen(ctx);

    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_intersectionobserverentry_init", JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);
    return 0;
}
