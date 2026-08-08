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
#include <wisp/content/llcache.h>
#include <wisp/content/content_protected.h>

struct content;
extern struct nsurl *content_get_url(struct content *c);

#include "wisp/utils/shm_dom.h"
extern bool wisp_is_js_process;
extern shm_dom_t *wisp_shm_dom;

#include "JSEvent.gen.h"
#include "JSCustomEvent.gen.h"
#include "JSMessageEvent.gen.h"
#include "JSErrorEvent.gen.h"

uint64_t allocate_virtual_shm_node(uint16_t type, const char *name, const char *value) {
    if (!wisp_shm_dom) return 0;

    shm_dom_lock_write(wisp_shm_dom);

    uint32_t new_id = wisp_shm_dom->node_count++;
    extern uint32_t wisp_shm_capacity;
    if (new_id >= wisp_shm_capacity) {
        uint32_t new_cap = wisp_shm_dom->node_capacity * 2;
        shm_dom_t *new_shm = shm_dom_remap(wisp_shm_dom, wisp_shm_capacity, new_cap);
        if (new_shm) {
            new_shm->node_capacity = new_cap;
            wisp_shm_dom = new_shm;
            wisp_shm_capacity = new_cap;
        } else {
            shm_dom_unlock_write(wisp_shm_dom);
            return 0;
        }
    }

    WispCompactNode *nodes_array = shm_dom_get_nodes(wisp_shm_dom);
    WispCompactNode *sn = &nodes_array[new_id];
    memset(sn, 0, sizeof(*sn));
    sn->node_type = type;

    WispNodeStrings *node_strings_array = shm_dom_get_node_strings(wisp_shm_dom);
    WispNodeStrings *sns = &node_strings_array[new_id];
    memset(sns, 0, sizeof(*sns));

    if (type == 1) { // DOM_ELEMENT_NODE
        sns->tag_name = wisp_shm_alloc_string(wisp_shm_dom, name);
        if (strcasecmp(name, "html") == 0) sn->tag_atom = 1;
        else if (strcasecmp(name, "head") == 0) sn->tag_atom = 2;
        else if (strcasecmp(name, "body") == 0) sn->tag_atom = 3;
        else if (strcasecmp(name, "title") == 0) sn->tag_atom = 4;
        else if (strcasecmp(name, "div") == 0) sn->tag_atom = 5;
        else if (strcasecmp(name, "span") == 0) sn->tag_atom = 6;
        else if (strcasecmp(name, "p") == 0) sn->tag_atom = 7;
        else if (strcasecmp(name, "a") == 0) sn->tag_atom = 8;
        else if (strcasecmp(name, "script") == 0) sn->tag_atom = 9;
        else if (strcasecmp(name, "style") == 0) sn->tag_atom = 10;
        else if (strcasecmp(name, "link") == 0) sn->tag_atom = 11;
        else if (strcasecmp(name, "img") == 0) sn->tag_atom = 12;
        else if (strcasecmp(name, "iframe") == 0) sn->tag_atom = 13;
        else sn->tag_atom = 14;
    } else if (type == 3 || type == 8) { // DOM_TEXT_NODE, DOM_COMMENT_NODE
        sns->value = wisp_shm_alloc_string(wisp_shm_dom, value);
    }

    shm_dom_get_dom_ptrs(wisp_shm_dom)[new_id] = 0;

    shm_dom_unlock_write(wisp_shm_dom);
    return new_id;
}

static struct nsurl *get_doc_url_local(JSContext *ctx)
{
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (!t) return NULL;

    if (wisp_is_js_process) {
        if (!t->location_url && t->origin) {
            nsurl_create(t->origin, &t->location_url);
        }
        return t->location_url;
    }

    if (t->doc_priv) {
        return content_get_url((struct content *)t->doc_priv);
    }
    return NULL;
}

JSValue wisp_document_URL_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct nsurl *url = get_doc_url_local(ctx);
    if (url) {
        return JS_NewString(ctx, nsurl_access(url));
    }
    return JS_NewString(ctx, "about:blank");
}

