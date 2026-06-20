/* Implementation for Document */
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include <dom/html/html_document.h>
#include <dom/html/html_element.h>
#include <dom/html/html_collection.h>

static void js_document_finalizer(JSRuntime *rt, JSValue val);

static void js_document_finalizer(JSRuntime *rt, JSValue val);

static void js_document_finalizer(JSRuntime *rt, JSValue val);

#include "document.inc"

static void js_document_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_document_class_id);
    if (priv) {
        qjs_bridge_remove_node(rt, (dom_node *)priv->node, priv->ctx);
        if (priv->is_dom_node && priv->node) dom_node_unref((dom_node *)priv->node);
        free(priv);
    }
}

static JSValue js_document_getElementsByTagName(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node || argc < 1) return JS_NewArray(ctx);

    const char *tag = JS_ToCString(ctx, argv[0]);
    if (!tag) return JS_NewArray(ctx);

    dom_string *tag_dom = NULL;
    dom_string_create((const uint8_t *)tag, strlen(tag), &tag_dom);
    JS_FreeCString(ctx, tag);

    struct dom_nodelist *list = NULL;
    dom_exception exc = dom_document_get_elements_by_tag_name((dom_document *)priv->node, tag_dom, &list);
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

static JSValue js_document_getElementsByTagNameNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.getElementsByTagNameNS() called (stub)");
    return JS_NewArray(ctx);
}

static JSValue js_document_getElementsByClassName(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    /* LibDOM 0.9.x does not have dom_document_get_elements_by_class_name.
       Stubbing it to return an empty array for now. */
    NSLOG(wisp, DEBUG, "Document.getElementsByClassName() called (not supported in current LibDOM)");
    return JS_NewArray(ctx);
}

static JSValue js_document_createElement(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node) return JS_EXCEPTION;
    if (argc < 1) return JS_EXCEPTION;

    const char *tag = JS_ToCString(ctx, argv[0]);
    if (!tag) return JS_EXCEPTION;

    dom_string *tag_dom = NULL;
    dom_string_create((const uint8_t *)tag, strlen(tag), &tag_dom);
    JS_FreeCString(ctx, tag);

    struct dom_element *result = NULL;
    dom_exception exc = dom_document_create_element((dom_document *)priv->node, tag_dom, &result);
    dom_string_unref(tag_dom);

    if (exc != DOM_NO_ERR || result == NULL) return JS_ThrowInternalError(ctx, "dom_document_create_element failed");

    JSValue val = qjs_wrap_node(ctx, (dom_node *)result);
    dom_node_unref((dom_node *)result);
    return val;
}

static JSValue js_document_createElementNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.createElementNS() called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_createDocumentFragment(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node) return JS_EXCEPTION;

    struct dom_document_fragment *result = NULL;
    dom_exception exc = dom_document_create_document_fragment((dom_document *)priv->node, &result);

    if (exc != DOM_NO_ERR || result == NULL) return JS_ThrowInternalError(ctx, "dom_document_create_document_fragment failed");

    JSValue val = qjs_wrap_node(ctx, (dom_node *)result);
    dom_node_unref((dom_node *)result);
    return val;
}

static JSValue js_document_createTextNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node) return JS_EXCEPTION;
    if (argc < 1) return JS_EXCEPTION;

    const char *data = JS_ToCString(ctx, argv[0]);
    if (!data) return JS_EXCEPTION;

    dom_string *data_dom = NULL;
    dom_string_create((const uint8_t *)data, strlen(data), &data_dom);
    JS_FreeCString(ctx, data);

    struct dom_text *result = NULL;
    dom_exception exc = dom_document_create_text_node((dom_document *)priv->node, data_dom, &result);
    dom_string_unref(data_dom);

    if (exc != DOM_NO_ERR || result == NULL) return JS_ThrowInternalError(ctx, "dom_document_create_text_node failed");

    JSValue val = qjs_wrap_node(ctx, (dom_node *)result);
    dom_node_unref((dom_node *)result);
    return val;
}

