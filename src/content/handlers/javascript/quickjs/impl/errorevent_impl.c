#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include "JSErrorEvent.gen.h"
#include "JSEvent.gen.h"
#include "qjs_internal.h"

typedef struct QJSErrorEventPrivate {
    uint32_t magic;
    void *node;
    JSContext *ctx;
    bool is_dom_node;
    char *message;
    char *filename;
    int lineno;
    int colno;
    JSValue error;
} QJSErrorEventPrivate;

static void js_errorevent_finalizer(JSRuntime *rt, JSValue val) {
    QJSErrorEventPrivate *priv = JS_GetOpaque(val, qjs_errorevent_class_id);
    if (priv) {
        free(priv->message);
        free(priv->filename);
        JS_FreeValueRT(rt, priv->error);
        free(priv);
    }
}

static JSClassDef wisp_errorevent_class = { "ErrorEvent", .finalizer = js_errorevent_finalizer };

JSValue qjs_new_errorevent_manual(JSContext *ctx, const char *msg, const char *file, int line, int col) {
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_errorevent_class_id == 0) JS_NewClassID(rt, &qjs_errorevent_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_errorevent_class_id)) JS_NewClass(rt, qjs_errorevent_class_id, &wisp_errorevent_class);

    JSValue obj = JS_NewObjectClass(ctx, qjs_errorevent_class_id);
    if (JS_IsException(obj)) return obj;

    QJSErrorEventPrivate *priv = calloc(1, sizeof(*priv));
    if (!priv) { JS_FreeValue(ctx, obj); return JS_ThrowOutOfMemory(ctx); }
    priv->magic = QJS_DOM_MAGIC;
    priv->node = NULL;
    priv->ctx = ctx;
    priv->is_dom_node = false;
    priv->message = strdup(msg ? msg : "");
    priv->filename = strdup(file ? file : "");
    if (!priv->message || !priv->filename) {
        free(priv->message);
        free(priv->filename);
        free(priv);
        JS_FreeValue(ctx, obj);
        return JS_ThrowOutOfMemory(ctx);
    }
    priv->lineno = line;
    priv->colno = col;
    priv->error = JS_NULL;

    JS_SetOpaque(obj, priv);
    return obj;
}

int qjs_init_errorevent(JSContext *ctx) {
    qjs_init_errorevent_gen(ctx);
    JSValue proto = JS_GetClassProto(ctx, qjs_errorevent_class_id);
    JSValue event_proto = JS_GetClassProto(ctx, qjs_event_class_id);
    if (JS_IsObject(proto) && JS_IsObject(event_proto)) JS_SetPrototype(ctx, proto, event_proto);
    JS_FreeValue(ctx, event_proto);
    JS_FreeValue(ctx, proto);
    return 0;
}

JSValue wisp_errorevent_constructor_impl(JSContext *ctx, const char * type, JSValue eventInitDict) {
    const char *msg = NULL;
    if (JS_IsObject(eventInitDict)) {
        JSValue v = JS_GetPropertyStr(ctx, eventInitDict, "message");
        if (!JS_IsUndefined(v) && !JS_IsNull(v)) {
            msg = JS_ToCString(ctx, v);
        }
        JS_FreeValue(ctx, v);
    }
    JSValue obj = qjs_new_errorevent_manual(ctx, msg ? msg : "", "", 0, 0);
    if (msg) JS_FreeCString(ctx, msg);
    return obj;
}

JSValue wisp_errorevent_message_get_impl(JSContext *ctx, QJSNodePrivate *priv_node) {
    QJSErrorEventPrivate *priv = (QJSErrorEventPrivate *)priv_node;
    return (priv && priv->message) ? JS_NewString(ctx, priv->message) : JS_UNDEFINED;
}

JSValue wisp_errorevent_filename_get_impl(JSContext *ctx, QJSNodePrivate *priv_node) {
    QJSErrorEventPrivate *priv = (QJSErrorEventPrivate *)priv_node;
    return (priv && priv->filename) ? JS_NewString(ctx, priv->filename) : JS_UNDEFINED;
}

JSValue wisp_errorevent_lineno_get_impl(JSContext *ctx, QJSNodePrivate *priv_node) {
    QJSErrorEventPrivate *priv = (QJSErrorEventPrivate *)priv_node;
    return priv ? JS_NewInt32(ctx, priv->lineno) : JS_UNDEFINED;
}

JSValue wisp_errorevent_colno_get_impl(JSContext *ctx, QJSNodePrivate *priv_node) {
    QJSErrorEventPrivate *priv = (QJSErrorEventPrivate *)priv_node;
    return priv ? JS_NewInt32(ctx, priv->colno) : JS_UNDEFINED;
}

JSValue wisp_errorevent_error_get_impl(JSContext *ctx, QJSNodePrivate *priv_node) {
    QJSErrorEventPrivate *priv = (QJSErrorEventPrivate *)priv_node;
    return priv ? JS_DupValue(ctx, priv->error) : JS_UNDEFINED;
}
