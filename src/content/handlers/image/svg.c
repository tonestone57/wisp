/*
 * Copyright 2007-2008 James Bursa <bursa@users.sourceforge.net>
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
 * implementation of content for image/svg using libsvgtiny.
 */

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <svgtiny.h>

#include <wisp/content.h>
#include <wisp/content/content_protected.h>
#include <wisp/desktop/gui_internal.h>
#include <wisp/layout.h>
#include <wisp/plotters.h>
#include <wisp/utils/log.h>
#include <wisp/utils/messages.h>
#include <wisp/utils/nsurl.h>
#include <wisp/utils/utils.h>
#include "content/content_factory.h"

#include "content/handlers/image/svg.h"

/**
 * Render a dashed line as a series of filled rectangles.
 *
 * This provides cross-platform dash rendering that works on ALL backends
 * (Qt, GTK, BeOS, framebuffer, Windows) without relying on platform-specific
 * dash pattern APIs.
 *
 * Only handles horizontal and vertical lines for now - diagonal lines
 * would require rotated rectangles which most plotters don't support.
 *
 * \param ctx          Redraw context
 * \param stroke_colour Stroke colour for the dashes
 * \param x1,y1        Start of line
 * \param x2,y2        End of line
 * \param stroke_width Width of the stroke
 * \param dasharray    Array of dash/gap lengths (scaled to display coords)
 * \param dasharray_count Number of elements in dasharray
 * \param dashoffset   Offset into the dash pattern
 * \param transform    Affine transform to apply
 * \return NSERROR_OK on success
 */
static nserror svg_plot_dashed_line_as_rects(const struct redraw_context *ctx, colour stroke_colour, float x1, float y1,
    float x2, float y2, float stroke_width, const float *dasharray, unsigned int dasharray_count, float dashoffset,
    const float transform[6])
{
    if (dasharray == NULL || dasharray_count == 0) {
        return NSERROR_INVALID;
    }

    /* Apply transform to line endpoints */
    float tx1 = x1 * transform[0] + y1 * transform[2] + transform[4];
    float ty1 = x1 * transform[1] + y1 * transform[3] + transform[5];
    float tx2 = x2 * transform[0] + y2 * transform[2] + transform[4];
    float ty2 = x2 * transform[1] + y2 * transform[3] + transform[5];

    /* Calculate line length and direction */
    float dx = tx2 - tx1;
    float dy = ty2 - ty1;
    float line_length = sqrtf(dx * dx + dy * dy);

    if (line_length < 1.0f) {
        return NSERROR_OK; /* Line too short */
    }

    /* Normalize direction */
    float ndx = dx / line_length;
    float ndy = dy / line_length;

    /* Perpendicular direction for stroke width */
    float px = -ndy * stroke_width / 2.0f;
    float py = ndx * stroke_width / 2.0f;

    /* Set up fill style for rectangles */
    plot_style_t fill_style = {
        .fill_type = PLOT_OP_TYPE_SOLID, .fill_colour = stroke_colour, .stroke_type = PLOT_OP_TYPE_NONE};

    /* Calculate total pattern length */
    float pattern_length = 0.0f;
    for (unsigned int i = 0; i < dasharray_count; i++) {
        pattern_length += dasharray[i];
    }
    if (pattern_length < 1.0f) {
        return NSERROR_OK; /* Pattern too short */
    }

    /* Start position, accounting for dash offset */
    float pos = -fmodf(dashoffset, pattern_length);
    if (pos > 0)
        pos -= pattern_length;

    unsigned int dash_idx = 0;
    bool draw_dash = true; /* Alternate between dash (draw) and gap (skip) */

    while (pos < line_length) {
        float dash_len = dasharray[dash_idx % dasharray_count];
        float dash_start = pos;
        float dash_end = pos + dash_len;

        /* Clamp to line bounds */
        if (dash_start < 0)
            dash_start = 0;
        if (dash_end > line_length)
            dash_end = line_length;

        /* Only draw if this is a dash (not a gap) and within bounds */
        if (draw_dash && dash_end > dash_start && dash_start < line_length) {
            /* Calculate rectangle corners */
            float sx = tx1 + ndx * dash_start;
            float sy = ty1 + ndy * dash_start;
            float ex = tx1 + ndx * dash_end;
            float ey = ty1 + ndy * dash_end;

            /* For horizontal lines (dy ≈ 0), create axis-aligned rect */
            if (fabsf(dy) < 0.01f) {
                struct rect r;
                r.x0 = (int)(fminf(sx, ex));
                r.y0 = (int)(sy - stroke_width / 2.0f);
                r.x1 = (int)(fmaxf(sx, ex));
                r.y1 = (int)(sy + stroke_width / 2.0f);
                ctx->plot->rectangle(ctx, &fill_style, &r);
            }
            /* For vertical lines (dx ≈ 0), create axis-aligned rect */
            else if (fabsf(dx) < 0.01f) {
                struct rect r;
                r.x0 = (int)(sx - stroke_width / 2.0f);
                r.y0 = (int)(fminf(sy, ey));
                r.x1 = (int)(sx + stroke_width / 2.0f);
                r.y1 = (int)(fmaxf(sy, ey));
                ctx->plot->rectangle(ctx, &fill_style, &r);
            }
            /* Diagonal lines - skip for now (would need polygon or rotated rect) */
        }

        pos += dash_len;
        dash_idx++;
        draw_dash = !draw_dash;
    }

    return NSERROR_OK;
}

