#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include "JSNavigator.gen.h"

JSValue wisp_navigator_cookieEnabled_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_TRUE;
}

JSValue wisp_navigator_userAgent_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewString(ctx, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36 Wisp/1.0");
}

JSValue wisp_navigator_appCodeName_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewString(ctx, "Mozilla");
}

JSValue wisp_navigator_appName_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewString(ctx, "Netscape");
}

JSValue wisp_navigator_appVersion_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewString(ctx, "5.0 (Windows)");
}

JSValue wisp_navigator_platform_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewString(ctx, "Win32");
}

JSValue wisp_navigator_product_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewString(ctx, "Gecko");
}

JSValue wisp_navigator_language_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewString(ctx, "en-US");
}

JSValue wisp_navigator_isContentHandlerRegistered_impl(JSContext *ctx, QJSNodePrivate *priv, const char * mimeType, const char * url)
{
    return JS_FALSE;
}

JSValue wisp_navigator_isProtocolHandlerRegistered_impl(JSContext *ctx, QJSNodePrivate *priv, const char * scheme, const char * url)
{
    return JS_FALSE;
}

JSValue wisp_navigator_registerContentHandler_impl(JSContext *ctx, QJSNodePrivate *priv, const char * mimeType, const char * url, const char * title)
{
    return JS_UNDEFINED;
}

JSValue wisp_navigator_registerProtocolHandler_impl(JSContext *ctx, QJSNodePrivate *priv, const char * scheme, const char * url, const char * title)
{
    return JS_UNDEFINED;
}

JSValue wisp_navigator_taintEnabled_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_FALSE;
}

JSValue wisp_navigator_unregisterContentHandler_impl(JSContext *ctx, QJSNodePrivate *priv, const char * mimeType, const char * url)
{
    return JS_UNDEFINED;
}

JSValue wisp_navigator_unregisterProtocolHandler_impl(JSContext *ctx, QJSNodePrivate *priv, const char * scheme, const char * url)
{
    return JS_UNDEFINED;
}

JSValue wisp_navigator_yieldForStorageUpdates_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_UNDEFINED;
}

JSValue wisp_navigator_javaEnabled_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_FALSE;
}

JSValue wisp_navigator_languages_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewArray(ctx);
}

JSValue wisp_navigator_mimeTypes_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewArray(ctx);
}

JSValue wisp_navigator_onLine_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_TRUE;
}

JSValue wisp_navigator_plugins_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewArray(ctx);
}

JSValue wisp_navigator_productSub_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewString(ctx, "20030107");
}

JSValue wisp_navigator_vendor_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewString(ctx, "Google Inc.");
}

JSValue wisp_navigator_vendorSub_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewString(ctx, "");
}

int qjs_init_navigator(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_navigator_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    qjs_init_navigator_gen(ctx);

    JSValue navigator = qjs_new_navigator(ctx, NULL, false);
    JS_DefinePropertyValueStr(ctx, global_obj, "navigator", navigator, JS_PROP_C_W_E);
    JS_SetPropertyStr(ctx, global_obj, "__wisp_navigator_init", JS_TRUE);
    JS_FreeValue(ctx, global_obj);
    return 0;
}
