/*
 * Copyright 2025 Neosurf Contributors
 *
 * This file is part of NeoSurf, http://www.netsurf-browser.org/
 */


#include "event_target.h"
#include <wisp/utils/log.h>
#include "quickjs.h"
#include <stdlib.h>
#include <string.h>

#include "utils/libdom.h"
extern void *qjs_get_document_priv(JSContext *ctx);
extern bool js_dom_event_add_listener(void *thread, struct dom_document *document, struct dom_node *node, struct dom_string *event_type_dom, void *js_funcval);

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

            /* Register with LibDOM if this is the global object or a known DOM node */
            void *thread = JS_GetContextOpaque(ctx);
            struct dom_document *doc = qjs_get_document_priv(ctx);
            JSValue global_obj = JS_GetGlobalObject(ctx);

            /* If this is the global object, register on the document */
            if (JS_VALUE_GET_PTR(this_val) == JS_VALUE_GET_PTR(global_obj) && doc) {
                struct dom_node *node = (struct dom_node *)doc;
                dom_string *type_dom = NULL;
                dom_string_create((const uint8_t *)type, strlen(type), &type_dom);

                js_dom_event_add_listener(thread, doc, node, type_dom, (void *)&argv[1]);

                dom_string_unref(type_dom);
            }
            JS_FreeValue(ctx, global_obj);
        }
        JS_FreeCString(ctx, type);
    }
    return JS_UNDEFINED;
}


JSValue js_removeEventListener(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc >= 2) {
        const char *type = JS_ToCString(ctx, argv[0]);
        bool is_func = JS_IsFunction(ctx, argv[1]);
        NSLOG(wisp, INFO, "removeEventListener('%s')", type);

        if (is_func) {
            JSValue listeners = JS_GetPropertyStr(ctx, this_val, "__listeners");
            if (!JS_IsUndefined(listeners)) {
                JSValue type_listeners = JS_GetPropertyStr(ctx, listeners, type);
                if (!JS_IsUndefined(type_listeners)) {
                    /* Since JS arrays don't have a simple 'remove value' function in standard JS without indexOf + splice,
                       we'll just use filter. */
                    const char *filter_script = "function(arr, fn) { return arr.filter(function(x) { return x !== fn; }); }";
                    JSValue filter_func = JS_Eval(ctx, filter_script, strlen(filter_script), "<filter>", JS_EVAL_TYPE_GLOBAL);

                    JSValue args[2] = { type_listeners, argv[1] };
                    JSValue new_arr = JS_Call(ctx, filter_func, JS_UNDEFINED, 2, args);
                    if (!JS_IsException(new_arr)) {
                        JS_SetPropertyStr(ctx, listeners, type, new_arr);
                    } else {
                        JSValue exc = JS_GetException(ctx);
                        JS_FreeValue(ctx, exc);
                    }
                    JS_FreeValue(ctx, filter_func);
                }
                JS_FreeValue(ctx, type_listeners);
            }
            JS_FreeValue(ctx, listeners);
        }
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
