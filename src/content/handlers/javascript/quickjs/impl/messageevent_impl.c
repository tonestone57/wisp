#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include "JSMessageEvent.gen.h"
#include "JSEvent.gen.h"
#include "qjs_internal.h"

typedef struct QJSMessageEventPrivate {
    uint32_t magic;
    void *node;
    JSContext *ctx;
    bool is_dom_node;
    JSValue data;
    char *origin;
    char *lastEventId;
} QJSMessageEventPrivate;

static void js_messageevent_finalizer(JSRuntime *rt, JSValue val) {
    QJSMessageEventPrivate *priv = JS_GetOpaque(val, qjs_messageevent_class_id);
    if (priv) {
        JS_FreeValueRT(rt, priv->data);
        free(priv->origin);
        free(priv->lastEventId);
        free(priv);
    }
}

static JSClassDef wisp_messageevent_class = { "MessageEvent", .finalizer = js_messageevent_finalizer };

JSValue qjs_new_messageevent_manual(JSContext *ctx, const char *type, JSValue data) {
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_messageevent_class_id == 0) JS_NewClassID(rt, &qjs_messageevent_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_messageevent_class_id)) JS_NewClass(rt, qjs_messageevent_class_id, &wisp_messageevent_class);

    JSValue obj = JS_NewObjectClass(ctx, qjs_messageevent_class_id);
    if (JS_IsException(obj)) return obj;

    QJSMessageEventPrivate *priv = calloc(1, sizeof(*priv));
    if (!priv) { JS_FreeValue(ctx, obj); return JS_ThrowOutOfMemory(ctx); }
    priv->magic = QJS_DOM_MAGIC;
    priv->node = NULL;
    priv->ctx = ctx;
    priv->is_dom_node = false;
    priv->data = JS_DupValue(ctx, data);
    priv->origin = strdup("");
    priv->lastEventId = strdup("");
    if (!priv->origin || !priv->lastEventId) {
        JS_FreeValue(ctx, priv->data);
        free(priv->origin);
        free(priv->lastEventId);
        free(priv);
        JS_FreeValue(ctx, obj);
        return JS_ThrowOutOfMemory(ctx);
    }

    JS_DefinePropertyValueStr(ctx, obj, "type", JS_NewString(ctx, type ? type : "message"), JS_PROP_C_W_E);
    JS_SetOpaque(obj, priv);
    return obj;
}

int qjs_init_messageevent(JSContext *ctx) {
    qjs_init_messageevent_gen(ctx);

    JSValue proto = JS_GetClassProto(ctx, qjs_messageevent_class_id);
    JSValue event_proto = JS_GetClassProto(ctx, qjs_event_class_id);
    if (JS_IsObject(proto) && JS_IsObject(event_proto)) {
        JS_SetPrototype(ctx, proto, event_proto);
    }
    JS_FreeValue(ctx, event_proto);
    JS_FreeValue(ctx, proto);
    return 0;
}

JSValue wisp_messageevent_constructor_impl(JSContext *ctx, const char * type, JSValue eventInitDict) {
    JSValue data = JS_UNDEFINED;
    if (JS_IsObject(eventInitDict)) {
        data = JS_GetPropertyStr(ctx, eventInitDict, "data");
    }
    JSValue obj = qjs_new_messageevent_manual(ctx, type ? type : "message", data);
    JS_FreeValue(ctx, data);
    return obj;
}

JSValue wisp_messageevent_data_get_impl(JSContext *ctx, QJSNodePrivate *priv_node) {
    QJSMessageEventPrivate *priv = (QJSMessageEventPrivate *)priv_node;
    return priv ? JS_DupValue(ctx, priv->data) : JS_UNDEFINED;
}

JSValue wisp_messageevent_origin_get_impl(JSContext *ctx, QJSNodePrivate *priv_node) {
    QJSMessageEventPrivate *priv = (QJSMessageEventPrivate *)priv_node;
    return JS_NewString(ctx, (priv && priv->origin) ? priv->origin : "");
}

JSValue wisp_messageevent_lastEventId_get_impl(JSContext *ctx, QJSNodePrivate *priv_node) {
    QJSMessageEventPrivate *priv = (QJSMessageEventPrivate *)priv_node;
    return JS_NewString(ctx, (priv && priv->lastEventId) ? priv->lastEventId : "");
}

JSValue wisp_messageevent_source_get_impl(JSContext *ctx, QJSNodePrivate *priv_node) { return JS_NULL; }
JSValue wisp_messageevent_ports_get_impl(JSContext *ctx, QJSNodePrivate *priv_node) { return JS_NewArray(ctx); }
JSValue wisp_messageevent_initMessageEvent_impl(JSContext *ctx, QJSNodePrivate *priv, const char * type, bool bubbles, bool cancelable, JSValue data, const char * origin, const char * lastEventId, JSValue source, JSValue ports) { return JS_UNDEFINED; }
