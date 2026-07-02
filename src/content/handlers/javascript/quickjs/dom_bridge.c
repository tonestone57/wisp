#include "dom_bridge.h"

#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include <wisp/utils/corestrings.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "utils/hashmap.h"


typedef struct {
    JSContext *ctx;
    struct dom_node *node;
} bridge_key_t;

static uint32_t bridge_key_hash(void *key) {
    bridge_key_t *k = key;
    return (uint32_t)((uintptr_t)k->ctx ^ (uintptr_t)k->node);
}

static bool bridge_key_eq(void *key1, void *key2) {
    bridge_key_t *k1 = key1, *k2 = key2;
    return k1->ctx == k2->ctx && k1->node == k2->node;
}

static void *bridge_key_clone(void *key) {
    bridge_key_t *k = malloc(sizeof(*k));
    if (k) memcpy(k, key, sizeof(*k));
    return k;
}

static void bridge_key_destroy(void *key) {
    free(key);
}

static void bridge_value_destroy(void *val) {
    free(val);
}

static void *bridge_value_alloc(void *key) {
    return malloc(sizeof(JSValue));
}

static hashmap_parameters_t bridge_map_params = {
    .key_clone = bridge_key_clone,
    .key_hash = bridge_key_hash,
    .key_eq = bridge_key_eq,
    .key_destroy = bridge_key_destroy,
    .value_alloc = bridge_value_alloc,
    .value_destroy = bridge_value_destroy
};

JSValue qjs_wrap_node(JSContext *ctx, struct dom_node *node)
{
    if (node == NULL) return JS_NULL;

    JSRuntime *rt = JS_GetRuntime(ctx);
    hashmap_t *map = JS_GetRuntimeOpaque(rt);
    if (!map) {
        NSLOG(wisp, ERROR, "DOM bridge map not initialized");
        return JS_UNDEFINED;
    }

    bridge_key_t key = { ctx, node };
    JSValue *existing = hashmap_lookup(map, &key);
    if (existing) {
        return JS_DupValue(ctx, *existing);
    }

    dom_node_type type;
    dom_node_get_node_type(node, &type);

    JSValue wrapper;
    switch (type) {
        case DOM_ELEMENT_NODE:
            wrapper = qjs_new_element(ctx, node, true);
            break;
        case DOM_DOCUMENT_NODE:
            wrapper = qjs_new_document(ctx, node, true);
            break;
        case DOM_TEXT_NODE:
            wrapper = qjs_new_text(ctx, node, true);
            break;
        case DOM_ATTRIBUTE_NODE:
            wrapper = qjs_new_attr(ctx, node, true);
            break;
        default:
            wrapper = qjs_new_node(ctx, node, true);
            break;
    }

    JSValue *val_ptr = hashmap_insert(map, &key);
    if (val_ptr) {
        /* Map stores a strong reference. We must increment the refcount
         * because qjs_new_* returned a reference that we are now returning
         * to the caller, and the map needs its own reference. */
        *val_ptr = JS_DupValue(ctx, wrapper);
        dom_node_ref(node);
    }

    return wrapper;
}

void qjs_bridge_remove_node(JSRuntime *rt, struct dom_node *node, JSContext *ctx)
{
    hashmap_t *map = JS_GetRuntimeOpaque(rt);
    if (map) {
        bridge_key_t key = { ctx, node };
        JSValue *val = hashmap_lookup(map, &key);
        if (val) {
            JS_FreeValueRT(rt, *val);
        }
        hashmap_remove(map, &key);
        dom_node_unref(node);
    }
}

int qjs_init_dom_bridge(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    hashmap_t *map = JS_GetRuntimeOpaque(rt);
    if (!map) {
        map = hashmap_create(&bridge_map_params);
        JS_SetRuntimeOpaque(rt, map);
    }
    return 0;
}

typedef struct {
    JSRuntime *rt;
    bridge_key_t *keys;
    size_t count;
    size_t capacity;
} bridge_full_cleanup_t;

static bool bridge_full_cleanup_cb(void *key, void *val, void *pw) {
    bridge_full_cleanup_t *cleanup = pw;
    if (cleanup->count == cleanup->capacity) {
        cleanup->capacity = cleanup->capacity ? cleanup->capacity * 2 : 16;
        bridge_key_t *new_keys = realloc(cleanup->keys, cleanup->capacity * sizeof(bridge_key_t));
        if (!new_keys) return true;
        cleanup->keys = new_keys;
    }
    cleanup->keys[cleanup->count++] = *(bridge_key_t *)key;
    return false;
}

void qjs_bridge_cleanup(JSRuntime *rt)
{
    hashmap_t *map = JS_GetRuntimeOpaque(rt);
    if (map) {
        bridge_full_cleanup_t cleanup = { .rt = rt, .keys = NULL, .count = 0, .capacity = 0 };
        hashmap_iterate(map, bridge_full_cleanup_cb, &cleanup);

        for (size_t i = 0; i < cleanup.count; i++) {
            /* Entries must be removed from map before unref to avoid re-entrant UAF. */
            JSValue *val = hashmap_lookup(map, &cleanup.keys[i]);
            if (val) {
                JS_FreeValueRT(rt, *val);
            }
            hashmap_remove(map, &cleanup.keys[i]);
            dom_node_unref(cleanup.keys[i].node);
        }
        free(cleanup.keys);
        hashmap_destroy(map);
    }
}

