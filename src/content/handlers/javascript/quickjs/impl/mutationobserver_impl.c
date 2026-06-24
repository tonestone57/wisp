#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSMutationObserver.gen.h"
#include "JSMutationRecord.gen.h"
#include <dom/core/mutation_observer.h>

typedef struct {
    bool childList;
    bool attributes;
    bool characterData;
    bool subtree;
    bool attributeOldValue;
    bool characterDataOldValue;
    /* attributeFilter not implemented yet */
} MutationObserverOptions;

typedef struct {
    struct dom_node *node;
    MutationObserverOptions options;
} WispObservedTarget;

typedef struct {
    JSValue callback;
    JSValue observer_val; /* Self reference for microtasks */
    struct jsthread *thread;

    WispObservedTarget *targets;
    uint32_t target_count;

    struct dom_document **docs;
    uint32_t doc_count;

    JSValue records;
    bool microtask_scheduled;
} WispMutationObserver;

static void libdom_mutation_callback(const struct dom_mutation_notification *notification, void *pw);

static void mutationobserver_cleanup_targets(WispMutationObserver *observer)
{
    for (uint32_t i = 0; i < observer->target_count; i++) {
        dom_node_unref(observer->targets[i].node);
    }
    free(observer->targets);
    observer->targets = NULL;
    observer->target_count = 0;

    for (uint32_t i = 0; i < observer->doc_count; i++) {
        dom_document_remove_mutation_callback(observer->docs[i], libdom_mutation_callback, observer);
        dom_node_unref(observer->docs[i]);
    }
    free(observer->docs);
    observer->docs = NULL;
    observer->doc_count = 0;
}

static void mutationobserver_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_mutationobserver_class_id);
    if (priv) {
        WispMutationObserver *observer = priv->node;
        if (observer) {
            mutationobserver_cleanup_targets(observer);
            JS_FreeValueRT(rt, observer->callback);
            JS_FreeValueRT(rt, observer->records);
            /* observer_val is managed by GC cycle detection */
            free(observer);
        }
        free(priv);
    }
}

static void mutationobserver_gc_mark(JSRuntime *rt, JSValueConst val, JS_MarkFunc *mark_func)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_mutationobserver_class_id);
    if (priv) {
        WispMutationObserver *observer = priv->node;
        if (observer) {
            JS_MarkValue(rt, observer->callback, mark_func);
            JS_MarkValue(rt, observer->records, mark_func);
            JS_MarkValue(rt, observer->observer_val, mark_func);
        }
    }
}

static JSClassDef wisp_mutationobserver_class = {
    "MutationObserver",
    .finalizer = mutationobserver_finalizer,
    .gc_mark = mutationobserver_gc_mark,
};

static JSValue mutation_observer_microtask(JSContext *ctx, int argc, JSValueConst *argv)
{
    WispMutationObserver *observer = JS_GetOpaque(argv[0], qjs_mutationobserver_class_id);
    if (!observer) return JS_UNDEFINED;

    observer->microtask_scheduled = false;

    JSValue records = observer->records;
    observer->records = JS_NewArray(ctx);

    JSValueConst args[2];
    args[0] = records;
    args[1] = argv[0]; // The observer itself

    JSValue ret = JS_Call(ctx, observer->callback, JS_UNDEFINED, 2, args);
    JS_FreeValue(ctx, ret);
    JS_FreeValue(ctx, records);

    return JS_UNDEFINED;
}

static JSValue dom_string_to_js(JSContext *ctx, struct dom_string *s)
{
    if (!s) return JS_NULL;
    return JS_NewStringLen(ctx, (const char *)dom_string_data(s), dom_string_byte_length(s));
}

