#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSCustomEvent.gen.h"

#define MAX_CUSTOM_EVENT_DETAILS 256
static struct {
    void *node;
    JSValue detail;
    JSContext *ctx;
} custom_event_details[MAX_CUSTOM_EVENT_DETAILS];
static int custom_event_details_idx = 0;

static void save_custom_event_detail(JSContext *ctx, void *node, JSValue detail) {
    if (!node) return;
    int idx = custom_event_details_idx;
    if (custom_event_details[idx].node != NULL && custom_event_details[idx].ctx == ctx) {
        JS_FreeValue(ctx, custom_event_details[idx].detail);
    }
    custom_event_details[idx].node = node;
    custom_event_details[idx].detail = JS_DupValue(ctx, detail);
    custom_event_details[idx].ctx = ctx;
    custom_event_details_idx = (idx + 1) % MAX_CUSTOM_EVENT_DETAILS;
}

static JSValue get_custom_event_detail(JSContext *ctx, void *node) {
    if (!node) return JS_NULL;
    for (int i = 0; i < MAX_CUSTOM_EVENT_DETAILS; i++) {
        if (custom_event_details[i].node == node && custom_event_details[i].ctx == ctx) {
            return JS_DupValue(ctx, custom_event_details[i].detail);
        }
    }
    return JS_NULL;
}

JSValue wisp_customevent_constructor_impl(JSContext *ctx, const char * type, JSValue eventInitDict) {
    dom_event *evt = NULL;
    dom_event_create(&evt);
    if (!evt) return JS_ThrowOutOfMemory(ctx);

    dom_string *type_str = NULL;
    dom_string_create((const uint8_t *)type, strlen(type), &type_str);
    if (!type_str) {
        dom_event_unref(evt);
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
