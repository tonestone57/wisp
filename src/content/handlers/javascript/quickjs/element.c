/* Implementation for Element */
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "content/handlers/html/box_construct.h"
#include "content/handlers/html/box_manipulate.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include <dom/html/html_element.h>
#include <dom/core/node.h>
JSClassID qjs_element_class_id;

static void js_element_finalizer(JSRuntime *rt, JSValue val);

static void js_element_finalizer(JSRuntime *rt, JSValue val);

static void js_element_finalizer(JSRuntime *rt, JSValue val);

#include "element.inc"
static void js_element_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_element_class_id);
    if (priv) {
        qjs_bridge_remove_node(rt, (dom_node *)priv->node, priv->ctx);
        if (priv->is_dom_node && priv->node) dom_node_unref((dom_node *)priv->node);
        free(priv);
    }
}

static JSValue js_element_hasAttributes(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node) return JS_FALSE;
    bool result = false;
    dom_node_has_attributes((dom_node *)priv->node, &result);
    return JS_NewBool(ctx, result);
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
    { struct box *b = box_for_node((dom_node *)priv->node); if (b) box_mark_dirty(b); }
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
    { struct box *b = box_for_node((dom_node *)priv->node); if (b) box_mark_dirty(b); }
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
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node || argc < 1) return JS_NULL;
    const char *name = JS_ToCString(ctx, argv[0]);
    if (!name) return JS_NULL;
    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)name, strlen(name), &name_dom);
    JS_FreeCString(ctx, name);
    struct dom_attr *result = NULL;
    dom_exception exc = dom_element_get_attribute_node((dom_element *)priv->node, name_dom, &result);
    dom_string_unref(name_dom);
    if (exc != DOM_NO_ERR || result == NULL) return JS_NULL;
    JSValue val = qjs_wrap_node(ctx, (dom_node *)result);
    dom_node_unref((dom_node *)result);
    return val;
}

static JSValue js_element_getAttributeNodeNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.getAttributeNodeNS() called (stub)");
    return JS_NULL;
}

static JSValue js_element_setAttributeNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node || argc < 1) return JS_NULL;
    QJSNodePrivate *attr_priv = JS_GetOpaque(argv[0], qjs_attr_class_id);
    if (!attr_priv || !attr_priv->node) return JS_ThrowTypeError(ctx, "Argument is not an Attr");
    struct dom_attr *old_attr = NULL;
    dom_exception exc = dom_element_set_attribute_node((dom_element *)priv->node, (dom_attr *)attr_priv->node, &old_attr);
    if (exc != DOM_NO_ERR) return JS_ThrowInternalError(ctx, "dom_element_set_attribute_node failed");
    if (old_attr) {
        JSValue val = qjs_wrap_node(ctx, (dom_node *)old_attr);
        dom_node_unref((dom_node *)old_attr);
        return val;
    }
    return JS_NULL;
}

static JSValue js_element_setAttributeNodeNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.setAttributeNodeNS() called (stub)");
    return JS_NULL;
}

static JSValue js_element_removeAttributeNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node || argc < 1) return JS_NULL;
    QJSNodePrivate *attr_priv = JS_GetOpaque(argv[0], qjs_attr_class_id);
    if (!attr_priv || !attr_priv->node) return JS_ThrowTypeError(ctx, "Argument is not an Attr");
    struct dom_attr *old_attr = NULL;
    dom_exception exc = dom_element_remove_attribute_node((dom_element *)priv->node, (dom_attr *)attr_priv->node, &old_attr);
    if (exc != DOM_NO_ERR) return JS_ThrowInternalError(ctx, "dom_element_remove_attribute_node failed");
    if (old_attr) {
        JSValue val = qjs_wrap_node(ctx, (dom_node *)old_attr);
        dom_node_unref((dom_node *)old_attr);
        return val;
    }
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
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node || argc < 1) return JS_NewArray(ctx);

    const char *tag = JS_ToCString(ctx, argv[0]);
    if (!tag) return JS_NewArray(ctx);

    dom_string *tag_dom = NULL;
    dom_string_create((const uint8_t *)tag, strlen(tag), &tag_dom);
    JS_FreeCString(ctx, tag);

    struct dom_nodelist *list = NULL;
    dom_exception exc = dom_element_get_elements_by_tag_name((dom_element *)priv->node, tag_dom, &list);
    dom_string_unref(tag_dom);

    if (exc != DOM_NO_ERR || list == NULL) return JS_NewArray(ctx);

    uint32_t len = 0;
    dom_nodelist_get_length(list, &len);

    JSValue arr = JS_NewArray(ctx);
    for (uint32_t i = 0; i < len; i++) {
        struct dom_node *node = NULL;
        dom_nodelist_item(list, i, &node);
        if (node) {
            JS_SetPropertyUint32(ctx, arr, i, qjs_wrap_node(ctx, node));
            dom_node_unref(node);
        }
    }
    dom_nodelist_unref(list);
    return arr;
}

