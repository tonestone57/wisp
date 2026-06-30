#import "WispView.h"
#import "gui.h"

@implementation WispView

- (instancetype)initWithFrame:(NSRect)frameRect browserWindow:(struct browser_window *)bw {
    self = [super initWithFrame:frameRect];
    if (self) {
        _bw = bw;
    }
    return self;
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

            CGContextSaveGState(ctx);
            CGRect cg_rect = CGRectMake(tile_clip.x0, tile_clip.y0,
                                        tile_clip.x1 - tile_clip.x0, tile_clip.y1 - tile_clip.y0);
            CGContextClipToRect(ctx, cg_rect);

            browser_window_redraw(_bw, 0, 0, &tile_clip, &redraw_ctx);

            CGContextRestoreGState(ctx);
        }
    }

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
