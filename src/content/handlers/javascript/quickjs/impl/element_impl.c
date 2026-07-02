#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSElement.gen.h"

JSValue wisp_element_getAttribute_impl(JSContext *ctx, QJSNodePrivate *priv, const char * qualifiedName)
{
    if (!priv || !priv->node) return JS_NULL;
    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)qualifiedName, strlen(qualifiedName), &name_dom);
    dom_string *value_dom = NULL;
    dom_element_get_attribute((dom_element *)priv->node, name_dom, &value_dom);
    dom_string_unref(name_dom);
    if (value_dom) {
        JSValue val = JS_NewStringLen(ctx, (const char *)dom_string_data(value_dom), dom_string_byte_length(value_dom));
        dom_string_unref(value_dom);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_element_setAttribute_impl(JSContext *ctx, QJSNodePrivate *priv, const char * qualifiedName, const char * value)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)qualifiedName, strlen(qualifiedName), &name_dom);
    dom_string *value_dom = NULL;
    dom_string_create((const uint8_t *)value, strlen(value), &value_dom);
    dom_element_set_attribute((dom_element *)priv->node, name_dom, value_dom);
    dom_string_unref(name_dom);
    dom_string_unref(value_dom);
    return JS_UNDEFINED;
}

JSValue wisp_element_removeAttribute_impl(JSContext *ctx, QJSNodePrivate *priv, const char * qualifiedName)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)qualifiedName, strlen(qualifiedName), &name_dom);
    dom_element_remove_attribute((dom_element *)priv->node, name_dom);
    dom_string_unref(name_dom);
    return JS_UNDEFINED;
}

JSValue wisp_element_hasAttribute_impl(JSContext *ctx, QJSNodePrivate *priv, const char * qualifiedName)
{
    if (!priv || !priv->node) return JS_FALSE;
    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)qualifiedName, strlen(qualifiedName), &name_dom);
    bool result = false;
    dom_element_has_attribute((dom_element *)priv->node, name_dom, &result);
    dom_string_unref(name_dom);
    return JS_NewBool(ctx, result);
}

JSValue wisp_element_id_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return wisp_element_getAttribute_impl(ctx, priv, "id"); }
JSValue wisp_element_id_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) { return wisp_element_setAttribute_impl(ctx, priv, "id", value); }
JSValue wisp_element_className_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return wisp_element_getAttribute_impl(ctx, priv, "class"); }
JSValue wisp_element_className_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) { return wisp_element_setAttribute_impl(ctx, priv, "class", value); }

JSValue wisp_element_innerHTML_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, ""); }
JSValue wisp_element_innerHTML_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) { return JS_UNDEFINED; }
JSValue wisp_element_tagName_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    dom_string *name = NULL;
    dom_element_get_tag_name((dom_element *)priv->node, &name);
    if (name) {
        JSValue val = JS_NewStringLen(ctx, (const char *)dom_string_data(name), dom_string_byte_length(name));
        dom_string_unref(name);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_element_classList_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NULL; }
JSValue wisp_element_attributes_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NULL; }
JSValue wisp_element_style_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewObject(ctx); }

JSValue wisp_element_querySelector_impl(JSContext *ctx, QJSNodePrivate *priv, const char * selectors)
{
    if (!priv || !priv->node) return JS_NULL;
    return qjs_dom_query_selector_internal(ctx, (dom_node *)priv->node, selectors, false);
}

JSValue wisp_element_querySelectorAll_impl(JSContext *ctx, QJSNodePrivate *priv, const char * selectors)
{
    if (!priv || !priv->node) return JS_NULL;
    return qjs_dom_query_selector_internal(ctx, (dom_node *)priv->node, selectors, true);
}

int qjs_init_element(JSContext *ctx) {
    qjs_init_element_gen(ctx);
    JSValue proto = JS_GetClassProto(ctx, qjs_element_class_id);
    JSValue node_proto = JS_GetClassProto(ctx, qjs_node_class_id);
    if (JS_IsObject(proto) && JS_IsObject(node_proto)) JS_SetPrototype(ctx, proto, node_proto);
    JS_FreeValue(ctx, node_proto);
    JS_FreeValue(ctx, proto);
    return 0;
}
