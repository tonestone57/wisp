/* Implementation for Window */
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"

#include "window.inc"

static void js_window_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_window_class_id);
    if (priv) {
        free(priv);
    }
}

static JSValue js_window_captureEvents(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_releaseEvents(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_getComputedStyle(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.getComputedStyle() called (stub)");
    return JS_NULL;
}

static JSValue js_window_matchMedia(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.matchMedia() called (stub)");
    return JS_NULL;
}

static JSValue js_window_moveTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_moveBy(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_resizeTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_resizeBy(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_scroll(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_scrollTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_scrollBy(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_alert(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc > 0) {
        const char *str = JS_ToCString(ctx, argv[0]);
        if (str) {
            NSLOG(wisp, INFO, "Alert: %s", str);
            JS_FreeCString(ctx, str);
        }
    }
    return JS_UNDEFINED;
}

static JSValue js_window_confirm(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_TRUE;
}

static JSValue js_window_prompt(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_NULL;
}

static JSValue js_window_print(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_postMessage(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_atob(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_NULL;
}

static JSValue js_window_btoa(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_NULL;
}

static JSValue js_window_stop(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_open(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_NULL;
}

static JSValue js_window_close(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_requestAnimationFrame(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_NewInt32(ctx, 1);
}

static JSValue js_window_cancelAnimationFrame(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

int qjs_init_window(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_window_class_id == 0) JS_NewClassID(rt, &qjs_window_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_window_class_id)) JS_NewClass(rt, qjs_window_class_id, &js_window_class);

    JSValue global_obj = JS_GetGlobalObject(ctx);
    JS_SetPropertyFunctionList(ctx, global_obj, js_window_proto_funcs, sizeof(js_window_proto_funcs) / sizeof(js_window_proto_funcs[0]));

    /* Set self references */
    JS_SetPropertyStr(ctx, global_obj, "window", JS_DupValue(ctx, global_obj));
    JS_SetPropertyStr(ctx, global_obj, "self", JS_DupValue(ctx, global_obj));

    JS_FreeValue(ctx, global_obj);
    return 0;
}

JSValue qjs_new_window(JSContext *ctx, void *node, bool is_dom_node)
{
    JSValue obj = JS_GetGlobalObject(ctx);
    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));
    if (!priv) return JS_ThrowOutOfMemory(ctx);
    priv->node = node; priv->is_dom_node = is_dom_node;
    JS_SetOpaque(obj, priv); return obj;
}
