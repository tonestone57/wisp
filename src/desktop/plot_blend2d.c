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

/**
 * \file
 * Blend2D plotter implementation.
 */

#include <blend2d/blend2d.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "wisp/plotters.h"
#include "wisp/bitmap.h"
#include "wisp/desktop/plot_blend2d.h"
#include "wisp/desktop/gui_internal.h"
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

    if (opacity >= 0.0f && opacity < 1.0f) {
        a = (uint8_t)(a * opacity);
    }

    bl_color.value = (a << 24) | (r << 16) | (g << 8) | b;

    if (fill) {
        bl_context_set_fill_style_rgba32(ctx, bl_color.value);
    } else {
        bl_context_set_stroke_style_rgba32(ctx, bl_color.value);
    }
}

static void blend2d_set_gradient_stops(BLGradientCore *gr, const struct gradient_stop *stops, unsigned int stop_count)
{
    for (unsigned int i = 0; i < stop_count; i++) {
        BLRgba32 bl_color;
        colour c = stops[i].color;
        uint8_t r = c & 0xFF;
        uint8_t g = (c >> 8) & 0xFF;
        uint8_t b = (c >> 16) & 0xFF;
        uint8_t a = 255 - ((c >> 24) & 0xFF);
        bl_color.value = (a << 24) | (r << 16) | (g << 8) | b;
        bl_gradient_add_stop_rgba32(gr, (double)stops[i].offset, bl_color.value);
    }
}

static nserror blend2d_plot_clip(const struct redraw_context *ctx, const struct rect *clip)
{
    struct blend2d_context *bl_ctx_wrap = (struct blend2d_context *)ctx->priv;
    BLContextCore *bl_ctx = bl_ctx_wrap->bl_ctx;
    BLRectI bl_rect = { clip->x0, clip->y0, clip->x1 - clip->x0, clip->y1 - clip->y0 };
    bl_context_clip_to_rect_i(bl_ctx, &bl_rect);
    return NSERROR_OK;
}

static nserror blend2d_plot_finalise(const struct redraw_context *ctx)
{
    if (current_path_inited) {
        bl_path_destroy(&current_path);
        current_path_inited = false;
    }
    return NSERROR_OK;
}

static nserror blend2d_plot_path_begin(const struct redraw_context *ctx);
static nserror blend2d_plot_path_move_to(const struct redraw_context *ctx, float x, float y);
static nserror blend2d_plot_path_line_to(const struct redraw_context *ctx, float x, float y);
static nserror blend2d_plot_path_bezier_to(const struct redraw_context *ctx, float x1, float y1, float x2, float y2, float x3, float y3);
static nserror blend2d_plot_path_close(const struct redraw_context *ctx);

static nserror blend2d_plot_arc(const struct redraw_context *ctx, const plot_style_t *pstyle, int x, int y, int radius, int angle1, int angle2)
{
    struct blend2d_context *bl_ctx_wrap = (struct blend2d_context *)ctx->priv;
    BLContextCore *bl_ctx = bl_ctx_wrap->bl_ctx;
    BLPathCore path;
    bl_path_init(&path);

    /* NetSurf: degrees, CCW from horizontal. Blend2D: radians, CW from horizontal.
     * Conversion: negate angles for CW, then to radians. */
    double start_angle = -angle1 * (M_PI / 180.0);
    double end_angle = -angle2 * (M_PI / 180.0);
    double sweep_angle = end_angle - start_angle;

    BLArc bl_arc = { (double)x, (double)y, (double)radius, (double)radius, start_angle, sweep_angle };
    bl_path_add_geometry(&path, BL_GEOMETRY_TYPE_ARC, &bl_arc, NULL, BL_GEOMETRY_DIRECTION_CW);

    if (pstyle->stroke_type != PLOT_OP_TYPE_NONE) {
        static const BLPoint bl_origin = {0, 0};
        blend2d_set_colour(bl_ctx, pstyle->stroke_colour, pstyle->stroke_opacity, false);
        bl_context_set_stroke_width(bl_ctx, plot_style_fixed_to_double(pstyle->stroke_width));
        bl_context_stroke_path_d(bl_ctx, &bl_origin, &path);
    }

    bl_path_reset(&path);
    return NSERROR_OK;
}

