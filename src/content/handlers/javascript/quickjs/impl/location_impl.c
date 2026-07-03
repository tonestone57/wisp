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

static JSValue js_location_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv)
{
    return qjs_new_location(ctx, NULL, false);
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

    JSValue proto = JS_GetClassProto(ctx, qjs_location_class_id);
    if (!JS_IsObject(proto)) {
        proto = JS_NewObject(ctx);
        JS_SetClassProto(ctx, qjs_location_class_id, JS_DupValue(ctx, proto));
    }

    JSValue loc = qjs_new_location(ctx, NULL, false);
    JSValue ctor = JS_NewCFunction2(ctx, js_location_constructor, "Location", 0, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, ctor, proto);
    JS_FreeValue(ctx, ctor);
    JS_FreeValue(ctx, proto);
    JS_DefinePropertyValueStr(ctx, global_obj, "location", loc, JS_PROP_C_W_E);

    /* Mark as initialized */
    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_location_init", JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);

    return 0;
}
