#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSEventTarget.gen.h"

static QJSNodePrivate *get_priv_with_global(JSContext *ctx, JSValueConst val) {
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, val);
    if (!priv) {
        JSValue global = JS_GetGlobalObject(ctx);
        if (JS_VALUE_GET_PTR(global) == JS_VALUE_GET_PTR(val)) {
            struct jsthread *t = JS_GetContextOpaque(ctx);
            priv = &t->global_window_priv;
        }
        JS_FreeValue(ctx, global);
    }
    return priv;
}


static JSValue js_eventtarget_addEventListener_manual(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = get_priv_with_global(ctx, this_val);
    if (!priv) return JS_ThrowTypeError(ctx, "Invalid this");
    if (argc < 2) return JS_UNDEFINED;

    const char *type = JS_ToCString(ctx, argv[0]);
    if (!type) return JS_EXCEPTION;
    dom_string *type_dom = NULL;
    dom_string_create((const uint8_t *)type, strlen(type), &type_dom);

    struct jsthread *thread = JS_GetContextOpaque(ctx);
    js_dom_event_add_listener(thread, (dom_document *)thread->doc_priv, (dom_node *)priv->node, type_dom, argv[1]);

    dom_string_unref(type_dom);
    JS_FreeCString(ctx, type);
    return JS_UNDEFINED;
}

static JSValue js_eventtarget_removeEventListener_manual(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = get_priv_with_global(ctx, this_val);
    if (!priv) return JS_ThrowTypeError(ctx, "Invalid this");
    if (argc < 2) return JS_UNDEFINED;

    const char *type = JS_ToCString(ctx, argv[0]);
    if (!type) return JS_EXCEPTION;
    dom_string *type_dom = NULL;
    dom_string_create((const uint8_t *)type, strlen(type), &type_dom);

    struct jsthread *thread = JS_GetContextOpaque(ctx);
    js_dom_event_remove_listener(thread, (dom_document *)thread->doc_priv, (dom_node *)priv->node, type_dom, argv[1]);

    dom_string_unref(type_dom);
    JS_FreeCString(ctx, type);
    return JS_UNDEFINED;
}

static JSValue js_eventtarget_dispatchEvent_manual(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = get_priv_with_global(ctx, this_val);
    if (!priv) return JS_ThrowTypeError(ctx, "Invalid this");
    if (argc < 1) return JS_FALSE;

    struct jsthread *thread = JS_GetContextOpaque(ctx);
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

    bool success = js_fire_event(thread, type ? type : "click", (struct dom_document *)thread->doc_priv, (dom_node *)priv->node);

    if (type) JS_FreeCString(ctx, (char *)type);
    JS_FreeValue(ctx, type_val);

    return JS_NewBool(ctx, success);
}

JSValue wisp_eventtarget_addEventListener_impl(JSContext *ctx, QJSNodePrivate *priv, const char * type, JSValue callback, bool capture) { return JS_UNDEFINED; }
JSValue wisp_eventtarget_removeEventListener_impl(JSContext *ctx, QJSNodePrivate *priv, const char * type, JSValue callback, bool capture) { return JS_UNDEFINED; }
JSValue wisp_eventtarget_dispatchEvent_impl(JSContext *ctx, QJSNodePrivate *priv, void * event)
{
    struct jsthread *thread = JS_GetContextOpaque(ctx);
    if (!thread || !priv || !event) return JS_FALSE;

    /* 'event' here is the LibDOM dom_event pointer extracted from the wrapper's private data */
    bool success = false;
    dom_event_target_dispatch_event((dom_event_target *)priv->node, (dom_event *)event, &success);
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
        proto = JS_NewObject(ctx);
        JS_SetClassProto(ctx, qjs_eventtarget_class_id, JS_DupValue(ctx, proto));
    }
    JS_DefinePropertyValueStr(ctx, proto, "addEventListener", JS_NewCFunction(ctx, js_eventtarget_addEventListener_manual, "addEventListener", 3), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, proto, "removeEventListener", JS_NewCFunction(ctx, js_eventtarget_removeEventListener_manual, "removeEventListener", 3), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, proto, "dispatchEvent", JS_NewCFunction(ctx, js_eventtarget_dispatchEvent_manual, "dispatchEvent", 1), JS_PROP_C_W_E);
    JS_FreeValue(ctx, proto);

    JS_DefinePropertyValueStr(ctx, global_obj, init_key, JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);
    return 0;
}
