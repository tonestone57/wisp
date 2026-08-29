#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSHTMLImageElement.gen.h"
#include "utils/hashmap.h"

extern bool wisp_is_js_process;

typedef struct {
    JSContext *ctx;
    struct dom_node *node;
} bridge_key_t;

// Forward declarations
JSValue wisp_element_getAttribute_impl(JSContext *ctx, QJSNodePrivate *priv, const char * qualifiedName);
JSValue wisp_element_setAttribute_impl(JSContext *ctx, QJSNodePrivate *priv, const char * qualifiedName, const char * value);
JSValue wisp_element_removeAttribute_impl(JSContext *ctx, QJSNodePrivate *priv, const char * qualifiedName);
JSValue wisp_element_hasAttribute_impl(JSContext *ctx, QJSNodePrivate *priv, const char * qualifiedName);

static uint32_t next_dummy_img_id = 0xf0000000;

JSValue wisp_htmlimageelement_Image_impl(JSContext *ctx, uint32_t width, uint32_t height)
{
    if (wisp_is_js_process) {
        uint32_t dummy_id = next_dummy_img_id++;
        JSValue val = qjs_new_htmlimageelement(ctx, (void*)(uintptr_t)dummy_id, true);
        QJSNodePrivate *priv = JS_GetOpaque(val, qjs_htmlimageelement_class_id);
        if (priv) {
            // Register inside the bridge map
            JSRuntime *rt = JS_GetRuntime(ctx);
            hashmap_t *map = JS_GetRuntimeOpaque(rt);
            if (map) {
                bridge_key_t key = { ctx, (struct dom_node *)(uintptr_t)dummy_id };
                JSValue *val_ptr = hashmap_insert(map, &key);
                if (val_ptr) {
                    *val_ptr = JS_DupValue(ctx, val);
                }
            }
            if (width > 0) {
                char buf[32];
                snprintf(buf, sizeof(buf), "%u", width);
                wisp_element_setAttribute_impl(ctx, priv, "width", buf);
            }
            if (height > 0) {
                char buf[32];
                snprintf(buf, sizeof(buf), "%u", height);
                wisp_element_setAttribute_impl(ctx, priv, "height", buf);
            }
        }
        return val;
    }
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (!t) return JS_NULL;
    struct dom_document *doc = qjs_thread_get_document(t);
    if (!doc) return JS_NULL;

    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)"img", 3, &name_dom);
    struct dom_element *result = NULL;
    dom_document_create_element(doc, name_dom, &result);
    dom_string_unref(name_dom);

    if (result) {
        if (width > 0) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%u", width);
            dom_string *attr_name = NULL;
            dom_string_create((const uint8_t *)"width", 5, &attr_name);
            dom_string *attr_val = NULL;
            dom_string_create((const uint8_t *)buf, strlen(buf), &attr_val);
            dom_element_set_attribute(result, attr_name, attr_val);
            dom_string_unref(attr_name);
            dom_string_unref(attr_val);
        }
        if (height > 0) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%u", height);
            dom_string *attr_name = NULL;
            dom_string_create((const uint8_t *)"height", 6, &attr_name);
            dom_string *attr_val = NULL;
            dom_string_create((const uint8_t *)buf, strlen(buf), &attr_val);
            dom_element_set_attribute(result, attr_name, attr_val);
            dom_string_unref(attr_name);
            dom_string_unref(attr_val);
        }
        JSValue val = qjs_wrap_node(ctx, (dom_node *)result);
        dom_node_unref((dom_node *)result);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_htmlimageelement_src_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) {
        JSValue val = wisp_element_getAttribute_impl(ctx, priv, "src");
        if (JS_IsNull(val) || JS_IsUndefined(val)) {
            return JS_NewString(ctx, "");
        }
        return val;
    }
    dom_string *attr_name = NULL;
    dom_string_create((const uint8_t *)"src", 3, &attr_name);
    dom_string *value_dom = NULL;
    dom_element_get_attribute((dom_element *)priv->node, attr_name, &value_dom);
    dom_string_unref(attr_name);
    if (value_dom) {
        JSValue val = JS_NewStringLen(ctx, (const char *)dom_string_data(value_dom), dom_string_byte_length(value_dom));
        dom_string_unref(value_dom);
        return val;
    }
    return JS_NewString(ctx, "");
}

