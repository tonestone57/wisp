#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include "JSStorage.gen.h"

JSValue wisp_storage_getItem_impl(JSContext *ctx, QJSNodePrivate *priv, const char * key) { return JS_NULL; }
JSValue wisp_storage_setItem_impl(JSContext *ctx, QJSNodePrivate *priv, const char * key, const char * value) { return JS_UNDEFINED; }
JSValue wisp_storage_removeItem_impl(JSContext *ctx, QJSNodePrivate *priv, const char * key) { return JS_UNDEFINED; }
JSValue wisp_storage_clear_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_storage_key_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index) { return JS_NULL; }
JSValue wisp_storage_length_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewInt32(ctx, 0); }

int qjs_init_storage(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_storage_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check); JS_FreeValue(ctx, global_obj); return 0;
    }
    JS_FreeValue(ctx, check);

    qjs_init_storage_gen(ctx);
    JSValue localStorage = qjs_new_storage(ctx, NULL, false);
    JS_DefinePropertyValueStr(ctx, global_obj, "localStorage", localStorage, JS_PROP_C_W_E);

    JSValue sessionStorage = qjs_new_storage(ctx, NULL, false);
    JS_DefinePropertyValueStr(ctx, global_obj, "sessionStorage", sessionStorage, JS_PROP_C_W_E);

    JS_SetPropertyStr(ctx, global_obj, "__wisp_storage_init", JS_TRUE);
    JS_FreeValue(ctx, global_obj);
    return 0;
}
