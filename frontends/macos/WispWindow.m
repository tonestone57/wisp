#import "WispWindow.h"

@implementation WispWindow

- (instancetype)initWithBrowserWindow:(struct browser_window *)bw {
    NSRect contentRect = NSMakeRect(0, 0, 1024, 768);
    self = [super initWithContentRect:contentRect
                             styleMask:(NSWindowStyleMaskTitled |
                                        NSWindowStyleMaskClosable |
                                        NSWindowStyleMaskMiniaturizable |
                                        NSWindowStyleMaskResizable)
                               backing:NSBackingStoreBuffered
                                 defer:NO];
    if (self) {
        [self setTitle:@"Wisp"];
        _wispView = [[WispView alloc] initWithFrame:contentRect browserWindow:bw];
        [self setContentView:_wispView];
        [self makeFirstResponder:_wispView];
        [self center];
    }
    return self;
}

@end
