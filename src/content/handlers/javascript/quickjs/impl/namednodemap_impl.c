#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSNamedNodeMap.gen.h"

extern bool wisp_is_js_process;
extern shm_dom_t *wisp_shm_dom;

JSValue wisp_namednodemap_length_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewInt32(ctx, 0);
    if (wisp_is_js_process) {
        uint32_t node_id = (uint32_t)(uintptr_t)priv->node;
        WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[node_id];
        return JS_NewInt32(ctx, sns->attr_count);
    }
    dom_ulong length = 0;
    dom_exception exc = dom_namednodemap_get_length((dom_namednodemap *)priv->node, &length);
    if (exc != DOM_NO_ERR) return JS_NewInt32(ctx, 0);
    return JS_NewInt32(ctx, length);
}

JSValue wisp_namednodemap_item_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) {
        uint32_t node_id = (uint32_t)(uintptr_t)priv->node;
        WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[node_id];
        if (index >= sns->attr_count) return JS_NULL;
        const char *attr_name = wisp_string_ref_data(wisp_shm_dom, sns->attrs[index].name);
        const char *attr_val = wisp_string_ref_data(wisp_shm_dom, sns->attrs[index].value);
        JSValue attr = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, attr, "name", JS_NewString(ctx, attr_name ? attr_name : ""));
        JS_SetPropertyStr(ctx, attr, "value", JS_NewString(ctx, attr_val ? attr_val : ""));
        JS_SetPropertyStr(ctx, attr, "expando", JS_FALSE);
        return attr;
    }
    dom_node *result = NULL;
    dom_exception exc = dom_namednodemap_item((dom_namednodemap *)priv->node, index, &result);
    if (exc != DOM_NO_ERR || !result) return JS_NULL;
    JSValue val = qjs_wrap_node(ctx, result);
    dom_node_unref(result);
    return val;
}

JSValue wisp_namednodemap_getNamedItem_impl(JSContext *ctx, QJSNodePrivate *priv, const char * name)
{
    if (!priv || !priv->node || !name) return JS_NULL;
    if (wisp_is_js_process) {
        uint32_t node_id = (uint32_t)(uintptr_t)priv->node;
        WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[node_id];
        uint32_t limit = sns->attr_count < WISP_SHM_MAX_ATTRIBUTES ? sns->attr_count : WISP_SHM_MAX_ATTRIBUTES;
        for (uint32_t i = 0; i < limit; i++) {
            const char *attr_name = wisp_string_ref_data(wisp_shm_dom, sns->attrs[i].name);
            if (attr_name && strcasecmp(attr_name, name) == 0) {
                const char *attr_val = wisp_string_ref_data(wisp_shm_dom, sns->attrs[i].value);
                JSValue attr = JS_NewObject(ctx);
                JS_SetPropertyStr(ctx, attr, "name", JS_NewString(ctx, attr_name));
                JS_SetPropertyStr(ctx, attr, "value", JS_NewString(ctx, attr_val ? attr_val : ""));
                JS_SetPropertyStr(ctx, attr, "expando", JS_FALSE);
                return attr;
            }
        }
        return JS_NULL;
    }
    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)name, strlen(name), &name_dom);
    if (!name_dom) return JS_NULL;

    dom_node *result = NULL;
    dom_exception exc = dom_namednodemap_get_named_item((dom_namednodemap *)priv->node, name_dom, &result);
    dom_string_unref(name_dom);

    if (exc != DOM_NO_ERR || !result) return JS_NULL;
    JSValue val = qjs_wrap_node(ctx, result);
    dom_node_unref(result);
    return val;
}

