#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include <wisp/utils/corestrings.h>
#include <wisp/content/handlers/html/private.h>
#include "utils/libdom.h"
#include "JSDocument.gen.h"
#include <dom/html/html_document.h>
#include <wisp/utils/nsurl.h>
#include <libwapcaplet/libwapcaplet.h>

struct content;
extern struct nsurl *content_get_url(struct content *c);
#include "JSEvent.gen.h"
#include "JSCustomEvent.gen.h"
#include "JSMessageEvent.gen.h"
#include "JSErrorEvent.gen.h"

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

JSValue wisp_document_head_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    return qjs_dom_query_selector_internal(ctx, (dom_node *)priv->node, "head", false);
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
    if (!priv || !priv->node || !localName) return JS_NewArray(ctx);
    return qjs_dom_query_selector_internal(ctx, (dom_node *)priv->node, localName, true);
}

JSValue wisp_document_getElementsByClassName_impl(JSContext *ctx, QJSNodePrivate *priv, const char * classNames)
{
    if (!priv || !priv->node || !classNames) return JS_NewArray(ctx);
    size_t len = strlen(classNames);
    char *selector = malloc(len + 2);
    if (!selector) return JS_ThrowOutOfMemory(ctx);
    selector[0] = '.';
    for (size_t i = 0; i < len; i++) {
        selector[i + 1] = (classNames[i] == ' ') ? '.' : classNames[i];
    }
    selector[len + 1] = '\0';
    JSValue res = qjs_dom_query_selector_internal(ctx, (dom_node *)priv->node, selector, true);
    free(selector);
    return res;
}

JSValue wisp_document_createEvent_impl(JSContext *ctx, QJSNodePrivate *priv, const char * interface)
{
    dom_event *evt = NULL;
    dom_event_create(&evt);
    if (evt) {
        JSValue obj;
        if (interface && strcasecmp(interface, "CustomEvent") == 0) {
            obj = qjs_new_customevent(ctx, evt, false);
        } else if (interface && strcasecmp(interface, "MessageEvent") == 0) {
            obj = qjs_new_messageevent(ctx, evt, false);
        } else if (interface && strcasecmp(interface, "ErrorEvent") == 0) {
            obj = qjs_new_errorevent(ctx, evt, false);
        } else {
            obj = qjs_new_event(ctx, evt, false);
        }
        dom_event_unref(evt);
        return obj;
    }
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

JSValue wisp_document_write_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue text) { return JS_UNDEFINED; }
JSValue wisp_document_writeln_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue text) { return JS_UNDEFINED; }
JSValue wisp_document_cookie_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, ""); }
JSValue wisp_document_cookie_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) { return JS_UNDEFINED; }

JSValue wisp_document_querySelector_impl(JSContext *ctx, QJSNodePrivate *priv, const char * selectors)
{
    if (!priv || !priv->node) return JS_NULL;
    return qjs_dom_query_selector_internal(ctx, (dom_node *)priv->node, selectors, false);
}

JSValue wisp_document_querySelectorAll_impl(JSContext *ctx, QJSNodePrivate *priv, const char * selectors)
{
    if (!priv || !priv->node) return JS_NULL;
    return qjs_dom_query_selector_internal(ctx, (dom_node *)priv->node, selectors, true);
}

JSValue wisp_document_defaultView_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_GetGlobalObject(ctx);
}