JSValue wisp_document_origin_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct nsurl *url = get_doc_url_local(ctx);
    if (url) {
        lwc_string *scheme_lwc = nsurl_get_component(url, NSURL_SCHEME);
        lwc_string *host_lwc = nsurl_get_component(url, NSURL_HOST);
        lwc_string *port_lwc = nsurl_get_component(url, NSURL_PORT);
        if (scheme_lwc && host_lwc) {
            const char *s_data = lwc_string_data(scheme_lwc);
            size_t s_len = lwc_string_length(scheme_lwc);
            const char *h_data = lwc_string_data(host_lwc);
            size_t h_len = lwc_string_length(host_lwc);
            size_t p_len = port_lwc ? lwc_string_length(port_lwc) : 0;
            const char *p_data = port_lwc ? lwc_string_data(port_lwc) : "";

            size_t buf_len = s_len + 3 + h_len + (p_len > 0 ? 1 + p_len : 0);
            char *buf = malloc(buf_len + 1);
            if (buf) {
                char *ptr = buf;
                memcpy(ptr, s_data, s_len); ptr += s_len;
                memcpy(ptr, "://", 3); ptr += 3;
                memcpy(ptr, h_data, h_len); ptr += h_len;
                if (p_len > 0) {
                    *ptr = ':'; ptr++;
                    memcpy(ptr, p_data, p_len); ptr += p_len;
                }
                *ptr = '\0';
                JSValue res = JS_NewString(ctx, buf);
                free(buf);
                lwc_string_unref(scheme_lwc);
                lwc_string_unref(host_lwc);
                if (port_lwc) lwc_string_unref(port_lwc);
                return res;
            }
        }
        if (scheme_lwc) lwc_string_unref(scheme_lwc);
        if (host_lwc) lwc_string_unref(host_lwc);
        if (port_lwc) lwc_string_unref(port_lwc);
    }
    return JS_NewString(ctx, "null");
}

JSValue wisp_document_characterSet_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewString(ctx, "UTF-8");
}

JSValue wisp_document_inputEncoding_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewString(ctx, "UTF-8");
}

JSValue wisp_document_contentType_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewString(ctx, "text/html");
}

JSValue wisp_document_body_set_impl(JSContext *ctx, QJSNodePrivate *priv, void * value)
{
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    if (wisp_is_js_process) {
        shm_mutation_enqueue(wisp_shm_dom, SHM_MUTATION_APPEND_CHILD, (uint64_t)(uintptr_t)priv->node, (uint64_t)(uintptr_t)value, 0, NULL, NULL);
        return JS_UNDEFINED;
    }
    dom_node *removed = NULL;
    dom_node *old_body = NULL;
    dom_node_get_first_child((dom_node *)priv->node, &old_body);
    while (old_body) {
        dom_node_type type;
        dom_node_get_node_type(old_body, &type);
        if (type == DOM_ELEMENT_NODE) {
            dom_string *tag = NULL;
            dom_node_get_node_name(old_body, &tag);
            if (tag) {
                if (strcasecmp((const char *)dom_string_data(tag), "body") == 0) {
                    dom_string_unref(tag);
                    break;
                }
                dom_string_unref(tag);
            }
        }
        dom_node *next = NULL;
        dom_node_get_next_sibling(old_body, &next);
        dom_node_unref(old_body);
        old_body = next;
    }
    if (old_body) {
        dom_node_replace_child((dom_node *)priv->node, (dom_node *)value, old_body, &removed);
        dom_node_unref(old_body);
        if (removed) dom_node_unref(removed);
    } else {
        dom_node_append_child((dom_node *)priv->node, (dom_node *)value, &removed);
        if (removed) dom_node_unref(removed);
    }
    return JS_UNDEFINED;
}

extern JSValue qjs_new_htmlcollection_with_type(JSContext *ctx, void *node, bool is_dom_node, const char *type);

JSValue wisp_document_images_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    return qjs_new_htmlcollection_with_type(ctx, priv->node, priv->is_dom_node, "images");
}

JSValue wisp_document_forms_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    return qjs_new_htmlcollection_with_type(ctx, priv->node, priv->is_dom_node, "forms");
}

JSValue wisp_document_scripts_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    return qjs_new_htmlcollection_with_type(ctx, priv->node, priv->is_dom_node, "scripts");
}

JSValue wisp_document_links_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    return qjs_new_htmlcollection_with_type(ctx, priv->node, priv->is_dom_node, "links");
}

