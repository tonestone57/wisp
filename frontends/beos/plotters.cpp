/*
 * Copyright 2008 François Revol <mmu_man@users.sourceforge.net>
 * Copyright 2006 Rob Kendrick <rjek@rjek.com>
 * Copyright 2005 James Bursa <bursa@users.sourceforge.net>
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
 * BeOS/Haiku implementation target independent plotting.
 */

#define __STDBOOL_H__ 1
#include <BeBuild.h>
#include <Bitmap.h>
#include <GraphicsDefs.h>
#include <Region.h>
#include <Shape.h>
#include <View.h>
#include <math.h>

#ifdef __HAIKU__
#include <AffineTransform.h>
#include <GradientLinear.h>
#include <GradientRadial.h>
#endif

extern "C" {
#include "utils/log.h"
#include "utils/nsoption.h"
#include "utils/nsurl.h"
#include "utils/utils.h"
#include "wisp/plotters.h"
}
#include "beos/bitmap.h"
#include "beos/font.h"
#include "beos/gui.h"
#include "beos/plotters.h"

/*static*/ BView *current_view;

static __thread BShape *stateful_shape = NULL;

static const pattern kDottedPattern = {0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa};
static const pattern kDashedPattern = {0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc, 0x33, 0x33};

static const rgb_color kBlackColor = {0, 0, 0, 255};

BView *nsbeos_current_gc(void)
{
    return current_view;
}

BView *nsbeos_current_gc_lock(void)
{
    BView *view = current_view;
    if (view && view->LockLooper())
        return view;
    return NULL;
}

void nsbeos_current_gc_unlock(void)
{
    if (current_view) {
        current_view->UnlockLooper();
    }
}

void nsbeos_current_gc_set(BView *view)
{
    current_view = view;
}

static nserror nsbeos_plot_bbitmap(int x, int y, int width, int height, BBitmap *b, colour bg)
{
    if (width == 0 || height == 0) {
        return NSERROR_OK;
    }

    BView *view = nsbeos_current_gc();
    if (view == NULL) {
        beos_warn_user("No GC", 0);
        return NSERROR_INVALID;
    }

    drawing_mode oldmode = view->DrawingMode();
    source_alpha alpha;
    alpha_function func;
    view->GetBlendingMode(&alpha, &func);
    view->SetDrawingMode(B_OP_ALPHA);
    view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

    BRect rect(x, y, x + width - 1, y + height - 1);
    view->DrawBitmap(b, rect);

    view->SetBlendingMode(alpha, func);
    view->SetDrawingMode(oldmode);

    return NSERROR_OK;
}

/**
 * Convert a Wisp color to a Haiku logical rgb_color structure.
 *
 * This function populates a logical structure that delegates final pixel
 * layout to the OS (AGG-based app_server).
 *
 * @param c       Wisp color in 0xAABBGGRR format (inverted alpha).
 * @param opacity Optional opacity multiplier (0.0 to 1.0).
 * @return        Haiku rgb_color structure.
 */
rgb_color nsbeos_rgb_colour(colour c, float opacity)
{
    rgb_color color;
    if (c == NS_TRANSPARENT)
        return B_TRANSPARENT_32_BIT;

    /* Wisp color integer format: 0xAABBGGRR
     * [MSB] Alpha | Blue | Green | Red [LSB]
     * Note: Alpha 0 is opaque, 255 is transparent.
     */
    color.red = c & 0xff;
    color.green = (c >> 8) & 0xff;
    color.blue = (c >> 16) & 0xff;
    color.alpha = 255 - ((c >> 24) & 0xff);

    if (opacity > 0.0f && opacity < 1.0f) {
        color.alpha = (uint8)(color.alpha * opacity);
    }

    return color;
}

void nsbeos_set_colour(colour c)
{
    rgb_color color = nsbeos_rgb_colour(c);
    BView *view = nsbeos_current_gc();
    view->SetHighColor(color);
}

