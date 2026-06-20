/* Implementation for Node */
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"

JSClassID qjs_node_class_id;

static void js_node_finalizer(JSRuntime *rt, JSValue val);

#include "node.inc"

static void js_node_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_node_class_id);
    if (priv) {
        qjs_bridge_remove_node(rt, (dom_node *)priv->node, priv->ctx);
        if (priv->is_dom_node && priv->node) dom_node_unref((dom_node *)priv->node);
        free(priv);
    }
}
static QJSNodePrivate *qjs_get_node_priv(JSContext *ctx, JSValueConst val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_node_class_id);
    if (priv) return priv;
    priv = JS_GetOpaque(val, qjs_element_class_id);
    if (priv) return priv;
    priv = JS_GetOpaque(val, qjs_text_class_id);
    if (priv) return priv;
    priv = JS_GetOpaque(val, qjs_document_class_id);
    return priv;
}

static JSValue js_node_hasChildNodes(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = qjs_get_node_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_FALSE;
    bool result = false;
    dom_node_has_child_nodes((dom_node *)priv->node, &result);
    return JS_NewBool(ctx, result);
}

static JSValue js_node_normalize(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = qjs_get_node_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    dom_node_normalize((dom_node *)priv->node);
    return JS_UNDEFINED;
}

static JSValue js_node_cloneNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = qjs_get_node_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_EXCEPTION;
    bool deep = false;
    if (argc > 0) deep = JS_ToBool(ctx, argv[0]);
    struct dom_node *result = NULL;
    dom_exception exc = dom_node_clone_node((dom_node *)priv->node, deep, &result);
    if (exc != DOM_NO_ERR || result == NULL) return JS_NULL;
    JSValue val = qjs_wrap_node(ctx, result);
    dom_node_unref(result);
    return val;
}

static JSValue js_node_isEqualNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = qjs_get_node_priv(ctx, this_val);
    if (!priv || !priv->node || argc < 1) return JS_FALSE;
    QJSNodePrivate *other_priv = qjs_get_node_priv(ctx, argv[0]);
    if (!other_priv || !other_priv->node) return JS_FALSE;
    bool result = false;
    dom_node_is_equal_node((dom_node *)priv->node, (dom_node *)other_priv->node, &result);
    return JS_NewBool(ctx, result);
}

static JSValue js_node_compareDocumentPosition(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Node.compareDocumentPosition() called (stub)");
    return JS_NewInt32(ctx, 0);
}

static JSValue js_node_contains(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = qjs_get_node_priv(ctx, this_val);
    if (!priv || !priv->node || argc < 1) return JS_FALSE;
    QJSNodePrivate *other_priv = qjs_get_node_priv(ctx, argv[0]);
    if (!other_priv || !other_priv->node) return JS_FALSE;
    bool result = false;
    dom_node_contains((dom_node *)priv->node, (dom_node *)other_priv->node, &result);
    return JS_NewBool(ctx, result);
}

static JSValue js_node_lookupPrefix(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Node.lookupPrefix() called (stub)");
    return JS_NULL;
}

static JSValue js_node_lookupNamespaceURI(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Node.lookupNamespaceURI() called (stub)");
    return JS_NULL;
}

static JSValue js_node_isDefaultNamespace(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Node.isDefaultNamespace() called (stub)");
    return JS_FALSE;
}

static JSValue js_node_insertBefore(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = qjs_get_node_priv(ctx, this_val);
    if (!priv || !priv->node || argc < 1) return JS_EXCEPTION;
    QJSNodePrivate *new_priv = qjs_get_node_priv(ctx, argv[0]);
    if (!new_priv || !new_priv->node) return JS_ThrowTypeError(ctx, "Argument 1 is not a Node");
    struct dom_node *ref_node = NULL;
    if (argc >= 2 && !JS_IsNull(argv[1])) {
        QJSNodePrivate *ref_priv = qjs_get_node_priv(ctx, argv[1]);
        if (ref_priv) ref_node = (dom_node *)ref_priv->node;
    }
    struct dom_node *result = NULL;
    dom_exception exc = dom_node_insert_before((dom_node *)priv->node, (dom_node *)new_priv->node, ref_node, &result);
    if (exc != DOM_NO_ERR) return JS_ThrowInternalError(ctx, "dom_node_insert_before failed");
    if (result) dom_node_unref(result);
    return JS_DupValue(ctx, argv[0]);
}

