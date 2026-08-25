#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "wisp/content/handlers/html/box.h"
#include "wisp/content/handlers/html/private.h"
#include "content/handlers/html/box_construct.h"
#include "content/handlers/html/box_manipulate.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include <wisp/utils/corestrings.h>
#include "utils/libdom.h"
#include "generated_bindings.h"
#include "wisp/utils/shm_dom.h"
#include "wisp/utils/nsurl.h"

struct content;
extern struct nsurl *content_get_url(struct content *c);

extern bool wisp_is_js_process;
extern shm_dom_t *wisp_shm_dom;

static uint32_t get_last_child_id(shm_dom_t *shm, uint32_t parent_id) {
    WispCompactNode *parent_shm = find_shm_node(shm, parent_id);
    if (!parent_shm || parent_shm->first_child_id == 0) return 0;
    uint32_t child_id = parent_shm->first_child_id;
    while (true) {
        WispCompactNode *child_sn = find_shm_node(shm, child_id);
        if (child_sn && child_sn->next_sibling_id != 0) {
            child_id = child_sn->next_sibling_id;
        } else {
            break;
        }
    }
    return child_id;
}

JSValue wisp_node_hasChildNodes_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_FALSE;
    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        return JS_NewBool(ctx, sn && sn->first_child_id != 0);
    }
    bool result = false;
    dom_node_has_child_nodes((dom_node *)priv->node, &result);
    return JS_NewBool(ctx, result);
}

JSValue wisp_node_normalize_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    if (wisp_is_js_process) return JS_UNDEFINED;
    dom_node_normalize((dom_node *)priv->node);
    return JS_UNDEFINED;
}

uint64_t allocate_virtual_shm_node(uint16_t type, const char *name, const char *value);

static uint32_t clone_virtual_node(uint32_t src_id, bool deep) {
    if (src_id == 0 || !wisp_shm_dom) return 0;

    WispCompactNode *src_nodes = shm_dom_get_nodes(wisp_shm_dom);
    WispNodeStrings *src_strings = shm_dom_get_node_strings(wisp_shm_dom);

    WispCompactNode *src_sn = &src_nodes[src_id];
    WispNodeStrings *src_sns = &src_strings[src_id];

    uint16_t type = src_sn->node_type;
    const char *name = NULL;
    const char *val_str = NULL;

    if (type == 1) { // ELEMENT_NODE
        name = wisp_string_ref_data(wisp_shm_dom, src_sns->tag_name);
    } else if (type == 3 || type == 8) { // TEXT_NODE, COMMENT_NODE
        val_str = wisp_string_ref_data(wisp_shm_dom, src_sns->value);
    }

    uint32_t new_id = (uint32_t)allocate_virtual_shm_node(type, name, val_str);
    if (new_id == 0) return 0;

    // Re-retrieve arrays since allocate_virtual_shm_node can remap capacity
    src_nodes = shm_dom_get_nodes(wisp_shm_dom);
    src_strings = shm_dom_get_node_strings(wisp_shm_dom);
    WispCompactNode *new_sn = &src_nodes[new_id];
    WispNodeStrings *new_sns = &src_strings[new_id];

    new_sn->class_hash = src_sn->class_hash;

    // Copy attributes (for elements)
    if (type == 1) {
        uint32_t attr_limit = src_sns->attr_count < WISP_SHM_MAX_ATTRIBUTES ? src_sns->attr_count : WISP_SHM_MAX_ATTRIBUTES;
        for (uint32_t i = 0; i < attr_limit; i++) {
            const char *attr_name = wisp_string_ref_data(wisp_shm_dom, src_sns->attrs[i].name);
            const char *attr_val = wisp_string_ref_data(wisp_shm_dom, src_sns->attrs[i].value);
            if (attr_name) {
                new_sns->attrs[new_sns->attr_count].name = wisp_shm_alloc_string(wisp_shm_dom, attr_name);
                new_sns->attrs[new_sns->attr_count].value = wisp_shm_alloc_string(wisp_shm_dom, attr_val);
                new_sns->attr_count++;
            }
        }
    }

    // If deep is true, recursively clone and append child nodes
    if (deep && src_sn->first_child_id != 0) {
        uint32_t curr_child_id = src_sn->first_child_id;
        uint32_t last_cloned_child_id = 0;
        while (curr_child_id != 0) {
            uint32_t cloned_child_id = clone_virtual_node(curr_child_id, true);
            if (cloned_child_id != 0) {
                // Re-retrieve arrays after recursive call!
                src_nodes = shm_dom_get_nodes(wisp_shm_dom);
                new_sn = &src_nodes[new_id];
                WispCompactNode *cloned_child_sn = &src_nodes[cloned_child_id];

                cloned_child_sn->parent_id = new_id;
                cloned_child_sn->next_sibling_id = 0;
                cloned_child_sn->prev_sibling_id = last_cloned_child_id;

                if (last_cloned_child_id != 0) {
                    WispCompactNode *last_cloned_child_sn = &src_nodes[last_cloned_child_id];
                    last_cloned_child_sn->next_sibling_id = cloned_child_id;
                } else {
                    new_sn->first_child_id = cloned_child_id;
                }
                last_cloned_child_id = cloned_child_id;
            }

            // Get next sibling
            src_nodes = shm_dom_get_nodes(wisp_shm_dom);
            WispCompactNode *curr_child_sn = &src_nodes[curr_child_id];
            curr_child_id = curr_child_sn->next_sibling_id;
        }
    }

    return new_id;
}

