#include "wisp/plotters.h"
#include "wisp/ipc/ipc.h"
#include "wisp/ipc/task_queue.h"
#include <stdlib.h>

// In Phase 0.5 we use the task queue instead of directly writing to IPC socket
// to avoid blocking the layout engine thread.
static struct task_queue *renderer_plotter_queue = NULL;

struct plot_rect_ctx {
    struct rect r;
    plot_style_t s;
};

static void do_plot_rectangle(void *vctx) {
    struct plot_rect_ctx *ctx = (struct plot_rect_ctx *)vctx;
    // Stub for Phase 0.5: this would reconstruct the UI layer draw command
    free(ctx);
}

static nserror ipc_plot_clip(const struct redraw_context *ctx, const struct rect *clip) {
    (void)ctx;
    (void)clip;
    if (!renderer_plotter_queue) return NSERROR_OK;
    return NSERROR_OK;
}

static nserror ipc_plot_arc(const struct redraw_context *ctx, const plot_style_t *style, int x, int y, int radius, int angle1, int angle2) {
    (void)ctx; (void)style; (void)x; (void)y; (void)radius; (void)angle1; (void)angle2;
    return NSERROR_OK;
}

static nserror ipc_plot_disc(const struct redraw_context *ctx, const plot_style_t *style, int x, int y, int radius) {
    (void)ctx; (void)style; (void)x; (void)y; (void)radius;
    return NSERROR_OK;
}

static nserror ipc_plot_line(const struct redraw_context *ctx, const plot_style_t *style, const struct rect *line) {
    (void)ctx; (void)style; (void)line;
    return NSERROR_OK;
}

static nserror ipc_plot_rectangle(const struct redraw_context *ctx, const plot_style_t *style, const struct rect *rect) {
    (void)ctx;
    if (renderer_plotter_queue && style && rect) {
        struct plot_rect_ctx *pctx = calloc(1, sizeof(*pctx));
        if (pctx) {
            pctx->r = *rect;
            pctx->s = *style;
            task_queue_push(renderer_plotter_queue, do_plot_rectangle, pctx);
        }
    }
    return NSERROR_OK;
}

static nserror ipc_plot_polygon(const struct redraw_context *ctx, const plot_style_t *style, const int *p, unsigned int n) {
    (void)ctx; (void)style; (void)p; (void)n;
    return NSERROR_OK;
}

static nserror ipc_plot_path(const struct redraw_context *ctx, const plot_style_t *style, const float *p, unsigned int n, const float transform[6]) {
    (void)ctx; (void)style; (void)p; (void)n; (void)transform;
    return NSERROR_OK;
}

static nserror ipc_plot_bitmap(const struct redraw_context *ctx, struct bitmap *bitmap, int x, int y, int width, int height, colour bg, bitmap_flags_t flags) {
    (void)ctx; (void)bitmap; (void)x; (void)y; (void)width; (void)height; (void)bg; (void)flags;
    return NSERROR_OK;
}

static nserror ipc_plot_text(const struct redraw_context *ctx, const struct plot_font_style *fstyle, int x, int y, const char *text, size_t length) {
    (void)ctx; (void)fstyle; (void)x; (void)y; (void)text; (void)length;
    return NSERROR_OK;
}

static nserror ipc_plot_flush(const struct redraw_context *ctx) {
    (void)ctx;
    return NSERROR_OK;
}

const struct plotter_table ipc_plotter = {
    .clip = ipc_plot_clip,
    .arc = ipc_plot_arc,
    .disc = ipc_plot_disc,
    .line = ipc_plot_line,
    .rectangle = ipc_plot_rectangle,
    .polygon = ipc_plot_polygon,
    .path = ipc_plot_path,
    .bitmap = ipc_plot_bitmap,
    .text = ipc_plot_text,
    .flush = ipc_plot_flush
};

void ipc_plotter_set_queue(struct task_queue *queue) {
    renderer_plotter_queue = queue;
}