static JSValue js_document_createComment(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node) return JS_EXCEPTION;
    if (argc < 1) return JS_EXCEPTION;

    const char *data = JS_ToCString(ctx, argv[0]);
    if (!data) return JS_EXCEPTION;

    dom_string *data_dom = NULL;
    dom_string_create((const uint8_t *)data, strlen(data), &data_dom);
    JS_FreeCString(ctx, data);

    struct dom_comment *result = NULL;
    dom_exception exc = dom_document_create_comment((dom_document *)priv->node, data_dom, &result);
    dom_string_unref(data_dom);

    if (exc != DOM_NO_ERR || result == NULL) return JS_ThrowInternalError(ctx, "dom_document_create_comment failed");

    JSValue val = qjs_wrap_node(ctx, (dom_node *)result);
    dom_node_unref((dom_node *)result);
    return val;
}

static JSValue js_document_createProcessingInstruction(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.createProcessingInstruction() called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_importNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.importNode() called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_adoptNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.adoptNode() called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_createAttribute(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.createAttribute() called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_createAttributeNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.createAttributeNS() called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_createEvent(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.createEvent() called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_createRange(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.createRange() called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_createNodeIterator(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.createNodeIterator() called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_createTreeWalker(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.createTreeWalker() called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_getElementById(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node) return JS_NULL;

    if (argc < 1) return JS_NULL;
    const char *id = JS_ToCString(ctx, argv[0]);
    if (!id) return JS_NULL;

    dom_string *id_dom = NULL;
    dom_string_create((const uint8_t *)id, strlen(id), &id_dom);
    JS_FreeCString(ctx, id);

    struct dom_element *result = NULL;
    dom_exception exc = dom_document_get_element_by_id((dom_document *)priv->node, id_dom, &result);
    dom_string_unref(id_dom);

    if (exc != DOM_NO_ERR || result == NULL) return JS_NULL;

    JSValue val = qjs_wrap_node(ctx, (dom_node *)result);
    dom_node_unref((dom_node *)result);
    return val;
}

static JSValue js_document_prepend(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.prepend() called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_append(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.append() called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_query(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.query() called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_queryAll(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.queryAll() called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_querySelector(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    /* Fallback implementation using getElementById for simple ID selectors */
    if (argc > 0) {
        const char *selector = JS_ToCString(ctx, argv[0]);
        if (selector && selector[0] == '#' && strpbrk(selector, " .[") == NULL) {
            JSValue id_val = JS_NewString(ctx, selector + 1);
            JSValue res = js_document_getElementById(ctx, this_val, 1, &id_val);
            JS_FreeValue(ctx, id_val);
            JS_FreeCString(ctx, selector);
            return res;
        }
        if (selector) JS_FreeCString(ctx, selector);
    }
    NSLOG(wisp, DEBUG, "Document.querySelector() called with non-trivial selector (stub)");
    return JS_NULL;
}

static JSValue js_document_querySelectorAll(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.querySelectorAll() called (stub)");
    return JS_NewArray(ctx);
}

static JSValue js_document_implementation_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Document.implementation getter called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_URL_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node) return JS_NULL;
    dom_string *url = NULL;
    dom_exception exc = dom_document_get_uri((dom_document *)priv->node, &url);
    if (exc != DOM_NO_ERR || url == NULL) return JS_NewString(ctx, "about:blank");
    JSValue val = JS_NewStringLen(ctx, dom_string_data(url), dom_string_byte_length(url));
    dom_string_unref(url);
    return val;
}

static JSValue js_document_documentURI_get(JSContext *ctx, JSValueConst this_val)
{
    return js_document_URL_get(ctx, this_val);
}

static JSValue js_document_origin_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Document.origin getter called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_compatMode_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Document.compatMode getter called (stub)");
    return JS_NewString(ctx, "CSS1Compat");
}

static JSValue js_document_characterSet_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node) return JS_NULL;
    dom_string *enc = NULL;
    dom_exception exc = dom_document_get_input_encoding((dom_document *)priv->node, &enc);
    if (exc != DOM_NO_ERR || enc == NULL) return JS_NewString(ctx, "UTF-8");
    JSValue val = JS_NewStringLen(ctx, dom_string_data(enc), dom_string_byte_length(enc));
    dom_string_unref(enc);
    return val;
}

static JSValue js_document_inputEncoding_get(JSContext *ctx, JSValueConst this_val)
{
    return js_document_characterSet_get(ctx, this_val);
}

static JSValue js_document_contentType_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Document.contentType getter called (stub)");
    return JS_NewString(ctx, "text/html");
}

static JSValue js_document_doctype_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node) return JS_NULL;
    struct dom_document_type *doctype = NULL;
    dom_document_get_doctype((dom_document *)priv->node, &doctype);
    if (doctype) {
        JSValue val = qjs_wrap_node(ctx, (dom_node *)doctype);
        dom_node_unref((dom_node *)doctype);
        return val;
    }
    return JS_NULL;
}

static JSValue js_document_documentElement_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node) return JS_NULL;
    struct dom_element *documentElement = NULL;
    dom_exception exc = dom_document_get_document_element((dom_document *)priv->node, &documentElement);
    if (exc != DOM_NO_ERR || !documentElement) return JS_NULL;
    JSValue val = qjs_wrap_node(ctx, (dom_node *)documentElement);
    dom_node_unref((dom_node *)documentElement);
    return val;
}