static JSValue js_element_getElementsByTagNameNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.getElementsByTagNameNS() called (stub)");
    return JS_NewArray(ctx);
}

static JSValue js_element_getElementsByClassName(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    /* LibDOM 0.9.x does not have dom_element_get_elements_by_class_name.
       Stubbing it to return an empty array for now. */
    NSLOG(wisp, DEBUG, "Element.getElementsByClassName() called (not supported in current LibDOM)");
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
        { struct box *b = box_for_node(parent); if (b) box_mark_dirty(b); }
        dom_node_unref(parent);
    }
    return JS_UNDEFINED;
}

static JSValue js_element_namespaceURI_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node) return JS_NULL;
    dom_string *ns = NULL;
    dom_exception exc = dom_node_get_namespace((dom_node *)priv->node, &ns);
    if (exc != DOM_NO_ERR || ns == NULL) return JS_NULL;
    JSValue val = JS_NewStringLen(ctx, dom_string_data(ns), dom_string_byte_length(ns));
    dom_string_unref(ns);
    return val;
}

static JSValue js_element_prefix_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node) return JS_NULL;
    dom_string *pre = NULL;
    dom_exception exc = dom_node_get_prefix((dom_node *)priv->node, &pre);
    if (exc != DOM_NO_ERR || pre == NULL) return JS_NULL;
    JSValue val = JS_NewStringLen(ctx, dom_string_data(pre), dom_string_byte_length(pre));
    dom_string_unref(pre);
    return val;
}

static JSValue js_element_localName_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node) return JS_UNDEFINED;
    dom_string *name = NULL;
    dom_string_create((const uint8_t *)"", 0, &name);
    dom_node_get_local_name((dom_node *)priv->node, &name);
    if (name) {
        JSValue val = JS_NewStringLen(ctx, dom_string_data(name), dom_string_byte_length(name));
        dom_string_unref(name);
        return val;
    }
    return JS_NULL;
}

static JSValue js_element_tagName_get(JSContext *ctx, JSValueConst this_val)
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
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node) return JS_NULL;
    struct dom_namednodemap *map = NULL;
    dom_exception exc = dom_node_get_attributes((dom_node *)priv->node, &map);
    if (exc != DOM_NO_ERR || map == NULL) return JS_NULL;
    JSValue val = qjs_new_namednodemap(ctx, map);
    dom_namednodemap_unref(map);
    return val;
}

static JSValue js_element_children_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node) return JS_NewArray(ctx);

    JSValue arr = JS_NewArray(ctx);
    struct dom_node *child = NULL;
    dom_node_get_first_child((dom_node *)priv->node, &child);
    uint32_t i = 0;
    while (child) {
        dom_node_type type;
        dom_node_get_node_type(child, &type);
        if (type == DOM_ELEMENT_NODE) {
            JS_SetPropertyUint32(ctx, arr, i++, qjs_wrap_node(ctx, child));
        }
        struct dom_node *next = NULL;
        dom_node_get_next_sibling(child, &next);
        dom_node_unref(child);
        child = next;
    }
    return arr;
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
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node) return JS_NULL;
    struct dom_node *child = NULL;
    dom_node_get_last_child((dom_node *)priv->node, &child);
    while (child) {
        dom_node_type type;
        dom_node_get_node_type(child, &type);
        if (type == DOM_ELEMENT_NODE) {
            JSValue val = qjs_wrap_node(ctx, child);
            dom_node_unref(child);
            return val;
        }
        struct dom_node *prev = NULL;
        dom_node_get_previous_sibling(child, &prev);
        dom_node_unref(child);
        child = prev;
    }
    return JS_NULL;
}

