#include "wisp/utils/config.h"

#ifdef WISP_WINDOWS_USE_D2D

#define _USE_MATH_DEFINES
#include <cmath>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <vector>
#include <stack>

extern "C" {
#include "wisp/plotters.h"
#include "wisp/plot_style.h"
#include "wisp/types.h"
#include "wisp/bitmap.h"
#include "wisp/utils/log.h"
#include "windows/window.h"
#include "windows/bitmap.h"
#include "windows/d2d_types.h"
}

/* Multi-window safe state access */
#define GW ((struct gui_window *)ctx->priv)
#define D2D_RT (GW ? (ID2D1RenderTarget *)GW->d2d_rt : d2d_rt_override)
#define HAS_STACK (GW && GW->d2d_transform_stack)
#define TRANSFORM_STACK (*((std::stack<D2D1_MATRIX_3X2_F>*)GW->d2d_transform_stack))
#define HAS_PATH (GW && GW->d2d_stateful_path)
#define STATEFUL_PATH (*((std::vector<d2d_path_command>*)GW->d2d_stateful_path))

#define D2D_CLIP (GW ? D2D1::RectF(GW->d2d_clip_x0, GW->d2d_clip_y0, GW->d2d_clip_x1, GW->d2d_clip_y1) : d2d_clip_override)
static D2D1_RECT_F d2d_clip_override;
static ID2D1RenderTarget *d2d_rt_override = NULL;

/**
 * Convert Wisp colour (XBGR) and opacity to D2D1_COLOR_F
 */
static D2D1_COLOR_F d2d_color(colour c, float opacity = 1.0f) {
    if (opacity == 0.0f) opacity = 1.0f;
    return D2D1::ColorF(
        (float)(c & 0xFF) / 255.0f,
        (float)((c >> 8) & 0xFF) / 255.0f,
        (float)((c >> 16) & 0xFF) / 255.0f,
        opacity
    );
}

static nserror clip(const struct redraw_context *ctx, const struct rect *clip) {
    if (GW) {
        GW->d2d_clip_x0 = (float)clip->x0;
        GW->d2d_clip_y0 = (float)clip->y0;
        GW->d2d_clip_x1 = (float)clip->x1 + 1;
        GW->d2d_clip_y1 = (float)clip->y1 + 1;
    } else {
        d2d_clip_override = D2D1::RectF((float)clip->x0, (float)clip->y0, (float)clip->x1 + 1, (float)clip->y1 + 1);
    }
    return NSERROR_OK;
}

static nserror rectangle(const struct redraw_context *ctx, const plot_style_t *style, const struct rect *rect) {
    ID2D1RenderTarget *rt = D2D_RT;
    if (!rt) return NSERROR_INVALID;
    D2D1_RECT_F d2d_rect = D2D1::RectF((float)rect->x0, (float)rect->y0, (float)rect->x1 + 1, (float)rect->y1 + 1);

    rt->PushAxisAlignedClip(D2D_CLIP, D2D1_ANTIALIAS_MODE_ALIASED);
    if (style->fill_type != PLOT_OP_TYPE_NONE) {
        ID2D1SolidColorBrush *brush;
        if (SUCCEEDED(rt->CreateSolidColorBrush(d2d_color(style->fill_colour, style->fill_opacity), &brush))) {
            rt->FillRectangle(d2d_rect, brush);
            brush->Release();
        }
    }
    if (style->stroke_type != PLOT_OP_TYPE_NONE) {
        ID2D1SolidColorBrush *brush;
        if (SUCCEEDED(rt->CreateSolidColorBrush(d2d_color(style->stroke_colour, style->stroke_opacity), &brush))) {
            rt->DrawRectangle(d2d_rect, brush, plot_style_fixed_to_float(style->stroke_width));
            brush->Release();
        }
    }
    rt->PopAxisAlignedClip();
    return NSERROR_OK;
}

