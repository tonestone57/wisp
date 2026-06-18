/* Implementation for Element */
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"

#include "element.inc"

static void js_element_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_element_class_id);
    if (priv) {
        qjs_bridge_remove_node(rt, (dom_node *)priv->node, priv->ctx);
        if (priv->is_dom_node && priv->node) dom_node_unref((dom_node *)priv->node);
        free(priv);
    }
static JSValue js_element_hasAttributes(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node) return JS_FALSE;
    return JS_NewBool(ctx, dom_element_has_attributes((dom_element *)priv->node));
}

static JSValue js_element_getAttribute(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node || argc < 1) return JS_NULL;
    const char *name = JS_ToCString(ctx, argv[0]);
    if (!name) return JS_NULL;
    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)name, strlen(name), &name_dom);
    JS_FreeCString(ctx, name);
    dom_string *value_dom = NULL;
    dom_exception exc = dom_element_get_attribute((dom_element *)priv->node, name_dom, &value_dom);
    dom_string_unref(name_dom);
    if (exc != DOM_NO_ERR || value_dom == NULL) return JS_NULL;
    JSValue val = JS_NewStringLen(ctx, dom_string_data(value_dom), dom_string_byte_length(value_dom));
    dom_string_unref(value_dom);
    return val;
}

static JSValue js_element_getAttributeNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.getAttributeNS() called (stub)");
    return JS_NULL;
}

static JSValue js_element_setAttribute(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node || argc < 2) return JS_UNDEFINED;
    const char *name = JS_ToCString(ctx, argv[0]);
    const char *value = JS_ToCString(ctx, argv[1]);
    if (!name || !value) {
        if (name) JS_FreeCString(ctx, name);
        if (value) JS_FreeCString(ctx, value);
        return JS_UNDEFINED;
    }
    dom_string *name_dom = NULL;
    dom_string *value_dom = NULL;
    dom_string_create((const uint8_t *)name, strlen(name), &name_dom);
    dom_string_create((const uint8_t *)value, strlen(value), &value_dom);
    JS_FreeCString(ctx, name);
    JS_FreeCString(ctx, value);
    dom_element_set_attribute((dom_element *)priv->node, name_dom, value_dom);
    dom_string_unref(name_dom);
    dom_string_unref(value_dom);
    return JS_UNDEFINED;
}

static JSValue js_element_setAttributeNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.setAttributeNS() called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_element_removeAttribute(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node || argc < 1) return JS_UNDEFINED;
    const char *name = JS_ToCString(ctx, argv[0]);
    if (!name) return JS_UNDEFINED;
    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)name, strlen(name), &name_dom);
    JS_FreeCString(ctx, name);
    dom_element_remove_attribute((dom_element *)priv->node, name_dom);
    dom_string_unref(name_dom);
    return JS_UNDEFINED;
}

static JSValue js_element_removeAttributeNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.removeAttributeNS() called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_element_hasAttribute(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node || argc < 1) return JS_FALSE;
    const char *name = JS_ToCString(ctx, argv[0]);
    if (!name) return JS_FALSE;
    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)name, strlen(name), &name_dom);
    JS_FreeCString(ctx, name);
    bool result = false;
    dom_element_has_attribute((dom_element *)priv->node, name_dom, &result);
    dom_string_unref(name_dom);
    return JS_NewBool(ctx, result);
}

static JSValue js_element_hasAttributeNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.hasAttributeNS() called (stub)");
    return JS_FALSE;
}

static JSValue js_element_getAttributeNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.getAttributeNode() called (stub)");
    return JS_NULL;
}

static JSValue js_element_getAttributeNodeNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.getAttributeNodeNS() called (stub)");
    return JS_NULL;
}

static JSValue js_element_setAttributeNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.setAttributeNode() called (stub)");
    return JS_NULL;
}

static JSValue js_element_setAttributeNodeNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.setAttributeNodeNS() called (stub)");
    return JS_NULL;
}

static JSValue js_element_removeAttributeNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.removeAttributeNode() called (stub)");
    return JS_NULL;
}

static JSValue js_element_closest(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.closest() called (stub)");
    return JS_NULL;
}

static JSValue js_element_matches(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.matches() called (stub)");
    return JS_FALSE;
}

static JSValue js_element_getElementsByTagName(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.getElementsByTagName() called (stub)");
    return JS_NewArray(ctx);
}

static JSValue js_element_getElementsByTagNameNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.getElementsByTagNameNS() called (stub)");
    return JS_NewArray(ctx);
}

static JSValue js_element_getElementsByClassName(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.getElementsByClassName() called (stub)");
    return JS_NewArray(ctx);
}

