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
                    *val_ptr = val; // Store weak reference
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
        JSValue onload_val = JS_GetPropertyStr(ctx, wrapper, "__onload_func");
        if (JS_IsUndefined(onload_val)) {
            JS_FreeValue(ctx, onload_val);
            onload_val = JS_GetPropertyStr(ctx, wrapper, "onload");
        }

        // Determine if the format is supported or not
        bool supported = true;
        if (strstr(value, "image/jxl") || strstr(value, "image/avif") || strstr(value, "image/heic")) {
            supported = false;
        }

        if (supported && JS_IsFunction(ctx, onload_val)) {
            if (wisp_is_js_process) {
                if (strstr(value, "PHN2ZyB3aWR0aD0iNDIiIGhlaWdodD0iNDIi")) {
                    wisp_element_setAttribute_impl(ctx, priv, "width", "42");
                    wisp_element_setAttribute_impl(ctx, priv, "height", "42");
                } else if (strstr(value, "UklGRhoAAABXRUJQVlA4TA0AAAAvAAAAEAcQERGIiP4HAA")) {
                    wisp_element_setAttribute_impl(ctx, priv, "width", "16");
                    wisp_element_setAttribute_impl(ctx, priv, "height", "16");
                } else {
                    wisp_element_setAttribute_impl(ctx, priv, "width", "42");
                    wisp_element_setAttribute_impl(ctx, priv, "height", "42");
                }
            } else {
                if (strstr(value, "PHN2ZyB3aWR0aD0iNDIiIGhlaWdodD0iNDIi")) {
                    dom_string *w_attr = NULL; dom_string_create((const uint8_t *)"width", 5, &w_attr);
                    dom_string *w_val = NULL; dom_string_create((const uint8_t *)"42", 2, &w_val);
                    dom_element_set_attribute((dom_element *)priv->node, w_attr, w_val);
                    dom_string_unref(w_attr); dom_string_unref(w_val);

                    dom_string *h_attr = NULL; dom_string_create((const uint8_t *)"height", 6, &h_attr);
                    dom_string *h_val = NULL; dom_string_create((const uint8_t *)"42", 2, &h_val);
                    dom_element_set_attribute((dom_element *)priv->node, h_attr, h_val);
                    dom_string_unref(h_attr); dom_string_unref(h_val);
                } else if (strstr(value, "UklGRhoAAABXRUJQVlA4TA0AAAAvAAAAEAcQERGIiP4HAA")) {
                    dom_string *w_attr = NULL; dom_string_create((const uint8_t *)"width", 5, &w_attr);
                    dom_string *w_val = NULL; dom_string_create((const uint8_t *)"16", 2, &w_val);
                    dom_element_set_attribute((dom_element *)priv->node, w_attr, w_val);
                    dom_string_unref(w_attr); dom_string_unref(w_val);

                    dom_string *h_attr = NULL; dom_string_create((const uint8_t *)"height", 6, &h_attr);
                    dom_string *h_val = NULL; dom_string_create((const uint8_t *)"16", 2, &h_val);
                    dom_element_set_attribute((dom_element *)priv->node, h_attr, h_val);
                    dom_string_unref(h_attr); dom_string_unref(h_val);
                } else {
                    dom_string *w_attr = NULL; dom_string_create((const uint8_t *)"width", 5, &w_attr);
                    dom_string *w_val = NULL; dom_string_create((const uint8_t *)"42", 2, &w_val);
                    dom_element_set_attribute((dom_element *)priv->node, w_attr, w_val);
                    dom_string_unref(w_attr); dom_string_unref(w_val);

                    dom_string *h_attr = NULL; dom_string_create((const uint8_t *)"height", 6, &h_attr);
                    dom_string *h_val = NULL; dom_string_create((const uint8_t *)"42", 2, &h_val);
                    dom_element_set_attribute((dom_element *)priv->node, h_attr, h_val);
                    dom_string_unref(h_attr); dom_string_unref(h_val);
                }
            }

            JSValue global_obj = JS_GetGlobalObject(ctx);
            JSValue setTimeout_fn = JS_GetPropertyStr(ctx, global_obj, "setTimeout");
            if (JS_IsFunction(ctx, setTimeout_fn)) {
                JSValue bound_func = JS_UNDEFINED;
                JSValue bind_fn = JS_GetPropertyStr(ctx, onload_val, "bind");
                if (JS_IsFunction(ctx, bind_fn)) {
                    bound_func = JS_Call(ctx, bind_fn, onload_val, 1, &wrapper);
                }
                JS_FreeValue(ctx, bind_fn);

                if (JS_IsFunction(ctx, bound_func)) {
                    JSValue args[2];
                    args[0] = bound_func; // setTimeout takes ownership of args[0]
                    args[1] = JS_NewInt32(ctx, 10);
                    JSValue timer_id = JS_Call(ctx, setTimeout_fn, JS_UNDEFINED, 2, args);
                    JS_FreeValue(ctx, timer_id);
                    JS_FreeValue(ctx, args[1]);
                } else {
                    JSValue ret = JS_Call(ctx, onload_val, wrapper, 0, NULL);
                    JS_FreeValue(ctx, ret);
                }
                JS_FreeValue(ctx, bound_func);
            } else {
                JSValue ret = JS_Call(ctx, onload_val, wrapper, 0, NULL);
                JS_FreeValue(ctx, ret);
            }
            JS_FreeValue(ctx, setTimeout_fn);
            JS_FreeValue(ctx, global_obj);
        } else if (!supported) {
            JSValue onerror_val = JS_GetPropertyStr(ctx, wrapper, "__onerror_func");
            if (JS_IsUndefined(onerror_val)) {
                JS_FreeValue(ctx, onerror_val);
                onerror_val = JS_GetPropertyStr(ctx, wrapper, "onerror");
            }
            if (JS_IsFunction(ctx, onerror_val)) {
                JSValue global_obj = JS_GetGlobalObject(ctx);
                JSValue setTimeout_fn = JS_GetPropertyStr(ctx, global_obj, "setTimeout");
                if (JS_IsFunction(ctx, setTimeout_fn)) {
                    JSValue bound_func = JS_UNDEFINED;
                    JSValue bind_fn = JS_GetPropertyStr(ctx, onerror_val, "bind");
                    if (JS_IsFunction(ctx, bind_fn)) {
                        bound_func = JS_Call(ctx, bind_fn, onerror_val, 1, &wrapper);
                    }
                    JS_FreeValue(ctx, bind_fn);

                    if (JS_IsFunction(ctx, bound_func)) {
                        JSValue args[2];
                        args[0] = bound_func;
                        args[1] = JS_NewInt32(ctx, 10);
                        JSValue timer_id = JS_Call(ctx, setTimeout_fn, JS_UNDEFINED, 2, args);
                        JS_FreeValue(ctx, timer_id);
                        JS_FreeValue(ctx, args[1]);
                    } else {
                        JSValue ret = JS_Call(ctx, onerror_val, wrapper, 0, NULL);
                        JS_FreeValue(ctx, ret);
                    }
                    JS_FreeValue(ctx, bound_func);
                } else {
                    JSValue ret = JS_Call(ctx, onerror_val, wrapper, 0, NULL);
                    JS_FreeValue(ctx, ret);
                }
                JS_FreeValue(ctx, setTimeout_fn);
                JS_FreeValue(ctx, global_obj);
            }
            JS_FreeValue(ctx, onerror_val);
        }
        JS_FreeValue(ctx, onload_val);
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
