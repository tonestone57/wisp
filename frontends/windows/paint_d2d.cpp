#include <d2d1.h>
#include <dwrite.h>

extern "C" {
#include "wisp/plotters.h"
#include "wisp/browser_window.h"
#include "windows/window.h"
}

extern "C" const struct plotter_table win_plotters_d2d;
extern "C" void nsws_d2d_set_rt(ID2D1RenderTarget *rt);

extern "C" void nsws_drawable_paint_d2d(struct gui_window *gw, HWND hwnd) {
    ID2D1HwndRenderTarget *rt = (ID2D1HwndRenderTarget *)gw->d2d_rt;
    struct rect clip;
    RECT rc;

    rt->BeginDraw();
    rt->Clear(D2D1::ColorF(D2D1::ColorF::White));

    nsws_d2d_set_rt(rt);
    struct redraw_context ctx = {
        .interactive = true,
        .background_images = true,
        .plot = &win_plotters_d2d,
        .priv = gw
    };

    GetUpdateRect(hwnd, &rc, FALSE);
    clip.x0 = rc.left;
    clip.y0 = rc.top;
    clip.x1 = rc.right;
    clip.y1 = rc.bottom;

    browser_window_redraw(gw->bw, -gw->scrollx, -gw->scrolly, &clip, &ctx);

    rt->EndDraw();
    ValidateRect(hwnd, NULL);
}
