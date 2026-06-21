/*
 * Copyright 2026 Neosurf Contributors
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include <dom/html/html_collection.h>
JSClassID qjs_htmlcollection_class_id;
typedef struct {
    void *col;
} QJSHTMLCollectionPrivate;

static void js_htmlcollection_finalizer(JSRuntime *rt, JSValue val)
{
    QJSHTMLCollectionPrivate *priv = JS_GetOpaque(val, qjs_htmlcollection_class_id);
    if (priv) {
        if (priv->col) dom_html_collection_unref((struct dom_html_collection *)priv->col);
        free(priv);
    }
}

static JSClassDef js_htmlcollection_class = {
    "HTMLCollection",
    .finalizer = js_htmlcollection_finalizer,
};

static JSValue js_htmlcollection_item(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSHTMLCollectionPrivate *priv = JS_GetOpaque(this_val, qjs_htmlcollection_class_id);
    if (!priv || !priv->col || argc < 1) return JS_NULL;
    uint32_t index;
    JS_ToUint32(ctx, &index, argv[0]);
    struct dom_node *node = NULL;
    dom_html_collection_item(priv->col, index, &node);
    if (!node) return JS_NULL;
    JSValue val = qjs_wrap_node(ctx, node);
    dom_node_unref(node);
    return val;
}

static JSValue js_htmlcollection_namedItem(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSHTMLCollectionPrivate *priv = JS_GetOpaque(this_val, qjs_htmlcollection_class_id);
    if (!priv || !priv->col || argc < 1) return JS_NULL;
    const char *name = JS_ToCString(ctx, argv[0]);
    if (!name) return JS_NULL;
    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)name, strlen(name), &name_dom);
    JS_FreeCString(ctx, name);
    struct dom_node *node = NULL;
    dom_html_collection_named_item(priv->col, name_dom, &node);
    dom_string_unref(name_dom);
    if (!node) return JS_NULL;
    JSValue val = qjs_wrap_node(ctx, node);
    dom_node_unref(node);
    return val;
}

static JSValue js_htmlcollection_length_get(JSContext *ctx, JSValueConst this_val)
{
    QJSHTMLCollectionPrivate *priv = JS_GetOpaque(this_val, qjs_htmlcollection_class_id);
    if (!priv || !priv->col) return JS_NewInt32(ctx, 0);
    uint32_t len = 0;
    dom_html_collection_get_length((struct dom_html_collection *)priv->col, &len);
    return JS_NewUint32(ctx, len);
}

static const JSCFunctionListEntry js_htmlcollection_proto_funcs[] = {
    JS_CFUNC_DEF("item", 1, js_htmlcollection_item),
    JS_CFUNC_DEF("namedItem", 1, js_htmlcollection_namedItem),
    JS_CGETSET_DEF("length", js_htmlcollection_length_get, NULL),
};

int qjs_init_htmlcollection(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_htmlcollection_class_id == 0) JS_NewClassID(rt, &qjs_htmlcollection_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_htmlcollection_class_id)) JS_NewClass(rt, qjs_htmlcollection_class_id, &js_htmlcollection_class);
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_htmlcollection_proto_funcs, sizeof(js_htmlcollection_proto_funcs) / sizeof(js_htmlcollection_proto_funcs[0]));
    JS_SetClassProto(ctx, qjs_htmlcollection_class_id, proto);
    return 0;
}

JSValue qjs_new_htmlcollection(JSContext *ctx, void *col)
{
    JSValue obj = JS_NewObjectClass(ctx, qjs_htmlcollection_class_id);
    QJSHTMLCollectionPrivate *priv = calloc(1, sizeof(QJSHTMLCollectionPrivate));
    if (!priv) return JS_ThrowOutOfMemory(ctx);
    priv->col = col;
    if (col) dom_html_collection_ref((struct dom_html_collection *)col);
    JS_SetOpaque(obj, priv); return obj;
}
