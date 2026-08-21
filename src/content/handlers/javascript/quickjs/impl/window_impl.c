#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include "JSWindow.gen.h"
#include "qjs_internal.h"
#include "JSHistory.gen.h"
#include "base64_helper.h"

/* Custom Window init */
int qjs_init_window(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);

    /* Check if already initialized on this global object */
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_window_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    /* Initialize the class and prototype using the generated function */
    qjs_init_window_gen(ctx);

    JSValue proto = JS_GetClassProto(ctx, qjs_window_class_id);
    if (!JS_IsObject(proto)) {
        JS_FreeValue(ctx, proto);
        proto = JS_NewObject(ctx);
        JS_SetClassProto(ctx, qjs_window_class_id, JS_DupValue(ctx, proto));
    }

    /* Link Window to EventTarget */
    JSValue et_proto = JS_GetClassProto(ctx, qjs_eventtarget_class_id);
    if (JS_IsObject(proto) && JS_IsObject(et_proto)) {
        JS_SetPrototype(ctx, proto, et_proto);
    }
    JS_FreeValue(ctx, et_proto);
    JS_FreeValue(ctx, proto);

    /* Mark as initialized */
    if (JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_window_init", JS_TRUE, 0) < 0) {
        NSLOG(wisp, ERROR, "Failed to define __wisp_window_init property");
        JS_FreeValue(ctx, global_obj);
        return -1;
    }
    JS_FreeValue(ctx, global_obj);

    return 0;
}

JSValue wisp_window_window_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_GetGlobalObject(ctx);
}

JSValue wisp_window_self_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_GetGlobalObject(ctx);
}

JSValue wisp_window_history_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue hist = JS_GetPropertyStr(ctx, global, "__wisp_history_cached");
    if (JS_IsUndefined(hist)) {
        hist = qjs_new_history(ctx, NULL, false);
        JS_SetPropertyStr(ctx, global, "__wisp_history_cached", JS_DupValue(ctx, hist));
    }
    JS_FreeValue(ctx, global);
    return hist;
}

JSValue wisp_window_document_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (t) {
        struct dom_document *doc_node = qjs_thread_get_document(t);
        if (doc_node) {
            return qjs_wrap_node(ctx, (dom_node *)doc_node);
        }
    }
    return JS_NULL;
}

JSValue wisp_window_navigator_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue nav = JS_GetPropertyStr(ctx, global, "navigator");
    JS_FreeValue(ctx, global);
    return nav;
}

JSValue wisp_window_location_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue loc = JS_GetPropertyStr(ctx, global, "__wisp_location_cached");
    JS_FreeValue(ctx, global);
    return loc;
}

JSValue wisp_window_localStorage_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue store = JS_GetPropertyStr(ctx, global, "__wisp_localStorage");
    JS_FreeValue(ctx, global);
    return store;
}

JSValue wisp_window_sessionStorage_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue store = JS_GetPropertyStr(ctx, global, "__wisp_sessionStorage");
    JS_FreeValue(ctx, global);
    return store;
}

JSValue wisp_window_console_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue console = JS_GetPropertyStr(ctx, global, "console");
    JS_FreeValue(ctx, global);
    return console;
}

JSValue wisp_window_alert_impl(JSContext *ctx, QJSNodePrivate *priv, const char * message)
{
    NSLOG(wisp, INFO, "Window.alert: %s", message ? message : "");
    return JS_UNDEFINED;
}

JSValue wisp_window_applicationCache_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NULL;
}

JSValue wisp_window_atob_impl(JSContext *ctx, QJSNodePrivate *priv, const char * atob)
{
    return common_atob(ctx, atob);
}

JSValue wisp_window_btoa_impl(JSContext *ctx, QJSNodePrivate *priv, const char * btoa)
{
    return common_btoa(ctx, btoa);
}

JSValue wisp_windowbase64_atob_impl(JSContext *ctx, QJSNodePrivate *priv, const char * atob)
{
    return common_atob(ctx, atob);
}

JSValue wisp_windowbase64_btoa_impl(JSContext *ctx, QJSNodePrivate *priv, const char * btoa)
{
    return common_btoa(ctx, btoa);
}
