#import "WispApp.h"
#include "macos/schedule.h"
#include <wisp/wisp.h>

@implementation WispApp {
    NSTimer *_timer;
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    [self scheduleNextStep:10];
}

- (void)scheduleNextStep:(int)ms {
    if (_timer) {
        [_timer invalidate];
    }
    _timer = [NSTimer scheduledTimerWithTimeInterval:ms / 1000.0
                                             target:self
                                           selector:@selector(runStep)
                                           userInfo:nil
                                            repeats:NO];
}

- (void)runStep {
    int next = schedule_run();
    if (next < 0) next = 100;
    if (next < 10) next = 10;
    [self scheduleNextStep:next];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    wisp_exit();
}

@end
