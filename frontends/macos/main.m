#import <Cocoa/Cocoa.h>
#import "WispApp.h"
#import "WispWindow.h"
#include <wisp/wisp.h>
#include <wisp/browser_window.h>
#include <wisp/utils/log.h>
#include <wisp/utils/nsurl.h>
#include <wisp/utils/nsoption.h>
#include "macos/schedule.h"

extern struct gui_window_table *macos_window_table;
extern struct gui_fetch_table *macos_fetch_table;
extern struct gui_audio_table *macos_audio_table;
extern struct gui_bitmap_table *macos_bitmap_table;
extern struct gui_layout_table *macos_layout_table;

static WispApp *macos_app_delegate = nil;

static void macos_task_queue_wake(void) {
    [macos_app_delegate wake];
}

static struct gui_misc_table macos_misc_table = {
    .schedule = macos_schedule,
    .task_queue_wake = macos_task_queue_wake,
};

int main(int argc, char *argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        macos_app_delegate = [[WispApp alloc] init];
        [app setDelegate:macos_app_delegate];

        struct wisp_table macos_table = {
            .misc = &macos_misc_table,
            .window = macos_window_table,
            .fetch = macos_fetch_table,
            .audio = macos_audio_table,
            .bitmap = macos_bitmap_table,
            .layout = macos_layout_table,
        };

        if (wisp_register(&macos_table) != NSERROR_OK) {
            return 1;
        }

        /* Initialize options */
        nsoption_init(NULL, &nsoptions, &nsoptions_default);
        nsoption_commandline(&argc, argv, nsoptions);

        if (wisp_init(NULL) != NSERROR_OK) {
            return 1;
        }

        const char *addr = "https://www.google.com";
        if (argc > 1) {
            addr = argv[1];
        } else if (nsoption_charp(homepage_url)) {
            addr = nsoption_charp(homepage_url);
        }

        nsurl *url;
        if (nsurl_create(addr, &url) == NSERROR_OK) {
            browser_window_create(BW_CREATE_HISTORY, url, NULL, NULL, NULL);
            nsurl_unref(url);
        }

        [app run];
    }
    return 0;
}
