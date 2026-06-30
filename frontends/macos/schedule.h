#ifndef WISP_MACOS_SCHEDULE_H
#define WISP_MACOS_SCHEDULE_H

#include "wisp/utils/errors.h"

/**
 * Schedule a callback to be run after a delay.
 *
 * \param ival The delay in milliseconds.
 * \param callback The function to call.
 * \param p The user parameter to pass to the callback.
 * \return NSERROR_OK on success, or an error code.
 */
nserror macos_schedule(int ival, void (*callback)(void *p), void *p);

/**
 * Run any scheduled callbacks that are due.
 *
 * \return The time in milliseconds until the next callback is due, or -1 if none.
 */
int schedule_run(void);

#endif
