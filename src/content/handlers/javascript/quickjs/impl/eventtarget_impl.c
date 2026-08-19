#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSEventTarget.gen.h"

extern bool wisp_is_js_process;

static QJSNodePrivate *get_priv_with_global(JSContext *ctx, JSValueConst val) {
    if (JS_IsUndefined(val) || JS_IsNull(val)) {
        struct jsthread *t = JS_GetContextOpaque(ctx);
        if (t) return &t->global_window_priv;
    }
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, val);
    if (!priv) {
        JSValue global = JS_GetGlobalObject(ctx);
        if (JS_VALUE_GET_PTR(global) == JS_VALUE_GET_PTR(val)) {
            struct jsthread *t = JS_GetContextOpaque(ctx);
            if (t) priv = &t->global_window_priv;
        }
        JS_FreeValue(ctx, global);
    }
    return priv;
}


static JSValue js_eventtarget_addEventListener_manual(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    JSValueConst actual_this = this_val;
    JSValue global_ref = JS_UNDEFINED;
    if (JS_IsUndefined(this_val) || JS_IsNull(this_val)) {
        global_ref = JS_GetGlobalObject(ctx);
        actual_this = global_ref;
    }

    QJSNodePrivate *priv = get_priv_with_global(ctx, actual_this);
    if (!priv) {
        JS_FreeValue(ctx, global_ref);
        return JS_ThrowTypeError(ctx, "Invalid this");
    }
    if (argc < 2) {
        JS_FreeValue(ctx, global_ref);
        return JS_UNDEFINED;
    }

    JSValue listeners = JS_GetPropertyStr(ctx, actual_this, "__wisp_listeners");
    if (JS_IsUndefined(listeners)) {
        listeners = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, actual_this, "__wisp_listeners", JS_DupValue(ctx, listeners));
    }
    const char *type = JS_ToCString(ctx, argv[0]);
    if (!type) {
        JS_FreeValue(ctx, listeners);
        JS_FreeValue(ctx, global_ref);
        return JS_EXCEPTION;
    }
    JSValue list = JS_GetPropertyStr(ctx, listeners, type);
    if (JS_IsUndefined(list)) {
        list = JS_NewArray(ctx);
        JS_SetPropertyStr(ctx, listeners, type, JS_DupValue(ctx, list));
    }
    int len = 0;
    JSValue js_len = JS_GetPropertyStr(ctx, list, "length");
    JS_ToInt32(ctx, &len, js_len);
    JS_FreeValue(ctx, js_len);

    bool is_capture = false;
    if (argc >= 3) {
        if (JS_IsBool(argv[2])) {
            is_capture = JS_ToBool(ctx, argv[2]);
        } else if (JS_IsObject(argv[2])) {
            JSValue cap_val = JS_GetPropertyStr(ctx, argv[2], "capture");
            is_capture = JS_ToBool(ctx, cap_val);
            JS_FreeValue(ctx, cap_val);
        }
    }

    bool found = false;
    for (int i = 0; i < len; i++) {
        JSValue item = JS_GetPropertyUint32(ctx, list, i);
        JSValue cb = JS_GetPropertyStr(ctx, item, "callback");
        if (JS_VALUE_GET_PTR(cb) == JS_VALUE_GET_PTR(argv[1])) {
            found = true;
            JS_FreeValue(ctx, cb);
            JS_FreeValue(ctx, item);
            break;
        }
        JS_FreeValue(ctx, cb);
        JS_FreeValue(ctx, item);
    }
    if (!found) {
        JSValue record = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, record, "callback", JS_DupValue(ctx, argv[1]));
        JS_SetPropertyStr(ctx, record, "capture", JS_NewBool(ctx, is_capture));
        JS_SetPropertyUint32(ctx, list, len, record);
    }
    JS_FreeValue(ctx, list);
    JS_FreeValue(ctx, listeners);

    struct jsthread *thread = JS_GetContextOpaque(ctx);
    bool is_real_dom_node = priv->is_dom_node || (thread && priv == &thread->global_window_priv);

    if (!found && !wisp_is_js_process && is_real_dom_node && priv->node != NULL) {
        dom_string *type_dom = NULL;
        dom_string_create((const uint8_t *)type, strlen(type), &type_dom);
        js_dom_event_add_listener(thread, qjs_thread_get_document(thread), (dom_node *)priv->node, type_dom, argv[1]);
        dom_string_unref(type_dom);
    }

    JS_FreeCString(ctx, type);
    JS_FreeValue(ctx, global_ref);
    return JS_UNDEFINED;
}

