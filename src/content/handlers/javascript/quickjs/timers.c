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

extern struct wisp_table *guit;

static int next_timer_id = 1;

static void qjs_timer_callback(void *p)
{
    struct qjs_timer *timer = p;
    if (timer->cancelled) {
        /* Timer was cancelled, but callback already scheduled */
        return;
    }

    JSContext *ctx = timer->ctx;

    JSValue ret = JS_Call(ctx, timer->func, JS_UNDEFINED, 0, NULL);
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
    while (JS_ExecutePendingJob(JS_GetRuntime(ctx), &ctx1) > 0) ;

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

int qjs_init_timers(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);

    JS_SetPropertyStr(ctx, global_obj, "setTimeout", JS_NewCFunction(ctx, js_setTimeout, "setTimeout", 2));
    JS_SetPropertyStr(ctx, global_obj, "clearTimeout", JS_NewCFunction(ctx, js_clearTimeout, "clearTimeout", 1));
    JS_SetPropertyStr(ctx, global_obj, "setInterval", JS_NewCFunction(ctx, js_setInterval, "setInterval", 2));
    JS_SetPropertyStr(ctx, global_obj, "clearInterval", JS_NewCFunction(ctx, js_clearInterval, "clearInterval", 1));

    JS_FreeValue(ctx, global_obj);
    return 0;
}
