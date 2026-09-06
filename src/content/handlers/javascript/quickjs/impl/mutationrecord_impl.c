#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSMutationRecord.gen.h"
#include "observer_internal.h"

static void free_mutation_record(WispMutationRecord *record);

static void mutationrecord_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_mutationrecord_class_id);
    if (priv) {
        free_mutation_record(priv->node);
        free(priv);
    }
}

static JSClassDef wisp_mutationrecord_class = { "MutationRecord", .finalizer = mutationrecord_finalizer };

extern bool wisp_is_js_process;

static void free_mutation_record(WispMutationRecord *record)
{
    if (!record) return;
    free(record->type);
    if (!wisp_is_js_process) {
        if (record->target) dom_node_unref(record->target);
        for (int i = 0; i < record->numAddedNodes; i++) dom_node_unref(record->addedNodes[i]);
        for (int i = 0; i < record->numRemovedNodes; i++) dom_node_unref(record->removedNodes[i]);
        if (record->previousSibling) dom_node_unref(record->previousSibling);
        if (record->nextSibling) dom_node_unref(record->nextSibling);
    }
    free(record->addedNodes);
    free(record->removedNodes);
    free(record->attributeName);
    free(record->attributeNamespace);
    free(record->oldValue);
    free(record);
}

JSValue qjs_new_mutationrecord_manual(JSContext *ctx, WispMutationRecord *record)
{
    JSValue obj = JS_NewObjectClass(ctx, qjs_mutationrecord_class_id);
    if (JS_IsException(obj)) {
        free_mutation_record(record);
        return obj;
    }
    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));
    if (!priv) {
        JS_FreeValue(ctx, obj);
        free_mutation_record(record);
        return JS_ThrowOutOfMemory(ctx);
    }
    priv->magic = QJS_DOM_MAGIC;
    priv->node = record;
    priv->is_dom_node = false;
    priv->ctx = ctx;
    JS_SetOpaque(obj, priv);
    return obj;
}

JSValue wisp_mutationrecord_type_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, ((WispMutationRecord*)priv->node)->type); }
JSValue wisp_mutationrecord_target_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return qjs_wrap_node(ctx, ((WispMutationRecord*)priv->node)->target); }
JSValue wisp_mutationrecord_addedNodes_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    WispMutationRecord *r = priv->node; JSValue arr = JS_NewArray(ctx);
    for (int i=0; i<r->numAddedNodes; i++) JS_SetPropertyUint32(ctx, arr, i, qjs_wrap_node(ctx, r->addedNodes[i]));
    return arr;
}
JSValue wisp_mutationrecord_removedNodes_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    WispMutationRecord *r = priv->node; JSValue arr = JS_NewArray(ctx);
    for (int i=0; i<r->numRemovedNodes; i++) JS_SetPropertyUint32(ctx, arr, i, qjs_wrap_node(ctx, r->removedNodes[i]));
    return arr;
}
JSValue wisp_mutationrecord_previousSibling_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return qjs_wrap_node(ctx, ((WispMutationRecord*)priv->node)->previousSibling); }
JSValue wisp_mutationrecord_nextSibling_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return qjs_wrap_node(ctx, ((WispMutationRecord*)priv->node)->nextSibling); }
JSValue wisp_mutationrecord_attributeName_get_impl(JSContext *ctx, QJSNodePrivate *priv) { char *n = ((WispMutationRecord*)priv->node)->attributeName; return n ? JS_NewString(ctx, n) : JS_NULL; }
JSValue wisp_mutationrecord_attributeNamespace_get_impl(JSContext *ctx, QJSNodePrivate *priv) { char *n = ((WispMutationRecord*)priv->node)->attributeNamespace; return n ? JS_NewString(ctx, n) : JS_NULL; }
JSValue wisp_mutationrecord_oldValue_get_impl(JSContext *ctx, QJSNodePrivate *priv) { char *n = ((WispMutationRecord*)priv->node)->oldValue; return n ? JS_NewString(ctx, n) : JS_NULL; }

int qjs_init_mutationrecord(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_mutationrecord_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_mutationrecord_class_id == 0) JS_NewClassID(rt, &qjs_mutationrecord_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_mutationrecord_class_id)) JS_NewClass(rt, qjs_mutationrecord_class_id, &wisp_mutationrecord_class);

    qjs_init_mutationrecord_gen(ctx);

    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_mutationrecord_init", JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);
    return 0;
}
