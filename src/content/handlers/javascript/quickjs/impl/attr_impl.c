#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSAttr.gen.h"

JSValue wisp_attr_name_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    dom_string *name = NULL;
    dom_attr_get_name((dom_attr *)priv->node, &name);
    if (name) {
        JSValue val = JS_NewStringLen(ctx, (const char *)dom_string_data(name), dom_string_byte_length(name));
        dom_string_unref(name);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_attr_value_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    dom_string *val = NULL;
    dom_attr_get_value((dom_attr *)priv->node, &val);
    if (val) {
        JSValue res = JS_NewStringLen(ctx, (const char *)dom_string_data(val), dom_string_byte_length(val));
        dom_string_unref(val);
        return res;
    }
    return JS_NULL;
}

JSValue wisp_attr_value_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    dom_string *ds; dom_string_create((const uint8_t *)value, strlen(value), &ds); dom_attr_set_value((dom_attr *)priv->node, ds); dom_string_unref(ds);
    return JS_UNDEFINED;
}

JSValue wisp_attr_specified_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_FALSE;
    bool result = false;
    dom_attr_get_specified((dom_attr *)priv->node, &result);
    return JS_NewBool(ctx, result);
}

JSValue wisp_attr_ownerElement_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    struct dom_element *el = NULL;
    dom_attr_get_owner_element((dom_attr *)priv->node, &el);
    if (el) {
        JSValue val = qjs_wrap_node(ctx, (dom_node *)el);
        dom_node_unref((dom_node *)el);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_attr_localName_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NULL; }
JSValue wisp_attr_namespaceURI_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NULL; }
JSValue wisp_attr_prefix_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NULL; }
JSValue wisp_attr_nodeValue_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return wisp_attr_value_get_impl(ctx, priv); }
JSValue wisp_attr_nodeValue_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) { return wisp_attr_value_set_impl(ctx, priv, value); }
JSValue wisp_attr_textContent_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return wisp_attr_value_get_impl(ctx, priv); }
JSValue wisp_attr_textContent_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) { return wisp_attr_value_set_impl(ctx, priv, value); }

int qjs_init_attr(JSContext *ctx) { return qjs_init_attr_gen(ctx); }