static JSValue js_element_childElementCount_get(JSContext *ctx, JSValueConst this_val)
{
    JSValue children = js_element_children_get(ctx, this_val);
    if (JS_IsException(children)) return JS_NewInt32(ctx, 0);
    JSValue len_val = JS_GetPropertyStr(ctx, children, "length");
    JS_FreeValue(ctx, children);
    return len_val;
}

static JSValue js_element_previousElementSibling_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node) return JS_NULL;
    struct dom_node *sibling = NULL;
    dom_node_get_previous_sibling((dom_node *)priv->node, &sibling);
    while (sibling) {
        dom_node_type type;
        dom_node_get_node_type(sibling, &type);
        if (type == DOM_ELEMENT_NODE) {
            JSValue val = qjs_wrap_node(ctx, sibling);
            dom_node_unref(sibling);
            return val;
        }
        struct dom_node *prev = NULL;
        dom_node_get_previous_sibling(sibling, &prev);
        dom_node_unref(sibling);
        sibling = prev;
    }
    return JS_NULL;
}

static JSValue js_element_nextElementSibling_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node) return JS_NULL;
    struct dom_node *sibling = NULL;
    dom_node_get_next_sibling((dom_node *)priv->node, &sibling);
    while (sibling) {
        dom_node_type type;
        dom_node_get_node_type(sibling, &type);
        if (type == DOM_ELEMENT_NODE) {
            JSValue val = qjs_wrap_node(ctx, sibling);
            dom_node_unref(sibling);
            return val;
        }
        struct dom_node *next = NULL;
        dom_node_get_next_sibling(sibling, &next);
        dom_node_unref(sibling);
        sibling = next;
    }
    return JS_NULL;
}

static JSValue js_element_dir_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node) return JS_NULL;
    dom_string *dir = NULL;
    dom_exception exc = dom_html_element_get_dir((dom_html_element *)priv->node, &dir);
    if (exc != DOM_NO_ERR || dir == NULL) return JS_NewString(ctx, "ltr");
    JSValue val = JS_NewStringLen(ctx, dom_string_data(dir), dom_string_byte_length(dir));
    dom_string_unref(dir);
    return val;
}

static JSValue js_element_lang_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node) return JS_NULL;
    dom_string *lang = NULL;
    dom_exception exc = dom_html_element_get_lang((dom_html_element *)priv->node, &lang);
    if (exc != DOM_NO_ERR || lang == NULL) return JS_NewString(ctx, "");
    JSValue val = JS_NewStringLen(ctx, dom_string_data(lang), dom_string_byte_length(lang));
    dom_string_unref(lang);
    return val;
}

static JSValue js_element_title_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node) return JS_NULL;
    dom_string *title = NULL;
    dom_exception exc = dom_html_element_get_title((dom_html_element *)priv->node, &title);
    if (exc != DOM_NO_ERR || title == NULL) return JS_NewString(ctx, "");
    JSValue val = JS_NewStringLen(ctx, dom_string_data(title), dom_string_byte_length(title));
    dom_string_unref(title);
    return val;
}

static JSValue js_element_dir_set(JSContext *ctx, JSValueConst this_val, JSValueConst val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node) return JS_UNDEFINED;
    const char *str = JS_ToCString(ctx, val);
    if (str) {
        dom_string *dir_dom = NULL;
        dom_string_create((const uint8_t *)str, strlen(str), &dir_dom);
        dom_html_element_set_dir((dom_html_element *)priv->node, dir_dom);
        dom_string_unref(dir_dom);
        JS_FreeCString(ctx, str);
    }
    return JS_UNDEFINED;
}