static nserror line(const struct redraw_context *ctx, const plot_style_t *style, const struct rect *line) {
    ID2D1RenderTarget *rt = D2D_RT;
    if (!rt) return NSERROR_INVALID;

    rt->PushAxisAlignedClip(D2D_CLIP, D2D1_ANTIALIAS_MODE_ALIASED);
    ID2D1SolidColorBrush *brush;
    if (SUCCEEDED(rt->CreateSolidColorBrush(d2d_color(style->stroke_colour, style->stroke_opacity), &brush))) {
        rt->DrawLine(D2D1::Point2F((float)line->x0, (float)line->y0), D2D1::Point2F((float)line->x1, (float)line->y1), brush, plot_style_fixed_to_float(style->stroke_width));
        brush->Release();
    }
    rt->PopAxisAlignedClip();
    return NSERROR_OK;
}

static nserror polygon(const struct redraw_context *ctx, const plot_style_t *style, const int *p, unsigned int n) {
    ID2D1RenderTarget *rt = D2D_RT;
    if (!rt || n < 2) return NSERROR_INVALID;
    ID2D1Factory *factory;
    rt->GetFactory(&factory);
    ID2D1PathGeometry *geometry;
    if (SUCCEEDED(factory->CreatePathGeometry(&geometry))) {
        ID2D1GeometrySink *sink;
        if (SUCCEEDED(geometry->Open(&sink))) {
            sink->BeginFigure(D2D1::Point2F((float)p[0], (float)p[1]), D2D1_FIGURE_BEGIN_FILLED);
            for (unsigned int i = 1; i < n; i++) {
                sink->AddLine(D2D1::Point2F((float)p[i*2], (float)p[i*2+1]));
            }
            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
            sink->Close();
            sink->Release();

            rt->PushAxisAlignedClip(D2D_CLIP, D2D1_ANTIALIAS_MODE_ALIASED);
            if (style->fill_type != PLOT_OP_TYPE_NONE) {
                ID2D1SolidColorBrush *brush;
                if (SUCCEEDED(rt->CreateSolidColorBrush(d2d_color(style->fill_colour, style->fill_opacity), &brush))) {
                    rt->FillGeometry(geometry, brush);
                    brush->Release();
                }
            }
            rt->PopAxisAlignedClip();
        }
        geometry->Release();
    }
    factory->Release();
    return NSERROR_OK;
}

static ID2D1PathGeometry* create_geometry_from_raw(ID2D1RenderTarget *rt, const float *p, unsigned int n) {
    ID2D1Factory *factory;
    rt->GetFactory(&factory);
    ID2D1PathGeometry *geometry = NULL;
    if (SUCCEEDED(factory->CreatePathGeometry(&geometry))) {
        ID2D1GeometrySink *sink;
        if (SUCCEEDED(geometry->Open(&sink))) {
            bool figure_open = false;
            unsigned int i = 0;
            while (i < n) {
                int cmd = (int)p[i++];
                switch (cmd) {
                    case PLOTTER_PATH_MOVE:
                        if (figure_open) sink->EndFigure(D2D1_FIGURE_END_OPEN);
                        sink->BeginFigure(D2D1::Point2F(p[i], p[i+1]), D2D1_FIGURE_BEGIN_FILLED);
                        i += 2;
                        figure_open = true;
                        break;
                    case PLOTTER_PATH_LINE:
                        sink->AddLine(D2D1::Point2F(p[i], p[i+1]));
                        i += 2;
                        break;
                    case PLOTTER_PATH_BEZIER:
                        sink->AddBezier(D2D1::BezierSegment(
                            D2D1::Point2F(p[i], p[i+1]),
                            D2D1::Point2F(p[i+2], p[i+3]),
                            D2D1::Point2F(p[i+4], p[i+5])
                        ));
                        i += 6;
                        break;
                    case PLOTTER_PATH_CLOSE:
                        if (figure_open) {
                            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                            figure_open = false;
                        }
                        break;
                }
            }
            if (figure_open) sink->EndFigure(D2D1_FIGURE_END_OPEN);
            sink->Close();
            sink->Release();
        }
    }
    factory->Release();
    return geometry;
}