static JSValue js_eventtarget_removeEventListener_manual(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    JSValueConst actual_this = this_val;
    JSValue global_ref = JS_UNDEFINED;
    if (JS_IsUndefined(this_val) || JS_IsNull(this_val)) {
        global_ref = JS_GetGlobalObject(ctx);
        actual_this = global_ref;
    }

    QJSNodePrivate *priv = get_priv_with_global(ctx, actual_this);
    if (!priv) {
        JS_FreeValue(ctx, global_ref);
        return JS_ThrowTypeError(ctx, "Invalid this");
    }
    if (argc < 2) {
        JS_FreeValue(ctx, global_ref);
        return JS_UNDEFINED;
    }

    JSValue listeners = JS_GetPropertyStr(ctx, actual_this, "__wisp_listeners");
    if (!JS_IsUndefined(listeners)) {
        const char *type = JS_ToCString(ctx, argv[0]);
        if (type) {
            JSValue list = JS_GetPropertyStr(ctx, listeners, type);
            if (!JS_IsUndefined(list)) {
                int len = 0;
                JSValue js_len = JS_GetPropertyStr(ctx, list, "length");
                JS_ToInt32(ctx, &len, js_len);
                JS_FreeValue(ctx, js_len);

                JSValue new_list = JS_NewArray(ctx);
                int new_idx = 0;
                for (int i = 0; i < len; i++) {
                    JSValue item = JS_GetPropertyUint32(ctx, list, i);
                    JSValue cb = JS_GetPropertyStr(ctx, item, "callback");
                    if (JS_VALUE_GET_PTR(cb) != JS_VALUE_GET_PTR(argv[1])) {
                        JS_SetPropertyUint32(ctx, new_list, new_idx++, JS_DupValue(ctx, item));
                    }
                    JS_FreeValue(ctx, cb);
                    JS_FreeValue(ctx, item);
                }
                JS_SetPropertyStr(ctx, listeners, type, new_list);
            }
            JS_FreeValue(ctx, list);
            JS_FreeCString(ctx, type);
        }
    }
    JS_FreeValue(ctx, listeners);

    struct jsthread *thread = JS_GetContextOpaque(ctx);
    bool is_real_dom_node = priv->is_dom_node || (thread && priv == &thread->global_window_priv);

    if (!wisp_is_js_process && is_real_dom_node && priv->node != NULL) {
        const char *type = JS_ToCString(ctx, argv[0]);
        if (type) {
            dom_string *type_dom = NULL;
            dom_string_create((const uint8_t *)type, strlen(type), &type_dom);
            js_dom_event_remove_listener(thread, qjs_thread_get_document(thread), (dom_node *)priv->node, type_dom, argv[1]);
            dom_string_unref(type_dom);
            JS_FreeCString(ctx, type);
        }
    }

    JS_FreeValue(ctx, global_ref);
    return JS_UNDEFINED;
}

