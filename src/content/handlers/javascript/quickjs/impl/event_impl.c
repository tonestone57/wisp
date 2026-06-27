#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include <nsutils/time.h>
#include "JSEvent.gen.h"

static void js_event_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_event_class_id);
    if (priv) {
        if (priv->magic == QJS_DOM_MAGIC && priv->node) {
            dom_event_unref((dom_event *)priv->node);
        }
        free(priv);
    }
}

static JSClassDef wisp_event_class = { "Event", .finalizer = js_event_finalizer };

JSValue qjs_new_event(JSContext *ctx, void *node, bool is_dom_node)
{
    JSValue obj = JS_NewObjectClass(ctx, qjs_event_class_id);
    if (JS_IsException(obj)) return obj;
    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));
    if (!priv) { JS_FreeValue(ctx, obj); return JS_ThrowOutOfMemory(ctx); }
    priv->magic = QJS_DOM_MAGIC; priv->node = node; priv->ctx = ctx; priv->is_dom_node = false;
    if (node) dom_event_ref((dom_event *)node);
    JS_SetOpaque(obj, priv); return obj;
}

static JSValue js_event_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv)
{
    if (argc < 1) return JS_ThrowTypeError(ctx, "Event type required");
    const char *type = JS_ToCString(ctx, argv[0]);
    dom_string *type_dom = NULL;
    dom_string_create((const uint8_t *)type, strlen(type), &type_dom);
    dom_event *evt = NULL;
    dom_event_create(&evt);
    if (evt) dom_event_init(evt, type_dom, false, false);
    dom_string_unref(type_dom);
    JS_FreeCString(ctx, type);
    if (!evt) return JS_ThrowInternalError(ctx, "Failed to create event");
    JSValue obj = qjs_new_event(ctx, evt, false);
    dom_event_unref(evt);
    return obj;
}

JSValue wisp_event_stopPropagation_impl(JSContext *ctx, QJSNodePrivate *priv) { dom_event_stop_propagation(priv->node); return JS_UNDEFINED; }
JSValue wisp_event_stopImmediatePropagation_impl(JSContext *ctx, QJSNodePrivate *priv) { dom_event_stop_propagation(priv->node); return JS_UNDEFINED; }
JSValue wisp_event_preventDefault_impl(JSContext *ctx, QJSNodePrivate *priv) { dom_event_prevent_default(priv->node); return JS_UNDEFINED; }
JSValue wisp_event_initEvent_impl(JSContext *ctx, QJSNodePrivate *priv, const char * type, bool bubbles, bool cancelable) {
    dom_string *type_dom = NULL; dom_string_create((const uint8_t *)type, strlen(type), &type_dom);
    dom_event_init(priv->node, type_dom, bubbles, cancelable);
    dom_string_unref(type_dom); return JS_UNDEFINED;
}

JSValue wisp_event_type_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    dom_string *type_dom = NULL; dom_event_get_type(priv->node, &type_dom);
    if (!type_dom) return JS_NewString(ctx, "");
    JSValue res = JS_NewStringLen(ctx, (const char *)dom_string_data(type_dom), dom_string_byte_length(type_dom));
    dom_string_unref(type_dom); return res;
}

JSValue wisp_event_target_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    dom_event_target *target = NULL; dom_event_get_target(priv->node, &target);
    if (target) { JSValue val = qjs_wrap_node(ctx, (dom_node *)target); dom_node_unref((dom_node *)target); return val; }
    return JS_NULL;
}

JSValue wisp_event_currentTarget_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    dom_event_target *target = NULL; dom_event_get_current_target(priv->node, &target);
    if (target) { JSValue val = qjs_wrap_node(ctx, (dom_node *)target); dom_node_unref((dom_node *)target); return val; }
    return JS_NULL;
}

JSValue wisp_event_eventPhase_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    dom_event_flow_phase phase; dom_event_get_event_phase(priv->node, &phase);
    return JS_NewInt32(ctx, phase);
}

JSValue wisp_event_bubbles_get_impl(JSContext *ctx, QJSNodePrivate *priv) { bool res; dom_event_get_bubbles(priv->node, &res); return JS_NewBool(ctx, res); }
JSValue wisp_event_cancelable_get_impl(JSContext *ctx, QJSNodePrivate *priv) { bool res; dom_event_get_cancelable(priv->node, &res); return JS_NewBool(ctx, res); }
JSValue wisp_event_defaultPrevented_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_FALSE; }
JSValue wisp_event_isTrusted_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_TRUE; }
JSValue wisp_event_timeStamp_get_impl(JSContext *ctx, QJSNodePrivate *priv) { uint64_t now; nsu_getmonotonic_ms(&now); return JS_NewFloat64(ctx, (double)now); }

int qjs_init_event(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_event_class_id == 0) JS_NewClassID(rt, &qjs_event_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_event_class_id)) JS_NewClass(rt, qjs_event_class_id, &wisp_event_class);
    qjs_init_event_gen(ctx);
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue proto = JS_GetClassProto(ctx, qjs_event_class_id);
    JSValue ctor = JS_NewCFunction2(ctx, js_event_constructor, "Event", 1, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, ctor, proto);
    JS_SetPropertyStr(ctx, global_obj, "Event", ctor);
    JS_FreeValue(ctx, proto); JS_FreeValue(ctx, global_obj);
    return 0;
}
