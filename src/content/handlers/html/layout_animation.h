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

/**
 * \file
 * CSS Transitions and Animations engine interface.
 */

#ifndef WISP_HTML_LAYOUT_ANIMATION_H
#define WISP_HTML_LAYOUT_ANIMATION_H

#include <stdbool.h>
#include <stdint.h>
#include <wisp/content/handlers/html/box.h>

struct wisp_transition {
	struct box *box;
	char property[32];
	float start_value;
	float end_value;
	uint64_t start_time_ms;
	uint32_t duration_ms;
	char easing[16]; /* linear, ease, ease-in, ease-out, ease-in-out */
	struct wisp_transition *next;
};

void wisp_animation_init(void);
void wisp_animation_fini(void);
void wisp_transition_start(struct box *box, const char *prop, float start, float end, uint32_t duration_ms, const char *easing);
void wisp_animation_step(void);

#endif
