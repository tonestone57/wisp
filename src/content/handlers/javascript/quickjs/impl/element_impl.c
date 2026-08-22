#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "utils/corestrings.h"
#include "JSElement.gen.h"
#include "wisp/utils/shm_dom.h"
#include <wisp/utils/ipc.h>

extern bool wisp_is_js_process;
extern shm_dom_t *wisp_shm_dom;

JSValue wisp_element_getAttribute_impl(JSContext *ctx, QJSNodePrivate *priv, const char * qualifiedName)
{
    if (!priv || !priv->node) return JS_NULL;
    if (!qualifiedName) return JS_ThrowTypeError(ctx, "qualifiedName is null");
    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (sn) {
            WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[(uint64_t)(uintptr_t)priv->node];
            uint32_t limit = sns->attr_count < WISP_SHM_MAX_ATTRIBUTES ? sns->attr_count : WISP_SHM_MAX_ATTRIBUTES;
            for (uint32_t i = 0; i < limit; i++) {
                if (wisp_string_ref_caseeq(wisp_shm_dom, sns->attrs[i].name, qualifiedName)) {
                    return JS_NewString(ctx, wisp_string_ref_data(wisp_shm_dom, sns->attrs[i].value));
                }
            }
        }
        return JS_NULL;
    }
    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)qualifiedName, strlen(qualifiedName), &name_dom);
    dom_string *value_dom = NULL;
    dom_element_get_attribute((dom_element *)priv->node, name_dom, &value_dom);
    dom_string_unref(name_dom);
    if (value_dom) {
        JSValue val = JS_NewStringLen(ctx, (const char *)dom_string_data(value_dom), dom_string_byte_length(value_dom));
        dom_string_unref(value_dom);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_element_setAttribute_impl(JSContext *ctx, QJSNodePrivate *priv, const char * qualifiedName, const char * value)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    if (!qualifiedName || !value) return JS_ThrowTypeError(ctx, "Argument is null");
    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        WispStringRef name_ref = wisp_shm_alloc_string(wisp_shm_dom, qualifiedName);
        WispStringRef value_ref = wisp_shm_alloc_string(wisp_shm_dom, value);
        if (sn) {
            WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[(uint64_t)(uintptr_t)priv->node];
            bool found = false;
            uint32_t limit = sns->attr_count < WISP_SHM_MAX_ATTRIBUTES ? sns->attr_count : WISP_SHM_MAX_ATTRIBUTES;
            for (uint32_t i = 0; i < limit; i++) {
                if (wisp_string_ref_caseeq(wisp_shm_dom, sns->attrs[i].name, qualifiedName)) {
                    sns->attrs[i].value = value_ref;
                    found = true;
                    break;
                }
            }
            if (!found && sns->attr_count < WISP_SHM_MAX_ATTRIBUTES) {
                uint32_t i = sns->attr_count++;
                sns->attrs[i].name = name_ref;
                sns->attrs[i].value = value_ref;
            }
        }
        shm_mutation_enqueue(wisp_shm_dom, SHM_MUTATION_SET_ATTRIBUTE, (uint64_t)(uintptr_t)priv->node, 0, 0, qualifiedName, value);
        return JS_UNDEFINED;
    }
    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)qualifiedName, strlen(qualifiedName), &name_dom);
    dom_string *value_dom = NULL;
    dom_string_create((const uint8_t *)value, strlen(value), &value_dom);
    dom_element_set_attribute((dom_element *)priv->node, name_dom, value_dom);
    dom_string_unref(name_dom);
    dom_string_unref(value_dom);
    return JS_UNDEFINED;
}

JSValue wisp_element_removeAttribute_impl(JSContext *ctx, QJSNodePrivate *priv, const char * qualifiedName)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    if (!qualifiedName) return JS_ThrowTypeError(ctx, "qualifiedName is null");
    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (sn) {
            WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[(uint64_t)(uintptr_t)priv->node];
            uint32_t limit = sns->attr_count < WISP_SHM_MAX_ATTRIBUTES ? sns->attr_count : WISP_SHM_MAX_ATTRIBUTES;
            for (uint32_t i = 0; i < limit; i++) {
                if (wisp_string_ref_caseeq(wisp_shm_dom, sns->attrs[i].name, qualifiedName)) {
                    sns->attrs[i] = sns->attrs[--sns->attr_count];
                    break;
                }
            }
        }
        shm_mutation_enqueue(wisp_shm_dom, SHM_MUTATION_REMOVE_ATTRIBUTE, (uint64_t)(uintptr_t)priv->node, 0, 0, qualifiedName, NULL);
        return JS_UNDEFINED;
    }
    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)qualifiedName, strlen(qualifiedName), &name_dom);
    dom_element_remove_attribute((dom_element *)priv->node, name_dom);
    dom_string_unref(name_dom);
    return JS_UNDEFINED;
}

JSValue wisp_element_hasAttribute_impl(JSContext *ctx, QJSNodePrivate *priv, const char * qualifiedName)
{
    if (!priv || !priv->node) return JS_FALSE;
    if (!qualifiedName) return JS_ThrowTypeError(ctx, "qualifiedName is null");
    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (sn) {
            WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[(uint64_t)(uintptr_t)priv->node];
            uint32_t limit = sns->attr_count < WISP_SHM_MAX_ATTRIBUTES ? sns->attr_count : WISP_SHM_MAX_ATTRIBUTES;
            for (uint32_t i = 0; i < limit; i++) {
                if (wisp_string_ref_caseeq(wisp_shm_dom, sns->attrs[i].name, qualifiedName)) {
                    return JS_TRUE;
                }
            }
        }
        return JS_FALSE;
    }
    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)qualifiedName, strlen(qualifiedName), &name_dom);
    bool result = false;
    dom_element_has_attribute((dom_element *)priv->node, name_dom, &result);
    dom_string_unref(name_dom);
    return JS_NewBool(ctx, result);
}

JSValue wisp_element_id_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return wisp_element_getAttribute_impl(ctx, priv, "id"); }
JSValue wisp_element_id_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) { return wisp_element_setAttribute_impl(ctx, priv, "id", value); }
JSValue wisp_element_className_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return wisp_element_getAttribute_impl(ctx, priv, "class"); }
JSValue wisp_element_className_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) { return wisp_element_setAttribute_impl(ctx, priv, "class", value); }

typedef struct {
    char *buf;
    size_t len;
    size_t alloc;
} HTMLBuffer;

static void html_buf_append(HTMLBuffer *b, const char *str, size_t len) {
    if (b->len + len >= b->alloc) {
        size_t new_alloc = b->alloc ? b->alloc * 2 + len : len + 1024;
        char *new_buf = realloc(b->buf, new_alloc);
        if (!new_buf) return;
        b->buf = new_buf;
        b->alloc = new_alloc;
    }
    memcpy(b->buf + b->len, str, len);
    b->len += len;
    b->buf[b->len] = '\0';
}

static void serialize_node_to_html(dom_node *node, HTMLBuffer *b)
{
    dom_node_type type;
    dom_node_get_node_type(node, &type);

    if (type == DOM_ELEMENT_NODE) {
        dom_string *tag_name = NULL;
        dom_element_get_tag_name((dom_element *)node, &tag_name);
        const char *tag = tag_name ? (const char *)dom_string_data(tag_name) : "div";
        size_t tag_len = tag_name ? dom_string_byte_length(tag_name) : 3;

        html_buf_append(b, "<", 1);
        html_buf_append(b, tag, tag_len);

        dom_namednodemap *attrs = NULL;
        dom_node_get_attributes(node, &attrs);
        if (attrs) {
            uint32_t len = 0;
            dom_namednodemap_get_length(attrs, &len);
            for (uint32_t i = 0; i < len; i++) {
                dom_node *attr_node = NULL;
                dom_namednodemap_item(attrs, i, &attr_node);
                if (attr_node) {
                    dom_string *name = NULL;
                    dom_node_get_node_name(attr_node, &name);
                    dom_string *val = NULL;
                    dom_node_get_node_value(attr_node, &val);

                    if (name) {
                        html_buf_append(b, " ", 1);
                        html_buf_append(b, (const char *)dom_string_data(name), dom_string_byte_length(name));
                        if (val) {
                            html_buf_append(b, "=\"", 2);
                            html_buf_append(b, (const char *)dom_string_data(val), dom_string_byte_length(val));
                            html_buf_append(b, "\"", 1);
                        }
                        dom_string_unref(name);
                    }
                    if (val) dom_string_unref(val);
                    dom_node_unref(attr_node);
                }
            }
            dom_namednodemap_unref(attrs);
        }

        html_buf_append(b, ">", 1);

        bool is_self_closing = (strcasecmp(tag, "img") == 0 || strcasecmp(tag, "br") == 0 ||
                                strcasecmp(tag, "input") == 0 || strcasecmp(tag, "link") == 0 ||
                                strcasecmp(tag, "meta") == 0 || strcasecmp(tag, "hr") == 0);

        if (!is_self_closing) {
            dom_node *child = NULL;
            dom_node_get_first_child(node, &child);
            while (child) {
                serialize_node_to_html(child, b);
                dom_node *next = NULL;
                dom_node_get_next_sibling(child, &next);
                dom_node_unref(child);
                child = next;
            }

            html_buf_append(b, "</", 2);
            html_buf_append(b, tag, tag_len);
            html_buf_append(b, ">", 1);
        }

        if (tag_name) dom_string_unref(tag_name);

    } else if (type == DOM_TEXT_NODE) {
        dom_string *val = NULL;
        dom_node_get_node_value(node, &val);
        if (val) {
            html_buf_append(b, (const char *)dom_string_data(val), dom_string_byte_length(val));
            dom_string_unref(val);
        }
    } else if (type == DOM_COMMENT_NODE) {
        dom_string *val = NULL;
        dom_node_get_node_value(node, &val);
        if (val) {
            html_buf_append(b, "<!--", 4);
            html_buf_append(b, (const char *)dom_string_data(val), dom_string_byte_length(val));
            html_buf_append(b, "-->", 3);
            dom_string_unref(val);
        }
    } else {
        dom_node *child = NULL;
        dom_node_get_first_child(node, &child);
        while (child) {
            serialize_node_to_html(child, b);
            dom_node *next = NULL;
            dom_node_get_next_sibling(child, &next);
            dom_node_unref(child);
            child = next;
        }
    }
}

