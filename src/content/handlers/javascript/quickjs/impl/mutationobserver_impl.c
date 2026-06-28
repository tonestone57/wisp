#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSMutationObserver.gen.h"
#include "observer_internal.h"

JSValue qjs_new_mutationrecord_manual(JSContext *ctx, WispMutationRecord *record);

static char *dom_string_to_c(struct dom_string *s)
{
    if (!s) return NULL;
    size_t len = dom_string_byte_length(s);
    char *res = malloc(len + 1);
    if (res) {
        memcpy(res, dom_string_data(s), len);
        res[len] = '\0';
    }
    return res;
}

static JSValue mutation_callback_job(JSContext *ctx, int argc, JSValueConst *argv)
{
    struct QJSNodePrivate *priv = qjs_get_dom_priv(ctx, argv[0]);
    if (!priv) return JS_UNDEFINED;
    WispMutationObserver *observer = priv->node;
    if (JS_IsUndefined(observer->queue)) return JS_UNDEFINED;
    JSValue args[2]; args[0] = observer->queue; args[1] = argv[0];
    JSValue ret = JS_Call(ctx, observer->callback, JS_UNDEFINED, 2, args);
    JS_FreeValue(ctx, ret); JS_FreeValue(ctx, observer->queue);
    observer->queue = JS_NewArray(ctx); observer->queued = false;
    return JS_UNDEFINED;
}

static void mutation_hook(dom_mutation_hook_category category, struct dom_node *node, struct dom_node *related, struct dom_string *prev_value, struct dom_string *new_value, struct dom_string *attr_name, struct dom_string *attr_ns, void *ctx)
{
    struct jsthread *t = ctx;
    if (!t) return;
    WispMutationObserver *observer = (WispMutationObserver *)t->mutation_observers;
    while (observer) {
        MutationObserverTarget *ot = observer->targets;
        while (ot) {
            bool matches_target = (ot->node == node);
            if (!matches_target && ot->subtree) {
                dom_node_contains(ot->node, node, &matches_target);
            }

            bool interested = false;
            if (matches_target) {
                if (category == DOM_MUTATION_HOOK_CHILD_LIST && ot->childList) interested = true;
                else if (category == DOM_MUTATION_HOOK_ATTRIBUTES && ot->attributes) interested = true;
                else if (category == DOM_MUTATION_HOOK_CHARACTER_DATA && ot->characterData) interested = true;
            }
            if (interested) {
                WispMutationRecord *record = calloc(1, sizeof(WispMutationRecord));
                if (category == DOM_MUTATION_HOOK_CHILD_LIST) {
                    record->type = strdup("childList");
                    /* For CHILD_LIST, related is the added/removed node */
                    /* LibDOM currently doesn't specify if it's add or remove in the hook,
                       assuming ADD for now as per PR 174's simplistic approach.
                       In a real engine we'd need more info from LibDOM. */
                    record->numAddedNodes = 1;
                    record->addedNodes = malloc(sizeof(struct dom_node *));
                    record->addedNodes[0] = related; dom_node_ref(related);
                } else if (category == DOM_MUTATION_HOOK_ATTRIBUTES) {
                    record->type = strdup("attributes");
                    if (attr_name) record->attributeName = dom_string_to_c(attr_name);
                    if (attr_ns) record->attributeNamespace = dom_string_to_c(attr_ns);
                    if (ot->attributeOldValue && prev_value) record->oldValue = dom_string_to_c(prev_value);
                } else if (category == DOM_MUTATION_HOOK_CHARACTER_DATA) {
                    record->type = strdup("characterData");
                    if (ot->characterDataOldValue && prev_value) record->oldValue = dom_string_to_c(prev_value);
                }
                record->target = node; dom_node_ref(node);
                if (JS_IsUndefined(observer->queue)) observer->queue = JS_NewArray(observer->ctx);
                uint32_t len = 0; JSValue js_len = JS_GetPropertyStr(observer->ctx, observer->queue, "length");
                JS_ToUint32(observer->ctx, &len, js_len); JS_FreeValue(observer->ctx, js_len);
                JS_SetPropertyUint32(observer->ctx, observer->queue, len, qjs_new_mutationrecord_manual(observer->ctx, record));
                if (!observer->queued) {
                    observer->queued = true;
                    JSValue dup = JS_DupValue(observer->ctx, observer->self);
                    JS_EnqueueJob(observer->ctx, mutation_callback_job, 1, &dup);
                    JS_FreeValue(observer->ctx, dup);
                }
            }
            ot = ot->next;
        }
        observer = observer->next;
    }
}

static void mutationobserver_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_mutationobserver_class_id);
    if (priv) {
        WispMutationObserver *observer = priv->node;
        if (observer) {
            JS_FreeValueRT(rt, observer->callback); JS_FreeValueRT(rt, observer->queue);
            MutationObserverTarget *ot = observer->targets;
            while (ot) {
                MutationObserverTarget *next = ot->next;
                dom_node_unref(ot->node); free(ot); ot = next;
            }
            free(observer);
        }
        free(priv);
    }
}