static nserror blend2d_plot_disc(const struct redraw_context *ctx, const plot_style_t *pstyle, int x, int y, int radius)
{
    struct blend2d_context *bl_ctx_wrap = (struct blend2d_context *)ctx->priv;
    BLContextCore *bl_ctx = bl_ctx_wrap->bl_ctx;
    BLEllipse bl_ellipse = { (double)x, (double)y, (double)radius, (double)radius };

    if (pstyle->fill_type != PLOT_OP_TYPE_NONE) {
        blend2d_set_colour(bl_ctx, pstyle->fill_colour, pstyle->fill_opacity, true);
        bl_context_fill_geometry(bl_ctx, BL_GEOMETRY_TYPE_ELLIPSE, &bl_ellipse);
    }

    if (pstyle->stroke_type != PLOT_OP_TYPE_NONE) {
        blend2d_set_colour(bl_ctx, pstyle->stroke_colour, pstyle->stroke_opacity, false);
        bl_context_set_stroke_width(bl_ctx, plot_style_fixed_to_double(pstyle->stroke_width));
        bl_context_stroke_geometry(bl_ctx, BL_GEOMETRY_TYPE_ELLIPSE, &bl_ellipse);
    }

    return NSERROR_OK;
}

static nserror blend2d_plot_line(const struct redraw_context *ctx, const plot_style_t *pstyle, const struct rect *line)
{
    struct blend2d_context *bl_ctx_wrap = (struct blend2d_context *)ctx->priv;
    BLContextCore *bl_ctx = bl_ctx_wrap->bl_ctx;
    BLLine bl_line = { (double)line->x0, (double)line->y0, (double)line->x1, (double)line->y1 };

    blend2d_set_colour(bl_ctx, pstyle->stroke_colour, pstyle->stroke_opacity, false);
    bl_context_set_stroke_width(bl_ctx, plot_style_fixed_to_double(pstyle->stroke_width));
    bl_context_stroke_geometry(bl_ctx, BL_GEOMETRY_TYPE_LINE, &bl_line);

    return NSERROR_OK;
}

static nserror blend2d_plot_rectangle(const struct redraw_context *ctx, const plot_style_t *pstyle, const struct rect *rectangle)
{
    struct blend2d_context *bl_ctx_wrap = (struct blend2d_context *)ctx->priv;
    BLContextCore *bl_ctx = bl_ctx_wrap->bl_ctx;
    BLRect bl_rect = { (double)rectangle->x0, (double)rectangle->y0, (double)(rectangle->x1 - rectangle->x0), (double)(rectangle->y1 - rectangle->y0) };

    if (pstyle->fill_type != PLOT_OP_TYPE_NONE) {
        blend2d_set_colour(bl_ctx, pstyle->fill_colour, pstyle->fill_opacity, true);
        bl_context_fill_rect_d(bl_ctx, &bl_rect);
    }

    if (pstyle->stroke_type != PLOT_OP_TYPE_NONE) {
        blend2d_set_colour(bl_ctx, pstyle->stroke_colour, pstyle->stroke_opacity, false);
        bl_context_set_stroke_width(bl_ctx, plot_style_fixed_to_double(pstyle->stroke_width));
        bl_context_stroke_rect_d(bl_ctx, &bl_rect);
    }

    return NSERROR_OK;
}

static nserror blend2d_plot_polygon(const struct redraw_context *ctx, const plot_style_t *pstyle, const int *p, unsigned int n)
{
    struct blend2d_context *bl_ctx_wrap = (struct blend2d_context *)ctx->priv;
    BLContextCore *bl_ctx = bl_ctx_wrap->bl_ctx;
    BLPoint *pts = malloc(sizeof(BLPoint) * n);
    if (!pts) return NSERROR_NOMEM;

    for (unsigned int i = 0; i < n; i++) {
        pts[i].x = (double)p[i * 2];
        pts[i].y = (double)p[i * 2 + 1];
    }

    if (pstyle->fill_type != PLOT_OP_TYPE_NONE) {
        BLArrayView view = { pts, n };
        blend2d_set_colour(bl_ctx, pstyle->fill_colour, pstyle->fill_opacity, true);
        bl_context_fill_geometry(bl_ctx, BL_GEOMETRY_TYPE_POLYGOND, &view);
    }

    free(pts);
    return NSERROR_OK;
}

static nserror blend2d_plot_path_fill(const struct redraw_context *ctx, const plot_style_t *pstyle, const float transform[6]);
static nserror blend2d_plot_path_stroke(const struct redraw_context *ctx, const plot_style_t *pstyle, const float transform[6]);