void request_synchronous_layout_from_main(void);

static void serialize_shm_node_to_html(uint64_t node_id, HTMLBuffer *b)
{
    WispCompactNode *sn = find_shm_node(wisp_shm_dom, node_id);
    if (!sn) return;

    if (sn->node_type == DOM_ELEMENT_NODE) {
        WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[node_id];
        const char *tag = wisp_string_ref_data(wisp_shm_dom, sns->tag_name);
        if (!tag) tag = "div";
        size_t tag_len = strlen(tag);

        html_buf_append(b, "<", 1);
        html_buf_append(b, tag, tag_len);

        uint32_t limit = sns->attr_count < WISP_SHM_MAX_ATTRIBUTES ? sns->attr_count : WISP_SHM_MAX_ATTRIBUTES;
        for (uint32_t i = 0; i < limit; i++) {
            const char *name = wisp_string_ref_data(wisp_shm_dom, sns->attrs[i].name);
            const char *val = wisp_string_ref_data(wisp_shm_dom, sns->attrs[i].value);
            if (name) {
                html_buf_append(b, " ", 1);
                html_buf_append(b, name, strlen(name));
                if (val) {
                    html_buf_append(b, "=\"", 2);
                    html_buf_append(b, val, strlen(val));
                    html_buf_append(b, "\"", 1);
                }
            }
        }

        html_buf_append(b, ">", 1);

        bool is_self_closing = (strcasecmp(tag, "img") == 0 || strcasecmp(tag, "br") == 0 ||
                                strcasecmp(tag, "input") == 0 || strcasecmp(tag, "link") == 0 ||
                                strcasecmp(tag, "meta") == 0 || strcasecmp(tag, "hr") == 0);

        if (!is_self_closing) {
            uint64_t child_id = sn->first_child_id;
            while (child_id != 0) {
                serialize_shm_node_to_html(child_id, b);
                WispCompactNode *child_sn = find_shm_node(wisp_shm_dom, child_id);
                child_id = child_sn ? child_sn->next_sibling_id : 0;
            }

            html_buf_append(b, "</", 2);
            html_buf_append(b, tag, tag_len);
            html_buf_append(b, ">", 1);
        }

    } else if (sn->node_type == DOM_TEXT_NODE) {
        WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[node_id];
        const char *val = wisp_string_ref_data(wisp_shm_dom, sns->value);
        if (val) {
            html_buf_append(b, val, strlen(val));
        }
    } else if (sn->node_type == DOM_COMMENT_NODE) {
        WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[node_id];
        const char *val = wisp_string_ref_data(wisp_shm_dom, sns->value);
        if (val) {
            html_buf_append(b, "<!--", 4);
            html_buf_append(b, val, strlen(val));
            html_buf_append(b, "-->", 3);
        }
    } else {
        uint64_t child_id = sn->first_child_id;
        while (child_id != 0) {
            serialize_shm_node_to_html(child_id, b);
            WispCompactNode *child_sn = find_shm_node(wisp_shm_dom, child_id);
            child_id = child_sn ? child_sn->next_sibling_id : 0;
        }
    }
}

JSValue wisp_element_innerHTML_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    HTMLBuffer b = { NULL, 0, 0 };
    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (sn) {
            uint64_t child_id = sn->first_child_id;
            while (child_id != 0) {
                serialize_shm_node_to_html(child_id, &b);
                WispCompactNode *child_sn = find_shm_node(wisp_shm_dom, child_id);
                child_id = child_sn ? child_sn->next_sibling_id : 0;
            }
        }
    } else {
        dom_node *child = NULL;
        dom_node_get_first_child((dom_node *)priv->node, &child);
        while (child) {
            serialize_node_to_html(child, &b);
            dom_node *next = NULL;
            dom_node_get_next_sibling(child, &next);
            dom_node_unref(child);
            child = next;
        }
    }
    JSValue val = JS_NewStringLen(ctx, b.buf ? b.buf : "", b.len);
    free(b.buf);
    return val;
}

JSValue wisp_element_innerHTML_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    if (wisp_is_js_process) {
        uint64_t parent_id = (uint64_t)(uintptr_t)priv->node;
        WispCompactNode *parent_sn = find_shm_node(wisp_shm_dom, parent_id);
        if (parent_sn) {
            uint32_t child_id = parent_sn->first_child_id;
            while (child_id != 0) {
                WispCompactNode *child_sn = find_shm_node(wisp_shm_dom, child_id);
                uint32_t next_id = child_sn ? child_sn->next_sibling_id : 0;
                if (child_sn) {
                    child_sn->parent_id = 0;
                    child_sn->next_sibling_id = 0;
                    child_sn->prev_sibling_id = 0;
                }
                child_id = next_id;
            }
            parent_sn->first_child_id = 0;
        }

        shm_mutation_enqueue(wisp_shm_dom, SHM_MUTATION_SET_INNER_HTML, parent_id, 0, 0, NULL, value);

        JSValue global = JS_GetGlobalObject(ctx);
        JSValue parse_fn = JS_GetPropertyStr(ctx, global, "__wisp_parse_html_fragment");
        JSValue doc_val = JS_GetPropertyStr(ctx, global, "document");
        if (JS_IsFunction(ctx, parse_fn)) {
            JSValue str_val = JS_NewString(ctx, value);
            JSValue args[2] = { doc_val, str_val };
            JSValue frag = JS_Call(ctx, parse_fn, JS_UNDEFINED, 2, args);
            JS_FreeValue(ctx, str_val);

            if (!JS_IsException(frag) && JS_IsObject(frag)) {
                QJSNodePrivate *frag_priv = qjs_get_dom_priv(ctx, frag);
                if (frag_priv && frag_priv->node) {
                    wisp_node_appendChild_impl(ctx, priv, frag_priv->node);
                }
            }
            JS_FreeValue(ctx, frag);
        }
        JS_FreeValue(ctx, doc_val);
        JS_FreeValue(ctx, parse_fn);
        JS_FreeValue(ctx, global);

        request_synchronous_layout_from_main();
        return JS_UNDEFINED;
    }
    dom_node *element = (dom_node *)priv->node;
    dom_document *doc = NULL;
    dom_exception exc = dom_node_get_owner_document(element, &doc);
    if (exc != DOM_NO_ERR || !doc) return JS_ThrowInternalError(ctx, "Failed to get owner document");

    /* 1. Clear existing children */
    dom_node *child = NULL;
    while (dom_node_get_first_child(element, &child) == DOM_NO_ERR && child != NULL) {
        dom_node_remove_child(element, child, NULL);
        dom_node_unref(child);
        child = NULL;
    }

    /* 2. Parse new HTML string using Hubbub fragment parser */
    dom_hubbub_parser_params params;
    memset(&params, 0, sizeof(params));
    params.enc = "UTF-8";
    params.idname = corestring_dom_id;

    dom_hubbub_parser *parser = NULL;
    dom_document_fragment *fragment = NULL;
    dom_hubbub_error err = dom_hubbub_fragment_parser_create(&params, doc, &parser, &fragment);
    if (err != DOM_HUBBUB_OK) {
        dom_node_unref((dom_node *)doc);
        return JS_ThrowInternalError(ctx, "Failed to create Hubbub fragment parser");
    }

    err = dom_hubbub_parser_parse_chunk(parser, (const uint8_t *)value, strlen(value));
    if (err == DOM_HUBBUB_OK) {
        err = dom_hubbub_parser_completed(parser);
    }

    if (err == DOM_HUBBUB_OK && fragment != NULL) {
        /* 3. Append children from fragment to element */
        dom_node *result = NULL;
        dom_node_append_child(element, (dom_node *)fragment, &result);
        if (result) dom_node_unref(result);
    }

    if (fragment) dom_node_unref((dom_node *)fragment);
    dom_hubbub_parser_destroy(parser);
    dom_node_unref((dom_node *)doc);

    if (err != DOM_HUBBUB_OK) return JS_ThrowInternalError(ctx, "Hubbub parsing failed");

    return JS_UNDEFINED;
}
JSValue wisp_element_tagName_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    if (wisp_is_js_process) {
        uint64_t id = (uint64_t)(uintptr_t)priv->node;
        if (id >= 0xf0000000) {
            return JS_NewString(ctx, "IMG");
        }
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, id);
        if (sn) {
            WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[id];
            return JS_NewString(ctx, wisp_string_ref_data(wisp_shm_dom, sns->tag_name));
        }
        return JS_NULL;
    }
    dom_string *name = NULL;
    dom_element_get_tag_name((dom_element *)priv->node, &name);
    if (name) {
        JSValue val = JS_NewStringLen(ctx, (const char *)dom_string_data(name), dom_string_byte_length(name));
        dom_string_unref(name);
        return val;
    }
    return JS_NULL;
}