JSValue wisp_node_cloneNode_impl(JSContext *ctx, QJSNodePrivate *priv, bool deep)
{
    if (!priv || !priv->node) return JS_EXCEPTION;
    if (wisp_is_js_process) {
        uint32_t src_id = (uint32_t)(uintptr_t)priv->node;
        uint32_t cloned_id = clone_virtual_node(src_id, deep);
        if (cloned_id == 0) return JS_NULL;
        return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)cloned_id);
    }
    struct dom_node *result = NULL;
    dom_exception exc = dom_node_clone_node((dom_node *)priv->node, deep, &result);
    if (exc != DOM_NO_ERR || result == NULL) return JS_NULL;
    JSValue val = qjs_wrap_node(ctx, result);
    dom_node_unref(result);
    return val;
}

JSValue wisp_node_isEqualNode_impl(JSContext *ctx, QJSNodePrivate *priv, void * otherNode)
{
    if (!priv || !priv->node || !otherNode) return JS_FALSE;
    if (wisp_is_js_process) {
        return JS_NewBool(ctx, (uint64_t)(uintptr_t)priv->node == (uint64_t)(uintptr_t)otherNode);
    }
    bool result = false;
    dom_node_is_equal((dom_node *)priv->node, (dom_node *)otherNode, &result);
    return JS_NewBool(ctx, result);
}

JSValue wisp_node_compareDocumentPosition_impl(JSContext *ctx, QJSNodePrivate *priv, void * other)
{
    if (!priv || !priv->node || !other) return JS_NewInt32(ctx, 0);
    if (wisp_is_js_process) return JS_NewInt32(ctx, 0);
    uint16_t result = 0;
    dom_node_compare_document_position((dom_node *)priv->node, (dom_node *)other, &result);
    return JS_NewInt32(ctx, result);
}

JSValue wisp_node_contains_impl(JSContext *ctx, QJSNodePrivate *priv, void * other)
{
    if (!priv || !priv->node || !other) return JS_FALSE;
    if (wisp_is_js_process) {
        uint32_t curr_id = (uint32_t)(uintptr_t)other;
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, curr_id);
        while (sn) {
            if (curr_id == (uint32_t)(uintptr_t)priv->node) return JS_TRUE;
            if (sn->parent_id == 0) break;
            curr_id = sn->parent_id;
            sn = find_shm_node(wisp_shm_dom, curr_id);
        }
        return JS_FALSE;
    }
    bool result = false;
    dom_node_contains((dom_node *)priv->node, (dom_node *)other, &result);
    return JS_NewBool(ctx, result);
}

