#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSEventTarget.gen.h"

static JSValue js_eventtarget_addEventListener_manual(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = qjs_get_dom_priv(this_val);
    if (!priv) return JS_EXCEPTION;
    if (argc < 2) return JS_UNDEFINED;

    const char *type = JS_ToCString(ctx, argv[0]);
    dom_string *type_dom = NULL;
    dom_string_create((const uint8_t *)type, strlen(type), &type_dom);

    struct jsthread *thread = JS_GetContextOpaque(ctx);
    js_dom_event_add_listener(thread, (dom_document *)priv->node, (dom_node *)priv->node, type_dom, argv[1]);

    dom_string_unref(type_dom);
    JS_FreeCString(ctx, type);
    return JS_UNDEFINED;
}

static JSValue js_eventtarget_removeEventListener_manual(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = qjs_get_dom_priv(this_val);
    if (!priv) return JS_EXCEPTION;
    if (argc < 2) return JS_UNDEFINED;

    const char *type = JS_ToCString(ctx, argv[0]);
    dom_string *type_dom = NULL;
    dom_string_create((const uint8_t *)type, strlen(type), &type_dom);

    struct jsthread *thread = JS_GetContextOpaque(ctx);
    js_dom_event_remove_listener(thread, (dom_document *)priv->node, (dom_node *)priv->node, type_dom, argv[1]);

    dom_string_unref(type_dom);
    JS_FreeCString(ctx, type);
    return JS_UNDEFINED;
}

static JSValue js_eventtarget_dispatchEvent_manual(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = qjs_get_dom_priv(this_val);
    if (!priv) return JS_EXCEPTION;
    if (argc < 1) return JS_FALSE;

    struct jsthread *thread = JS_GetContextOpaque(ctx);
    const char *type = "click";
    JSValue type_val = JS_GetPropertyStr(ctx, argv[0], "type");
    if (JS_IsString(type_val)) {
        type = JS_ToCString(ctx, type_val);
    } else if (JS_IsString(argv[0])) {
        type = JS_ToCString(ctx, argv[0]);
    }

    bool success = js_fire_event(thread, type, NULL, (dom_node *)priv->node);

    if (type != (const char *)"click") JS_FreeCString(ctx, (char *)type);
    JS_FreeValue(ctx, type_val);

    return JS_NewBool(ctx, success);
}

JSValue wisp_eventtarget_addEventListener_impl(JSContext *ctx, QJSNodePrivate *priv, const char * type, void * callback, bool capture) { return JS_UNDEFINED; }
JSValue wisp_eventtarget_removeEventListener_impl(JSContext *ctx, QJSNodePrivate *priv, const char * type, void * callback, bool capture) { return JS_UNDEFINED; }
JSValue wisp_eventtarget_dispatchEvent_impl(JSContext *ctx, QJSNodePrivate *priv, void * event) { return JS_FALSE; }

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
    if (JS_IsObject(proto)) {
        JS_DefinePropertyValueStr(ctx, proto, "addEventListener", JS_NewCFunction(ctx, js_eventtarget_addEventListener_manual, "addEventListener", 3), JS_PROP_C_W_E);
        JS_DefinePropertyValueStr(ctx, proto, "removeEventListener", JS_NewCFunction(ctx, js_eventtarget_removeEventListener_manual, "removeEventListener", 3), JS_PROP_C_W_E);
        JS_DefinePropertyValueStr(ctx, proto, "dispatchEvent", JS_NewCFunction(ctx, js_eventtarget_dispatchEvent_manual, "dispatchEvent", 1), JS_PROP_C_W_E);
    }
    JS_FreeValue(ctx, proto);

    JS_DefinePropertyValueStr(ctx, global_obj, init_key, JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);
    return 0;
}