/**
 * Render a gradient fill for an SVG shape using native gradient APIs.
 *
 * This is called for shapes that have fill_gradient_type != svgtiny_GRADIENT_NONE.
 * Uses native gradient plotting when available, otherwise skips (shape would have
 * been pre-rendered as triangles in older libsvgtiny versions).
 *
 * \param ctx          Redraw context
 * \param shape        SVG shape with gradient fill
 * \param path         Path data (float array with path commands)
 * \param path_len     Number of elements in path array
 * \param bbox         Bounding box in display coordinates (for clipping)
 * \param sx, sy       Scale factors (display/intrinsic)
 * \param transform    Affine transform to apply
 * \return NSERROR_OK on success, error code otherwise
 */
static nserror svg_plot_gradient_fill(const struct redraw_context *ctx, const struct svgtiny_shape *shape,
    const float *path, unsigned int path_len, const struct rect *bbox, float sx, float sy, const float transform[6])
{
#ifdef WISP_USE_NATIVE_GRADIENTS
    if (shape->fill_gradient_type == svgtiny_GRADIENT_NONE) {
        return NSERROR_OK;
    }

    NSLOG(wisp, DEEPDEBUG, "SVG gradient: Using NATIVE rendering path for %s gradient",
        shape->fill_gradient_type == svgtiny_GRADIENT_LINEAR ? "linear" : "radial");

    if (ctx->plot->linear_gradient == NULL && shape->fill_gradient_type == svgtiny_GRADIENT_LINEAR) {
        NSLOG(wisp, WARNING, "SVG gradient: Native linear_gradient plotter is NULL!");
        return NSERROR_NOT_IMPLEMENTED;
    }

    if (ctx->plot->radial_gradient == NULL && shape->fill_gradient_type == svgtiny_GRADIENT_RADIAL) {
        NSLOG(wisp, WARNING, "SVG gradient: Native radial_gradient plotter is NULL!");
        return NSERROR_NOT_IMPLEMENTED;
    }

    /* Convert gradient stops from SVG format to plotter format */
    struct gradient_stop *stops = malloc(shape->fill_grad_stop_count * sizeof(struct gradient_stop));
    if (stops == NULL) return NSERROR_NOMEM;
    for (unsigned int i = 0; i < shape->fill_grad_stop_count; i++) {
        /* Convert svgtiny RGB color to neosurf color format (BGR) */
        svgtiny_colour c = shape->fill_grad_stops[i].color;
        stops[i].color = (svgtiny_RED(c)) | (svgtiny_GREEN(c) << 8) | (svgtiny_BLUE(c) << 16);
        stops[i].offset = shape->fill_grad_stops[i].offset;
    }

#ifdef SVG_GRADIENT_BBOX_CLIP
    /* Optional: Set clip to the shape's bounding box. */
    ctx->plot->clip(ctx, bbox);
#else
    (void)bbox; /* Unused when not clipping to bbox */
#endif

    /* Create scaled path for the old gradient API */
    float *scaled_path = malloc(sizeof(float) * path_len);
    if (!scaled_path) {
        free(stops);
        return NSERROR_NOMEM;
    }
    unsigned int k = 0, jj = 0;
    while (jj < path_len) {
        int cmd = (int)path[jj++];
        scaled_path[k++] = (float)cmd;
        if (cmd == PLOTTER_PATH_MOVE || cmd == PLOTTER_PATH_LINE) {
            scaled_path[k++] = path[jj++] * sx;
            scaled_path[k++] = path[jj++] * sy;
        } else if (cmd == PLOTTER_PATH_BEZIER) {
            scaled_path[k++] = path[jj++] * sx;
            scaled_path[k++] = path[jj++] * sy;
            scaled_path[k++] = path[jj++] * sx;
            scaled_path[k++] = path[jj++] * sy;
            scaled_path[k++] = path[jj++] * sx;
            scaled_path[k++] = path[jj++] * sy;
        }
    }

    /* Scale gradient coordinates to match path space (scaled but not translated).
     * The transform is applied during rendering by the plotter. */
    float gx1 = shape->fill_grad_x1 * sx;
    float gy1 = shape->fill_grad_y1 * sy;
    float gx2 = shape->fill_grad_x2 * sx;
    float gy2 = shape->fill_grad_y2 * sy;

    float grad_trans[6];
    bool has_grad_trans = (shape->fill_grad_transform[0] != 1.0f || shape->fill_grad_transform[1] != 0.0f ||
                           shape->fill_grad_transform[2] != 0.0f || shape->fill_grad_transform[3] != 1.0f ||
                           shape->fill_grad_transform[4] != 0.0f || shape->fill_grad_transform[5] != 0.0f);
    if (has_grad_trans) {
        float g_sx[6] = {
            shape->fill_grad_transform[0],
            shape->fill_grad_transform[1],
            shape->fill_grad_transform[2],
            shape->fill_grad_transform[3],
            shape->fill_grad_transform[4] * sx,
            shape->fill_grad_transform[5] * sy
        };
        grad_trans[0] = transform[0] * g_sx[0] + transform[2] * g_sx[1];
        grad_trans[1] = transform[1] * g_sx[0] + transform[3] * g_sx[1];
        grad_trans[2] = transform[0] * g_sx[2] + transform[2] * g_sx[3];
        grad_trans[3] = transform[1] * g_sx[2] + transform[3] * g_sx[3];
        grad_trans[4] = transform[0] * g_sx[4] + transform[2] * g_sx[5] + transform[4];
        grad_trans[5] = transform[1] * g_sx[4] + transform[3] * g_sx[5] + transform[5];
    } else {
        memcpy(grad_trans, transform, sizeof(grad_trans));
    }

    nserror err;
    if (shape->fill_gradient_type == svgtiny_GRADIENT_LINEAR) {
        NSLOG(wisp, DEEPDEBUG,
            "SVG gradient: Calling native linear plotter (%.1f,%.1f) to (%.1f,%.1f) with %u stops, path_len=%u", gx1,
            gy1, gx2, gy2, shape->fill_grad_stop_count, path_len);
        err = ctx->plot->linear_gradient(
            ctx, scaled_path, k, grad_trans, gx1, gy1, gx2, gy2, stops, shape->fill_grad_stop_count);
    } else {
        /* Radial gradient: fill_grad_x1,y1 = center, fill_grad_x2,y2 = radii
         * Scale to match path space (scaled but not translated). */
        float cx = shape->fill_grad_x1 * sx;
        float cy = shape->fill_grad_y1 * sy;
        float rx = shape->fill_grad_x2 * sx;
        float ry = shape->fill_grad_y2 * sy;
        NSLOG(wisp, DEEPDEBUG,
            "SVG gradient: Calling native radial plotter (%.1f,%.1f) rx=%.1f ry=%.1f with %u stops, path_len=%u", cx,
            cy, rx, ry, shape->fill_grad_stop_count, path_len);
        err = ctx->plot->radial_gradient(
            ctx, scaled_path, k, grad_trans, cx, cy, rx, ry, stops, shape->fill_grad_stop_count);
    }

    if (err == NSERROR_OK) {
        NSLOG(wisp, DEEPDEBUG, "SVG gradient: Native plotter succeeded");
    } else {
        NSLOG(wisp, WARNING, "SVG gradient: Native plotter FAILED with error %d", err);
    }

    free(scaled_path);
    free(stops);
    return err;
#else
    NSLOG(wisp, DEEPDEBUG, "SVG gradient: Native gradients DISABLED at compile time, using fallback");
    /* Native gradients disabled - nothing to do (triangle fallback not implemented here) */
    (void)ctx;
    (void)shape;
    (void)path;
    (void)path_len;
    (void)bbox;
    (void)sx;
    (void)sy;
    (void)transform;
    return NSERROR_NOT_IMPLEMENTED;
#endif
}



