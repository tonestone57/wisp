#import <Cocoa/Cocoa.h>
#import "WispView.h"

@interface WispWindow : NSWindow
@property (nonatomic, readonly) WispView *wispView;
- (instancetype)initWithBrowserWindow:(struct browser_window *)bw;
@end
