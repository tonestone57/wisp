/*
 * Copyright 2027 Jules
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
 */

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <wisp/desktop/gui_internal.h>
#include <wisp/desktop/gui_table.h>
#include <wisp/content/content_protected.h>
#include <wisp/misc.h>
#include <wisp/utils/log.h>
#include "content/handlers/html/layout_animation.h"

static struct wisp_transition *active_transitions = NULL;
static bool timer_running = false;

static uint64_t get_current_time_ms(void)
{
#ifdef _WIN32
	return GetTickCount();
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#endif
}

static float ease_apply(const char *easing, float t)
{
	if (strcmp(easing, "ease-in") == 0) {
		return t * t;
	} else if (strcmp(easing, "ease-out") == 0) {
		return t * (2.0f - t);
	} else if (strcmp(easing, "ease-in-out") == 0) {
		if (t < 0.5f) {
			return 2.0f * t * t;
		} else {
			return -1.0f + (4.0f - 2.0f * t) * t;
		}
	} else {
		/* Default: linear */
		return t;
	}
}

static void wisp_animation_step_cb(void *p)
{
	(void)p;
	timer_running = false;
	wisp_animation_step();
}

void wisp_animation_init(void)
{
	active_transitions = NULL;
	timer_running = false;
}

void wisp_animation_fini(void)
{
	struct wisp_transition *curr = active_transitions;
	while (curr != NULL) {
		struct wisp_transition *next = curr->next;
		free(curr);
		curr = next;
	}
	active_transitions = NULL;
	timer_running = false;
}

void wisp_transition_start(struct box *box, const char *prop, float start, float end, uint32_t duration_ms, const char *easing)
{
	if (box == NULL || prop == NULL) return;

	/* Look for existing transition on this box/property */
	struct wisp_transition *curr = active_transitions;
	while (curr != NULL) {
		if (curr->box == box && strcmp(curr->property, prop) == 0) {
			curr->start_value = start;
			curr->end_value = end;
			curr->start_time_ms = get_current_time_ms();
			curr->duration_ms = duration_ms;
			strncpy(curr->easing, easing ? easing : "linear", sizeof(curr->easing) - 1);
			curr->easing[sizeof(curr->easing) - 1] = '\0';
			return;
		}
		curr = curr->next;
	}

	/* Add new transition */
	struct wisp_transition *t = malloc(sizeof(struct wisp_transition));
	if (t == NULL) return;

	t->box = box;
	strncpy(t->property, prop, sizeof(t->property) - 1);
	t->property[sizeof(t->property) - 1] = '\0';
	t->start_value = start;
	t->end_value = end;
	t->start_time_ms = get_current_time_ms();
	t->duration_ms = duration_ms > 0 ? duration_ms : 300;
	strncpy(t->easing, easing ? easing : "linear", sizeof(t->easing) - 1);
	t->easing[sizeof(t->easing) - 1] = '\0';

	t->next = active_transitions;
	active_transitions = t;

	/* Start timer loop if not already running */
	if (!timer_running && guit != NULL && guit->misc != NULL && guit->misc->schedule != NULL) {
		timer_running = true;
		guit->misc->schedule(16, wisp_animation_step_cb, NULL);
	}
}

void wisp_animation_step(void)
{
	uint64_t now = get_current_time_ms();
	struct wisp_transition *curr = active_transitions;
	struct wisp_transition *prev = NULL;

	while (curr != NULL) {
		bool finished = false;
		float progress = 0.0f;

		if (now >= curr->start_time_ms + curr->duration_ms) {
			progress = 1.0f;
			finished = true;
		} else {
			progress = (float)(now - curr->start_time_ms) / (float)curr->duration_ms;
			if (progress < 0.0f) progress = 0.0f;
			if (progress > 1.0f) progress = 1.0f;
		}

		float eased = ease_apply(curr->easing, progress);
		float val = curr->start_value + eased * (curr->end_value - curr->start_value);

		/* Apply animated value to box properties */
		if (strcmp(curr->property, "width") == 0) {
			curr->box->width = (int)val;
		} else if (strcmp(curr->property, "height") == 0) {
			curr->box->height = (int)val;
		} else if (strcmp(curr->property, "opacity") == 0) {
			/* Symmetrical style property updating */
			if (curr->box->style != NULL) {
				/* Update opacity style variable */
			}
		}

		/* Request repaint of the transition region */
		if (curr->box->content != NULL) {
			/* Direct targeted invalidation of the box region */
			content__request_redraw((struct content *)curr->box->content,
				curr->box->x, curr->box->y, curr->box->width, curr->box->height);
		}

		if (finished) {
			/* Remove completed transition */
			struct wisp_transition *next = curr->next;
			if (prev == NULL) {
				active_transitions = next;
			} else {
				prev->next = next;
			}
			free(curr);
			curr = next;
		} else {
			prev = curr;
			curr = curr->next;
		}
	}

	/* Reschedule step if transitions are still active */
	if (active_transitions != NULL && guit != NULL && guit->misc != NULL && guit->misc->schedule != NULL) {
		timer_running = true;
		guit->misc->schedule(16, wisp_animation_step_cb, NULL);
	}
}
