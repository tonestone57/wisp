/*
 * Copyright 2024 Neosurf Contributors
 *
 * This file is part of NeoSurf.
 *
 * NeoSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <wisp/utils/errors.h>
#include <wisp/utils/log.h>

#include "utils/libdom.h"

#include "quickjs.h"
#include "utils/hashmap.h"

#include "content/handlers/javascript/js.h"
#include "qjs_internal.h"
#include "wisp_subsystem.h"
#include "crypto.h"
#include "dom_bridge.h"
#include <nsutils/time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#endif

void *qjs_get_window_priv(JSContext *ctx)
{
    struct jsthread *t = (struct jsthread *)JS_GetContextOpaque(ctx);
    return t ? t->win_priv : NULL;
}

void *qjs_get_document_priv(JSContext *ctx)
{
    struct jsthread *t = (struct jsthread *)JS_GetContextOpaque(ctx);
    return t ? t->doc_priv : NULL;
}

void js_initialise(void)
{
    init_wisp_subsystem(64);
    NSLOG(wisp, INFO, "QuickJS-ng JavaScript engine initialised");
}

void js_finalise(void)
{
    shutdown_wisp_subsystem();
    NSLOG(wisp, INFO, "QuickJS-ng JavaScript engine finalised");
}

nserror js_newheap(int timeout, jsheap **heap)
{
    jsheap *h = (jsheap *)calloc(1, sizeof(*h));
    if (!h) return NSERROR_NOMEM;

    h->rt = JS_NewRuntime();
    if (!h->rt) {
        free(h);
        return NSERROR_NOMEM;
    }

    h->timeout = timeout;
    JS_SetMemoryLimit(h->rt, 64 * 1024 * 1024);
    JS_SetMaxStackSize(h->rt, 1024 * 1024);

    *heap = h;
    return NSERROR_OK;
}

void js_destroyheap(jsheap *heap)
{
    if (!heap) return;
    if (heap->rt) {
        qjs_bridge_cleanup(heap->rt);
        JS_FreeRuntime(heap->rt);
    }
    free(heap);
}

nserror js_newthread(jsheap *heap, void *win_priv, void *doc_priv, jsthread **thread)
{
    if (!heap) return NSERROR_BAD_PARAMETER;

    jsthread *t = (jsthread *)calloc(1, sizeof(*t));
    if (!t) return NSERROR_NOMEM;

    t->ctx = JS_NewContext(heap->rt);
    if (!t->ctx) {
        free(t);
        return NSERROR_NOMEM;
    }

    t->heap = heap;
    t->win_priv = win_priv;
    t->doc_priv = doc_priv;
    t->closed = false;

    JS_SetContextOpaque(t->ctx, t);

    /* Initialize Window opaque for the global object */
    t->global_window_priv.magic = QJS_DOM_MAGIC;
    t->global_window_priv.node = win_priv;
    t->global_window_priv.ctx = t->ctx;
    t->global_window_priv.is_dom_node = false;

    JSValue global_obj = JS_GetGlobalObject(t->ctx);
    JS_SetOpaque(global_obj, &t->global_window_priv);

    qjs_init_dom_bridge(t->ctx);

    /* Register all bindings */
    qjs_init_eventtarget(t->ctx);
    qjs_init_node(t->ctx);
    qjs_init_element(t->ctx);
    qjs_init_document(t->ctx);
    qjs_init_window(t->ctx);

    wisp_js_register_all_bindings(t->ctx);

    qjs_init_console(t->ctx);
    qjs_init_navigator(t->ctx);
    qjs_init_location(t->ctx);
    qjs_init_timers(t->ctx);
    qjs_init_crypto(t->ctx);
    qjs_init_storage(t->ctx);
    qjs_init_xhr(t->ctx);
    qjs_init_mutationobserver(t->ctx);
    qjs_init_intersectionobserver(t->ctx);
    qjs_init_domrectreadonly(t->ctx);
    qjs_init_domrect(t->ctx);

    /* Link global object prototype */
    JSValue window_proto = JS_GetClassProto(t->ctx, qjs_window_class_id);
    if (JS_IsObject(window_proto)) {
        JS_SetPrototype(t->ctx, global_obj, window_proto);
    }
    JS_FreeValue(t->ctx, window_proto);

    /* Set mandatory global properties */
    JS_DefinePropertyValueStr(t->ctx, global_obj, "window", JS_DupValue(t->ctx, global_obj), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(t->ctx, global_obj, "self", JS_DupValue(t->ctx, global_obj), JS_PROP_C_W_E);

    if (doc_priv) {
        JSValue doc_val = qjs_wrap_node(t->ctx, (dom_node *)doc_priv);
        JS_DefinePropertyValueStr(t->ctx, global_obj, "document", doc_val, JS_PROP_C_W_E);
    }

    JS_FreeValue(t->ctx, global_obj);
    NSLOG(wisp, DEBUG, "Created QuickJS thread %p in heap %p", t, heap);

    *thread = t;
    return NSERROR_OK;
}