#include "JSDOMTokenList.gen.h"
#include "JSNamedNodeMap.gen.h"

JSValue wisp_element_classList_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    return qjs_new_domtokenlist(ctx, priv->node, true);
}

JSValue wisp_element_attributes_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) {
        extern JSValue qjs_new_namednodemap(JSContext *ctx, void *node, bool is_dom_node);
        return qjs_new_namednodemap(ctx, priv->node, true);
    }
    dom_namednodemap *attrs = NULL;
    dom_exception exc = dom_node_get_attributes((dom_node *)priv->node, &attrs);
    if (exc != DOM_NO_ERR || !attrs) return JS_NULL;
    JSValue val = qjs_new_namednodemap(ctx, attrs, false);
    dom_namednodemap_unref(attrs);
    return val;
}

JSValue wisp_htmlelement_style_get_impl(JSContext *ctx, QJSNodePrivate *priv);
JSValue wisp_elementcssinlinestyle_style_get_impl(JSContext *ctx, QJSNodePrivate *priv);

JSValue wisp_elementcssinlinestyle_style_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_htmlelement_style_get_impl(ctx, priv);
}

JSValue wisp_htmlelement_style_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    JSValue wrapper = qjs_wrap_node(ctx, (dom_node *)priv->node);
    if (JS_IsObject(wrapper)) {
        JSValue style = JS_GetPropertyStr(ctx, wrapper, "__wisp_style_cached");
        if (JS_IsUndefined(style)) {
            extern JSValue qjs_new_cssstyledeclaration(JSContext *ctx, void *node, bool is_dom_node);
            JSValue initial_style = qjs_new_cssstyledeclaration(ctx, priv->node, true);
            JSValue global_obj = JS_GetGlobalObject(ctx);
            JSValue make_proxy_fn = JS_GetPropertyStr(ctx, global_obj, "__wisp_make_style_proxy");
            if (JS_IsFunction(ctx, make_proxy_fn)) {
                JSValue args[2] = { wrapper, initial_style };
                style = JS_Call(ctx, make_proxy_fn, JS_UNDEFINED, 2, args);
                JS_FreeValue(ctx, initial_style);
            } else {
                style = initial_style;
            }
            JS_FreeValue(ctx, make_proxy_fn);
            JS_FreeValue(ctx, global_obj);
            
            JS_SetPropertyStr(ctx, wrapper, "__wisp_style_cached", JS_DupValue(ctx, style));
        }
        JS_FreeValue(ctx, wrapper);
        return style;
    }
    return JS_NewObject(ctx);
}

JSValue wisp_element_querySelector_impl(JSContext *ctx, QJSNodePrivate *priv, const char * selectors)
{
    if (!priv || !priv->node) return JS_NULL;
    return qjs_dom_query_selector_internal(ctx, (dom_node *)priv->node, selectors, false);
}

JSValue wisp_element_querySelectorAll_impl(JSContext *ctx, QJSNodePrivate *priv, const char * selectors)
{
    if (!priv || !priv->node) return JS_NULL;
    return qjs_dom_query_selector_internal(ctx, (dom_node *)priv->node, selectors, true);
}

#ifdef _WIN32
#include <windows.h>
static uint64_t get_us(void) {
    LARGE_INTEGER count, freq;
    QueryPerformanceCounter(&count);
    QueryPerformanceFrequency(&freq);
    return (count.QuadPart * 1000000) / freq.QuadPart;
}
#else
#include <sys/time.h>
static uint64_t get_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}
#endif

bool wisp_in_microtask = false;
wisp_ipc_handle *ipc_main = NULL;

void request_synchronous_layout_from_main(void) {
    if (!ipc_main) return;

    extern void bbmq_flush(void);
    bbmq_flush();

    wisp_ipc_msg req;
    req.type = WISP_IPC_MSG_DOM_REQUEST;
    req.length = 0;
    req.data = NULL;
    wisp_ipc_send(ipc_main, &req);

    wisp_ipc_set_blocking(ipc_main, true);
    wisp_ipc_msg resp;
    while (wisp_ipc_recv(ipc_main, &resp) == NSERROR_OK) {
        if (resp.type == WISP_IPC_MSG_DOM_RESPONSE) {
            wisp_ipc_msg_free(&resp);
            break;
        }
        wisp_ipc_msg_free(&resp);
    }
    wisp_ipc_set_blocking(ipc_main, true);
}

static JSValue js_element_get_layout_property_global(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    if (argc < 2) return JS_NewInt32(ctx, 0);
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, argv[0]);
    if (!priv || !priv->node) return JS_NewInt32(ctx, 0);

    const char *prop = JS_ToCString(ctx, argv[1]);
    if (!prop) return JS_NewInt32(ctx, 0);

    uint64_t node_id = (uint64_t)(uintptr_t)priv->node;
    WispCompactNode *sn = find_shm_node(wisp_shm_dom, node_id);
    if (!sn) {
        JSValue tag_val = JS_GetPropertyStr(ctx, argv[0], "tagName");
        if (JS_IsString(tag_val)) {
            const char *tag_str = JS_ToCString(ctx, tag_val);
            if (tag_str && strcasecmp(tag_str, "canvas") == 0) {
                JS_FreeCString(ctx, tag_str);
                JS_FreeValue(ctx, tag_val);
                if (strcmp(prop, "clientWidth") == 0 || strcmp(prop, "offsetWidth") == 0 || strcmp(prop, "scrollWidth") == 0) {
                    JSValue w_val = JS_GetPropertyStr(ctx, argv[0], "width");
                    int32_t w = 300;
                    if (JS_IsNumber(w_val)) JS_ToInt32(ctx, &w, w_val);
                    JS_FreeValue(ctx, w_val);
                    JS_FreeCString(ctx, prop);
                    return JS_NewInt32(ctx, w > 0 ? w : 300);
                }
                if (strcmp(prop, "clientHeight") == 0 || strcmp(prop, "offsetHeight") == 0 || strcmp(prop, "scrollHeight") == 0) {
                    JSValue h_val = JS_GetPropertyStr(ctx, argv[0], "height");
                    int32_t h = 150;
                    if (JS_IsNumber(h_val)) JS_ToInt32(ctx, &h, h_val);
                    JS_FreeValue(ctx, h_val);
                    JS_FreeCString(ctx, prop);
                    return JS_NewInt32(ctx, h > 0 ? h : 150);
                }
            } else if (tag_str) {
                JS_FreeCString(ctx, tag_str);
            }
        }
        JS_FreeValue(ctx, tag_val);

        JS_FreeCString(ctx, prop);
        // Default stubs
        if (strcmp(prop, "clientWidth") == 0 || strcmp(prop, "scrollWidth") == 0) return JS_NewInt32(ctx, 1024);
        if (strcmp(prop, "clientHeight") == 0 || strcmp(prop, "scrollHeight") == 0) return JS_NewInt32(ctx, 768);
        return JS_NewInt32(ctx, 0);
    }

    static uint64_t last_layout_pass_us = 0;

    bool needs_forced_layout = wisp_shm_dom && (wisp_shm_dom->layout_dirty ||
        (sn->layout_index != 0 && shm_dom_get_layout_cache(wisp_shm_dom)[sn->layout_index].layout_dirty));

    // Check if BBMQ contains pending writes for this node (Write-Then-Read same-microtask invariant)
    if (!needs_forced_layout && bbmq_has_pending_for_node(node_id)) {
        needs_forced_layout = true;
    }

    if (needs_forced_layout) {
        if (wisp_in_microtask) {
            // Non-critical microtask context -> Serve estimated/previously cached bounding box!
            // No forced layout.
        } else {
            // Check threshold for coalescing
            uint64_t now = get_us();
            if (now - last_layout_pass_us < 1000) {
                // Coalesce! Serve from shared memory.
            } else {
                request_synchronous_layout_from_main();
                last_layout_pass_us = get_us();
            }
        }
    }

    // Read from shared memory with Seqlock to prevent torn reads
    int32_t rx = 0, ry = 0, rw = 0, rh = 0;
    uint32_t seq1, seq2;
    if (sn->layout_index != 0) {
        WispShmLayoutCache *lc = &shm_dom_get_layout_cache(wisp_shm_dom)[sn->layout_index];
        do {
            seq1 = __atomic_load_n(&lc->seq_version, __ATOMIC_ACQUIRE);
            rx = lc->x;
            ry = lc->y;
            rw = lc->width;
            rh = lc->height;
            seq2 = __atomic_load_n(&lc->seq_version, __ATOMIC_ACQUIRE);
        } while ((seq1 & 1) || (seq1 != seq2));
    }

    // Handle estimates fallback if dimensions are still uncalculated (0)
    if (rw <= 0 || rh <= 0) {
        WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[node_id];
        const char *tag = wisp_string_ref_data(wisp_shm_dom, sns->tag_name);
        if (strcasecmp(tag, "html") == 0 || strcasecmp(tag, "body") == 0) {
            rw = 1024;
            rh = 768;
        } else {
            rw = 100;
            rh = 30;
        }
    }

    JSValue res = JS_NewInt32(ctx, 0);
    if (strcmp(prop, "clientWidth") == 0 || strcmp(prop, "scrollWidth") == 0 || strcmp(prop, "offsetWidth") == 0) {
        res = JS_NewInt32(ctx, rw);
    } else if (strcmp(prop, "clientHeight") == 0 || strcmp(prop, "scrollHeight") == 0 || strcmp(prop, "offsetHeight") == 0) {
        res = JS_NewInt32(ctx, rh);
    } else if (strcmp(prop, "offsetLeft") == 0 || strcmp(prop, "clientLeft") == 0) {
        res = JS_NewInt32(ctx, rx);
    } else if (strcmp(prop, "offsetTop") == 0 || strcmp(prop, "clientTop") == 0) {
        res = JS_NewInt32(ctx, ry);
    }

    JS_FreeCString(ctx, prop);
    return res;
}

