/* Implementation for Document */
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"

#include "document.inc"

static void js_document_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_document_class_id);
    if (priv) {
        qjs_bridge_remove_node(rt, (dom_node *)priv->node, priv->ctx);
        if (priv->is_dom_node && priv->node) dom_node_unref((dom_node *)priv->node);
        free(priv);
    }
static JSValue js_document_getElementsByTagName(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.getElementsByTagName() called (stub)");
    return JS_NewArray(ctx);
}

static JSValue js_document_getElementsByTagNameNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.getElementsByTagNameNS() called (stub)");
    return JS_NewArray(ctx);
}

static JSValue js_document_getElementsByClassName(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.getElementsByClassName() called (stub)");
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
    NSLOG(wisp, DEBUG, "Document.createDocumentFragment() called (stub)");
    return JS_UNDEFINED;
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
    NSLOG(wisp, DEBUG, "Document.createComment() called (stub)");
    return JS_UNDEFINED;
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
    NSLOG(wisp, DEBUG, "Document.URL getter called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_documentURI_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Document.documentURI getter called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_origin_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Document.origin getter called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_compatMode_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Document.compatMode getter called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_characterSet_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Document.characterSet getter called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_inputEncoding_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Document.inputEncoding getter called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_contentType_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Document.contentType getter called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_doctype_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Document.doctype getter called (stub)");
    return JS_UNDEFINED;
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
    NSLOG(wisp, DEBUG, "Document.children getter called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_firstElementChild_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Document.firstElementChild getter called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_lastElementChild_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Document.lastElementChild getter called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_document_childElementCount_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Document.childElementCount getter called (stub)");
    return JS_UNDEFINED;
}

int qjs_init_document(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_document_class_id == 0) JS_NewClassID(rt, &qjs_document_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_document_class_id)) JS_NewClass(rt, qjs_document_class_id, &js_document_class);
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_document_proto_funcs, sizeof(js_document_proto_funcs) / sizeof(js_document_proto_funcs[0]));
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
