#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "utils/corestrings.h"
#include "JSElement.gen.h"

JSValue wisp_element_getAttribute_impl(JSContext *ctx, QJSNodePrivate *priv, const char * qualifiedName)
{
    if (!priv || !priv->node) return JS_NULL;
    if (!qualifiedName) return JS_ThrowTypeError(ctx, "qualifiedName is null");
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
    if (!qualifiedName || !value) return JS_ThrowTypeError(ctx, "Argument is null");
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
    if (!qualifiedName) return JS_ThrowTypeError(ctx, "qualifiedName is null");
    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)qualifiedName, strlen(qualifiedName), &name_dom);
    dom_element_remove_attribute((dom_element *)priv->node, name_dom);
    dom_string_unref(name_dom);
    return JS_UNDEFINED;
}

JSValue wisp_element_hasAttribute_impl(JSContext *ctx, QJSNodePrivate *priv, const char * qualifiedName)
{
    if (!priv || !priv->node) return JS_FALSE;
    if (!qualifiedName) return JS_ThrowTypeError(ctx, "qualifiedName is null");
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
JSValue wisp_element_innerHTML_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    dom_node *element = (dom_node *)priv->node;
    dom_document *doc = NULL;
    dom_exception exc = dom_node_get_owner_document(element, &doc);
    if (exc != DOM_NO_ERR || !doc) return JS_ThrowInternalError(ctx, "Failed to get owner document");

    /* 1. Clear existing children */
    dom_node *child = NULL;
    while (dom_node_get_first_child(element, &child) == DOM_NO_ERR && child != NULL) {
        dom_node *removed = NULL;
        dom_node_remove_child(element, child, &removed);
        if (removed) dom_node_unref(removed);
        dom_node_unref(child);
        child = NULL;
    }

    /* 2. Parse new HTML string using Hubbub fragment parser */
    dom_hubbub_parser_params params;
    memset(&params, 0, sizeof(params));
    params.enc = "UTF-8";
    params.idname = corestring_dom_id;

    dom_hubbub_parser *parser = NULL;
    dom_document_fragment *fragment = NULL;
    dom_hubbub_error err = dom_hubbub_fragment_parser_create(&params, doc, &parser, &fragment);
    if (err != DOM_HUBBUB_OK) {
        dom_node_unref((dom_node *)doc);
        return JS_ThrowInternalError(ctx, "Failed to create Hubbub fragment parser");
    }

    err = dom_hubbub_parser_parse_chunk(parser, (const uint8_t *)value, strlen(value));
    if (err == DOM_HUBBUB_OK) {
        err = dom_hubbub_parser_completed(parser);
    }

    if (err == DOM_HUBBUB_OK && fragment != NULL) {
        /* 3. Append children from fragment to element */
        dom_node *f_child = NULL;
        while (dom_node_get_first_child((dom_node *)fragment, &f_child) == DOM_NO_ERR && f_child != NULL) {
            dom_node *result = NULL;
            /* dom_node_append_child on a fragment moves nodes from the fragment to the element */
            dom_node_append_child(element, f_child, &result);
            if (result) dom_node_unref(result);
            dom_node_unref(f_child);
            f_child = NULL;
        }
    }

    if (fragment) dom_node_unref((dom_node *)fragment);
    dom_hubbub_parser_destroy(parser);
    dom_node_unref((dom_node *)doc);

    if (err != DOM_HUBBUB_OK) return JS_ThrowInternalError(ctx, "Hubbub parsing failed");

    return JS_UNDEFINED;
}
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

JSValue wisp_element_style_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    JSValue wrapper = qjs_wrap_node(ctx, (dom_node *)priv->node);
    if (JS_IsObject(wrapper)) {
        JSValue style = JS_GetPropertyStr(ctx, wrapper, "__wisp_style_cached");
        if (JS_IsUndefined(style)) {
            style = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, wrapper, "__wisp_style_cached", JS_DupValue(ctx, style));
        }
        JS_FreeValue(ctx, wrapper);
        return style;
    }
    return JS_NewObject(ctx);
}

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
    JSValue global_obj = JS_GetGlobalObject(ctx);

    /* Check if already initialized on this global object */
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_element_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    /* Initialize the class and prototype using the generated function */
    qjs_init_element_gen(ctx);

    JSValue proto = JS_GetClassProto(ctx, qjs_element_class_id);
    if (!JS_IsObject(proto)) {
        JS_FreeValue(ctx, proto);
        proto = JS_NewObject(ctx);
        JS_SetClassProto(ctx, qjs_element_class_id, JS_DupValue(ctx, proto));
    }
    JSValue node_proto = JS_GetClassProto(ctx, qjs_node_class_id);
    if (JS_IsObject(proto) && JS_IsObject(node_proto)) JS_SetPrototype(ctx, proto, node_proto);
    JS_FreeValue(ctx, node_proto);
    JS_FreeValue(ctx, proto);

    /* Mark as initialized */
    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_element_init", JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);

    return 0;
}