static JSValue js_document_children_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
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

static JSValue js_document_firstElementChild_get(JSContext *ctx, JSValueConst this_val)
{
    return js_document_documentElement_get(ctx, this_val);
}

static JSValue js_document_lastElementChild_get(JSContext *ctx, JSValueConst this_val)
{
    return js_document_documentElement_get(ctx, this_val);
}

static JSValue js_document_childElementCount_get(JSContext *ctx, JSValueConst this_val)
{
    JSValue children = js_document_children_get(ctx, this_val);
    if (JS_IsException(children)) return JS_NewInt32(ctx, 0);
    JSValue len_val = JS_GetPropertyStr(ctx, children, "length");
    JS_FreeValue(ctx, children);
    return len_val;
}

static JSValue js_document_body_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node) return JS_NULL;

    struct dom_html_element *body = NULL;
    dom_exception exc = dom_html_document_get_body((dom_html_document *)priv->node, &body);
    if (exc != DOM_NO_ERR || body == NULL) return JS_NULL;

    JSValue val = qjs_wrap_node(ctx, (dom_node *)body);
    dom_node_unref((dom_node *)body);
    return val;
}

static JSValue js_document_body_set(JSContext *ctx, JSValueConst this_val, JSValueConst val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node) return JS_UNDEFINED;
    QJSNodePrivate *body_priv = JS_GetOpaque(val, qjs_element_class_id);
    if (!body_priv || !body_priv->node) return JS_ThrowTypeError(ctx, "Argument is not an Element");

    (dom_html_document_set_body)((dom_html_document *)priv->node, (struct dom_html_element *)body_priv->node);
    return JS_UNDEFINED;
}

static JSValue js_document_title_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node) return JS_NULL;

    dom_string *title = NULL;
    dom_exception exc = dom_html_document_get_title((dom_html_document *)priv->node, &title);
    if (exc != DOM_NO_ERR || title == NULL) return JS_NewString(ctx, "");

    JSValue val = JS_NewStringLen(ctx, dom_string_data(title), dom_string_byte_length(title));
    dom_string_unref(title);
    return val;
}

static JSValue js_document_title_set(JSContext *ctx, JSValueConst this_val, JSValueConst val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node) return JS_UNDEFINED;
    const char *str = JS_ToCString(ctx, val);
    if (str) {
        dom_string *title_dom = NULL;
        dom_string_create((const uint8_t *)str, strlen(str), &title_dom);
        dom_html_document_set_title((dom_html_document *)priv->node, title_dom);
        dom_string_unref(title_dom);
        JS_FreeCString(ctx, str);
    }
    return JS_UNDEFINED;
}

static JSValue js_document_cookie_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node) return JS_NULL;

    dom_string *cookie = NULL;
    dom_exception exc = dom_html_document_get_cookie((dom_html_document *)priv->node, &cookie);
    if (exc != DOM_NO_ERR || cookie == NULL) return JS_NewString(ctx, "");

    JSValue val = JS_NewStringLen(ctx, dom_string_data(cookie), dom_string_byte_length(cookie));
    dom_string_unref(cookie);
    return val;
}

static JSValue js_document_cookie_set(JSContext *ctx, JSValueConst this_val, JSValueConst val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node) return JS_UNDEFINED;
    const char *str = JS_ToCString(ctx, val);
    if (str) {
        dom_string *cookie_dom = NULL;
        dom_string_create((const uint8_t *)str, strlen(str), &cookie_dom);
        dom_html_document_set_cookie((dom_html_document *)priv->node, cookie_dom);
        dom_string_unref(cookie_dom);
        JS_FreeCString(ctx, str);
    }
    return JS_UNDEFINED;