static ID2D1PathGeometry* create_geometry_from_commands(ID2D1RenderTarget *rt, const std::vector<d2d_path_command>& commands) {
    ID2D1Factory *factory;
    rt->GetFactory(&factory);
    ID2D1PathGeometry *geometry = NULL;
    if (SUCCEEDED(factory->CreatePathGeometry(&geometry))) {
        ID2D1GeometrySink *sink;
        if (SUCCEEDED(geometry->Open(&sink))) {
            bool figure_open = false;
            for (const auto& cmd : commands) {
                switch (cmd.type) {
                    case PLOTTER_PATH_MOVE:
                        if (figure_open) sink->EndFigure(D2D1_FIGURE_END_OPEN);
                        sink->BeginFigure(D2D1::Point2F(cmd.x1, cmd.y1), D2D1_FIGURE_BEGIN_FILLED);
                        figure_open = true;
                        break;
                    case PLOTTER_PATH_LINE:
                        sink->AddLine(D2D1::Point2F(cmd.x1, cmd.y1));
                        break;
                    case PLOTTER_PATH_BEZIER:
                        sink->AddBezier(D2D1::BezierSegment(
                            D2D1::Point2F(cmd.x1, cmd.y1),
                            D2D1::Point2F(cmd.x2, cmd.y2),
                            D2D1::Point2F(cmd.x3, cmd.y3)
                        ));
                        break;
                    case PLOTTER_PATH_CLOSE:
                        if (figure_open) {
                            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                            figure_open = false;
                        }
                        break;
                }
            }
            if (figure_open) sink->EndFigure(D2D1_FIGURE_END_OPEN);
            sink->Close();
            sink->Release();
        }
    }
    factory->Release();
    return geometry;
}

static nserror path(const struct redraw_context *ctx, const plot_style_t *pstyle, const float *p, unsigned int n, const float transform[6]) {
    ID2D1RenderTarget *rt = D2D_RT;
    if (!rt) return NSERROR_INVALID;
    ID2D1PathGeometry *geometry = create_geometry_from_raw(rt, p, n);
    if (geometry) {
        D2D1_MATRIX_3X2_F old_transform;
        rt->GetTransform(&old_transform);
        if (transform) {
            D2D1_MATRIX_3X2_F d2d_transform = D2D1::Matrix3x2F(transform[0], transform[1], transform[2], transform[3], transform[4], transform[5]);
            rt->SetTransform(d2d_transform * old_transform);
        }

        rt->PushAxisAlignedClip(D2D_CLIP, D2D1_ANTIALIAS_MODE_ALIASED);
        if (pstyle->fill_type != PLOT_OP_TYPE_NONE) {
            ID2D1SolidColorBrush *brush;
            if (SUCCEEDED(rt->CreateSolidColorBrush(d2d_color(pstyle->fill_colour, pstyle->fill_opacity), &brush))) {
                rt->FillGeometry(geometry, brush);
                brush->Release();
            }
        }
        if (pstyle->stroke_type != PLOT_OP_TYPE_NONE) {
            ID2D1SolidColorBrush *brush;
            if (SUCCEEDED(rt->CreateSolidColorBrush(d2d_color(pstyle->stroke_colour, pstyle->stroke_opacity), &brush))) {
                rt->DrawGeometry(geometry, brush, plot_style_fixed_to_float(pstyle->stroke_width));
                brush->Release();
            }
        }
        rt->PopAxisAlignedClip();

        rt->SetTransform(old_transform);
        geometry->Release();
    }
    return NSERROR_OK;
}

static nserror disc(const struct redraw_context *ctx, const plot_style_t *style, int x, int y, int radius) {
    ID2D1RenderTarget *rt = D2D_RT;
    if (!rt) return NSERROR_INVALID;
    D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F((float)x, (float)y), (float)radius, (float)radius);

    rt->PushAxisAlignedClip(D2D_CLIP, D2D1_ANTIALIAS_MODE_ALIASED);
    if (style->fill_type != PLOT_OP_TYPE_NONE) {
        ID2D1SolidColorBrush *brush;
        if (SUCCEEDED(rt->CreateSolidColorBrush(d2d_color(style->fill_colour, style->fill_opacity), &brush))) {
            rt->FillEllipse(ellipse, brush);
            brush->Release();
        }
    }
    if (style->stroke_type != PLOT_OP_TYPE_NONE) {
        ID2D1SolidColorBrush *brush;
        if (SUCCEEDED(rt->CreateSolidColorBrush(d2d_color(style->stroke_colour, style->stroke_opacity), &brush))) {
            rt->DrawEllipse(ellipse, brush, plot_style_fixed_to_float(style->stroke_width));
            brush->Release();
        }
    }
    rt->PopAxisAlignedClip();
    return NSERROR_OK;
}

