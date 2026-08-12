#import "WispView.h"
#import "gui.h"
#include <wisp/browser.h>
#include <wisp/utils/nsoption.h>

#ifdef WITH_BLEND2D
#include <blend2d/blend2d.h>
#include "wisp/desktop/plot_blend2d.h"

extern nserror macos_plot_text_ns(const struct redraw_context *ctx, const plot_font_style_t *fstyle, int x, int y, const char *text, size_t length);
#endif

typedef struct {
    struct rect tile_clip;
    float priority;
} macos_tile_task_t;

static int macos_tile_task_compare(const void *a, const void *b)
{
    const macos_tile_task_t *ta = (const macos_tile_task_t *)a;
    const macos_tile_task_t *tb = (const macos_tile_task_t *)b;
    if (ta->priority > tb->priority) return -1;
    if (ta->priority < tb->priority) return 1;
    return 0;
}

@implementation WispView

- (instancetype)initWithFrame:(NSRect)frameRect browserWindow:(struct browser_window *)bw {
    self = [super initWithFrame:frameRect];
    if (self) {
        _bw = bw;
        _blend2d_data = NULL;
        _blend2d_width = 0;
        _blend2d_height = 0;
    }
    return self;
}

- (void)dealloc {
    if (_blend2d_data) {
        free(_blend2d_data);
    }
    [super dealloc];
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)isFlipped {
    return YES;
}

- (void)viewDidEndLiveResize {
    [super viewDidEndLiveResize];
    browser_window_reformat(_bw, NO, (int)self.bounds.size.width, (int)self.bounds.size.height);
}

- (void)drawRect:(NSRect)dirtyRect {
    CGContextRef ctx = [[NSGraphicsContext currentContext] CGContext];
    int backend = nsoption_int(render_backend);

#ifdef WITH_BLEND2D
    bool use_blend2d = false;
    if (backend == OPTION_RENDER_BACKEND_BLEND2D) {
        use_blend2d = true;
    } else if (backend == OPTION_RENDER_BACKEND_AUTO) {
        /* On macOS, Blend2D is often faster than Core Graphics for complex paths */
        use_blend2d = true;
    }

    if (use_blend2d) {
        /* Render into a Bitmap Context using Blend2D */
        int width = (int)self.bounds.size.width;
        int height = (int)self.bounds.size.height;
        size_t bytesPerRow = width * 4;

        if (!_blend2d_data || _blend2d_width != width || _blend2d_height != height) {
            if (_blend2d_data) free(_blend2d_data);
            _blend2d_data = malloc(height * bytesPerRow);
            _blend2d_width = width;
            _blend2d_height = height;
        }

        if (_blend2d_data) {
            memset(_blend2d_data, 0, height * bytesPerRow);
            BLContextCore bl_ctx;
            BLImageCore bl_img;
            bl_image_init_as_from_data(&bl_img, width, height, BL_FORMAT_PRGB32, _blend2d_data, bytesPerRow, BL_DATA_ACCESS_RW, NULL, NULL);
            bl_context_init_as(&bl_ctx, &bl_img, NULL);

            /* Create a temporary CGContext that shares the same memory as Blend2D */
            CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
            CGContextRef bitmapCtx = CGBitmapContextCreate(_blend2d_data, width, height, 8, bytesPerRow, colorSpace, kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host);

            struct blend2d_context b2d_ctx = {
                .bl_ctx = &bl_ctx,
                .native_ctx = bitmapCtx,
                .native_text_handler = macos_plot_text_ns
            };

            struct redraw_context bl_ctx_ns = {
                .interactive = true,
                .background_images = true,
                .plot = &blend2d_plotters,
                .priv = &b2d_ctx,
            };

            struct rect full_clip = {0, 0, width, height};
            browser_window_redraw(_bw, 0, 0, &full_clip, &bl_ctx_ns);

            bl_context_end(&bl_ctx);
            bl_context_destroy(&bl_ctx);
            bl_image_destroy(&bl_img);

            CGImageRef image = CGBitmapContextCreateImage(bitmapCtx);

            /* Since it's flipped, we might need to handle orientation, but for now blit as is */
            CGContextDrawImage(ctx, CGRectMake(0, 0, width, height), image);

            CGImageRelease(image);
            CGContextRelease(bitmapCtx);
            CGColorSpaceRelease(colorSpace);
            return;
        }
    }
#endif

    macos_plot_push_context(ctx);

    /* Fixed-Tile Redraw Implementation */
    int tile_size = browser_get_tile_size();
    int rect_left = (int)NSMinX(dirtyRect);
    int rect_top = (int)NSMinY(dirtyRect);
    int rect_right = (int)NSMaxX(dirtyRect);
    int rect_bottom = (int)NSMaxY(dirtyRect);

    int x_start = rect_left - (rect_left % tile_size);
    int y_start = rect_top - (rect_top % tile_size);

    struct redraw_context redraw_ctx = {
        .interactive = true,
        .background_images = true,
        .plot = macos_plot_table,
        .priv = ctx
    };

    int v_x = 0; /* scrolled offset already applied in macOS view */
    int v_y = 0;
    int v_w = (int)self.bounds.size.width;
    int v_h = (int)self.bounds.size.height;

    /* Collect all tiles in the update region */
    int max_tiles = ((rect_right - x_start) / tile_size + 1) * ((rect_bottom - y_start) / tile_size + 1);
    macos_tile_task_t *tasks = (macos_tile_task_t *)malloc(sizeof(macos_tile_task_t) * max_tiles);
    if (!tasks) { macos_plot_pop_context(); return; }
    int task_count = 0;

    for (int ty = y_start; ty < rect_bottom; ty += tile_size) {
        int t_y0 = (ty > rect_top) ? ty : rect_top;
        int t_y1 = (ty + tile_size < rect_bottom) ? ty + tile_size : rect_bottom;

        for (int tx = x_start; tx < rect_right; tx += tile_size) {
            struct rect tile_clip;
            tile_clip.x0 = (tx > rect_left) ? tx : rect_left;
            tile_clip.y0 = t_y0;
            tile_clip.x1 = (tx + tile_size < rect_right) ? tx + tile_size : rect_right;
            tile_clip.y1 = t_y1;

            if (tile_clip.x0 >= tile_clip.x1 || tile_clip.y0 >= tile_clip.y1)
                continue;

            tasks[task_count].tile_clip = tile_clip;
            tasks[task_count].priority = browser_calculate_tile_priority(tx, ty, v_x, v_y, v_w, v_h);
            task_count++;
        }
    }

    /* Sort tiles by priority to ensure visible/near ones are drawn first */
    qsort(tasks, task_count, sizeof(macos_tile_task_t), macos_tile_task_compare);

    /* Execute prioritized redraw loop */
    for (int i = 0; i < task_count; i++) {
        CGContextSaveGState(ctx);
        CGRect cg_rect = CGRectMake(tasks[i].tile_clip.x0, tasks[i].tile_clip.y0,
                                    tasks[i].tile_clip.x1 - tasks[i].tile_clip.x0,
                                    tasks[i].tile_clip.y1 - tasks[i].tile_clip.y0);
        CGContextClipToRect(ctx, cg_rect);

        browser_window_redraw(_bw, 0, 0, &tasks[i].tile_clip, &redraw_ctx);

        CGContextRestoreGState(ctx);
    }

    free(tasks);

    macos_plot_pop_context();
}