static JSClassDef wisp_mutationobserver_class = { "MutationObserver", .finalizer = mutationobserver_finalizer };

JSValue wisp_mutationobserver_observe_impl(JSContext *ctx, QJSNodePrivate *priv, void * target, JSValue options)
{
    WispMutationObserver *observer = priv->node;
    MutationObserverTarget *ot = calloc(1, sizeof(MutationObserverTarget));
    ot->node = target; dom_node_ref(target);

    if (JS_IsObject(options)) {
        JSValue val;
        val = JS_GetPropertyStr(ctx, options, "childList");
        ot->childList = JS_ToBool(ctx, val); JS_FreeValue(ctx, val);
        val = JS_GetPropertyStr(ctx, options, "attributes");
        ot->attributes = JS_ToBool(ctx, val); JS_FreeValue(ctx, val);
        val = JS_GetPropertyStr(ctx, options, "characterData");
        ot->characterData = JS_ToBool(ctx, val); JS_FreeValue(ctx, val);
        val = JS_GetPropertyStr(ctx, options, "subtree");
        ot->subtree = JS_ToBool(ctx, val); JS_FreeValue(ctx, val);
        val = JS_GetPropertyStr(ctx, options, "attributeOldValue");
        ot->attributeOldValue = JS_ToBool(ctx, val); JS_FreeValue(ctx, val);
        val = JS_GetPropertyStr(ctx, options, "characterDataOldValue");
        ot->characterDataOldValue = JS_ToBool(ctx, val); JS_FreeValue(ctx, val);
    } else {
        /* Default options if none provided? Spec says it should throw if no options
           but for now let's be lenient or match previous behavior if any.
           Actually, the PR ignored them, so let's at least expect something. */
    }

    ot->next = observer->targets; observer->targets = ot;
    struct jsthread *t = JS_GetContextOpaque(ctx);
    dom_document_set_mutation_hook((struct dom_document *)t->doc_priv, mutation_hook, t);
    return JS_UNDEFINED;
}

JSValue wisp_mutationobserver_disconnect_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    WispMutationObserver *observer = priv->node;
    MutationObserverTarget *ot = observer->targets;
    while (ot) {
        MutationObserverTarget *next = ot->next;
        dom_node_unref(ot->node); free(ot); ot = next;
    }
    observer->targets = NULL;
    return JS_UNDEFINED;
}

JSValue wisp_mutationobserver_takeRecords_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    WispMutationObserver *observer = priv->node;
    JSValue queue = observer->queue; observer->queue = JS_NewArray(ctx);
    return queue;
}

static JSValue js_mutationobserver_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv)
{
    if (argc < 1 || !JS_IsFunction(ctx, argv[0])) return JS_ThrowTypeError(ctx, "Callback required");
    WispMutationObserver *observer = calloc(1, sizeof(WispMutationObserver));
    if (!observer) return JS_ThrowOutOfMemory(ctx);
    observer->callback = JS_DupValue(ctx, argv[0]);
    observer->ctx = ctx; observer->queue = JS_NewArray(ctx);
    JSValue obj = JS_NewObjectClass(ctx, qjs_mutationobserver_class_id);
    if (JS_IsException(obj)) { JS_FreeValue(ctx, observer->callback); JS_FreeValue(ctx, observer->queue); free(observer); return obj; }
    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));
    if (!priv) { JS_FreeValue(ctx, observer->callback); JS_FreeValue(ctx, observer->queue); free(observer); JS_FreeValue(ctx, obj); return JS_ThrowOutOfMemory(ctx); }
    priv->magic = QJS_DOM_MAGIC; priv->node = observer; priv->is_dom_node = false; priv->ctx = ctx;
    JS_SetOpaque(obj, priv); observer->self = obj;
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (t) { observer->next = t->mutation_observers; t->mutation_observers = observer; }
    return obj;
}

int qjs_init_mutationobserver(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_mutationobserver_class_id == 0) JS_NewClassID(rt, &qjs_mutationobserver_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_mutationobserver_class_id)) JS_NewClass(rt, qjs_mutationobserver_class_id, &wisp_mutationobserver_class);
    qjs_init_mutationobserver_gen(ctx);
    JSValue proto = JS_GetClassProto(ctx, qjs_mutationobserver_class_id);
    JSValue ctor = JS_NewCFunction2(ctx, js_mutationobserver_constructor, "MutationObserver", 1, JS_CFUNC_constructor, 0);
    JSValue global_obj = JS_GetGlobalObject(ctx); JS_DefinePropertyValueStr(ctx, global_obj, "MutationObserver", ctor, JS_PROP_C_W_E); JS_FreeValue(ctx, global_obj);
    JS_FreeValue(ctx, proto);
    return 0;
}