static nserror blend2d_plot_path(const struct redraw_context *ctx, const plot_style_t *pstyle, const float *p, unsigned int n, const float transform[6])
{
    nserror err;
    if ((err = blend2d_plot_path_begin(ctx)) != NSERROR_OK) return err;

    for (unsigned int i = 0; i < n; ) {
        int cmd = (int)p[i++];
        switch (cmd) {
        case PLOTTER_PATH_MOVE:
            blend2d_plot_path_move_to(ctx, p[i], p[i+1]);
            i += 2;
            break;
        case PLOTTER_PATH_LINE:
            blend2d_plot_path_line_to(ctx, p[i], p[i+1]);
            i += 2;
            break;
        case PLOTTER_PATH_BEZIER:
            blend2d_plot_path_bezier_to(ctx, p[i], p[i+1], p[i+2], p[i+3], p[i+4], p[i+5]);
            i += 6;
            break;
        case PLOTTER_PATH_CLOSE:
            blend2d_plot_path_close(ctx);
            break;
        }
    }

    if (pstyle->fill_type != PLOT_OP_TYPE_NONE) blend2d_plot_path_fill(ctx, pstyle, transform);
    if (pstyle->stroke_type != PLOT_OP_TYPE_NONE) blend2d_plot_path_stroke(ctx, pstyle, transform);

    return NSERROR_OK;
}

static nserror blend2d_plot_path_begin(const struct redraw_context *ctx)
{
    if (!current_path_inited) {
        bl_path_init(&current_path);
        current_path_inited = true;
    }
    bl_path_clear(&current_path);
    return NSERROR_OK;
}

static nserror blend2d_plot_path_move_to(const struct redraw_context *ctx, float x, float y)
{
    bl_path_move_to(&current_path, (double)x, (double)y);
    return NSERROR_OK;
}

static nserror blend2d_plot_path_line_to(const struct redraw_context *ctx, float x, float y)
{
    bl_path_line_to(&current_path, (double)x, (double)y);
    return NSERROR_OK;
}

static nserror blend2d_plot_path_bezier_to(const struct redraw_context *ctx, float x1, float y1, float x2, float y2, float x3, float y3)
{
    bl_path_cubic_to(&current_path, (double)x1, (double)y1, (double)x2, (double)y2, (double)x3, (double)y3);
    return NSERROR_OK;
}

static nserror blend2d_plot_path_close(const struct redraw_context *ctx)
{
    bl_path_close(&current_path);
    return NSERROR_OK;
}

static nserror blend2d_plot_path_fill(const struct redraw_context *ctx, const plot_style_t *pstyle, const float transform[6])
{
    struct blend2d_context *bl_ctx_wrap = (struct blend2d_context *)ctx->priv;
    BLContextCore *bl_ctx = bl_ctx_wrap->bl_ctx;
    static const BLPoint bl_origin = {0, 0};

    bl_context_save(bl_ctx, NULL);
    if (transform) {
        BLMatrix2D m = { (double)transform[0], (double)transform[1], (double)transform[2], (double)transform[3], (double)transform[4], (double)transform[5] };
        bl_context_apply_transform_op(bl_ctx, BL_TRANSFORM_OP_POST_TRANSFORM, &m);
    }

    blend2d_set_colour(bl_ctx, pstyle->fill_colour, pstyle->fill_opacity, true);
    bl_context_fill_path_d(bl_ctx, &bl_origin, &current_path);

    bl_context_restore(bl_ctx, NULL);
    return NSERROR_OK;
}

static nserror blend2d_plot_path_stroke(const struct redraw_context *ctx, const plot_style_t *pstyle, const float transform[6])
{
    struct blend2d_context *bl_ctx_wrap = (struct blend2d_context *)ctx->priv;
    BLContextCore *bl_ctx = bl_ctx_wrap->bl_ctx;
    static const BLPoint bl_origin = {0, 0};

    bl_context_save(bl_ctx, NULL);
    if (transform) {
        BLMatrix2D m = { (double)transform[0], (double)transform[1], (double)transform[2], (double)transform[3], (double)transform[4], (double)transform[5] };
        bl_context_apply_transform_op(bl_ctx, BL_TRANSFORM_OP_POST_TRANSFORM, &m);
    }

    blend2d_set_colour(bl_ctx, pstyle->stroke_colour, pstyle->stroke_opacity, false);
    bl_context_set_stroke_width(bl_ctx, plot_style_fixed_to_double(pstyle->stroke_width));
    bl_context_stroke_path_d(bl_ctx, &bl_origin, &current_path);

    bl_context_restore(bl_ctx, NULL);
    return NSERROR_OK;
}