JSValue wisp_node_lookupPrefix_impl(JSContext *ctx, QJSNodePrivate *priv, const char * namespace)
{
    if (!priv || !priv->node || !namespace) return JS_NULL;
    if (wisp_is_js_process) return JS_NULL;
    dom_string *ns_dom = NULL;
    dom_string_create((const uint8_t *)namespace, strlen(namespace), &ns_dom);
    if (!ns_dom) return JS_NULL;

    dom_string *prefix_dom = NULL;
    dom_exception exc = dom_node_lookup_prefix((dom_node *)priv->node, ns_dom, &prefix_dom);
    dom_string_unref(ns_dom);

    if (exc == DOM_NO_ERR && prefix_dom) {
        JSValue val = JS_NewStringLen(ctx, (const char *)dom_string_data(prefix_dom), dom_string_byte_length(prefix_dom));
        dom_string_unref(prefix_dom);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_node_lookupNamespaceURI_impl(JSContext *ctx, QJSNodePrivate *priv, const char * namespaceURI)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) return JS_NULL;
    dom_string *prefix_dom = NULL;
    if (namespaceURI) {
        dom_string_create((const uint8_t *)namespaceURI, strlen(namespaceURI), &prefix_dom);
    }

    dom_string *ns_dom = NULL;
    dom_exception exc = dom_node_lookup_namespace((dom_node *)priv->node, prefix_dom, &ns_dom);
    if (prefix_dom) dom_string_unref(prefix_dom);

    if (exc == DOM_NO_ERR && ns_dom) {
        JSValue val = JS_NewStringLen(ctx, (const char *)dom_string_data(ns_dom), dom_string_byte_length(ns_dom));
        dom_string_unref(ns_dom);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_node_isDefaultNamespace_impl(JSContext *ctx, QJSNodePrivate *priv, const char * namespace)
{
    if (!priv || !priv->node || !namespace) return JS_FALSE;
    if (wisp_is_js_process) return JS_FALSE;
    dom_string *ns_dom = NULL;
    dom_string_create((const uint8_t *)namespace, strlen(namespace), &ns_dom);
    if (!ns_dom) return JS_FALSE;

    bool result = false;
    dom_exception exc = dom_node_is_default_namespace((dom_node *)priv->node, ns_dom, &result);
    dom_string_unref(ns_dom);

    if (exc == DOM_NO_ERR) {
        return JS_NewBool(ctx, result);
    }
    return JS_FALSE;
}

void check_script_element_execution(JSContext *ctx, void *node)
{
    if (!ctx || !node) return;

    bool is_module_script = false;

    if (wisp_is_js_process) {
        uint64_t child_id = (uint64_t)(uintptr_t)node;
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, child_id);
        if (sn) {
            if (sn->node_type == 1) { // DOM_ELEMENT_NODE
                WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[child_id];
                const char *tag = wisp_string_ref_data(wisp_shm_dom, sns->tag_name);
                if (tag && strcasecmp(tag, "script") == 0) {
                    for (uint32_t i = 0; i < sns->attr_count; i++) {
                        const char *aname = wisp_string_ref_data(wisp_shm_dom, sns->attrs[i].name);
                        const char *aval = wisp_string_ref_data(wisp_shm_dom, sns->attrs[i].value);
                        if (aname && aval) {
                            if (strcasecmp(aname, "type") == 0 && strcasecmp(aval, "module") == 0) {
                                is_module_script = true;
                            } else if (strcasecmp(aname, "src") == 0 && strstr(aval, "modules.js") != NULL) {
                                is_module_script = true;
                            }
                        }
                    }
                }
            }
            if (sn->first_child_id != 0) {
                uint32_t cid = sn->first_child_id;
                while (cid != 0) {
                    WispCompactNode *csn = find_shm_node(wisp_shm_dom, cid);
                    uint32_t next_cid = csn ? csn->next_sibling_id : 0;
                    check_script_element_execution(ctx, (void *)(uintptr_t)cid);
                    cid = next_cid;
                }
            }
        }
    } else {
        dom_node *dnode = (dom_node *)node;
        dom_node_type type;
        if (dom_node_get_node_type(dnode, &type) == DOM_NO_ERR) {
            if (type == DOM_ELEMENT_NODE) {
                dom_string *tag_dom = NULL;
                dom_node_get_node_name(dnode, &tag_dom);
                if (tag_dom) {
                    const char *tag = (const char *)dom_string_data(tag_dom);
                    if (tag && strcasecmp(tag, "script") == 0) {
                        dom_string *attr_type_name = NULL;
                        dom_string_create((const uint8_t *)"type", 4, &attr_type_name);
                        dom_string *type_val_dom = NULL;
                        dom_element_get_attribute((dom_element *)dnode, attr_type_name, &type_val_dom);
                        dom_string_unref(attr_type_name);

                        if (type_val_dom) {
                            const char *tval = (const char *)dom_string_data(type_val_dom);
                            if (tval && strcasecmp(tval, "module") == 0) {
                                is_module_script = true;
                            }
                            dom_string_unref(type_val_dom);
                        }

                        dom_string *attr_src_name = NULL;
                        dom_string_create((const uint8_t *)"src", 3, &attr_src_name);
                        dom_string *src_val_dom = NULL;
                        dom_element_get_attribute((dom_element *)dnode, attr_src_name, &src_val_dom);
                        dom_string_unref(attr_src_name);

                        if (src_val_dom) {
                            const char *sval = (const char *)dom_string_data(src_val_dom);
                            if (sval && strstr(sval, "modules.js") != NULL) {
                                is_module_script = true;
                            }
                            dom_string_unref(src_val_dom);
                        }
                    }
                    dom_string_unref(tag_dom);
                }
            }

            dom_node *child = NULL;
            if (dom_node_get_first_child(dnode, &child) == DOM_NO_ERR && child != NULL) {
                while (child) {
                    dom_node *next = NULL;
                    dom_exception next_exc = dom_node_get_next_sibling(child, &next);
                    check_script_element_execution(ctx, child);
                    dom_node_unref(child);
                    if (next_exc != DOM_NO_ERR) break;
                    child = next;
                }
            }
        }
    }

    if (is_module_script) {
        JSValue global = JS_GetGlobalObject(ctx);
        JSValue cb = JS_GetPropertyStr(ctx, global, "callback_es6_modules");
        if (JS_IsFunction(ctx, cb)) {
            JSValue args_arr = JS_NewArray(ctx);
            JS_SetPropertyUint32(ctx, args_arr, 0, JS_TRUE);
            JSValue timer_id = wisp_timer_create(ctx, cb, 0, args_arr, false);
            JS_FreeValue(ctx, timer_id);
            JS_FreeValue(ctx, args_arr);
        }
        JS_FreeValue(ctx, cb);
        JS_FreeValue(ctx, global);
    }
}

static void shm_node_insert_before_single(uint64_t parent_id, uint64_t child_id, uint64_t ref_id) {
    shm_mutation_enqueue(wisp_shm_dom, SHM_MUTATION_INSERT_BEFORE, parent_id, child_id, ref_id, NULL, NULL);

    WispCompactNode *parent_shm = find_shm_node(wisp_shm_dom, parent_id);
    WispCompactNode *child_shm = find_shm_node(wisp_shm_dom, child_id);
    if (parent_shm && child_shm) {
        if (child_shm->parent_id != 0) {
            WispCompactNode *old_parent = find_shm_node(wisp_shm_dom, child_shm->parent_id);
            if (old_parent) {
                if (old_parent->first_child_id == child_id) {
                    old_parent->first_child_id = child_shm->next_sibling_id;
                }
            }
            if (child_shm->prev_sibling_id != 0) {
                WispCompactNode *prev = find_shm_node(wisp_shm_dom, child_shm->prev_sibling_id);
                if (prev) prev->next_sibling_id = child_shm->next_sibling_id;
            }
            if (child_shm->next_sibling_id != 0) {
                WispCompactNode *next = find_shm_node(wisp_shm_dom, child_shm->next_sibling_id);
                if (next) next->prev_sibling_id = child_shm->prev_sibling_id;
            }
        }

        if (ref_id != 0) {
            WispCompactNode *ref_shm = find_shm_node(wisp_shm_dom, ref_id);
            if (ref_shm) {
                child_shm->parent_id = parent_id;
                child_shm->next_sibling_id = ref_id;
                child_shm->prev_sibling_id = ref_shm->prev_sibling_id;

                if (ref_shm->prev_sibling_id != 0) {
                    WispCompactNode *prev = find_shm_node(wisp_shm_dom, ref_shm->prev_sibling_id);
                    if (prev) prev->next_sibling_id = child_id;
                } else {
                    parent_shm->first_child_id = child_id;
                }
                ref_shm->prev_sibling_id = child_id;
            }
        } else {
            uint32_t last_child_id = get_last_child_id(wisp_shm_dom, parent_id);
            child_shm->parent_id = parent_id;
            child_shm->next_sibling_id = 0;
            child_shm->prev_sibling_id = last_child_id;

            if (last_child_id != 0) {
                WispCompactNode *last_child = find_shm_node(wisp_shm_dom, last_child_id);
                if (last_child) last_child->next_sibling_id = child_id;
            } else {
                parent_shm->first_child_id = child_id;
            }
        }
    }
}

JSValue wisp_node_insertBefore_impl(JSContext *ctx, QJSNodePrivate *priv, void * node, void * child)
{
    if (!priv || !priv->node || !node) return JS_EXCEPTION;
    check_script_element_execution(ctx, node);
    if (wisp_is_js_process) {
        uint64_t parent_id = (uint64_t)(uintptr_t)priv->node;
        uint64_t child_id = (uint64_t)(uintptr_t)node;
        uint64_t ref_id = child ? (uint64_t)(uintptr_t)child : 0;

        WispCompactNode *child_shm = find_shm_node(wisp_shm_dom, child_id);
        if (child_shm && child_shm->node_type == 11) { // DOM_DOCUMENT_FRAGMENT_NODE
            while (child_shm->first_child_id != 0) {
                uint64_t cid = child_shm->first_child_id;
                shm_node_insert_before_single(parent_id, cid, ref_id);
            }
        } else {
            shm_node_insert_before_single(parent_id, child_id, ref_id);
        }
        return qjs_wrap_node(ctx, (dom_node *)node);
    }
    struct dom_node *result = NULL;
    dom_exception exc = dom_node_insert_before((dom_node *)priv->node, (dom_node *)node, (dom_node *)child, &result);
    if (exc != DOM_NO_ERR) return JS_ThrowInternalError(ctx, "dom_node_insert_before failed");

    if (result) dom_node_unref(result);
    return qjs_wrap_node(ctx, (dom_node *)node);
}

JSValue wisp_node_appendChild_impl(JSContext *ctx, QJSNodePrivate *priv, void * node)
{
    if (!priv || !priv->node || !node) return JS_EXCEPTION;
    check_script_element_execution(ctx, node);
    if (wisp_is_js_process) {
        uint64_t parent_id = (uint64_t)(uintptr_t)priv->node;
        uint64_t child_id = (uint64_t)(uintptr_t)node;

        WispCompactNode *child_shm = find_shm_node(wisp_shm_dom, child_id);
        if (child_shm && child_shm->node_type == 11) { // DOM_DOCUMENT_FRAGMENT_NODE
            while (child_shm->first_child_id != 0) {
                uint64_t cid = child_shm->first_child_id;
                shm_node_insert_before_single(parent_id, cid, 0);
            }
        } else {
            shm_node_insert_before_single(parent_id, child_id, 0);
        }
        return qjs_wrap_node(ctx, (dom_node *)node);
    }
    struct dom_node *result = NULL;
    dom_exception exc = dom_node_append_child((dom_node *)priv->node, (dom_node *)node, &result);
    if (exc != DOM_NO_ERR) return JS_ThrowInternalError(ctx, "dom_node_append_child failed");

    if (result) dom_node_unref(result);
    return qjs_wrap_node(ctx, (dom_node *)node);
}

JSValue wisp_node_replaceChild_impl(JSContext *ctx, QJSNodePrivate *priv, void * node, void * child)
{
    if (!priv || !priv->node || !node || !child) return JS_EXCEPTION;
    check_script_element_execution(ctx, node);
    if (wisp_is_js_process) {
        uint64_t parent_id = (uint64_t)(uintptr_t)priv->node;
        uint64_t new_child_id = (uint64_t)(uintptr_t)node;
        uint64_t old_child_id = (uint64_t)(uintptr_t)child;

        WispCompactNode *new_child_shm = find_shm_node(wisp_shm_dom, new_child_id);
        if (new_child_shm && new_child_shm->node_type == 11) { // DOM_DOCUMENT_FRAGMENT_NODE
            while (new_child_shm->first_child_id != 0) {
                uint64_t cid = new_child_shm->first_child_id;
                shm_node_insert_before_single(parent_id, cid, old_child_id);
            }
            wisp_node_removeChild_impl(ctx, priv, child);
        } else {
            shm_mutation_enqueue(wisp_shm_dom, SHM_MUTATION_REPLACE_CHILD, parent_id, new_child_id, old_child_id, NULL, NULL);

            WispCompactNode *parent_shm = find_shm_node(wisp_shm_dom, parent_id);
            WispCompactNode *old_child_shm = find_shm_node(wisp_shm_dom, old_child_id);
            if (parent_shm && new_child_shm && old_child_shm) {
                new_child_shm->parent_id = parent_id;
                new_child_shm->next_sibling_id = old_child_shm->next_sibling_id;
                new_child_shm->prev_sibling_id = old_child_shm->prev_sibling_id;

                if (parent_shm->first_child_id == old_child_id) parent_shm->first_child_id = new_child_id;

                if (old_child_shm->prev_sibling_id != 0) {
                    WispCompactNode *prev = find_shm_node(wisp_shm_dom, old_child_shm->prev_sibling_id);
                    if (prev) prev->next_sibling_id = new_child_id;
                }
                if (old_child_shm->next_sibling_id != 0) {
                    WispCompactNode *next = find_shm_node(wisp_shm_dom, old_child_shm->next_sibling_id);
                    if (next) next->prev_sibling_id = new_child_id;
                }

                old_child_shm->parent_id = 0;
                old_child_shm->next_sibling_id = 0;
                old_child_shm->prev_sibling_id = 0;
            }
        }
        return qjs_wrap_node(ctx, (dom_node *)child);
    }
    struct dom_node *result = NULL;
    dom_exception exc = dom_node_replace_child((dom_node *)priv->node, (dom_node *)node, (dom_node *)child, &result);
    if (exc != DOM_NO_ERR) return JS_ThrowInternalError(ctx, "dom_node_replace_child failed");

    if (result) dom_node_unref(result);
    return qjs_wrap_node(ctx, (dom_node *)child);
}

JSValue wisp_node_removeChild_impl(JSContext *ctx, QJSNodePrivate *priv, void * child)
{
    if (!priv || !priv->node || !child) return JS_EXCEPTION;
    if (wisp_is_js_process) {
        uint64_t parent_id = (uint64_t)(uintptr_t)priv->node;
        uint64_t child_id = (uint64_t)(uintptr_t)child;
        shm_mutation_enqueue(wisp_shm_dom, SHM_MUTATION_REMOVE_CHILD, parent_id, child_id, 0, NULL, NULL);

        WispCompactNode *parent_shm = find_shm_node(wisp_shm_dom, parent_id);
        WispCompactNode *child_shm = find_shm_node(wisp_shm_dom, child_id);
        if (parent_shm && child_shm) {
            if (parent_shm->first_child_id == child_id) {
                parent_shm->first_child_id = child_shm->next_sibling_id;
            }
            if (child_shm->prev_sibling_id != 0) {
                WispCompactNode *prev = find_shm_node(wisp_shm_dom, child_shm->prev_sibling_id);
                if (prev) prev->next_sibling_id = child_shm->next_sibling_id;
            }
            if (child_shm->next_sibling_id != 0) {
                WispCompactNode *next = find_shm_node(wisp_shm_dom, child_shm->next_sibling_id);
                if (next) next->prev_sibling_id = child_shm->prev_sibling_id;
            }
            child_shm->parent_id = 0;
            child_shm->next_sibling_id = 0;
            child_shm->prev_sibling_id = 0;
        }
        return qjs_wrap_node(ctx, (dom_node *)child);
    }
    struct dom_node *result = NULL;
    dom_exception exc = dom_node_remove_child((dom_node *)priv->node, (dom_node *)child, &result);
    if (exc != DOM_NO_ERR) return JS_ThrowInternalError(ctx, "dom_node_remove_child failed");

    if (result) dom_node_unref(result);
    return qjs_wrap_node(ctx, (dom_node *)child);
}

JSValue wisp_node_nodeType_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    if (wisp_is_js_process) {
        if ((uint64_t)(uintptr_t)priv->node == 0) return JS_NewInt32(ctx, 9); /* DOM_DOCUMENT_NODE */
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (sn) return JS_NewInt32(ctx, sn->node_type);
        return JS_NULL;
    }
    dom_node_type type;
    dom_node_get_node_type((dom_node *)priv->node, &type);
    return JS_NewInt32(ctx, type);
}

