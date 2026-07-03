#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include "JSLocation.gen.h"

JSValue wisp_location_href_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewString(ctx, "about:blank");
}

int qjs_init_location(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_location_init");
    bool already_init = JS_ToBool(ctx, check);
    JS_FreeValue(ctx, check);

    if (already_init) {
        JS_FreeValue(ctx, global_obj);
        return 0;
    }

    qjs_init_location_gen(ctx);
    JSValue loc = qjs_new_location(ctx, NULL, false);
    JS_DefinePropertyValueStr(ctx, global_obj, "location", loc, JS_PROP_C_W_E);
    JS_SetPropertyStr(ctx, global_obj, "__wisp_location_init", JS_TRUE);
    JS_FreeValue(ctx, global_obj);
    return 0;
}
