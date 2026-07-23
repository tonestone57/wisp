/*
 * Copyright 2025 Neosurf Contributors
 *
 * This file is part of NeoSurf, http://www.netsurf-browser.org/
 */

#include <wisp/utils/log.h>
#include <wisp/desktop/gui_table.h>
#include <wisp/misc.h>
#include "quickjs.h"
#include "qjs_internal.h"
#include "dom_bridge.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <nsutils/time.h>

extern struct wisp_table *guit;

static int next_timer_id = 1;

void qjs_raf_callback_fn(void *p);
void qjs_idle_callback_fn(void *p);

static JSValue js_idle_deadline_timeRemaining(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    JSValue start_val = JS_GetPropertyStr(ctx, this_val, "__start");
    double start = 0;
    JS_ToFloat64(ctx, &start, start_val);
    JS_FreeValue(ctx, start_val);

    uint64_t now = 0;
    nsu_getmonotonic_ms(&now);

    double elapsed = (double)now - start;
    double remaining = 50.0 - elapsed;
    if (remaining < 0) remaining = 0;
    return JS_NewFloat64(ctx, remaining);
}

void qjs_raf_callback_fn(void *p)
{
    struct qjs_raf_callback *raf = p;
    JSContext *ctx = raf->ctx;

    if (raf->cancelled) {
        struct jsthread *t = JS_GetContextOpaque(ctx);
        if (t) {
            struct qjs_raf_callback **prev = &t->raf_callbacks;
            struct qjs_raf_callback *curr = t->raf_callbacks;
            while (curr) {
                if (curr == raf) {
                    *prev = curr->next;
                    break;
                }
                prev = &curr->next;
                curr = curr->next;
            }
        }
        JS_FreeValue(ctx, raf->func);
        free(raf);
        return;
    }

    struct jsthread *t = JS_GetContextOpaque(ctx);
    uint64_t old_deadline = 0;
    uint64_t old_last_yield = 0;
    if (t && t->heap) {
        old_deadline = t->heap->deadline_ms;
        old_last_yield = t->heap->last_yield_ms;
        uint64_t now;
        nsu_getmonotonic_ms(&now);
        t->heap->deadline_ms = now + 3000;
        t->heap->last_yield_ms = now;
    }

    uint64_t now_ms = 0;
    nsu_getmonotonic_ms(&now_ms);
    JSValue arg = JS_NewFloat64(ctx, (double)now_ms);

    JSValue ret = JS_Call(ctx, raf->func, JS_UNDEFINED, 1, &arg);
    JS_FreeValue(ctx, arg);

    if (t && t->heap) {
        t->heap->deadline_ms = old_deadline;
        t->heap->last_yield_ms = old_last_yield;
    }

    if (JS_IsException(ret)) {
        JSValue exc = JS_GetException(ctx);
        const char *exc_str = JS_ToCString(ctx, exc);
        NSLOG(wisp, WARNING, "JS Error in rAF callback: %s", exc_str ? exc_str : "unknown");
        if (exc_str) JS_FreeCString(ctx, exc_str);
        JS_FreeValue(ctx, exc);
    }
    JS_FreeValue(ctx, ret);

    JSContext *ctx1;
    int job_ret;
    while ((job_ret = JS_ExecutePendingJob(JS_GetRuntime(ctx), &ctx1)) != 0) {
        if (job_ret < 0) {
            JSValue exc = JS_GetException(ctx1);
            const char *exc_str = JS_ToCString(ctx1, exc);
            NSLOG(wisp, WARNING, "JS Error in microtask: %s", exc_str ? exc_str : "unknown");
            if (exc_str) JS_FreeCString(ctx1, exc_str);
            JS_FreeValue(ctx1, exc);
        }
    }

    if (t) {
        struct qjs_raf_callback **prev = &t->raf_callbacks;
        struct qjs_raf_callback *curr = t->raf_callbacks;
        while (curr) {
            if (curr == raf) {
                *prev = curr->next;
                break;
            }
            prev = &curr->next;
            curr = curr->next;
        }
    }
    JS_FreeValue(ctx, raf->func);
    free(raf);
}

