#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSMutationObserver.gen.h"

/*
 * Note: A real MutationObserver implementation requires deep integration
 * with LibDOM's node modification internal hooks. Since LibDOM doesn't
 * provide a public MutationObserver-style API yet, we implement the
 * infrastructure for the JS-side here, which can be backed by LibDOM's
 * MutationEvents or a future internal hook.
 */

typedef struct {
    JSValue callback;
    /* In a real implementation, this would also track observed targets and options */
} WispMutationObserver;

static void mutationobserver_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_mutationobserver_class_id);
    if (priv) {
        WispMutationObserver *observer = priv->node;
        if (observer) {
            JS_FreeValueRT(rt, observer->callback);
            free(observer);
        }
        free(priv);
    }
}

static JSClassDef wisp_mutationobserver_class = {
    "MutationObserver",
    .finalizer = mutationobserver_finalizer,
};

JSValue wisp_mutationobserver_observe_impl(JSContext *ctx, QJSNodePrivate *priv, void * target, void * options)
{
    if (!priv || !priv->node || !target) return JS_EXCEPTION;
    NSLOG(wisp, INFO, "MutationObserver.observe() called (infrastructure only)");
    /*
     * TODO: Register this observer to receive mutation events for 'target'.
     * This requires LibDOM-side support for multiple mutation observers.
     */
    return JS_UNDEFINED;
}

JSValue wisp_mutationobserver_disconnect_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_EXCEPTION;
    NSLOG(wisp, INFO, "MutationObserver.disconnect() called");
    return JS_UNDEFINED;
}

JSValue wisp_mutationobserver_takeRecords_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_EXCEPTION;
    return JS_NewArray(ctx);
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

    JSValue obj = JS_NewObjectClass(ctx, qjs_mutationobserver_class_id);
    if (JS_IsException(obj)) {
        JS_FreeValue(ctx, observer->callback);
        free(observer);
        return obj;
    }

    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));
    if (!priv) {
        JS_FreeValue(ctx, obj);
        JS_FreeValue(ctx, observer->callback);
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
    /* Register the class with our custom finalizer BEFORE the generator does it */
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