typedef struct svg_content {
    struct content base;

    struct svgtiny_diagram *diagram;

    bool parsed; /**< True if SVG has been parsed at least once */
    bool has_intrinsic_dimensions; /**< True if SVG has explicit width/height attrs */
    int ratio_width; /**< viewBox/intrinsic width for aspect ratio */
    int ratio_height; /**< viewBox/intrinsic height for aspect ratio */
} svg_content;


static nserror svg_create_svg_data(svg_content *c)
{
    c->diagram = svgtiny_create();
    if (c->diagram == NULL)
        goto no_memory;

    c->parsed = false;

    return NSERROR_OK;

no_memory:
    content_broadcast_error(&c->base, NSERROR_NOMEM, NULL);
    return NSERROR_NOMEM;
}


/**
 * Create a CONTENT_SVG.
 */

static nserror svg_create(const content_handler *handler, lwc_string *imime_type, const struct http_parameter *params,
    struct llcache_handle *llcache, const char *fallback_charset, bool quirks, struct content **c)
{
    svg_content *svg;
    nserror error;

    svg = calloc(1, sizeof(svg_content));
    if (svg == NULL)
        return NSERROR_NOMEM;

    error = content__init(&svg->base, handler, imime_type, params, llcache, fallback_charset, quirks);
    if (error != NSERROR_OK) {
        free(svg);
        return error;
    }

    error = svg_create_svg_data(svg);
    if (error != NSERROR_OK) {
        free(svg);
        return error;
    }

    *c = (struct content *)svg;

    return NSERROR_OK;
}


/**
 * Convert a CONTENT_SVG for display.
 */

