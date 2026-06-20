/*
 * Copyright 2026 Wisp Contributors
 *
 * This file is part of Wisp, http://www.netsurf-browser.org/
 */

#ifndef WISP_JS_QJS_CRYPTO_H
#define WISP_JS_QJS_CRYPTO_H

#include "quickjs.h"

/**
 * Initialize Crypto bindings for the given context.
 */
int qjs_init_crypto(JSContext *ctx);

#endif
