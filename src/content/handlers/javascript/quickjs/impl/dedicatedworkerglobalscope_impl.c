#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include "JSDedicatedWorkerGlobalScope.gen.h"
#include "JSMessageEvent.gen.h"
#include "qjs_internal.h"
#include "wisp_subsystem.h"
#include "wisp/desktop/gui_table.h"
#include "wisp/misc.h"
#include "base64_helper.h"

int qjs_init_dedicatedworkerglobalscope(JSContext *ctx) {
    qjs_init_dedicatedworkerglobalscope_gen(ctx);

    JSValue global = JS_GetGlobalObject(ctx);
    JSValue proto = JS_GetClassProto(ctx, qjs_dedicatedworkerglobalscope_class_id);
    JSValue wg_proto = JS_GetClassProto(ctx, qjs_workerglobalscope_class_id);
    if (JS_IsObject(proto) && JS_IsObject(wg_proto)) {
        JS_SetPrototype(ctx, proto, wg_proto);
    }
    JS_FreeValue(ctx, wg_proto);

    if (JS_IsObject(proto)) {
        JS_SetPrototype(ctx, global, proto);
    }

    JS_FreeValue(ctx, proto);
    JS_FreeValue(ctx, global);
    return 0;
}

extern void wisp_worker_notify_main_thread(WispWorkerHandle *h);

JSValue wisp_dedicatedworkerglobalscope_postMessage_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue message, JSValue transfer) {
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (!t || !t->is_worker || !t->worker_handle) return JS_UNDEFINED;

    WispWorkerHandle *h = (WispWorkerHandle *)t->worker_handle;
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue json = JS_GetPropertyStr(ctx, global, "JSON");
    JSValue stringify = JS_GetPropertyStr(ctx, json, "stringify");
    JSValue json_str = JS_Call(ctx, stringify, json, 1, &message);
    JS_FreeValue(ctx, stringify);
    JS_FreeValue(ctx, json);
    JS_FreeValue(ctx, global);
    if (JS_IsException(json_str)) {
        return json_str;
    }
    const char *cstr = JS_ToCString(ctx, json_str);
    if (!cstr) {
        JS_FreeValue(ctx, json_str);
        return JS_Throw(ctx, JS_NewString(ctx, "DataCloneError"));
    }
    size_t size = strlen(cstr);
    uint8_t *data = (uint8_t *)strdup(cstr);
    JS_FreeCString(ctx, cstr);
    JS_FreeValue(ctx, json_str);

    WispMessage *msg = calloc(1, sizeof(*msg));
    if (!msg) {
        free(data);
        return JS_ThrowOutOfMemory(ctx);
    }
    msg->type = WISP_MSG_TYPE_DATA;
    msg->data = data;
    msg->size = size;

    wisp_message_queue_push(&h->from_worker, msg);
    wisp_worker_notify_main_thread(h);
    return JS_UNDEFINED;
}

JSValue wisp_dedicatedworkerglobalscope_onmessage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue val = JS_GetPropertyStr(ctx, global, "onmessage");
    JS_FreeValue(ctx, global);
    return val;
}

JSValue wisp_dedicatedworkerglobalscope_onmessage_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "onmessage", JS_DupValue(ctx, value));
    JS_FreeValue(ctx, global);
    return JS_UNDEFINED;
}

/* WorkerGlobalScope implementation */
JSValue wisp_workerglobalscope_self_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_GetGlobalObject(ctx);
}

JSValue wisp_workerglobalscope_close_impl(JSContext *ctx, QJSNodePrivate *priv) {
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (t && t->is_worker && t->worker_handle) {
        ((WispWorkerHandle *)t->worker_handle)->running = false;
    }
    return JS_UNDEFINED;
}

JSValue wisp_workerglobalscope_atob_impl(JSContext *ctx, QJSNodePrivate *priv, const char * atob) {
    return common_atob(ctx, atob);
}

JSValue wisp_workerglobalscope_btoa_impl(JSContext *ctx, QJSNodePrivate *priv, const char * btoa) {
    return common_btoa(ctx, btoa);
}

typedef struct WispFetchRequest {
    const char *url;
    uint8_t *out_buffer;
    size_t out_len;
    bool success;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool completed;
} WispFetchRequest;

extern void wisp_worker_fetch_cb(void *p);

JSValue wisp_workerglobalscope_importScripts_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue urls) {
    extern struct wisp_table *guit;
    uint32_t argc = 0;
    JSValue len_val = JS_GetPropertyStr(ctx, urls, "length");
    JS_ToUint32(ctx, &argc, len_val);
    JS_FreeValue(ctx, len_val);

    for (uint32_t i = 0; i < argc; i++) {
        JSValue url_val = JS_GetPropertyUint32(ctx, urls, i);
        const char *url_str = JS_ToCString(ctx, url_val);
        JS_FreeValue(ctx, url_val);
        if (!url_str) continue;

        WispFetchRequest req;
        memset(&req, 0, sizeof(req));
        req.url = url_str;
        pthread_mutex_init(&req.mutex, NULL);
        pthread_cond_init(&req.cond, NULL);

        guit->misc->schedule(0, wisp_worker_fetch_cb, &req);

        pthread_mutex_lock(&req.mutex);
        while (!req.completed) {
            pthread_cond_wait(&req.cond, &req.mutex);
        }
        pthread_mutex_unlock(&req.mutex);

        if (req.success && req.out_buffer) {
            JSValue res = js_eval_with_aot_cache(ctx, req.out_buffer, req.out_len, url_str, JS_EVAL_TYPE_GLOBAL);
            JS_FreeValue(ctx, res);
            free(req.out_buffer);
        }

        pthread_mutex_destroy(&req.mutex);
        pthread_cond_destroy(&req.cond);
        JS_FreeCString(ctx, url_str);
    }
    return JS_UNDEFINED;
}
