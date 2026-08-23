#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSHTMLScriptElement.gen.h"

extern bool wisp_is_js_process;
extern shm_dom_t *wisp_shm_dom;
extern JSValue wisp_node_textContent_get_impl(JSContext *ctx, QJSNodePrivate *priv);
extern JSValue wisp_node_textContent_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value);

JSValue wisp_htmlscriptelement_src_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) {
        JSValue val = wisp_element_getAttribute_impl(ctx, priv, "src");
        if (JS_IsNull(val) || JS_IsUndefined(val)) {
            return JS_NewString(ctx, "");
        }
        return val;
    }
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

JSValue wisp_htmlscriptelement_src_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    JSValue res = JS_UNDEFINED;
    if (wisp_is_js_process) {
        res = wisp_element_setAttribute_impl(ctx, priv, "src", value);
    } else {
        dom_string *attr_name = NULL;
        dom_string_create((const uint8_t *)"src", 3, &attr_name);
        dom_string *value_dom = NULL;
        dom_string_create((const uint8_t *)value, strlen(value), &value_dom);
        dom_element_set_attribute((dom_element *)priv->node, attr_name, value_dom);
        dom_string_unref(attr_name);
        dom_string_unref(value_dom);
    }
    bool is_connected = false;
    if (wisp_is_js_process) {
        uint64_t id = (uint64_t)(uintptr_t)priv->node;
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, id);
        if (sn && sn->parent_id != 0) is_connected = true;
    } else {
        struct dom_node *parent = NULL;
        dom_node_get_parent_node((struct dom_node *)priv->node, &parent);
        if (parent) {
            is_connected = true;
            dom_node_unref(parent);
        }
    }
    if (is_connected) {
        check_script_element_execution(ctx, priv->node);
    }
    return res;
}

JSValue wisp_htmlscriptelement_type_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) {
        JSValue val = wisp_element_getAttribute_impl(ctx, priv, "type");
        if (JS_IsNull(val) || JS_IsUndefined(val)) {
            return JS_NewString(ctx, "");
        }
        return val;
    }
    dom_string *attr_name = NULL;
    dom_string_create((const uint8_t *)"type", 4, &attr_name);
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

JSValue wisp_htmlscriptelement_type_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    JSValue res = JS_UNDEFINED;
    if (wisp_is_js_process) {
        res = wisp_element_setAttribute_impl(ctx, priv, "type", value);
    } else {
        dom_string *attr_name = NULL;
        dom_string_create((const uint8_t *)"type", 4, &attr_name);
        dom_string *value_dom = NULL;
        dom_string_create((const uint8_t *)value, strlen(value), &value_dom);
        dom_element_set_attribute((dom_element *)priv->node, attr_name, value_dom);
        dom_string_unref(attr_name);
        dom_string_unref(value_dom);
    }
    bool is_connected = false;
    if (wisp_is_js_process) {
        uint64_t id = (uint64_t)(uintptr_t)priv->node;
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, id);
        if (sn && sn->parent_id != 0) is_connected = true;
    } else {
        struct dom_node *parent = NULL;
        dom_node_get_parent_node((struct dom_node *)priv->node, &parent);
        if (parent) {
            is_connected = true;
            dom_node_unref(parent);
        }
    }
    if (is_connected) {
        check_script_element_execution(ctx, priv->node);
    }
    return res;
}

JSValue wisp_htmlscriptelement_async_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_FALSE;
    return wisp_element_hasAttribute_impl(ctx, priv, "async");
}

JSValue wisp_htmlscriptelement_async_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    if (value) {
        return wisp_element_setAttribute_impl(ctx, priv, "async", "async");
    } else {
        return wisp_element_removeAttribute_impl(ctx, priv, "async");
    }
}

JSValue wisp_htmlscriptelement_defer_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_FALSE;
    return wisp_element_hasAttribute_impl(ctx, priv, "defer");
}

JSValue wisp_htmlscriptelement_defer_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    if (value) {
        return wisp_element_setAttribute_impl(ctx, priv, "defer", "defer");
    } else {
        return wisp_element_removeAttribute_impl(ctx, priv, "defer");
    }
}

JSValue wisp_htmlscriptelement_noModule_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_FALSE;
    return wisp_element_hasAttribute_impl(ctx, priv, "nomodule");
}

JSValue wisp_htmlscriptelement_noModule_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    if (value) {
        return wisp_element_setAttribute_impl(ctx, priv, "nomodule", "");
    } else {
        return wisp_element_removeAttribute_impl(ctx, priv, "nomodule");
    }
}

JSValue wisp_htmlscriptelement_text_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_node_textContent_get_impl(ctx, priv);
}

JSValue wisp_htmlscriptelement_text_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_node_textContent_set_impl(ctx, priv, value);
}

JSValue wisp_htmlscriptelement_charset_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    return wisp_element_getAttribute_impl(ctx, priv, "charset");
}

JSValue wisp_htmlscriptelement_charset_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    return wisp_element_setAttribute_impl(ctx, priv, "charset", value);
}

JSValue wisp_htmlscriptelement_crossOrigin_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    return wisp_element_getAttribute_impl(ctx, priv, "crossorigin");
}

JSValue wisp_htmlscriptelement_crossOrigin_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    return wisp_element_setAttribute_impl(ctx, priv, "crossorigin", value);
}

JSValue wisp_htmlscriptelement_event_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    return wisp_element_getAttribute_impl(ctx, priv, "event");
}

JSValue wisp_htmlscriptelement_event_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    return wisp_element_setAttribute_impl(ctx, priv, "event", value);
}

JSValue wisp_htmlscriptelement_htmlFor_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    return wisp_element_getAttribute_impl(ctx, priv, "for");
}

JSValue wisp_htmlscriptelement_htmlFor_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    return wisp_element_setAttribute_impl(ctx, priv, "for", value);
}

JSValue wisp_htmlscriptelement_nonce_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    return wisp_element_getAttribute_impl(ctx, priv, "nonce");
}

JSValue wisp_htmlscriptelement_nonce_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    return wisp_element_setAttribute_impl(ctx, priv, "nonce", value);
}