static nserror blend2d_plot_bitmap(const struct redraw_context *ctx, struct bitmap *bitmap, int x, int y, int width, int height, colour bg, bitmap_flags_t flags)
{
    struct blend2d_context *bl_ctx_wrap = (struct blend2d_context *)ctx->priv;
    BLContextCore *bl_ctx = bl_ctx_wrap->bl_ctx;
    void *pixel_data = guit->bitmap->get_buffer(bitmap);
    int w = guit->bitmap->get_width(bitmap);
    int h = guit->bitmap->get_height(bitmap);
    size_t stride = guit->bitmap->get_rowstride(bitmap);
    bool opaque = guit->bitmap->get_opaque(bitmap);

    BLImageCore img;
    bl_image_init_as_from_data(&img, w, h, opaque ? BL_FORMAT_XRGB32 : BL_FORMAT_PRGB32, pixel_data, (intptr_t)stride, BL_DATA_ACCESS_READ, NULL, NULL);

    BLRect bl_dst_rect = { (double)x, (double)y, (double)width, (double)height };
    BLRectI bl_src_rect = { 0, 0, w, h };

    bl_context_blit_scaled_image_d(bl_ctx, &bl_dst_rect, &img, &bl_src_rect);

    bl_image_destroy(&img);

    return NSERROR_OK;
}

static nserror blend2d_plot_text(const struct redraw_context *ctx, const plot_font_style_t *fstyle, int x, int y, const char *text, size_t length)
{
    struct blend2d_context *bl_ctx_wrap = (struct blend2d_context *)ctx->priv;

    if (bl_ctx_wrap->native_text_handler) {
        /* Use native plotter instead */
        struct redraw_context native_ctx = *ctx;
        native_ctx.priv = bl_ctx_wrap->native_priv;
        return bl_ctx_wrap->native_text_handler(&native_ctx, fstyle, x, y, text, length);
    }

    /* Fallback/Generic Blend2D text rendering could be implemented here using bl_context_fill_utf8_text */
    return NSERROR_NOT_IMPLEMENTED;
}

static nserror blend2d_push_transform(const struct redraw_context *ctx, const float transform[6])
{
    struct blend2d_context *bl_ctx_wrap = (struct blend2d_context *)ctx->priv;
    BLContextCore *bl_ctx = bl_ctx_wrap->bl_ctx;
    bl_context_save(bl_ctx, NULL);
    BLMatrix2D m = { (double)transform[0], (double)transform[1], (double)transform[2], (double)transform[3], (double)transform[4], (double)transform[5] };
    bl_context_apply_transform_op(bl_ctx, BL_TRANSFORM_OP_POST_TRANSFORM, &m);
    return NSERROR_OK;
}

static nserror blend2d_pop_transform(const struct redraw_context *ctx)
{
    struct blend2d_context *bl_ctx_wrap = (struct blend2d_context *)ctx->priv;
    BLContextCore *bl_ctx = bl_ctx_wrap->bl_ctx;
    bl_context_restore(bl_ctx, NULL);
    return NSERROR_OK;
}

static nserror blend2d_plot_linear_gradient(const struct redraw_context *ctx, const float *p, unsigned int n,
    const float transform[6], float x0, float y0, float x1, float y1, const struct gradient_stop *stops,
    unsigned int stop_count)
{
    struct blend2d_context *bl_ctx_wrap = (struct blend2d_context *)ctx->priv;
    BLContextCore *bl_ctx = bl_ctx_wrap->bl_ctx;
    BLGradientCore gr;
    BLLinearGradientValues values = { (double)x0, (double)y0, (double)x1, (double)y1 };
    static const BLPoint bl_origin = {0, 0};

    bl_gradient_init_as(&gr, BL_GRADIENT_TYPE_LINEAR, &values, BL_EXTEND_MODE_PAD, NULL, 0, NULL);
    blend2d_set_gradient_stops(&gr, stops, stop_count);

    BLPathCore path;
    bl_path_init(&path);
    for (unsigned int i = 0; i < n; ) {
        int cmd = (int)p[i++];
        switch (cmd) {
        case PLOTTER_PATH_MOVE:
            bl_path_move_to(&path, (double)p[i], (double)p[i+1]);
            i += 2;
            break;
        case PLOTTER_PATH_LINE:
            bl_path_line_to(&path, (double)p[i], (double)p[i+1]);
            i += 2;
            break;
        case PLOTTER_PATH_BEZIER:
            bl_path_cubic_to(&path, (double)p[i], (double)p[i+1], (double)p[i+2], (double)p[i+3], (double)p[i+4], (double)p[i+5]);
            i += 6;
            break;
        case PLOTTER_PATH_CLOSE:
            bl_path_close(&path);
            break;
        }
    }

    bl_context_save(bl_ctx, NULL);
    if (transform) {
        BLMatrix2D m = { (double)transform[0], (double)transform[1], (double)transform[2], (double)transform[3], (double)transform[4], (double)transform[5] };
        bl_context_apply_transform_op(bl_ctx, BL_TRANSFORM_OP_POST_TRANSFORM, &m);
    }

    bl_context_set_fill_style(bl_ctx, &gr);
    bl_context_fill_path_d(bl_ctx, &bl_origin, &path);

    bl_context_restore(bl_ctx, NULL);
    bl_path_reset(&path);
    bl_gradient_reset(&gr);

    return NSERROR_OK;
}

