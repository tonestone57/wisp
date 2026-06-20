/*
 * Copyright 2026 Jules
 *
 * This file is part of Wisp.
 *
 * Wisp is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * Wisp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 * Blend2D plotter implementation.
 */

#include <blend2d.h>
#include <assert.h>
#include <math.h>

#include "wisp/plotters.h"
#include "wisp/utils/log.h"
#include "wisp/utils/errors.h"

static __thread BLPathCore current_path;
static __thread bool current_path_inited = false;

static void blend2d_set_colour(BLContextCore *ctx, colour c, float opacity, bool fill)
{
    BLRgba32 bl_color;
    /* NS color format is 0xAABBGGRR with inverted alpha */
    uint8_t r = c & 0xFF;
    uint8_t g = (c >> 8) & 0xFF;
    uint8_t b = (c >> 16) & 0xFF;
    uint8_t a = 255 - ((c >> 24) & 0xFF);

    if (opacity > 0.0f && opacity < 1.0f) {
        a = (uint8_t)(a * opacity);
    }

    bl_color.value = (a << 24) | (r << 16) | (g << 8) | b;

    if (fill) {
        blContextSetFillStyleRgba32(ctx, bl_color.value);
    } else {
        blContextSetStrokeStyleRgba32(ctx, bl_color.value);
    }
}

static nserror blend2d_plot_clip(const struct redraw_context *ctx, const struct rect *clip)
{
    BLContextCore *bl_ctx = (BLContextCore *)ctx->priv;
    BLRectI bl_rect = { clip->x0, clip->y0, clip->x1 - clip->x0, clip->y1 - clip->y0 };
    blContextClipToRectI(bl_ctx, &bl_rect);
    return NSERROR_OK;
}

static nserror blend2d_plot_arc(const struct redraw_context *ctx, const plot_style_t *pstyle, int x, int y, int radius, int angle1, int angle2)
{
    /* Blend2D doesn't have a direct arc function in the same way,
     * would need to build a path. Skipping for brevity in this initial integration. */
    return NSERROR_NOT_IMPLEMENTED;
}

static nserror blend2d_plot_disc(const struct redraw_context *ctx, const plot_style_t *pstyle, int x, int y, int radius)
{
    BLContextCore *bl_ctx = (BLContextCore *)ctx->priv;
    BLEllipse bl_ellipse = { (double)x, (double)y, (double)radius, (double)radius };

    if (pstyle->fill_type != PLOT_OP_TYPE_NONE) {
        blend2d_set_colour(bl_ctx, pstyle->fill_colour, pstyle->fill_opacity, true);
        blContextFillGeometry(bl_ctx, BL_GEOMETRY_TYPE_ELLIPSE, &bl_ellipse);
    }

    if (pstyle->stroke_type != PLOT_OP_TYPE_NONE) {
        blend2d_set_colour(bl_ctx, pstyle->stroke_colour, pstyle->stroke_opacity, false);
        blContextSetStrokeWidth(bl_ctx, plot_style_fixed_to_double(pstyle->stroke_width));
        blContextStrokeGeometry(bl_ctx, BL_GEOMETRY_TYPE_ELLIPSE, &bl_ellipse);
    }

    return NSERROR_OK;
}

static nserror blend2d_plot_line(const struct redraw_context *ctx, const plot_style_t *pstyle, const struct rect *line)
{
    BLContextCore *bl_ctx = (BLContextCore *)ctx->priv;
    BLLine bl_line = { (double)line->x0, (double)line->y0, (double)line->x1, (double)line->y1 };

    blend2d_set_colour(bl_ctx, pstyle->stroke_colour, pstyle->stroke_opacity, false);
    blContextSetStrokeWidth(bl_ctx, plot_style_fixed_to_double(pstyle->stroke_width));
    blContextStrokeGeometry(bl_ctx, BL_GEOMETRY_TYPE_LINE, &bl_line);

    return NSERROR_OK;
}

static nserror blend2d_plot_rectangle(const struct redraw_context *ctx, const plot_style_t *pstyle, const struct rect *rectangle)
{
    BLContextCore *bl_ctx = (BLContextCore *)ctx->priv;
    BLRect bl_rect = { (double)rectangle->x0, (double)rectangle->y0, (double)(rectangle->x1 - rectangle->x0), (double)(rectangle->y1 - rectangle->y0) };

    if (pstyle->fill_type != PLOT_OP_TYPE_NONE) {
        blend2d_set_colour(bl_ctx, pstyle->fill_colour, pstyle->fill_opacity, true);
        blContextFillRect(bl_ctx, &bl_rect);
    }

    if (pstyle->stroke_type != PLOT_OP_TYPE_NONE) {
        blend2d_set_colour(bl_ctx, pstyle->stroke_colour, pstyle->stroke_opacity, false);
        blContextSetStrokeWidth(bl_ctx, plot_style_fixed_to_double(pstyle->stroke_width));
        blContextStrokeRect(bl_ctx, &bl_rect);
    }

    return NSERROR_OK;
}

static nserror blend2d_plot_polygon(const struct redraw_context *ctx, const plot_style_t *pstyle, const int *p, unsigned int n)
{
    BLContextCore *bl_ctx = (BLContextCore *)ctx->priv;
    BLPoint *pts = malloc(sizeof(BLPoint) * n);
    if (!pts) return NSERROR_NOMEM;

    for (unsigned int i = 0; i < n; i++) {
        pts[i].x = (double)p[i * 2];
        pts[i].y = (double)p[i * 2 + 1];
    }

    if (pstyle->fill_type != PLOT_OP_TYPE_NONE) {
        blend2d_set_colour(bl_ctx, pstyle->fill_colour, pstyle->fill_opacity, true);
        blContextFillGeometry(bl_ctx, BL_GEOMETRY_TYPE_POLYGON, pts);
    }

    free(pts);
    return NSERROR_OK;
}