JSValue wisp_document_plugins_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    return qjs_new_htmlcollection_with_type(ctx, priv->node, priv->is_dom_node, "plugins");
}

JSValue wisp_document_embeds_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    return qjs_new_htmlcollection_with_type(ctx, priv->node, priv->is_dom_node, "embeds");
}

JSValue wisp_document_anchors_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    return qjs_new_htmlcollection_with_type(ctx, priv->node, priv->is_dom_node, "anchors");
}

JSValue wisp_document_applets_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    return qjs_new_htmlcollection_with_type(ctx, priv->node, priv->is_dom_node, "applets");
}

JSValue wisp_document_firstElementChild_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
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

JSValue wisp_document_lastElementChild_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
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

JSValue wisp_document_childElementCount_get_impl(JSContext *ctx, QJSNodePrivate *priv)
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

JSValue wisp_document_children_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    extern JSValue qjs_new_htmlcollection(JSContext *ctx, void *node, bool is_dom_node);
    return qjs_new_htmlcollection(ctx, priv->node, priv->is_dom_node);
}

JSValue wisp_document_createElement_impl(JSContext *ctx, QJSNodePrivate *priv, const char * localName)
{
    if (wisp_is_js_process) {
        uint64_t virtual_id = allocate_virtual_shm_node(1, localName, NULL);
        if (virtual_id == 0) return JS_NULL;
        return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)virtual_id);
    }
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
    if (wisp_is_js_process) {
        if (wisp_shm_dom) {
            WispCompactNode *nodes_arr = shm_dom_get_nodes(wisp_shm_dom);
            WispNodeStrings *strings_arr = shm_dom_get_node_strings(wisp_shm_dom);
            for (uint32_t i = 1; i < wisp_shm_dom->node_count; i++) {
                if (nodes_arr[i].node_type == 1 && // DOM_ELEMENT_NODE
                    (wisp_string_ref_caseeq(wisp_shm_dom, strings_arr[i].tag_name, "head"))) {
                    return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)i);
                }
            }
        }
        return JS_NULL;
    }
    return qjs_dom_query_selector_internal(ctx, (dom_node *)priv->node, "head", false);
}

JSValue wisp_document_createTextNode_impl(JSContext *ctx, QJSNodePrivate *priv, const char * data)
{
    if (wisp_is_js_process) {
        uint64_t virtual_id = allocate_virtual_shm_node(3, NULL, data);
        if (virtual_id == 0) return JS_NULL;
        return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)virtual_id);
    }
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
    if (wisp_is_js_process) {
        if (wisp_shm_dom && elementId) {
            WispCompactNode *nodes_arr = shm_dom_get_nodes(wisp_shm_dom);
            WispNodeStrings *strings_arr = shm_dom_get_node_strings(wisp_shm_dom);
            for (uint32_t i = 1; i < wisp_shm_dom->node_count; i++) {
                if (nodes_arr[i].node_type == 1) { // DOM_ELEMENT_NODE
                    uint32_t limit = strings_arr[i].attr_count < WISP_SHM_MAX_ATTRIBUTES ? strings_arr[i].attr_count : WISP_SHM_MAX_ATTRIBUTES;
                    for (uint32_t j = 0; j < limit; j++) {
                        if (wisp_string_ref_caseeq(wisp_shm_dom, strings_arr[i].attrs[j].name, "id") &&
                            wisp_string_ref_eq(wisp_shm_dom, strings_arr[i].attrs[j].value, elementId)) {
                            return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)i);
                        }
                    }
                }
            }
        }
        return JS_NULL;
    }
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
    if (wisp_is_js_process) {
        JSValue arr = JS_NewArray(ctx);
        uint32_t count = 0;
        if (wisp_shm_dom) {
            WispCompactNode *nodes_arr = shm_dom_get_nodes(wisp_shm_dom);
            WispNodeStrings *strings_arr = shm_dom_get_node_strings(wisp_shm_dom);
            for (uint32_t i = 1; i < wisp_shm_dom->node_count; i++) {
                if (nodes_arr[i].node_type == 1 && // DOM_ELEMENT_NODE
                    (strcmp(localName, "*") == 0 || wisp_string_ref_caseeq(wisp_shm_dom, strings_arr[i].tag_name, localName))) {
                    JS_SetPropertyUint32(ctx, arr, count++, qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)i));
                }
            }
        }
        return arr;
    }
    return qjs_dom_query_selector_internal(ctx, (dom_node *)priv->node, localName, true);
}