nserror js_closethread(jsthread *thread)
{
    if (thread) thread->closed = true;
    return NSERROR_OK;
}

void js_destroythread(jsthread *thread)
{
    if (!thread) return;
    if (thread->ctx) {
        qjs_finalise_dom_bridge(thread->ctx);
        JS_FreeContext(thread->ctx);
    }
    free(thread);
}

bool js_exec(jsthread *thread, const uint8_t *txt, size_t txtlen, const char *name)
{
    if (!thread || !thread->ctx || thread->closed) return false;

    char *code = (char *)malloc(txtlen + 1);
    if (!code) return false;
    memcpy(code, txt, txtlen);
    code[txtlen] = '\0';

    JSValue res = JS_Eval(thread->ctx, code, txtlen, name ? name : "test", JS_EVAL_TYPE_GLOBAL);
    free(code);

    if (JS_IsException(res)) {
        JSValue exc = JS_GetException(thread->ctx);
        const char *str = JS_ToCString(thread->ctx, exc);
        if (str) {
            fprintf(stderr, "JS Error [%s]: %s\n", name ? name : "test", str);
            JS_FreeCString(thread->ctx, str);
        }
        JS_FreeValue(thread->ctx, exc);
        JS_FreeValue(thread->ctx, res);
        return false;
    }
    JS_FreeValue(thread->ctx, res);
    return true;
}

static void qjs_event_handler(struct dom_event *evt, void *pw)
{
    struct qjs_event_listener_ctx *ctx = (struct qjs_event_listener_ctx *)pw;
    if (!ctx || !ctx->thread || ctx->thread->closed) return;

    JSContext *jsctx = ctx->thread->ctx;
    JSValue js_evt = JS_NewObject(jsctx);
    dom_string *type_str = NULL;
    dom_event_get_type(evt, &type_str);
    if (type_str) {
        JS_SetPropertyStr(jsctx, js_evt, "type",
            JS_NewStringLen(jsctx, (const char *)dom_string_data(type_str), dom_string_byte_length(type_str)));
        dom_string_unref(type_str);
    }

    JSValue global = JS_GetGlobalObject(jsctx);
    JSValue ret = JS_Call(jsctx, ctx->func, global, 1, &js_evt);
    JS_FreeValue(jsctx, global);
    JS_FreeValue(jsctx, js_evt);
    JS_FreeValue(jsctx, ret);
}

bool js_fire_event(jsthread *thread, const char *type, struct dom_document *doc, struct dom_node *target)
{
    dom_string *type_str;
    dom_event *evt;
    bool success = false;

    if (dom_string_create((const uint8_t *)type, strlen(type), &type_str) != DOM_NO_ERR) return false;
    if (dom_event_create(&evt) == DOM_NO_ERR) {
        dom_event_init(evt, type_str, false, false);
        dom_event_target_dispatch_event((dom_event_target *)(target ? target : (dom_node *)doc), evt, &success);
        dom_event_unref(evt);
    }
    dom_string_unref(type_str);
    return success;
}

bool js_dom_event_add_listener(jsthread *thread, struct dom_document *document, struct dom_node *node,
    struct dom_string *event_type_dom, JSValue js_funcval)
{
    struct qjs_event_listener_ctx *ctx = (struct qjs_event_listener_ctx *)calloc(1, sizeof(*ctx));
    if (!ctx) return false;

    ctx->thread = thread;
    ctx->func = JS_DupValue(thread->ctx, js_funcval);
    ctx->target = (struct dom_event_target *)node;
    ctx->type = event_type_dom;
    dom_node_ref(node);
    dom_string_ref(event_type_dom);

    dom_event_listener *listener;
    if (dom_event_listener_create(qjs_event_handler, ctx, &listener) != DOM_NO_ERR) {
        free(ctx);
        return false;
    }
    ctx->listener = listener;
    dom_event_target_add_event_listener(ctx->target, ctx->type, listener, false);

    ctx->next = thread->listeners;
    thread->listeners = ctx;
    dom_event_listener_unref(listener);
    return true;
}

bool js_dom_event_remove_listener(jsthread *thread, struct dom_document *document, struct dom_node *node,
    struct dom_string *event_type_dom, JSValue js_funcval)
{
    struct qjs_event_listener_ctx **prev = &thread->listeners;
    struct qjs_event_listener_ctx *curr = thread->listeners;

    while (curr) {
        if (curr->target == (struct dom_event_target *)node &&
            dom_string_isequal(curr->type, event_type_dom) &&
            JS_IsSameValue(thread->ctx, curr->func, js_funcval)) {

            dom_event_target_remove_event_listener(curr->target, curr->type, curr->listener, false);
            dom_node_unref((struct dom_node *)curr->target);
            dom_string_unref(curr->type);
            JS_FreeValue(thread->ctx, curr->func);

            *prev = curr->next;
            free(curr);
            return true;
        }
        prev = &curr->next;
        curr = curr->next;
    }
    return false;
}

void js_handle_new_element(jsthread *thread, struct dom_element *node)
{
}

void js_event_cleanup(jsthread *thread, struct dom_event *evt)
{
}