static JSValue js_document_referrer_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node) return JS_NULL;

    dom_string *referrer = NULL;
    dom_exception exc = dom_html_document_get_referrer((dom_html_document *)priv->node, &referrer);
    if (exc != DOM_NO_ERR || referrer == NULL) return JS_NewString(ctx, "");

    JSValue val = JS_NewStringLen(ctx, dom_string_data(referrer), dom_string_byte_length(referrer));
    dom_string_unref(referrer);
    return val;
}

static JSValue js_document_domain_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node) return JS_NULL;

    dom_string *domain = NULL;
    dom_exception exc = dom_html_document_get_domain((dom_html_document *)priv->node, &domain);
    if (exc != DOM_NO_ERR || domain == NULL) return JS_NewString(ctx, "");

    JSValue val = JS_NewStringLen(ctx, dom_string_data(domain), dom_string_byte_length(domain));
    dom_string_unref(domain);
    return val;
}

static JSValue js_document_referrer_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node) return JS_NULL;

    dom_string *referrer = NULL;
    dom_exception exc = dom_html_document_get_referrer((dom_html_document *)priv->node, &referrer);
    if (exc != DOM_NO_ERR || referrer == NULL) return JS_NewString(ctx, "");

    JSValue val = JS_NewStringLen(ctx, dom_string_data(referrer), dom_string_byte_length(referrer));
    dom_string_unref(referrer);
    return val;
}

static JSValue js_document_domain_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node) return JS_NULL;

    dom_string *domain = NULL;
    dom_exception exc = dom_html_document_get_domain((dom_html_document *)priv->node, &domain);
    if (exc != DOM_NO_ERR || domain == NULL) return JS_NewString(ctx, "");

    JSValue val = JS_NewStringLen(ctx, dom_string_data(domain), dom_string_byte_length(domain));
    dom_string_unref(domain);
    return val;
}

static JSValue js_document_getElementsByName(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node || argc < 1) return JS_NewArray(ctx);

    const char *name = JS_ToCString(ctx, argv[0]);
    if (!name) return JS_NewArray(ctx);

    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)name, strlen(name), &name_dom);
    JS_FreeCString(ctx, name);

    struct dom_nodelist *list = NULL;
    dom_exception exc = dom_html_document_get_elements_by_name((dom_html_document *)priv->node, name_dom, &list);
    dom_string_unref(name_dom);

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

static JSValue js_document_images_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node) return JS_NULL;
    struct dom_html_collection *col = NULL;
    dom_exception exc = dom_html_document_get_images((dom_html_document *)priv->node, &col);
    if (exc != DOM_NO_ERR || col == NULL) return JS_NULL;
    JSValue val = qjs_new_htmlcollection(ctx, col);
    dom_html_collection_unref(col);
    return val;
}

static JSValue js_document_links_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node) return JS_NULL;
    struct dom_html_collection *col = NULL;
    dom_exception exc = dom_html_document_get_links((dom_html_document *)priv->node, &col);
    if (exc != DOM_NO_ERR || col == NULL) return JS_NULL;
    JSValue val = qjs_new_htmlcollection(ctx, col);
    dom_html_collection_unref(col);
    return val;
}

static JSValue js_document_forms_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node) return JS_NULL;
    struct dom_html_collection *col = NULL;
    dom_exception exc = dom_html_document_get_forms((dom_html_document *)priv->node, &col);
    if (exc != DOM_NO_ERR || col == NULL) return JS_NULL;
    JSValue val = qjs_new_htmlcollection(ctx, col);
    dom_html_collection_unref(col);
    return val;
}

static JSValue js_document_anchors_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNodePrivate *priv = JS_GetOpaque(this_val, qjs_document_class_id);
    if (!priv || !priv->node) return JS_NULL;
    struct dom_html_collection *col = NULL;
    dom_exception exc = dom_html_document_get_anchors((dom_html_document *)priv->node, &col);
    if (exc != DOM_NO_ERR || col == NULL) return JS_NULL;
    JSValue val = qjs_new_htmlcollection(ctx, col);
    dom_html_collection_unref(col);
    return val;
}

