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

JSValue wisp_navigator_appCodeName_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, "Mozilla"); }
JSValue wisp_navigator_appName_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, "Netscape"); }
JSValue wisp_navigator_appVersion_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, "5.0 (Windows)"); }
JSValue wisp_navigator_platform_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, "Win32"); }
JSValue wisp_navigator_product_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, "Gecko"); }

JSValue wisp_navigator_language_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewString(ctx, "en-US");
}

int qjs_init_navigator(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "navigator");
    if (JS_IsObject(check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    qjs_init_navigator_gen(ctx);
    JSValue navigator = qjs_new_navigator(ctx, NULL, false);
    JS_DefinePropertyValueStr(ctx, global_obj, "navigator", navigator, JS_PROP_C_W_E);
    JS_FreeValue(ctx, global_obj);
    return 0;
}