static bool svg_convert(struct content *c)
{
    svg_content *svg = (svg_content *)c;
    const uint8_t *source_data;
    size_t source_size;
    int intrinsic_width = 0, intrinsic_height = 0;

    assert(svg->diagram);

    /* Extract intrinsic dimensions from SVG width/height/viewBox attributes.
     * This lightweight parse only reads the header, not shapes.
     * This is needed so that layout can calculate aspect ratio for images
     * with width=100% height=auto before reformat is called. */
    source_data = content__get_source_data(c, &source_size);
    if (source_data != NULL && source_size > 0) {
        svgtiny_dimension_source dim_source = svgtiny_DIMS_DEFAULT;
        svgtiny_parse_dimensions(
            (const char *)source_data, source_size, &intrinsic_width, &intrinsic_height, &dim_source);

        /* Always store dimensions for the rendering pipeline.
         * c->width/c->height are needed by svg_reformat and
         * svg_redraw_internal for correct CTM scaling. */
        c->width = intrinsic_width;
        c->height = intrinsic_height;

        /* Store whether SVG has real intrinsic dimensions or only a
         * viewBox-derived ratio.  Only svgtiny_DIMS_VIEWBOX triggers
         * the ratio-only path; explicit and default (no viewBox
         * either) both keep intrinsic dimensions as-is.
         *
         * AG: Forcing has_intrinsic_dimensions = true even for VIEWBOX
         * because layout seems to ignore the SVG otherwise when width/height are auto.
         * The viewBox dimensions (200x100) serve as a valid intrinsic size.
         */
        if (dim_source == svgtiny_DIMS_VIEWBOX) {
            /* viewBox-only: treat as intrinsic for layout default size */
            svg->has_intrinsic_dimensions = true;
            svg->ratio_width = intrinsic_width;
            svg->ratio_height = intrinsic_height;
        } else {
            /* Explicit width/height or default (300×150) */
            svg->has_intrinsic_dimensions = true;
            svg->ratio_width = 0;
            svg->ratio_height = 0;
        }
    }

    NSLOG(wisp, WARNING,
        "SVGDIAG svg_convert: url=%s c->width=%d c->height=%d "
        "has_intrinsic=%d ratio=%dx%d",
        nsurl_access(content_get_url(c)), c->width, c->height, svg->has_intrinsic_dimensions, svg->ratio_width,
        svg->ratio_height);

    content_set_ready(c);
    content_set_done(c);
    /* Done: update status bar */
    content_set_status(c, "");

    return true;
}

/**
 * Reformat a CONTENT_SVG.
 */

static void svg_reformat(struct content *c, int width, int height)
{
    svg_content *svg = (svg_content *)c;
    const uint8_t *source_data;
    size_t source_size;

    assert(svg->diagram);

    /* Skip reformat if dimensions are unknown (0x0).
     * We can't do a meaningful parse without knowing the target viewport.
     * Intrinsic dimensions are already available from svg_convert via
     * svgtiny_parse_dimensions(), so we just wait for layout to call
     * again with real dimensions.  Fix 1 (shape clearing in svgtiny_parse)
     * ensures any re-parse starts clean. */
    if (width <= 0 || height <= 0 || width >= 100000000 || height >= 100000000 || width == INT_MIN || height == INT_MIN) {
        NSLOG(wisp, DEBUG, "SVG reformat skipped: dimensions unknown or invalid (%dx%d)", width, height);
        return;
    }

    /* Parse the SVG at the viewport dimensions.
     * libsvgtiny bakes the CTM into shape coordinates and stroke widths,
     * so we need to parse at the actual display size for correct scaling. */
    NSLOG(wisp, DEBUG, "SVG parsing with viewport: %dx%d", width, height);
    source_data = content__get_source_data(c, &source_size);

    /* For viewBox-only SVGs (no explicit width/height), parse at the
     * viewBox dimensions rather than the display viewport.  svgtiny_parse
     * resolves percentage values relative to the viewport passed in,
     * and the viewBox CTM also scales them.  Using viewBox dimensions
     * gives a CTM of identity (viewBox/viewport = 1:1), avoiding
     * double-scaling.  svg_redraw_internal handles display scaling. */
    svgtiny_code code = svgtiny_parse(
        svg->diagram, (const char *)source_data, source_size, nsurl_access(content_get_url(c)), width, height);

    NSLOG(wisp, DEBUG, "svg_reformat: url=%s w=%d h=%d code=%d diag_w=%u diag_h=%u shapes=%u",
        nsurl_access(content_get_url(c)), width, height, code, svg->diagram ? svg->diagram->width : 0,
        svg->diagram ? svg->diagram->height : 0, svg->diagram ? svg->diagram->shape_count : 0);

    if (code == svgtiny_OK) {
        /* Wisp SVG Parse Flag Transition: Set the parsed flag to true upon
         * successful parsing of the SVG diagram. This critical state change
         * ensures that subsequent redraw and layout loops correctly identify
         * that the SVG has been parsed and do not skip rendering or layout
         * calculation.
         */
        svg->parsed = true;
    } else if (code == svgtiny_SVG_ERROR) {
        /* Allow partial SVGs (e.g. ones with unsupported gradients) to render
         * anyway instead of showing nothing.
         */
        svg->parsed = true;
    }

    if (svg->diagram->width > 0 && svg->diagram->height > 0) {
        c->width = svg->diagram->width;
        c->height = svg->diagram->height;
    }
}


/**
 * Redraw a CONTENT_SVG.
 */