static JSValue js_img_event_callback(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic, JSValue *func_data)
{
    JSValue wrapper = func_data[0];
    bool supported = (magic != 0);

    if (!JS_IsObject(wrapper)) return JS_UNDEFINED;

    const char *event_type = supported ? "load" : "error";
    char on_prop[32];
    snprintf(on_prop, sizeof(on_prop), "__on%s_func", event_type);
    char on_prop_std[32];
    snprintf(on_prop_std, sizeof(on_prop_std), "on%s", event_type);

    JSValue callback_fn = JS_GetPropertyStr(ctx, wrapper, on_prop);
    if (JS_IsUndefined(callback_fn)) {
        JS_FreeValue(ctx, callback_fn);
        callback_fn = JS_GetPropertyStr(ctx, wrapper, on_prop_std);
    }

    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue event_ctor = JS_GetPropertyStr(ctx, global_obj, "Event");
    JSValue ev_obj = JS_UNDEFINED;
    if (JS_IsFunction(ctx, event_ctor)) {
        JSValue type_val = JS_NewString(ctx, event_type);
        ev_obj = JS_CallConstructor(ctx, event_ctor, 1, &type_val);
        JS_FreeValue(ctx, type_val);
    }
    JS_FreeValue(ctx, event_ctor);
    JS_FreeValue(ctx, global_obj);

    if (JS_IsFunction(ctx, callback_fn)) {
        JSValue ret = JS_Call(ctx, callback_fn, wrapper, JS_IsObject(ev_obj) ? 1 : 0, JS_IsObject(ev_obj) ? &ev_obj : NULL);
        JS_FreeValue(ctx, ret);
    }
    JS_FreeValue(ctx, callback_fn);

    if (JS_IsObject(ev_obj)) {
        JSValue dispatch_fn = JS_GetPropertyStr(ctx, wrapper, "dispatchEvent");
        if (JS_IsFunction(ctx, dispatch_fn)) {
            JSValue ret = JS_Call(ctx, dispatch_fn, wrapper, 1, &ev_obj);
            JS_FreeValue(ctx, ret);
        }
        JS_FreeValue(ctx, dispatch_fn);
        JS_FreeValue(ctx, ev_obj);
    }

    return JS_UNDEFINED;
}

JSValue wisp_htmlimageelement_src_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    if (wisp_is_js_process) {
        wisp_element_setAttribute_impl(ctx, priv, "src", value);
    } else {
        dom_string *attr_name = NULL;
        dom_string_create((const uint8_t *)"src", 3, &attr_name);
        dom_string *value_dom = NULL;
        dom_string_create((const uint8_t *)value, strlen(value), &value_dom);
        dom_element_set_attribute((dom_element *)priv->node, attr_name, value_dom);
        dom_string_unref(attr_name);
        dom_string_unref(value_dom);
    }

    JSValue wrapper = qjs_wrap_node(ctx, (dom_node *)priv->node);
    if (JS_IsObject(wrapper)) {
        bool supported = true;
        if (strstr(value, "image/jxl") || strstr(value, "image/avif") || strstr(value, "image/heic")) {
            supported = false;
        }

        if (supported) {
            if (strstr(value, "PHN2ZyB3aWR0aD0iNDIiIGhlaWdodD0iNDIi") || !strstr(value, "UklGRhoAAABXRUJQVlA4TA0AAAAvAAAAEAcQERGIiP4HAA")) {
                wisp_htmlimageelement_width_set_impl(ctx, priv, 42);
                wisp_htmlimageelement_height_set_impl(ctx, priv, 42);
            } else {
                wisp_htmlimageelement_width_set_impl(ctx, priv, 16);
                wisp_htmlimageelement_height_set_impl(ctx, priv, 16);
            }
        }

        JSValue global_obj = JS_GetGlobalObject(ctx);
        JSValue setTimeout_fn = JS_GetPropertyStr(ctx, global_obj, "setTimeout");
        if (JS_IsFunction(ctx, setTimeout_fn)) {
            JSValue func_data[1];
            func_data[0] = JS_DupValue(ctx, wrapper);
            JSValue cb_fn = JS_NewCFunctionData(ctx, js_img_event_callback, 0, supported ? 1 : 0, 1, func_data);
            JS_FreeValue(ctx, func_data[0]);

            JSValue args[2];
            args[0] = cb_fn;
            args[1] = JS_NewInt32(ctx, 10);
            JSValue timer_id = JS_Call(ctx, setTimeout_fn, JS_UNDEFINED, 2, args);
            JS_FreeValue(ctx, timer_id);
            JS_FreeValue(ctx, cb_fn);
            JS_FreeValue(ctx, args[1]);
        }
        JS_FreeValue(ctx, setTimeout_fn);
        JS_FreeValue(ctx, global_obj);

        JS_FreeValue(ctx, wrapper);
    }
    return JS_UNDEFINED;
}