JSValue wisp_document_getElementsByClassName_impl(JSContext *ctx, QJSNodePrivate *priv, const char * classNames)
{
    if (!priv || !priv->node || !classNames) return JS_NewArray(ctx);
    if (wisp_is_js_process) {
        JSValue arr = JS_NewArray(ctx);
        uint32_t count = 0;
        if (wisp_shm_dom) {
            WispCompactNode *nodes_arr = shm_dom_get_nodes(wisp_shm_dom);
            WispNodeStrings *strings_arr = shm_dom_get_node_strings(wisp_shm_dom);
            for (uint32_t i = 1; i < wisp_shm_dom->node_count; i++) {
                if (nodes_arr[i].node_type == 1) { // DOM_ELEMENT_NODE
                    uint32_t limit = strings_arr[i].attr_count < WISP_SHM_MAX_ATTRIBUTES ? strings_arr[i].attr_count : WISP_SHM_MAX_ATTRIBUTES;
                    for (uint32_t j = 0; j < limit; j++) {
                        if (wisp_string_ref_caseeq(wisp_shm_dom, strings_arr[i].attrs[j].name, "class")) {
                            const char *cls = wisp_string_ref_data(wisp_shm_dom, strings_arr[i].attrs[j].value);
                            if (strstr(cls, classNames)) {
                                JS_SetPropertyUint32(ctx, arr, count++, qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)i));
                            }
                        }
                    }
                }
            }
        }
        return arr;
    }
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
        if (!wisp_is_js_process) {
            dom_event_unref(evt);
        }
        return obj;
    }
    return JS_NULL;
}

JSValue wisp_document_body_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) {
        if (wisp_shm_dom) {
            WispCompactNode *nodes_arr = shm_dom_get_nodes(wisp_shm_dom);
            WispNodeStrings *strings_arr = shm_dom_get_node_strings(wisp_shm_dom);
            for (uint32_t i = 1; i < wisp_shm_dom->node_count; i++) {
                if (nodes_arr[i].node_type == 1 && // DOM_ELEMENT_NODE
                    (wisp_string_ref_caseeq(wisp_shm_dom, strings_arr[i].tag_name, "body"))) {
                    return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)i);
                }
            }
        }
        return JS_NULL;
    }
    dom_string *body_name = NULL;
    dom_string_create((const uint8_t *)"body", 4, &body_name);
    if (!body_name) return JS_NULL;
    
    dom_nodelist *nodes = NULL;
    dom_document_get_elements_by_tag_name((dom_document *)priv->node, body_name, &nodes);
    dom_string_unref(body_name);
    
    if (nodes) {
        uint32_t len = 0;
        dom_nodelist_get_length(nodes, &len);
        if (len > 0) {
            dom_node *body = NULL;
            dom_nodelist_item(nodes, 0, &body);
            dom_nodelist_unref(nodes);
            if (body) {
                JSValue val = qjs_wrap_node(ctx, body);
                dom_node_unref(body);
                return val;
            }
        } else {
            dom_nodelist_unref(nodes);
        }
    }
    return JS_NULL;
}

JSValue wisp_document_documentElement_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) {
        if (wisp_shm_dom) {
            WispCompactNode *nodes_arr = shm_dom_get_nodes(wisp_shm_dom);
            WispNodeStrings *strings_arr = shm_dom_get_node_strings(wisp_shm_dom);
            for (uint32_t i = 1; i < wisp_shm_dom->node_count; i++) {
                if (nodes_arr[i].node_type == 1 && // DOM_ELEMENT_NODE
                    (wisp_string_ref_caseeq(wisp_shm_dom, strings_arr[i].tag_name, "html"))) {
                    return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)i);
                }
            }
        }
        return JS_NULL;
    }
    struct dom_element *root = NULL;
    dom_document_get_document_element((dom_document *)priv->node, &root);
    if (root) {
        JSValue val = qjs_wrap_node(ctx, (dom_node *)root);
        dom_node_unref((dom_node *)root);
        return val;
    }
    return JS_NULL;
}

