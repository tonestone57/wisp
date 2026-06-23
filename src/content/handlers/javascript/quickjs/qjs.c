/*
 * Copyright 2024 Neosurf Contributors
 *
 * This file is part of NeoSurf, http://www.netsurf-browser.org/
 *
 * NeoSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NeoSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  See the file COPYING for details.
 */

/**
 * \file
 * QuickJS-ng implementation of JavaScript engine functions.
 *
 * This implements the js.h interface using the QuickJS-ng engine.
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
#include <nsutils/time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#endif

#include "content/handlers/javascript/quickjs/dom_bridge.h"

void *qjs_get_window_priv(JSContext *ctx)
{
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (t == NULL) {
        return NULL;
    }
    return t->win_priv;
}

void *qjs_get_document_priv(JSContext *ctx)
{
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (t == NULL) {
        return NULL;
    }
    return t->doc_priv;
}

void js_initialise(void)
{
    init_wisp_subsystem(64);
    NSLOG(wisp, INFO, "QuickJS-ng JavaScript engine initialised");
}

static int qjs_interrupt_handler(JSRuntime *rt, void *opaque)
{
    struct jsheap *heap = opaque;
    uint64_t now;

    if (heap->deadline_ms > 0) {
        nsu_getmonotonic_ms(&now);
        if (now > heap->deadline_ms) {
            NSLOG(wisp, WARNING, "JavaScript execution timeout exceeded");
            return 1; /* Interrupt execution */
        }
    }

    return 0; /* Continue execution */
}

void js_finalise(void)
{
    shutdown_wisp_subsystem();
    NSLOG(wisp, INFO, "QuickJS-ng JavaScript engine finalised");
}

nserror js_newheap(int timeout, jsheap **heap)
{
    jsheap *h;

    h = calloc(1, sizeof(*h));
    if (h == NULL) {
        return NSERROR_NOMEM;
    }

    h->rt = JS_NewRuntime();
    if (h->rt == NULL) {
        free(h);
        return NSERROR_NOMEM;
    }

    h->timeout = timeout;
    JS_SetMemoryLimit(h->rt, 64 * 1024 * 1024);
    JS_SetMaxStackSize(h->rt, 1024 * 1024);
    JS_SetInterruptHandler(h->rt, qjs_interrupt_handler, h);

    NSLOG(wisp, DEBUG, "Created QuickJS heap %p", h);

    *heap = h;
    return NSERROR_OK;
}

void js_destroyheap(jsheap *heap)
{
    if (heap == NULL) {
        return;
    }

    NSLOG(wisp, DEBUG, "Destroying QuickJS heap %p", heap);

    if (heap->rt != NULL) {
        qjs_bridge_cleanup(heap->rt);
        JS_FreeRuntime(heap->rt);
    }

    free(heap);
}

nserror js_newthread(jsheap *heap, void *win_priv, void *doc_priv, jsthread **thread)
{
    jsthread *t;

    if (heap == NULL) {
        return NSERROR_BAD_PARAMETER;
    }

    t = calloc(1, sizeof(*t));
    if (t == NULL) {
        return NSERROR_NOMEM;
    }

    t->ctx = JS_NewContext(heap->rt);
    if (t->ctx == NULL) {
        free(t);
        return NSERROR_NOMEM;
    }

    t->heap = heap;
    t->win_priv = win_priv;
    t->doc_priv = doc_priv;
    t->closed = false;

    JS_SetContextOpaque(t->ctx, t);

    qjs_init_dom_bridge(t->ctx);
    wisp_js_register_all_bindings(t->ctx);
    qjs_init_crypto(t->ctx);

    if (doc_priv) {
        JSValue doc_val = qjs_wrap_node(t->ctx, (dom_node *)doc_priv);
        JSValue global_obj = JS_GetGlobalObject(t->ctx);
        JS_SetPropertyStr(t->ctx, global_obj, "document", JS_DupValue(t->ctx, doc_val));
        JS_FreeValue(t->ctx, doc_val);
        JS_FreeValue(t->ctx, global_obj);
    }

    qjs_init_storage(t->ctx);
    qjs_init_xhr(t->ctx);

    NSLOG(wisp, DEBUG, "Created QuickJS thread %p in heap %p", t, heap);

    *thread = t;
    return NSERROR_OK;
}

nserror js_closethread(jsthread *thread)
{
    if (thread == NULL) {
        return NSERROR_BAD_PARAMETER;
    }

    NSLOG(wisp, DEBUG, "Closing QuickJS thread %p", thread);

    thread->closed = true;

    return NSERROR_OK;
}

