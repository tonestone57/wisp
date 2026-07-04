/*
 * Copyright 2019 Vincent Sanders <vince@netsurf-browser.org>
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

/**
 * \file
 *
 * Browser core functionality
 */

#include "wisp/browser.h"
#include <math.h>
#include <wisp/content/handlers/css/utils.h>
#include <wisp/utils/errors.h>
#include <wisp/utils/log.h>
#include <wisp/utils/utils.h>

/* exported interface documented in netsurf/browser.h */
nserror browser_set_dpi(int dpi)
{
    if (dpi < 72 || dpi > 250) {
        int bad = dpi;
        dpi = min(max(dpi, 72), 250);
        NSLOG(wisp, INFO, "Clamping invalid DPI %d to %d", bad, dpi);
    }
    nscss_screen_dpi = INTTOFIX(dpi);

    return NSERROR_OK;
}

/* exported interface documented in netsurf/browser.h */
int browser_get_dpi(void)
{
    return FIXTOINT(nscss_screen_dpi);
}

/* exported interface documented in netsurf/browser.h */
int browser_get_tile_size(void)
{
    /* Scale-aware fixed tiles (256x256 for i586/retro, 512x512 for High-DPI). */
    if (browser_get_dpi() > 144) {
        return 512;
    }
    return 256;
}

/* exported interface documented in netsurf/browser.h */
float browser_calculate_tile_priority(int tile_x, int tile_y, int viewport_x, int viewport_y, int viewport_width,
    int viewport_height)
{
    int dx = 0;
    int dy = 0;
    int tile_size = browser_get_tile_size();
    int tile_cx = tile_x + tile_size / 2;
    int tile_cy = tile_y + tile_size / 2;

    /* Viewport bounds */
    int v_left = viewport_x;
    int v_right = viewport_x + viewport_width;
    int v_top = viewport_y;
    int v_bottom = viewport_y + viewport_height;

    /* Distance from tile center to viewport rectangle */
    if (tile_cx < v_left) dx = v_left - tile_cx;
    else if (tile_cx > v_right) dx = tile_cx - v_right;

    if (tile_cy < v_top) dy = v_top - tile_cy;
    else if (tile_cy > v_bottom) dy = tile_cy - v_bottom;

    float fdx = (float)dx;
    float fdy = (float)dy;
    float distance = sqrtf(fdx * fdx + fdy * fdy);

    /* Priority is inverse of distance: 1.0 for visible/near tiles, approaching 0 for distant ones. */
    return 1.0f / (1.0f + distance);
}
