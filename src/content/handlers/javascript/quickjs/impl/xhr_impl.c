#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSXMLHttpRequest.gen.h"

JSClassID qjs_xmlhttprequest_class_id;

static JSValue js_xhr_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv)
{
    return qjs_new_xmlhttprequest(ctx, NULL, false);
}

JSValue wisp_xmlhttprequest_responseType_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, ""); }
JSValue wisp_xmlhttprequest_responseType_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    NSLOG(wisp, INFO, "XMLHttpRequest.responseType set to: %s", value);
    return JS_UNDEFINED;
}
JSValue wisp_xmlhttprequest_status_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewInt32(ctx, 0); }
JSValue wisp_xmlhttprequest_statusText_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, ""); }
JSValue wisp_xmlhttprequest_responseText_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, ""); }
JSValue wisp_xmlhttprequest_responseXML_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NULL; }
JSValue wisp_xmlhttprequest_response_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NULL; }

JSValue wisp_xmlhttprequest_open_impl(JSContext *ctx, QJSNodePrivate *priv, const char * method, const char * url)
{
    NSLOG(wisp, INFO, "XMLHttpRequest.open called: %s %s", method, url);
    return JS_UNDEFINED;
}

JSValue wisp_xmlhttprequest_send_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_xmlhttprequest_abort_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_xmlhttprequest_setRequestHeader_impl(JSContext *ctx, QJSNodePrivate *priv, const char * header, const char * value) { return JS_UNDEFINED; }
JSValue wisp_xmlhttprequest_getResponseHeader_impl(JSContext *ctx, QJSNodePrivate *priv, const char * header) { return JS_NULL; }
JSValue wisp_xmlhttprequest_getAllResponseHeaders_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, ""); }
JSValue wisp_xmlhttprequest_overrideMimeType_impl(JSContext *ctx, QJSNodePrivate *priv, const char * mime) { return JS_UNDEFINED; }

JSValue wisp_xmlhttprequest_readyState_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewInt32(ctx, 0); }
JSValue wisp_xmlhttprequest_timeout_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewInt32(ctx, 0); }
JSValue wisp_xmlhttprequest_timeout_set_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t value) { return JS_UNDEFINED; }
JSValue wisp_xmlhttprequest_withCredentials_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_FALSE; }
JSValue wisp_xmlhttprequest_withCredentials_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) { return JS_UNDEFINED; }
JSValue wisp_xmlhttprequest_upload_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NULL; }
JSValue wisp_xmlhttprequest_responseURL_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, ""); }

int qjs_init_xhr(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);

    /* Check if already initialized on this global object */
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_xhr_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_xmlhttprequest_class_id == 0) JS_NewClassID(rt, &qjs_xmlhttprequest_class_id);

    /* Initialize the class and prototype using the generated function */
    qjs_init_xmlhttprequest_gen(ctx);

    JSValue proto = JS_GetClassProto(ctx, qjs_xmlhttprequest_class_id);
    if (!JS_IsObject(proto)) {
        JS_FreeValue(ctx, proto);
        proto = JS_NewObject(ctx);
        JS_SetClassProto(ctx, qjs_xmlhttprequest_class_id, JS_DupValue(ctx, proto));
    }
    JSValue ctor = JS_NewCFunction2(ctx, js_xhr_constructor, "XMLHttpRequest", 0, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, ctor, proto);
    JS_SetPropertyStr(ctx, global_obj, "XMLHttpRequest", ctor);
    JS_FreeValue(ctx, proto);

    /* Mark as initialized */
    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_xhr_init", JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);

    return 0;
}