static void libdom_mutation_callback(const struct dom_mutation_notification *notification, void *pw)
{
    WispMutationObserver *observer = pw;
    JSContext *ctx = observer->thread->ctx;

    /* Find if any observed target matches this mutation */
    MutationObserverOptions *matched_options = NULL;
    for (uint32_t i = 0; i < observer->target_count; i++) {
        WispObservedTarget *target = &observer->targets[i];
        bool match = false;
        if (target->options.subtree) {
            bool contains;
            dom_node_contains(target->node, notification->target, &contains);
            if (contains) match = true;
        } else {
            if (notification->target == target->node) match = true;
        }

        if (match) {
            /* Further filter by type */
            if (notification->type == DOM_MUTATION_NOTIFICATION_CHILD_LIST && target->options.childList) {
                matched_options = &target->options; break;
            }
            if (notification->type == DOM_MUTATION_NOTIFICATION_ATTRIBUTES && target->options.attributes) {
                matched_options = &target->options; break;
            }
            if (notification->type == DOM_MUTATION_NOTIFICATION_CHARACTER_DATA && target->options.characterData) {
                matched_options = &target->options; break;
            }
        }
    }

    if (!matched_options) return;

    JSValue record = JS_NewObjectClass(ctx, qjs_mutationrecord_class_id);

    JS_SetPropertyStr(ctx, record, "type",
        JS_NewString(ctx, notification->type == DOM_MUTATION_NOTIFICATION_CHILD_LIST ? "childList" :
                         notification->type == DOM_MUTATION_NOTIFICATION_ATTRIBUTES ? "attributes" : "characterData"));

    JS_SetPropertyStr(ctx, record, "target", qjs_wrap_node(ctx, notification->target));

    if (notification->type == DOM_MUTATION_NOTIFICATION_CHILD_LIST) {
        JSValue added = JS_NewArray(ctx);
        if (notification->added_node) {
            JS_SetPropertyUint32(ctx, added, 0, qjs_wrap_node(ctx, notification->added_node));
        }
        JS_SetPropertyStr(ctx, record, "addedNodes", added);

        JSValue removed = JS_NewArray(ctx);
        if (notification->removed_node) {
            JS_SetPropertyUint32(ctx, removed, 0, qjs_wrap_node(ctx, notification->removed_node));
        }
        JS_SetPropertyStr(ctx, record, "removedNodes", removed);

        if (notification->previous_sibling)
            JS_SetPropertyStr(ctx, record, "previousSibling", qjs_wrap_node(ctx, notification->previous_sibling));
        if (notification->next_sibling)
            JS_SetPropertyStr(ctx, record, "nextSibling", qjs_wrap_node(ctx, notification->next_sibling));
    }

    if (notification->attr_name) {
        JS_SetPropertyStr(ctx, record, "attributeName", dom_string_to_js(ctx, notification->attr_name));
    }

    if (notification->attr_namespace) {
        JS_SetPropertyStr(ctx, record, "attributeNamespace", dom_string_to_js(ctx, notification->attr_namespace));
    }

    if ((notification->type == DOM_MUTATION_NOTIFICATION_ATTRIBUTES && matched_options->attributeOldValue) ||
        (notification->type == DOM_MUTATION_NOTIFICATION_CHARACTER_DATA && matched_options->characterDataOldValue)) {
        if (notification->old_value) {
            JS_SetPropertyStr(ctx, record, "oldValue", dom_string_to_js(ctx, notification->old_value));
        }
    }

    /* Push to records array */
    uint32_t len;
    JSValue len_val = JS_GetPropertyStr(ctx, observer->records, "length");
    JS_ToUint32(ctx, &len, len_val);
    JS_FreeValue(ctx, len_val);
    JS_SetPropertyUint32(ctx, observer->records, len, record);

    if (!observer->microtask_scheduled) {
        observer->microtask_scheduled = true;
        JS_EnqueueJob(ctx, mutation_observer_microtask, 1, &observer->observer_val);
    }
}

