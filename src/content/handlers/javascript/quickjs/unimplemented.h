/*
 * Copyright 2025 Neosurf Contributors
 *
 * This file is part of NeoSurf, http://www.netsurf-browser.org/
 */

#ifndef WISP_QUICKJS_UNIMPLEMENTED_H
#define WISP_QUICKJS_UNIMPLEMENTED_H

#include "quickjs.h"

/**
 * Initialize unimplemented APIs as stubs.
 *
 * @param ctx QuickJS context
 * @return 0 on success, -1 on failure
 */
int qjs_init_unimplemented(JSContext *ctx);

#endif /* WISP_QUICKJS_UNIMPLEMENTED_H */