JSValue wisp_namednodemap_getNamedItemNS_impl(JSContext *ctx, QJSNodePrivate *priv, const char * namespace, const char * localName)
{
    if (!priv || !priv->node || !localName) return JS_NULL;
    dom_string *ns_dom = NULL;
    dom_string *local_dom = NULL;

    if (namespace) {
        dom_string_create((const uint8_t *)namespace, strlen(namespace), &ns_dom);
    }
    dom_string_create((const uint8_t *)localName, strlen(localName), &local_dom);

    dom_node *result = NULL;
    dom_exception exc = dom_namednodemap_get_named_item_ns((dom_namednodemap *)priv->node, ns_dom, local_dom, &result);

    if (ns_dom) dom_string_unref(ns_dom);
    if (local_dom) dom_string_unref(local_dom);

    if (exc != DOM_NO_ERR || !result) return JS_NULL;
    JSValue val = qjs_wrap_node(ctx, result);
    dom_node_unref(result);
    return val;
}

JSValue wisp_namednodemap_setNamedItem_impl(JSContext *ctx, QJSNodePrivate *priv, void * attr)
{
    if (!priv || !priv->node || !attr) return JS_NULL;
    QJSNodePrivate *attr_priv = (QJSNodePrivate *)attr;
    if (!attr_priv || !attr_priv->node) return JS_NULL;

    dom_node *old_node = NULL;
    dom_exception exc = dom_namednodemap_set_named_item((dom_namednodemap *)priv->node, (dom_node *)attr_priv->node, &old_node);
    if (exc != DOM_NO_ERR) return JS_NULL;

    if (old_node) {
        JSValue val = qjs_wrap_node(ctx, old_node);
        dom_node_unref(old_node);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_namednodemap_setNamedItemNS_impl(JSContext *ctx, QJSNodePrivate *priv, void * attr)
{
    if (!priv || !priv->node || !attr) return JS_NULL;
    QJSNodePrivate *attr_priv = (QJSNodePrivate *)attr;
    if (!attr_priv || !attr_priv->node) return JS_NULL;

    dom_node *old_node = NULL;
    dom_exception exc = dom_namednodemap_set_named_item_ns((dom_namednodemap *)priv->node, (dom_node *)attr_priv->node, &old_node);
    if (exc != DOM_NO_ERR) return JS_NULL;

    if (old_node) {
        JSValue val = qjs_wrap_node(ctx, old_node);
        dom_node_unref(old_node);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_namednodemap_removeNamedItem_impl(JSContext *ctx, QJSNodePrivate *priv, const char * name)
{
    if (!priv || !priv->node || !name) return JS_NULL;
    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)name, strlen(name), &name_dom);
    if (!name_dom) return JS_NULL;

    dom_node *result = NULL;
    dom_exception exc = dom_namednodemap_remove_named_item((dom_namednodemap *)priv->node, name_dom, &result);
    dom_string_unref(name_dom);

    if (exc != DOM_NO_ERR || !result) return JS_NULL;
    JSValue val = qjs_wrap_node(ctx, result);
    dom_node_unref(result);
    return val;
}

JSValue wisp_namednodemap_removeNamedItemNS_impl(JSContext *ctx, QJSNodePrivate *priv, const char * namespace, const char * localName)
{
    if (!priv || !priv->node || !localName) return JS_NULL;
    dom_string *ns_dom = NULL;
    dom_string *local_dom = NULL;

    if (namespace) {
        dom_string_create((const uint8_t *)namespace, strlen(namespace), &ns_dom);
    }
    dom_string_create((const uint8_t *)localName, strlen(localName), &local_dom);

    dom_node *result = NULL;
    dom_exception exc = dom_namednodemap_remove_named_item_ns((dom_namednodemap *)priv->node, ns_dom, local_dom, &result);

    if (ns_dom) dom_string_unref(ns_dom);
    if (local_dom) dom_string_unref(local_dom);

    if (exc != DOM_NO_ERR || !result) return JS_NULL;
    JSValue val = qjs_wrap_node(ctx, result);
    dom_node_unref(result);
    return val;
}

static void namednodemap_finalizer_manual(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_namednodemap_class_id);
    if (priv) {
        if (priv->magic == QJS_DOM_MAGIC && priv->node) {
            if (priv->is_dom_node) {
                JSContext *c = (priv->ctx && JS_ContextIsAlive(rt, priv->ctx)) ? priv->ctx : NULL;
                qjs_bridge_remove_node(rt, (dom_node *)priv->node, c);
                if (!wisp_is_js_process) dom_node_unref((dom_node *)priv->node);
            } else {
                if (!wisp_is_js_process) dom_namednodemap_unref((dom_namednodemap *)priv->node);
            }
        }
        free(priv);
    }
}

JSValue qjs_new_namednodemap(JSContext *ctx, void *node, bool is_dom_node)
{
    JSValue obj = JS_NewObjectClass(ctx, qjs_namednodemap_class_id);
    if (JS_IsException(obj)) return obj;
    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));
    if (!priv) {
        JS_FreeValue(ctx, obj);
        return JS_ThrowOutOfMemory(ctx);
    }
    priv->magic = QJS_DOM_MAGIC;
    priv->node = node;
    priv->is_dom_node = is_dom_node;
    priv->ctx = ctx;
    if (!wisp_is_js_process && node) {
        if (is_dom_node) {
            dom_node_ref((dom_node *)node);
        } else {
            dom_namednodemap_ref((dom_namednodemap *)node);
        }
    }
    JS_SetOpaque(obj, priv);

    /* Wrap in proxy to support indexed attributes access */
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue make_proxy_fn = JS_GetPropertyStr(ctx, global_obj, "__wisp_make_namednodemap_proxy");
    if (JS_IsFunction(ctx, make_proxy_fn)) {
        JSValue proxy_obj = JS_Call(ctx, make_proxy_fn, JS_UNDEFINED, 1, &obj);
        JS_FreeValue(ctx, obj);
        obj = proxy_obj;
    }
    JS_FreeValue(ctx, make_proxy_fn);
    JS_FreeValue(ctx, global_obj);

    return obj;
}