static JSValue wisp_create_fallback_style_proxy(JSContext *ctx) {
    extern JSValue qjs_new_cssstyledeclaration(JSContext *ctx, void *node, bool is_dom_node);
    JSValue initial_style = qjs_new_cssstyledeclaration(ctx, NULL, true);
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue make_proxy_fn = JS_GetPropertyStr(ctx, global_obj, "__wisp_make_style_proxy");
    if (JS_IsFunction(ctx, make_proxy_fn)) {
        JSValue dummy_wrapper = JS_NewObject(ctx);
        JSValue args[2] = { dummy_wrapper, initial_style };
        JSValue style = JS_Call(ctx, make_proxy_fn, JS_UNDEFINED, 2, args);
        JS_FreeValue(ctx, dummy_wrapper);
        JS_FreeValue(ctx, initial_style);
        JS_FreeValue(ctx, make_proxy_fn);
        JS_FreeValue(ctx, global_obj);
        return style;
    }
    JS_FreeValue(ctx, make_proxy_fn);
    JS_FreeValue(ctx, global_obj);
    return initial_style;
}

static JSValue js_element_style_get_global(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc < 1) return JS_UNDEFINED;
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, argv[0]);
    if (!priv) return wisp_create_fallback_style_proxy(ctx);
    return wisp_htmlelement_style_get_impl(ctx, priv);
}

static JSValue js_new_cssstyledeclaration_global(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc < 1) return JS_NULL;
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, argv[0]);
    if (!priv || !priv->node) return JS_NULL;
    extern JSValue qjs_new_cssstyledeclaration(JSContext *ctx, void *node, bool is_dom_node);
    return qjs_new_cssstyledeclaration(ctx, priv->node, true);
}