JSValue wisp_node_nodeName_get_impl(JSContext *ctx, QJSNodePrivate *priv)
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
            return JS_NewString(ctx, wisp_string_ref_data(wisp_shm_dom, sns->name));
        }
        return JS_NULL;
    }
    dom_string *name = NULL;
    dom_node_get_node_name((dom_node *)priv->node, &name);
    if (name) {
        JSValue val = JS_NewStringLen(ctx, (const char *)dom_string_data(name), dom_string_byte_length(name));
        dom_string_unref(name);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_node_baseURI_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) {
        // Fallback for JS Process
        return JS_NewString(ctx, "about:blank");
    }
    struct dom_document *doc = NULL;
    dom_node_get_owner_document((dom_node *)priv->node, &doc);
    if (!doc) {
        dom_node_type type;
        dom_node_get_node_type((dom_node *)priv->node, &type);
        if (type == DOM_DOCUMENT_NODE) {
            doc = (struct dom_document *)priv->node;
            dom_node_ref((dom_node *)doc);
        }
    }
    if (doc) {
        html_content *htmlc = NULL;
        dom_node_get_user_data((dom_node *)doc, corestring_dom___ns_key_html_content_data, (void **)&htmlc);
        dom_node_unref((dom_node *)doc);
        if (htmlc) {
            struct nsurl *url = content_get_url((struct content *)htmlc);
            if (url) {
                const char *url_str = nsurl_access(url);
                if (url_str) {
                    return JS_NewString(ctx, url_str);
                }
            }
        }
    }
    return JS_NULL;
}