static JSValue js_node_appendChild(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = qjs_get_node_priv(ctx, this_val);
    if (!priv || !priv->node || argc < 1) return JS_EXCEPTION;
    QJSNodePrivate *child_priv = qjs_get_node_priv(ctx, argv[0]);
    if (!child_priv || !child_priv->node) return JS_ThrowTypeError(ctx, "Argument is not a Node");
    struct dom_node *result = NULL;
    dom_exception exc = dom_node_append_child((dom_node *)priv->node, (dom_node *)child_priv->node, &result);
    if (exc != DOM_NO_ERR) return JS_ThrowInternalError(ctx, "dom_node_append_child failed");
    if (result) dom_node_unref(result);
    return JS_DupValue(ctx, argv[0]);
}

static JSValue js_node_replaceChild(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = qjs_get_node_priv(ctx, this_val);
    if (!priv || !priv->node || argc < 2) return JS_EXCEPTION;
    QJSNodePrivate *new_priv = qjs_get_node_priv(ctx, argv[0]);
    QJSNodePrivate *old_priv = qjs_get_node_priv(ctx, argv[1]);
    if (!new_priv || !old_priv) return JS_ThrowTypeError(ctx, "Arguments must be Nodes");
    struct dom_node *result = NULL;
    dom_exception exc = dom_node_replace_child((dom_node *)priv->node, (dom_node *)new_priv->node, (dom_node *)old_priv->node, &result);
    if (exc != DOM_NO_ERR) return JS_ThrowInternalError(ctx, "dom_node_replace_child failed");
    if (result) dom_node_unref(result);
    return JS_DupValue(ctx, argv[1]);
}

static JSValue js_node_removeChild(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = qjs_get_node_priv(ctx, this_val);
    if (!priv || !priv->node || argc < 1) return JS_EXCEPTION;
    QJSNodePrivate *child_priv = qjs_get_node_priv(ctx, argv[0]);
    if (!child_priv || !child_priv->node) return JS_ThrowTypeError(ctx, "Argument is not a Node");
    struct dom_node *result = NULL;
    dom_exception exc = dom_node_remove_child((dom_node *)priv->node, (dom_node *)child_priv->node, &result);
    if (exc != DOM_NO_ERR) return JS_ThrowInternalError(ctx, "dom_node_remove_child failed");
    if (result) dom_node_unref(result);
    return JS_DupValue(ctx, argv[0]);
}

static JSValue js_node_nodeType_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = qjs_get_node_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    dom_node_type type;
    dom_node_get_node_type((dom_node *)priv->node, &type);
    return JS_NewInt32(ctx, type);
}

static JSValue js_node_nodeName_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = qjs_get_node_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    dom_string *name = NULL;
    dom_node_get_node_name((dom_node *)priv->node, &name);
    if (name) {
        JSValue val = JS_NewStringLen(ctx, dom_string_data(name), dom_string_byte_length(name));
        dom_string_unref(name);
        return val;
    }
    return JS_NULL;
}

static JSValue js_node_baseURI_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Node.baseURI getter called (stub)");
    return JS_NULL;
}

static JSValue js_node_ownerDocument_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = qjs_get_node_priv(ctx, this_val);
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

static JSValue js_node_parentNode_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = qjs_get_node_priv(ctx, this_val);
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

