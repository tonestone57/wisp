#import <Cocoa/Cocoa.h>
struct browser_window;

@interface WispView : NSView {
    void *_blend2d_data;
    int _blend2d_width;
    int _blend2d_height;
}
@property (nonatomic, assign) struct browser_window *bw;
- (instancetype)initWithFrame:(NSRect)frameRect browserWindow:(struct browser_window *)bw;
@end
