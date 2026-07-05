#ifndef WISP_DESKTOP_PLOT_BLEND2D_H_
#define WISP_DESKTOP_PLOT_BLEND2D_H_

#include <blend2d.h>
#include "wisp/plotters.h"

/**
 * Blend2D context wrapper for native interop.
 */
struct blend2d_context {
    BLContextCore *bl_ctx;

    /**
     * Native text rendering callback.
     * If NULL, Blend2D text rendering will be used (if implemented).
     */
    nserror (*native_text_handler)(const struct redraw_context *ctx, const plot_font_style_t *fstyle, int x, int y, const char *text, size_t length);

    /**
     * Private data for the native text handler (e.g., HDC or cairo_t).
     */
    void *native_priv;
};

extern const struct plotter_table blend2d_plotters;

#endif