static void nsbeos_apply_style(const plot_style_t *pstyle, bool fill)
{
    BView *view = nsbeos_current_gc();
    if (fill) {
        view->SetHighColor(nsbeos_rgb_colour(pstyle->fill_colour, pstyle->fill_opacity));
    } else {
        view->SetHighColor(nsbeos_rgb_colour(pstyle->stroke_colour, pstyle->stroke_opacity));
        view->SetPenSize(plot_style_fixed_to_float(pstyle->stroke_width));

        pattern pat = B_SOLID_HIGH;
        if (pstyle->stroke_type == PLOT_OP_TYPE_DOT) pat = kDottedPattern;
        else if (pstyle->stroke_type == PLOT_OP_TYPE_DASH) pat = kDashedPattern;

        /* Note: BView doesn't have a direct way to set pattern globally for all calls,
           so we may need to pass it to Stroke* calls. */
    }
}

void nsbeos_plot_caret(int x, int y, int h)
{
    BView *view = nsbeos_current_gc();
    if (view == NULL)
        return;

    BPoint start(x, y);
    BPoint end(x, y + h - 1);
#if defined(__HAIKU__) || defined(B_BEOS_VERSION_DANO)
    view->SetHighColor(ui_color(B_DOCUMENT_TEXT_COLOR));
#else
    view->SetHighColor(kBlackColor);
#endif
    view->StrokeLine(start, end);
}

static nserror nsbeos_plot_clip(const struct redraw_context *ctx, const struct rect *ns_clip)
{
    BView *view = nsbeos_current_gc();
    if (view == NULL) {
        beos_warn_user("No GC", 0);
        return NSERROR_INVALID;
    }

    BRect rect(ns_clip->x0, ns_clip->y0, ns_clip->x1 - 1, ns_clip->y1 - 1);
    BRegion clip(rect);
    view->ConstrainClippingRegion(NULL);
    if (view->Bounds() != rect) {
        view->ConstrainClippingRegion(&clip);
    }

    return NSERROR_OK;
}

static nserror nsbeos_plot_arc(
    const struct redraw_context *ctx, const plot_style_t *style, int x, int y, int radius, int angle1, int angle2)
{
    BView *view = nsbeos_current_gc();
    if (view == NULL) return NSERROR_INVALID;

    nsbeos_apply_style(style, false);

    BPoint center(x, y);
    float angle = angle1;
    float span = angle2 - angle1;
    view->StrokeArc(center, radius, radius, angle, span);

    return NSERROR_OK;
}

static nserror nsbeos_plot_disc(const struct redraw_context *ctx, const plot_style_t *style, int x, int y, int radius)
{
    BView *view = nsbeos_current_gc();
    if (view == NULL) return NSERROR_INVALID;

    /* Use BShape for circles to benefit from AGG's subpixel anti-aliasing */
    BShape shape;
    shape.MoveTo(BPoint(x + radius, y));
    shape.ArcTo(radius, radius, 0, true, true, BPoint(x - radius, y));
    shape.ArcTo(radius, radius, 0, true, true, BPoint(x + radius, y));
    shape.Close();

    if (style->fill_type != PLOT_OP_TYPE_NONE) {
        nsbeos_apply_style(style, true);
        view->FillShape(&shape);
    }

    if (style->stroke_type != PLOT_OP_TYPE_NONE) {
        nsbeos_apply_style(style, false);
        view->StrokeShape(&shape);
    }

    return NSERROR_OK;
}

static nserror nsbeos_plot_line(const struct redraw_context *ctx, const plot_style_t *style, const struct rect *line)
{
    BView *view = nsbeos_current_gc();
    if (view == NULL) return NSERROR_OK;

    nsbeos_apply_style(style, false);

    pattern pat = B_SOLID_HIGH;
    if (style->stroke_type == PLOT_OP_TYPE_DOT) pat = kDottedPattern;
    else if (style->stroke_type == PLOT_OP_TYPE_DASH) pat = kDashedPattern;

    BPoint start(line->x0, line->y0);
    BPoint end(line->x1, line->y1);
    view->StrokeLine(start, end, pat);

    return NSERROR_OK;
}