void qjs_idle_callback_fn(void *p)
{
    struct qjs_idle_callback *idle = p;
    JSContext *ctx = idle->ctx;

    if (idle->cancelled) {
        struct jsthread *t = JS_GetContextOpaque(ctx);
        if (t) {
            struct qjs_idle_callback **prev = &t->idle_callbacks;
            struct qjs_idle_callback *curr = t->idle_callbacks;
            while (curr) {
                if (curr == idle) {
                    *prev = curr->next;
                    break;
                }
                prev = &curr->next;
                curr = curr->next;
            }
        }
        JS_FreeValue(ctx, idle->func);
        free(idle);
        return;
    }

    struct jsthread *t = JS_GetContextOpaque(ctx);
    uint64_t old_deadline = 0;
    uint64_t old_last_yield = 0;
    if (t && t->heap) {
        old_deadline = t->heap->deadline_ms;
        old_last_yield = t->heap->last_yield_ms;
        uint64_t now;
        nsu_getmonotonic_ms(&now);
        t->heap->deadline_ms = now + 3000;
        t->heap->last_yield_ms = now;
    }

    uint64_t now_ms = 0;
    nsu_getmonotonic_ms(&now_ms);

    bool did_timeout = false;
    if (idle->timeout > 0 && (now_ms - idle->scheduled_time >= idle->timeout)) {
        did_timeout = true;
    }

    JSValue deadline = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, deadline, "didTimeout", JS_NewBool(ctx, did_timeout));
    JS_SetPropertyStr(ctx, deadline, "__start", JS_NewFloat64(ctx, (double)now_ms));
    JS_SetPropertyStr(ctx, deadline, "timeRemaining", JS_NewCFunction(ctx, js_idle_deadline_timeRemaining, "timeRemaining", 0));

    JSValue ret = JS_Call(ctx, idle->func, JS_UNDEFINED, 1, &deadline);
    JS_FreeValue(ctx, deadline);

    if (t && t->heap) {
        t->heap->deadline_ms = old_deadline;
        t->heap->last_yield_ms = old_last_yield;
    }

    if (JS_IsException(ret)) {
        JSValue exc = JS_GetException(ctx);
        const char *exc_str = JS_ToCString(ctx, exc);
        NSLOG(wisp, WARNING, "JS Error in idle callback: %s", exc_str ? exc_str : "unknown");
        if (exc_str) JS_FreeCString(ctx, exc_str);
        JS_FreeValue(ctx, exc);
    }
    JS_FreeValue(ctx, ret);

    JSContext *ctx1;
    int job_ret;
    while ((job_ret = JS_ExecutePendingJob(JS_GetRuntime(ctx), &ctx1)) != 0) {
        if (job_ret < 0) {
            JSValue exc = JS_GetException(ctx1);
            const char *exc_str = JS_ToCString(ctx1, exc);
            NSLOG(wisp, WARNING, "JS Error in microtask: %s", exc_str ? exc_str : "unknown");
            if (exc_str) JS_FreeCString(ctx1, exc_str);
            JS_FreeValue(ctx1, exc);
        }
    }

    if (t) {
        struct qjs_idle_callback **prev = &t->idle_callbacks;
        struct qjs_idle_callback *curr = t->idle_callbacks;
        while (curr) {
            if (curr == idle) {
                *prev = curr->next;
                break;
            }
            prev = &curr->next;
            curr = curr->next;
        }
    }
    JS_FreeValue(ctx, idle->func);
    free(idle);
}

