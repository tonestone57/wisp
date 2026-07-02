/*
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * Copyright 2026 Wisp
 *
 * NeoSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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
#include <wisp/content/handlers/html/box_inspect.h>
#include <wisp/content/handlers/html/box.h>
#include <math.h>
#include "impl/observer_internal.h"

#ifdef _WIN32
#include <windows.h>
#endif

void *qjs_get_window_priv(JSContext *ctx)
{
    struct jsthread *t = JS_GetContextOpaque(ctx);
    return t ? t->win_priv : NULL;
}

void *qjs_get_document_priv(JSContext *ctx)
{
    struct jsthread *t = JS_GetContextOpaque(ctx);
    return t ? t->doc_priv : NULL;
}

void js_initialise(void)
{
}

static int qjs_interrupt_handler(JSRuntime *rt, void *opaque)
{
    struct jsheap *heap = opaque;
    uint64_t now;
    if (heap->deadline_ms > 0) {
        nsu_getmonotonic_ms(&now);
        if (now > heap->deadline_ms) return 1;
    }
    return 0;
}

void js_finalise(void)
{
}

nserror js_newheap(int timeout, jsheap **heap)
{
    jsheap *h = calloc(1, sizeof(*h));
    if (!h) return NSERROR_NOMEM;
    h->rt = JS_NewRuntime();
    if (!h->rt) { free(h); return NSERROR_NOMEM; }
    h->timeout = timeout;
    JS_SetMemoryLimit(h->rt, 64 * 1024 * 1024);
    JS_SetMaxStackSize(h->rt, 1024 * 1024);
    JS_SetInterruptHandler(h->rt, qjs_interrupt_handler, h);
    *heap = h;
    return NSERROR_OK;
}

void js_destroyheap(jsheap *heap)
{
    if (!heap) return;
    if (heap->rt) {
        /* Clean up the DOM bridge first while the runtime opaque is still valid.
         * qjs_bridge_cleanup will set the opaque to NULL when finished. */
        qjs_bridge_cleanup(heap->rt);
        JS_RunGC(heap->rt);
        JS_RunGC(heap->rt);
        /* QuickJS-ng: list_empty(&rt->gc_obj_list) assertion fix.
         * Explicitly free GC objects that might be pending after bridge cleanup. */
        JS_SetRuntimeOpaque(heap->rt, NULL);
        JS_FreeRuntime(heap->rt);
    }
    free(heap);
}