static bool svg_redraw_internal(svg_content *svg, int x, int y, int width, int height, const struct rect *clip,
    const struct redraw_context *ctx, colour background_colour, colour current_color)
{
    float transform[6];
    struct svgtiny_diagram *diagram = svg->diagram;
    unsigned int i;
    plot_font_style_t fstyle = *plot_style_font;
    nserror res;
    bool ok = true;
    nsurl *content_url = content_get_url(&svg->base);
    const char *url_str = content_url ? nsurl_access(content_url) : "(inline)";

    assert(diagram);

    int intrinsic_w = svg->base.width;
    int intrinsic_h = svg->base.height;
    int parse_w = intrinsic_w;
    int parse_h = intrinsic_h;
    if (svg->diagram->width > 0 && svg->diagram->height > 0) {
        parse_w = svg->diagram->width;
        parse_h = svg->diagram->height;
    }
    float sx = (float)width / (float)parse_w;
    float sy = (float)height / (float)parse_h;
    float stroke_scale = (sx + sy) / 2.0f;

    transform[0] = 1.0f; transform[1] = 0.0f;
    transform[2] = 0.0f; transform[3] = 1.0f;
    transform[4] = (float)x;    transform[5] = (float)y;

    if (current_color == 0) {
        current_color = 0x00ffffff;
    }

    NSLOG(wisp, DEBUG, "PROFILER: START SVG rendering %p", svg);

#define BGR(c) (((svgtiny_RED((c))) | (svgtiny_GREEN((c)) << 8) | (svgtiny_BLUE((c)) << 16)))

    /* Force use_stateful = false to use the non-stateful path batching.
     * This avoids the Cairo sequence bug where changing CTM in path_fill/stroke
     * has no effect on already constructed paths. */
    bool use_stateful = false;

    /* Batching state */
    bool batch_active = false;
    plot_style_t batch_style;
    float *batch_path = NULL;
    unsigned int batch_path_len = 0;
    unsigned int batch_path_alloc = 0;

    #define FLUSH_BATCH() do { \
        if (batch_active) { \
            if (use_stateful) { \
                if (batch_style.fill_type != PLOT_OP_TYPE_NONE) ctx->plot->path_fill(ctx, &batch_style, transform); \
                if (batch_style.stroke_type != PLOT_OP_TYPE_NONE) ctx->plot->path_stroke(ctx, &batch_style, transform); \
            } else { \
                if (batch_style.fill_type != PLOT_OP_TYPE_NONE) { \
                    plot_style_t _fs = batch_style; _fs.stroke_type = PLOT_OP_TYPE_NONE; \
                    ctx->plot->path(ctx, &_fs, batch_path, batch_path_len, transform); \
                } \
                if (batch_style.stroke_type != PLOT_OP_TYPE_NONE) { \
                    plot_style_t _ss = batch_style; _ss.fill_type = PLOT_OP_TYPE_NONE; \
                    ctx->plot->path(ctx, &_ss, batch_path, batch_path_len, transform); \
                } \
            } \
            batch_active = false; \
            batch_path_len = 0; \
        } \
    } while(0)

    for (i = 0; i != diagram->shape_count; i++) {
        if (diagram->shape[i].path) {
            plot_style_t current_pstyle;
            memset(&current_pstyle, 0, sizeof(plot_style_t));
            svgtiny_colour stroke_c = diagram->shape[i].stroke;
            if (stroke_c == svgtiny_CURRENT_COLOR) {
                current_pstyle.stroke_type = PLOT_OP_TYPE_SOLID;
                current_pstyle.stroke_colour = current_color;
            } else if (stroke_c == svgtiny_TRANSPARENT) {
                current_pstyle.stroke_type = PLOT_OP_TYPE_NONE;
                current_pstyle.stroke_colour = NS_TRANSPARENT;
            } else {
                current_pstyle.stroke_type = PLOT_OP_TYPE_SOLID;
                current_pstyle.stroke_colour = BGR(stroke_c);
            }
            float sw = (float)diagram->shape[i].stroke_width * stroke_scale;
            if (diagram->shape[i].stroke_width > 0 && sw < 1.0f) sw = 1.0f;
            current_pstyle.stroke_width = (plot_style_fixed)(sw * PLOT_STYLE_SCALE);

            bool has_dash = (diagram->shape[i].stroke_dasharray_set && diagram->shape[i].stroke_dasharray_count > 0);

            if (has_dash) {
                current_pstyle.stroke_dasharray = diagram->shape[i].stroke_dasharray;
                current_pstyle.stroke_dasharray_count = diagram->shape[i].stroke_dasharray_count;
                current_pstyle.stroke_dashoffset = diagram->shape[i].stroke_dashoffset * stroke_scale;
            }

            svgtiny_colour fill_c = diagram->shape[i].fill;
            if (fill_c == svgtiny_CURRENT_COLOR) {
                current_pstyle.fill_type = PLOT_OP_TYPE_SOLID;
                current_pstyle.fill_colour = current_color;
            } else if (fill_c == svgtiny_TRANSPARENT) {
                current_pstyle.fill_type = PLOT_OP_TYPE_NONE;
                current_pstyle.fill_colour = NS_TRANSPARENT;
            } else {
                current_pstyle.fill_type = PLOT_OP_TYPE_SOLID;
                current_pstyle.fill_colour = BGR(fill_c);
            }
            current_pstyle.fill_opacity = diagram->shape[i].fill_opacity_set ? diagram->shape[i].fill_opacity : 1.0f;
            current_pstyle.stroke_opacity = diagram->shape[i].stroke_opacity_set ? diagram->shape[i].stroke_opacity : 1.0f;

            bool can_batch = !has_dash && (diagram->shape[i].fill_gradient_type == svgtiny_GRADIENT_NONE);

            if (batch_active && (!can_batch ||
                memcmp(&batch_style, &current_pstyle, sizeof(plot_style_t)) != 0)) {
                FLUSH_BATCH();
            }

            if (!batch_active && can_batch) {
                batch_active = true;
                batch_style = current_pstyle;
                if (use_stateful) ctx->plot->path_begin(ctx);
            }

            if (can_batch) {
                unsigned int j = 0;
                if (!use_stateful) {
                    if (batch_path_len + diagram->shape[i].path_length > batch_path_alloc) {
                        batch_path_alloc = (batch_path_len + diagram->shape[i].path_length) * 2;
                        float *nb = realloc(batch_path, sizeof(float) * batch_path_alloc);
                        if (!nb) { ok = false; break; }
                        batch_path = nb;
                    }
                }
                while (j < diagram->shape[i].path_length) {
                    int cmd = (int)diagram->shape[i].path[j++];
                    if (!use_stateful) batch_path[batch_path_len++] = (float)cmd;
                    if (cmd == PLOTTER_PATH_MOVE || cmd == PLOTTER_PATH_LINE) {
                        float xx = diagram->shape[i].path[j++] * sx; float yy = diagram->shape[i].path[j++] * sy;
                        if (use_stateful) {
                            if (cmd == PLOTTER_PATH_MOVE) ctx->plot->path_move_to(ctx, xx, yy);
                            else ctx->plot->path_line_to(ctx, xx, yy);
                        } else {
                            batch_path[batch_path_len++] = xx; batch_path[batch_path_len++] = yy;
                        }
                    } else if (cmd == PLOTTER_PATH_BEZIER) {
                        float x1 = diagram->shape[i].path[j++] * sx; float y1 = diagram->shape[i].path[j++] * sy;
                        float x2 = diagram->shape[i].path[j++] * sx; float y2 = diagram->shape[i].path[j++] * sy;
                        float x3 = diagram->shape[i].path[j++] * sx; float y3 = diagram->shape[i].path[j++] * sy;
                        if (use_stateful) ctx->plot->path_bezier_to(ctx, x1, y1, x2, y2, x3, y3);
                        else {
                            batch_path[batch_path_len++] = x1; batch_path[batch_path_len++] = y1;
                            batch_path[batch_path_len++] = x2; batch_path[batch_path_len++] = y2;
                            batch_path[batch_path_len++] = x3; batch_path[batch_path_len++] = y3;
                        }
                    } else if (cmd == PLOTTER_PATH_CLOSE) {
                        if (use_stateful) ctx->plot->path_close(ctx);
                    }
                }
            } else {
                FLUSH_BATCH();
                if (diagram->shape[i].fill_gradient_type != svgtiny_GRADIENT_NONE) {
                    float minx = 0.0f, miny = 0.0f, maxx = 0.0f, maxy = 0.0f;
                    unsigned int jj = 0; int initbb = 0;
                    while (jj < diagram->shape[i].path_length) {
                        int cmd = (int)diagram->shape[i].path[jj++];
                        if (cmd == PLOTTER_PATH_MOVE || cmd == PLOTTER_PATH_LINE) {
                            float xx = diagram->shape[i].path[jj++] * sx; float yy = diagram->shape[i].path[jj++] * sy;
                            if (!initbb) { minx = maxx = xx; miny = maxy = yy; initbb = 1; }
                            if (xx < minx) minx = xx; if (xx > maxx) maxx = xx; if (yy < miny) miny = yy; if (yy > maxy) maxy = yy;
                        } else if (cmd == PLOTTER_PATH_BEZIER) {
                            for (int k = 0; k < 3; k++) {
                                float xx = diagram->shape[i].path[jj++] * sx; float yy = diagram->shape[i].path[jj++] * sy;
                                if (!initbb) { minx = maxx = xx; miny = maxy = yy; initbb = 1; }
                                if (xx < minx) minx = xx; if (xx > maxx) maxx = xx; if (yy < miny) miny = yy; if (yy > maxy) maxy = yy;
                            }
                        }
                    }
                    struct rect grad_clip = {(int)floorf(minx + (float)x), (int)floorf(miny + (float)y), (int)ceilf(maxx + (float)x), (int)ceilf(maxy + (float)y)};
                    svg_plot_gradient_fill(ctx, &diagram->shape[i], diagram->shape[i].path, diagram->shape[i].path_length, &grad_clip, sx, sy, transform);
                } else if (has_dash) {
                    if ((diagram->shape[i].path_length == 6 || diagram->shape[i].path_length == 7) &&
                        (int)diagram->shape[i].path[0] == PLOTTER_PATH_MOVE && (int)diagram->shape[i].path[3] == PLOTTER_PATH_LINE) {
                        float x1 = diagram->shape[i].path[1] * sx; float y1 = diagram->shape[i].path[2] * sy;
                        float x2 = diagram->shape[i].path[4] * sx; float y2 = diagram->shape[i].path[5] * sy;
                        static float sd[16];
                        for (unsigned int d = 0; d < diagram->shape[i].stroke_dasharray_count && d < 16; d++)
                            sd[d] = diagram->shape[i].stroke_dasharray[d] * stroke_scale;
                        svg_plot_dashed_line_as_rects(ctx, current_pstyle.stroke_colour, x1, y1, x2, y2, sw, sd, diagram->shape[i].stroke_dasharray_count, diagram->shape[i].stroke_dashoffset * stroke_scale, transform);
                    } else {
                        if (use_stateful) {
                            ctx->plot->path_begin(ctx);
                            unsigned int jj = 0;
                            while (jj < diagram->shape[i].path_length) {
                                int cmd = (int)diagram->shape[i].path[jj++];
                                if (cmd == PLOTTER_PATH_MOVE || cmd == PLOTTER_PATH_LINE) {
                                    float xx = diagram->shape[i].path[jj++] * sx; float yy = diagram->shape[i].path[jj++] * sy;
                                    if (cmd == PLOTTER_PATH_MOVE) ctx->plot->path_move_to(ctx, xx, yy); else ctx->plot->path_line_to(ctx, xx, yy);
                                } else if (cmd == PLOTTER_PATH_BEZIER) {
                                    float x1 = diagram->shape[i].path[jj++] * sx; float y1 = diagram->shape[i].path[jj++] * sy;
                                    float x2 = diagram->shape[i].path[jj++] * sx; float y2 = diagram->shape[i].path[jj++] * sy;
                                    float x3 = diagram->shape[i].path[jj++] * sx; float y3 = diagram->shape[i].path[jj++] * sy;
                                    ctx->plot->path_bezier_to(ctx, x1, y1, x2, y2, x3, y3);
                                } else if (cmd == PLOTTER_PATH_CLOSE) ctx->plot->path_close(ctx);
                            }
                            if (current_pstyle.fill_type != PLOT_OP_TYPE_NONE) ctx->plot->path_fill(ctx, &current_pstyle, transform);
                            if (current_pstyle.stroke_type != PLOT_OP_TYPE_NONE) ctx->plot->path_stroke(ctx, &current_pstyle, transform);
                        } else {
                            float *scaled = malloc(sizeof(float) * diagram->shape[i].path_length);
                            if (scaled) {
                                unsigned int jj = 0, kk = 0;
                                while (jj < diagram->shape[i].path_length) {
                                    int cmd = (int)diagram->shape[i].path[jj++];
                                    scaled[kk++] = (float)cmd;
                                    if (cmd == PLOTTER_PATH_MOVE || cmd == PLOTTER_PATH_LINE) {
                                        scaled[kk++] = diagram->shape[i].path[jj++] * sx; scaled[kk++] = diagram->shape[i].path[jj++] * sy;
                                    } else if (cmd == PLOTTER_PATH_BEZIER) {
                                        for (int m = 0; m < 3; m++) { scaled[kk++] = diagram->shape[i].path[jj++] * sx; scaled[kk++] = diagram->shape[i].path[jj++] * sy; }
                                    }
                                }
                                ctx->plot->path(ctx, &current_pstyle, scaled, kk, transform);
                                free(scaled);
                            }
                        }
                    }
                }
            }
        } else if (diagram->shape[i].text) {
            FLUSH_BATCH();
            int tpx = (int)(diagram->shape[i].text_x * sx) + x;
            int tpy = (int)(diagram->shape[i].text_y * sy) + y;
            fstyle.background = 0xffffff;
            if (diagram->shape[i].fill == svgtiny_TRANSPARENT) fstyle.foreground = 0x000000;
            else fstyle.foreground = BGR(diagram->shape[i].fill);
            float fsize = diagram->shape[i].font_size;
            if (fsize <= 0.0f) fsize = 12.0f;
            fstyle.size = (int)(fsize * sx * PLOT_STYLE_SCALE);
            fstyle.flags |= FONTF_SIZE_PIXELS;
            if (diagram->shape[i].font_weight_bold) fstyle.weight = 700;

            if (diagram->shape[i].text_anchor != svgtiny_TEXT_ANCHOR_START) {
                int text_width = 0; size_t text_len = strlen(diagram->shape[i].text);
                if (guit != NULL && guit->layout != NULL && guit->layout->width != NULL)
                    guit->layout->width(&fstyle, diagram->shape[i].text, text_len, &text_width);
                else { int cw = (fstyle.size / PLOT_STYLE_SCALE) * 6 / 10; text_width = (int)text_len * cw; }
                if (diagram->shape[i].text_anchor == svgtiny_TEXT_ANCHOR_MIDDLE) tpx -= text_width / 2;
                else if (diagram->shape[i].text_anchor == svgtiny_TEXT_ANCHOR_END) tpx -= text_width;
            }
            res = ctx->plot->text(ctx, &fstyle, tpx, tpy, diagram->shape[i].text, strlen(diagram->shape[i].text));
            if (res != NSERROR_OK) ok = false;
        }
    }

    FLUSH_BATCH();
    if (batch_path) free(batch_path);