void qjs_timer_callback(void *p)
{
    struct qjs_timer *timer = p;
    JSContext *ctx = timer->ctx;

    if (timer->cancelled) {
        /* Timer was cancelled, but callback already scheduled.
         * Remove from active list and free. */
        struct jsthread *t = JS_GetContextOpaque(ctx);
        if (t) {
            struct qjs_timer **prev = &t->timers;
            struct qjs_timer *curr = t->timers;
            while (curr) {
                if (curr == timer) {
                    *prev = curr->next;
                    break;
                }
                prev = &curr->next;
                curr = curr->next;
            }
        }
        JS_FreeValue(ctx, timer->func);
        free(timer);
        return;
    }

    struct jsthread *t = JS_GetContextOpaque(ctx);
    uint64_t old_deadline = 0;
    uint64_t old_last_yield = 0;
    if (t && t->heap) {
        old_deadline = t->heap->deadline_ms;
        old_last_yield = t->heap->last_yield_ms;
        uint64_t now;
        nsu_getmonotonic_ms(&now);
        t->heap->deadline_ms = now + 3000; // Absolute deadline 3s in future
        t->heap->last_yield_ms = now;
    }

    JSValue ret = JS_Call(ctx, timer->func, JS_UNDEFINED, 0, NULL);

    if (t && t->heap) {
        t->heap->deadline_ms = old_deadline;
        t->heap->last_yield_ms = old_last_yield;
    }

    if (JS_IsException(ret)) {
        JSValue exc = JS_GetException(ctx);
        const char *exc_str = JS_ToCString(ctx, exc);
        NSLOG(wisp, WARNING, "JS Error in timer callback: %s", exc_str ? exc_str : "unknown");
        if (exc_str) JS_FreeCString(ctx, exc_str);
        JS_FreeValue(ctx, exc);
    }
    JS_FreeValue(ctx, ret);

    /* Process pending jobs (Promises) after timer execution */
    JSContext *ctx1;
    int job_ret;
    while ((job_ret = JS_ExecutePendingJob(JS_GetRuntime(ctx), &ctx1)) != 0) {
        if (job_ret < 0) {
            JSValue exc = JS_GetException(ctx1);
            const char *exc_str = JS_ToCString(ctx1, exc);
            NSLOG(wisp, WARNING, "JS Error in microtask: %s", exc_str ? exc_str : "unknown");
            if (exc_str) JS_FreeCString(ctx1, exc_str);
            JS_FreeValue(ctx1, exc);
        }
    }

    if (timer->repeat && !timer->cancelled) {
        /* Re-schedule if interval and not cancelled */
        if (guit && guit->misc && guit->misc->schedule) {
            guit->misc->schedule(timer->interval, qjs_timer_callback, timer);
        }
    } else {
        /* Remove from active list and free */
        struct jsthread *t = JS_GetContextOpaque(ctx);
        if (t) {
            struct qjs_timer **prev = &t->timers;
            struct qjs_timer *curr = t->timers;
            while (curr) {
                if (curr == timer) {
                    *prev = curr->next;
                    break;
                }
                prev = &curr->next;
                curr = curr->next;
            }
        }
        JS_FreeValue(ctx, timer->func);
        free(timer);
    }
}

static JSValue js_setTimeout_internal(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, bool repeat)
{
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (!t) return JS_EXCEPTION;

    if (argc < 1 || !JS_IsFunction(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "Expected function as first argument");
    }

    int32_t delay = 0;
    if (argc >= 2) {
        JS_ToInt32(ctx, &delay, argv[1]);
    }
    if (delay < 0) delay = 0;

    struct qjs_timer *timer = malloc(sizeof(*timer));
    if (!timer) return JS_ThrowOutOfMemory(ctx);

    timer->ctx = ctx;
    timer->func = JS_DupValue(ctx, argv[0]);
    timer->repeat = repeat;
    timer->interval = delay;
    timer->id = next_timer_id++;
    timer->cancelled = false;

    timer->next = t->timers;
    t->timers = timer;

    if (guit && guit->misc && guit->misc->schedule) {
        if (guit->misc->schedule(delay, qjs_timer_callback, timer) != NSERROR_OK) {
            t->timers = timer->next;
            JS_FreeValue(ctx, timer->func);
            free(timer);
            return JS_ThrowInternalError(ctx, "Failed to schedule timer");
        }
    } else {
        NSLOG(wisp, WARNING, "No GUI scheduler available for timers");
        t->timers = timer->next;
        JS_FreeValue(ctx, timer->func);
        free(timer);
        return JS_UNDEFINED;
    }

    return JS_NewInt32(ctx, timer->id);
}

