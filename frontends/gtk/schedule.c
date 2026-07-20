/*
 * Copyright 2006-2007 Daniel Silverstone <dsilvers@digital-scurf.org>
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>

#include <wisp/utils/errors.h>
#include <wisp/utils/log.h>

#include "gtk/schedule.h"


/** Killable callback closure embodiment. */
typedef struct {
    void (*callback)(void *); /**< The callback function. */
    void *context; /**< The context for the callback. */
    bool callback_killed; /**< Whether or not this was killed. */
    uintptr_t id; /**< Unique callback ID. */
} _nsgtk_callback_t;

/** List of callbacks which have occurred and are pending running. */
static GList *pending_callbacks = NULL;
/** List of callbacks which are queued to occur in the future. */
static GList *queued_callbacks = NULL;
/** List of callbacks which are about to be run in this ::schedule_run. */
static GList *this_run = NULL;
/** List of callbacks which are currently active (allocated and not yet run or freed). */
static GList *active_callbacks = NULL;
static uintptr_t next_callback_id = 1;

static pthread_mutex_t schedule_lock = PTHREAD_MUTEX_INITIALIZER;

static _nsgtk_callback_t *find_active_callback_by_id(uintptr_t id)
{
    GList *l;
    for (l = active_callbacks; l != NULL; l = l->next) {
        _nsgtk_callback_t *cb = (_nsgtk_callback_t *)l->data;
        if (cb->id == id) {
            return cb;
        }
    }
    return NULL;
}

static gboolean nsgtk_schedule_generic_callback(gpointer data)
{
    uintptr_t cb_id = (uintptr_t)data;
    pthread_mutex_lock(&schedule_lock);
    _nsgtk_callback_t *cb = find_active_callback_by_id(cb_id);
    if (cb == NULL) {
        pthread_mutex_unlock(&schedule_lock);
        return FALSE;
    }
    if (cb->callback_killed) {
        /* This callback instance has been killed. */
        NSLOG(schedule, DEEPDEBUG, "CB at %p already dead.", cb);
    }
    queued_callbacks = g_list_remove(queued_callbacks, cb);
    pending_callbacks = g_list_append(pending_callbacks, cb);
    pthread_mutex_unlock(&schedule_lock);
    return FALSE;
}

static void nsgtk_schedule_kill_callback(void *_target, void *_match)
{
    _nsgtk_callback_t *target = (_nsgtk_callback_t *)_target;
    _nsgtk_callback_t *match = (_nsgtk_callback_t *)_match;
    if ((target->callback == match->callback) && (target->context == match->context)) {
        NSLOG(schedule, DEBUG, "Found match for %p(%p), killing.", target->callback, target->context);
        target->callback = NULL;
        target->context = NULL;
        target->callback_killed = true;
        match->callback_killed = true;
    }
}

/**
 * remove a matching callback and context tuple from all lists
 *
 * \param callback The callback to match
 * \param cbctx The callback context to match
 * \return NSERROR_OK if the tuple was removed from at least one list else
 * NSERROR_NOT_FOUND
 */
static nserror schedule_remove(void (*callback)(void *p), void *cbctx)
{
    _nsgtk_callback_t cb_match = {
        .callback = callback,
        .context = cbctx,
        .callback_killed = false,
    };

    pthread_mutex_lock(&schedule_lock);
    g_list_foreach(queued_callbacks, nsgtk_schedule_kill_callback, &cb_match);
    g_list_foreach(pending_callbacks, nsgtk_schedule_kill_callback, &cb_match);
    g_list_foreach(this_run, nsgtk_schedule_kill_callback, &cb_match);
    pthread_mutex_unlock(&schedule_lock);

    if (cb_match.callback_killed == false) {
        return NSERROR_NOT_FOUND;
    }
    return NSERROR_OK;
}

typedef struct {
    int t;
    uintptr_t cb_id;
} invoke_data_t;

static gboolean nsgtk_schedule_invoke_cb(gpointer data)
{
    invoke_data_t *id = (invoke_data_t *)data;
    pthread_mutex_lock(&schedule_lock);
    _nsgtk_callback_t *cb = find_active_callback_by_id(id->cb_id);
    if (cb != NULL) {
        g_timeout_add(id->t, nsgtk_schedule_generic_callback, (gpointer)id->cb_id);
        g_main_context_wakeup(NULL);
    }
    pthread_mutex_unlock(&schedule_lock);
    free(id);
    return FALSE;
}

/* exported interface documented in gtk/schedule.h */
nserror nsgtk_schedule(int t, void (*callback)(void *p), void *cbctx)
{
    _nsgtk_callback_t *cb;
    nserror res;

    /* Kill any pending schedule of this kind. */
    res = schedule_remove(callback, cbctx);

    /* only removal */
    if (t < 0) {
        return res;
    }

    cb = malloc(sizeof(_nsgtk_callback_t));
    if (cb == NULL) {
        return NSERROR_NOMEM;
    }
    cb->callback = callback;
    cb->context = cbctx;
    cb->callback_killed = false;

    pthread_mutex_lock(&schedule_lock);
    cb->id = next_callback_id++;
    /* Prepend is faster right now. */
    queued_callbacks = g_list_prepend(queued_callbacks, cb);
    active_callbacks = g_list_append(active_callbacks, cb);
    pthread_mutex_unlock(&schedule_lock);

    invoke_data_t *id = malloc(sizeof(*id));
    if (id != NULL) {
        id->t = t;
        id->cb_id = cb->id;
        g_main_context_invoke(NULL, nsgtk_schedule_invoke_cb, id);
    } else {
        /* Fallback if OOM */
        g_timeout_add(t, nsgtk_schedule_generic_callback, (gpointer)cb->id);
        g_main_context_wakeup(NULL);
    }

    return NSERROR_OK;
}

bool schedule_run(void)
{
    pthread_mutex_lock(&schedule_lock);
    /* Capture this run of pending callbacks into the list. */
    this_run = pending_callbacks;

    if (this_run == NULL) {
        pthread_mutex_unlock(&schedule_lock);
        return false; /* Nothing to do */
    }

    /* Clear the pending list. */
    pending_callbacks = NULL;
    pthread_mutex_unlock(&schedule_lock);

    NSLOG(schedule, DEBUG, "Captured a run of %d callbacks to fire.", g_list_length(this_run));

    /* Run all the callbacks which made it this far. */
    while (true) {
        pthread_mutex_lock(&schedule_lock);
        if (this_run == NULL) {
            pthread_mutex_unlock(&schedule_lock);
            break;
        }
        _nsgtk_callback_t *cb = (_nsgtk_callback_t *)(this_run->data);
        this_run = g_list_remove(this_run, this_run->data);
        active_callbacks = g_list_remove(active_callbacks, cb);
        pthread_mutex_unlock(&schedule_lock);

        if (!cb->callback_killed)
            cb->callback(cb->context);
        free(cb);
    }
    return true;
}

void nsgtk_schedule_finalise(void)
{
    GList *l;
    pthread_mutex_lock(&schedule_lock);
    for (l = queued_callbacks; l != NULL; l = l->next) {
        free(l->data);
    }
    g_list_free(queued_callbacks);
    queued_callbacks = NULL;

    for (l = pending_callbacks; l != NULL; l = l->next) {
        free(l->data);
    }
    g_list_free(pending_callbacks);
    pending_callbacks = NULL;

    for (l = this_run; l != NULL; l = l->next) {
        free(l->data);
    }
    g_list_free(this_run);
    this_run = NULL;

    g_list_free(active_callbacks);
    active_callbacks = NULL;
    pthread_mutex_unlock(&schedule_lock);
}