#undef BGR
#undef FLUSH_BATCH

    NSLOG(wisp, DEBUG, "PROFILER: STOP SVG rendering %p", svg);
    return ok;
}


bool svg_redraw_diagram(struct svgtiny_diagram *diagram, int x, int y, int width, int height, const struct rect *clip,
    const struct redraw_context *ctx, colour background_colour, colour current_color)
{
    svg_content tmp;
    memset(&tmp, 0, sizeof(tmp));
    tmp.diagram = diagram;
    tmp.base.width = width;
    tmp.base.height = height;
    return svg_redraw_internal(&tmp, x, y, width, height, clip, ctx, background_colour, current_color);
}


static bool svg_redraw_tiled_internal(
    svg_content *svg, struct content_redraw_data *data, const struct rect *clip, const struct redraw_context *ctx)
{
    /* Tiled redraw required.  SVG repeats to extents of clip
     * rectangle, in x, y or both directions */
    int x, y, x0, y0, x1, y1;

    x = x0 = data->x;
    y = y0 = data->y;

    /* Find the redraw boundaries to loop within */
    if (data->repeat_x) {
        for (; x0 > clip->x0; x0 -= data->width)
            ;
        x1 = clip->x1;
    } else {
        x1 = x + 1;
    }
    if (data->repeat_y) {
        for (; y0 > clip->y0; y0 -= data->height)
            ;
        y1 = clip->y1;
    } else {
        y1 = y + 1;
    }

    /* Repeatedly plot the SVG across the area */
    for (y = y0; y < y1; y += data->height) {
        for (x = x0; x < x1; x += data->width) {
            if (!svg_redraw_internal(svg, x, y, data->width, data->height, clip, ctx, data->background_colour, 0)) {
                return false;
            }
        }
    }

    return true;
}