static JSValue js_element_lang_set(JSContext *ctx, JSValueConst this_val, JSValueConst val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node) return JS_UNDEFINED;
    const char *str = JS_ToCString(ctx, val);
    if (str) {
        dom_string *lang_dom = NULL;
        dom_string_create((const uint8_t *)str, strlen(str), &lang_dom);
        dom_html_element_set_lang((dom_html_element *)priv->node, lang_dom);
        dom_string_unref(lang_dom);
        JS_FreeCString(ctx, str);
    }
    return JS_UNDEFINED;
}

static JSValue js_element_title_set(JSContext *ctx, JSValueConst this_val, JSValueConst val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_element_class_id);
    if (!priv || !priv->node) return JS_UNDEFINED;
    const char *str = JS_ToCString(ctx, val);
    if (str) {
        dom_string *title_dom = NULL;
        dom_string_create((const uint8_t *)str, strlen(str), &title_dom);
        dom_html_element_set_title((dom_html_element *)priv->node, title_dom);
        dom_string_unref(title_dom);
        JS_FreeCString(ctx, str);
    }
    return JS_UNDEFINED;
}

int qjs_init_element(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_element_class_id == 0) JS_NewClassID(rt, &qjs_element_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_element_class_id)) JS_NewClass(rt, qjs_element_class_id, &js_element_class);
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_element_proto_funcs, sizeof(js_element_proto_funcs) / sizeof(js_element_proto_funcs[0]));

    /* Add extra HTMLElement properties */
    JS_SetPropertyStr(ctx, proto, "dir", JS_NewCFunction2(ctx, (JSCFunction *)js_element_dir_get, "dir", 0, JS_CFUNC_getter, 0));
    JS_SetPropertyStr(ctx, proto, "lang", JS_NewCFunction2(ctx, (JSCFunction *)js_element_lang_get, "lang", 0, JS_CFUNC_getter, 0));
    JS_SetPropertyStr(ctx, proto, "title", JS_NewCFunction2(ctx, (JSCFunction *)js_element_title_get, "title", 0, JS_CFUNC_getter, 0));

    /* Setters for dir, lang, title */
    JS_DefinePropertyValueStr(ctx, proto, "dir", JS_UNDEFINED, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_SetPropertyStr(ctx, proto, "dir", JS_NewCFunction2(ctx, (JSCFunction *)js_element_dir_get, "dir", 0, JS_CFUNC_getter, 0));
    /* Need to use JS_DefineProperty for getters/setters properly if not using JSCFunctionListEntry */
    JSValue dir_name = JS_NewString(ctx, "dir");
    JSValue dir_get = JS_NewCFunction2(ctx, (JSCFunction *)js_element_dir_get, "dir", 0, JS_CFUNC_getter, 0);
    JSValue dir_set = JS_NewCFunction2(ctx, (JSCFunction *)js_element_dir_set, "dir", 1, JS_CFUNC_setter, 0);
    JS_DefinePropertyGetSet(ctx, proto, JS_NewAtom(ctx, "dir"), dir_get, dir_set, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeValue(ctx, dir_name);

    JSValue lang_get = JS_NewCFunction2(ctx, (JSCFunction *)js_element_lang_get, "lang", 0, JS_CFUNC_getter, 0);
    JSValue lang_set = JS_NewCFunction2(ctx, (JSCFunction *)js_element_lang_set, "lang", 1, JS_CFUNC_setter, 0);
    JS_DefinePropertyGetSet(ctx, proto, JS_NewAtom(ctx, "lang"), lang_get, lang_set, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);

    JSValue title_get = JS_NewCFunction2(ctx, (JSCFunction *)js_element_title_get, "title", 0, JS_CFUNC_getter, 0);
    JSValue title_set = JS_NewCFunction2(ctx, (JSCFunction *)js_element_title_set, "title", 1, JS_CFUNC_setter, 0);
    JS_DefinePropertyGetSet(ctx, proto, JS_NewAtom(ctx, "title"), title_get, title_set, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);

    JS_SetClassProto(ctx, qjs_element_class_id, proto);
    JS_FreeValue(ctx, proto);
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
