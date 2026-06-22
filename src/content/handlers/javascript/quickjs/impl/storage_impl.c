#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include "JSStorage.gen.h"

JSValue wisp_storage_length_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewInt32(ctx, 0);
}

JSValue wisp_storage_key_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index)
{
    return JS_NULL;
}

JSValue wisp_storage_getItem_impl(JSContext *ctx, QJSNodePrivate *priv, const char *key)
{
    return JS_NULL;
}

JSValue wisp_storage_setItem_impl(JSContext *ctx, QJSNodePrivate *priv, const char *key, const char *value)
{
    return JS_UNDEFINED;
}

JSValue wisp_storage_removeItem_impl(JSContext *ctx, QJSNodePrivate *priv, const char *key)
{
    return JS_UNDEFINED;
}

JSValue wisp_storage_clear_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_UNDEFINED;
}

int qjs_init_storage(JSContext *ctx)
{
    qjs_init_storage_gen(ctx);
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global_obj, "localStorage", qjs_new_storage(ctx, NULL, false));
    JS_SetPropertyStr(ctx, global_obj, "sessionStorage", qjs_new_storage(ctx, NULL, false));
    JS_FreeValue(ctx, global_obj);
    return 0;
}