typedef struct {
    JSContext *ctx;
    struct dom_node **nodes;
    size_t count;
    size_t capacity;
} bridge_cleanup_t;

static bool bridge_cleanup_ctx_cb(void *key, void *val, void *pw)
{
    bridge_cleanup_t *cleanup = pw;
    bridge_key_t *k = key;
    if (k->ctx == cleanup->ctx) {
        if (cleanup->count == cleanup->capacity) {
            cleanup->capacity = cleanup->capacity ? cleanup->capacity * 2 : 16;
            struct dom_node **new_nodes = realloc(cleanup->nodes, cleanup->capacity * sizeof(struct dom_node *));
            if (!new_nodes) return true;
            cleanup->nodes = new_nodes;
        }
        cleanup->nodes[cleanup->count++] = k->node;
        dom_node_ref(k->node);
    }
    return false;
}

void qjs_finalise_dom_bridge(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    hashmap_t *map = JS_GetRuntimeOpaque(rt);
    if (!map) return;

    bridge_cleanup_t cleanup = { .ctx = ctx, .nodes = NULL, .count = 0, .capacity = 0 };
    hashmap_iterate(map, bridge_cleanup_ctx_cb, &cleanup);

    for (size_t i = 0; i < cleanup.count; i++) {
        bridge_key_t key = { .ctx = ctx, .node = cleanup.nodes[i] };
        /* Entries must be removed from map before unref to avoid re-entrant UAF. */
        JSValue *val = hashmap_lookup(map, &key);
        if (val) {
            JS_FreeValue(ctx, *val);
        }
        hashmap_remove(map, &key);
        dom_node_unref(cleanup.nodes[i]);
    }
    free(cleanup.nodes);
}

static bool qjs_dom_match_node(struct dom_node *node, const char *selector)
{
    dom_node_type type;
    dom_node_get_node_type(node, &type);
    if (type != DOM_ELEMENT_NODE) return false;

    if (selector[0] == '#') {
        dom_string *id = NULL;
        dom_element_get_attribute((dom_element *)node, corestring_dom_id, &id);
        if (id) {
            bool match = false;
            dom_string *target = NULL;
            dom_string_create((const uint8_t *)selector + 1, strlen(selector + 1), &target);
            if (target) {
                match = dom_string_isequal(id, target);
                dom_string_unref(target);
            }
            dom_string_unref(id);
            return match;
        }
    } else if (selector[0] == '.') {
        dom_string *cls = NULL;
        dom_element_get_attribute((dom_element *)node, corestring_dom_class, &cls);
        if (cls) {
            const char *data = dom_string_data(cls);
            size_t len = dom_string_byte_length(cls);
            const char *target = selector + 1;
            size_t target_len = strlen(target);
            bool found = false;
            if (len >= target_len) {
                for (size_t i = 0; i <= len - target_len; i++) {
                    if ((i == 0 || data[i - 1] == ' ') && (i + target_len == len || data[i + target_len] == ' ')) {
                        if (strncmp(data + i, target, target_len) == 0) {
                        found = true;
                        break;
                    }
                }
                }
            }
            dom_string_unref(cls);
            return found;
        }
    } else if (strcmp(selector, "*") == 0) {
        return true;
    } else {
        dom_string *tag = NULL;
        dom_element_get_tag_name((dom_element *)node, &tag);
        if (tag) {
            bool match = false;
            dom_string *target = NULL;
            dom_string_create((const uint8_t *)selector, strlen(selector), &target);
            if (target) {
                match = dom_string_caseless_isequal(tag, target);
                dom_string_unref(target);
            }
            dom_string_unref(tag);
            return match;
        }
    }
    return false;
}

JSValue qjs_dom_query_selector_internal(JSContext *ctx, struct dom_node *root, const char *selector, bool all)
{
    JSValue result = all ? JS_NewArray(ctx) : JS_NULL;
    uint32_t count = 0;

    struct dom_node *curr = NULL;
    dom_node_get_first_child(root, &curr);
    while (curr) {
        if (qjs_dom_match_node(curr, selector)) {
            if (!all) {
                JSValue val = qjs_wrap_node(ctx, curr);
                dom_node_unref(curr);
                return val;
            }
            JS_SetPropertyUint32(ctx, result, count++, qjs_wrap_node(ctx, curr));
        }

        struct dom_node *next = NULL;
        dom_node_get_first_child(curr, &next);
        if (next) {
            dom_node_unref(curr);
            curr = next;
            continue;
        }

        dom_node_get_next_sibling(curr, &next);
        if (next) {
            dom_node_unref(curr);
            curr = next;
            continue;
        }

        while (curr) {
            struct dom_node *parent = NULL;
            dom_node_get_parent_node(curr, &parent);
            dom_node_unref(curr);
            if (parent == NULL || parent == root) {
                if (parent) dom_node_unref(parent);
                curr = NULL;
                break;
            }
            dom_node_get_next_sibling(parent, &next);
            if (next) {
                dom_node_unref(parent);
                curr = next;
                break;
            }
            curr = parent;
        }
    }

    return result;
}
