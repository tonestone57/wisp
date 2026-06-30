#import "WispApp.h"
#include "macos/schedule.h"

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
    if (next < 5) next = 5;
    [self scheduleNextStep:next];
}

- (void)wake {
    dispatch_async(dispatch_get_main_queue(), ^{
        [self runStep];
    });
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    /* Cleanup is handled in main.m after [app run] returns */
}

@end