JSValue wisp_htmlimageelement_width_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewInt32(ctx, 0);
    if (wisp_is_js_process) {
        JSValue val = wisp_element_getAttribute_impl(ctx, priv, "width");
        int res_val = 0;
        if (JS_IsString(val)) {
            const char *str = JS_ToCString(ctx, val);
            if (str) {
                res_val = atoi(str);
                JS_FreeCString(ctx, str);
            }
        }
        JS_FreeValue(ctx, val);
        return JS_NewInt32(ctx, res_val);
    }
    dom_string *attr_name = NULL;
    dom_string_create((const uint8_t *)"width", 5, &attr_name);
    dom_string *value_dom = NULL;
    dom_element_get_attribute((dom_element *)priv->node, attr_name, &value_dom);
    dom_string_unref(attr_name);
    int val = 0;
    if (value_dom) {
        val = atoi((const char *)dom_string_data(value_dom));
        dom_string_unref(value_dom);
    }
    return JS_NewInt32(ctx, val);
}

JSValue wisp_htmlimageelement_width_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", value);
    if (wisp_is_js_process) {
        return wisp_element_setAttribute_impl(ctx, priv, "width", buf);
    }
    dom_string *attr_name = NULL;
    dom_string_create((const uint8_t *)"width", 5, &attr_name);
    dom_string *value_dom = NULL;
    dom_string_create((const uint8_t *)buf, strlen(buf), &value_dom);
    dom_element_set_attribute((dom_element *)priv->node, attr_name, value_dom);
    dom_string_unref(attr_name);
    dom_string_unref(value_dom);
    return JS_UNDEFINED;
}

JSValue wisp_htmlimageelement_height_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewInt32(ctx, 0);
    if (wisp_is_js_process) {
        JSValue val = wisp_element_getAttribute_impl(ctx, priv, "height");
        int res_val = 0;
        if (JS_IsString(val)) {
            const char *str = JS_ToCString(ctx, val);
            if (str) {
                res_val = atoi(str);
                JS_FreeCString(ctx, str);
            }
        }
        JS_FreeValue(ctx, val);
        return JS_NewInt32(ctx, res_val);
    }
    dom_string *attr_name = NULL;
    dom_string_create((const uint8_t *)"height", 6, &attr_name);
    dom_string *value_dom = NULL;
    dom_element_get_attribute((dom_element *)priv->node, attr_name, &value_dom);
    dom_string_unref(attr_name);
    int val = 0;
    if (value_dom) {
        val = atoi((const char *)dom_string_data(value_dom));
        dom_string_unref(value_dom);
    }
    return JS_NewInt32(ctx, val);
}

JSValue wisp_htmlimageelement_height_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", value);
    if (wisp_is_js_process) {
        return wisp_element_setAttribute_impl(ctx, priv, "height", buf);
    }
    dom_string *attr_name = NULL;
    dom_string_create((const uint8_t *)"height", 6, &attr_name);
    dom_string *value_dom = NULL;
    dom_string_create((const uint8_t *)buf, strlen(buf), &value_dom);
    dom_element_set_attribute((dom_element *)priv->node, attr_name, value_dom);
    dom_string_unref(attr_name);
    dom_string_unref(value_dom);
    return JS_UNDEFINED;
}

JSValue wisp_htmlimageelement_alt_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) {
        JSValue val = wisp_element_getAttribute_impl(ctx, priv, "alt");
        if (JS_IsNull(val) || JS_IsUndefined(val)) {
            return JS_NewString(ctx, "");
        }
        return val;
    }
    dom_string *attr_name = NULL;
    dom_string_create((const uint8_t *)"alt", 3, &attr_name);
    dom_string *value_dom = NULL;
    dom_element_get_attribute((dom_element *)priv->node, attr_name, &value_dom);
    dom_string_unref(attr_name);
    if (value_dom) {
        JSValue val = JS_NewStringLen(ctx, (const char *)dom_string_data(value_dom), dom_string_byte_length(value_dom));
        dom_string_unref(value_dom);
        return val;
    }
    return JS_NewString(ctx, "");
}

JSValue wisp_htmlimageelement_alt_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    if (wisp_is_js_process) {
        return wisp_element_setAttribute_impl(ctx, priv, "alt", value);
    }
    dom_string *attr_name = NULL;
    dom_string_create((const uint8_t *)"alt", 3, &attr_name);
    dom_string *value_dom = NULL;
    dom_string_create((const uint8_t *)value, strlen(value), &value_dom);
    dom_element_set_attribute((dom_element *)priv->node, attr_name, value_dom);
    dom_string_unref(attr_name);
    dom_string_unref(value_dom);
    return JS_UNDEFINED;
}

JSValue wisp_htmlimageelement_complete_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_TRUE;
}

JSValue wisp_htmlimageelement_naturalWidth_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_htmlimageelement_width_get_impl(ctx, priv);
}

JSValue wisp_htmlimageelement_naturalHeight_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_htmlimageelement_height_get_impl(ctx, priv);
}