static JSValue js_eventtarget_dispatchEvent_manual(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    JSValueConst actual_this = this_val;
    JSValue global_ref = JS_UNDEFINED;
    if (JS_IsUndefined(this_val) || JS_IsNull(this_val)) {
        global_ref = JS_GetGlobalObject(ctx);
        actual_this = global_ref;
    }

    QJSNodePrivate *priv = get_priv_with_global(ctx, actual_this);
    if (!priv) {
        JS_FreeValue(ctx, global_ref);
        return JS_ThrowTypeError(ctx, "Invalid this");
    }
    if (argc < 1) {
        JS_FreeValue(ctx, global_ref);
        return JS_FALSE;
    }

    struct jsthread *thread = JS_GetContextOpaque(ctx);
    bool is_real_dom_node = priv->is_dom_node || (thread && priv == &thread->global_window_priv);

    if (wisp_is_js_process || !is_real_dom_node || priv->node == NULL) {
        const char *type = NULL;
        JSValue type_val = JS_UNDEFINED;
        if (JS_IsObject(argv[0])) {
            type_val = JS_GetPropertyStr(ctx, argv[0], "type");
            if (JS_IsString(type_val)) {
                type = JS_ToCString(ctx, type_val);
            }
        }
        if (!type && JS_IsString(argv[0])) {
            type = JS_ToCString(ctx, argv[0]);
        }
        if (!type) {
            JS_FreeValue(ctx, type_val);
            JS_FreeValue(ctx, global_ref);
            return JS_FALSE;
        }

        JSValue listeners = JS_GetPropertyStr(ctx, actual_this, "__wisp_listeners");
        if (!JS_IsUndefined(listeners)) {
            JSValue list = JS_GetPropertyStr(ctx, listeners, type);
            if (!JS_IsUndefined(list)) {
                int len = 0;
                JSValue js_len = JS_GetPropertyStr(ctx, list, "length");
                JS_ToInt32(ctx, &len, js_len);
                JS_FreeValue(ctx, js_len);

                if (JS_IsObject(argv[0])) {
                    JS_SetPropertyStr(ctx, argv[0], "target", JS_DupValue(ctx, actual_this));
                    JS_SetPropertyStr(ctx, argv[0], "currentTarget", JS_DupValue(ctx, actual_this));
                }

                for (int i = 0; i < len; i++) {
                    JSValue item = JS_GetPropertyUint32(ctx, list, i);
                    JSValue cb = JS_GetPropertyStr(ctx, item, "callback");
                    if (JS_IsFunction(ctx, cb)) {
                        JSValue ret = JS_Call(ctx, cb, actual_this, 1, argv);
                        if (JS_IsException(ret)) {
                            JSValue exception = JS_GetException(ctx);
                            const char *err_msg = JS_ToCString(ctx, exception);
                            NSLOG(wisp, WARNING, "Error in event listener: %s", err_msg ? err_msg : "unknown");
                            if (err_msg) JS_FreeCString(ctx, err_msg);
                            JS_FreeValue(ctx, exception);
                        }
                        JS_FreeValue(ctx, ret);
                    } else if (JS_IsObject(cb)) {
                        JSValue handleEvent = JS_GetPropertyStr(ctx, cb, "handleEvent");
                        if (JS_IsFunction(ctx, handleEvent)) {
                            JSValue ret = JS_Call(ctx, handleEvent, cb, 1, argv);
                            if (JS_IsException(ret)) {
                                JSValue exception = JS_GetException(ctx);
                                const char *err_msg = JS_ToCString(ctx, exception);
                                NSLOG(wisp, WARNING, "Error in event listener handleEvent: %s", err_msg ? err_msg : "unknown");
                                if (err_msg) JS_FreeCString(ctx, err_msg);
                                JS_FreeValue(ctx, exception);
                            }
                            JS_FreeValue(ctx, ret);
                        }
                        JS_FreeValue(ctx, handleEvent);
                    }
                    JS_FreeValue(ctx, cb);
                    JS_FreeValue(ctx, item);
                }
            }
            JS_FreeValue(ctx, list);
        }
        JS_FreeValue(ctx, listeners);
        if (type) JS_FreeCString(ctx, (char *)type);
        JS_FreeValue(ctx, type_val);
        JS_FreeValue(ctx, global_ref);
        return JS_TRUE;
    }

    const char *type = NULL;
    JSValue type_val = JS_UNDEFINED;
    if (JS_IsObject(argv[0])) {
        type_val = JS_GetPropertyStr(ctx, argv[0], "type");
        if (JS_IsString(type_val)) {
            type = JS_ToCString(ctx, type_val);
        }
    }
    if (!type && JS_IsString(argv[0])) {
        type = JS_ToCString(ctx, argv[0]);
    }

    bool success = js_fire_event(thread, type ? type : "click", qjs_thread_get_document(thread), (dom_node *)priv->node);

    if (type) JS_FreeCString(ctx, (char *)type);
    JS_FreeValue(ctx, type_val);
    JS_FreeValue(ctx, global_ref);

    return JS_NewBool(ctx, success);
}