- (void)mouseDown:(NSEvent *)event {
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
    browser_window_mouse_click(_bw, BROWSER_MOUSE_PRESS_1, (int)p.x, (int)p.y);
}

- (void)mouseUp:(NSEvent *)event {
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
    browser_window_mouse_click(_bw, BROWSER_MOUSE_CLICK_1, (int)p.x, (int)p.y);
}

- (void)keyDown:(NSEvent *)event {
    NSString *chars = [event charactersIgnoringModifiers];
    if ([chars length] > 0) {
        unichar key = [chars characterAtIndex:0];
        uint32_t ns_key = key;

        NSEventModifierFlags modifiers = [event modifierFlags];
        bool shift = (modifiers & NSEventModifierFlagShift) != 0;

        switch (key) {
            case NSUpArrowFunctionKey: ns_key = NS_KEY_UP; break;
            case NSDownArrowFunctionKey: ns_key = NS_KEY_DOWN; break;
            case NSLeftArrowFunctionKey: ns_key = NS_KEY_LEFT; break;
            case NSRightArrowFunctionKey: ns_key = NS_KEY_RIGHT; break;
            case 0x7F: ns_key = NS_KEY_DELETE_LEFT; break;
            case NSDeleteFunctionKey: ns_key = NS_KEY_DELETE_RIGHT; break;
            case NSTabCharacter: ns_key = shift ? NS_KEY_SHIFT_TAB : NS_KEY_TAB; break;
            case NSCarriageReturnCharacter:
            case NSNewlineCharacter: ns_key = NS_KEY_CR; break;
            case 0x1B: ns_key = NS_KEY_ESCAPE; break;
        }

        browser_window_key_press(_bw, ns_key);
    }
}

@end
