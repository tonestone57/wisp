#ifndef QJS_TIMERS_H
#define QJS_TIMERS_H

#include "quickjs.h"

int qjs_init_timers(JSContext *ctx);
JSValue js_setTimeout(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_setInterval(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_clearTimeout(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_clearInterval(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

#endif