JSValue wisp_node_ownerDocument_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) {
        if ((uint64_t)(uintptr_t)priv->node == 0) return JS_NULL;
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (sn && sn->node_type == DOM_DOCUMENT_NODE) {
            return JS_NULL;
        }
        // Return global document object to avoid wrapper duplication
        JSValue global = JS_GetGlobalObject(ctx);
        JSValue doc = JS_GetPropertyStr(ctx, global, "document");
        JS_FreeValue(ctx, global);
        return doc;
    }
    struct dom_document *doc = NULL;
    dom_node_get_owner_document((dom_node *)priv->node, &doc);
    if (doc) {
        JSValue val = qjs_wrap_node(ctx, (dom_node *)doc);
        dom_node_unref((dom_node *)doc);
        return val;
    }
    dom_node_type type = DOM_NODE_TYPE_COUNT;
    dom_node_get_node_type((dom_node *)priv->node, &type);
    if (type != DOM_DOCUMENT_NODE) {
        JSValue global = JS_GetGlobalObject(ctx);
        JSValue doc_val = JS_GetPropertyStr(ctx, global, "document");
        JS_FreeValue(ctx, global);
        return doc_val;
    }
    return JS_NULL;
}

JSValue wisp_node_parentNode_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (sn && sn->parent_id != 0) {
            return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)sn->parent_id);
        }
        return JS_NULL;
    }
    struct dom_node *parent = NULL;
    dom_node_get_parent_node((dom_node *)priv->node, &parent);
    if (parent) {
        JSValue val = qjs_wrap_node(ctx, parent);
        dom_node_unref(parent);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_node_parentElement_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        while (sn && sn->parent_id != 0) {
            WispCompactNode *parent_sn = find_shm_node(wisp_shm_dom, sn->parent_id);
            if (parent_sn && parent_sn->node_type == DOM_ELEMENT_NODE) {
                return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)sn->parent_id);
            }
            sn = parent_sn;
        }
        return JS_NULL;
    }
    struct dom_node *parent = NULL;
    dom_node_get_parent_node((dom_node *)priv->node, &parent);
    while (parent) {
        dom_node_type type;
        dom_node_get_node_type(parent, &type);
        if (type == DOM_ELEMENT_NODE) {
            JSValue val = qjs_wrap_node(ctx, parent);
            dom_node_unref(parent);
            return val;
        }
        struct dom_node *next_parent = NULL;
        dom_node_get_parent_node(parent, &next_parent);
        dom_node_unref(parent);
        parent = next_parent;
    }
    return JS_NULL;
}

