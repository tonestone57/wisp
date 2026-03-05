/*
 * Copyright 2025 Neosurf Contributors
 *
 * This file is part of NeoSurf, http://www.netsurf-browser.org/
 */

#include "event_target.h"
#include <wisp/utils/log.h>
#include "quickjs.h"
#include <stdlib.h>

JSValue js_addEventListener(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc >= 2) {
        const char *type = JS_ToCString(ctx, argv[0]);
        bool is_func = JS_IsFunction(ctx, argv[1]);
        NSLOG(wisp, INFO, "addEventListener('%s', %s)", type, is_func ? "function" : "non-function");

        if (is_func) {
            JSValue listeners = JS_GetPropertyStr(ctx, this_val, "__listeners");
            if (JS_IsUndefined(listeners)) {
                listeners = JS_NewObject(ctx);
                JS_SetPropertyStr(ctx, this_val, "__listeners", JS_DupValue(ctx, listeners));
            }

            JSValue type_listeners = JS_GetPropertyStr(ctx, listeners, type);
            if (JS_IsUndefined(type_listeners)) {
                type_listeners = JS_NewArray(ctx);
                JS_SetPropertyStr(ctx, listeners, type, JS_DupValue(ctx, type_listeners));
            }

            JSValue push_func = JS_GetPropertyStr(ctx, type_listeners, "push");
            JSValue ret = JS_Call(ctx, push_func, type_listeners, 1, &argv[1]);
            JS_FreeValue(ctx, ret);

            JS_FreeValue(ctx, push_func);
            JS_FreeValue(ctx, type_listeners);
            JS_FreeValue(ctx, listeners);
        }
        JS_FreeCString(ctx, type);
    }
    return JS_UNDEFINED;
}

JSValue js_removeEventListener(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc >= 2) {
        const char *type = JS_ToCString(ctx, argv[0]);
        NSLOG(wisp, INFO, "removeEventListener('%s')", type);
        JS_FreeCString(ctx, type);
    }
    return JS_UNDEFINED;
}

JSValue js_dispatchEvent(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc >= 1) {
        JSValue event = argv[0];
        JSValue type_val = JS_GetPropertyStr(ctx, event, "type");
        const char *type = JS_ToCString(ctx, type_val);

        NSLOG(wisp, INFO, "dispatchEvent called for '%s'", type ? type : "unknown");

        if (type) {
            JSValue listeners = JS_GetPropertyStr(ctx, this_val, "__listeners");
            if (!JS_IsUndefined(listeners)) {
                JSValue type_listeners = JS_GetPropertyStr(ctx, listeners, type);
                if (!JS_IsUndefined(type_listeners)) {
                    JSValue length_val = JS_GetPropertyStr(ctx, type_listeners, "length");
                    int32_t len;
                    JS_ToInt32(ctx, &len, length_val);
                    JS_FreeValue(ctx, length_val);

                    for (int32_t i = 0; i < len; i++) {
                        JSValue func = JS_GetPropertyUint32(ctx, type_listeners, i);
                        JSValue ret = JS_Call(ctx, func, this_val, 1, &event);
                        JS_FreeValue(ctx, ret);
                        JS_FreeValue(ctx, func);
                    }
                }
                JS_FreeValue(ctx, type_listeners);
            }
            JS_FreeValue(ctx, listeners);
            JS_FreeCString(ctx, type);
        }
        JS_FreeValue(ctx, type_val);
    }
    return JS_NewBool(ctx, 1);
}

int qjs_init_event_target(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);

    /* Add EventTarget methods to global (window) object */
    JS_SetPropertyStr(
        ctx, global_obj, "addEventListener", JS_NewCFunction(ctx, js_addEventListener, "addEventListener", 2));
    JS_SetPropertyStr(
        ctx, global_obj, "removeEventListener", JS_NewCFunction(ctx, js_removeEventListener, "removeEventListener", 2));
    JS_SetPropertyStr(ctx, global_obj, "dispatchEvent", JS_NewCFunction(ctx, js_dispatchEvent, "dispatchEvent", 1));

    JS_FreeValue(ctx, global_obj);
    return 0;
}
