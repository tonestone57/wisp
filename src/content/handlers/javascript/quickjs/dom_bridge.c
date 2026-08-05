#include "dom_bridge.h"

#include <strings.h>
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include <wisp/utils/corestrings.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "utils/hashmap.h"
#include "wisp/utils/shm_dom.h"


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

extern bool wisp_is_js_process;
extern shm_dom_t *wisp_shm_dom;

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
    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)node);
        type = sn ? (dom_node_type)sn->node_type : 0;
    } else {
        dom_node_get_node_type(node, &type);
    }

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
        /* Map stores a WEAK reference. We don't increment the refcount
         * because the JS object's finalizer will remove it from the map. */
        *val_ptr = wrapper;
        if (!wisp_is_js_process) dom_node_ref(node);
    } else {
        /* Failed to insert into map, but we still have the wrapper.
         * This shouldn't normally happen unless OOM. */
        NSLOG(wisp, WARNING, "Failed to insert node wrapper into bridge map");
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
            hashmap_remove(map, &key);
            if (!wisp_is_js_process) dom_node_unref(node);
        }
    }
}

void qjs_bridge_unref_node(struct dom_node *node)
{
    if (!wisp_is_js_process && node) {
        dom_node_unref(node);
    }
}

int qjs_init_dom_bridge(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    hashmap_t *map = JS_GetRuntimeOpaque(rt);
    if (!map) {
        map = hashmap_create(&bridge_map_params);
        if (!map) {
            NSLOG(wisp, ERROR, "Failed to create DOM bridge hashmap");
            return -1;
        }
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

/**
 * Fully clean up the DOM bridge for a runtime.
 * This is called during heap destruction.
 */
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
                hashmap_remove(map, &cleanup.keys[i]);
                if (!wisp_is_js_process) dom_node_unref(cleanup.keys[i].node);
            }
        }
        free(cleanup.keys);
        hashmap_destroy(map);
        JS_SetRuntimeOpaque(rt, NULL);
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
    }
    return false;
}

typedef enum {
    QJS_COMBINATOR_NONE,
    QJS_COMBINATOR_DESCENDANT,      /* ' ' */
    QJS_COMBINATOR_CHILD,           /* '>' */
    QJS_COMBINATOR_ADJACENT_SIBLING, /* '+' */
    QJS_COMBINATOR_GENERAL_SIBLING  /* '~' */
} qjs_combinator_t;

typedef struct {
    char *tag;
    char *id;
    char **classes;
    uint32_t class_count;
    bool universal;
} qjs_compound_selector_t;

typedef struct {
    qjs_compound_selector_t compound;
    qjs_combinator_t next_combinator; /* Combinator connecting this to the next component (to its right) */
} qjs_selector_component_t;

typedef struct {
    qjs_selector_component_t *components;
    uint32_t component_count;
} qjs_selector_group_t;

typedef struct {
    qjs_selector_group_t *groups;
    uint32_t group_count;
} qjs_selector_root_t;

static void qjs_selector_root_free(qjs_selector_root_t *root)
{
    if (!root) return;
    for (uint32_t i = 0; i < root->group_count; i++) {
        qjs_selector_group_t *group = &root->groups[i];
        for (uint32_t j = 0; j < group->component_count; j++) {
            qjs_selector_component_t *comp = &group->components[j];
            free(comp->compound.tag);
            free(comp->compound.id);
            for (uint32_t k = 0; k < comp->compound.class_count; k++) {
                free(comp->compound.classes[k]);
            }
            free(comp->compound.classes);
        }
        free(group->components);
    }
    free(root->groups);
    free(root);
}