static nserror blend2d_plot_path(const struct redraw_context *ctx, const plot_style_t *pstyle, const float *p, unsigned int n, const float transform[6])
{
    /* This can be implemented using the new path API calls internally or by direct path building.
     * For now, we'll implement the new Path API first and maybe redirect this there. */
    return NSERROR_NOT_IMPLEMENTED;
}

static nserror blend2d_plot_path_begin(const struct redraw_context *ctx)
{
    if (!current_path_inited) {
        blPathInit(&current_path);
        current_path_inited = true;
    }
    blPathClear(&current_path);
    return NSERROR_OK;
}

static nserror blend2d_plot_path_move_to(const struct redraw_context *ctx, float x, float y)
{
    blPathMoveTo(&current_path, (double)x, (double)y);
    return NSERROR_OK;
}

static nserror blend2d_plot_path_line_to(const struct redraw_context *ctx, float x, float y)
{
    blPathLineTo(&current_path, (double)x, (double)y);
    return NSERROR_OK;
}

static nserror blend2d_plot_path_bezier_to(const struct redraw_context *ctx, float x1, float y1, float x2, float y2, float x3, float y3)
{
    blPathCubicTo(&current_path, (double)x1, (double)y1, (double)x2, (double)y2, (double)x3, (double)y3);
    return NSERROR_OK;
}

static nserror blend2d_plot_path_close(const struct redraw_context *ctx)
{
    blPathClose(&current_path);
    return NSERROR_OK;
}

static nserror blend2d_plot_path_fill(const struct redraw_context *ctx, const plot_style_t *pstyle, const float transform[6])
{
    BLContextCore *bl_ctx = (BLContextCore *)ctx->priv;

    blContextSave(bl_ctx, NULL);
    if (transform) {
        BLMatrix2D m = { (double)transform[0], (double)transform[1], (double)transform[2], (double)transform[3], (double)transform[4], (double)transform[5] };
        blContextApplyTransform(bl_ctx, &m);
    }

    blend2d_set_colour(bl_ctx, pstyle->fill_colour, pstyle->fill_opacity, true);
    blContextFillPath(bl_ctx, &current_path);

    blContextRestore(bl_ctx, NULL);
    return NSERROR_OK;
}

static nserror blend2d_plot_path_stroke(const struct redraw_context *ctx, const plot_style_t *pstyle, const float transform[6])
{
    BLContextCore *bl_ctx = (BLContextCore *)ctx->priv;

    blContextSave(bl_ctx, NULL);
    if (transform) {
        BLMatrix2D m = { (double)transform[0], (double)transform[1], (double)transform[2], (double)transform[3], (double)transform[4], (double)transform[5] };
        blContextApplyTransform(bl_ctx, &m);
    }

    blend2d_set_colour(bl_ctx, pstyle->stroke_colour, pstyle->stroke_opacity, false);
    blContextSetStrokeWidth(bl_ctx, plot_style_fixed_to_double(pstyle->stroke_width));
    blContextStrokePath(bl_ctx, &current_path);

    blContextRestore(bl_ctx, NULL);
    return NSERROR_OK;
}

static nserror blend2d_plot_bitmap(const struct redraw_context *ctx, struct bitmap *bitmap, int x, int y, int width, int height, colour bg, bitmap_flags_t flags)
{
    /* Blend2D integration for bitmaps requires matching Wisp's bitmap structure.
     * Skipping for now. */
    return NSERROR_NOT_IMPLEMENTED;
}

static nserror blend2d_plot_text(const struct redraw_context *ctx, const plot_font_style_t *fstyle, int x, int y, const char *text, size_t length)
{
    /* Text rendering in Blend2D requires font management.
     * This would likely bridge to Wisp's existing font system or use Blend2D fonts. */
    return NSERROR_NOT_IMPLEMENTED;
}

static nserror blend2d_push_transform(const struct redraw_context *ctx, const float transform[6])
{
    BLContextCore *bl_ctx = (BLContextCore *)ctx->priv;
    blContextSave(bl_ctx, NULL);
    BLMatrix2D m = { (double)transform[0], (double)transform[1], (double)transform[2], (double)transform[3], (double)transform[4], (double)transform[5] };
    blContextApplyTransform(bl_ctx, &m);
    return NSERROR_OK;
}

static nserror blend2d_pop_transform(const struct redraw_context *ctx)
{
    BLContextCore *bl_ctx = (BLContextCore *)ctx->priv;
    blContextRestore(bl_ctx, NULL);
    return NSERROR_OK;
}

const struct plotter_table blend2d_plotters = {
    .clip = blend2d_plot_clip,
    .arc = blend2d_plot_arc,
    .disc = blend2d_plot_disc,
    .line = blend2d_plot_line,
    .rectangle = blend2d_plot_rectangle,
    .polygon = blend2d_plot_polygon,
    .path = blend2d_plot_path,
    .path_begin = blend2d_plot_path_begin,
    .path_move_to = blend2d_plot_path_move_to,
    .path_line_to = blend2d_plot_path_line_to,
    .path_bezier_to = blend2d_plot_path_bezier_to,
    .path_close = blend2d_plot_path_close,
    .path_fill = blend2d_plot_path_fill,
    .path_stroke = blend2d_plot_path_stroke,
    .bitmap = blend2d_plot_bitmap,
    .text = blend2d_plot_text,
    .push_transform = blend2d_push_transform,
    .pop_transform = blend2d_pop_transform,
    .option_knockout = true
};
