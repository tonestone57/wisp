/*
 * Copyright 2026 Neosurf Contributors
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include <dom/core/attr.h>
JSClassID qjs_attr_class_id;
static void js_attr_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_attr_class_id);
    if (priv) {
        qjs_bridge_remove_node(rt, (dom_node *)priv->node, priv->ctx);
        if (priv->is_dom_node && priv->node) dom_node_unref((dom_node *)priv->node);
        free(priv);
    }
}

static JSClassDef js_attr_class = {
    "Attr",
    .finalizer = js_attr_finalizer,
};

static JSValue js_attr_name_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_attr_class_id);
    if (!priv || !priv->node) return JS_NULL;
    dom_string *name = NULL;
    dom_exception exc = dom_attr_get_name((struct dom_attr *)priv->node, &name);
    if (exc != DOM_NO_ERR || name == NULL) return JS_NewString(ctx, "");
    JSValue val = JS_NewStringLen(ctx, dom_string_data(name), dom_string_byte_length(name));
    dom_string_unref(name);
    return val;
}

static JSValue js_attr_value_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_attr_class_id);
    if (!priv || !priv->node) return JS_NULL;
    dom_string *value = NULL;
    dom_exception exc = dom_attr_get_value((struct dom_attr *)priv->node, &value);
    if (exc != DOM_NO_ERR || value == NULL) return JS_NewString(ctx, "");
    JSValue val = JS_NewStringLen(ctx, dom_string_data(value), dom_string_byte_length(value));
    dom_string_unref(value);
    return val;
}

static JSValue js_attr_value_set(JSContext *ctx, JSValueConst this_val, JSValueConst val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_attr_class_id);
    if (!priv || !priv->node) return JS_UNDEFINED;
    const char *str = JS_ToCString(ctx, val);
    if (str) {
        dom_string *data_dom = NULL;
        dom_string_create((const uint8_t *)str, strlen(str), &data_dom);
        dom_attr_set_value((struct dom_attr *)priv->node, data_dom);
        dom_string_unref(data_dom);
        JS_FreeCString(ctx, str);
    }
    return JS_UNDEFINED;
}

static JSValue js_attr_specified_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_attr_class_id);
    if (!priv || !priv->node) return JS_FALSE;
    bool result = false;
    dom_attr_get_specified((struct dom_attr *)priv->node, &result);
    return JS_NewBool(ctx, result);
}

static JSValue js_attr_ownerElement_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_attr_class_id);
    if (!priv || !priv->node) return JS_NULL;
    struct dom_element *element = NULL;
    dom_exception exc = dom_attr_get_owner_element((struct dom_attr *)priv->node, &element);
    if (exc != DOM_NO_ERR || element == NULL) return JS_NULL;
    JSValue val = qjs_wrap_node(ctx, (dom_node *)element);
    dom_node_unref((dom_node *)element);
    return val;
}

static const JSCFunctionListEntry js_attr_proto_funcs[] = {
    JS_CGETSET_DEF("name", js_attr_name_get, NULL),
    JS_CGETSET_DEF("value", js_attr_value_get, js_attr_value_set),
    JS_CGETSET_DEF("specified", js_attr_specified_get, NULL),
    JS_CGETSET_DEF("ownerElement", js_attr_ownerElement_get, NULL),
};

int qjs_init_attr(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_attr_class_id == 0) JS_NewClassID(rt, &qjs_attr_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_attr_class_id)) JS_NewClass(rt, qjs_attr_class_id, &js_attr_class);
    JSValue proto = JS_NewObject(ctx);
    JSValue node_proto = JS_GetClassProto(ctx, qjs_node_class_id);
    JS_SetPrototype(ctx, proto, node_proto);
    JS_FreeValue(ctx, node_proto);
    JS_SetPropertyFunctionList(ctx, proto, js_attr_proto_funcs, sizeof(js_attr_proto_funcs) / sizeof(js_attr_proto_funcs[0]));
    JS_SetClassProto(ctx, qjs_attr_class_id, proto);
    JS_FreeValue(ctx, proto);
    return 0;
}

JSValue qjs_new_attr(JSContext *ctx, void *node, bool is_dom_node)
{
    JSValue obj = JS_NewObjectClass(ctx, qjs_attr_class_id);
    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));
    if (!priv) return JS_ThrowOutOfMemory(ctx);
    priv->node = node; priv->ctx = ctx; priv->is_dom_node = is_dom_node;
    if (is_dom_node && node) dom_node_ref((dom_node *)node);
    JS_SetOpaque(obj, priv); return obj;
}