/**
 * Redraw a CONTENT_SVG.
 */
static bool svg_redraw(
    struct content *c, struct content_redraw_data *data, const struct rect *clip, const struct redraw_context *ctx)
{
    svg_content *svg = (svg_content *)c;
    nsurl *u = content_get_url(c);
    const char *us = u ? nsurl_access(u) : "(inline)";

    NSLOG(wisp, WARNING,
        "SVGDIAG svg_redraw ENTRY: url=%s data={x=%d y=%d w=%d h=%d} "
        "c->width=%d c->height=%d diagram=%p shapes=%u parsed=%d",
        us, data->x, data->y, data->width, data->height, c->width, c->height, svg->diagram,
        svg->diagram ? svg->diagram->shape_count : 0, svg->parsed);

    if ((data->width <= 0) && (data->height <= 0)) {
        /* No point trying to plot SVG if it does not occupy a
         * valid area */
        NSLOG(wisp, WARNING, "SVGDIAG svg_redraw SKIP: width=%d height=%d (both <= 0), url=%s", data->width,
            data->height, us);
        return true;
    }

    if ((data->repeat_x == false) && (data->repeat_y == false)) {
        return svg_redraw_internal(
            svg, data->x, data->y, data->width, data->height, clip, ctx, data->background_colour, 0);
    }