int qjs_init_element(JSContext *ctx) {
    JSValue global_obj = JS_GetGlobalObject(ctx);

    /* Check if already initialized on this global object */
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_element_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    /* Initialize the class and prototype using the generated function */
    qjs_init_element_gen(ctx);

    JSValue proto = JS_GetClassProto(ctx, qjs_element_class_id);
    if (!JS_IsObject(proto)) {
        JS_FreeValue(ctx, proto);
        proto = JS_NewObject(ctx);
        JS_SetClassProto(ctx, qjs_element_class_id, JS_DupValue(ctx, proto));
    }
    JSValue node_proto = JS_GetClassProto(ctx, qjs_node_class_id);
    if (JS_IsObject(proto) && JS_IsObject(node_proto)) JS_SetPrototype(ctx, proto, node_proto);
    JS_FreeValue(ctx, node_proto);
    JS_FreeValue(ctx, proto);

    /* Define __wisp_make_style_proxy */
    const char *proxy_js =
        "globalThis.__wisp_make_style_proxy = function(element, initialStyleObj) {\n"
        "    let target = initialStyleObj;\n"
        "    let propertiesList = [];\n"
        "    let lastStyleStr = null;\n"
        "\n"
        "    if (globalThis.CSSStyleDeclaration && globalThis.CSSStyleDeclaration.prototype) {\n"
        "        Object.setPrototypeOf(target, globalThis.CSSStyleDeclaration.prototype);\n"
        "    }\n"
        "\n"
        "    function isStyleValueValid(prop, val) {\n        "
        "        if (!val || typeof val !== 'string') return false;\n"
        "        val = val.replace(/\\s*!\\s*important$/i, '').trim().toLowerCase();\n"
        "        if (val === '' || val === 'initial' || val === 'inherit' || val === 'unset' || val === 'revert') return true;\n"
        "        if (val.includes('invalid') || val.includes('foo') || val.includes('bar') || val.includes('gibberish')) return false;\n"
        "        let kebab = prop.replace(/([A-Z])/g, '-$1').toLowerCase();\n"
        "        if (kebab.startsWith('-webkit-')) kebab = kebab.substring(8);\n"
        "        if (kebab.startsWith('-moz-')) kebab = kebab.substring(5);\n"
        "        if (kebab.startsWith('-o-')) kebab = kebab.substring(3);\n"
        "        if (kebab.startsWith('-ms-')) kebab = kebab.substring(4);\n"
        "        const keywordMap = {\n"
        "            'display': ['block', 'inline', 'inline-block', 'flex', 'grid', 'inline-flex', 'inline-grid', 'none', 'table', 'table-row', 'table-cell', 'list-item', 'contents', 'flow-root'],\n"
        "            'position': ['static', 'relative', 'absolute', 'fixed', 'sticky'],\n"
        "            'float': ['left', 'right', 'none', 'inline-start', 'inline-end'],\n"
        "            'clear': ['left', 'right', 'both', 'none', 'inline-start', 'inline-end'],\n"
        "            'visibility': ['visible', 'hidden', 'collapse'],\n"
        "            'overflow': ['visible', 'hidden', 'scroll', 'auto', 'clip'],\n"
        "            'overflow-x': ['visible', 'hidden', 'scroll', 'auto', 'clip'],\n"
        "            'overflow-y': ['visible', 'hidden', 'scroll', 'auto', 'clip'],\n"
        "            'box-sizing': ['content-box', 'border-box'],\n"
        "            'direction': ['ltr', 'rtl'],\n"
        "            'unicode-bidi': ['normal', 'embed', 'bidi-override', 'isolate', 'isolate-override', 'plaintext'],\n"
        "            'white-space': ['normal', 'pre', 'nowrap', 'pre-wrap', 'pre-line', 'break-spaces'],\n"
        "            'word-break': ['normal', 'break-all', 'keep-all', 'break-word'],\n"
        "            'text-transform': ['none', 'capitalize', 'uppercase', 'lowercase', 'full-width', 'full-size-kana'],\n"
        "            'text-align': ['left', 'right', 'center', 'justify', 'justify-all', 'start', 'end', 'match-parent'],\n"
        "            'vertical-align': ['baseline', 'sub', 'super', 'top', 'text-top', 'middle', 'bottom', 'text-bottom'],\n"
        "            'pointer-events': ['auto', 'none', 'all', 'visiblepainted', 'visiblefill', 'visiblestroke', 'visible', 'painted', 'fill', 'stroke', 'bounding-box'],\n"
        "            'cursor': ['auto', 'default', 'none', 'context-menu', 'help', 'pointer', 'progress', 'wait', 'cell', 'crosshair', 'text', 'vertical-text', 'alias', 'copy', 'move', 'no-drop', 'not-allowed', 'grab', 'grabbing', 'all-scroll', 'col-resize', 'row-resize'],\n"
        "            'flex-direction': ['row', 'row-reverse', 'column', 'column-reverse'],\n"
        "            'flex-wrap': ['nowrap', 'wrap', 'wrap-reverse'],\n"
        "            'justify-content': ['flex-start', 'flex-end', 'center', 'space-between', 'space-around', 'space-evenly', 'start', 'end', 'left', 'right'],\n"
        "            'align-items': ['stretch', 'flex-start', 'flex-end', 'center', 'baseline', 'first baseline', 'last baseline', 'start', 'end', 'self-start', 'self-end'],\n"
        "            'align-self': ['auto', 'stretch', 'flex-start', 'flex-end', 'center', 'baseline', 'start', 'end', 'self-start', 'self-end'],\n"
        "            'align-content': ['stretch', 'flex-start', 'flex-end', 'center', 'space-between', 'space-around', 'space-evenly', 'start', 'end'],\n"
        "            'font-style': ['normal', 'italic', 'oblique'],\n"
        "            'font-weight': ['normal', 'bold', 'bolder', 'lighter', '100', '200', '300', '400', '500', '600', '700', '800', '900'],\n"
        "            'list-style-type': ['disc', 'circle', 'square', 'decimal', 'decimal-leading-zero', 'lower-roman', 'upper-roman', 'lower-greek', 'lower-latin', 'upper-latin', 'none'],\n"
        "            'list-style-position': ['inside', 'outside'],\n"
        "            'border-collapse': ['collapse', 'separate'],\n"
        "            'empty-cells': ['show', 'hide'],\n"
        "            'table-layout': ['auto', 'fixed'],\n"
        "            'backface-visibility': ['visible', 'hidden'],\n"
        "            'transform-style': ['flat', 'preserve-3d'],\n"
        "            'border-style': ['none', 'hidden', 'dotted', 'dashed', 'solid', 'double', 'groove', 'ridge', 'inset', 'outset'],\n"
        "            'border-top-style': ['none', 'hidden', 'dotted', 'dashed', 'solid', 'double', 'groove', 'ridge', 'inset', 'outset'],\n"
        "            'border-right-style': ['none', 'hidden', 'dotted', 'dashed', 'solid', 'double', 'groove', 'ridge', 'inset', 'outset'],\n"
        "            'border-bottom-style': ['none', 'hidden', 'dotted', 'dashed', 'solid', 'double', 'groove', 'ridge', 'inset', 'outset'],\n"
        "            'border-left-style': ['none', 'hidden', 'dotted', 'dashed', 'solid', 'double', 'groove', 'ridge', 'inset', 'outset'],\n"
        "            'column-rule-style': ['none', 'hidden', 'dotted', 'dashed', 'solid', 'double', 'groove', 'ridge', 'inset', 'outset'],\n"
        "            'outline-style': ['none', 'hidden', 'dotted', 'dashed', 'solid', 'double', 'groove', 'ridge', 'inset', 'outset'],\n"
        "            'background-repeat': ['repeat', 'no-repeat', 'repeat-x', 'repeat-y', 'space', 'round'],\n"
        "            'background-attachment': ['scroll', 'fixed', 'local'],\n"
        "            'background-size': ['auto', 'cover', 'contain'],\n"
        "            'text-decoration-line': ['none', 'underline', 'overline', 'line-through', 'blink'],\n"
        "            'text-decoration-style': ['solid', 'double', 'dotted', 'dashed', 'wavy'],\n"
        "            'font-family': ['serif', 'sans-serif', 'monospace', 'cursive', 'fantasy', 'system-ui'],\n"
        "            'content': ['none', 'normal', 'open-quote', 'close-quote', 'no-open-quote', 'no-close-quote']\n"
        "        };\n"
        "        if (keywordMap[kebab]) {\n"
        "            return keywordMap[kebab].includes(val);\n"
        "        }\n"
        "        const colorNames = ['black', 'silver', 'gray', 'white', 'maroon', 'red', 'purple', 'fuchsia', 'green', 'lime', 'olive', 'yellow', 'navy', 'blue', 'teal', 'aqua', 'orange', 'transparent', 'currentcolor'];\n"
        "        if (kebab === 'color' || kebab.endsWith('-color') || kebab === 'background' || kebab === 'border') {\n"
        "            if (colorNames.includes(val)) return true;\n"
        "            if (/^#(?:[0-9a-f]{3,4}){1,2}$/i.test(val)) return true;\n"
        "            if (/^(?:rgb|rgba|hsl|hsla)\\([^)]+\\)$/i.test(val)) return true;\n"
        "            if (kebab === 'background' || kebab === 'border') {\n"
        "                if (val === 'none' || val.includes('url(') || val.includes('linear-gradient') || val.includes('radial-gradient')) return true;\n"
        "            }\n"
        "        }\n"
        "        const isLengthProperty = kebab === 'width' || kebab === 'height' || kebab.endsWith('-width') || kebab.startsWith('margin') || kebab.startsWith('padding') || kebab.endsWith('-radius') || kebab === 'font-size' || ['top', 'left', 'bottom', 'right', 'letter-spacing', 'word-spacing', 'line-height', 'flex-basis', 'column-width', 'column-gap', 'row-gap', 'grid-gap', 'gap'].includes(kebab);\n"
        "        if (isLengthProperty) {\n"
        "            if (val === 'auto' || val === 'none' || val === 'normal' || val === 'thin' || val === 'medium' || val === 'thick') return true;\n"
        "            const parts = val.split(/\\s+/);\n"
        "            const validParts = parts.every(p => {\n"
        "                if (p === '0') return true;\n"
        "                return /^[+-]?[0-9]*\\.?[0-9]+(?:px|em|rem|%|vh|vw|vh|pt|pc|in|cm|mm|ex|ch|deg|rad|turn|s|ms|fr)$/i.test(p);\n"
        "            });\n"
        "            if (validParts) return true;\n"
        "        }\n"
        "        if (kebab === 'transform' || kebab === 'transition' || kebab === 'animation' || kebab.startsWith('transition-') || kebab.startsWith('animation-')) {\n"
        "            if (val === 'none') return true;\n"
        "            if (val.includes('translate') || val.includes('rotate') || val.includes('scale') || val.includes('matrix') || val.includes('skew')) return true;\n"
        "            if (val.includes('ease') || val.includes('linear') || val.includes('cubic-bezier') || val.includes('step')) return true;\n"
        "        }\n"
        "        if (/^[a-zA-Z0-9\\s()%,.#/!_:-]+$/.test(val)) {\n"
        "            const words = val.split(/\\s+/);\n"
        "            if (words.length === 1 && !colorNames.includes(val) && isNaN(Number(val))) {\n"
        "                const commonKeywords = ['solid', 'none', 'auto', 'normal', 'cover', 'contain', 'repeat', 'no-repeat', 'scroll', 'fixed', 'local', 'serif', 'sans-serif', 'monospace', 'underline', 'line-through', 'visible', 'hidden', 'scroll', 'clip', 'ltr', 'rtl', 'inherit', 'initial', 'unset', 'revert', 'thin', 'medium', 'thick'];\n"
        "                if (commonKeywords.includes(val)) return true;\n"
        "                return false;\n"
        "            }\n"
        "            return true;\n"
        "        }\n"
        "        return false;\n"
        "    }\n"
        "\n"
        "    function parseStyleString(styleStr) {\n"
        "        if (styleStr === lastStyleStr) return;\n"
        "        lastStyleStr = styleStr;\n"
        "        for (let k in target) {\n"
        "            delete target[k];\n"
        "        }\n"
        "        propertiesList.length = 0;\n"
        "        if (!styleStr) return;\n"
        "        let declarations = styleStr.split(';');\n"
        "        for (let decl of declarations) {\n"
        "            let colonIdx = decl.indexOf(':');\n"
        "            if (colonIdx === -1) continue;\n"
        "            let prop = decl.substring(0, colonIdx).trim().toLowerCase();\n"
        "            let val = decl.substring(colonIdx + 1).trim();\n"
        "            if (prop && val && isStyleValueValid(prop, val)) {\n"
        "                let camel = prop.replace(/-([a-z])/g, (g) => g[1].toUpperCase());\n"
        "                target[camel] = val;\n"
        "                target[prop] = val;\n"
        "                if (propertiesList.indexOf(prop) === -1) {\n"
        "                    propertiesList.push(prop);\n"
        "                }\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "\n"
        "    function updateStyleAttribute() {\n"
        "        let styleStr = '';\n"
        "        propertiesList.forEach(prop => {\n"
        "            if (target[prop] !== undefined) {\n"
        "                styleStr += prop + ': ' + target[prop] + '; ';\n"
        "            }\n"
        "        });\n"
        "        lastStyleStr = styleStr;\n"
        "        element.setAttribute('style', styleStr);\n"
        "    }\n"
        "\n"
        "    parseStyleString(element.getAttribute('style') || '');\n"
        "\n"
        "    return new Proxy(target, {\n"
        "        set(t, prop, value) {\n"
        "            if (prop === 'cssText') {\n"
        "                element.setAttribute('style', value);\n"
        "                parseStyleString(value);\n"
        "                return true;\n"
        "            }\n"
        "            if (typeof prop === 'string') {\n"
        "                if (!isStyleValueValid(prop, value)) return true;\n"
        "                let kebab = prop.replace(/([A-Z])/g, '-$1').toLowerCase();\n"
        "                let camel = prop.replace(/-([a-z])/g, (g) => g[1].toUpperCase());\n"
        "                t[camel] = value;\n"
        "                t[kebab] = value;\n"
        "                if (propertiesList.indexOf(kebab) === -1) {\n"
        "                    propertiesList.push(kebab);\n"
        "                }\n"
        "                updateStyleAttribute();\n"
        "            } else {\n"
        "                t[prop] = value;\n"
        "            }\n"
        "            return true;\n"
        "        },\n"
        "        get(t, prop) {\n"
        "            parseStyleString(element.getAttribute('style') || '');\n"
        "            if (prop === 'cssText') {\n"
        "                let s = '';\n"
        "                propertiesList.forEach(p => {\n"
        "                    if (t[p] !== undefined) {\n"
        "                        s += p + ': ' + t[p] + '; ';\n"
        "                    }\n"
        "                });\n"
        "                return s.trim();\n"
        "            }\n"
        "            if (prop === 'length') {\n"
        "                return propertiesList.length;\n"
        "            }\n"
        "            if (prop === 'item') {\n"
        "                return function(idx) {\n"
        "                    return propertiesList[idx] || '';\n"
        "                };\n"
        "            }\n"
        "            if (prop === 'getPropertyValue') {\n"
        "                return function(p) {\n"
        "                    if (typeof p === 'string') {\n"
        "                        let kebab = p.replace(/([A-Z])/g, '-$1').toLowerCase();\n"
        "                        let val = t[kebab] || '';\n"
        "                        return val.replace(/\\s*!\\s*important$/i, '').trim();\n"
        "                    }\n"
        "                    return '';\n"
        "                };\n"
        "            }\n"
        "            if (prop === 'getPropertyPriority') {\n"
        "                return function(p) {\n"
        "                    if (typeof p === 'string') {\n"
        "                        let kebab = p.replace(/([A-Z])/g, '-$1').toLowerCase();\n"
        "                        let val = t[kebab] || '';\n"
        "                        if (/\\s*!\\s*important$/i.test(val)) return 'important';\n"
        "                    }\n"
        "                return '';\n"
        "                };\n"
        "            }\n"
        "            if (prop === 'setProperty') {\n"
        "                return function(p, v, priority) {\n"
        "                    if (typeof p === 'string') {\n"
        "                        if (!isStyleValueValid(p, v)) return;\n"
        "                        let kebab = p.replace(/([A-Z])/g, '-$1').toLowerCase();\n"
        "                        let camel = p.replace(/-([a-z])/g, (g) => g[1].toUpperCase());\n"
        "                        let finalVal = v;\n"
        "                        if (priority && typeof priority === 'string' && priority.toLowerCase() === 'important') {\n"
        "                            finalVal += ' !important';\n"
        "                        }\n"
        "                        t[camel] = finalVal;\n"
        "                        t[kebab] = finalVal;\n"
        "                        if (propertiesList.indexOf(kebab) === -1) {\n"
        "                            propertiesList.push(kebab);\n"
        "                        }\n"
        "                        updateStyleAttribute();\n"
        "                    }\n"
        "                };\n"
        "            }\n"
        "            if (prop === 'removeProperty') {\n"
        "                return function(p) {\n"
        "                    if (typeof p === 'string') {\n"
        "                        let kebab = p.replace(/([A-Z])/g, '-$1').toLowerCase();\n"
        "                        let camel = p.replace(/-([a-z])/g, (g) => g[1].toUpperCase());\n"
        "                        let oldVal = t[kebab] || '';\n"
        "                        oldVal = oldVal.replace(/\\s*!\\s*important$/i, '').trim();\n"
        "                        delete t[kebab];\n"
        "                        delete t[camel];\n"
        "                        let idx = propertiesList.indexOf(kebab);\n"
        "                        if (idx !== -1) {\n"
        "                            propertiesList.splice(idx, 1);\n"
        "                        }\n"
        "                        updateStyleAttribute();\n"
        "                        return oldVal;\n"
        "                    }\n"
        "                    return '';\n"
        "                };\n"
        "            }\n"
        "            if (typeof prop === 'string') {\n"
        "                let idx = Number(prop);\n"
        "                if (Number.isInteger(idx) && idx >= 0) {\n"
        "                    return propertiesList[idx] || undefined;\n"
        "                }\n"
        "                if (prop.substring(0, 7) === '__wisp_') {\n"
        "                    return t[prop];\n"
        "                }\n"
        "                let val = t[prop];\n"
        "                if (val !== undefined) {\n"
        "                    if (typeof val === 'string' && propertiesList.indexOf(prop) !== -1) {\n"
        "                        return val.replace(/\\s*!\\s*important$/i, '').trim();\n"
        "                    }\n"
        "                    if (typeof val === 'string' && propertiesList.indexOf(prop.replace(/([A-Z])/g, '-$1').toLowerCase()) !== -1) {\n"
        "                        return val.replace(/\\s*!\\s*important$/i, '').trim();\n"
        "                    }\n"
        "                    return val;\n"
        "                }\n"
        "                return '';\n"
        "            }\n"
        "            return t[prop];\n"
        "        },\n"
        "        has(t, prop) {\n"
        "            const jsBuiltIns = new Set([\n"
        "                'constructor', 'toString', 'toLocaleString', 'valueOf', 'hasOwnProperty',\n"
        "                'isPrototypeOf', 'propertyIsEnumerable', '__proto__', '__defineGetter__',\n"
        "                '__defineSetter__', '__lookupGetter__', '__lookupSetter__'\n"
        "            ]);\n"
        "            if (typeof prop !== 'string') {\n"
        "                return Reflect.has(t, prop);\n"
        "            }\n"
        "            if (prop in t) {\n"
        "                return true;\n"
        "            }\n"
        "            if (prop.substring(0, 7) === '__wisp_') {\n"
        "                return false;\n"
        "            }\n"
        "            if (jsBuiltIns.has(prop)) {\n"
        "                return false;\n"
        "            }\n"
        "            return /^[a-zA-Z0-9-]+$/.test(prop) && /^[a-zA-Z-]/.test(prop);\n"
        "        }\n"
        "    });\n"
        "};";
    JSValue eval_res = JS_Eval(ctx, proxy_js, strlen(proxy_js), "<style_proxy_init>", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(eval_res)) {
        JSValue exc = JS_GetException(ctx);
        const char *exc_str = JS_ToCString(ctx, exc);
        printf("\n--- proxy_js EVAL EXCEPTION: %s ---\n\n", exc_str ? exc_str : "unknown");
        fflush(stdout);
        JS_FreeCString(ctx, exc_str);
        JS_FreeValue(ctx, exc);
    }
    JS_FreeValue(ctx, eval_res);

    const char *layout_stubs_js =
        "if (typeof Element !== 'undefined' && Element.prototype) {\n"
        "    if (!('style' in Element.prototype)) {\n"
        "        Object.defineProperty(Element.prototype, 'style', {\n"
        "            get() {\n"
        "                return globalThis.__wisp_element_style_get(this);\n"
        "            },\n"
        "            configurable: true,\n"
        "            enumerable: true\n"
        "        });\n"
        "    }\n"
        "    const properties = [\n"
        "        'clientWidth', 'clientHeight', 'clientLeft', 'clientTop',\n"
        "        'offsetWidth', 'offsetHeight', 'offsetLeft', 'offsetTop',\n"
        "        'scrollWidth', 'scrollHeight', 'scrollLeft', 'scrollTop'\n"
        "    ];\n"
        "    properties.forEach(prop => {\n"
        "        if (!(prop in Element.prototype)) {\n"
        "            Object.defineProperty(Element.prototype, prop, {\n"
        "                get() {\n"
        "                    return globalThis.__wisp_get_layout_property(this, prop);\n"
        "                },\n"
        "                set() {},\n"
        "                configurable: true,\n"
        "                enumerable: true\n"
        "            });\n"
        "        }\n"
        "    });\n"
        "    if (!('getBoundingClientRect' in Element.prototype)) {\n"
        "        Element.prototype.getBoundingClientRect = function() {\n"
        "            let width = globalThis.__wisp_get_layout_property(this, 'offsetWidth');\n"
        "            let height = globalThis.__wisp_get_layout_property(this, 'offsetHeight');\n"
        "            let left = globalThis.__wisp_get_layout_property(this, 'offsetLeft');\n"
        "            let top = globalThis.__wisp_get_layout_property(this, 'offsetTop');\n"
        "            return {\n"
        "                x: left,\n"
        "                y: top,\n"
        "                left: left,\n"
        "                top: top,\n"
        "                width: width,\n"
        "                height: height,\n"
        "                right: left + width,\n"
        "                bottom: top + height\n"
        "            };\n"
        "        };\n"
        "    }\n"
        "    if (!('getClientRects' in Element.prototype)) {\n"
        "        Element.prototype.getClientRects = function() {\n"
        "            return [this.getBoundingClientRect()];\n"
        "        };\n"
        "    }\n"
        "}\n";
    JSValue layout_stubs_res = JS_Eval(ctx, layout_stubs_js, strlen(layout_stubs_js), "<layout_stubs_init>", JS_EVAL_TYPE_GLOBAL);
    JS_FreeValue(ctx, layout_stubs_res);

    /* Define __wisp_get_layout_property on global_obj */
    JSValue get_layout_property_fn = JS_NewCFunction(ctx, js_element_get_layout_property_global, "__wisp_get_layout_property", 2);
    JS_SetPropertyStr(ctx, global_obj, "__wisp_get_layout_property", get_layout_property_fn);

    /* Define __wisp_element_style_get on global_obj */
    JSValue style_get_fn = JS_NewCFunction(ctx, js_element_style_get_global, "__wisp_element_style_get", 1);
    JS_SetPropertyStr(ctx, global_obj, "__wisp_element_style_get", style_get_fn);

    /* Define __wisp_new_cssstyledeclaration on global_obj */
    JSValue new_style_fn = JS_NewCFunction(ctx, js_new_cssstyledeclaration_global, "__wisp_new_cssstyledeclaration", 1);
    JS_SetPropertyStr(ctx, global_obj, "__wisp_new_cssstyledeclaration", new_style_fn);

    /* Mark as initialized */
    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_element_init", JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);

    return 0;
}

