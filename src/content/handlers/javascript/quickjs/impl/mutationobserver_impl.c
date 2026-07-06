#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include <dom/core/mutation_observer.h>
#include "JSMutationObserver.gen.h"
#include "observer_internal.h"

JSValue qjs_new_mutationrecord_manual(JSContext *ctx, WispMutationRecord *record);

static char *dom_string_to_c(struct dom_string *s)
{
    if (!s) return NULL;
    const char *data = dom_string_data(s);
    size_t len = dom_string_byte_length(s);
    if (!data) return strdup("");
    char *res = malloc(len + 1);
    if (res) {
        memcpy(res, data, len);
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

    /* Swap queue before callback to handle nested mutations correctly */
    JSValue queue = observer->queue;
    observer->queue = JS_NewArray(ctx);
    observer->queued = false;

    JSValue args[2];
    args[0] = queue;
    args[1] = argv[0];

    JSValue ret = JS_Call(ctx, observer->callback, JS_UNDEFINED, 2, args);
    JS_FreeValue(ctx, ret);
    JS_FreeValue(ctx, queue);

    return JS_UNDEFINED;
}

static void mutation_callback(const struct dom_mutation_notification *notification, void *pw)
{
    struct jsthread *t = pw;
    if (!t) return;
    WispMutationObserver *observer = (WispMutationObserver *)t->mutation_observers;
    while (observer) {
        MutationObserverTarget *ot = observer->targets;
        while (ot) {
            bool matches_target = (ot->node == notification->target);
            if (!matches_target && ot->subtree) {
                dom_node_contains(ot->node, notification->target, &matches_target);
            }

            bool interested = false;
            if (matches_target) {
                if (notification->type == DOM_MUTATION_NOTIFICATION_CHILD_LIST && ot->childList) interested = true;
                else if (notification->type == DOM_MUTATION_NOTIFICATION_ATTRIBUTES && ot->attributes) interested = true;
                else if (notification->type == DOM_MUTATION_NOTIFICATION_CHARACTER_DATA && ot->characterData) interested = true;
            }
            if (interested) {
                WispMutationRecord *record = calloc(1, sizeof(WispMutationRecord));
                if (notification->type == DOM_MUTATION_NOTIFICATION_CHILD_LIST) {
                    record->type = strdup("childList");
                    if (notification->added_node) {
                        record->numAddedNodes = 1;
                        record->addedNodes = malloc(sizeof(struct dom_node *));
                        record->addedNodes[0] = notification->added_node; dom_node_ref(notification->added_node);
                    } else if (notification->removed_node) {
                        record->numRemovedNodes = 1;
                        record->removedNodes = malloc(sizeof(struct dom_node *));
                        record->removedNodes[0] = notification->removed_node; dom_node_ref(notification->removed_node);
                    }
                    if (notification->previous_sibling) {
                        record->previousSibling = notification->previous_sibling;
                        dom_node_ref(notification->previous_sibling);
                    }
                    if (notification->next_sibling) {
                        record->nextSibling = notification->next_sibling;
                        dom_node_ref(notification->next_sibling);
                    }
                } else if (notification->type == DOM_MUTATION_NOTIFICATION_ATTRIBUTES) {
                    record->type = strdup("attributes");
                    if (notification->attr_name) record->attributeName = dom_string_to_c(notification->attr_name);
                    if (notification->attr_namespace) record->attributeNamespace = dom_string_to_c(notification->attr_namespace);
                    if (ot->attributeOldValue && notification->old_value) record->oldValue = dom_string_to_c(notification->old_value);
                } else if (notification->type == DOM_MUTATION_NOTIFICATION_CHARACTER_DATA) {
                    record->type = strdup("characterData");
                    if (ot->characterDataOldValue && notification->old_value) record->oldValue = dom_string_to_c(notification->old_value);
                }
                record->target = notification->target; dom_node_ref(notification->target);
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


static void mutationobserver_mark(JSRuntime *rt, JSValueConst val, JS_MarkFunc *mark_func)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_mutationobserver_class_id);
    if (priv && priv->node) {
        WispMutationObserver *observer = priv->node;
        JS_MarkValue(rt, observer->callback, mark_func);
        JS_MarkValue(rt, observer->queue, mark_func);
        JS_MarkValue(rt, observer->self, mark_func);
    }
}

static void mutationobserver_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_mutationobserver_class_id);
    if (priv) {
        WispMutationObserver *observer = priv->node;
        if (observer) {
            struct jsthread *t = JS_GetContextOpaque(priv->ctx);
            if (t) {
                WispMutationObserver **curr = &t->mutation_observers;
                while (*curr) {
                    if (*curr == observer) {
                        *curr = observer->next;
                        break;
                    }
                    curr = &((*curr)->next);
                }
            }
            JS_FreeValueRT(rt, observer->callback);
            JS_FreeValueRT(rt, observer->queue);
            if (!JS_IsUndefined(observer->self)) JS_FreeValueRT(rt, observer->self);
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

static JSClassDef wisp_mutationobserver_class = { "MutationObserver", .finalizer = mutationobserver_finalizer, .gc_mark = mutationobserver_mark };

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
        free(ot); dom_node_unref(target);
        return JS_ThrowTypeError(ctx, "MutationObserver.observe: options must be an object");
    }

    if (!ot->childList && !ot->attributes && !ot->characterData) {
        free(ot); dom_node_unref(target);
        return JS_ThrowTypeError(ctx, "MutationObserver.observe: at least one of childList, attributes, or characterData must be true");
    }

    ot->next = observer->targets; observer->targets = ot;
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (t->mutation_callback_registered_doc != t->doc_priv) {
        dom_document_add_mutation_callback((struct dom_document *)t->doc_priv, mutation_callback, t);
        t->mutation_callback_registered_doc = t->doc_priv;
    }
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
    JS_SetOpaque(obj, priv);
    observer->self = JS_DupValue(ctx, obj);
    observer->magic = QJS_DOM_MAGIC;
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (t) { observer->next = t->mutation_observers; t->mutation_observers = observer; }
    return obj;
}

int qjs_init_mutationobserver(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);

    /* Check if already initialized on this global object */
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_mutationobserver_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_mutationobserver_class_id == 0) JS_NewClassID(rt, &qjs_mutationobserver_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_mutationobserver_class_id)) JS_NewClass(rt, qjs_mutationobserver_class_id, &wisp_mutationobserver_class);

    /* Initialize the class and prototype using the generated function */
    qjs_init_mutationobserver_gen(ctx);

    JSValue proto = JS_GetClassProto(ctx, qjs_mutationobserver_class_id);
    if (!JS_IsObject(proto)) {
        JS_FreeValue(ctx, proto);
        proto = JS_NewObject(ctx);
        JS_SetClassProto(ctx, qjs_mutationobserver_class_id, JS_DupValue(ctx, proto));
    }

    JSValue ctor = JS_NewCFunction2(ctx, js_mutationobserver_constructor, "MutationObserver", 1, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, ctor, proto);
    JS_FreeValue(ctx, proto);
    JS_DefinePropertyValueStr(ctx, global_obj, "MutationObserver", ctor, JS_PROP_C_W_E);

    /* Mark as initialized */
    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_mutationobserver_init", JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);

    return 0;
}
