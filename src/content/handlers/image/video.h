/*
 * Copyright 2011 John-Mark Bell <jmb@netsurf-browser.org>
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

#ifndef WISP_IMAGE_VIDEO_H_
#define WISP_IMAGE_VIDEO_H_

#include <wisp/utils/errors.h>
#include <wisp/content.h>

nserror nsvideo_init(void);

void nsvideo_play(struct content *c);
void nsvideo_pause(struct content *c);
void nsvideo_seek_to(struct content *c, double time);
void nsvideo_set_volume(struct content *c, float volume);
double nsvideo_get_duration(struct content *c);
double nsvideo_get_time(struct content *c);
bool nsvideo_is_paused(struct content *c);

#endif
