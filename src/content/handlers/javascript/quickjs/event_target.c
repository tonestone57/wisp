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

        if (type && is_func) {
            JSValue listeners = JS_GetPropertyStr(ctx, this_val, "__listeners");
            if (JS_IsException(listeners) || JS_IsUndefined(listeners) || JS_IsNull(listeners)) {
                listeners = JS_NewObject(ctx);
                JS_SetPropertyStr(ctx, this_val, "__listeners", JS_DupValue(ctx, listeners));
            }

            JSValue type_listeners = JS_GetPropertyStr(ctx, listeners, type);
            if (JS_IsException(type_listeners) || JS_IsUndefined(type_listeners) || JS_IsNull(type_listeners)) {
                type_listeners = JS_NewArray(ctx);
                JS_SetPropertyStr(ctx, listeners, type, JS_DupValue(ctx, type_listeners));
            }

            /* Get current array length */
            JSValue length_val = JS_GetPropertyStr(ctx, type_listeners, "length");
            uint32_t len = 0;
            if (!JS_IsException(length_val)) {
                JS_ToUint32(ctx, &len, length_val);
                JS_FreeValue(ctx, length_val);
            }

            /* Check for duplicates */
            bool duplicate = false;
            for (uint32_t i = 0; i < len; i++) {
                JSValue func = JS_GetPropertyUint32(ctx, type_listeners, i);
                if (!JS_IsException(func)) {
                    if (JS_IsObject(func) && JS_VALUE_GET_PTR(func) == JS_VALUE_GET_PTR(argv[1])) {
                        duplicate = true;
                        JS_FreeValue(ctx, func);
                        break;
                    }
                    JS_FreeValue(ctx, func);
                }
            }

            /* Push listener function if not duplicate */
            if (!duplicate) {
                JS_SetPropertyUint32(ctx, type_listeners, len, JS_DupValue(ctx, argv[1]));
            }

            JS_FreeValue(ctx, type_listeners);
            JS_FreeValue(ctx, listeners);
        }

        if (type) {
            JS_FreeCString(ctx, type);
        }
    }
    return JS_UNDEFINED;
}

JSValue js_removeEventListener(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc >= 2) {
        const char *type = JS_ToCString(ctx, argv[0]);
        NSLOG(wisp, INFO, "removeEventListener('%s')", type);

        if (type && JS_IsObject(argv[1])) {
            JSValue listeners = JS_GetPropertyStr(ctx, this_val, "__listeners");
            if (!JS_IsException(listeners) && !JS_IsUndefined(listeners) && !JS_IsNull(listeners)) {
                JSValue type_listeners = JS_GetPropertyStr(ctx, listeners, type);
                if (!JS_IsException(type_listeners) && !JS_IsUndefined(type_listeners) && !JS_IsNull(type_listeners)) {
                    /* Find and remove */
                    JSValue length_val = JS_GetPropertyStr(ctx, type_listeners, "length");
                    uint32_t len = 0;
                    if (!JS_IsException(length_val)) {
                        JS_ToUint32(ctx, &len, length_val);
                        JS_FreeValue(ctx, length_val);
                    }

                    for (uint32_t i = 0; i < len; i++) {
                        JSValue func = JS_GetPropertyUint32(ctx, type_listeners, i);
                        /* Strict equality equivalent check (we assume it's the same object) */
                        if (!JS_IsException(func)) {
                            /* Basic object identity comparison */
                            if (JS_IsObject(func) && JS_VALUE_GET_PTR(func) == JS_VALUE_GET_PTR(argv[1])) {
                                /* Shift elements down */
                                for (uint32_t j = i; j < len - 1; j++) {
                                    JSValue next = JS_GetPropertyUint32(ctx, type_listeners, j + 1);
                                    JS_SetPropertyUint32(ctx, type_listeners, j, next);
                                }
                                /* Adjust length */
                                JS_SetPropertyStr(ctx, type_listeners, "length", JS_NewUint32(ctx, len - 1));
                                JS_FreeValue(ctx, func);
                                break;
                            }
                            JS_FreeValue(ctx, func);
                        }
                    }
                    JS_FreeValue(ctx, type_listeners);
                }
                JS_FreeValue(ctx, listeners);
            }
            JS_FreeCString(ctx, type);
        }
    }
    return JS_UNDEFINED;
}

JSValue js_dispatchEvent(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, INFO, "dispatchEvent called");
    if (argc >= 1) {
        JSValue event_val = argv[0];
        const char *type = NULL;

        /* Extract event type: argv[0] could be a string (custom event) or an Event object with a 'type' property */
        if (JS_IsString(event_val)) {
            type = JS_ToCString(ctx, event_val);
        } else if (JS_IsObject(event_val)) {
            JSValue type_val = JS_GetPropertyStr(ctx, event_val, "type");
            if (!JS_IsException(type_val) && JS_IsString(type_val)) {
                type = JS_ToCString(ctx, type_val);
            }
            JS_FreeValue(ctx, type_val);
        }

        if (type) {
            JSValue listeners = JS_GetPropertyStr(ctx, this_val, "__listeners");
            if (!JS_IsException(listeners) && !JS_IsUndefined(listeners) && !JS_IsNull(listeners)) {
                JSValue type_listeners = JS_GetPropertyStr(ctx, listeners, type);
                if (!JS_IsException(type_listeners) && !JS_IsUndefined(type_listeners) && !JS_IsNull(type_listeners)) {
                    JSValue length_val = JS_GetPropertyStr(ctx, type_listeners, "length");
                    uint32_t len = 0;
                    if (!JS_IsException(length_val)) {
                        JS_ToUint32(ctx, &len, length_val);
                        JS_FreeValue(ctx, length_val);
                    }

                    /* Clone the listeners array to prevent issues if listeners are removed during dispatch */
                    JSValue *funcs = NULL;
                    if (len > 0) {
                        funcs = malloc(len * sizeof(JSValue));
                        if (funcs) {
                            for (uint32_t i = 0; i < len; i++) {
                                /* JS_GetPropertyUint32 returns a new reference, no need to DupValue */
                                funcs[i] = JS_GetPropertyUint32(ctx, type_listeners, i);
                            }
                        }
                    }

                    for (uint32_t i = 0; i < len; i++) {
                        if (funcs) {
                            JSValue func = funcs[i];
                            if (JS_IsFunction(ctx, func)) {
                                JSValue ret = JS_Call(ctx, func, this_val, 1, &event_val);
                                if (JS_IsException(ret)) {
                                    JSValue exception_val = JS_GetException(ctx);
                                    const char *ex_str = JS_ToCString(ctx, exception_val);
                                    NSLOG(wisp, ERROR, "Error in event listener for '%s': %s", type, ex_str ? ex_str : "unknown");
                                    if (ex_str) JS_FreeCString(ctx, ex_str);
                                    JS_FreeValue(ctx, exception_val);
                                }
                                JS_FreeValue(ctx, ret);
                            }
                            JS_FreeValue(ctx, func);
                        }
                    }

                    if (funcs) {
                        free(funcs);
                    }

                    JS_FreeValue(ctx, type_listeners);
                }
                JS_FreeValue(ctx, listeners);
            }
            JS_FreeCString(ctx, type);
        }
    }

    /* Always return true - event was handled */
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
