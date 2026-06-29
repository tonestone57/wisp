#import <Cocoa/Cocoa.h>
#include <wisp/plotters.h>
#include <wisp/plot_style.h>

static CGContextRef current_ctx = NULL;

static nserror macos_plot_clipping(const struct rect *clip) {
    if (!current_ctx) return NSERROR_OK;
    CGContextClipToRect(current_ctx, CGRectMake(clip->x0, clip->y0, clip->x1 - clip->x0, clip->y1 - clip->y0));
    return NSERROR_OK;
}

static nserror macos_plot_rectangle(const struct rect *rect, const plot_style_t *style) {
    if (!current_ctx) return NSERROR_OK;
    CGRect r = CGRectMake(rect->x0, rect->y0, rect->x1 - rect->x0, rect->y1 - rect->y0);

    if (style->fill_type != PLOT_OP_TYPE_NONE) {
        NSColor *fill = [NSColor colorWithDeviceRed:((style->fill_colour >> 16) & 0xFF) / 255.0
                                              green:((style->fill_colour >> 8) & 0xFF) / 255.0
                                               blue:(style->fill_colour & 0xFF) / 255.0
                                              alpha:1.0];
        CGContextSetFillColorWithColor(current_ctx, fill.CGColor);
        CGContextFillRect(current_ctx, r);
    }

    if (style->stroke_type != PLOT_OP_TYPE_NONE) {
        NSColor *stroke = [NSColor colorWithDeviceRed:((style->stroke_colour >> 16) & 0xFF) / 255.0
                                                green:((style->stroke_colour >> 8) & 0xFF) / 255.0
                                                 blue:(style->stroke_colour & 0xFF) / 255.0
                                                alpha:1.0];
        CGContextSetStrokeColorWithColor(current_ctx, stroke.CGColor);
        CGContextSetLineWidth(current_ctx, style->stroke_width);
        CGContextStrokeRect(current_ctx, r);
    }
    return NSERROR_OK;
}

static nserror macos_plot_text(const struct rect *clip, const plot_font_style_t *fstyle, int x, int y, const char *text, size_t length) {
    if (!current_ctx) return NSERROR_OK;
    @autoreleasepool {
        NSString *nsStr = [[NSString alloc] initWithBytes:text length:length encoding:NSUTF8StringEncoding];
        if (!nsStr) return NSERROR_OK;

        NSFont *font = [NSFont systemFontOfSize:fstyle->size / 1000.0];
        NSColor *color = [NSColor colorWithDeviceRed:((fstyle->foreground >> 16) & 0xFF) / 255.0
                                               green:((fstyle->foreground >> 8) & 0xFF) / 255.0
                                                blue:(fstyle->foreground & 0xFF) / 255.0
                                               alpha:1.0];

        NSDictionary *attrs = @{NSFontAttributeName: font, NSForegroundColorAttributeName: color};

        /* Core Graphics flip logic would be needed here for proper text orientation */
        [nsStr drawAtPoint:NSMakePoint(x, y) withAttributes:attrs];
    }
    return NSERROR_OK;
}

static struct gui_plot_table plot_table = {
    .clipping = macos_plot_clipping,
    .rectangle = macos_plot_rectangle,
    .text = macos_plot_text,
    /* More primitives like line, polygon, path would go here */
};

struct gui_plot_table *macos_plot_table = &plot_table;

void macos_plot_set_context(CGContextRef ctx) {
    current_ctx = ctx;
}
