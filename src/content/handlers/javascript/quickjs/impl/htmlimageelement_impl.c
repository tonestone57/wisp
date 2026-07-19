#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSHTMLImageElement.gen.h"

extern bool wisp_is_js_process;

JSValue wisp_htmlimageelement_Image_impl(JSContext *ctx, uint32_t width, uint32_t height)
{
    if (wisp_is_js_process) return JS_NULL;
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (!t) return JS_NULL;
    struct dom_document *doc = qjs_thread_get_document(t);
    if (!doc) return JS_NULL;

    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)"img", 3, &name_dom);
    struct dom_element *result = NULL;
    dom_document_create_element(doc, name_dom, &result);
    dom_string_unref(name_dom);

    if (result) {
        if (width > 0) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%u", width);
            dom_string *attr_name = NULL;
            dom_string_create((const uint8_t *)"width", 5, &attr_name);
            dom_string *attr_val = NULL;
            dom_string_create((const uint8_t *)buf, strlen(buf), &attr_val);
            dom_element_set_attribute(result, attr_name, attr_val);
            dom_string_unref(attr_name);
            dom_string_unref(attr_val);
        }
        if (height > 0) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%u", height);
            dom_string *attr_name = NULL;
            dom_string_create((const uint8_t *)"height", 6, &attr_name);
            dom_string *attr_val = NULL;
            dom_string_create((const uint8_t *)buf, strlen(buf), &attr_val);
            dom_element_set_attribute(result, attr_name, attr_val);
            dom_string_unref(attr_name);
            dom_string_unref(attr_val);
        }
        JSValue val = qjs_wrap_node(ctx, (dom_node *)result);
        dom_node_unref((dom_node *)result);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_htmlimageelement_src_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    dom_string *attr_name = NULL;
    dom_string_create((const uint8_t *)"src", 3, &attr_name);
    dom_string *value_dom = NULL;
    dom_element_get_attribute((dom_element *)priv->node, attr_name, &value_dom);
    dom_string_unref(attr_name);
    if (value_dom) {
        JSValue val = JS_NewStringLen(ctx, (const char *)dom_string_data(value_dom), dom_string_byte_length(value_dom));
        dom_string_unref(value_dom);
        return val;
    }
    return JS_NewString(ctx, "");
}

JSValue wisp_htmlimageelement_src_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    dom_string *attr_name = NULL;
    dom_string_create((const uint8_t *)"src", 3, &attr_name);
    dom_string *value_dom = NULL;
    dom_string_create((const uint8_t *)value, strlen(value), &value_dom);
    dom_element_set_attribute((dom_element *)priv->node, attr_name, value_dom);
    dom_string_unref(attr_name);
    dom_string_unref(value_dom);
    return JS_UNDEFINED;
}

JSValue wisp_htmlimageelement_width_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewInt32(ctx, 0);
    dom_string *attr_name = NULL;
    dom_string_create((const uint8_t *)"width", 5, &attr_name);
    dom_string *value_dom = NULL;
    dom_element_get_attribute((dom_element *)priv->node, attr_name, &value_dom);
    dom_string_unref(attr_name);
    int val = 0;
    if (value_dom) {
        val = atoi((const char *)dom_string_data(value_dom));
        dom_string_unref(value_dom);
    }
    return JS_NewInt32(ctx, val);
}

JSValue wisp_htmlimageelement_width_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", value);
    dom_string *attr_name = NULL;
    dom_string_create((const uint8_t *)"width", 5, &attr_name);
    dom_string *value_dom = NULL;
    dom_string_create((const uint8_t *)buf, strlen(buf), &value_dom);
    dom_element_set_attribute((dom_element *)priv->node, attr_name, value_dom);
    dom_string_unref(attr_name);
    dom_string_unref(value_dom);
    return JS_UNDEFINED;
}

JSValue wisp_htmlimageelement_height_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewInt32(ctx, 0);
    dom_string *attr_name = NULL;
    dom_string_create((const uint8_t *)"height", 6, &attr_name);
    dom_string *value_dom = NULL;
    dom_element_get_attribute((dom_element *)priv->node, attr_name, &value_dom);
    dom_string_unref(attr_name);
    int val = 0;
    if (value_dom) {
        val = atoi((const char *)dom_string_data(value_dom));
        dom_string_unref(value_dom);
    }
    return JS_NewInt32(ctx, val);
}

JSValue wisp_htmlimageelement_height_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", value);
    dom_string *attr_name = NULL;
    dom_string_create((const uint8_t *)"height", 6, &attr_name);
    dom_string *value_dom = NULL;
    dom_string_create((const uint8_t *)buf, strlen(buf), &value_dom);
    dom_element_set_attribute((dom_element *)priv->node, attr_name, value_dom);
    dom_string_unref(attr_name);
    dom_string_unref(value_dom);
    return JS_UNDEFINED;
}
