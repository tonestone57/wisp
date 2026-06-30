#import "gui.h"
#include <wisp/mouse.h>

static struct gui_window *macos_window_create(struct browser_window *bw, struct gui_window *existing, gui_window_create_flags flags) {
    struct gui_window *gw = calloc(1, sizeof(struct gui_window));
    if (!gw) return NULL;

    gw->bw = bw;
    gw->win = [[WispWindow alloc] initWithBrowserWindow:bw];
    [gw->win makeKeyAndOrderFront:nil];

    return gw;
}

static void macos_window_destroy(struct gui_window *gw) {
    [gw->win close];
    gw->win = nil;
    free(gw);
}

static nserror macos_window_invalidate(struct gui_window *gw, const struct rect *rect) {
    if (rect) {
        NSRect r = NSMakeRect(rect->x0, rect->y0, rect->x1 - rect->x0, rect->y1 - rect->y0);
        [gw->win.wispView setNeedsDisplayInRect:r];
    } else {
        [gw->win.wispView setNeedsDisplay:YES];
    }
    return NSERROR_OK;
}

static bool macos_window_get_scroll(struct gui_window *gw, int *sx, int *sy) {
    *sx = 0; *sy = 0;
    return true;
}

static nserror macos_window_set_scroll(struct gui_window *gw, const struct rect *rect) {
    return NSERROR_OK;
}

static nserror macos_window_get_dimensions(struct gui_window *gw, int *width, int *height) {
    NSRect bounds = gw->win.wispView.bounds;
    *width = (int)bounds.size.width;
    *height = (int)bounds.size.height;
    return NSERROR_OK;
}

static nserror macos_window_get_scrollbar_width(struct gui_window *gw, int *width) {
    *width = 15;
    return NSERROR_OK;
}

static nserror macos_window_event(struct gui_window *gw, enum gui_window_event event) {
    return NSERROR_OK;
}

static void macos_window_set_title(struct gui_window *gw, const char *title) {
    [gw->win setTitle:[NSString stringWithUTF8String:title]];
}

static void macos_window_set_status(struct gui_window *gw, const char *status) {
    if (status) {
        NSLog(@"Status: %s", status);
    }
}

static void macos_window_set_pointer(struct gui_window *gw, enum gui_pointer_shape shape) {
    NSCursor *cursor;
    switch (shape) {
        case GUI_POINTER_CARET:
        case GUI_POINTER_MENU:
        case GUI_POINTER_PROGRESS:
        case GUI_POINTER_DEFAULT: cursor = [NSCursor arrowCursor]; break;
        case GUI_POINTER_POINT:   cursor = [NSCursor pointingHandCursor]; break;
        case GUI_POINTER_TEXT:    cursor = [NSCursor IBeamCursor]; break;
        case GUI_POINTER_MOVE:    cursor = [NSCursor openHandCursor]; break;
        default:                  cursor = [NSCursor arrowCursor]; break;
    }
    [cursor set];
}

static void macos_window_set_icon(struct gui_window *gw, struct hlcache_handle *icon) {
    if (!icon) return;
    struct bitmap *bitmap = hlcache_handle_get_bitmap(icon);
    if (!bitmap) return;

    struct gui_bitmap *bm = (struct gui_bitmap *)bitmap;
    if (bm && bm->rep) {
        NSImage *image = [[NSImage alloc] initWithSize:bm->rep.size];
        [image addRepresentation:bm->rep];
        [NSApp setApplicationIconImage:image];
    }
}

static struct gui_window_table window_table = {
    .create = macos_window_create,
    .destroy = macos_window_destroy,
    .invalidate = macos_window_invalidate,
    .get_scroll = macos_window_get_scroll,
    .set_scroll = macos_window_set_scroll,
    .get_dimensions = macos_window_get_dimensions,
    .get_scrollbar_width = macos_window_get_scrollbar_width,
    .event = macos_window_event,
    .set_title = macos_window_set_title,
    .set_status = macos_window_set_status,
    .set_pointer = macos_window_set_pointer,
    .set_icon = macos_window_set_icon,
};

struct gui_window_table *macos_window_table = &window_table;
