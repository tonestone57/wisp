#ifndef WISP_MACOS_GUI_H
#define WISP_MACOS_GUI_H

#import <Cocoa/Cocoa.h>
#include <wisp/window.h>
#include <wisp/browser_window.h>
#include <wisp/plotters.h>
#import "WispWindow.h"

struct gui_window {
    WispWindow *__strong win;
    struct browser_window *bw;
};

struct gui_bitmap {
    NSBitmapImageRep *__strong rep;
    bool opaque;
};

/* Plotting context management */
void macos_plot_push_context(CGContextRef ctx);
void macos_plot_pop_context(void);

/* Table accessors */
extern struct gui_window_table *macos_window_table;
extern struct gui_plot_table *macos_plot_table;
extern struct gui_fetch_table *macos_fetch_table;
extern struct gui_audio_table *macos_audio_table;
extern struct gui_bitmap_table *macos_bitmap_table;
extern struct gui_layout_table *macos_layout_table;

/* Cleanup */
void macos_fetch_cleanup(void);

#endif