#include "content/urldb.h"

JSValue wisp_document_write_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue text)
{
    if (wisp_is_js_process) return JS_UNDEFINED;
    if (!priv || !priv->node || JS_IsUndefined(text) || JS_IsNull(text)) return JS_UNDEFINED;
    
    // Convert text to C string
    const char *value = JS_ToCString(ctx, text);
    if (!value) return JS_UNDEFINED;
    
    dom_document *doc = (dom_document *)priv->node;
    
    // Get body element or document element to append to
    struct dom_html_document *html_doc = (struct dom_html_document *)doc;
    struct dom_html_element *body = NULL;
    dom_html_document_get_body(html_doc, &body);
    
    dom_node *target = body ? (dom_node *)body : (dom_node *)doc;
    
    // Parse new HTML string using Hubbub fragment parser
    dom_hubbub_parser_params params;
    memset(&params, 0, sizeof(params));
    params.enc = "UTF-8";
    params.idname = corestring_dom_id;

    dom_hubbub_parser *parser = NULL;
    dom_document_fragment *fragment = NULL;
    dom_hubbub_error err = dom_hubbub_fragment_parser_create(&params, doc, &parser, &fragment);
    if (err == DOM_HUBBUB_OK) {
        err = dom_hubbub_parser_parse_chunk(parser, (const uint8_t *)value, strlen(value));
        if (err == DOM_HUBBUB_OK) {
            err = dom_hubbub_parser_completed(parser);
        }
        
        if (err == DOM_HUBBUB_OK && fragment != NULL) {
            // Append children from fragment to target (body)
            dom_node *f_child = NULL;
            while (dom_node_get_first_child((dom_node *)fragment, &f_child) == DOM_NO_ERR && f_child != NULL) {
                dom_node *appended = NULL;
                dom_node_append_child(target, f_child, &appended);
                if (appended) dom_node_unref(appended);
                dom_node_unref(f_child);
                f_child = NULL;
            }
        }
    }
    
    if (parser) dom_hubbub_parser_destroy(parser);
    if (fragment) dom_node_unref((dom_node *)fragment);
    if (body) dom_node_unref((dom_node *)body);
    
    JS_FreeCString(ctx, value);
    return JS_UNDEFINED;
}

JSValue wisp_document_writeln_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue text)
{
    if (!priv || !priv->node || JS_IsUndefined(text) || JS_IsNull(text)) return JS_UNDEFINED;
    const char *value = JS_ToCString(ctx, text);
    if (!value) return JS_UNDEFINED;
    
    size_t len = strlen(value);
    char *new_val = malloc(len + 2);
    if (!new_val) {
        JS_FreeCString(ctx, value);
        return JS_ThrowOutOfMemory(ctx);
    }
    memcpy(new_val, value, len);
    new_val[len] = '\n';
    new_val[len + 1] = '\0';
    JS_FreeCString(ctx, value);
    
    JSValue text_with_nl = JS_NewString(ctx, new_val);
    free(new_val);
    
    JSValue res = wisp_document_write_impl(ctx, priv, text_with_nl);
    JS_FreeValue(ctx, text_with_nl);
    return res;
}

JSValue wisp_document_cookie_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (wisp_is_js_process) return JS_NewString(ctx, "");
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (t && t->doc_priv) {
        struct nsurl *url = content_get_url((struct content *)t->doc_priv);
        if (url) {
            char *cookie_str = urldb_get_cookie(url, false); // HTTP-only should be false for document.cookie in JS
            if (cookie_str) {
                JSValue res = JS_NewString(ctx, cookie_str);
                free(cookie_str);
                return res;
            }
        }
    }
    return JS_NewString(ctx, "");
}

JSValue wisp_document_cookie_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    if (wisp_is_js_process) return JS_UNDEFINED;
    if (!value) return JS_UNDEFINED;
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (t && t->doc_priv) {
        struct nsurl *url = content_get_url((struct content *)t->doc_priv);
        if (url) {
            urldb_set_cookie(value, url, NULL);
        }
    }
    return JS_UNDEFINED;
}

