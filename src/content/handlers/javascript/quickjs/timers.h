#ifndef WISP_QUICKJS_TIMERS_H
#define WISP_QUICKJS_TIMERS_H

#include "quickjs.h"

/**
 * Initialize timer support for a context.
 */
int qjs_init_timers(JSContext *ctx);

JSValue js_requestAnimationFrame(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_cancelAnimationFrame(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

#endif /* WISP_QUICKJS_TIMERS_H */