static nserror arc(const struct redraw_context *ctx, const plot_style_t *style, int x, int y, int radius, int angle1, int angle2) {
    ID2D1RenderTarget *rt = D2D_RT;
    if (!rt) return NSERROR_INVALID;
    ID2D1Factory *factory;
    rt->GetFactory(&factory);
    ID2D1PathGeometry *geometry;
    if (SUCCEEDED(factory->CreatePathGeometry(&geometry))) {
        ID2D1GeometrySink *sink;
        if (SUCCEEDED(geometry->Open(&sink))) {
            float start_angle = (float)angle1 * (float)M_PI / 180.0f;
            float end_angle = (float)angle2 * (float)M_PI / 180.0f;
            D2D1_POINT_2F start_pt = D2D1::Point2F(x + radius * cosf(start_angle), y - radius * sinf(start_angle));
            D2D1_POINT_2F end_pt = D2D1::Point2F(x + radius * cosf(end_angle), y - radius * sinf(end_angle));
            sink->BeginFigure(start_pt, D2D1_FIGURE_BEGIN_HOLLOW);
            sink->AddArc(D2D1::ArcSegment(end_pt, D2D1::SizeF((float)radius, (float)radius), 0.0f, D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE, (angle2 - angle1) > 180 ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL));
            sink->EndFigure(D2D1_FIGURE_END_OPEN);
            sink->Close();
            sink->Release();

            rt->PushAxisAlignedClip(D2D_CLIP, D2D1_ANTIALIAS_MODE_ALIASED);
            ID2D1SolidColorBrush *brush;
            if (SUCCEEDED(rt->CreateSolidColorBrush(d2d_color(style->stroke_colour, style->stroke_opacity), &brush))) {
                rt->DrawGeometry(geometry, brush, plot_style_fixed_to_float(style->stroke_width));
                brush->Release();
            }
            rt->PopAxisAlignedClip();
        }
        geometry->Release();
    }
    factory->Release();
    return NSERROR_OK;
}

static nserror bitmap(const struct redraw_context *ctx, struct bitmap *bitmap, int x, int y, int width, int height, colour bg, bitmap_flags_t flags) {
    ID2D1RenderTarget *rt = D2D_RT;
    if (!rt || !bitmap) return NSERROR_INVALID;

    ID2D1Bitmap *d2d_bmp = (ID2D1Bitmap *)bitmap->d2d_bmp;
    if (!d2d_bmp) {
        D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
        if (FAILED(rt->CreateBitmap(D2D1::SizeU(bitmap->width, bitmap->height), bitmap->pixdata, bitmap->width * 4, &props, &d2d_bmp))) {
            return NSERROR_INVALID;
        }
        bitmap->d2d_bmp = (void *)d2d_bmp;
    }

    D2D1_RECT_F dest_rect = D2D1::RectF((float)x, (float)y, (float)x + width, (float)y + height);

    rt->PushAxisAlignedClip(D2D_CLIP, D2D1_ANTIALIAS_MODE_ALIASED);
    if (flags & (BITMAPF_REPEAT_X | BITMAPF_REPEAT_Y)) {
        ID2D1BitmapBrush *brush;
        if (SUCCEEDED(rt->CreateBitmapBrush(d2d_bmp, &brush))) {
            brush->SetExtendModeX((flags & BITMAPF_REPEAT_X) ? D2D1_EXTEND_MODE_WRAP : D2D1_EXTEND_MODE_CLAMP);
            brush->SetExtendModeY((flags & BITMAPF_REPEAT_Y) ? D2D1_EXTEND_MODE_WRAP : D2D1_EXTEND_MODE_CLAMP);
            brush->SetTransform(D2D1::Matrix3x2F::Translation((float)x, (float)y));
            rt->FillRectangle(D2D_CLIP, brush);
            brush->Release();
        }
    } else {
        rt->DrawBitmap(d2d_bmp, dest_rect);
    }
    rt->PopAxisAlignedClip();

    return NSERROR_OK;
}

extern "C" IDWriteTextFormat* win32_dwrite_get_format(const plot_font_style_t* style);