static nserror blend2d_plot_radial_gradient(const struct redraw_context *ctx, const float *p, unsigned int n,
    const float transform[6], float cx, float cy, float rx, float ry, const struct gradient_stop *stops,
    unsigned int stop_count)
{
    struct blend2d_context *bl_ctx_wrap = (struct blend2d_context *)ctx->priv;
    BLContextCore *bl_ctx = bl_ctx_wrap->bl_ctx;
    BLGradientCore gr;
    /* Blend2D radial is centered at (x1, y1) with focal point at (x0, y0). We use cx, cy for both. */
    BLRadialGradientValues values = { (double)cx, (double)cy, (double)cx, (double)cy, (double)rx };
    static const BLPoint bl_origin = {0, 0};

    bl_gradient_init_as(&gr, BL_GRADIENT_TYPE_RADIAL, &values, BL_EXTEND_MODE_PAD, NULL, 0, NULL);
    blend2d_set_gradient_stops(&gr, stops, stop_count);

    if (rx != ry && rx > 0) {
        /* If radii are different, we apply a scaling transform to the gradient style */
        BLMatrix2D m;
        bl_matrix2d_set_identity(&m);
        double tl_data[2] = {(double)cx, (double)cy};
        bl_matrix2d_apply_op(&m, BL_TRANSFORM_OP_TRANSLATE, tl_data);
        double sc_data[2] = {1.0, (double)ry / (double)rx};
        bl_matrix2d_apply_op(&m, BL_TRANSFORM_OP_SCALE, sc_data);
        double ntl_data[2] = {-(double)cx, -(double)cy};
        bl_matrix2d_apply_op(&m, BL_TRANSFORM_OP_TRANSLATE, ntl_data);
        bl_gradient_apply_transform_op(&gr, BL_TRANSFORM_OP_POST_TRANSFORM, &m);
    }

    BLPathCore path;
    bl_path_init(&path);
    for (unsigned int i = 0; i < n; ) {
        int cmd = (int)p[i++];
        switch (cmd) {
        case PLOTTER_PATH_MOVE:
            bl_path_move_to(&path, (double)p[i], (double)p[i+1]);
            i += 2;
            break;
        case PLOTTER_PATH_LINE:
            bl_path_line_to(&path, (double)p[i], (double)p[i+1]);
            i += 2;
            break;
        case PLOTTER_PATH_BEZIER:
            bl_path_cubic_to(&path, (double)p[i], (double)p[i+1], (double)p[i+2], (double)p[i+3], (double)p[i+4], (double)p[i+5]);
            i += 6;
            break;
        case PLOTTER_PATH_CLOSE:
            bl_path_close(&path);
            break;
        }
    }

    bl_context_save(bl_ctx, NULL);
    if (transform) {
        BLMatrix2D m = { (double)transform[0], (double)transform[1], (double)transform[2], (double)transform[3], (double)transform[4], (double)transform[5] };
        bl_context_apply_transform_op(bl_ctx, BL_TRANSFORM_OP_POST_TRANSFORM, &m);
    }

    bl_context_set_fill_style(bl_ctx, &gr);
    bl_context_fill_path_d(bl_ctx, &bl_origin, &path);

    bl_context_restore(bl_ctx, NULL);
    bl_path_reset(&path);
    bl_gradient_reset(&gr);

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
    .linear_gradient = blend2d_plot_linear_gradient,
    .radial_gradient = blend2d_plot_radial_gradient,
    .finalise = blend2d_plot_finalise,
    .option_knockout = true
};