static nserror
nsbeos_plot_rectangle(const struct redraw_context *ctx, const plot_style_t *style, const struct rect *nsrect)
{
    BView *view = nsbeos_current_gc();
    if (view == NULL) return NSERROR_INVALID;

    BRect rect(nsrect->x0, nsrect->y0, nsrect->x1 - 1, nsrect->y1 - 1);

    if (style->fill_type != PLOT_OP_TYPE_NONE) {
        nsbeos_apply_style(style, true);
        view->FillRect(rect);
    }

    if (style->stroke_type != PLOT_OP_TYPE_NONE) {
        nsbeos_apply_style(style, false);
        pattern pat = B_SOLID_HIGH;
        if (style->stroke_type == PLOT_OP_TYPE_DOT) pat = kDottedPattern;
        else if (style->stroke_type == PLOT_OP_TYPE_DASH) pat = kDashedPattern;
        view->StrokeRect(rect, pat);
    }

    return NSERROR_OK;
}

static nserror
nsbeos_plot_polygon(const struct redraw_context *ctx, const plot_style_t *style, const int *p, unsigned int n)
{
    unsigned int i;
    BView *view = nsbeos_current_gc();
    if (view == NULL) return NSERROR_INVALID;

    BPoint points[n];
    for (i = 0; i < n; i++) {
        points[i] = BPoint(p[2 * i], p[2 * i + 1]);
    }

    if (style->fill_type != PLOT_OP_TYPE_NONE) {
        nsbeos_apply_style(style, true);
        view->FillPolygon(points, (int32)n);
    }

    if (style->stroke_type != PLOT_OP_TYPE_NONE) {
        nsbeos_apply_style(style, false);
        view->StrokePolygon(points, (int32)n);
    }

    return NSERROR_OK;
}

static nserror nsbeos_plot_path(const struct redraw_context *ctx, const plot_style_t *pstyle, const float *p,
    unsigned int n, const float transform[6])
{
    unsigned int i;
    BShape shape;

    if (n == 0) return NSERROR_OK;

    if (p[0] != PLOTTER_PATH_MOVE) {
        NSLOG(wisp, INFO, "path doesn't start with a move");
        return NSERROR_INVALID;
    }

    for (i = 0; i < n;) {
        if (p[i] == PLOTTER_PATH_MOVE) {
            shape.MoveTo(BPoint(p[i + 1], p[i + 2]));
            i += 3;
        } else if (p[i] == PLOTTER_PATH_CLOSE) {
            shape.Close();
            i++;
        } else if (p[i] == PLOTTER_PATH_LINE) {
            shape.LineTo(BPoint(p[i + 1], p[i + 2]));
            i += 3;
        } else if (p[i] == PLOTTER_PATH_BEZIER) {
            BPoint pt[3] = {BPoint(p[i + 1], p[i + 2]), BPoint(p[i + 3], p[i + 4]), BPoint(p[i + 5], p[i + 6])};
            shape.BezierTo(pt);
            i += 7;
        } else {
            NSLOG(wisp, INFO, "bad path command %f", p[i]);
            return NSERROR_INVALID;
        }
    }

    BView *view = nsbeos_current_gc();
    if (view == NULL) return NSERROR_INVALID;

#ifdef __HAIKU__
    view->PushState();
    if (transform) {
        BAffineTransform matrix(transform[0], transform[1], transform[2], transform[3], transform[4], transform[5]);
        BAffineTransform current = view->Transform();
        current.Multiply(matrix);
        view->SetTransform(current);
    }
    view->SetDrawingMode(B_OP_ALPHA);
    view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
#endif

    if (pstyle->fill_type != PLOT_OP_TYPE_NONE) {
        view->SetHighColor(nsbeos_rgb_colour(pstyle->fill_colour, pstyle->fill_opacity));
        view->FillShape(&shape);
    }
    if (pstyle->stroke_type != PLOT_OP_TYPE_NONE) {
        view->SetHighColor(nsbeos_rgb_colour(pstyle->stroke_colour, pstyle->stroke_opacity));
        view->SetPenSize(plot_style_fixed_to_float(pstyle->stroke_width));

        pattern pat = B_SOLID_HIGH;
        if (pstyle->stroke_type == PLOT_OP_TYPE_DOT) pat = kDottedPattern;
        else if (pstyle->stroke_type == PLOT_OP_TYPE_DASH) pat = kDashedPattern;

        view->StrokeShape(&shape, pat);
    }

#ifdef __HAIKU__
    view->PopState();
#endif

    return NSERROR_OK;
}