static nserror text(const struct redraw_context *ctx, const plot_font_style_t *fstyle, int x, int y, const char *text, size_t length) {
    ID2D1RenderTarget *rt = D2D_RT;
    if (!rt) return NSERROR_INVALID;
    int wlen = MultiByteToWideChar(CP_UTF8, 0, text, (int)length, NULL, 0);
    std::vector<WCHAR> wstr(wlen + 1);
    MultiByteToWideChar(CP_UTF8, 0, text, (int)length, wstr.data(), wlen);
    wstr[wlen] = 0;
    IDWriteTextFormat *text_format = win32_dwrite_get_format(fstyle);
    if (text_format) {
        ID2D1SolidColorBrush *brush;
        if (SUCCEEDED(rt->CreateSolidColorBrush(d2d_color(fstyle->foreground), &brush))) {
            rt->PushAxisAlignedClip(D2D_CLIP, D2D1_ANTIALIAS_MODE_ALIASED);
            rt->DrawText(wstr.data(), wlen, text_format, D2D1::RectF((float)x, (float)y - plot_style_fixed_to_float(fstyle->size), (float)x + 10000.0f, (float)y + 1000.0f), brush);
            rt->PopAxisAlignedClip();
            brush->Release();
        }
        // format is cached and managed by font_dwrite.cpp
    }
    return NSERROR_OK;
}

static nserror push_transform(const struct redraw_context *ctx, const float transform[6]) {
    ID2D1RenderTarget *rt = D2D_RT;
    if (!rt) return NSERROR_INVALID;
    D2D1_MATRIX_3X2_F current;
    rt->GetTransform(&current);
    if (HAS_STACK) TRANSFORM_STACK.push(current);
    D2D1_MATRIX_3X2_F next = D2D1::Matrix3x2F(transform[0], transform[1], transform[2], transform[3], transform[4], transform[5]);
    rt->SetTransform(next * current);
    return NSERROR_OK;
}

static nserror pop_transform(const struct redraw_context *ctx) {
    ID2D1RenderTarget *rt = D2D_RT;
    if (!rt) return NSERROR_INVALID;
    if (HAS_STACK && !TRANSFORM_STACK.empty()) {
        rt->SetTransform(TRANSFORM_STACK.top());
        TRANSFORM_STACK.pop();
    }
    return NSERROR_OK;
}

static nserror linear_gradient(const struct redraw_context *ctx, const float *path_data, unsigned int path_len, const float transform[6], float x0, float y0, float x1, float y1, const struct gradient_stop *stops, unsigned int stop_count) {
    ID2D1RenderTarget *rt = D2D_RT;
    if (!rt) return NSERROR_INVALID;
    std::vector<D2D1_GRADIENT_STOP> d2d_stops;
    for (unsigned int i = 0; i < stop_count; i++) d2d_stops.push_back({stops[i].offset, d2d_color(stops[i].color)});
    ID2D1GradientStopCollection *stop_collection;
    if (SUCCEEDED(rt->CreateGradientStopCollection(d2d_stops.data(), stop_count, &stop_collection))) {
        ID2D1LinearGradientBrush *brush;
        if (SUCCEEDED(rt->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(D2D1::Point2F(x0, y0), D2D1::Point2F(x1, y1)), stop_collection, &brush))) {
            ID2D1PathGeometry *geometry = (path_data && path_len > 0) ? create_geometry_from_raw(rt, path_data, path_len) : NULL;

            rt->PushAxisAlignedClip(D2D_CLIP, D2D1_ANTIALIAS_MODE_ALIASED);
            if (geometry) {
                D2D1_MATRIX_3X2_F old_transform;
                rt->GetTransform(&old_transform);
                if (transform) {
                    D2D1_MATRIX_3X2_F d2d_transform = D2D1::Matrix3x2F(transform[0], transform[1], transform[2], transform[3], transform[4], transform[5]);
                    rt->SetTransform(d2d_transform * old_transform);
                }
                rt->FillGeometry(geometry, brush);
                rt->SetTransform(old_transform);
                geometry->Release();
            } else {
                rt->FillRectangle(D2D_CLIP, brush);
            }
            rt->PopAxisAlignedClip();
            brush->Release();
        }
        stop_collection->Release();
    }
    return NSERROR_OK;
}

