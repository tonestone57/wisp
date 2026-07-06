/*
 * Copyright 2026 Jules
 *
 * This file is part of Wisp.
 *
 * Wisp is free software; you can redistribute it and/or modify
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

#ifndef WISP_DESKTOP_PLOT_BLEND2D_H_
#define WISP_DESKTOP_PLOT_BLEND2D_H_

#ifdef WITH_BLEND2D

#include <blend2d/blend2d.h>
#include "wisp/plotters.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef nserror (*blend2d_native_text_handler)(const struct redraw_context *ctx, const plot_font_style_t *fstyle, int x, int y, const char *text, size_t length);

/**
 * Context for Blend2D plotter.
 * This structure allows interop with native text rendering engines.
 */
struct blend2d_context {
    BLContextCore *bl_ctx;             /**< The Blend2D context */
    void *native_ctx;                  /**< Native context (e.g. HDC, QImage*, CGContextRef) */
    blend2d_native_text_handler native_text_handler; /**< Optional native text renderer */
};

extern const struct plotter_table blend2d_plotters;

#ifdef __cplusplus
}
#endif

#endif

#endif