JSValue wisp_document_createComment_impl(JSContext *ctx, QJSNodePrivate *priv, const char * data)
{
    if (!priv || !priv->node) return JS_NULL;
    dom_string *data_dom = NULL;
    dom_string_create((const uint8_t *)data, strlen(data), &data_dom);
    struct dom_comment *result = NULL;
    dom_document_create_comment((dom_document *)priv->node, data_dom, &result);
    dom_string_unref(data_dom);
    if (result) {
        JSValue val = qjs_wrap_node(ctx, (dom_node *)result);
        dom_node_unref((dom_node *)result);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_document_getElementsByName_impl(JSContext *ctx, QJSNodePrivate *priv, const char * name)
{
    if (!priv || !priv->node || !name) return JS_NewArray(ctx);
    size_t len = strlen(name);
    char *selector = malloc(len + 16);
    if (!selector) return JS_ThrowOutOfMemory(ctx);
    sprintf(selector, "[name=\"%s\"]", name);
    JSValue res = qjs_dom_query_selector_internal(ctx, (dom_node *)priv->node, selector, true);
    free(selector);
    return res;
}

JSValue wisp_document_createDocumentFragment_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    struct dom_document_fragment *result = NULL;
    dom_document_create_document_fragment((dom_document *)priv->node, &result);
    if (result) {
        JSValue val = qjs_wrap_node(ctx, (dom_node *)result);
        dom_node_unref((dom_node *)result);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_document_readyState_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewString(ctx, "complete");
    html_content *htmlc = NULL;
    dom_node_get_user_data((dom_node *)priv->node, corestring_dom___ns_key_html_content_data, (void **)&htmlc);
    if (htmlc) {
        if (htmlc->parse_completed) {
            return JS_NewString(ctx, "complete");
        } else if (htmlc->conversion_begun) {
            return JS_NewString(ctx, "interactive");
        } else {
            return JS_NewString(ctx, "loading");
        }
    }
    return JS_NewString(ctx, "complete");
}

int qjs_init_document(JSContext *ctx) {
    JSValue global_obj = JS_GetGlobalObject(ctx);

    /* Check if already initialized on this global object */
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_document_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    /* Initialize the class and prototype using the generated function */
    qjs_init_document_gen(ctx);

    JSValue proto = JS_GetClassProto(ctx, qjs_document_class_id);
    if (!JS_IsObject(proto)) {
        JS_FreeValue(ctx, proto);
        proto = JS_NewObject(ctx);
        JS_SetClassProto(ctx, qjs_document_class_id, JS_DupValue(ctx, proto));
    }
    JSValue node_proto = JS_GetClassProto(ctx, qjs_node_class_id);
    if (JS_IsObject(proto) && JS_IsObject(node_proto)) JS_SetPrototype(ctx, proto, node_proto);
    JS_FreeValue(ctx, node_proto);
    JS_FreeValue(ctx, proto);

    /* Mark as initialized */
    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_document_init", JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);

    return 0;
}

JSValue wisp_document_domain_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (t && t->doc_priv) {
        struct nsurl *url = content_get_url((struct content *)t->doc_priv);
        if (url) {
            lwc_string *host_lwc = nsurl_get_component(url, NSURL_HOST);
            if (host_lwc) {
                const char *data = lwc_string_data(host_lwc);
                size_t len = lwc_string_length(host_lwc);
                JSValue res = JS_NewStringLen(ctx, data, len);
                lwc_string_unref(host_lwc);
                return res;
            }
        }
    }
    return JS_NewString(ctx, "");
}

JSValue wisp_document_domain_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return JS_UNDEFINED;
}

JSValue wisp_document_title_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    dom_html_document *html_doc = (dom_html_document *)priv->node;
    dom_string *title_dom = NULL;
    dom_exception exc = dom_html_document_get_title(html_doc, &title_dom);
    if (exc == DOM_NO_ERR && title_dom) {
        JSValue val = JS_NewStringLen(ctx, (const char *)dom_string_data(title_dom), dom_string_byte_length(title_dom));
        dom_string_unref(title_dom);
        return val;
    }
    return JS_NewString(ctx, "");
}

JSValue wisp_document_title_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    dom_html_document *html_doc = (dom_html_document *)priv->node;
    dom_string *title_dom = NULL;
    dom_string_create((const uint8_t *)value, strlen(value), &title_dom);
    if (title_dom) {
        dom_html_document_set_title(html_doc, title_dom);
        dom_string_unref(title_dom);
    }
    return JS_UNDEFINED;
}

JSValue wisp_document_activeElement_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    /* Try body first */
    JSValue body = wisp_document_body_get_impl(ctx, priv);
    if (!JS_IsNull(body)) {
        return body;
    }
    JS_FreeValue(ctx, body);

    /* Fall back to documentElement */
    JSValue doc_el = wisp_document_documentElement_get_impl(ctx, priv);
    if (!JS_IsNull(doc_el)) {
        return doc_el;
    }
    JS_FreeValue(ctx, doc_el);

    return JS_NULL;
}

JSValue wisp_document_currentScript_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NULL;
}