static nserror nsbeos_plot_bitmap(const struct redraw_context *ctx, struct bitmap *bitmap, int x, int y, int width,
    int height, colour bg, bitmap_flags_t flags)
{
    BBitmap *primary;
    BBitmap *pretiled;
    bool repeat_x = (flags & BITMAPF_REPEAT_X);
    bool repeat_y = (flags & BITMAPF_REPEAT_Y);

    if (!(repeat_x || repeat_y)) {
        primary = nsbeos_bitmap_get_primary(bitmap);
        return nsbeos_plot_bbitmap(x, y, width, height, primary, bg);
    }

    if (repeat_x && !repeat_y)
        pretiled = nsbeos_bitmap_get_pretile_x(bitmap);
    else if (repeat_x && repeat_y)
        pretiled = nsbeos_bitmap_get_pretile_xy(bitmap);
    else if (!repeat_x && repeat_y)
        pretiled = nsbeos_bitmap_get_pretile_y(bitmap);
    else
        pretiled = nsbeos_bitmap_get_primary(bitmap);

    primary = nsbeos_bitmap_get_primary(bitmap);
    int p_w = primary->Bounds().Width() + 1;
    int p_h = primary->Bounds().Height() + 1;
    int t_w = pretiled->Bounds().Width() + 1;
    int t_h = pretiled->Bounds().Height() + 1;

    width *= t_w;
    width /= p_w;
    height *= t_h;
    height /= p_h;

    BView *view = nsbeos_current_gc();
    if (view == NULL) return NSERROR_INVALID;

    BRegion clipreg;
    view->GetClippingRegion(&clipreg);
    BRect cliprect = clipreg.Frame();

    int doneheight = (y > cliprect.top) ? ((int)cliprect.top - height) + ((y - (int)cliprect.top) % height) : y;

    while (doneheight < cliprect.bottom) {
        int donewidth = (x > cliprect.left) ? ((int)cliprect.left - width) + ((x - (int)cliprect.left) % width) : x;
        while (donewidth < cliprect.right) {
            nsbeos_plot_bbitmap(donewidth, doneheight, width, height, pretiled, bg);
            donewidth += width;
            if (!repeat_x) break;
        }
        doneheight += height;
        if (!repeat_y) break;
    }

    return NSERROR_OK;
}

static nserror nsbeos_plot_text(const struct redraw_context *ctx, const struct plot_font_style *fstyle, int x, int y,
    const char *text, size_t length)
{
    if (!nsfont_paint(fstyle, text, length, x, y)) {
        return NSERROR_INVALID;
    }
    return NSERROR_OK;
}

static nserror nsbeos_plot_finalise(const struct redraw_context *ctx)
{
    if (stateful_shape) {
        delete stateful_shape;
        stateful_shape = NULL;
    }
    return NSERROR_OK;
}

static nserror nsbeos_plot_path_begin(const struct redraw_context *ctx)
{
    if (!stateful_shape) stateful_shape = new BShape();
    stateful_shape->Clear();
    return NSERROR_OK;
}

static nserror nsbeos_plot_path_move_to(const struct redraw_context *ctx, float x, float y)
{
    if (stateful_shape) stateful_shape->MoveTo(BPoint(x, y));
    return NSERROR_OK;
}

static nserror nsbeos_plot_path_line_to(const struct redraw_context *ctx, float x, float y)
{
    if (stateful_shape) stateful_shape->LineTo(BPoint(x, y));
    return NSERROR_OK;
}

static nserror nsbeos_plot_path_bezier_to(const struct redraw_context *ctx, float x1, float y1, float x2, float y2, float x3, float y3)
{
    if (stateful_shape) {
        BPoint pts[3] = { BPoint(x1, y1), BPoint(x2, y2), BPoint(x3, y3) };
        stateful_shape->BezierTo(pts);
    }
    return NSERROR_OK;
}

static nserror nsbeos_plot_path_close(const struct redraw_context *ctx)
{
    if (stateful_shape) stateful_shape->Close();
    return NSERROR_OK;
}