static JSValue js_setTimeout(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return js_setTimeout_internal(ctx, this_val, argc, argv, false);
}

static JSValue js_setInterval(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return js_setTimeout_internal(ctx, this_val, argc, argv, true);
}

static JSValue js_clearTimeout(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (!t) return JS_UNDEFINED;

    if (argc < 1) return JS_UNDEFINED;
    int32_t id;
    JS_ToInt32(ctx, &id, argv[0]);

    struct qjs_timer *curr = t->timers;
    while (curr) {
        if (curr->id == id) {
            curr->cancelled = true;
            NSLOG(wisp, INFO, "Timer %d cancelled", id);
            return JS_UNDEFINED;
        }
        curr = curr->next;
    }
    return JS_UNDEFINED;
}

static JSValue js_clearInterval(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return js_clearTimeout(ctx, this_val, argc, argv);
}

static JSValue js_requestAnimationFrame(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (!t) return JS_EXCEPTION;

    if (argc < 1 || !JS_IsFunction(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "Expected function as first argument");
    }

    struct qjs_raf_callback *raf = malloc(sizeof(*raf));
    if (!raf) return JS_ThrowOutOfMemory(ctx);

    if (t->next_raf_id == 0) t->next_raf_id = 1;

    raf->ctx = ctx;
    raf->func = JS_DupValue(ctx, argv[0]);
    raf->id = t->next_raf_id++;
    raf->cancelled = false;

    raf->next = t->raf_callbacks;
    t->raf_callbacks = raf;

    if (guit && guit->misc && guit->misc->schedule) {
        if (guit->misc->schedule(16, qjs_raf_callback_fn, raf) != NSERROR_OK) {
            t->raf_callbacks = raf->next;
            JS_FreeValue(ctx, raf->func);
            free(raf);
            return JS_ThrowInternalError(ctx, "Failed to schedule requestAnimationFrame");
        }
    } else {
        NSLOG(wisp, WARNING, "No GUI scheduler available for requestAnimationFrame");
        t->raf_callbacks = raf->next;
        JS_FreeValue(ctx, raf->func);
        free(raf);
        return JS_UNDEFINED;
    }

    return JS_NewInt32(ctx, raf->id);
}

static JSValue js_cancelAnimationFrame(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (!t) return JS_UNDEFINED;

    if (argc < 1) return JS_UNDEFINED;
    int32_t id;
    JS_ToInt32(ctx, &id, argv[0]);

    struct qjs_raf_callback **prev = &t->raf_callbacks;
    struct qjs_raf_callback *curr = t->raf_callbacks;
    while (curr) {
        if (curr->id == id) {
            *prev = curr->next;
            if (guit && guit->misc && guit->misc->schedule) {
                guit->misc->schedule(-1, qjs_raf_callback_fn, curr);
            }
            JS_FreeValue(ctx, curr->func);
            free(curr);
            NSLOG(wisp, INFO, "rAF %d cancelled", id);
            return JS_UNDEFINED;
        }
        prev = &curr->next;
        curr = curr->next;
    }
    return JS_UNDEFINED;
}