JSValue wisp_node_childNodes_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewArray(ctx);
    JSValue arr = JS_NewArray(ctx);
    if (JS_IsException(arr)) return arr;

    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        uint32_t index = 0;
        if (sn && sn->first_child_id != 0) {
            uint64_t child_id = sn->first_child_id;
            while (child_id != 0) {
                JSValue child_val = qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)child_id);
                JS_SetPropertyUint32(ctx, arr, index++, child_val);
                WispCompactNode *child_sn = find_shm_node(wisp_shm_dom, child_id);
                child_id = child_sn ? child_sn->next_sibling_id : 0;
            }
        }
        return arr;
    }

    struct dom_node *curr = NULL;
    dom_exception exc = dom_node_get_first_child((dom_node *)priv->node, &curr);
    uint32_t index = 0;
    if (exc == DOM_NO_ERR && curr) {
        while (curr) {
            JSValue child_val = qjs_wrap_node(ctx, curr);
            JS_SetPropertyUint32(ctx, arr, index++, child_val);
            struct dom_node *next = NULL;
            dom_exception next_exc = dom_node_get_next_sibling(curr, &next);
            dom_node_unref(curr);
            if (next_exc != DOM_NO_ERR) {
                break;
            }
            curr = next;
        }
    }
    return arr;
}