static nserror nsbeos_plot_path_fill(const struct redraw_context *ctx, const plot_style_t *pstyle, const float transform[6])
{
    if (!stateful_shape) return NSERROR_OK;
    BView *view = nsbeos_current_gc();
    if (view == NULL) return NSERROR_INVALID;

    if (pstyle->fill_type != PLOT_OP_TYPE_NONE) {
        view->SetHighColor(nsbeos_rgb_colour(pstyle->fill_colour, pstyle->fill_opacity));
#ifdef __HAIKU__
        view->PushState();
        if (transform) {
            BAffineTransform matrix(transform[0], transform[1], transform[2], transform[3], transform[4], transform[5]);
            BAffineTransform current = view->Transform();
            current.Multiply(matrix);
            view->SetTransform(current);
        }
        view->FillShape(stateful_shape);
        view->PopState();
#else
        view->FillShape(stateful_shape);
#endif
    }
    return NSERROR_OK;
}

static nserror nsbeos_plot_path_stroke(const struct redraw_context *ctx, const plot_style_t *pstyle, const float transform[6])
{
    if (!stateful_shape) return NSERROR_OK;
    BView *view = nsbeos_current_gc();
    if (view == NULL) return NSERROR_INVALID;

    if (pstyle->stroke_type != PLOT_OP_TYPE_NONE) {
        view->SetHighColor(nsbeos_rgb_colour(pstyle->stroke_colour, pstyle->stroke_opacity));
        view->SetPenSize(plot_style_fixed_to_float(pstyle->stroke_width));
#ifdef __HAIKU__
        view->PushState();
        if (transform) {
            BAffineTransform matrix(transform[0], transform[1], transform[2], transform[3], transform[4], transform[5]);
            BAffineTransform current = view->Transform();
            current.Multiply(matrix);
            view->SetTransform(current);
        }
        view->StrokeShape(stateful_shape);
        view->PopState();
#else
        view->StrokeShape(stateful_shape);
#endif
    }
    return NSERROR_OK;
}

static nserror nsbeos_plot_push_transform(const struct redraw_context *ctx, const float transform[6])
{
#ifdef __HAIKU__
    BView *view = nsbeos_current_gc();
    if (view == NULL) return NSERROR_INVALID;
    view->PushState();
    BAffineTransform matrix(transform[0], transform[1], transform[2], transform[3], transform[4], transform[5]);
    BAffineTransform current = view->Transform();
    current.Multiply(matrix);
    view->SetTransform(current);
    return NSERROR_OK;
#else
    return NSERROR_NOT_IMPLEMENTED;
#endif
}

static nserror nsbeos_plot_pop_transform(const struct redraw_context *ctx)
{
#ifdef __HAIKU__
    BView *view = nsbeos_current_gc();
    if (view == NULL) return NSERROR_INVALID;
    view->PopState();
    return NSERROR_OK;
#else
    return NSERROR_NOT_IMPLEMENTED;
#endif
}

static nserror nsbeos_plot_linear_gradient(const struct redraw_context *ctx, const float *path, unsigned int path_len,
    const float transform[6], float x0, float y0, float x1, float y1, const struct gradient_stop *stops,
    unsigned int stop_count)
{
#ifdef __HAIKU__
    BView *view = nsbeos_current_gc();
    if (view == NULL) return NSERROR_INVALID;

    BGradientLinear gradient(BPoint(x0, y0), BPoint(x1, y1));
    for (unsigned int i = 0; i < stop_count; i++) {
        gradient.AddColor(nsbeos_rgb_colour(stops[i].color), stops[i].offset);
    }

    view->PushState();
    if (transform) {
        BAffineTransform matrix(transform[0], transform[1], transform[2], transform[3], transform[4], transform[5]);
        BAffineTransform current = view->Transform();
        current.Multiply(matrix);
        view->SetTransform(current);
    }

    if (path && path_len > 0) {
        BShape shape;
        for (unsigned int i = 0; i < path_len;) {
            if (path[i] == PLOTTER_PATH_MOVE) {
                shape.MoveTo(BPoint(path[i+1], path[i+2]));
                i += 3;
            } else if (path[i] == PLOTTER_PATH_CLOSE) {
                shape.Close();
                i++;
            } else if (path[i] == PLOTTER_PATH_LINE) {
                shape.LineTo(BPoint(path[i+1], path[i+2]));
                i += 3;
            } else if (path[i] == PLOTTER_PATH_BEZIER) {
                BPoint pt[3] = { BPoint(path[i+1], path[i+2]), BPoint(path[i+3], path[i+4]), BPoint(path[i+5], path[i+6]) };
                shape.BezierTo(pt);
                i += 7;
            } else break;
        }
        view->FillShape(&shape, gradient);
    } else {
        view->FillRect(view->Bounds(), gradient);
    }
    view->PopState();
    return NSERROR_OK;
#else
    return NSERROR_NOT_IMPLEMENTED;
#endif
}