JSValue wisp_mutationobserver_observe_impl(JSContext *ctx, QJSNodePrivate *priv, void * target, void * options)
{
    if (!priv || !priv->node || !target) return JS_EXCEPTION;
    WispMutationObserver *observer = priv->node;

    struct dom_node *node = target;
    struct dom_document *doc;
    dom_node_get_owner_document(node, &doc);
    if (!doc) {
        doc = (struct dom_document *)dom_node_ref(node);
    }

    /* Parse options */
    MutationObserverOptions mo_opts = {0};
    JSValue opts = (JSValue)options;
    JSValue val;

    val = JS_GetPropertyStr(ctx, opts, "childList");
    mo_opts.childList = JS_ToBool(ctx, val);
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, opts, "attributes");
    mo_opts.attributes = JS_ToBool(ctx, val);
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, opts, "characterData");
    mo_opts.characterData = JS_ToBool(ctx, val);
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, opts, "subtree");
    mo_opts.subtree = JS_ToBool(ctx, val);
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, opts, "attributeOldValue");
    mo_opts.attributeOldValue = JS_ToBool(ctx, val);
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, opts, "characterDataOldValue");
    mo_opts.characterDataOldValue = JS_ToBool(ctx, val);
    JS_FreeValue(ctx, val);

    /* Update or add target */
    bool found = false;
    for (uint32_t i = 0; i < observer->target_count; i++) {
        if (observer->targets[i].node == node) {
            observer->targets[i].options = mo_opts;
            found = true;
            break;
        }
    }
    if (!found) {
        observer->targets = realloc(observer->targets, (observer->target_count + 1) * sizeof(WispObservedTarget));
        observer->targets[observer->target_count].node = dom_node_ref(node);
        observer->targets[observer->target_count].options = mo_opts;
        observer->target_count++;
    }

    /* Register with document if not already registered */
    bool doc_found = false;
    for (uint32_t i = 0; i < observer->doc_count; i++) {
        if (observer->docs[i] == doc) {
            doc_found = true;
            break;
        }
    }
    if (!doc_found) {
        observer->docs = realloc(observer->docs, (observer->doc_count + 1) * sizeof(struct dom_document *));
        observer->docs[observer->doc_count] = (struct dom_document *)dom_node_ref(doc);
        observer->doc_count++;
        dom_document_add_mutation_callback(doc, libdom_mutation_callback, observer);
    }

    dom_node_unref(doc);

    return JS_UNDEFINED;
}

JSValue wisp_mutationobserver_disconnect_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_EXCEPTION;
    WispMutationObserver *observer = priv->node;
    mutationobserver_cleanup_targets(observer);
    JS_FreeValue(ctx, observer->records);
    observer->records = JS_NewArray(ctx);
    return JS_UNDEFINED;
}

JSValue wisp_mutationobserver_takeRecords_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_EXCEPTION;
    WispMutationObserver *observer = priv->node;
    JSValue records = observer->records;
    observer->records = JS_NewArray(ctx);
    return records;
}

static JSValue js_mutationobserver_constructor(JSContext *ctx, JSValueConst new_target,
                                              int argc, JSValueConst *argv)
{
    if (argc < 1 || !JS_IsFunction(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "MutationObserver constructor requires a callback function");
    }

    WispMutationObserver *observer = calloc(1, sizeof(WispMutationObserver));
    if (!observer) return JS_ThrowOutOfMemory(ctx);

    observer->callback = JS_DupValue(ctx, argv[0]);
    observer->thread = JS_GetContextOpaque(ctx);
    observer->records = JS_NewArray(ctx);

    JSValue obj = JS_NewObjectClass(ctx, qjs_mutationobserver_class_id);
    if (JS_IsException(obj)) {
        JS_FreeValue(ctx, observer->callback);
        JS_FreeValue(ctx, observer->records);
        free(observer);
        return obj;
    }

    observer->observer_val = JS_DupValue(ctx, obj);

    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));
    if (!priv) {
        JS_FreeValue(ctx, obj);
        JS_FreeValue(ctx, observer->callback);
        JS_FreeValue(ctx, observer->records);
        free(observer);
        return JS_ThrowOutOfMemory(ctx);
    }

    priv->magic = QJS_DOM_MAGIC;
    priv->node = observer;
    priv->is_dom_node = false;
    priv->ctx = ctx;

    JS_SetOpaque(obj, priv);
    return obj;
}

int qjs_init_mutationobserver(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_mutationobserver_class_id == 0) JS_NewClassID(rt, &qjs_mutationobserver_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_mutationobserver_class_id)) {
        JS_NewClass(rt, qjs_mutationobserver_class_id, &wisp_mutationobserver_class);
    }

    qjs_init_mutationobserver_gen(ctx);

    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue proto = JS_GetClassProto(ctx, qjs_mutationobserver_class_id);
    JSValue ctor = JS_NewCFunction2(ctx, js_mutationobserver_constructor, "MutationObserver", 1, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, ctor, proto);
    JS_SetPropertyStr(ctx, global_obj, "MutationObserver", ctor);

    JS_FreeValue(ctx, proto);
    JS_FreeValue(ctx, global_obj);
    return 0;
}