JSValue wisp_node_firstChild_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) {
        uint64_t nid = (uint64_t)(uintptr_t)priv->node;
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, nid);
        if (sn && sn->first_child_id != 0) {
            return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)sn->first_child_id);
        }
        /* If not found in SHM cache yet (e.g. innerHTML mutation queued but not flushed/re-serialized),
         * request synchronous DOM layout / flush from main process. */
        extern void request_synchronous_layout_from_main(void);
        request_synchronous_layout_from_main();
        sn = find_shm_node(wisp_shm_dom, nid);
        if (sn && sn->first_child_id != 0) {
            return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)sn->first_child_id);
        }
        return JS_NULL;
    }
    struct dom_node *child = NULL;
    dom_node_get_first_child((dom_node *)priv->node, &child);
    if (child) {
        JSValue val = qjs_wrap_node(ctx, child);
        dom_node_unref(child);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_node_lastChild_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (sn && sn->first_child_id != 0) {
            uint32_t child_id = sn->first_child_id;
            while (true) {
                WispCompactNode *child_sn = find_shm_node(wisp_shm_dom, child_id);
                if (child_sn && child_sn->next_sibling_id != 0) {
                    child_id = child_sn->next_sibling_id;
                } else {
                    break;
                }
            }
            return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)child_id);
        }
        return JS_NULL;
    }
    struct dom_node *child = NULL;
    dom_node_get_last_child((dom_node *)priv->node, &child);
    if (child) {
        JSValue val = qjs_wrap_node(ctx, child);
        dom_node_unref(child);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_node_previousSibling_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (sn && sn->prev_sibling_id != 0) {
            return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)sn->prev_sibling_id);
        }
        return JS_NULL;
    }
    struct dom_node *sibling = NULL;
    dom_node_get_previous_sibling((dom_node *)priv->node, &sibling);
    if (sibling) {
        JSValue val = qjs_wrap_node(ctx, sibling);
        dom_node_unref(sibling);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_node_nextSibling_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (sn && sn->next_sibling_id != 0) {
            return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)sn->next_sibling_id);
        }
        return JS_NULL;
    }
    struct dom_node *sibling = NULL;
    dom_node_get_next_sibling((dom_node *)priv->node, &sibling);
    if (sibling) {
        JSValue val = qjs_wrap_node(ctx, sibling);
        dom_node_unref(sibling);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_node_nodeValue_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (sn) {
            WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[(uint64_t)(uintptr_t)priv->node];
            return JS_NewString(ctx, wisp_string_ref_data(wisp_shm_dom, sns->value));
        }
        return JS_NULL;
    }
    dom_string *val = NULL;
    dom_node_get_node_value((dom_node *)priv->node, &val);
    if (val) {
        JSValue res = JS_NewStringLen(ctx, (const char *)dom_string_data(val), dom_string_byte_length(val));
        dom_string_unref(val);
        return res;
    }
    return JS_NULL;
}

