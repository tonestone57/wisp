#ifndef WISP_QUICKJS_TIMERS_H
#define WISP_QUICKJS_TIMERS_H

#include "quickjs.h"

/**
 * Initialize timer support for a context.
 */
int qjs_init_timers(JSContext *ctx);

#endif /* WISP_QUICKJS_TIMERS_H */
