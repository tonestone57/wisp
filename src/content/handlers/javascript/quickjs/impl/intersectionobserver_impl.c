#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSIntersectionObserver.gen.h"
#include "observer_internal.h"

extern bool wisp_is_js_process;

static void intersectionobserver_mark(JSRuntime *rt, JSValueConst val, JS_MarkFunc *mark_func)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_intersectionobserver_class_id);
    if (priv && priv->node) {
        WispIntersectionObserver *observer = priv->node;
        JS_MarkValue(rt, observer->callback, mark_func);
        JS_MarkValue(rt, observer->queue, mark_func);
        JS_MarkValue(rt, observer->self, mark_func);
    }
}

static void intersectionobserver_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_intersectionobserver_class_id);
    if (priv) {
        WispIntersectionObserver *observer = priv->node;
        if (observer) {
            if (JS_ContextIsAlive(rt, priv->ctx)) {
                struct jsthread *t = JS_GetContextOpaque(priv->ctx);
                if (t) {
                    WispIntersectionObserver **curr = &t->intersection_observers;
                    while (*curr) {
                        if (*curr == observer) {
                            *curr = observer->next;
                            break;
                        }
                        curr = &((*curr)->next);
                    }
                }
            }
            JS_FreeValueRT(rt, observer->callback);
            JS_FreeValueRT(rt, observer->queue);
            if (!JS_IsUndefined(observer->self)) JS_FreeValueRT(rt, observer->self);
            IntersectionObserverTarget *ot = observer->targets;
            while (ot) {
                IntersectionObserverTarget *next = ot->next;
                if (!wisp_is_js_process) dom_node_unref(ot->node);
                free(ot); ot = next;
            }
            if (observer->root && !wisp_is_js_process) dom_node_unref(observer->root);
            free(observer->root_margin);
            free(observer->thresholds);
            free(observer);
        }
        free(priv);
    }
}

static JSClassDef wisp_intersectionobserver_class = { "IntersectionObserver", .finalizer = intersectionobserver_finalizer, .gc_mark = intersectionobserver_mark };

JSValue wisp_intersectionobserver_observe_impl(JSContext *ctx, QJSNodePrivate *priv, void * target)
{
    WispIntersectionObserver *observer = priv->node;
    IntersectionObserverTarget *ot = calloc(1, sizeof(IntersectionObserverTarget));
    if (!ot) return JS_ThrowOutOfMemory(ctx);
    ot->node = target;
    if (!wisp_is_js_process) dom_node_ref(target);
    ot->lastRatio = -1.0;
    ot->wasIntersecting = false;
    ot->next = observer->targets; observer->targets = ot;
    return JS_UNDEFINED;
}

JSValue wisp_intersectionobserver_unobserve_impl(JSContext *ctx, QJSNodePrivate *priv, void * target)
{
    WispIntersectionObserver *observer = priv->node;
    IntersectionObserverTarget **curr = &observer->targets;
    while (*curr) {
        if ((*curr)->node == target) {
            IntersectionObserverTarget *to_free = *curr;
            *curr = to_free->next;
            if (!wisp_is_js_process) dom_node_unref(to_free->node);
            free(to_free);
            return JS_UNDEFINED;
        }
        curr = &((*curr)->next);
    }
    return JS_UNDEFINED;
}

JSValue wisp_intersectionobserver_disconnect_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    WispIntersectionObserver *observer = priv->node;
    IntersectionObserverTarget *ot = observer->targets;
    while (ot) {
        IntersectionObserverTarget *next = ot->next;
        if (!wisp_is_js_process) dom_node_unref(ot->node);
        free(ot); ot = next;
    }
    observer->targets = NULL;
    return JS_UNDEFINED;
}

JSValue wisp_intersectionobserver_takeRecords_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    WispIntersectionObserver *observer = priv->node;
    JSValue queue = observer->queue; observer->queue = JS_NewArray(ctx);
    return queue;
}

JSValue wisp_intersectionobserver_root_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    WispIntersectionObserver *observer = priv->node;
    if (observer->root) {
        return qjs_wrap_node(ctx, observer->root);
    }
    return JS_NULL;
}

JSValue wisp_intersectionobserver_rootMargin_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    WispIntersectionObserver *observer = priv->node;
    if (observer->root_margin) {
        return JS_NewString(ctx, observer->root_margin);
    }
    return JS_NewString(ctx, "0px");
}

JSValue wisp_intersectionobserver_thresholds_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    WispIntersectionObserver *observer = priv->node;
    JSValue arr = JS_NewArray(ctx);
    if (JS_IsException(arr)) return arr;
    for (int i = 0; i < observer->num_thresholds; i++) {
        JS_SetPropertyUint32(ctx, arr, i, JS_NewFloat64(ctx, observer->thresholds[i]));
    }
    return arr;
}

