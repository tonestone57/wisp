/* Implementation for EventTarget */
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"

JSClassID qjs_eventtarget_class_id;

static void js_eventtarget_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_eventtarget_class_id);
    if (priv) {
        qjs_bridge_remove_node(rt, (dom_node *)priv->node, priv->ctx);
        if (priv->is_dom_node && priv->node) dom_node_unref((dom_node *)priv->node);
        free(priv);
    }
}

static JSClassDef js_eventtarget_class = {
    "EventTarget",
    .finalizer = js_eventtarget_finalizer,
};

static JSValue js_eventtarget_addEventListener(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_eventtarget_class_id);
    if (!priv) {
        /* Could be a Node/Element/Document which inherits from EventTarget */
        priv = JS_GetOpaque(this_val, qjs_node_class_id);
    }
    if (!priv) {
        priv = JS_GetOpaque(this_val, qjs_element_class_id);
    }
    if (!priv) {
        priv = JS_GetOpaque(this_val, qjs_document_class_id);
    }
    if (!priv || !priv->node) return JS_ThrowTypeError(ctx, "Not an EventTarget");

    if (argc < 2) return JS_ThrowTypeError(ctx, "Expected 2 arguments");

    const char *type = JS_ToCString(ctx, argv[0]);
    if (!type) return JS_EXCEPTION;

    dom_string *type_dom = NULL;
    dom_string_create((const uint8_t *)type, strlen(type), &type_dom);
    JS_FreeCString(ctx, type);

    struct jsthread *thread = JS_GetContextOpaque(ctx);
    struct dom_document *doc = qjs_get_document_priv(ctx);

    js_dom_event_add_listener(thread, doc, (dom_node *)priv->node, type_dom, argv[1]);

    dom_string_unref(type_dom);

    return JS_UNDEFINED;
}

static JSValue js_eventtarget_removeEventListener(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_eventtarget_class_id);
    if (!priv) {
        priv = JS_GetOpaque(this_val, qjs_node_class_id);
    }
    if (!priv) {
        priv = JS_GetOpaque(this_val, qjs_element_class_id);
    }
    if (!priv) {
        priv = JS_GetOpaque(this_val, qjs_document_class_id);
    }
    if (!priv || !priv->node) return JS_ThrowTypeError(ctx, "Not an EventTarget");

    if (argc < 2) return JS_ThrowTypeError(ctx, "Expected 2 arguments");

    const char *type = JS_ToCString(ctx, argv[0]);
    if (!type) return JS_EXCEPTION;

    dom_string *type_dom = NULL;
    dom_string_create((const uint8_t *)type, strlen(type), &type_dom);
    JS_FreeCString(ctx, type);

    struct jsthread *thread = JS_GetContextOpaque(ctx);
    struct dom_document *doc = qjs_get_document_priv(ctx);

    js_dom_event_remove_listener(thread, doc, (dom_node *)priv->node, type_dom, argv[1]);

    dom_string_unref(type_dom);

    return JS_UNDEFINED;
}

static JSValue js_eventtarget_dispatchEvent(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "EventTarget.dispatchEvent() called (stub)");
    return JS_FALSE;
}

static const JSCFunctionListEntry js_eventtarget_proto_funcs[] = {
    JS_CFUNC_DEF("addEventListener", 2, js_eventtarget_addEventListener),
    JS_CFUNC_DEF("removeEventListener", 2, js_eventtarget_removeEventListener),
    JS_CFUNC_DEF("dispatchEvent", 1, js_eventtarget_dispatchEvent),
};

int qjs_init_eventtarget(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_eventtarget_class_id == 0) JS_NewClassID(rt, &qjs_eventtarget_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_eventtarget_class_id)) JS_NewClass(rt, qjs_eventtarget_class_id, &js_eventtarget_class);
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_eventtarget_proto_funcs, sizeof(js_eventtarget_proto_funcs) / sizeof(js_eventtarget_proto_funcs[0]));
    JS_SetClassProto(ctx, qjs_eventtarget_class_id, proto);
    JS_FreeValue(ctx, proto);
    return 0;
}