static nserror path_begin(const struct redraw_context *ctx) { if (HAS_PATH) STATEFUL_PATH.clear(); return NSERROR_OK; }
static nserror path_move_to(const struct redraw_context *ctx, float x, float y) { if (HAS_PATH) STATEFUL_PATH.push_back({PLOTTER_PATH_MOVE, x, y}); return NSERROR_OK; }
static nserror path_line_to(const struct redraw_context *ctx, float x, float y) { if (HAS_PATH) STATEFUL_PATH.push_back({PLOTTER_PATH_LINE, x, y}); return NSERROR_OK; }
static nserror path_bezier_to(const struct redraw_context *ctx, float x1, float y1, float x2, float y2, float x3, float y3) { if (HAS_PATH) STATEFUL_PATH.push_back({PLOTTER_PATH_BEZIER, x1, y1, x2, y2, x3, y3}); return NSERROR_OK; }
static nserror path_close(const struct redraw_context *ctx) { if (HAS_PATH) STATEFUL_PATH.push_back({PLOTTER_PATH_CLOSE}); return NSERROR_OK; }

static nserror path_fill(const struct redraw_context *ctx, const plot_style_t *pstyle, const float transform[6]) {
    ID2D1RenderTarget *rt = D2D_RT;
    if (!rt || !HAS_PATH) return NSERROR_INVALID;
    ID2D1PathGeometry *geometry = create_geometry_from_commands(rt, STATEFUL_PATH);
    if (geometry) {
        D2D1_MATRIX_3X2_F old_transform;
        rt->GetTransform(&old_transform);
        if (transform) {
            D2D1_MATRIX_3X2_F d2d_transform = D2D1::Matrix3x2F(transform[0], transform[1], transform[2], transform[3], transform[4], transform[5]);
            rt->SetTransform(d2d_transform * old_transform);
        }

        rt->PushAxisAlignedClip(D2D_CLIP, D2D1_ANTIALIAS_MODE_ALIASED);
        ID2D1SolidColorBrush *brush;
        if (SUCCEEDED(rt->CreateSolidColorBrush(d2d_color(pstyle->fill_colour, pstyle->fill_opacity), &brush))) {
            rt->FillGeometry(geometry, brush);
            brush->Release();
        }
        rt->PopAxisAlignedClip();

        rt->SetTransform(old_transform);
        geometry->Release();
    }
    return NSERROR_OK;
}

static nserror path_stroke(const struct redraw_context *ctx, const plot_style_t *pstyle, const float transform[6]) {
    ID2D1RenderTarget *rt = D2D_RT;
    if (!rt || !HAS_PATH) return NSERROR_INVALID;
    ID2D1PathGeometry *geometry = create_geometry_from_commands(rt, STATEFUL_PATH);
    if (geometry) {
        D2D1_MATRIX_3X2_F old_transform;
        rt->GetTransform(&old_transform);
        if (transform) {
            D2D1_MATRIX_3X2_F d2d_transform = D2D1::Matrix3x2F(transform[0], transform[1], transform[2], transform[3], transform[4], transform[5]);
            rt->SetTransform(d2d_transform * old_transform);
        }

        rt->PushAxisAlignedClip(D2D_CLIP, D2D1_ANTIALIAS_MODE_ALIASED);
        ID2D1SolidColorBrush *brush;
        if (SUCCEEDED(rt->CreateSolidColorBrush(d2d_color(pstyle->stroke_colour, pstyle->stroke_opacity), &brush))) {
            rt->DrawGeometry(geometry, brush, plot_style_fixed_to_float(pstyle->stroke_width));
            brush->Release();
        }
        rt->PopAxisAlignedClip();

        rt->SetTransform(old_transform);
        geometry->Release();
    }
    return NSERROR_OK;
}

extern "C" void nsws_d2d_set_rt(ID2D1RenderTarget *rt) {
    d2d_rt_override = rt;
}

extern "C" const struct plotter_table win_plotters_d2d = {
    clip,
    arc,
    disc,
    line,
    rectangle,
    polygon,
    path,
    NULL, // finalise
    path_begin,
    path_move_to,
    path_line_to,
    path_bezier_to,
    path_close,
    path_fill,
    path_stroke,
    bitmap,
    text,
    NULL, // group_start
    NULL, // group_end
    NULL, // flush
    push_transform,
    pop_transform,
    linear_gradient,
    NULL, // radial_gradient
    true, // option_knockout
};

#endif