JSValue wisp_intersectionobserver_constructor_impl(JSContext *ctx, JSValue callback, JSValue options)
{
    if (!JS_IsFunction(ctx, callback)) return JS_ThrowTypeError(ctx, "Callback required");

    struct dom_node *root_node = NULL;
    char *root_margin_str = NULL;
    double *thresholds = NULL;
    int num_thresholds = 0;

    if (JS_IsObject(options)) {
        JSValue js_root = JS_GetPropertyStr(ctx, options, "root");
        if (!JS_IsUndefined(js_root) && !JS_IsNull(js_root)) {
            QJSNodePrivate *root_priv = qjs_get_dom_priv(ctx, js_root);
            if (root_priv) {
                root_node = root_priv->node;
            }
        }
        JS_FreeValue(ctx, js_root);

        JSValue js_margin = JS_GetPropertyStr(ctx, options, "rootMargin");
        if (JS_IsString(js_margin)) {
            const char *margin_cstr = JS_ToCString(ctx, js_margin);
            if (margin_cstr) {
                root_margin_str = strdup(margin_cstr);
                JS_FreeCString(ctx, margin_cstr);
                if (!root_margin_str) {
                    JS_FreeValue(ctx, js_margin);
                    return JS_ThrowOutOfMemory(ctx);
                }
            }
        }
        JS_FreeValue(ctx, js_margin);

        JSValue js_threshold = JS_GetPropertyStr(ctx, options, "threshold");
        if (JS_IsArray(js_threshold)) {
            JSValue js_len = JS_GetPropertyStr(ctx, js_threshold, "length");
            uint32_t len = 0;
            JS_ToUint32(ctx, &len, js_len);
            JS_FreeValue(ctx, js_len);
            if (len > 0) {
                thresholds = calloc(len, sizeof(double));
                if (!thresholds) {
                    free(root_margin_str);
                    JS_FreeValue(ctx, js_threshold);
                    return JS_ThrowOutOfMemory(ctx);
                }
                num_thresholds = len;
                for (uint32_t i = 0; i < len; i++) {
                    JSValue val = JS_GetPropertyUint32(ctx, js_threshold, i);
                    double d = 0.0;
                    JS_ToFloat64(ctx, &d, val);
                    JS_FreeValue(ctx, val);
                    if (d < 0.0 || d > 1.0) {
                        free(thresholds);
                        free(root_margin_str);
                        JS_FreeValue(ctx, js_threshold);
                        return JS_ThrowRangeError(ctx, "Threshold bounds must be between 0 and 1");
                    }
                    thresholds[i] = d;
                }
                // Sort thresholds in ascending order according to spec
                for (int i = 0; i < num_thresholds - 1; i++) {
                    for (int j = i + 1; j < num_thresholds; j++) {
                        if (thresholds[i] > thresholds[j]) {
                            double temp = thresholds[i];
                            thresholds[i] = thresholds[j];
                            thresholds[j] = temp;
                        }
                    }
                }
            }
        } else if (!JS_IsUndefined(js_threshold) && !JS_IsNull(js_threshold)) {
            double d = 0.0;
            JS_ToFloat64(ctx, &d, js_threshold);
            if (d < 0.0 || d > 1.0) {
                free(root_margin_str);
                JS_FreeValue(ctx, js_threshold);
                return JS_ThrowRangeError(ctx, "Threshold bounds must be between 0 and 1");
            }
            thresholds = malloc(sizeof(double));
            if (!thresholds) {
                free(root_margin_str);
                JS_FreeValue(ctx, js_threshold);
                return JS_ThrowOutOfMemory(ctx);
            }
            thresholds[0] = d;
            num_thresholds = 1;
        }
        JS_FreeValue(ctx, js_threshold);
    }

    if (!root_margin_str) root_margin_str = strdup("0px");
    if (!root_margin_str) {
        free(thresholds);
        return JS_ThrowOutOfMemory(ctx);
    }
    if (!thresholds) {
        thresholds = calloc(1, sizeof(double));
        if (!thresholds) {
            free(root_margin_str);
            return JS_ThrowOutOfMemory(ctx);
        }
        thresholds[0] = 0.0;
        num_thresholds = 1;
    }

    WispIntersectionObserver *observer = calloc(1, sizeof(WispIntersectionObserver));
    if (!observer) {
        free(root_margin_str);
        free(thresholds);
        return JS_ThrowOutOfMemory(ctx);
    }
    observer->callback = JS_DupValue(ctx, callback);
    observer->ctx = ctx;
    observer->queue = JS_NewArray(ctx);
    observer->root = root_node;
    if (root_node && !wisp_is_js_process) dom_node_ref(root_node);
    observer->root_margin = root_margin_str;
    observer->thresholds = thresholds;
    observer->num_thresholds = num_thresholds;

    JSValue obj = JS_NewObjectClass(ctx, qjs_intersectionobserver_class_id);
    if (JS_IsException(obj)) {
        if (root_node && !wisp_is_js_process) dom_node_unref(root_node);
        free(root_margin_str);
        free(thresholds);
        JS_FreeValue(ctx, observer->callback);
        JS_FreeValue(ctx, observer->queue);
        free(observer);
        return obj;
    }
    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));
    if (!priv) {
        if (root_node && !wisp_is_js_process) dom_node_unref(root_node);
        free(root_margin_str);
        free(thresholds);
        JS_FreeValue(ctx, observer->callback);
        JS_FreeValue(ctx, observer->queue);
        free(observer);
        JS_FreeValue(ctx, obj);
        return JS_ThrowOutOfMemory(ctx);
    }
    priv->magic = QJS_DOM_MAGIC; priv->node = observer; priv->is_dom_node = false; priv->ctx = ctx;
    JS_SetOpaque(obj, priv);
    observer->self = JS_DupValue(ctx, obj);
    observer->magic = QJS_DOM_MAGIC;
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (t) { observer->next = t->intersection_observers; t->intersection_observers = observer; }
    return obj;
}

int qjs_init_intersectionobserver(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);

    /* Check if already initialized on this global object */
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_intersectionobserver_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_intersectionobserver_class_id == 0) JS_NewClassID(rt, &qjs_intersectionobserver_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_intersectionobserver_class_id)) JS_NewClass(rt, qjs_intersectionobserver_class_id, &wisp_intersectionobserver_class);

    /* Initialize the class and prototype using the generated function */
    qjs_init_intersectionobserver_gen(ctx);

    /* Mark as initialized */
    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_intersectionobserver_init", JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);

    return 0;
}