JSValue wisp_document_querySelector_impl(JSContext *ctx, QJSNodePrivate *priv, const char * selectors)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) {
        if (selectors && selectors[0] == '#') {
            return wisp_document_getElementById_impl(ctx, priv, selectors + 1);
        }
        if (selectors && selectors[0] == '.') {
            JSValue arr = wisp_document_getElementsByClassName_impl(ctx, priv, selectors + 1);
            JSValue first = JS_GetPropertyUint32(ctx, arr, 0);
            JS_FreeValue(ctx, arr);
            return first;
        }
        JSValue arr = wisp_document_getElementsByTagName_impl(ctx, priv, selectors);
        JSValue first = JS_GetPropertyUint32(ctx, arr, 0);
        JS_FreeValue(ctx, arr);
        return first;
    }
    return qjs_dom_query_selector_internal(ctx, (dom_node *)priv->node, selectors, false);
}

JSValue wisp_document_querySelectorAll_impl(JSContext *ctx, QJSNodePrivate *priv, const char * selectors)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) {
        if (selectors && selectors[0] == '#') {
            JSValue arr = JS_NewArray(ctx);
            JSValue item = wisp_document_getElementById_impl(ctx, priv, selectors + 1);
            if (!JS_IsNull(item)) {
                JS_SetPropertyUint32(ctx, arr, 0, item);
            } else {
                JS_FreeValue(ctx, item);
            }
            return arr;
        }
        if (selectors && selectors[0] == '.') {
            return wisp_document_getElementsByClassName_impl(ctx, priv, selectors + 1);
        }
        return wisp_document_getElementsByTagName_impl(ctx, priv, selectors);
    }
    return qjs_dom_query_selector_internal(ctx, (dom_node *)priv->node, selectors, true);
}

JSValue wisp_document_defaultView_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_GetGlobalObject(ctx);
}

JSValue wisp_document_createComment_impl(JSContext *ctx, QJSNodePrivate *priv, const char * data)
{
    if (wisp_is_js_process) {
        uint64_t virtual_id = allocate_virtual_shm_node(8, NULL, data);
        if (virtual_id == 0) return JS_NULL;
        return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)virtual_id);
    }
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
    if (wisp_is_js_process) {
        return wisp_document_querySelectorAll_impl(ctx, priv, name);
    }
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
    if (wisp_is_js_process) {
        uint64_t virtual_id = allocate_virtual_shm_node(11, NULL, NULL);
        if (virtual_id == 0) return JS_NULL;
        return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)virtual_id);
    }
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
    if (wisp_is_js_process) return JS_NewString(ctx, "complete");
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
    if (wisp_is_js_process) return JS_NewString(ctx, "");
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
    if (wisp_is_js_process) {
        if (wisp_shm_dom) {
            WispCompactNode *nodes_arr = shm_dom_get_nodes(wisp_shm_dom);
            WispNodeStrings *strings_arr = shm_dom_get_node_strings(wisp_shm_dom);
            for (uint32_t i = 1; i < wisp_shm_dom->node_count; i++) {
                if (nodes_arr[i].node_type == 1 && // DOM_ELEMENT_NODE
                    (wisp_string_ref_caseeq(wisp_shm_dom, strings_arr[i].tag_name, "title"))) {
                    uint64_t title_id = i;
                    for (uint32_t j = 1; j < wisp_shm_dom->node_count; j++) {
                        if (nodes_arr[j].parent_id == title_id &&
                            nodes_arr[j].node_type == 3) { // DOM_TEXT_NODE
                            return JS_NewString(ctx, wisp_string_ref_data(wisp_shm_dom, strings_arr[j].value));
                        }
                    }
                }
            }
        }
        return JS_NewString(ctx, "");
    }
    
    dom_string *title_name = NULL;
    dom_string_create((const uint8_t *)"title", 5, &title_name);
    if (!title_name) return JS_NewString(ctx, "");
    
    dom_nodelist *nodes = NULL;
    dom_document_get_elements_by_tag_name((dom_document *)priv->node, title_name, &nodes);
    dom_string_unref(title_name);
    
    if (nodes) {
        uint32_t len = 0;
        dom_nodelist_get_length(nodes, &len);
        if (len > 0) {
            dom_node *title_node = NULL;
            dom_nodelist_item(nodes, 0, &title_node);
            dom_nodelist_unref(nodes);
            if (title_node) {
                dom_string *text = NULL;
                dom_node_get_text_content(title_node, &text);
                dom_node_unref(title_node);
                if (text) {
                    JSValue val = JS_NewStringLen(ctx, (const char *)dom_string_data(text), dom_string_byte_length(text));
                    dom_string_unref(text);
                    return val;
                }
            }
        } else {
            dom_nodelist_unref(nodes);
        }
    }
    return JS_NewString(ctx, "");
}

