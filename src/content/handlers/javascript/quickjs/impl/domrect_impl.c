#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSDOMRect.gen.h"
#include "JSDOMRectReadOnly.gen.h"

typedef struct {
    double x, y, width, height;
} WispRect;

static void domrect_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_domrect_class_id);
    if (priv) {
        if (priv->node) free(priv->node);
        free(priv);
    }
}

static JSClassDef wisp_domrect_class = {
    "DOMRect",
    .finalizer = domrect_finalizer,
};

static void domrectreadonly_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_domrectreadonly_class_id);
    if (priv) {
        if (priv->node) free(priv->node);
        free(priv);
    }
}

static JSClassDef wisp_domrectreadonly_class = {
    "DOMRectReadOnly",
    .finalizer = domrectreadonly_finalizer,
};

JSValue wisp_domrectreadonly_x_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return priv ? JS_NewFloat64(ctx, ((WispRect*)priv->node)->x) : JS_UNDEFINED; }
JSValue wisp_domrectreadonly_y_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return priv ? JS_NewFloat64(ctx, ((WispRect*)priv->node)->y) : JS_UNDEFINED; }
JSValue wisp_domrectreadonly_width_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return priv ? JS_NewFloat64(ctx, ((WispRect*)priv->node)->width) : JS_UNDEFINED; }
JSValue wisp_domrectreadonly_height_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return priv ? JS_NewFloat64(ctx, ((WispRect*)priv->node)->height) : JS_UNDEFINED; }
JSValue wisp_domrectreadonly_top_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv) return JS_UNDEFINED;
    WispRect *r = (WispRect*)priv->node;
    return JS_NewFloat64(ctx, fmin(r->y, r->y + r->height));
}
JSValue wisp_domrectreadonly_left_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv) return JS_UNDEFINED;
    WispRect *r = (WispRect*)priv->node;
    return JS_NewFloat64(ctx, fmin(r->x, r->x + r->width));
}
JSValue wisp_domrectreadonly_right_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv) return JS_UNDEFINED;
    WispRect *r = (WispRect*)priv->node;
    return JS_NewFloat64(ctx, fmax(r->x, r->x + r->width));
}
JSValue wisp_domrectreadonly_bottom_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv) return JS_UNDEFINED;
    WispRect *r = (WispRect*)priv->node;
    return JS_NewFloat64(ctx, fmax(r->y, r->y + r->height));
}

JSValue wisp_domrect_x_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return wisp_domrectreadonly_x_get_impl(ctx, priv); }
JSValue wisp_domrect_y_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return wisp_domrectreadonly_y_get_impl(ctx, priv); }
JSValue wisp_domrect_width_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return wisp_domrectreadonly_width_get_impl(ctx, priv); }
JSValue wisp_domrect_height_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return wisp_domrectreadonly_height_get_impl(ctx, priv); }

JSValue wisp_domrect_x_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) { if (priv) ((WispRect*)priv->node)->x = value; return JS_UNDEFINED; }
JSValue wisp_domrect_y_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) { if (priv) ((WispRect*)priv->node)->y = value; return JS_UNDEFINED; }
JSValue wisp_domrect_width_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) { if (priv) ((WispRect*)priv->node)->width = value; return JS_UNDEFINED; }
JSValue wisp_domrect_height_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) { if (priv) ((WispRect*)priv->node)->height = value; return JS_UNDEFINED; }

int qjs_init_domrect(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_domrect_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_domrect_class_id == 0) JS_NewClassID(rt, &qjs_domrect_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_domrect_class_id)) {
        JS_NewClass(rt, qjs_domrect_class_id, &wisp_domrect_class);
    }
    qjs_init_domrect_gen(ctx);

    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_domrect_init", JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);
    return 0;
}

int qjs_init_domrectreadonly(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_domrectreadonly_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_domrectreadonly_class_id == 0) JS_NewClassID(rt, &qjs_domrectreadonly_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_domrectreadonly_class_id)) {
        JS_NewClass(rt, qjs_domrectreadonly_class_id, &wisp_domrectreadonly_class);
    }
    qjs_init_domrectreadonly_gen(ctx);

    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_domrectreadonly_init", JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);
    return 0;
}