    return svg_redraw_tiled_internal(svg, data, clip, ctx);
}


/**
 * Destroy a CONTENT_SVG and free all resources it owns.
 */

static void svg_destroy(struct content *c)
{
    svg_content *svg = (svg_content *)c;

    if (svg->diagram != NULL)
        svgtiny_free(svg->diagram);
}


static nserror svg_clone(const struct content *old, struct content **newc)
{
    svg_content *svg;
    nserror error;

    svg = calloc(1, sizeof(svg_content));
    if (svg == NULL)
        return NSERROR_NOMEM;

    error = content__clone(old, &svg->base);
    if (error != NSERROR_OK) {
        free(svg);
        return error;
    }

    /* Simply replay create/convert */
    error = svg_create_svg_data(svg);
    if (error != NSERROR_OK) {
        content_destroy(&svg->base);
        return error;
    }

    if (old->status == CONTENT_STATUS_READY || old->status == CONTENT_STATUS_DONE) {
        if (svg_convert(&svg->base) == false) {
            content_destroy(&svg->base);
            return NSERROR_CLONE_FAILED;
        }
    }

    *newc = (struct content *)svg;

    return NSERROR_OK;
}

static content_type svg_content_type(void)
{
    return CONTENT_IMAGE;
}

/**
 * Get the intrinsic aspect ratio for this SVG content.
 *
 * Returns the ratio from the viewBox (or explicit width/height) so that
 * layout can compute proportional dimensions for SVGs that have no
 * intrinsic dimensions (viewBox-only).
 */
static bool svg_get_intrinsic_ratio(struct content *c, int *ratio_w, int *ratio_h)
{
    svg_content *svg = (svg_content *)c;
    if (svg->ratio_width > 0 && svg->ratio_height > 0) {
        *ratio_w = svg->ratio_width;
        *ratio_h = svg->ratio_height;
        return true;
    }
    return false;
}

static const content_handler svg_content_handler = {.create = svg_create,
    .data_complete = svg_convert,
    .reformat = svg_reformat,
    .destroy = svg_destroy,
    .redraw = svg_redraw,
    .clone = svg_clone,
    .type = svg_content_type,
    .get_intrinsic_ratio = svg_get_intrinsic_ratio,
    .no_share = true};

static const char *svg_types[] = {"image/svg", "image/svg+xml"};


CONTENT_FACTORY_REGISTER_TYPES(svg, svg_types, svg_content_handler);
