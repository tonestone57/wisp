#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include "JSWorker.gen.h"
#include "JSMessageEvent.gen.h"
#include "JSErrorEvent.gen.h"
#include "qjs_internal.h"
#include "wisp_subsystem.h"

typedef struct QJSWorkerPrivate {
    WispWorkerHandle *handle;
    JSContext *ctx;
    JSValue onmessage;
    JSValue onerror;
} QJSWorkerPrivate;

static void worker_finalizer(JSRuntime *rt, JSValue val) {
    QJSWorkerPrivate *priv = JS_GetOpaque(val, qjs_worker_class_id);
    if (priv) {
        JS_FreeValueRT(rt, priv->onmessage);
        JS_FreeValueRT(rt, priv->onerror);
        if (priv->handle) {
            priv->handle->worker_priv = NULL;
            priv->handle->running = false; /* Signal worker thread to terminate */
            wisp_worker_handle_unref(priv->handle);
        }
        free(priv);
    }
}

static JSClassDef wisp_worker_class = { "Worker", .finalizer = worker_finalizer };

int qjs_init_worker(JSContext *ctx) {
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_worker_class_id == 0) JS_NewClassID(rt, &qjs_worker_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_worker_class_id)) JS_NewClass(rt, qjs_worker_class_id, &wisp_worker_class);
    qjs_init_worker_gen(ctx);
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue proto = JS_GetClassProto(ctx, qjs_worker_class_id);
    JSValue et_proto = JS_GetClassProto(ctx, qjs_eventtarget_class_id);
    if (JS_IsObject(proto) && JS_IsObject(et_proto)) JS_SetPrototype(ctx, proto, et_proto);
    JS_FreeValue(ctx, et_proto);
    JSValue ctor = JS_NewCFunction2(ctx, (JSCFunction *)wisp_worker_constructor_impl, "Worker", 1, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, ctor, proto);
    JS_SetPropertyStr(ctx, global, "Worker", ctor);
    JS_FreeValue(ctx, proto);
    JS_FreeValue(ctx, global);
    return 0;
}

JSValue wisp_worker_constructor_impl(JSContext *ctx, const char * scriptURL) {
    JSValue obj = JS_NewObjectClass(ctx, qjs_worker_class_id);
    if (JS_IsException(obj)) return obj;
    QJSWorkerPrivate *priv = calloc(1, sizeof(QJSWorkerPrivate));
    if (!priv) { JS_FreeValue(ctx, obj); return JS_ThrowOutOfMemory(ctx); }
    priv->ctx = ctx; priv->onmessage = JS_NULL; priv->onerror = JS_NULL;
    priv->handle = wisp_subsystem_spawn_worker(scriptURL);
    if (!priv->handle) { free(priv); JS_FreeValue(ctx, obj); return JS_ThrowTypeError(ctx, "Worker limit reached"); }
    priv->handle->worker_priv = priv;
    JS_SetOpaque(obj, priv);
    return obj;
}

JSValue wisp_worker_postMessage_impl(JSContext *ctx, QJSNodePrivate *priv_node, JSValue message, JSValue transfer) {
    QJSWorkerPrivate *priv = (QJSWorkerPrivate *)priv_node;
    if (!priv || !priv->handle || priv->handle->terminated) return JS_UNDEFINED;
    size_t size;
    uint8_t *data = JS_WriteObject(ctx, &size, message, JS_WRITE_OBJ_SAB | JS_WRITE_OBJ_REFERENCE);
    if (!data) return JS_Throw(ctx, JS_NewString(ctx, "DataCloneError"));
    WispMessage *msg = calloc(1, sizeof(*msg));
    if (!msg) {
        js_free(ctx, data);
        return JS_ThrowOutOfMemory(ctx);
    }
    msg->type = WISP_MSG_TYPE_DATA; msg->data = data; msg->size = size;
    wisp_message_queue_push(&priv->handle->to_worker, msg);
    return JS_UNDEFINED;
}

JSValue wisp_worker_terminate_impl(JSContext *ctx, QJSNodePrivate *priv_node) {
    QJSWorkerPrivate *priv = (QJSWorkerPrivate *)priv_node;
    if (priv && priv->handle) priv->handle->terminated = true;
    return JS_UNDEFINED;
}

