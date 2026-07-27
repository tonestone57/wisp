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

JSValue wisp_node_cloneNode_impl(JSContext *ctx, QJSNodePrivate *priv, bool deep)
{
    if (!priv || !priv->node) return JS_EXCEPTION;
    if (wisp_is_js_process) return JS_NULL;
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

JSValue wisp_node_insertBefore_impl(JSContext *ctx, QJSNodePrivate *priv, void * node, void * child)
{
    if (!priv || !priv->node || !node) return JS_EXCEPTION;
    if (wisp_is_js_process) {
        uint64_t parent_id = (uint64_t)(uintptr_t)priv->node;
        uint64_t child_id = (uint64_t)(uintptr_t)node;
        uint64_t ref_id = child ? (uint64_t)(uintptr_t)child : 0;
        shm_mutation_enqueue(wisp_shm_dom, SHM_MUTATION_INSERT_BEFORE, parent_id, child_id, ref_id, NULL, NULL);

        WispCompactNode *parent_shm = find_shm_node(wisp_shm_dom, parent_id);
        WispCompactNode *child_shm = find_shm_node(wisp_shm_dom, child_id);
        if (parent_shm && child_shm) {
            if (child_shm->parent_id != 0) {
                WispCompactNode *old_parent = find_shm_node(wisp_shm_dom, child_shm->parent_id);
                if (old_parent) {
                    if (old_parent->first_child_id == child_id) old_parent->first_child_id = child_shm->next_sibling_id;
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
    if (wisp_is_js_process) {
        uint64_t parent_id = (uint64_t)(uintptr_t)priv->node;
        uint64_t child_id = (uint64_t)(uintptr_t)node;
        shm_mutation_enqueue(wisp_shm_dom, SHM_MUTATION_APPEND_CHILD, parent_id, child_id, 0, NULL, NULL);

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
    if (wisp_is_js_process) {
        uint64_t parent_id = (uint64_t)(uintptr_t)priv->node;
        uint64_t new_child_id = (uint64_t)(uintptr_t)node;
        uint64_t old_child_id = (uint64_t)(uintptr_t)child;
        shm_mutation_enqueue(wisp_shm_dom, SHM_MUTATION_REPLACE_CHILD, parent_id, new_child_id, old_child_id, NULL, NULL);

        WispCompactNode *parent_shm = find_shm_node(wisp_shm_dom, parent_id);
        WispCompactNode *new_child_shm = find_shm_node(wisp_shm_dom, new_child_id);
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
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (sn) {
            WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[(uint64_t)(uintptr_t)priv->node];
            return JS_NewString(ctx, sns->name);
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
    if (!priv || !priv->node) return JS_UNDEFINED;
    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (sn && sn->node_type == DOM_DOCUMENT_NODE) {
            return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)priv->node);
        }
        // Scan for the document node
        if (wisp_shm_dom) {
            WispCompactNode *nodes_arr = shm_dom_get_nodes(wisp_shm_dom);
            for (uint32_t i = 0; i < wisp_shm_dom->node_count; i++) {
                if (nodes_arr[i].node_type == 9) {
                    return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)i);
                }
            }
        }
        return JS_NULL;
    }
    struct dom_document *doc = NULL;
    dom_node_get_owner_document((dom_node *)priv->node, &doc);
    if (doc) {
        JSValue val = qjs_wrap_node(ctx, (dom_node *)doc);
        dom_node_unref((dom_node *)doc);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_node_parentNode_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
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
    if (!priv || !priv->node) return JS_UNDEFINED;
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
    if (!priv || !priv->node) return JS_UNDEFINED;
    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
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
    if (!priv || !priv->node) return JS_UNDEFINED;
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
    if (!priv || !priv->node) return JS_UNDEFINED;
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
    if (!priv || !priv->node) return JS_UNDEFINED;
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
            return JS_NewString(ctx, sns->value);
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
            strncpy(sns->value, value, SHM_DOM_STRING_MAX - 1);
            sns->value[SHM_DOM_STRING_MAX - 1] = '\0';
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
            return JS_NewString(ctx, sns->value);
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
            strncpy(sns->value, value, SHM_DOM_STRING_MAX - 1);
            sns->value[SHM_DOM_STRING_MAX - 1] = '\0';
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
