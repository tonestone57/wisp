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
#include <dom/core/namednodemap.h>
JSClassID qjs_namednodemap_class_id;
typedef struct {
    void *map;
} QJSNamedNodeMapPrivate;

static void js_namednodemap_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNamedNodeMapPrivate *priv = JS_GetOpaque(val, qjs_namednodemap_class_id);
    if (priv) {
        if (priv->map) dom_namednodemap_unref((struct dom_namednodemap *)priv->map);
        free(priv);
    }
}

static JSClassDef js_namednodemap_class = {
    "NamedNodeMap",
    .finalizer = js_namednodemap_finalizer,
};

static JSValue js_namednodemap_item(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNamedNodeMapPrivate *priv = JS_GetOpaque(this_val, qjs_namednodemap_class_id);
    if (!priv || !priv->map || argc < 1) return JS_NULL;
    uint32_t index;
    JS_ToUint32(ctx, &index, argv[0]);
    struct dom_node *node = NULL;
    dom_namednodemap_item(priv->map, index, &node);
    if (!node) return JS_NULL;
    JSValue val = qjs_wrap_node(ctx, node);
    dom_node_unref(node);
    return val;
}

static JSValue js_namednodemap_getNamedItem(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNamedNodeMapPrivate *priv = JS_GetOpaque(this_val, qjs_namednodemap_class_id);
    if (!priv || !priv->map || argc < 1) return JS_NULL;
    const char *name = JS_ToCString(ctx, argv[0]);
    if (!name) return JS_NULL;
    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)name, strlen(name), &name_dom);
    JS_FreeCString(ctx, name);
    struct dom_node *node = NULL;
    dom_namednodemap_get_named_item(priv->map, name_dom, &node);
    dom_string_unref(name_dom);
    if (!node) return JS_NULL;
    JSValue val = qjs_wrap_node(ctx, node);
    dom_node_unref(node);
    return val;
}

static JSValue js_namednodemap_length_get(JSContext *ctx, JSValueConst this_val)
{
    QJSNamedNodeMapPrivate *priv = JS_GetOpaque(this_val, qjs_namednodemap_class_id);
    if (!priv || !priv->map) return JS_NewInt32(ctx, 0);
    dom_ulong len = 0;
    dom_namednodemap_get_length((struct dom_namednodemap *)priv->map, &len);
    return JS_NewUint32(ctx, (uint32_t)len);
}

static const JSCFunctionListEntry js_namednodemap_proto_funcs[] = {
    JS_CFUNC_DEF("item", 1, js_namednodemap_item),
    JS_CFUNC_DEF("getNamedItem", 1, js_namednodemap_getNamedItem),
    JS_CGETSET_DEF("length", js_namednodemap_length_get, NULL),
};

int qjs_init_namednodemap(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_namednodemap_class_id == 0) JS_NewClassID(rt, &qjs_namednodemap_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_namednodemap_class_id)) JS_NewClass(rt, qjs_namednodemap_class_id, &js_namednodemap_class);
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_namednodemap_proto_funcs, sizeof(js_namednodemap_proto_funcs) / sizeof(js_namednodemap_proto_funcs[0]));
    JS_SetClassProto(ctx, qjs_namednodemap_class_id, proto);
    return 0;
}

JSValue qjs_new_namednodemap(JSContext *ctx, void *map)
{
    JSValue obj = JS_NewObjectClass(ctx, qjs_namednodemap_class_id);
    QJSNamedNodeMapPrivate *priv = calloc(1, sizeof(QJSNamedNodeMapPrivate));
    if (!priv) return JS_ThrowOutOfMemory(ctx);
    priv->map = map;
    if (map) dom_namednodemap_ref((struct dom_namednodemap *)map);
    JS_SetOpaque(obj, priv); return obj;
}
