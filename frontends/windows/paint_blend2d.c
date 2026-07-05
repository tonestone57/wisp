#include "wisp/utils/config.h"

#ifdef WITH_BLEND2D

#include <windows.h>
#include <blend2d/blend2d.h>
#include <assert.h>

#include "wisp/plotters.h"
#include "wisp/browser_window.h"
#include "windows/window.h"
#include "windows/plot.h"
#include "wisp/utils/log.h"
#include "wisp/desktop/plot_blend2d.h"

void nsws_drawable_paint_blend2d(struct gui_window *gw, HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    if (!hdc) return;

    RECT rc;
    GetClientRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    if (width <= 0 || height <= 0) {
        EndPaint(hwnd, &ps);
        return;
    }

    /* Create a DIB section for shared GDI/Blend2D memory */
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; /* Top-down */
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pixel_data = NULL;
    HBITMAP hbm = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &pixel_data, NULL, 0);
    if (!hbm) {
        EndPaint(hwnd, &ps);
        return;
    }

    HDC memHdc = CreateCompatibleDC(hdc);
    HGDIOBJ oldHbm = SelectObject(memHdc, hbm);

    BLImageCore img;
    bl_image_init_as_from_data(&img, width, height, BL_FORMAT_PRGB32, pixel_data, width * 4, NULL, NULL);

    BLContextCore ctx_bl;
    bl_context_init_as(&ctx_bl, &img, NULL);

    /* Clear background to white */
    bl_context_set_fill_style_rgba32(&ctx_bl, 0xFFFFFFFF);
    bl_context_fill_all(&ctx_bl);

    struct blend2d_context bl_wrap = {
        .bl_ctx = &ctx_bl,
        .native_text_handler = win_plotters.text,
        .native_priv = NULL /* HDC is set globally via plot_hdc */
    };

    struct redraw_context ctx = {
        .interactive = true,
        .background_images = true,
        .plot = &blend2d_plotters,
        .priv = &bl_wrap
    };

    /* win_plotters.text uses global plot_hdc. We point it to our memory DC. */
    HDC old_plot_hdc = plot_hdc;
    plot_hdc = memHdc;

    struct rect clip = { ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom };

    browser_window_redraw(gw->bw, -gw->scrollx, -gw->scrolly, &clip, &ctx);

    if (ctx.plot->finalise) ctx.plot->finalise(&ctx);

    bl_context_end(&ctx_bl);

    /* Blit the final composed buffer to the screen */
    BitBlt(hdc, 0, 0, width, height, memHdc, 0, 0, SRCCOPY);

    plot_hdc = old_plot_hdc;
    bl_context_destroy(&ctx_bl);
    bl_image_destroy(&img);
    SelectObject(memHdc, oldHbm);
    DeleteDC(memHdc);
    DeleteObject(hbm);
    EndPaint(hwnd, &ps);
}

#endif