static JSValue js_node_parentElement_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = qjs_get_node_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    struct dom_element *parent = NULL;
    dom_node_get_parent_element((dom_node *)priv->node, &parent);
    if (parent) {
        JSValue val = qjs_wrap_node(ctx, (dom_node *)parent);
        dom_node_unref((dom_node *)parent);
        return val;
    }
    return JS_NULL;
}

static JSValue js_node_childNodes_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Node.childNodes getter called (stub)");
    return JS_NewArray(ctx);
}

static JSValue js_node_firstChild_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = qjs_get_node_priv(ctx, this_val);
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

static JSValue js_node_lastChild_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = qjs_get_node_priv(ctx, this_val);
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

static JSValue js_node_previousSibling_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = qjs_get_node_priv(ctx, this_val);
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

static JSValue js_node_nextSibling_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = qjs_get_node_priv(ctx, this_val);
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

static JSValue js_node_nodeValue_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = qjs_get_node_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    dom_string *val = NULL;
    dom_node_get_node_value((dom_node *)priv->node, &val);
    if (val) {
        JSValue res = JS_NewStringLen(ctx, dom_string_data(val), dom_string_byte_length(val));
        dom_string_unref(val);
        return res;
    }
    return JS_NULL;
}

static JSValue js_node_nodeValue_set(JSContext *ctx, JSValueConst this_val, JSValueConst val)
{
    QJSNodePrivate *priv = qjs_get_node_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    const char *str = JS_ToCString(ctx, val);
    if (str) {
        dom_string *dstr = NULL;
        dom_string_create((const uint8_t *)str, strlen(str), &dstr);
        dom_node_set_node_value((dom_node *)priv->node, dstr);
        dom_string_unref(dstr);
        JS_FreeCString(ctx, str);
    }
    return JS_UNDEFINED;
}

static JSValue js_node_textContent_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = qjs_get_node_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    dom_string *text = NULL;
    dom_node_get_text_content((dom_node *)priv->node, &text);
    if (text) {
        JSValue val = JS_NewStringLen(ctx, dom_string_data(text), dom_string_byte_length(text));
        dom_string_unref(text);
        return val;
    }
    return JS_NULL;
}

static JSValue js_node_textContent_set(JSContext *ctx, JSValueConst this_val, JSValueConst val)
{
    QJSNodePrivate *priv = qjs_get_node_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    const char *str = JS_ToCString(ctx, val);
    if (str) {
        dom_string *dstr = NULL;
        dom_string_create((const uint8_t *)str, strlen(str), &dstr);
        dom_node_set_text_content((dom_node *)priv->node, dstr);
        dom_string_unref(dstr);
        JS_FreeCString(ctx, str);
    }
    return JS_UNDEFINED;
}

int qjs_init_node(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_node_class_id == 0) JS_NewClassID(rt, &qjs_node_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_node_class_id)) JS_NewClass(rt, qjs_node_class_id, &js_node_class);
    JSValue proto = JS_NewObject(ctx);

    JSValue parent_proto = JS_GetClassProto(ctx, qjs_eventtarget_class_id);
    JS_SetPrototype(ctx, proto, parent_proto);
    JS_FreeValue(ctx, parent_proto);

    JS_SetPropertyFunctionList(ctx, proto, js_node_proto_funcs, sizeof(js_node_proto_funcs) / sizeof(js_node_proto_funcs[0]));
    JS_SetClassProto(ctx, qjs_node_class_id, proto);
    return 0;
}

JSValue qjs_new_node(JSContext *ctx, void *node, bool is_dom_node)
{
    JSValue obj = JS_NewObjectClass(ctx, qjs_node_class_id);
    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));
    if (!priv) return JS_ThrowOutOfMemory(ctx);
    priv->node = node; priv->ctx = ctx; priv->is_dom_node = is_dom_node;
    if (is_dom_node && node) dom_node_ref((dom_node *)node);
    JS_SetOpaque(obj, priv); return obj;
}