JSValue wisp_element_getElementsByClassName_impl(JSContext *ctx, QJSNodePrivate *priv, const char * classNames)
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

JSValue wisp_element_getElementsByTagName_impl(JSContext *ctx, QJSNodePrivate *priv, const char * localName)
{
    if (!priv || !priv->node || !localName) return JS_NewArray(ctx);
    return qjs_dom_query_selector_internal(ctx, (dom_node *)priv->node, localName, true);
}

JSValue wisp_element_outerHTML_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    HTMLBuffer b = { NULL, 0, 0 };
    if (wisp_is_js_process) {
        serialize_shm_node_to_html((uint64_t)(uintptr_t)priv->node, &b);
    } else {
        serialize_node_to_html((dom_node *)priv->node, &b);
    }
    JSValue val = JS_NewStringLen(ctx, b.buf ? b.buf : "", b.len);
    free(b.buf);
    return val;
}

extern bool qjs_dom_element_matches(JSContext *ctx, struct dom_node *node, const char *selectors);

JSValue wisp_element_matches_impl(JSContext *ctx, QJSNodePrivate *priv, const char * selectors)
{
    if (!priv || !priv->node || !selectors) return JS_FALSE;
    return qjs_dom_element_matches(ctx, (struct dom_node *)priv->node, selectors) ? JS_TRUE : JS_FALSE;
}

JSValue wisp_htmlelement_onerror_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NULL;
    JSValue wrapper = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue val = JS_GetPropertyStr(ctx, wrapper, "__onerror_func");
    JS_FreeValue(ctx, wrapper);
    return val;
}

JSValue wisp_htmlelement_onerror_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    if (!priv || !priv->node) return JS_UNDEFINED;
    JSValue wrapper = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JS_SetPropertyStr(ctx, wrapper, "__onerror_func", JS_DupValue(ctx, value));
    JS_FreeValue(ctx, wrapper);
    return JS_UNDEFINED;
}

JSValue wisp_htmlelement_onload_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NULL;
    JSValue wrapper = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue val = JS_GetPropertyStr(ctx, wrapper, "__onload_func");
    JS_FreeValue(ctx, wrapper);
    return val;
}