JSValue wisp_document_title_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    if (wisp_is_js_process) return JS_UNDEFINED;
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    
    dom_string *title_name = NULL;
    dom_string_create((const uint8_t *)"title", 5, &title_name);
    if (!title_name) return JS_UNDEFINED;
    
    dom_nodelist *nodes = NULL;
    dom_document_get_elements_by_tag_name((dom_document *)priv->node, title_name, &nodes);
    
    dom_node *title_node = NULL;
    if (nodes) {
        uint32_t len = 0;
        dom_nodelist_get_length(nodes, &len);
        if (len > 0) {
            dom_nodelist_item(nodes, 0, &title_node);
        }
        dom_nodelist_unref(nodes);
    }
    
    if (!title_node) {
        dom_element *title_el = NULL;
        dom_document_create_element((dom_document *)priv->node, title_name, &title_el);
        if (title_el) {
            title_node = (dom_node *)title_el;
            dom_string *head_name = NULL;
            dom_string_create((const uint8_t *)"head", 4, &head_name);
            dom_node *head_node = NULL;
            if (head_name) {
                dom_nodelist *head_nodes = NULL;
                dom_document_get_elements_by_tag_name((dom_document *)priv->node, head_name, &head_nodes);
                dom_string_unref(head_name);
                if (head_nodes) {
                    uint32_t head_len = 0;
                    dom_nodelist_get_length(head_nodes, &head_len);
                    if (head_len > 0) {
                        dom_nodelist_item(head_nodes, 0, &head_node);
                    }
                    dom_nodelist_unref(head_nodes);
                }
            }
            
            dom_node *target = head_node;
            if (!target) {
                dom_element *doc_el = NULL;
                dom_document_get_document_element((dom_document *)priv->node, &doc_el);
                target = (dom_node *)doc_el;
            }
            
            if (target) {
                dom_node *appended = NULL;
                dom_node_append_child(target, title_node, &appended);
                if (appended) dom_node_unref(appended);
                dom_node_unref(target);
            }
        }
    }
    
    dom_string_unref(title_name);
    
    if (title_node) {
        dom_string *val_dom = NULL;
        dom_string_create((const uint8_t *)value, strlen(value), &val_dom);
        if (val_dom) {
            dom_node_set_text_content(title_node, val_dom);
            dom_string_unref(val_dom);
        }
        dom_node_unref(title_node);
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

JSValue wisp_document_compatMode_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewString(ctx, "CSS1Compat");
    if (wisp_is_js_process) {
        return JS_NewString(ctx, "CSS1Compat");
    }
    dom_document_quirks_mode quirks_mode = DOM_DOCUMENT_QUIRKS_MODE_NONE;
    dom_document_get_quirks_mode((dom_document *)priv->node, &quirks_mode);
    if (quirks_mode == DOM_DOCUMENT_QUIRKS_MODE_FULL) {
        return JS_NewString(ctx, "BackCompat");
    }
    return JS_NewString(ctx, "CSS1Compat");
}

