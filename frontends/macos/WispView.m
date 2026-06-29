#import "WispView.h"
#include <wisp/browser_window.h>
#include <wisp/mouse.h>
#include <wisp/keypress.h>
#include <wisp/plotters.h>

extern void macos_plot_set_context(CGContextRef ctx);
extern struct gui_plot_table *macos_plot_table;

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

- (void)drawRect:(NSRect)dirtyRect {
    CGContextRef ctx = [[NSGraphicsContext currentContext] CGContext];
    macos_plot_set_context(ctx);

    struct rect wisp_rect = {
        .x0 = (int)NSMinX(dirtyRect),
        .y0 = (int)NSMinY(dirtyRect),
        .x1 = (int)NSMaxX(dirtyRect),
        .y1 = (int)NSMaxY(dirtyRect)
    };

    browser_window_redraw(_bw, &wisp_rect, macos_plot_table);

    macos_plot_set_context(NULL);
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

        switch (key) {
            case NSUpArrowFunctionKey: ns_key = NS_KEY_UP; break;
            case NSDownArrowFunctionKey: ns_key = NS_KEY_DOWN; break;
            case NSLeftArrowFunctionKey: ns_key = NS_KEY_LEFT; break;
            case NSRightArrowFunctionKey: ns_key = NS_KEY_RIGHT; break;
            case 0x7F: ns_key = NS_KEY_DELETE_LEFT; break;
            case NSDeleteFunctionKey: ns_key = NS_KEY_DELETE_RIGHT; break;
            case NSTabCharacter: ns_key = NS_KEY_TAB; break;
            case NSCarriageReturnCharacter:
            case NSNewlineCharacter: ns_key = NS_KEY_CR; break;
            case 0x1B: ns_key = NS_KEY_ESCAPE; break;
        }

        browser_window_key_press(_bw, ns_key);
    }
}

@end
