#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include "JSXMLHttpRequest.gen.h"

JSValue wisp_xmlhttprequest_readyState_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewInt32(ctx, 0);
}

JSValue wisp_xmlhttprequest_responseType_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, ""); }
JSValue wisp_xmlhttprequest_responseType_set_impl(JSContext *ctx, QJSNodePrivate *priv, void * value) { return JS_UNDEFINED; }
JSValue wisp_xmlhttprequest_response_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NULL; }
JSValue wisp_xmlhttprequest_responseText_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, ""); }
JSValue wisp_xmlhttprequest_responseXML_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NULL; }
JSValue wisp_xmlhttprequest_status_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewInt32(ctx, 0); }
JSValue wisp_xmlhttprequest_statusText_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, ""); }

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

static JSValue js_xmlhttprequest_constructor(JSContext *ctx, JSValueConst new_target,
                                           int argc, JSValueConst *argv)
{
    return qjs_new_xmlhttprequest(ctx, NULL, false);
}

int qjs_init_xhr(JSContext *ctx)
{
    qjs_init_xmlhttprequest_gen(ctx);
    JSValue global_obj = JS_GetGlobalObject(ctx);

    JSValue proto = JS_GetClassProto(ctx, qjs_xmlhttprequest_class_id);
    JSValue ctor = JS_NewCFunction2(ctx, js_xmlhttprequest_constructor, "XMLHttpRequest", 0, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, ctor, proto);
    JS_SetPropertyStr(ctx, global_obj, "XMLHttpRequest", ctor);

    JS_FreeValue(ctx, proto);
    JS_FreeValue(ctx, global_obj);
    return 0;
}