int qjs_init_document(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_document_class_id == 0) JS_NewClassID(rt, &qjs_document_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_document_class_id)) JS_NewClass(rt, qjs_document_class_id, &js_document_class);
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_document_proto_funcs, sizeof(js_document_proto_funcs) / sizeof(js_document_proto_funcs[0]));

    /* Add extra HTMLDocument properties that might not be in document.inc yet */
    JSValue body_get = JS_NewCFunction2(ctx, (JSCFunction *)js_document_body_get, "body", 0, JS_CFUNC_getter, 0);
    JSValue body_set = JS_NewCFunction2(ctx, (JSCFunction *)js_document_body_set, "body", 1, JS_CFUNC_setter, 0);
    JS_DefinePropertyGetSet(ctx, proto, JS_NewAtom(ctx, "body"), body_get, body_set, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);

    JSValue title_get = JS_NewCFunction2(ctx, (JSCFunction *)js_document_title_get, "title", 0, JS_CFUNC_getter, 0);
    JSValue title_set = JS_NewCFunction2(ctx, (JSCFunction *)js_document_title_set, "title", 1, JS_CFUNC_setter, 0);
    JS_DefinePropertyGetSet(ctx, proto, JS_NewAtom(ctx, "title"), title_get, title_set, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);

    JSValue cookie_get = JS_NewCFunction2(ctx, (JSCFunction *)js_document_cookie_get, "cookie", 0, JS_CFUNC_getter, 0);
    JSValue cookie_set = JS_NewCFunction2(ctx, (JSCFunction *)js_document_cookie_set, "cookie", 1, JS_CFUNC_setter, 0);
    JS_DefinePropertyGetSet(ctx, proto, JS_NewAtom(ctx, "cookie"), cookie_get, cookie_set, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);

    JS_SetPropertyStr(ctx, proto, "referrer", JS_NewCFunction2(ctx, (JSCFunction *)js_document_referrer_get, "referrer", 0, JS_CFUNC_getter, 0));
    JS_SetPropertyStr(ctx, proto, "domain", JS_NewCFunction2(ctx, (JSCFunction *)js_document_domain_get, "domain", 0, JS_CFUNC_getter, 0));
    JS_SetPropertyStr(ctx, proto, "getElementsByName", JS_NewCFunction(ctx, js_document_getElementsByName, "getElementsByName", 1));
    JS_SetPropertyStr(ctx, proto, "images", JS_NewCFunction2(ctx, (JSCFunction *)js_document_images_get, "images", 0, JS_CFUNC_getter, 0));
    JS_SetPropertyStr(ctx, proto, "links", JS_NewCFunction2(ctx, (JSCFunction *)js_document_links_get, "links", 0, JS_CFUNC_getter, 0));
    JS_SetPropertyStr(ctx, proto, "forms", JS_NewCFunction2(ctx, (JSCFunction *)js_document_forms_get, "forms", 0, JS_CFUNC_getter, 0));
    JS_SetPropertyStr(ctx, proto, "anchors", JS_NewCFunction2(ctx, (JSCFunction *)js_document_anchors_get, "anchors", 0, JS_CFUNC_getter, 0));
    JS_SetPropertyStr(ctx, proto, "body", JS_NewCFunction2(ctx, (JSCFunction *)js_document_body_get, "body", 0, JS_CFUNC_getter, 0));
    JS_SetPropertyStr(ctx, proto, "title", JS_NewCFunction2(ctx, (JSCFunction *)js_document_title_get, "title", 0, JS_CFUNC_getter, 0));
    JS_SetPropertyStr(ctx, proto, "cookie", JS_NewCFunction2(ctx, (JSCFunction *)js_document_cookie_get, "cookie", 0, JS_CFUNC_getter, 0));
    JS_SetPropertyStr(ctx, proto, "referrer", JS_NewCFunction2(ctx, (JSCFunction *)js_document_referrer_get, "referrer", 0, JS_CFUNC_getter, 0));
    JS_SetPropertyStr(ctx, proto, "domain", JS_NewCFunction2(ctx, (JSCFunction *)js_document_domain_get, "domain", 0, JS_CFUNC_getter, 0));

    JS_SetClassProto(ctx, qjs_document_class_id, proto);
    return 0;
}

JSValue qjs_new_document(JSContext *ctx, void *node, bool is_dom_node)
{
    JSValue obj = JS_NewObjectClass(ctx, qjs_document_class_id);
    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));
    if (!priv) return JS_ThrowOutOfMemory(ctx);
    priv->node = node; priv->ctx = ctx; priv->is_dom_node = is_dom_node;
    if (is_dom_node && node) dom_node_ref((dom_node *)node);
    JS_SetOpaque(obj, priv); return obj;
}
