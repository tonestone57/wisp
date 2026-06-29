#import <Cocoa/Cocoa.h>
struct browser_window;

@interface WispView : NSView
@property (nonatomic, assign) struct browser_window *bw;
- (instancetype)initWithFrame:(NSRect)frameRect browserWindow:(struct browser_window *)bw;
@end
