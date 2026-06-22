#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSDocument.gen.h"

JSValue wisp_document_createElement_impl(JSContext *ctx, QJSNodePrivate *priv, const char * localName)
{
    if (!priv || !priv->node) return JS_NULL;
    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)localName, strlen(localName), &name_dom);
    struct dom_element *result = NULL;
    dom_document_create_element((dom_document *)priv->node, name_dom, &result);
    dom_string_unref(name_dom);
    if (result) {
        JSValue val = qjs_wrap_node(ctx, (dom_node *)result);
        dom_node_unref((dom_node *)result);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_document_createTextNode_impl(JSContext *ctx, QJSNodePrivate *priv, const char * data)
{
    if (!priv || !priv->node) return JS_NULL;
    dom_string *data_dom = NULL;
    dom_string_create((const uint8_t *)data, strlen(data), &data_dom);
    struct dom_text *result = NULL;
    dom_document_create_text_node((dom_document *)priv->node, data_dom, &result);
    dom_string_unref(data_dom);
    if (result) {
        JSValue val = qjs_wrap_node(ctx, (dom_node *)result);
        dom_node_unref((dom_node *)result);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_document_getElementById_impl(JSContext *ctx, QJSNodePrivate *priv, const char * elementId)
{
    if (!priv || !priv->node) return JS_NULL;
    dom_string *id_dom = NULL;
    dom_string_create((const uint8_t *)elementId, strlen(elementId), &id_dom);
    struct dom_element *result = NULL;
    dom_document_get_element_by_id((dom_document *)priv->node, id_dom, &result);
    dom_string_unref(id_dom);
    if (result) {
        JSValue val = qjs_wrap_node(ctx, (dom_node *)result);
        dom_node_unref((dom_node *)result);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_document_getElementsByTagName_impl(JSContext *ctx, QJSNodePrivate *priv, const char * localName)
{
    NSLOG(wisp, DEBUG, "Document.getElementsByTagName stub");
    return JS_NULL;
}

JSValue wisp_document_body_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    struct dom_html_document *html_doc = (struct dom_html_document *)priv->node;
    struct dom_html_element *body = NULL;
    dom_html_document_get_body(html_doc, &body);
    if (body) {
        JSValue val = qjs_wrap_node(ctx, (dom_node *)body);
        dom_node_unref((dom_node *)body);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_document_documentElement_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    struct dom_element *root = NULL;
    dom_document_get_document_element((dom_document *)priv->node, &root);
    if (root) {
        JSValue val = qjs_wrap_node(ctx, (dom_node *)root);
        dom_node_unref((dom_node *)root);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_document_write_impl(JSContext *ctx, QJSNodePrivate *priv, const char * text) { return JS_UNDEFINED; }
JSValue wisp_document_writeln_impl(JSContext *ctx, QJSNodePrivate *priv, const char * text) { return JS_UNDEFINED; }
JSValue wisp_document_cookie_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, ""); }
JSValue wisp_document_cookie_set_impl(JSContext *ctx, QJSNodePrivate *priv, void * value) { return JS_UNDEFINED; }

int qjs_init_document(JSContext *ctx) { return qjs_init_document_gen(ctx); }