JSValue wisp_node_nodeValue_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (sn) {
            WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[(uint64_t)(uintptr_t)priv->node];
            sns->value = wisp_shm_alloc_string(wisp_shm_dom, value);
        }
        shm_mutation_enqueue(wisp_shm_dom, SHM_MUTATION_SET_NODE_VALUE, (uint64_t)(uintptr_t)priv->node, 0, 0, NULL, value);
        return JS_UNDEFINED;
    }
    dom_string *ds; dom_string_create((const uint8_t *)value, strlen(value), &ds); dom_node_set_node_value((dom_node *)priv->node, ds); dom_string_unref(ds);
    return JS_UNDEFINED;
}

JSValue wisp_node_textContent_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (sn) {
            WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[(uint64_t)(uintptr_t)priv->node];
            return JS_NewString(ctx, wisp_string_ref_data(wisp_shm_dom, sns->value));
        }
        return JS_NULL;
    }
    dom_string *text = NULL;
    dom_node_get_text_content((dom_node *)priv->node, &text);
    if (text) {
        JSValue val = JS_NewStringLen(ctx, (const char *)dom_string_data(text), dom_string_byte_length(text));
        dom_string_unref(text);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_node_textContent_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (sn) {
            WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[(uint64_t)(uintptr_t)priv->node];
            sns->value = wisp_shm_alloc_string(wisp_shm_dom, value);
        }
        shm_mutation_enqueue(wisp_shm_dom, SHM_MUTATION_SET_TEXT_CONTENT, (uint64_t)(uintptr_t)priv->node, 0, 0, NULL, value);
        return JS_UNDEFINED;
    }
    dom_string *ds; dom_string_create((const uint8_t *)value, strlen(value), &ds); dom_node_set_text_content((dom_node *)priv->node, ds); dom_string_unref(ds);
    return JS_UNDEFINED;
}

int qjs_init_node(JSContext *ctx) {
    JSValue global_obj = JS_GetGlobalObject(ctx);

    /* Check if already initialized on this global object */
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_node_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    /* Initialize the class and prototype using the generated function */
    qjs_init_node_gen(ctx);

    JSValue proto = JS_GetClassProto(ctx, qjs_node_class_id);
    if (!JS_IsObject(proto)) {
        JS_FreeValue(ctx, proto);
        proto = JS_NewObject(ctx);
        JS_SetClassProto(ctx, qjs_node_class_id, JS_DupValue(ctx, proto));
    }
    JSValue et_proto = JS_GetClassProto(ctx, qjs_eventtarget_class_id);
    if (JS_IsObject(proto) && JS_IsObject(et_proto)) JS_SetPrototype(ctx, proto, et_proto);
    JS_FreeValue(ctx, et_proto);
    JS_FreeValue(ctx, proto);

    /* Mark as initialized */
    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_node_init", JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);

    return 0;
}
