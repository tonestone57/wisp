#import "gui.h"
#include <wisp/plot_style.h>
#include <wisp/bitmap.h>

#define MAX_CTX_STACK 8
static CGContextRef ctx_stack[MAX_CTX_STACK];
static int ctx_stack_ptr = 0;

static CGContextRef current_ctx(void) {
    if (ctx_stack_ptr > 0) return ctx_stack[ctx_stack_ptr - 1];
    return NULL;
}

static nserror macos_plot_clipping(const struct rect *clip) {
    CGContextRef ctx = current_ctx();
    if (!ctx) return NSERROR_OK;
    CGContextClipToRect(ctx, CGRectMake(clip->x0, clip->y0, clip->x1 - clip->x0, clip->y1 - clip->y0));
    return NSERROR_OK;
}

static void macos_set_stroke(CGContextRef ctx, const plot_style_t *style) {
    NSColor *stroke = [NSColor colorWithDeviceRed:((style->stroke_colour >> 16) & 0xFF) / 255.0
                                            green:((style->stroke_colour >> 8) & 0xFF) / 255.0
                                             blue:(style->stroke_colour & 0xFF) / 255.0
                                            alpha:1.0];
    CGContextSetStrokeColorWithColor(ctx, stroke.CGColor);
    CGContextSetLineWidth(ctx, style->stroke_width);
}

static void macos_set_fill(CGContextRef ctx, const plot_style_t *style) {
    NSColor *fill = [NSColor colorWithDeviceRed:((style->fill_colour >> 16) & 0xFF) / 255.0
                                          green:((style->fill_colour >> 8) & 0xFF) / 255.0
                                           blue:(style->fill_colour & 0xFF) / 255.0
                                          alpha:1.0];
    CGContextSetFillColorWithColor(ctx, fill.CGColor);
}

static nserror macos_plot_rectangle(const struct rect *rect, const plot_style_t *style) {
    CGContextRef ctx = current_ctx();
    if (!ctx) return NSERROR_OK;
    CGRect r = CGRectMake(rect->x0, rect->y0, rect->x1 - rect->x0, rect->y1 - rect->y0);

    if (style->fill_type != PLOT_OP_TYPE_NONE) {
        macos_set_fill(ctx, style);
        CGContextFillRect(ctx, r);
    }

    if (style->stroke_type != PLOT_OP_TYPE_NONE) {
        macos_set_stroke(ctx, style);
        CGContextStrokeRect(ctx, r);
    }
    return NSERROR_OK;
}

static nserror macos_plot_line(int x0, int y0, int x1, int y1, const plot_style_t *style) {
    CGContextRef ctx = current_ctx();
    if (!ctx) return NSERROR_OK;
    macos_set_stroke(ctx, style);
    CGContextBeginPath(ctx);
    CGContextMoveToPoint(ctx, x0, y0);
    CGContextAddLineToPoint(ctx, x1, y1);
    CGContextStrokePath(ctx);
    return NSERROR_OK;
}

static nserror macos_plot_polygon(const int *p, unsigned int n, const plot_style_t *style) {
    CGContextRef ctx = current_ctx();
    if (!ctx || n < 3) return NSERROR_OK;

    CGContextBeginPath(ctx);
    CGContextMoveToPoint(ctx, p[0], p[1]);
    for (unsigned int i = 1; i < n; i++) {
        CGContextAddLineToPoint(ctx, p[i*2], p[i*2+1]);
    }
    CGContextClosePath(ctx);

    if (style->fill_type != PLOT_OP_TYPE_NONE) {
        macos_set_fill(ctx, style);
        CGContextFillPath(ctx);
    }
    if (style->stroke_type != PLOT_OP_TYPE_NONE) {
        macos_set_stroke(ctx, style);
        CGContextStrokePath(ctx);
    }
    return NSERROR_OK;
}

static nserror macos_plot_text(const struct rect *clip, const plot_font_style_t *fstyle, int x, int y, const char *text, size_t length) {
    if (!current_ctx()) return NSERROR_OK;
    @autoreleasepool {
        NSString *nsStr = [[NSString alloc] initWithBytes:text length:length encoding:NSUTF8StringEncoding];
        if (!nsStr) return NSERROR_OK;

        NSFont *font = [NSFont systemFontOfSize:fstyle->size / 1000.0];
        NSColor *color = [NSColor colorWithDeviceRed:((fstyle->foreground >> 16) & 0xFF) / 255.0
                                               green:((fstyle->foreground >> 8) & 0xFF) / 255.0
                                                blue:(fstyle->foreground & 0xFF) / 255.0
                                               alpha:1.0];

        NSDictionary *attrs = @{NSFontAttributeName: font, NSForegroundColorAttributeName: color};
        [nsStr drawAtPoint:NSMakePoint(x, y - [font ascender]) withAttributes:attrs];
    }
    return NSERROR_OK;
}

static nserror macos_plot_bitmap(int x, int y, int width, int height, struct bitmap *bitmap, color bg, unsigned int flags) {
    if (!current_ctx()) return NSERROR_OK;
    struct gui_bitmap *bm = (struct gui_bitmap *)bitmap;
    if (!bm || !bm->rep) return NSERROR_OK;

    CGRect r = CGRectMake(x, y, width, height);
    [bm->rep drawInRect:r];
    return NSERROR_OK;
}

static struct gui_plot_table plot_table = {
    .clipping = macos_plot_clipping,
    .rectangle = macos_plot_rectangle,
    .line = macos_plot_line,
    .polygon = macos_plot_polygon,
    .text = macos_plot_text,
    .bitmap = macos_plot_bitmap,
};

struct gui_plot_table *macos_plot_table = &plot_table;

void macos_plot_push_context(CGContextRef ctx) {
    if (ctx_stack_ptr < MAX_CTX_STACK) {
        ctx_stack[ctx_stack_ptr++] = ctx;
    }
}

void macos_plot_pop_context(void) {
    if (ctx_stack_ptr > 0) {
        ctx_stack_ptr--;
    }
}