static const char *qjs_selector_skip_ws(const char *s)
{
    while (*s && (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')) s++;
    return s;
}

static qjs_selector_root_t *qjs_selector_parse(const char *selector_str)
{
    qjs_selector_root_t *root = calloc(1, sizeof(qjs_selector_root_t));
    if (!root) return NULL;

    const char *p = selector_str;
    while (*p) {
        p = qjs_selector_skip_ws(p);
        if (!*p) break;

        const char *comma = strchr(p, ',');
        size_t group_len = comma ? (size_t)(comma - p) : strlen(p);
        char *group_str = strndup(p, group_len);

        root->groups = realloc(root->groups, (root->group_count + 1) * sizeof(qjs_selector_group_t));
        qjs_selector_group_t *group = &root->groups[root->group_count++];
        memset(group, 0, sizeof(*group));

        const char *gp = group_str;
        while (*gp) {
            gp = qjs_selector_skip_ws(gp);
            if (!*gp) break;

            group->components = realloc(group->components, (group->component_count + 1) * sizeof(qjs_selector_component_t));
            qjs_selector_component_t *comp = &group->components[group->component_count++];
            memset(comp, 0, sizeof(*comp));

            /* Find end of compound selector and the next combinator */
            const char *cp = gp;
            while (*cp && !strchr(" \t\n\r>+~", *cp)) cp++;

            size_t comp_len = (size_t)(cp - gp);
            char *comp_str = strndup(gp, comp_len);

            /* Parse compound selector part */
            const char *csp = comp_str;
            if (*csp == '*') { comp->compound.universal = true; csp++; }
            while (*csp) {
                if (*csp == '#') {
                    const char *id_start = ++csp;
                    while (*csp && !strchr("#.", *csp)) csp++;
                    if (comp->compound.id) free(comp->compound.id);
                    comp->compound.id = strndup(id_start, (size_t)(csp - id_start));
                } else if (*csp == '.') {
                    const char *class_start = ++csp;
                    while (*csp && !strchr("#.", *csp)) csp++;
                    comp->compound.classes = realloc(comp->compound.classes, (comp->compound.class_count + 1) * sizeof(char *));
                    comp->compound.classes[comp->compound.class_count++] = strndup(class_start, (size_t)(csp - class_start));
                } else {
                    const char *tag_start = csp;
                    while (*csp && !strchr("#.", *csp)) csp++;
                    if (comp->compound.tag) free(comp->compound.tag);
                    comp->compound.tag = strndup(tag_start, (size_t)(csp - tag_start));
                }
            }
            free(comp_str);

            gp = qjs_selector_skip_ws(cp);
            if (*gp == '>') { comp->next_combinator = QJS_COMBINATOR_CHILD; gp++; }
            else if (*gp == '+') { comp->next_combinator = QJS_COMBINATOR_ADJACENT_SIBLING; gp++; }
            else if (*gp == '~') { comp->next_combinator = QJS_COMBINATOR_GENERAL_SIBLING; gp++; }
            else if (*gp) { comp->next_combinator = QJS_COMBINATOR_DESCENDANT; }
            else { comp->next_combinator = QJS_COMBINATOR_NONE; }
        }
        free(group_str);
        if (comma) p = comma + 1; else break;
    }
    return root;
}

static bool qjs_compound_selector_matches(struct dom_node *node, const qjs_compound_selector_t *comp)
{
    dom_node_type type;
    dom_node_get_node_type(node, &type);
    if (type != DOM_ELEMENT_NODE) return false;

    if (comp->universal) {
        /* Matches any element */
    } else if (comp->tag) {
        dom_string *tag = NULL;
        dom_element_get_tag_name((dom_element *)node, &tag);
        if (tag) {
            dom_string *target = NULL;
            dom_string_create((const uint8_t *)comp->tag, strlen(comp->tag), &target);
            bool match = target ? dom_string_caseless_isequal(tag, target) : false;
            if (target) dom_string_unref(target);
            dom_string_unref(tag);
            if (!match) return false;
        } else return false;
    }

    if (comp->id) {
        dom_string *id = NULL;
        dom_element_get_attribute((dom_element *)node, corestring_dom_id, &id);
        if (id) {
            dom_string *target = NULL;
            dom_string_create((const uint8_t *)comp->id, strlen(comp->id), &target);
            bool match = target ? dom_string_isequal(id, target) : false;
            if (target) dom_string_unref(target);
            dom_string_unref(id);
            if (!match) return false;
        } else return false;
    }

    for (uint32_t i = 0; i < comp->class_count; i++) {
        dom_string *cls = NULL;
        dom_element_get_attribute((dom_element *)node, corestring_dom_class, &cls);
        if (cls) {
            const char *data = dom_string_data(cls);
            size_t len = dom_string_byte_length(cls);
            const char *target = comp->classes[i];
            size_t target_len = strlen(target);
            bool found = false;
            if (len >= target_len) {
                for (size_t j = 0; j <= len - target_len; j++) {
                    if ((j == 0 || data[j - 1] == ' ') && (j + target_len == len || data[j + target_len] == ' ')) {
                        if (strncmp(data + j, target, target_len) == 0) {
                            found = true;
                            break;
                        }
                    }
                }
            }
            dom_string_unref(cls);
            if (!found) return false;
        } else return false;
    }

    return true;
}

static bool qjs_selector_group_matches(struct dom_node *node, const qjs_selector_group_t *group)
{
    if (group->component_count == 0) return false;

    /* Start matching from the last component (the rightmost one) */
    int comp_idx = group->component_count - 1;
    if (!qjs_compound_selector_matches(node, &group->components[comp_idx].compound)) return false;

    struct dom_node *curr = dom_node_ref(node);
    while (comp_idx > 0) {
        qjs_combinator_t comb = group->components[comp_idx - 1].next_combinator;
        comp_idx--;
        const qjs_compound_selector_t *target = &group->components[comp_idx].compound;

        if (comb == QJS_COMBINATOR_CHILD) {
            struct dom_node *parent = NULL;
            dom_node_get_parent_node(curr, &parent);
            dom_node_unref(curr);
            if (!parent || !qjs_compound_selector_matches(parent, target)) {
                if (parent) dom_node_unref(parent);
                return false;
            }
            curr = parent;
        } else if (comb == QJS_COMBINATOR_DESCENDANT) {
            struct dom_node *parent = NULL;
            bool found = false;
            while (true) {
                dom_node_get_parent_node(curr, &parent);
                dom_node_unref(curr);
                if (!parent) break;
                if (qjs_compound_selector_matches(parent, target)) {
                    curr = parent;
                    found = true;
                    break;
                }
                curr = parent;
            }
            if (!found) return false;
        } else if (comb == QJS_COMBINATOR_ADJACENT_SIBLING) {
            struct dom_node *prev = NULL;
            dom_node_get_previous_sibling(curr, &prev);
            while (prev) {
                dom_node_type type;
                dom_node_get_node_type(prev, &type);
                if (type == DOM_ELEMENT_NODE) break;
                struct dom_node *tmp = NULL;
                dom_node_get_previous_sibling(prev, &tmp);
                dom_node_unref(prev);
                prev = tmp;
            }
            dom_node_unref(curr);
            if (!prev || !qjs_compound_selector_matches(prev, target)) {
                if (prev) dom_node_unref(prev);
                return false;
            }
            curr = prev;
        } else if (comb == QJS_COMBINATOR_GENERAL_SIBLING) {
            struct dom_node *prev = NULL;
            bool found = false;
            while (true) {
                struct dom_node *tmp = NULL;
                dom_node_get_previous_sibling(curr, &tmp);
                dom_node_unref(curr);
                curr = tmp;
                if (!curr) break;
                dom_node_type type;
                dom_node_get_node_type(curr, &type);
                if (type == DOM_ELEMENT_NODE && qjs_compound_selector_matches(curr, target)) {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
    }

    dom_node_unref(curr);
    return true;
}

static bool qjs_get_node_type(struct dom_node *node, dom_node_type *out_type)
{
    if (wisp_is_js_process) {
        if (!wisp_shm_dom) return false;
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)node);
        if (!sn) return false;
        *out_type = (dom_node_type)sn->node_type;
        return true;
    }
    return dom_node_get_node_type(node, out_type) == DOM_NO_ERR;
}

void qjs_finalise_dom_bridge(JSRuntime *rt, JSContext *ctx)
{
    hashmap_t *map = JS_GetRuntimeOpaque(rt);
    if (!map) return;

    bridge_cleanup_t cleanup = { .ctx = ctx, .nodes = NULL, .count = 0, .capacity = 0 };
    hashmap_iterate(map, bridge_cleanup_ctx_cb, &cleanup);

    /* First pass: unref all non-document nodes first to ensure their reference
     * counts drop to 0 before the document node is finalising. This prevents
     * document teardown from being blocked by pending child node references. */
    for (size_t i = 0; i < cleanup.count; i++) {
        dom_node_type type = 0;
        bool has_type = qjs_get_node_type(cleanup.nodes[i], &type);

        if (has_type && type == DOM_DOCUMENT_NODE) {
            continue;
        }
        bridge_key_t key = { .ctx = ctx, .node = cleanup.nodes[i] };
        JSValue *val = hashmap_lookup(map, &key);
        if (val) {
            hashmap_remove(map, &key);
            if (!wisp_is_js_process) dom_node_unref(cleanup.nodes[i]);
        }
    }

    /* Second pass: unref all document nodes */
    for (size_t i = 0; i < cleanup.count; i++) {
        dom_node_type type = 0;
        bool has_type = qjs_get_node_type(cleanup.nodes[i], &type);

        if (has_type && type != DOM_DOCUMENT_NODE) {
            continue;
        }
        bridge_key_t key = { .ctx = ctx, .node = cleanup.nodes[i] };
        JSValue *val = hashmap_lookup(map, &key);
        if (val) {
            hashmap_remove(map, &key);
            if (!wisp_is_js_process) dom_node_unref(cleanup.nodes[i]);
        }
    }
    free(cleanup.nodes);
}

static bool qjs_compound_selector_matches_shm(uint32_t node_id, const qjs_compound_selector_t *comp)
{
    if (!wisp_shm_dom || node_id == 0) return false;
    WispCompactNode *nodes = shm_dom_get_nodes(wisp_shm_dom);
    WispNodeStrings *strings = shm_dom_get_node_strings(wisp_shm_dom);

    WispCompactNode *sn = &nodes[node_id];
    WispNodeStrings *sns = &strings[node_id];

    if (sn->node_type != 1) return false; // Must be ELEMENT

    if (comp->universal) {
        /* Matches any element */
    } else if (comp->tag) {
        const char *tag = wisp_string_ref_data(wisp_shm_dom, sns->tag_name);
        if (!tag || strcasecmp(tag, comp->tag) != 0) return false;
    }

    if (comp->id) {
        const char *id_val = NULL;
        uint32_t limit = sns->attr_count < WISP_SHM_MAX_ATTRIBUTES ? sns->attr_count : WISP_SHM_MAX_ATTRIBUTES;
        for (uint32_t i = 0; i < limit; i++) {
            const char *attr_name = wisp_string_ref_data(wisp_shm_dom, sns->attrs[i].name);
            if (attr_name && strcasecmp(attr_name, "id") == 0) {
                id_val = wisp_string_ref_data(wisp_shm_dom, sns->attrs[i].value);
                break;
            }
        }
        if (!id_val || strcmp(id_val, comp->id) != 0) return false;
    }

    for (uint32_t i = 0; i < comp->class_count; i++) {
        const char *cls_val = NULL;
        uint32_t limit = sns->attr_count < WISP_SHM_MAX_ATTRIBUTES ? sns->attr_count : WISP_SHM_MAX_ATTRIBUTES;
        for (uint32_t j = 0; j < limit; j++) {
            const char *attr_name = wisp_string_ref_data(wisp_shm_dom, sns->attrs[j].name);
            if (attr_name && strcasecmp(attr_name, "class") == 0) {
                cls_val = wisp_string_ref_data(wisp_shm_dom, sns->attrs[j].value);
                break;
            }
        }
        if (!cls_val) return false;

        size_t len = strlen(cls_val);
        const char *target = comp->classes[i];
        size_t target_len = strlen(target);
        bool found = false;
        if (len >= target_len) {
            for (size_t j = 0; j <= len - target_len; j++) {
                if ((j == 0 || cls_val[j - 1] == ' ') && (j + target_len == len || cls_val[j + target_len] == ' ')) {
                    if (strncmp(cls_val + j, target, target_len) == 0) {
                        found = true;
                        break;
                    }
                }
            }
        }
        if (!found) return false;
    }

    return true;
}

static bool qjs_selector_group_matches_shm(uint32_t node_id, const qjs_selector_group_t *group)
{
    if (group->component_count == 0) return false;
    if (!wisp_shm_dom) return false;
    WispCompactNode *nodes = shm_dom_get_nodes(wisp_shm_dom);

    int comp_idx = group->component_count - 1;
    if (!qjs_compound_selector_matches_shm(node_id, &group->components[comp_idx].compound)) return false;

    uint32_t curr = node_id;
    while (comp_idx > 0) {
        qjs_combinator_t comb = group->components[comp_idx - 1].next_combinator;
        comp_idx--;
        const qjs_compound_selector_t *target = &group->components[comp_idx].compound;

        if (comb == QJS_COMBINATOR_CHILD) {
            uint32_t parent = nodes[curr].parent_id;
            if (parent == 0 || parent == curr || !qjs_compound_selector_matches_shm(parent, target)) {
                return false;
            }
            curr = parent;
        } else if (comb == QJS_COMBINATOR_DESCENDANT) {
            uint32_t parent = nodes[curr].parent_id;
            bool found = false;
            while (parent != 0 && parent != curr) {
                if (qjs_compound_selector_matches_shm(parent, target)) {
                    curr = parent;
                    found = true;
                    break;
                }
                curr = parent;
                parent = nodes[curr].parent_id;
            }
            if (!found) return false;
        } else if (comb == QJS_COMBINATOR_ADJACENT_SIBLING) {
            uint32_t prev = nodes[curr].prev_sibling_id;
            while (prev != 0) {
                if (nodes[prev].node_type == 1) break;
                prev = nodes[prev].prev_sibling_id;
            }
            if (prev == 0 || !qjs_compound_selector_matches_shm(prev, target)) {
                return false;
            }
            curr = prev;
        } else if (comb == QJS_COMBINATOR_GENERAL_SIBLING) {
            uint32_t prev = nodes[curr].prev_sibling_id;
            bool found = false;
            while (prev != 0) {
                if (qjs_compound_selector_matches_shm(prev, target)) {
                    curr = prev;
                    found = true;
                    break;
                }
                prev = nodes[prev].prev_sibling_id;
            }
            if (!found) return false;
        }
    }

    return true;
}

JSValue qjs_dom_query_selector_internal_shm(JSContext *ctx, uint32_t root_id, const char *selector, bool all)
{
    qjs_selector_root_t *parsed = qjs_selector_parse(selector);
    if (!parsed) return all ? JS_NewArray(ctx) : JS_NULL;

    JSValue result = all ? JS_NewArray(ctx) : JS_NULL;
    uint32_t count = 0;

    if (!wisp_shm_dom || root_id == 0) {
        qjs_selector_root_free(parsed);
        return result;
    }

    shm_dom_lock_read(wisp_shm_dom);

    WispCompactNode *nodes = shm_dom_get_nodes(wisp_shm_dom);
    uint32_t curr = nodes[root_id].first_child_id;

    uint32_t max_iterations = 100000;

    while (curr != 0 && max_iterations-- > 0) {
        nodes = shm_dom_get_nodes(wisp_shm_dom);
        bool match = false;
        for (uint32_t i = 0; i < parsed->group_count; i++) {
            if (qjs_selector_group_matches_shm(curr, &parsed->groups[i])) {
                match = true;
                break;
            }
        }

        if (match) {
            if (!all) {
                JSValue val = qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)curr);
                shm_dom_unlock_read(wisp_shm_dom);
                qjs_selector_root_free(parsed);
                return val;
            }
            JS_SetPropertyUint32(ctx, result, count++, qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)curr));
        }

        uint32_t next = nodes[curr].first_child_id;
        if (next != 0) {
            curr = next;
            continue;
        }

        next = nodes[curr].next_sibling_id;
        if (next != 0) {
            curr = next;
            continue;
        }

        while (curr != 0) {
            uint32_t parent = nodes[curr].parent_id;
            if (parent == 0 || parent == root_id) {
                curr = 0;
                break;
            }
            next = nodes[parent].next_sibling_id;
            if (next != 0) {
                curr = next;
                break;
            }
            curr = parent;
        }
    }

    shm_dom_unlock_read(wisp_shm_dom);
    qjs_selector_root_free(parsed);
    return result;
}

JSValue qjs_dom_query_selector_internal(JSContext *ctx, struct dom_node *root, const char *selector, bool all)
{
    if (wisp_is_js_process) {
        return qjs_dom_query_selector_internal_shm(ctx, (uint32_t)(uintptr_t)root, selector, all);
    }
    qjs_selector_root_t *parsed = qjs_selector_parse(selector);
    if (!parsed) return all ? JS_NewArray(ctx) : JS_NULL;

    JSValue result = all ? JS_NewArray(ctx) : JS_NULL;
    uint32_t count = 0;

    struct dom_node *curr = NULL;
    dom_node_get_first_child(root, &curr);
    while (curr) {
        bool match = false;
        for (uint32_t i = 0; i < parsed->group_count; i++) {
            if (qjs_selector_group_matches(curr, &parsed->groups[i])) {
                match = true;
                break;
            }
        }

        if (match) {
            if (!all) {
                JSValue val = qjs_wrap_node(ctx, curr);
                dom_node_unref(curr);
                qjs_selector_root_free(parsed);
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

    qjs_selector_root_free(parsed);
    return result;
}