static JSValue js_element_prepend(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.prepend() called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_element_append(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.append() called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_element_query(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.query() called (stub)");
    return JS_NULL;
}

static JSValue js_element_queryAll(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.queryAll() called (stub)");
    return JS_NewArray(ctx);
}

static JSValue js_element_querySelector(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.querySelector() called (stub)");
    return JS_NULL;
}

static JSValue js_element_querySelectorAll(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.querySelectorAll() called (stub)");
    return JS_NewArray(ctx);
}

static JSValue js_element_before(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.before() called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_element_after(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.after() called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_element_replaceWith(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.replaceWith() called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_element_remove(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node) return JS_UNDEFINED;
    struct dom_node *parent = NULL;
    dom_node_get_parent_node((dom_node *)priv->node, &parent);
    if (parent) {
        dom_node_remove_child(parent, (dom_node *)priv->node, NULL);
        dom_node_unref(parent);
    }
    return JS_UNDEFINED;
}

static JSValue js_element_namespaceURI_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Element.namespaceURI getter called (stub)");
    return JS_NULL;
}

static JSValue js_element_prefix_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Element.prefix getter called (stub)");
    return JS_NULL;
}

static JSValue js_element_localName_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node) return JS_UNDEFINED;
    dom_string *name = NULL;
    dom_element_get_tag_name((dom_element *)priv->node, &name);
    if (name) {
        JSValue val = JS_NewStringLen(ctx, dom_string_data(name), dom_string_byte_length(name));
        dom_string_unref(name);
        return val;
    }
    return JS_NULL;
}

static JSValue js_element_tagName_get(JSContext *ctx, JSValueConst this_val)
{
    return js_element_localName_get(ctx, this_val);
}

static JSValue js_element_id_get(JSContext *ctx, JSValueConst this_val)
{
    JSValue id_str = JS_NewString(ctx, "id");
    JSValue val = js_element_getAttribute(ctx, this_val, 1, &id_str);
    JS_FreeValue(ctx, id_str);
    if (JS_IsNull(val)) return JS_NewString(ctx, "");
    return val;
}

static JSValue js_element_id_set(JSContext *ctx, JSValueConst this_val, JSValueConst val)
{
    JSValue args[2] = { JS_NewString(ctx, "id"), JS_DupValue(ctx, val) };
    js_element_setAttribute(ctx, this_val, 2, args);
    JS_FreeValue(ctx, args[0]);
    JS_FreeValue(ctx, args[1]);
    return JS_UNDEFINED;
}

static JSValue js_element_className_get(JSContext *ctx, JSValueConst this_val)
{
    JSValue class_str = JS_NewString(ctx, "class");
    JSValue val = js_element_getAttribute(ctx, this_val, 1, &class_str);
    JS_FreeValue(ctx, class_str);
    if (JS_IsNull(val)) return JS_NewString(ctx, "");
    return val;
}

static JSValue js_element_className_set(JSContext *ctx, JSValueConst this_val, JSValueConst val)
{
    JSValue args[2] = { JS_NewString(ctx, "class"), JS_DupValue(ctx, val) };
    js_element_setAttribute(ctx, this_val, 2, args);
    JS_FreeValue(ctx, args[0]);
    JS_FreeValue(ctx, args[1]);
    return JS_UNDEFINED;
}

static JSValue js_element_classList_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Element.classList getter called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_element_attributes_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Element.attributes getter called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_element_children_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Element.children getter called (stub)");
    return JS_NewArray(ctx);
}

static JSValue js_element_firstElementChild_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node) return JS_NULL;
    struct dom_node *child = NULL;
    dom_node_get_first_child((dom_node *)priv->node, &child);
    while (child) {
        dom_node_type type;
        dom_node_get_node_type(child, &type);
        if (type == DOM_ELEMENT_NODE) {
            JSValue val = qjs_wrap_node(ctx, child);
            dom_node_unref(child);
            return val;
        }
        struct dom_node *next = NULL;
        dom_node_get_next_sibling(child, &next);
        dom_node_unref(child);
        child = next;
    }
    return JS_NULL;
}

static JSValue js_element_lastElementChild_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Element.lastElementChild getter called (stub)");
    return JS_NULL;
}

static JSValue js_element_childElementCount_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Element.childElementCount getter called (stub)");
    return JS_NewInt32(ctx, 0);
}

static JSValue js_element_previousElementSibling_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Element.previousElementSibling getter called (stub)");
    return JS_NULL;
}

static JSValue js_element_nextElementSibling_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Element.nextElementSibling getter called (stub)");
    return JS_NULL;
}

int qjs_init_element(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_element_class_id == 0) JS_NewClassID(rt, &qjs_element_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_element_class_id)) JS_NewClass(rt, qjs_element_class_id, &js_element_class);
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_element_proto_funcs, sizeof(js_element_proto_funcs) / sizeof(js_element_proto_funcs[0]));
    JS_SetClassProto(ctx, qjs_element_class_id, proto);
    return 0;
}

JSValue qjs_new_element(JSContext *ctx, void *node, bool is_dom_node)
{
    JSValue obj = JS_NewObjectClass(ctx, qjs_element_class_id);
    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));
    if (!priv) return JS_ThrowOutOfMemory(ctx);
    priv->node = node; priv->ctx = ctx; priv->is_dom_node = is_dom_node;
    if (is_dom_node && node) dom_node_ref((dom_node *)node);
    JS_SetOpaque(obj, priv); return obj;
}
