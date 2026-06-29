#import "gui.h"
#include <wisp/wisp.h>
#include <wisp/utils/log.h>
#include <wisp/utils/nsurl.h>
#include <wisp/utils/nsoption.h>
#include "macos/schedule.h"
#import "WispApp.h"

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

        macos_fetch_cleanup();
        wisp_exit();
    }
    return 0;
}