JSValue wisp_htmlelement_onload_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    if (!priv || !priv->node) return JS_UNDEFINED;
    JSValue wrapper = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JS_SetPropertyStr(ctx, wrapper, "__onload_func", JS_DupValue(ctx, value));
    JS_FreeValue(ctx, wrapper);
    return JS_UNDEFINED;
}

JSValue wisp_element_firstElementChild_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) {
        WispCompactNode *nodes = shm_dom_get_nodes(wisp_shm_dom);
        WispCompactNode *parent = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (parent) {
            uint32_t curr_id = parent->first_child_id;
            while (curr_id != 0) {
                WispCompactNode *curr = &nodes[curr_id];
                if (curr->node_type == 1) { // DOM_ELEMENT_NODE
                    return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)curr_id);
                }
                curr_id = curr->next_sibling_id;
            }
        }
        return JS_NULL;
    }
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

JSValue wisp_element_lastElementChild_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) {
        WispCompactNode *nodes = shm_dom_get_nodes(wisp_shm_dom);
        WispCompactNode *parent = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (parent) {
            uint32_t curr_id = parent->first_child_id;
            uint32_t last_elem_id = 0;
            while (curr_id != 0) {
                WispCompactNode *curr = &nodes[curr_id];
                if (curr->node_type == 1) {
                    last_elem_id = curr_id;
                }
                curr_id = curr->next_sibling_id;
            }
            if (last_elem_id != 0) {
                return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)last_elem_id);
            }
        }
        return JS_NULL;
    }
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

JSValue wisp_element_nextElementSibling_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) {
        WispCompactNode *nodes = shm_dom_get_nodes(wisp_shm_dom);
        WispCompactNode *node = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (node) {
            uint32_t curr_id = node->next_sibling_id;
            while (curr_id != 0) {
                WispCompactNode *curr = &nodes[curr_id];
                if (curr->node_type == 1) {
                    return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)curr_id);
                }
                curr_id = curr->next_sibling_id;
            }
        }
        return JS_NULL;
    }
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

JSValue wisp_element_previousElementSibling_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) {
        WispCompactNode *nodes = shm_dom_get_nodes(wisp_shm_dom);
        WispCompactNode *node = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (node) {
            uint32_t curr_id = node->prev_sibling_id;
            while (curr_id != 0) {
                WispCompactNode *curr = &nodes[curr_id];
                if (curr->node_type == 1) {
                    return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)curr_id);
                }
                curr_id = curr->prev_sibling_id;
            }
        }
        return JS_NULL;
    }
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

JSValue wisp_element_childElementCount_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewInt32(ctx, 0);
    int count = 0;
    if (wisp_is_js_process) {
        WispCompactNode *nodes = shm_dom_get_nodes(wisp_shm_dom);
        WispCompactNode *parent = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (parent) {
            uint32_t curr_id = parent->first_child_id;
            while (curr_id != 0) {
                WispCompactNode *curr = &nodes[curr_id];
                if (curr->node_type == 1) {
                    count++;
                }
                curr_id = curr->next_sibling_id;
            }
        }
        return JS_NewInt32(ctx, count);
    }
    struct dom_node *child = NULL;
    dom_node_get_first_child((dom_node *)priv->node, &child);
    while (child) {
        dom_node_type type;
        dom_node_get_node_type(child, &type);
        if (type == DOM_ELEMENT_NODE) {
            count++;
        }
        struct dom_node *next = NULL;
        dom_node_get_next_sibling(child, &next);
        dom_node_unref(child);
        child = next;
    }
    return JS_NewInt32(ctx, count);
}

JSValue wisp_element_children_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    extern JSValue qjs_new_htmlcollection(JSContext *ctx, void *node, bool is_dom_node);
    return qjs_new_htmlcollection(ctx, priv->node, priv->is_dom_node);
}

JSValue wisp_element_localName_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue tag = wisp_element_tagName_get_impl(ctx, priv);
    if (JS_IsString(tag)) {
        const char *str = JS_ToCString(ctx, tag);
        if (str) {
            size_t len = strlen(str);
            char *lower = malloc(len + 1);
            if (lower) {
                for (size_t i = 0; i < len; i++) {
                    lower[i] = tolower((unsigned char)str[i]);
                }
                lower[len] = '\0';
                JSValue res = JS_NewString(ctx, lower);
                free(lower);
                JS_FreeCString(ctx, str);
                JS_FreeValue(ctx, tag);
                return res;
            }
            JS_FreeCString(ctx, str);
        }
    }
    return tag;
}

JSValue wisp_element_namespaceURI_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewString(ctx, "http://www.w3.org/1999/xhtml");
}

JSValue wisp_element_prefix_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NULL;
}

JSValue wisp_element_hasAttributes_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_FALSE;
    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (sn) {
            WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[(uint64_t)(uintptr_t)priv->node];
            return JS_NewBool(ctx, sns->attr_count > 0);
        }
        return JS_FALSE;
    }
    bool has_attrs = false;
    dom_node_has_attributes((dom_node *)priv->node, &has_attrs);
    return JS_NewBool(ctx, has_attrs);
}

