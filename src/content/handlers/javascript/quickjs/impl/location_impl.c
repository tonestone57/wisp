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

    /* Check if already initialized on this global object */
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_location_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    /* Initialize the class and prototype using the generated function */
    qjs_init_location_gen(ctx);
    JSValue loc = qjs_new_location(ctx, NULL, false);
    JS_DefinePropertyValueStr(ctx, global_obj, "location", loc, JS_PROP_C_W_E);

    /* Mark as initialized */
    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_location_init", JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);

    return 0;
}