static JSValue js_requestIdleCallback(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (!t) return JS_EXCEPTION;

    if (argc < 1 || !JS_IsFunction(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "Expected function as first argument");
    }

    uint32_t timeout = 0;
    if (argc >= 2 && JS_IsObject(argv[1])) {
        JSValue timeout_val = JS_GetPropertyStr(ctx, argv[1], "timeout");
        if (JS_IsNumber(timeout_val)) {
            JS_ToUint32(ctx, &timeout, timeout_val);
        }
        JS_FreeValue(ctx, timeout_val);
    }

    struct qjs_idle_callback *idle = malloc(sizeof(*idle));
    if (!idle) return JS_ThrowOutOfMemory(ctx);

    if (t->next_idle_id == 0) t->next_idle_id = 1;

    uint64_t now_ms = 0;
    nsu_getmonotonic_ms(&now_ms);

    idle->ctx = ctx;
    idle->func = JS_DupValue(ctx, argv[0]);
    idle->id = t->next_idle_id++;
    idle->cancelled = false;
    idle->timeout = timeout;
    idle->scheduled_time = now_ms;

    idle->next = t->idle_callbacks;
    t->idle_callbacks = idle;

    int delay = 50;
    if (timeout > 0 && timeout < 50) {
        delay = timeout;
    }

    if (guit && guit->misc && guit->misc->schedule) {
        if (guit->misc->schedule(delay, qjs_idle_callback_fn, idle) != NSERROR_OK) {
            t->idle_callbacks = idle->next;
            JS_FreeValue(ctx, idle->func);
            free(idle);
            return JS_ThrowInternalError(ctx, "Failed to schedule requestIdleCallback");
        }
    } else {
        NSLOG(wisp, WARNING, "No GUI scheduler available for requestIdleCallback");
        t->idle_callbacks = idle->next;
        JS_FreeValue(ctx, idle->func);
        free(idle);
        return JS_UNDEFINED;
    }

    return JS_NewInt32(ctx, idle->id);
}

static JSValue js_cancelIdleCallback(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (!t) return JS_UNDEFINED;

    if (argc < 1) return JS_UNDEFINED;
    int32_t id;
    JS_ToInt32(ctx, &id, argv[0]);

    struct qjs_idle_callback **prev = &t->idle_callbacks;
    struct qjs_idle_callback *curr = t->idle_callbacks;
    while (curr) {
        if (curr->id == id) {
            *prev = curr->next;
            if (guit && guit->misc && guit->misc->schedule) {
                guit->misc->schedule(-1, qjs_idle_callback_fn, curr);
            }
            JS_FreeValue(ctx, curr->func);
            free(curr);
            NSLOG(wisp, INFO, "rIC %d cancelled", id);
            return JS_UNDEFINED;
        }
        prev = &curr->next;
        curr = curr->next;
    }
    return JS_UNDEFINED;
}

int qjs_init_timers(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);

    /* Check if already initialized */
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_timers_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    JS_SetPropertyStr(ctx, global_obj, "setTimeout", JS_NewCFunction(ctx, js_setTimeout, "setTimeout", 2));
    JS_SetPropertyStr(ctx, global_obj, "clearTimeout", JS_NewCFunction(ctx, js_clearTimeout, "clearTimeout", 1));
    JS_SetPropertyStr(ctx, global_obj, "setInterval", JS_NewCFunction(ctx, js_setInterval, "setInterval", 2));
    JS_SetPropertyStr(ctx, global_obj, "clearInterval", JS_NewCFunction(ctx, js_clearTimeout, "clearInterval", 1));
    JS_SetPropertyStr(ctx, global_obj, "requestAnimationFrame", JS_NewCFunction(ctx, js_requestAnimationFrame, "requestAnimationFrame", 1));
    JS_SetPropertyStr(ctx, global_obj, "cancelAnimationFrame", JS_NewCFunction(ctx, js_cancelAnimationFrame, "cancelAnimationFrame", 1));
    JS_SetPropertyStr(ctx, global_obj, "requestIdleCallback", JS_NewCFunction(ctx, js_requestIdleCallback, "requestIdleCallback", 1));
    JS_SetPropertyStr(ctx, global_obj, "cancelIdleCallback", JS_NewCFunction(ctx, js_cancelIdleCallback, "cancelIdleCallback", 1));

    /* Mark as initialized */
    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_timers_init", JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);
    return 0;
}