static JSValue js_eventtarget_constructor_manual(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv)
{
    return qjs_new_eventtarget(ctx, NULL, false);
}

JSValue wisp_eventtarget_constructor_impl(JSContext *ctx)
{
    return qjs_new_eventtarget(ctx, NULL, false);
}

JSValue wisp_eventtarget_addEventListener_impl(JSContext *ctx, QJSNodePrivate *priv, const char * type, JSValue callback, bool capture) { return JS_UNDEFINED; }
JSValue wisp_eventtarget_removeEventListener_impl(JSContext *ctx, QJSNodePrivate *priv, const char * type, JSValue callback, bool capture) { return JS_UNDEFINED; }
JSValue wisp_eventtarget_dispatchEvent_impl(JSContext *ctx, QJSNodePrivate *priv, void * event)
{
    struct jsthread *thread = JS_GetContextOpaque(ctx);
    if (!thread || !priv || !event) return JS_FALSE;

    /* 'event' here is the LibDOM dom_event pointer extracted from the wrapper's private data */
    bool success = false;
    dom_node *target = (dom_node *)priv->node;
    if (target == (dom_node *)thread->win_priv) {
        target = (dom_node *)qjs_thread_get_document(thread);
        if (!target) return JS_FALSE;
    }
    dom_event_target_dispatch_event((dom_event_target *)target, (dom_event *)event, &success);
    return JS_NewBool(ctx, success);
}

int qjs_init_eventtarget(JSContext *ctx)
{
    static const char *init_key = "__wisp_eventtarget_init";
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue check = JS_GetPropertyStr(ctx, global_obj, init_key);
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    qjs_init_eventtarget_gen(ctx);
    JSValue proto = JS_GetClassProto(ctx, qjs_eventtarget_class_id);
    if (!JS_IsObject(proto)) {
        JS_FreeValue(ctx, proto);
        proto = JS_NewObject(ctx);
        JS_SetClassProto(ctx, qjs_eventtarget_class_id, JS_DupValue(ctx, proto));
    }
    JS_DefinePropertyValueStr(ctx, proto, "addEventListener", JS_NewCFunction(ctx, js_eventtarget_addEventListener_manual, "addEventListener", 3), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, proto, "removeEventListener", JS_NewCFunction(ctx, js_eventtarget_removeEventListener_manual, "removeEventListener", 3), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, proto, "dispatchEvent", JS_NewCFunction(ctx, js_eventtarget_dispatchEvent_manual, "dispatchEvent", 1), JS_PROP_C_W_E);

    // Overwrite the non-constructible EventTarget global constructor with our constructible manual constructor
    JSValue ctor = JS_NewCFunction2(ctx, js_eventtarget_constructor_manual, "EventTarget", 0, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, ctor, proto);
    JS_SetPropertyStr(ctx, global_obj, "EventTarget", ctor);

    JS_FreeValue(ctx, proto);

    JS_DefinePropertyValueStr(ctx, global_obj, init_key, JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);
    return 0;
}
