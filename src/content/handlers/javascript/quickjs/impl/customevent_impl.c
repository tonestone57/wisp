#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSCustomEvent.gen.h"

struct custom_event_ctx_map {
    void *node;
    JSContext *ctx;
    struct custom_event_ctx_map *next;
};

static struct custom_event_ctx_map *event_ctx_list = NULL;
static pthread_mutex_t event_ctx_mutex = PTHREAD_MUTEX_INITIALIZER;

static int JS_DeletePropertyStr(JSContext *ctx, JSValueConst obj, const char *prop) {
    JSAtom atom = JS_NewAtom(ctx, prop);
    if (atom == JS_ATOM_NULL) return -1;
    int ret = JS_DeleteProperty(ctx, obj, atom, 0);
    JS_FreeAtom(ctx, atom);
    return ret;
}

static void save_custom_event_detail(JSContext *ctx, void *node, JSValue detail) {
    if (!node) return;
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue registry = JS_GetPropertyStr(ctx, global, "__custom_event_details");
    if (JS_IsUndefined(registry)) {
        registry = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, global, "__custom_event_details", JS_DupValue(ctx, registry));
    }
    char key[64];
    snprintf(key, sizeof(key), "%p", node);
    JS_SetPropertyStr(ctx, registry, key, JS_DupValue(ctx, detail));
    JS_FreeValue(ctx, registry);
    JS_FreeValue(ctx, global);

    /* Save node -> ctx mapping */
    pthread_mutex_lock(&event_ctx_mutex);
    struct custom_event_ctx_map *entry = malloc(sizeof(*entry));
    if (entry) {
        entry->node = node;
        entry->ctx = ctx;
        entry->next = event_ctx_list;
        event_ctx_list = entry;
    }
    pthread_mutex_unlock(&event_ctx_mutex);
}

void wisp_dom_event_cleanup_ctx(JSContext *ctx) {
    pthread_mutex_lock(&event_ctx_mutex);
    struct custom_event_ctx_map **prev = &event_ctx_list;
    struct custom_event_ctx_map *curr = event_ctx_list;
    while (curr) {
        if (curr->ctx == ctx) {
            *prev = curr->next;
            struct custom_event_ctx_map *to_free = curr;
            curr = curr->next;
            free(to_free);
        } else {
            prev = &curr->next;
            curr = curr->next;
        }
    }
    pthread_mutex_unlock(&event_ctx_mutex);
}

static JSValue get_custom_event_detail(JSContext *ctx, void *node) {
    if (!node) return JS_NULL;
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue registry = JS_GetPropertyStr(ctx, global, "__custom_event_details");
    if (JS_IsObject(registry)) {
        char key[64];
        snprintf(key, sizeof(key), "%p", node);
        JSValue val = JS_GetPropertyStr(ctx, registry, key);
        JS_FreeValue(ctx, registry);
        JS_FreeValue(ctx, global);
        if (JS_IsUndefined(val)) return JS_NULL;
        return val;
    }
    JS_FreeValue(ctx, registry);
    JS_FreeValue(ctx, global);
    return JS_NULL;
}

void wisp_dom_event_destroyed_hook(void *evt) {
    if (!evt) return;
    pthread_mutex_lock(&event_ctx_mutex);
    struct custom_event_ctx_map **prev = &event_ctx_list;
    struct custom_event_ctx_map *curr = event_ctx_list;
    while (curr) {
        if (curr->node == evt) {
            JSContext *ctx = curr->ctx;
            JSValue global = JS_GetGlobalObject(ctx);
            JSValue registry = JS_GetPropertyStr(ctx, global, "__custom_event_details");
            if (JS_IsObject(registry)) {
                char key[64];
                snprintf(key, sizeof(key), "%p", evt);
                JS_DeletePropertyStr(ctx, registry, key);
            }
            JS_FreeValue(ctx, registry);
            JS_FreeValue(ctx, global);

            *prev = curr->next;
            free(curr);
            break;
        }
        prev = &curr->next;
        curr = curr->next;
    }
    pthread_mutex_unlock(&event_ctx_mutex);
}

extern bool wisp_is_js_process;

JSValue wisp_customevent_constructor_impl(JSContext *ctx, const char * type, JSValue eventInitDict) {
    dom_event *evt = NULL;
    dom_event_create(&evt);
    if (!evt) return JS_ThrowOutOfMemory(ctx);

    dom_string *type_str = NULL;
    dom_string_create((const uint8_t *)type, strlen(type), &type_str);
    if (!type_str) {
        if (!wisp_is_js_process) {
            dom_event_unref(evt);
        }
        return JS_ThrowOutOfMemory(ctx);
    }

    bool bubbles = false;
    bool cancelable = false;
    JSValue detail = JS_NULL;

    if (JS_IsObject(eventInitDict)) {
        JSValue b_val = JS_GetPropertyStr(ctx, eventInitDict, "bubbles");
        if (!JS_IsUndefined(b_val)) bubbles = JS_ToBool(ctx, b_val);
        JS_FreeValue(ctx, b_val);

        JSValue c_val = JS_GetPropertyStr(ctx, eventInitDict, "cancelable");
        if (!JS_IsUndefined(c_val)) cancelable = JS_ToBool(ctx, c_val);
        JS_FreeValue(ctx, c_val);

        detail = JS_GetPropertyStr(ctx, eventInitDict, "detail");
    }

    dom_event_init(evt, type_str, bubbles, cancelable);
    dom_event_set_is_trusted(evt, false);
    dom_string_unref(type_str);

    if (!JS_IsUndefined(detail) && !JS_IsNull(detail)) {
        save_custom_event_detail(ctx, evt, detail);
    }
    if (JS_IsObject(eventInitDict)) {
        JS_FreeValue(ctx, detail);
    }

    JSValue obj = qjs_new_customevent(ctx, evt, false);
    dom_event_unref(evt);
    return obj;
}

JSValue wisp_customevent_initCustomEvent_0_impl(JSContext *ctx, QJSNodePrivate *priv, const char * type, bool bubbles, bool cancelable, JSValue detail) {
    if (!priv || !priv->node) return JS_UNDEFINED;
    dom_event *evt = (dom_event *)priv->node;

    dom_string *type_str = NULL;
    dom_string_create((const uint8_t *)type, strlen(type), &type_str);
    if (type_str) {
        dom_event_init(evt, type_str, bubbles, cancelable);
        dom_string_unref(type_str);
    }

    if (!JS_IsUndefined(detail)) {
        save_custom_event_detail(ctx, evt, detail);
    }
    return JS_UNDEFINED;
}

JSValue wisp_customevent_initCustomEvent_1_impl(JSContext *ctx, QJSNodePrivate *priv, const char * typeArg, bool bubblesArg, bool cancelableArg, JSValue detailArg) {
    return wisp_customevent_initCustomEvent_0_impl(ctx, priv, typeArg, bubblesArg, cancelableArg, detailArg);
}

JSValue wisp_customevent_detail_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NULL;
    return get_custom_event_detail(ctx, priv->node);
}
