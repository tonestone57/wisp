#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include "JSStorage.gen.h"

typedef struct {
    char *key;
    char *value;
} StorageEntry;

typedef struct {
    StorageEntry *entries;
    uint32_t count;
    uint32_t capacity;
} StorageStore;

static void storage_finalizer_manual(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_storage_class_id);
    if (priv) {
        if (priv->magic == QJS_DOM_MAGIC && priv->node) {
            StorageStore *store = (StorageStore *)priv->node;
            for (uint32_t i = 0; i < store->count; i++) {
                free(store->entries[i].key);
                free(store->entries[i].value);
            }
            free(store->entries);
            free(store);
        }
        free(priv);
    }
}

JSValue wisp_storage_getItem_impl(JSContext *ctx, QJSNodePrivate *priv, const char * key)
{
    if (!priv || !priv->node || !key) return JS_NULL;
    StorageStore *store = (StorageStore *)priv->node;
    for (uint32_t i = 0; i < store->count; i++) {
        if (strcmp(store->entries[i].key, key) == 0) {
            return JS_NewString(ctx, store->entries[i].value);
        }
    }
    return JS_NULL;
}

JSValue wisp_storage_setItem_impl(JSContext *ctx, QJSNodePrivate *priv, const char * key, const char * value)
{
    if (!priv || !priv->node || !key || !value) return JS_UNDEFINED;
    StorageStore *store = (StorageStore *)priv->node;
    for (uint32_t i = 0; i < store->count; i++) {
        if (strcmp(store->entries[i].key, key) == 0) {
            free(store->entries[i].value);
            store->entries[i].value = strdup(value);
            return JS_UNDEFINED;
        }
    }

    if (store->count >= store->capacity) {
        store->capacity = store->capacity ? store->capacity * 2 : 4;
        StorageEntry *new_entries = realloc(store->entries, store->capacity * sizeof(StorageEntry));
        if (!new_entries) return JS_ThrowOutOfMemory(ctx);
        store->entries = new_entries;
    }

    store->entries[store->count].key = strdup(key);
    store->entries[store->count].value = strdup(value);
    store->count++;
    return JS_UNDEFINED;
}

JSValue wisp_storage_removeItem_impl(JSContext *ctx, QJSNodePrivate *priv, const char * key)
{
    if (!priv || !priv->node || !key) return JS_UNDEFINED;
    StorageStore *store = (StorageStore *)priv->node;
    for (uint32_t i = 0; i < store->count; i++) {
        if (strcmp(store->entries[i].key, key) == 0) {
            free(store->entries[i].key);
            free(store->entries[i].value);
            for (uint32_t j = i; j < store->count - 1; j++) {
                store->entries[j] = store->entries[j + 1];
            }
            store->count--;
            return JS_UNDEFINED;
        }
    }
    return JS_UNDEFINED;
}

JSValue wisp_storage_clear_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    StorageStore *store = (StorageStore *)priv->node;
    for (uint32_t i = 0; i < store->count; i++) {
        free(store->entries[i].key);
        free(store->entries[i].value);
    }
    free(store->entries);
    store->entries = NULL;
    store->count = 0;
    store->capacity = 0;
    return JS_UNDEFINED;
}

JSValue wisp_storage_key_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index)
{
    if (!priv || !priv->node) return JS_NULL;
    StorageStore *store = (StorageStore *)priv->node;
    if (index < store->count) {
        return JS_NewString(ctx, store->entries[index].key);
    }
    return JS_NULL;
}

JSValue wisp_storage_length_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewInt32(ctx, 0);
    StorageStore *store = (StorageStore *)priv->node;
    return JS_NewInt32(ctx, store->count);
}

int qjs_init_storage(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSRuntime *rt = JS_GetRuntime(ctx);

    /* Check if already initialized on this global object */
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_storage_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    if (qjs_storage_class_id == 0) {
        JS_NewClassID(rt, &qjs_storage_class_id);
    }

    JSClassDef qjs_storage_class_manual = {
        "Storage",
        .finalizer = storage_finalizer_manual,
    };

    if (!JS_IsRegisteredClass(rt, qjs_storage_class_id)) {
        JS_NewClass(rt, qjs_storage_class_id, &qjs_storage_class_manual);
    }

    /* Initialize the class and prototype using the generated function */
    qjs_init_storage_gen(ctx);

    JSValue proto = JS_GetClassProto(ctx, qjs_storage_class_id);
    if (!JS_IsObject(proto)) {
        JS_FreeValue(ctx, proto);
        proto = JS_NewObject(ctx);
        JS_SetClassProto(ctx, qjs_storage_class_id, JS_DupValue(ctx, proto));
    }
    JS_FreeValue(ctx, proto);

    StorageStore *local_s = calloc(1, sizeof(StorageStore));
    JSValue localStorage = qjs_new_storage(ctx, local_s, false);
    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_localStorage", localStorage, 0);

    StorageStore *session_s = calloc(1, sizeof(StorageStore));
    JSValue sessionStorage = qjs_new_storage(ctx, session_s, false);
    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_sessionStorage", sessionStorage, 0);

    /* Mark as initialized */
    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_storage_init", JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);

    return 0;
}