JSValue wisp_document_currentScript_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (!t || !t->current_script_name) {
        return JS_NULL;
    }

    if (wisp_is_js_process) {
        if (wisp_shm_dom) {
            WispCompactNode *nodes_arr = shm_dom_get_nodes(wisp_shm_dom);
            WispNodeStrings *strings_arr = shm_dom_get_node_strings(wisp_shm_dom);

            uint32_t matched_id = 0;
            uint32_t last_inline_id = 0;

            for (uint32_t i = 1; i < wisp_shm_dom->node_count; i++) {
                if (nodes_arr[i].node_type == 1 &&
                    (nodes_arr[i].tag_atom == 9 || wisp_string_ref_caseeq(wisp_shm_dom, strings_arr[i].tag_name, "script"))) {
                    const char *src_val = NULL;
                    uint32_t limit = strings_arr[i].attr_count < WISP_SHM_MAX_ATTRIBUTES ? strings_arr[i].attr_count : WISP_SHM_MAX_ATTRIBUTES;
                    for (uint32_t j = 0; j < limit; j++) {
                        if (wisp_string_ref_caseeq(wisp_shm_dom, strings_arr[i].attrs[j].name, "src")) {
                            src_val = wisp_string_ref_data(wisp_shm_dom, strings_arr[i].attrs[j].value);
                            break;
                        }
                    }

                    if (src_val) {
                        if (strcmp(t->current_script_name, "?inline script?") != 0) {
                            if (strcmp(t->current_script_name, src_val) == 0 ||
                                (strncmp(src_val, "//", 2) == 0 && strstr(t->current_script_name, src_val + 2)) ||
                                strstr(t->current_script_name, src_val) ||
                                strstr(src_val, t->current_script_name)) {
                                matched_id = i;
                            }
                        }
                    } else {
                        last_inline_id = i;
                    }
                }
            }

            if (strcmp(t->current_script_name, "?inline script?") == 0) {
                matched_id = last_inline_id;
            }

            if (matched_id != 0) {
                return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)matched_id);
            }
        }
        return JS_NULL;
    } else {
        struct dom_document *doc = qjs_thread_get_document(t);
        if (doc) {
            dom_string *script_name_dom = NULL;
            dom_string_create((const uint8_t *)"script", 6, &script_name_dom);
            dom_nodelist *nodes = NULL;
            dom_document_get_elements_by_tag_name(doc, script_name_dom, &nodes);
            dom_string_unref(script_name_dom);

            if (nodes) {
                uint32_t len = 0;
                dom_nodelist_get_length(nodes, &len);
                dom_node *matched_node = NULL;
                dom_node *last_inline_node = NULL;

                dom_string *src_name = NULL;
                dom_string_create((const uint8_t *)"src", 3, &src_name);

                for (uint32_t i = 0; i < len; i++) {
                    dom_node *node = NULL;
                    dom_nodelist_item(nodes, i, &node);
                    if (node) {
                        dom_string *src_val = NULL;
                        dom_element_get_attribute((dom_element *)node, src_name, &src_val);

                        if (src_val) {
                            if (strcmp(t->current_script_name, "?inline script?") != 0) {
                                const char *src_cstr = (const char *)dom_string_data(src_val);
                                if (strcmp(t->current_script_name, src_cstr) == 0 ||
                                    (strncmp(src_cstr, "//", 2) == 0 && strstr(t->current_script_name, src_cstr + 2)) ||
                                    strstr(t->current_script_name, src_cstr) ||
                                    strstr(src_cstr, t->current_script_name)) {
                                    if (matched_node) dom_node_unref(matched_node);
                                    matched_node = dom_node_ref(node);
                                }
                            }
                            dom_string_unref(src_val);
                        } else {
                            if (last_inline_node) dom_node_unref(last_inline_node);
                            last_inline_node = dom_node_ref(node);
                        }
                        dom_node_unref(node);
                    }
                }

                if (src_name) dom_string_unref(src_name);
                dom_nodelist_unref(nodes);

                if (strcmp(t->current_script_name, "?inline script?") == 0) {
                    if (matched_node) dom_node_unref(matched_node);
                    matched_node = last_inline_node;
                    last_inline_node = NULL;
                } else {
                    if (last_inline_node) dom_node_unref(last_inline_node);
                }

                if (matched_node) {
                    JSValue val = qjs_wrap_node(ctx, matched_node);
                    dom_node_unref(matched_node);
                    return val;
                }
            }
        }
        return JS_NULL;
    }
}

JSValue wisp_document_referrer_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (wisp_is_js_process) return JS_NewString(ctx, "");
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    struct jsthread *t = (struct jsthread *)JS_GetContextOpaque(ctx);
    if (t && t->doc_priv) {
        struct content *c = (struct content *)t->doc_priv;
        if (c->llcache) {
            struct nsurl *ref = llcache_handle_get_referer(c->llcache);
            if (ref) {
                return JS_NewString(ctx, nsurl_access(ref));
            }
        }
    }
    return JS_NewString(ctx, "");
}