void js_destroythread(jsthread *thread)
{
    if (thread == NULL) {
        return;
    }

    NSLOG(wisp, DEBUG, "Destroying QuickJS thread %p", thread);

    struct qjs_timer *tim = thread->timers;
    while (tim != NULL) {
        struct qjs_timer *next = tim->next;
        tim->cancelled = true;
        JS_FreeValue(thread->ctx, tim->func);
        free(tim);
        tim = next;
    }
    thread->timers = NULL;

    struct qjs_event_listener_ctx *l = thread->listeners;
    while (l != NULL) {
        struct qjs_event_listener_ctx *next = l->next;
        dom_event_target_remove_event_listener(l->target, l->type, l->listener, false);
        dom_node_unref((struct dom_node *)l->target);
        dom_string_unref(l->type);
        JS_FreeValue(thread->ctx, l->func);
        free(l);
        l = next;
    }
    thread->listeners = NULL;

    struct qjs_event_map *e = thread->events;
    while (e != NULL) {
        struct qjs_event_map *next = e->next;
        JS_FreeValue(thread->ctx, e->js_evt);
        free(e);
        e = next;
    }
    thread->events = NULL;

    if (thread->ctx != NULL) {
        JSRuntime *rt = JS_GetRuntime(thread->ctx);
        JSContext *ctx1;

        if (thread->heap->timeout > 0) {
            uint64_t now;
            nsu_getmonotonic_ms(&now);
            thread->heap->deadline_ms = now + (thread->heap->timeout * 1000);
        }

        while (JS_ExecutePendingJob(rt, &ctx1) > 0) {
        }

        thread->heap->deadline_ms = 0;

        qjs_finalise_dom_bridge(thread->ctx);
        JS_FreeContext(thread->ctx);
    }

    free(thread);
}

bool js_exec(jsthread *thread, const uint8_t *txt, size_t txtlen, const char *name)
{
    JSValue result;
    bool success = true;
    char stack_buf[1024];
    char *term_txt = NULL;

    if (thread == NULL || thread->ctx == NULL || thread->closed) {
        NSLOG(wisp, WARNING, "Attempted to execute JS on invalid/closed thread");
        return false;
    }

    if (txt == NULL || txtlen == 0) {
        return true;
    }

    NSLOG(wisp, INFO, "Executing JS: %s (length %zu)", name ? name : "<anonymous>", txtlen);

    if (thread->heap->timeout > 0) {
        uint64_t now;
        nsu_getmonotonic_ms(&now);
        thread->heap->deadline_ms = now + (thread->heap->timeout * 1000);
    }

    if (txtlen < sizeof(stack_buf)) {
        memcpy(stack_buf, txt, txtlen);
        stack_buf[txtlen] = '\0';
        term_txt = stack_buf;
    } else {
        term_txt = malloc(txtlen + 1);
        if (term_txt == NULL) {
            NSLOG(wisp, ERROR, "Failed to allocate memory for JS execution");
            return false;
        }
        memcpy(term_txt, txt, txtlen);
        term_txt[txtlen] = '\0';
    }

    result = JS_Eval(thread->ctx, term_txt, txtlen, name ? name : "<script>", JS_EVAL_TYPE_GLOBAL);
    thread->heap->deadline_ms = 0;

    JSContext *ctx1;
    while (JS_ExecutePendingJob(JS_GetRuntime(thread->ctx), &ctx1) > 0) {
    }

    if (JS_IsException(result)) {
        JSValue exc = JS_GetException(thread->ctx);
        const char *exc_str = JS_ToCString(thread->ctx, exc);

        NSLOG(wisp, WARNING, "JavaScript error: %s", exc_str ? exc_str : "<unknown error>");

        if (exc_str) {
            JS_FreeCString(thread->ctx, exc_str);
        }
        JS_FreeValue(thread->ctx, exc);
        success = false;
    }

    JS_FreeValue(thread->ctx, result);

    if (term_txt != stack_buf) {
        free(term_txt);
    }

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
    while (map != NULL) {
        if (map->evt == evt) {
            js_evt = JS_DupValue(jsctx, map->js_evt);
            break;
        }
        map = map->next;
    }

    if (JS_IsUndefined(js_evt)) {
        js_evt = JS_NewObject(jsctx);
        dom_string *type_str = NULL;
        dom_event_get_type(evt, &type_str);
        if (type_str) {
            JS_SetPropertyStr(jsctx, js_evt, "type",
                JS_NewStringLen(jsctx, (const char *)dom_string_data(type_str), dom_string_byte_length(type_str)));
            dom_string_unref(type_str);
        }

        struct qjs_event_map *new_map = malloc(sizeof(*new_map));
        if (new_map) {
            dom_event_ref(evt);
            new_map->evt = evt;
            new_map->js_evt = JS_DupValue(jsctx, js_evt);
            new_map->next = ctx->thread->events;
            ctx->thread->events = new_map;
        }
    }

    JSValue this_obj = global;
    if (ctx->target != (struct dom_event_target *)ctx->thread->doc_priv) {
        this_obj = JS_UNDEFINED;
    }
    JSValue ret = JS_Call(jsctx, ctx->func, this_obj, 1, &js_evt);
    if (JS_IsException(ret)) {
        JSValue exc = JS_GetException(jsctx);
        const char *exc_str = JS_ToCString(jsctx, exc);
        NSLOG(wisp, WARNING, "JS Error in event handler: %s", exc_str ? exc_str : "unknown");
        if (exc_str) JS_FreeCString(jsctx, exc_str);
        JS_FreeValue(jsctx, exc);
    }
    JS_FreeValue(jsctx, ret);
    JS_FreeValue(jsctx, js_evt);
    JS_FreeValue(jsctx, global);
}

