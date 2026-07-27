#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
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
            for (uint32_t i = 0; i < sns->attr_count; i++) {
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
            for (uint32_t i = 0; i < sns->attr_count; i++) {
                if (wisp_string_ref_caseeq(wisp_shm_dom, sns->attrs[i].name, qualifiedName)) {
                    sns->attrs[i].value = value_ref;
                    found = true;
                    break;
                }
            }
            if (!found && sns->attr_count < 16) {
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
            for (uint32_t i = 0; i < sns->attr_count; i++) {
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
            for (uint32_t i = 0; i < sns->attr_count; i++) {
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
        b->alloc = b->alloc ? b->alloc * 2 + len : len + 1024;
        b->buf = realloc(b->buf, b->alloc);
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

JSValue wisp_element_innerHTML_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    HTMLBuffer b = { NULL, 0, 0 };
    dom_node *child = NULL;
    dom_node_get_first_child((dom_node *)priv->node, &child);
    while (child) {
        serialize_node_to_html(child, &b);
        dom_node *next = NULL;
        dom_node_get_next_sibling(child, &next);
        dom_node_unref(child);
        child = next;
    }
    JSValue val = JS_NewStringLen(ctx, b.buf ? b.buf : "", b.len);
    free(b.buf);
    return val;
}
JSValue wisp_element_innerHTML_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    dom_node *element = (dom_node *)priv->node;
    dom_document *doc = NULL;
    dom_exception exc = dom_node_get_owner_document(element, &doc);
    if (exc != DOM_NO_ERR || !doc) return JS_ThrowInternalError(ctx, "Failed to get owner document");

    /* 1. Clear existing children */
    dom_node *child = NULL;
    while (dom_node_get_first_child(element, &child) == DOM_NO_ERR && child != NULL) {
        dom_node *removed = NULL;
        dom_node_remove_child(element, child, &removed);
        if (removed) dom_node_unref(removed);
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
        dom_node *f_child = NULL;
        while (dom_node_get_first_child((dom_node *)fragment, &f_child) == DOM_NO_ERR && f_child != NULL) {
            dom_node *result = NULL;
            /* dom_node_append_child on a fragment moves nodes from the fragment to the element */
            dom_node_append_child(element, f_child, &result);
            if (result) dom_node_unref(result);
            dom_node_unref(f_child);
            f_child = NULL;
        }
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
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (sn) {
            WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[(uint64_t)(uintptr_t)priv->node];
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
    if (wisp_is_js_process) return JS_NULL;
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
            JSValue initial_style = JS_NewObject(ctx);
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

static void request_synchronous_layout_from_main(void) {
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
    wisp_ipc_set_blocking(ipc_main, false);
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

static JSValue js_element_style_get_global(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc < 1) return JS_UNDEFINED;
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, argv[0]);
    if (!priv) return JS_UNDEFINED;
    return wisp_htmlelement_style_get_impl(ctx, priv);
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
        "            if (prop && val) {\n"
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
        "                return element.getAttribute('style') || '';\n"
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
        "                        return t[kebab] || '';\n"
        "                    }\n"
        "                    return '';\n"
        "                };\n"
        "            }\n"
        "            if (prop === 'getPropertyPriority') {\n"
        "                return function(p) {\n"
        "                    return '';\n"
        "                };\n"
        "            }\n"
        "            if (prop === 'setProperty') {\n"
        "                return function(p, v) {\n"
        "                    if (typeof p === 'string') {\n"
        "                        let kebab = p.replace(/([A-Z])/g, '-$1').toLowerCase();\n"
        "                        let camel = p.replace(/-([a-z])/g, (g) => g[1].toUpperCase());\n"
        "                        t[camel] = v;\n"
        "                        t[kebab] = v;\n"
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
        "                return t[prop] !== undefined ? t[prop] : '';\n"
        "            }\n"
        "            return t[prop];\n"
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
    serialize_node_to_html((dom_node *)priv->node, &b);
    JSValue val = JS_NewStringLen(ctx, b.buf ? b.buf : "", b.len);
    free(b.buf);
    return val;
}

JSValue wisp_element_matches_impl(JSContext *ctx, QJSNodePrivate *priv, const char * selectors)
{
    if (!priv || !priv->node || !selectors) return JS_FALSE;
    dom_node *element = (dom_node *)priv->node;

    dom_node *parent = NULL;
    dom_node_get_parent_node(element, &parent);
    dom_node *root = parent ? parent : element;

    JSValue list = qjs_dom_query_selector_internal(ctx, root, selectors, true);
    if (parent) dom_node_unref(parent);

    if (JS_IsArray(list)) {
        JSValue len_val = JS_GetPropertyStr(ctx, list, "length");
        uint32_t len = 0;
        JS_ToUint32(ctx, &len, len_val);
        JS_FreeValue(ctx, len_val);

        for (uint32_t i = 0; i < len; i++) {
            JSValue item = JS_GetPropertyUint32(ctx, list, i);
            QJSNodePrivate *ipriv = qjs_get_dom_priv(ctx, item);
            if (ipriv && ipriv->node == element) {
                JS_FreeValue(ctx, item);
                JS_FreeValue(ctx, list);
                return JS_TRUE;
            }
            JS_FreeValue(ctx, item);
        }
    }
    JS_FreeValue(ctx, list);
    return JS_FALSE;
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
            if (strcasecmp(tag, "img") == 0) {
                extern JSValue qjs_new_htmlimageelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlimageelement(ctx, node, is_dom_node);
            }
        }
    } else {
        dom_html_element_type tag_type;
        dom_exception exc = dom_html_element_get_tag_type((dom_html_element *)node, &tag_type);
        if (exc == DOM_NO_ERR) {
            if (tag_type == DOM_HTML_ELEMENT_TYPE_SCRIPT) {
                extern JSValue qjs_new_htmlscriptelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlscriptelement(ctx, node, is_dom_node);
            }
            if (tag_type == DOM_HTML_ELEMENT_TYPE_IMG) {
                extern JSValue qjs_new_htmlimageelement(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_htmlimageelement(ctx, node, is_dom_node);
            }
        }
    }

    JSValue obj = JS_NewObjectClass(ctx, qjs_element_class_id);
    if (JS_IsException(obj)) return obj;
    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));
    if (!priv) { JS_FreeValue(ctx, obj); return JS_ThrowOutOfMemory(ctx); }
    priv->magic = QJS_DOM_MAGIC; priv->node = node; priv->is_dom_node = is_dom_node; priv->ctx = ctx;
    if (!wisp_is_js_process && is_dom_node && node) dom_node_ref((dom_node *)node);
    JS_SetOpaque(obj, priv);
    return obj;
}
