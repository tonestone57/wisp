
#include "dom_bridge.h"

extern JSClassID qjs_eventtarget_class_id;
extern JSClassID qjs_node_class_id;
extern JSClassID qjs_element_class_id;
extern JSClassID qjs_document_class_id;
extern JSClassID qjs_text_class_id;
extern JSClassID qjs_attr_class_id;
extern JSClassID qjs_namednodemap_class_id;
extern JSClassID qjs_htmlcollection_class_id;
extern JSClassID qjs_window_class_id;
extern JSClassID qjs_event_class_id;
extern JSClassID qjs_console_class_id;
extern JSClassID qjs_location_class_id;
extern JSClassID qjs_navigator_class_id;
extern JSClassID qjs_storage_class_id;
extern JSClassID qjs_xhr_class_id;
#include "qjs_internal.h"
#include <wisp/utils/log.h>
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
    /* We don't free the JSValue here because it's a weak-like ref managed by GC and finalizers */
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
        *val_ptr = wrapper; /* Map stores the JSValue (no extra ref, weak-like) */
    }

    return wrapper;
}

void qjs_bridge_remove_node(JSRuntime *rt, struct dom_node *node, JSContext *ctx)
{
    hashmap_t *map = JS_GetRuntimeOpaque(rt);
    if (map) {
        bridge_key_t key = { ctx, node };
        hashmap_remove(map, &key);
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

void qjs_finalise_dom_bridge(JSContext *ctx) { (void)ctx; }

static bool bridge_cleanup_iter(void *key, void *val, void *pw)
{
    JSRuntime *rt = pw;
    JSValue *v = val;
    JS_FreeValueRT(rt, *v);
    return false;
}

void qjs_bridge_cleanup(JSRuntime *rt)
{
    hashmap_t *map = JS_GetRuntimeOpaque(rt);
    if (map) {
        /* Set opaque to NULL first so that finalizers triggered by
         * JS_FreeValueRT don't try to access/mutate the map during iteration.
         */
        JS_SetRuntimeOpaque(rt, NULL);
        hashmap_iterate(map, bridge_cleanup_iter, rt);
        hashmap_destroy(map);
    }
}