static nserror nsbeos_plot_radial_gradient(const struct redraw_context *ctx, const float *path, unsigned int path_len,
    const float transform[6], float cx, float cy, float rx, float ry, const struct gradient_stop *stops,
    unsigned int stop_count)
{
#ifdef __HAIKU__
    BView *view = nsbeos_current_gc();
    if (view == NULL) return NSERROR_INVALID;

    BGradientRadial gradient(BPoint(cx, cy), rx);
    for (unsigned int i = 0; i < stop_count; i++) {
        gradient.AddColor(nsbeos_rgb_colour(stops[i].color), stops[i].offset);
    }

    view->PushState();
    BAffineTransform current = view->Transform();
    if (transform) {
        BAffineTransform matrix(transform[0], transform[1], transform[2], transform[3], transform[4], transform[5]);
        current.Multiply(matrix);
    }
    if (rx != ry && rx > 0) {
        current.ScaleBy(BPoint(cx, cy), 1.0, ry / rx);
    }
    view->SetTransform(current);

    if (path && path_len > 0) {
        BShape shape;
        for (unsigned int i = 0; i < path_len;) {
            if (path[i] == PLOTTER_PATH_MOVE) {
                shape.MoveTo(BPoint(path[i+1], path[i+2]));
                i += 3;
            } else if (path[i] == PLOTTER_PATH_CLOSE) {
                shape.Close();
                i++;
            } else if (path[i] == PLOTTER_PATH_LINE) {
                shape.LineTo(BPoint(path[i+1], path[i+2]));
                i += 3;
            } else if (path[i] == PLOTTER_PATH_BEZIER) {
                BPoint pt[3] = { BPoint(path[i+1], path[i+2]), BPoint(path[i+3], path[i+4]), BPoint(path[i+5], path[i+6]) };
                shape.BezierTo(pt);
                i += 7;
            } else break;
        }
        view->FillShape(&shape, gradient);
    } else {
        view->FillRect(view->Bounds(), gradient);
    }
    view->PopState();
    return NSERROR_OK;
#else
    return NSERROR_NOT_IMPLEMENTED;
#endif
}

static nserror nsbeos_plot_group_start(const struct redraw_context *ctx, const char *name)
{
    return NSERROR_OK;
}

static nserror nsbeos_plot_group_end(const struct redraw_context *ctx)
{
    return NSERROR_OK;
}

static nserror nsbeos_plot_flush(const struct redraw_context *ctx)
{
    BView *view = nsbeos_current_gc();
    if (view != NULL) view->Sync();
    return NSERROR_OK;
}

/**
 * beos plotter operation table
 */
const struct plotter_table nsbeos_plotters = {
    .clip = nsbeos_plot_clip,
    .arc = nsbeos_plot_arc,
    .disc = nsbeos_plot_disc,
    .line = nsbeos_plot_line,
    .rectangle = nsbeos_plot_rectangle,
    .polygon = nsbeos_plot_polygon,
    .path = nsbeos_plot_path,
    .finalise = nsbeos_plot_finalise,
    .path_begin = nsbeos_plot_path_begin,
    .path_move_to = nsbeos_plot_path_move_to,
    .path_line_to = nsbeos_plot_path_line_to,
    .path_bezier_to = nsbeos_plot_path_bezier_to,
    .path_close = nsbeos_plot_path_close,
    .path_fill = nsbeos_plot_path_fill,
    .path_stroke = nsbeos_plot_path_stroke,
    .bitmap = nsbeos_plot_bitmap,
    .text = nsbeos_plot_text,
    .group_start = nsbeos_plot_group_start,
    .group_end = nsbeos_plot_group_end,
    .flush = nsbeos_plot_flush,
    .push_transform = nsbeos_plot_push_transform,
    .pop_transform = nsbeos_plot_pop_transform,
    .linear_gradient = nsbeos_plot_linear_gradient,
    .radial_gradient = nsbeos_plot_radial_gradient,
    .draw_gadget = gui_window_draw_gadget,
    .option_knockout = true
};