int qjs_init_namednodemap(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_namednodemap_class_id == 0) {
        JS_NewClassID(rt, &qjs_namednodemap_class_id);
    }

    JSClassDef js_namednodemap_class_manual = {
        "NamedNodeMap",
        .finalizer = namednodemap_finalizer_manual,
    };

    if (!JS_IsRegisteredClass(rt, qjs_namednodemap_class_id)) {
        JS_NewClass(rt, qjs_namednodemap_class_id, &js_namednodemap_class_manual);
    }

    qjs_init_namednodemap_gen(ctx);

    /* Define __wisp_make_namednodemap_proxy */
    JSValue global_obj = JS_GetGlobalObject(ctx);
    const char *proxy_js =
        "globalThis.__wisp_make_namednodemap_proxy = function(map) {\n"
        "    return new Proxy(map, {\n"
        "        get(target, prop) {\n"
        "            if (typeof prop !== 'symbol') {\n"
        "                let idx = Number(prop);\n"
        "                if (Number.isInteger(idx) && idx >= 0) {\n"
        "                    return target.item(idx);\n"
        "                }\n"
        "                if (prop !== 'length' && prop !== 'item' && prop !== 'getNamedItem' &&\n"
        "                    prop !== 'getNamedItemNS' && prop !== 'setNamedItem' &&\n"
        "                    prop !== 'setNamedItemNS' && prop !== 'removeNamedItem' &&\n"
        "                    prop !== 'removeNamedItemNS') {\n"
        "                    let attr = target.getNamedItem(prop);\n"
        "                    if (attr !== null) return attr;\n"
        "                }\n"
        "            }\n"
        "            let val = target[prop];\n"
        "            if (typeof val === 'function') {\n"
        "                return val.bind(target);\n"
        "            }\n"
        "            return val;\n"
        "        }\n"
        "    });\n"
        "};";
    JSValue eval_res = JS_Eval(ctx, proxy_js, strlen(proxy_js), "<namednodemap_proxy_init>", JS_EVAL_TYPE_GLOBAL);
    JS_FreeValue(ctx, eval_res);
    JS_FreeValue(ctx, global_obj);

    return 0;
}