nserror js_newthread(jsheap *heap, void *win_priv, void *doc_priv, jsthread **thread)
{
    jsthread *t = calloc(1, sizeof(*t));
    if (!t) return NSERROR_NOMEM;
    t->ctx = JS_NewContext(heap->rt);
    if (!t->ctx) { free(t); return NSERROR_NOMEM; }
    t->heap = heap; t->win_priv = win_priv;
    JS_SetContextOpaque(t->ctx, t);

    /* core initialization - registration handles dependencies */
    wisp_js_register_all_bindings(t->ctx);

    if (qjs_init_dom_bridge(t->ctx) != 0 ||
        qjs_init_eventtarget(t->ctx) != 0 ||
        qjs_init_event(t->ctx) != 0 ||
        qjs_init_node(t->ctx) != 0 ||
        qjs_init_element(t->ctx) != 0 ||
        qjs_init_document(t->ctx) != 0 ||
        qjs_init_window(t->ctx) != 0 ||
        qjs_init_console(t->ctx) != 0 ||
        qjs_init_timers(t->ctx) != 0 ||
        qjs_init_crypto(t->ctx) != 0 ||
        qjs_init_navigator(t->ctx) != 0 ||
        qjs_init_location(t->ctx) != 0 ||
        qjs_init_storage(t->ctx) != 0 ||
        qjs_init_xhr(t->ctx) != 0 ||
        qjs_init_mutationobserver(t->ctx) != 0 ||
        qjs_init_intersectionobserver(t->ctx) != 0) {
        js_destroythread(t);
        return NSERROR_NOMEM;
    }

    JSValue global_obj = JS_GetGlobalObject(t->ctx);
    t->global_window_priv.magic = QJS_DOM_MAGIC;
    t->global_window_priv.node = win_priv;
    t->global_window_priv.ctx = t->ctx;
    t->global_window_priv.is_dom_node = false;

    JSValue window_proto = JS_GetClassProto(t->ctx, qjs_window_class_id);
    if (JS_IsObject(window_proto)) JS_SetPrototype(t->ctx, global_obj, window_proto);
    JS_FreeValue(t->ctx, window_proto);

    JS_DefinePropertyValueStr(t->ctx, global_obj, "window", JS_DupValue(t->ctx, global_obj), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(t->ctx, global_obj, "self", JS_DupValue(t->ctx, global_obj), JS_PROP_C_W_E);
    if (doc_priv) {
        JS_DefinePropertyValueStr(t->ctx, global_obj, "document", qjs_wrap_node(t->ctx, (dom_node *)doc_priv), JS_PROP_C_W_E);
        dom_node_ref((dom_node *)doc_priv);
        t->doc_priv = doc_priv;
    }

    JS_FreeValue(t->ctx, global_obj);
    *thread = t;
    return NSERROR_OK;
}

nserror js_closethread(jsthread *thread) { if (thread) thread->closed = true; return NSERROR_OK; }

void js_destroythread(jsthread *thread)
{
    if (!thread) return;

    if (thread->ctx) {
        JSRuntime *rt = JS_GetRuntime(thread->ctx);
        JSContext *ctx1;
        while (JS_ExecutePendingJob(rt, &ctx1) > 0);
    }

    struct qjs_timer *tim = thread->timers;
    thread->timers = NULL;
    while (tim) {
        struct qjs_timer *next = tim->next;
        JS_FreeValue(thread->ctx, tim->func);
        free(tim);
        tim = next;
    }

    struct qjs_event_listener_ctx *l = thread->listeners;
    thread->listeners = NULL;
    while (l) {
        struct qjs_event_listener_ctx *next = l->next;
        dom_event_target_remove_event_listener(l->target, l->type, l->listener, false);
        dom_node_unref((struct dom_node *)l->target);
        dom_string_unref(l->type);
        JS_FreeValue(thread->ctx, l->func);
        dom_event_listener_unref(l->listener);
        free(l);
        l = next;
    }

    struct qjs_event_map *e = thread->events;
    thread->events = NULL;
    while (e) {
        struct qjs_event_map *next = e->next;
        JS_FreeValue(thread->ctx, e->js_evt);
        dom_event_unref(e->evt);
        free(e);
        e = next;
    }

    /* Break MutationObserver cycles and orphan them */
    struct WispMutationObserver *mo_list = (struct WispMutationObserver *)thread->mutation_observers;
    thread->mutation_observers = NULL;
    while (mo_list) {
        struct WispMutationObserver *mo = mo_list;
        mo_list = mo->next;
        JSValue self = mo->self;
        mo->self = JS_UNDEFINED;
        mo->next = NULL;
        JS_FreeValue(thread->ctx, self);
    }

    /* Break IntersectionObserver cycles and orphan them */
    struct WispIntersectionObserver *io_list = (struct WispIntersectionObserver *)thread->intersection_observers;
    thread->intersection_observers = NULL;
    while (io_list) {
        struct WispIntersectionObserver *io = io_list;
        io_list = io->next;
        JSValue self = io->self;
        io->self = JS_UNDEFINED;
        io->next = NULL;
        JS_FreeValue(thread->ctx, self);
    }

    if (thread->ctx) {
        JSRuntime *rt = JS_GetRuntime(thread->ctx);
        qjs_finalise_dom_bridge(thread->ctx);
        JS_SetContextOpaque(thread->ctx, NULL);
        JS_FreeContext(thread->ctx);

        /* Final GC passes to ensure all orphaned objects (including MutationObservers) are collected. */
        JS_RunGC(rt);
        JS_RunGC(rt);
    }
    if (thread->doc_priv) dom_node_unref((dom_node *)thread->doc_priv);
    free(thread);
}

bool js_exec(jsthread *thread, const uint8_t *txt, size_t txtlen, const char *name)
{
    if (!thread || !thread->ctx || thread->closed) return false;
    char *term_txt = malloc(txtlen + 1);
    if (!term_txt) return false;
    memcpy(term_txt, txt, txtlen); term_txt[txtlen] = '\0';
    JSValue result = JS_Eval(thread->ctx, term_txt, txtlen, name ? name : "<script>", JS_EVAL_TYPE_GLOBAL);
    free(term_txt);

    bool success = !JS_IsException(result);
    if (!success) {
        JSValue exc = JS_GetException(thread->ctx);
        const char *exc_str = JS_ToCString(thread->ctx, exc);
        if (exc_str) fprintf(stderr, "JS Error [%s]: %s\n", name ? name : "<script>", exc_str);
        JS_FreeCString(thread->ctx, exc_str);
        JS_FreeValue(thread->ctx, exc);
    }

    JSContext *ctx1;
    while (JS_ExecutePendingJob(JS_GetRuntime(thread->ctx), &ctx1) > 0);
    JS_FreeValue(thread->ctx, result);
    return success;
}

static void qjs_event_handler(struct dom_event *evt, void *pw)
{
    struct qjs_event_listener_ctx *ctx = pw;
    if (!ctx || !ctx->thread || ctx->thread->closed) return;
    JSContext *jsctx = ctx->thread->ctx;
    JSValue global = JS_GetGlobalObject(jsctx);
    JSValue js_evt = JS_UNDEFINED;
    struct qjs_event_map *map = ctx->thread->events;
    while (map) {
        if (map->evt == evt) { js_evt = JS_DupValue(jsctx, map->js_evt); break; }
        map = map->next;
    }
    if (JS_IsUndefined(js_evt)) {
        js_evt = qjs_new_event(jsctx, evt, true);
        struct qjs_event_map *new_map = malloc(sizeof(*new_map));
        if (new_map) {
            dom_event_ref(evt); new_map->evt = evt;
            new_map->js_evt = JS_DupValue(jsctx, js_evt);
            new_map->next = ctx->thread->events; ctx->thread->events = new_map;
        }
    }
    JSValue this_obj = (ctx->target == (struct dom_event_target *)ctx->thread->win_priv || ctx->target == (struct dom_event_target *)ctx->thread->doc_priv) ? JS_DupValue(jsctx, global) : qjs_wrap_node(jsctx, (dom_node *)ctx->target);
    JSValue ret = JS_Call(jsctx, ctx->func, this_obj, 1, &js_evt);
    if (JS_IsException(ret)) {
        JSValue exc = JS_GetException(jsctx); const char *exc_str = JS_ToCString(jsctx, exc);
        if (exc_str) JS_FreeCString(jsctx, exc_str); JS_FreeValue(jsctx, exc);
    }
    JS_FreeValue(jsctx, ret); JS_FreeValue(jsctx, this_obj); JS_FreeValue(jsctx, js_evt); JS_FreeValue(jsctx, global);
}

bool js_fire_event(jsthread *thread, const char *type, struct dom_document *doc, struct dom_node *target)
{
    if (!thread || !doc) return false;
    if (!target) target = (dom_node *)doc;
    dom_string *type_str = NULL; dom_string_create((const uint8_t *)type, strlen(type), &type_str);
    dom_event *evt = NULL; dom_event_create(&evt);
    bool success = false;
    if (evt) {
        dom_event_init(evt, type_str, false, false);
        dom_event_target_dispatch_event((dom_event_target *)target, evt, &success);
        dom_event_unref(evt);
    }
    dom_string_unref(type_str);
    return success;
}

bool js_dom_event_add_listener(jsthread *thread, struct dom_document *document, struct dom_node *node, struct dom_string *event_type_dom, JSValue js_funcval)
{
    if (!thread || !node) return false;
    struct qjs_event_listener_ctx *ctx = malloc(sizeof(*ctx));
    if (!ctx) return false;
    ctx->thread = thread; ctx->func = JS_DupValue(thread->ctx, js_funcval);
    ctx->target = (struct dom_event_target *)node; ctx->type = event_type_dom;
    dom_node_ref(node); dom_string_ref(event_type_dom);
    dom_event_listener *listener;
    if (dom_event_listener_create(qjs_event_handler, ctx, &listener) != DOM_NO_ERR) {
        dom_node_unref(node); dom_string_unref(event_type_dom); JS_FreeValue(thread->ctx, ctx->func); free(ctx);
        return false;
    }
    ctx->listener = listener;
    dom_event_target_add_event_listener(ctx->target, ctx->type, listener, false);
    ctx->next = thread->listeners; thread->listeners = ctx;
    return true;
}

bool js_dom_event_remove_listener(jsthread *thread, struct dom_document *document, struct dom_node *node, struct dom_string *event_type_dom, JSValue js_funcval)
{
    if (!thread || !node) return false;
    struct qjs_event_listener_ctx **prev = &thread->listeners;
    struct qjs_event_listener_ctx *curr = thread->listeners;
    while (curr) {
        if (curr->target == (struct dom_event_target *)node && dom_string_isequal(curr->type, event_type_dom) && JS_VALUE_GET_PTR(curr->func) == JS_VALUE_GET_PTR(js_funcval)) {
            dom_event_target_remove_event_listener(curr->target, curr->type, curr->listener, false);
            dom_node_unref((struct dom_node *)curr->target); dom_string_unref(curr->type);
            JS_FreeValue(thread->ctx, curr->func); dom_event_listener_unref(curr->listener);
            *prev = curr->next; free(curr); return true;
        }
        prev = &curr->next; curr = curr->next;
    }
    return false;
}

void js_handle_new_element(jsthread *thread, struct dom_element *node) {}

bool js_event_cleanup(jsthread *thread, struct dom_event *evt)
{
    if (!thread || !evt) return false;
    struct qjs_event_map **prev = &thread->events, *curr = thread->events;
    while (curr) {
        if (curr->evt == evt) {
            *prev = curr->next; JS_FreeValue(thread->ctx, curr->js_evt);
            dom_event_unref(evt); free(curr); return true;
        }
        prev = &curr->next; curr = curr->next;
    }
    return false;
}

JSValue qjs_new_intersectionobserverentry_manual(JSContext *ctx, WispIntersectionObserverEntry *entry);

void js_handle_intersection_check(jsthread *thread, struct box *layout, int viewport_width, int viewport_height)
{
    if (!thread || !thread->intersection_observers || !layout) return;
    uint64_t now_ms; nsu_getmonotonic_ms(&now_ms);
    WispIntersectionObserver *obs = thread->intersection_observers;
    while (obs) {
        bool changed = false; IntersectionObserverTarget *ot = obs->targets;
        while (ot) {
            struct box *target_box = box_find_by_node(layout, ot->node);
            if (target_box) {
                int tx, ty; box_coords(target_box, &tx, &ty);
                int tw = target_box->width + target_box->padding[LEFT] + target_box->padding[RIGHT];
                int th = target_box->height + target_box->padding[TOP] + target_box->padding[BOTTOM];
                int ix0 = tx > 0 ? tx : 0, iy0 = ty > 0 ? ty : 0;
                int ix1 = (tx + tw) < viewport_width ? (tx + tw) : viewport_width;
                int iy1 = (ty + th) < viewport_height ? (ty + th) : viewport_height;
                bool isIntersecting = (ix1 > ix0) && (iy1 > iy0);
                if (isIntersecting != ot->wasIntersecting) {
                    WispIntersectionObserverEntry entry; memset(&entry, 0, sizeof(entry));
                    entry.time = (double)now_ms; entry.target = ot->node; dom_node_ref(ot->node);
                    entry.isIntersecting = isIntersecting; entry.targetX = tx; entry.targetY = ty;
                    entry.targetWidth = tw; entry.targetHeight = th;
                    entry.rootWidth = viewport_width; entry.rootHeight = viewport_height;
                    if (isIntersecting) {
                        entry.intersectX = ix0; entry.intersectY = iy0;
                        entry.intersectWidth = ix1 - ix0; entry.intersectHeight = iy1 - iy0;
                        entry.intersectionRatio = (double)(entry.intersectWidth * entry.intersectHeight) / (tw * th);
                    }
                    uint32_t len = 0; JSValue js_len = JS_GetPropertyStr(obs->ctx, obs->queue, "length");
                    JS_ToUint32(obs->ctx, &len, js_len); JS_FreeValue(obs->ctx, js_len);
                    JS_SetPropertyUint32(obs->ctx, obs->queue, len, qjs_new_intersectionobserverentry_manual(obs->ctx, &entry));
                    ot->wasIntersecting = isIntersecting; changed = true;
                }
            }
            ot = ot->next;
        }
        if (changed) {
            JSValue args[2]; args[0] = obs->queue; args[1] = JS_NULL;
            JSValue ret = JS_Call(obs->ctx, obs->callback, JS_UNDEFINED, 2, args);
            JS_FreeValue(obs->ctx, ret); JS_FreeValue(obs->ctx, obs->queue);
            obs->queue = JS_NewArray(obs->ctx);
        }
        obs = obs->next;
    }
}
