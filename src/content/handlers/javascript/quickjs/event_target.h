/*
 * Copyright 2025 Neosurf Contributors
 *
 * This file is part of NeoSurf, http://www.netsurf-browser.org/
 */

#ifndef WISP_QUICKJS_EVENT_TARGET_H
#define WISP_QUICKJS_EVENT_TARGET_H

#include "quickjs.h"

/**
 * Initialize EventTarget methods (addEventListener, removeEventListener) on
 * the Window object (global).
 *
 * @param ctx QuickJS context
 * @return 0 on success, -1 on failure
 */
int qjs_init_event_target(JSContext *ctx);

JSValue js_addEventListener(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_removeEventListener(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_dispatchEvent(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

#endif /* NEOSURF_QUICKJS_EVENT_TARGET_H */

JSValue js_addEventListener(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_removeEventListener(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_dispatchEvent(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
