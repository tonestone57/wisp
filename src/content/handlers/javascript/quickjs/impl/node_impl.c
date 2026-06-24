#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "wisp/content/handlers/html/box.h"
#include "wisp/content/handlers/html/private.h"
#include "content/handlers/html/box_construct.h"
#include "content/handlers/html/box_manipulate.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "generated_bindings.h"

JSValue wisp_node_hasChildNodes_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_FALSE;
    bool result = false;
    dom_node_has_child_nodes((dom_node *)priv->node, &result);
    return JS_NewBool(ctx, result);
}

JSValue wisp_node_normalize_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    dom_node_normalize((dom_node *)priv->node);
    return JS_UNDEFINED;
}

JSValue wisp_node_cloneNode_impl(JSContext *ctx, QJSNodePrivate *priv, bool deep)
{
    if (!priv || !priv->node) return JS_EXCEPTION;
    struct dom_node *result = NULL;
    dom_exception exc = dom_node_clone_node((dom_node *)priv->node, deep, &result);
    if (exc != DOM_NO_ERR || result == NULL) return JS_NULL;
    JSValue val = qjs_wrap_node(ctx, result);
    dom_node_unref(result);
    return val;
}

JSValue wisp_node_isEqualNode_impl(JSContext *ctx, QJSNodePrivate *priv, void * otherNode)
{
    if (!priv || !priv->node || !otherNode) return JS_FALSE;
    bool result = false;
    dom_node_is_equal((dom_node *)priv->node, (dom_node *)otherNode, &result);
    return JS_NewBool(ctx, result);
}

JSValue wisp_node_compareDocumentPosition_impl(JSContext *ctx, QJSNodePrivate *priv, void * other)
{
    if (!priv || !priv->node || !other) return JS_NewInt32(ctx, 0);
    uint16_t result = 0;
    dom_node_compare_document_position((dom_node *)priv->node, (dom_node *)other, &result);
    return JS_NewInt32(ctx, result);
}

JSValue wisp_node_contains_impl(JSContext *ctx, QJSNodePrivate *priv, void * other)
{
    if (!priv || !priv->node || !other) return JS_FALSE;
    bool result = false;
    dom_node_contains((dom_node *)priv->node, (dom_node *)other, &result);
    return JS_NewBool(ctx, result);
}

JSValue wisp_node_lookupPrefix_impl(JSContext *ctx, QJSNodePrivate *priv, const char * namespace)
{
    NSLOG(wisp, DEBUG, "Node.lookupPrefix() called (stub)");
    return JS_NULL;
}

JSValue wisp_node_lookupNamespaceURI_impl(JSContext *ctx, QJSNodePrivate *priv, const char * namespaceURI)
{
    NSLOG(wisp, DEBUG, "Node.lookupNamespaceURI() called (stub)");
    return JS_NULL;
}

JSValue wisp_node_isDefaultNamespace_impl(JSContext *ctx, QJSNodePrivate *priv, const char * namespace)
{
    NSLOG(wisp, DEBUG, "Node.isDefaultNamespace() called (stub)");
    return JS_FALSE;
}

JSValue wisp_node_insertBefore_impl(JSContext *ctx, QJSNodePrivate *priv, void * node, void * child)
{
    if (!priv || !priv->node || !node) return JS_EXCEPTION;
    struct dom_node *result = NULL;
    dom_exception exc = dom_node_insert_before((dom_node *)priv->node, (dom_node *)node, (dom_node *)child, &result);
    if (exc != DOM_NO_ERR) return JS_ThrowInternalError(ctx, "dom_node_insert_before failed");

    if (result) dom_node_unref(result);
    return qjs_wrap_node(ctx, (dom_node *)node);
}

JSValue wisp_node_appendChild_impl(JSContext *ctx, QJSNodePrivate *priv, void * node)
{
    if (!priv || !priv->node || !node) return JS_EXCEPTION;
    struct dom_node *result = NULL;
    dom_exception exc = dom_node_append_child((dom_node *)priv->node, (dom_node *)node, &result);
    if (exc != DOM_NO_ERR) return JS_ThrowInternalError(ctx, "dom_node_append_child failed");

    if (result) dom_node_unref(result);
    return qjs_wrap_node(ctx, (dom_node *)node);
}

JSValue wisp_node_replaceChild_impl(JSContext *ctx, QJSNodePrivate *priv, void * node, void * child)
{
    if (!priv || !priv->node || !node || !child) return JS_EXCEPTION;
    struct dom_node *result = NULL;
    dom_exception exc = dom_node_replace_child((dom_node *)priv->node, (dom_node *)node, (dom_node *)child, &result);
    if (exc != DOM_NO_ERR) return JS_ThrowInternalError(ctx, "dom_node_replace_child failed");

    if (result) dom_node_unref(result);
    return qjs_wrap_node(ctx, (dom_node *)child);
}

