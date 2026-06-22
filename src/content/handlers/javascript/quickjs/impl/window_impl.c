#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include "JSWindow.gen.h"

/* Custom Window init to handle global object */
int qjs_init_window(JSContext *ctx)
{
    /* Initialize the class and prototype using the generated function */
    qjs_init_window_gen(ctx);

    JSValue global_obj = JS_GetGlobalObject(ctx);

    /* NetSurf/Wisp specific: The global object needs to have the Window properties.
     * We achieve this by setting the Window prototype as the global object's prototype.
     */
    JSValue proto = JS_GetClassProto(ctx, qjs_window_class_id);
    if (JS_IsObject(proto)) {
        JS_SetPrototype(ctx, global_obj, proto);
    }
    JS_FreeValue(ctx, proto);

    /* Set self references */
    JS_SetPropertyStr(ctx, global_obj, "window", JS_DupValue(ctx, global_obj));
    JS_SetPropertyStr(ctx, global_obj, "self", JS_DupValue(ctx, global_obj));

    JS_FreeValue(ctx, global_obj);
    return 0;
}