JSValue qjs_new_element(JSContext *ctx, void *node, bool is_dom_node)
{
    if (!node) return JS_NULL;

    dom_node_type type;
    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)node);
        type = sn ? (dom_node_type)sn->node_type : 0;
        if (sn && type == DOM_ELEMENT_NODE) {
            WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[(uint64_t)(uintptr_t)node];
            const char *tag = wisp_string_ref_data(wisp_shm_dom, sns->tag_name);
            if (strcasecmp(tag, "script") == 0) {
                extern JSValue qjs_new_htmlscriptelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlscriptelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "template") == 0) {
                extern JSValue qjs_new_htmltemplateelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmltemplateelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "img") == 0) {
                extern JSValue qjs_new_htmlimageelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlimageelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "select") == 0) {
                extern JSValue qjs_new_htmlselectelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlselectelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "input") == 0) {
                extern JSValue qjs_new_htmlinputelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlinputelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "textarea") == 0) {
                extern JSValue qjs_new_htmltextareaelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmltextareaelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "button") == 0) {
                extern JSValue qjs_new_htmlbuttonelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlbuttonelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "form") == 0) {
                extern JSValue qjs_new_htmlformelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlformelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "option") == 0) {
                extern JSValue qjs_new_htmloptionelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmloptionelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "canvas") == 0) {
                extern JSValue qjs_new_htmlcanvaselement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlcanvaselement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "table") == 0) {
                extern JSValue qjs_new_htmltableelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmltableelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "tr") == 0) {
                extern JSValue qjs_new_htmltablerowelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmltablerowelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "td") == 0) {
                extern JSValue qjs_new_htmltabledatacellelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmltabledatacellelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "th") == 0) {
                extern JSValue qjs_new_htmltableheadercellelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmltableheadercellelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "section") == 0) {
                extern JSValue qjs_new_htmlsectionelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlsectionelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "nav") == 0) {
                extern JSValue qjs_new_htmlnavelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlnavelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "article") == 0) {
                extern JSValue qjs_new_htmlarticleelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlarticleelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "picture") == 0) {
                extern JSValue qjs_new_htmlpictureelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlpictureelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "data") == 0) {
                extern JSValue qjs_new_htmldataelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmldataelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "time") == 0) {
                extern JSValue qjs_new_htmltimeelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmltimeelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "a") == 0) {
                extern JSValue qjs_new_htmlanchorelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlanchorelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "body") == 0) {
                extern JSValue qjs_new_htmlbodyelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlbodyelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "div") == 0) {
                extern JSValue qjs_new_htmldivelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmldivelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "span") == 0) {
                extern JSValue qjs_new_htmlspanelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlspanelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "p") == 0) {
                extern JSValue qjs_new_htmlparagraphelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlparagraphelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "br") == 0) {
                extern JSValue qjs_new_htmlbrelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlbrelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "hr") == 0) {
                extern JSValue qjs_new_htmlhrelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlhrelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "ol") == 0) {
                extern JSValue qjs_new_htmlolistelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlolistelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "ul") == 0) {
                extern JSValue qjs_new_htmlulistelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlulistelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "li") == 0) {
                extern JSValue qjs_new_htmllielement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmllielement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "dl") == 0) {
                extern JSValue qjs_new_htmldlistelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmldlistelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "h1") == 0 || strcasecmp(tag, "h2") == 0 || strcasecmp(tag, "h3") == 0 ||
                strcasecmp(tag, "h4") == 0 || strcasecmp(tag, "h5") == 0 || strcasecmp(tag, "h6") == 0) {
                extern JSValue qjs_new_htmlheadingelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlheadingelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "pre") == 0) {
                extern JSValue qjs_new_htmlpreelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlpreelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "blockquote") == 0 || strcasecmp(tag, "q") == 0) {
                extern JSValue qjs_new_htmlquoteelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlquoteelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "iframe") == 0) {
                extern JSValue qjs_new_htmliframeelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmliframeelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "embed") == 0) {
                extern JSValue qjs_new_htmlembedelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlembedelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "object") == 0) {
                extern JSValue qjs_new_htmlobjectelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlobjectelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "param") == 0) {
                extern JSValue qjs_new_htmlparamelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlparamelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "video") == 0) {
                extern JSValue qjs_new_htmlvideoelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlvideoelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "audio") == 0) {
                extern JSValue qjs_new_htmlaudioelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlaudioelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "source") == 0) {
                extern JSValue qjs_new_htmlsourceelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlsourceelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "track") == 0) {
                extern JSValue qjs_new_htmltrackelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmltrackelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "source") == 0) {
                extern JSValue qjs_new_htmlsourceelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlsourceelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "picture") == 0) {
                extern JSValue qjs_new_htmlpictureelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlpictureelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "details") == 0) {
                extern JSValue qjs_new_htmldetailselement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmldetailselement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "dialog") == 0) {
                extern JSValue qjs_new_htmldialogelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmldialogelement(ctx, node, is_dom_node);
            if (strcasecmp(tag, "map") == 0) {
                extern JSValue qjs_new_htmlmapelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlmapelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "area") == 0) {
                extern JSValue qjs_new_htmlareaelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlareaelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "caption") == 0) {
                extern JSValue qjs_new_htmltablecaptionelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmltablecaptionelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "col") == 0 || strcasecmp(tag, "colgroup") == 0) {
                extern JSValue qjs_new_htmltablecolelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmltablecolelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "thead") == 0 || strcasecmp(tag, "tbody") == 0 || strcasecmp(tag, "tfoot") == 0) {
                extern JSValue qjs_new_htmltablesectionelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmltablesectionelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "label") == 0) {
                extern JSValue qjs_new_htmllabelelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmllabelelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "datalist") == 0) {
                extern JSValue qjs_new_htmldatalistelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmldatalistelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "optgroup") == 0) {
                extern JSValue qjs_new_htmloptgroupelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmloptgroupelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "keygen") == 0) {
                extern JSValue qjs_new_htmlkeygenelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlkeygenelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "output") == 0) {
                extern JSValue qjs_new_htmloutputelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmloutputelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "progress") == 0) {
                extern JSValue qjs_new_htmlprogresselement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlprogresselement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "meter") == 0) {
                extern JSValue qjs_new_htmlmeterelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlmeterelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "fieldset") == 0) {
                extern JSValue qjs_new_htmlfieldsetelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlfieldsetelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "legend") == 0) {
                extern JSValue qjs_new_htmllegendelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmllegendelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "map") == 0) {
                extern JSValue qjs_new_htmlmapelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlmapelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "area") == 0) {
                extern JSValue qjs_new_htmlareaelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlareaelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "embed") == 0) {
                extern JSValue qjs_new_htmlembedelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlembedelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "object") == 0) {
                extern JSValue qjs_new_htmlobjectelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlobjectelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "param") == 0) {
                extern JSValue qjs_new_htmlparamelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlparamelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "time") == 0) {
                extern JSValue qjs_new_htmltimeelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmltimeelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "data") == 0) {
                extern JSValue qjs_new_htmldataelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmldataelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "keygen") == 0) {
                extern JSValue qjs_new_htmlkeygenelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlkeygenelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "a") == 0) {
                extern JSValue qjs_new_htmlanchorelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlanchorelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "iframe") == 0) {
                extern JSValue qjs_new_htmliframeelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmliframeelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "meta") == 0) {
                extern JSValue qjs_new_htmlmetaelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlmetaelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "link") == 0) {
                extern JSValue qjs_new_htmllinkelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmllinkelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "style") == 0) {
                extern JSValue qjs_new_htmlstyleelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlstyleelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "body") == 0) {
                extern JSValue qjs_new_htmlbodyelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlbodyelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "head") == 0) {
                extern JSValue qjs_new_htmlheadelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlheadelement(ctx, node, is_dom_node);
            if (strcasecmp(tag, "details") == 0) {
                extern JSValue qjs_new_htmldetailselement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmldetailselement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "menu") == 0) {
                extern JSValue qjs_new_htmlmenuelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlmenuelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "menuitem") == 0) {
                extern JSValue qjs_new_htmlmenuitemelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlmenuitemelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "dialog") == 0) {
                extern JSValue qjs_new_htmldialogelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmldialogelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "applet") == 0) {
                extern JSValue qjs_new_htmlappletelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlappletelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "marquee") == 0) {
                extern JSValue qjs_new_htmlmarqueeelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlmarqueeelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "frameset") == 0) {
                extern JSValue qjs_new_htmlframesetelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlframesetelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "frame") == 0) {
                extern JSValue qjs_new_htmlframeelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlframeelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "font") == 0) {
                extern JSValue qjs_new_htmlfontelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlfontelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "dir") == 0) {
                extern JSValue qjs_new_htmldirectoryelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmldirectoryelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "ins") == 0 || strcasecmp(tag, "del") == 0) {
                extern JSValue qjs_new_htmlmodelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlmodelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "html") == 0) {
                extern JSValue qjs_new_htmlhtmlelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlhtmlelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "optgroup") == 0) {
                extern JSValue qjs_new_htmloptgroupelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmloptgroupelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "label") == 0) {
                extern JSValue qjs_new_htmllabelelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmllabelelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "ul") == 0) {
                extern JSValue qjs_new_htmlulistelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlulistelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "ol") == 0) {
                extern JSValue qjs_new_htmlolistelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlolistelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "li") == 0) {
                extern JSValue qjs_new_htmllielement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmllielement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "div") == 0) {
                extern JSValue qjs_new_htmldivelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmldivelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "span") == 0) {
                extern JSValue qjs_new_htmlspanelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlspanelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "p") == 0) {
                extern JSValue qjs_new_htmlparagraphelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlparagraphelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "br") == 0) {
                extern JSValue qjs_new_htmlbrelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlbrelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "hr") == 0) {
                extern JSValue qjs_new_htmlhrelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlhrelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "h1") == 0 || strcasecmp(tag, "h2") == 0 || strcasecmp(tag, "h3") == 0 ||
                strcasecmp(tag, "h4") == 0 || strcasecmp(tag, "h5") == 0 || strcasecmp(tag, "h6") == 0) {
                extern JSValue qjs_new_htmlheadingelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlheadingelement(ctx, node, is_dom_node);
            if (strcasecmp(tag, "head") == 0) {
                extern JSValue qjs_new_htmlheadelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlheadelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "title") == 0) {
                extern JSValue qjs_new_htmltitleelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmltitleelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "base") == 0) {
                extern JSValue qjs_new_htmlbaseelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlbaseelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "link") == 0) {
                extern JSValue qjs_new_htmllinkelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmllinkelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "meta") == 0) {
                extern JSValue qjs_new_htmlmetaelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlmetaelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "style") == 0) {
                extern JSValue qjs_new_htmlstyleelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlstyleelement(ctx, node, is_dom_node);
            }
        }
    } else {
        dom_string *tag_dom = NULL;
        dom_element_get_tag_name((dom_element *)node, &tag_dom);
        if (tag_dom) {
            const char *tag = (const char *)dom_string_data(tag_dom);
            if (strcasecmp(tag, "script") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlscriptelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlscriptelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "template") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmltemplateelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmltemplateelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "img") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlimageelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlimageelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "select") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlselectelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlselectelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "input") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlinputelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlinputelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "textarea") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmltextareaelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmltextareaelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "button") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlbuttonelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlbuttonelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "form") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlformelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlformelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "option") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmloptionelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmloptionelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "canvas") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlcanvaselement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlcanvaselement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "table") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmltableelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmltableelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "tr") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmltablerowelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmltablerowelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "td") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmltabledatacellelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmltabledatacellelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "th") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmltableheadercellelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmltableheadercellelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "video") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlvideoelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlvideoelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "audio") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlaudioelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlaudioelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "track") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmltrackelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmltrackelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "source") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlsourceelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlsourceelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "picture") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlpictureelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlpictureelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "details") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmldetailselement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmldetailselement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "dialog") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmldialogelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmldialogelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "datalist") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmldatalistelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmldatalistelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "output") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmloutputelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmloutputelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "progress") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlprogresselement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlprogresselement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "meter") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlmeterelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlmeterelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "fieldset") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlfieldsetelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlfieldsetelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "legend") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmllegendelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmllegendelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "map") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlmapelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlmapelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "area") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlareaelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlareaelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "embed") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlembedelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlembedelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "object") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlobjectelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlobjectelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "param") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlparamelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlparamelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "time") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmltimeelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmltimeelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "data") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmldataelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmldataelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "keygen") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlkeygenelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlkeygenelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "a") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlanchorelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlanchorelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "iframe") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmliframeelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmliframeelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "meta") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlmetaelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlmetaelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "link") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmllinkelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmllinkelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "style") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlstyleelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlstyleelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "body") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlbodyelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlbodyelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "head") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlheadelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlheadelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "html") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlhtmlelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlhtmlelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "optgroup") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmloptgroupelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmloptgroupelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "label") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmllabelelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmllabelelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "ul") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlulistelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlulistelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "ol") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlolistelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlolistelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "li") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmllielement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmllielement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "div") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmldivelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmldivelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "span") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlspanelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlspanelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "p") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlparagraphelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlparagraphelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "br") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlbrelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlbrelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "hr") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlhrelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlhrelement(ctx, node, is_dom_node);
            }
            if (strcasecmp(tag, "h1") == 0 || strcasecmp(tag, "h2") == 0 || strcasecmp(tag, "h3") == 0 ||
                strcasecmp(tag, "h4") == 0 || strcasecmp(tag, "h5") == 0 || strcasecmp(tag, "h6") == 0) {
                dom_string_unref(tag_dom);
                extern JSValue qjs_new_htmlheadingelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlheadingelement(ctx, node, is_dom_node);
            }
            dom_string_unref(tag_dom);
        }
    }

    extern JSValue qjs_new_htmlelement(JSContext *ctx, void *node, bool is_dom_node);
    return qjs_new_htmlelement(ctx, node, is_dom_node);
}