JSValue wisp_node_removeChild_impl(JSContext *ctx, QJSNodePrivate *priv, void * child)
{
    if (!priv || !priv->node || !child) return JS_EXCEPTION;
    struct dom_node *result = NULL;
    dom_exception exc = dom_node_remove_child((dom_node *)priv->node, (dom_node *)child, &result);
    if (exc != DOM_NO_ERR) return JS_ThrowInternalError(ctx, "dom_node_remove_child failed");

    if (result) dom_node_unref(result);
    return qjs_wrap_node(ctx, (dom_node *)child);
}

JSValue wisp_node_nodeType_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    dom_node_type type;
    dom_node_get_node_type((dom_node *)priv->node, &type);
    return JS_NewInt32(ctx, type);
}

JSValue wisp_node_nodeName_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    dom_string *name = NULL;
    dom_node_get_node_name((dom_node *)priv->node, &name);
    if (name) {
        JSValue val = JS_NewStringLen(ctx, (const char *)dom_string_data(name), dom_string_byte_length(name));
        dom_string_unref(name);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_node_baseURI_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    NSLOG(wisp, DEBUG, "Node.baseURI getter called (stub)");
    return JS_NULL;
}

JSValue wisp_node_ownerDocument_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    struct dom_document *doc = NULL;
    dom_node_get_owner_document((dom_node *)priv->node, &doc);
    if (doc) {
        JSValue val = qjs_wrap_node(ctx, (dom_node *)doc);
        dom_node_unref((dom_node *)doc);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_node_parentNode_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    struct dom_node *parent = NULL;
    dom_node_get_parent_node((dom_node *)priv->node, &parent);
    if (parent) {
        JSValue val = qjs_wrap_node(ctx, parent);
        dom_node_unref(parent);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_node_parentElement_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    struct dom_node *parent = NULL;
    dom_node_get_parent_node((dom_node *)priv->node, &parent);
    while (parent) {
        dom_node_type type;
        dom_node_get_node_type(parent, &type);
        if (type == DOM_ELEMENT_NODE) {
            JSValue val = qjs_wrap_node(ctx, parent);
            dom_node_unref(parent);
            return val;
        }
        struct dom_node *next_parent = NULL;
        dom_node_get_parent_node(parent, &next_parent);
        dom_node_unref(parent);
        parent = next_parent;
    }
    return JS_NULL;
}

JSValue wisp_node_childNodes_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    NSLOG(wisp, DEBUG, "Node.childNodes getter called (stub)");
    return JS_NewArray(ctx);
}

JSValue wisp_node_firstChild_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    struct dom_node *child = NULL;
    dom_node_get_first_child((dom_node *)priv->node, &child);
    if (child) {
        JSValue val = qjs_wrap_node(ctx, child);
        dom_node_unref(child);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_node_lastChild_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    struct dom_node *child = NULL;
    dom_node_get_last_child((dom_node *)priv->node, &child);
    if (child) {
        JSValue val = qjs_wrap_node(ctx, child);
        dom_node_unref(child);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_node_previousSibling_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    struct dom_node *sibling = NULL;
    dom_node_get_previous_sibling((dom_node *)priv->node, &sibling);
    if (sibling) {
        JSValue val = qjs_wrap_node(ctx, sibling);
        dom_node_unref(sibling);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_node_nextSibling_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    struct dom_node *sibling = NULL;
    dom_node_get_next_sibling((dom_node *)priv->node, &sibling);
    if (sibling) {
        JSValue val = qjs_wrap_node(ctx, sibling);
        dom_node_unref(sibling);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_node_nodeValue_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    dom_string *val = NULL;
    dom_node_get_node_value((dom_node *)priv->node, &val);
    if (val) {
        JSValue res = JS_NewStringLen(ctx, (const char *)dom_string_data(val), dom_string_byte_length(val));
        dom_string_unref(val);
        return res;
    }
    return JS_NULL;
}

JSValue wisp_node_nodeValue_set_impl(JSContext *ctx, QJSNodePrivate *priv, void * value)
{
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    dom_node_set_node_value((dom_node *)priv->node, (dom_string *)value);
    return JS_UNDEFINED;
}

JSValue wisp_node_textContent_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    dom_string *text = NULL;
    dom_node_get_text_content((dom_node *)priv->node, &text);
    if (text) {
        JSValue val = JS_NewStringLen(ctx, (const char *)dom_string_data(text), dom_string_byte_length(text));
        dom_string_unref(text);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_node_textContent_set_impl(JSContext *ctx, QJSNodePrivate *priv, void * value)
{
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    dom_node_set_text_content((dom_node *)priv->node, (dom_string *)value);
    return JS_UNDEFINED;
}

int qjs_init_node(JSContext *ctx) {
    qjs_init_node_gen(ctx);
    JSValue proto = JS_GetClassProto(ctx, qjs_node_class_id);
    JSValue et_proto = JS_GetClassProto(ctx, qjs_eventtarget_class_id);
    if (JS_IsObject(proto) && JS_IsObject(et_proto)) JS_SetPrototype(ctx, proto, et_proto);
    JS_FreeValue(ctx, et_proto);
    JS_FreeValue(ctx, proto);
    return 0;
}