bool js_fire_event(jsthread *thread, const char *type, struct dom_document *doc, struct dom_node *target)
{
    dom_exception exc;
    dom_string *type_str = NULL;
    dom_event *evt = NULL;
    bool success = true;

    if (thread == NULL || doc == NULL) return false;

    if (target == NULL) {
        target = (dom_node *)doc;
    }

    exc = dom_string_create((const uint8_t *)type, strlen(type), &type_str);
    if (exc != DOM_NO_ERR) return false;

    exc = dom_event_create(&evt);
    if (exc == DOM_NO_ERR) {
        exc = dom_event_init(evt, type_str, false, false);
        if (exc == DOM_NO_ERR) {
            exc = dom_event_target_dispatch_event((dom_event_target *)target, evt, &success);
        }
        dom_event_unref(evt);
    }

    dom_string_unref(type_str);
    return success;
}

bool js_dom_event_add_listener(jsthread *thread, struct dom_document *document, struct dom_node *node,
    struct dom_string *event_type_dom, JSValue js_funcval)
{
    if (!thread || !node) return false;

    struct qjs_event_listener_ctx *ctx = malloc(sizeof(*ctx));
    if (!ctx) return false;

    ctx->thread = thread;
    JSContext *jsctx = thread->ctx;
    ctx->func = JS_DupValue(jsctx, js_funcval);
    ctx->target = (struct dom_event_target *)node;
    ctx->type = event_type_dom;
    dom_node_ref(node);
    dom_string_ref(event_type_dom);

    dom_event_listener *listener;
    dom_exception exc = dom_event_listener_create(qjs_event_handler, ctx, &listener);
    if (exc != DOM_NO_ERR) {
        dom_node_unref(node);
        dom_string_unref(event_type_dom);
        JS_FreeValue(jsctx, ctx->func);
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
    if (!thread || !node) return false;

    JSContext *jsctx = thread->ctx;

    struct qjs_event_listener_ctx **prev = &thread->listeners;
    struct qjs_event_listener_ctx *curr = thread->listeners;

    while (curr != NULL) {
        if (curr->target == (struct dom_event_target *)node &&
            dom_string_isequal(curr->type, event_type_dom) &&
            JS_VALUE_GET_PTR(curr->func) == JS_VALUE_GET_PTR(js_funcval)) {

            dom_event_target_remove_event_listener(curr->target, curr->type, curr->listener, false);
            dom_node_unref((struct dom_node *)curr->target);
            dom_string_unref(curr->type);
            JS_FreeValue(jsctx, curr->func);
            dom_event_listener_unref(curr->listener);

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
    NSLOG(wisp, DEBUG, "js_handle_new_element called (not yet implemented)");
}

void js_event_cleanup(jsthread *thread, struct dom_event *evt)
{
    if (thread == NULL || evt == NULL) return;

    struct qjs_event_map **prev = &thread->events;
    struct qjs_event_map *curr = thread->events;

    while (curr != NULL) {
        if (curr->evt == evt) {
            *prev = curr->next;
            JS_FreeValue(thread->ctx, curr->js_evt);
            dom_event_unref(evt);
            free(curr);
            NSLOG(wisp, DEBUG, "js_event_cleanup successfully cleaned up event.");
            return;
        }
        prev = &curr->next;
        curr = curr->next;
    }
}