JSValue wisp_worker_onmessage_get_impl(JSContext *ctx, QJSNodePrivate *priv_node) {
    QJSWorkerPrivate *priv = (QJSWorkerPrivate *)priv_node;
    return priv ? JS_DupValue(ctx, priv->onmessage) : JS_NULL;
}

JSValue wisp_worker_onmessage_set_impl(JSContext *ctx, QJSNodePrivate *priv_node, JSValue value) {
    QJSWorkerPrivate *priv = (QJSWorkerPrivate *)priv_node;
    if (priv) { JS_FreeValue(ctx, priv->onmessage); priv->onmessage = JS_DupValue(ctx, value); }
    return JS_UNDEFINED;
}

JSValue wisp_worker_onerror_get_impl(JSContext *ctx, QJSNodePrivate *priv_node) {
    QJSWorkerPrivate *priv = (QJSWorkerPrivate *)priv_node;
    return priv ? JS_DupValue(ctx, priv->onerror) : JS_NULL;
}

JSValue wisp_worker_onerror_set_impl(JSContext *ctx, QJSNodePrivate *priv_node, JSValue value) {
    QJSWorkerPrivate *priv = (QJSWorkerPrivate *)priv_node;
    if (priv) { JS_FreeValue(ctx, priv->onerror); priv->onerror = JS_DupValue(ctx, value); }
    return JS_UNDEFINED;
}

void wisp_dispatch_message_to_worker_object(WispWorkerHandle *h, WispMessage *msg) {
    QJSWorkerPrivate *priv = (QJSWorkerPrivate *)h->worker_priv;
    if (!priv || !priv->ctx) return;
    JSContext *ctx = priv->ctx;
    if (msg->type == WISP_MSG_TYPE_DATA) {
        JSValue msg_data = JS_ReadObject(ctx, msg->data, msg->size, JS_READ_OBJ_SAB | JS_READ_OBJ_REFERENCE);
        extern JSValue qjs_new_messageevent_manual(JSContext *ctx, const char *type, JSValue data);
        JSValue event = qjs_new_messageevent_manual(ctx, "message", msg_data);
        if (JS_IsFunction(ctx, priv->onmessage)) {
            JSValue ret = JS_Call(ctx, priv->onmessage, JS_UNDEFINED, 1, &event);
            JS_FreeValue(ctx, ret);
            JSContext *ctx1;
            int job_ret;
            while ((job_ret = JS_ExecutePendingJob(JS_GetRuntime(ctx), &ctx1)) != 0) {
                if (job_ret < 0) {
                    JSValue exc = JS_GetException(ctx1);
                    const char *exc_str = JS_ToCString(ctx1, exc);
                    NSLOG(wisp, WARNING, "JS Error in microtask: %s", exc_str ? exc_str : "unknown");
                    if (exc_str) JS_FreeCString(ctx1, exc_str);
                    JS_FreeValue(ctx1, exc);
                }
            }
        }
        JS_FreeValue(ctx, event); JS_FreeValue(ctx, msg_data);
    } else if (msg->type == WISP_MSG_TYPE_ERROR) {
        extern JSValue qjs_new_errorevent_manual(JSContext *ctx, const char *msg, const char *file, int line, int col);
        JSValue event = qjs_new_errorevent_manual(ctx, msg->error_message, msg->filename, msg->line_number, msg->col_number);
        if (JS_IsFunction(ctx, priv->onerror)) {
            JSValue ret = JS_Call(ctx, priv->onerror, JS_UNDEFINED, 1, &event);
            JS_FreeValue(ctx, ret);
            JSContext *ctx1;
            int job_ret;
            while ((job_ret = JS_ExecutePendingJob(JS_GetRuntime(ctx), &ctx1)) != 0) {
                if (job_ret < 0) {
                    JSValue exc = JS_GetException(ctx1);
                    const char *exc_str = JS_ToCString(ctx1, exc);
                    NSLOG(wisp, WARNING, "JS Error in microtask: %s", exc_str ? exc_str : "unknown");
                    if (exc_str) JS_FreeCString(ctx1, exc_str);
                    JS_FreeValue(ctx1, exc);
                }
            }
        }
        JS_FreeValue(ctx, event);
    }
}
