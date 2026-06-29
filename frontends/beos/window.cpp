/*
 * Copyright 2008 François Revol <mmu_man@users.sourceforge.net>
 * Copyright 2006 Daniel Silverstone <dsilvers@digital-scurf.org>
 * Copyright 2006 Rob Kendrick <rjek@rjek.com>
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define __STDBOOL_H__ 1
#include <AppDefs.h>
#include <BeBuild.h>
#include <Clipboard.h>
#include <Cursor.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <ScrollBar.h>
#include <String.h>
#include <TextView.h>
#include <View.h>
#include <Window.h>
#include <assert.h>
#include <stdlib.h>

extern "C" {
#include "utils/log.h"
#include "utils/nsoption.h"
#include "utils/nsurl.h"
#include "utils/utf8.h"
#include "utils/utils.h"
#include "wisp/browser.h"
#include "wisp/browser_window.h"
#include "wisp/clipboard.h"
#include "wisp/content_type.h"
#include "wisp/inttypes.h"
#include "wisp/keypress.h"
#include "wisp/mouse.h"
#include "wisp/plotters.h"
#include "wisp/url_db.h"
#include "wisp/window.h"
}

#include "beos/about.h"
#include "beos/font.h"
#include "beos/gui.h"
#include "beos/plotters.h"
#include "beos/scaffolding.h"
#include "beos/window.h"


class NSBrowserFrameView;

struct gui_window {
    nsbeos_scaffolding *scaffold;
    bool toplevel;
    struct browser_window *bw;

    struct {
        int pressed_x;
        int pressed_y;
        int state;
    } mouse;

    int caretx, carety, careth;
    gui_pointer_shape current_pointer;
    int last_x, last_y;

    NSBrowserFrameView *view;

    int32 pending_resizes;
    BRect pendingRedraw;

    struct gui_window *next, *prev;
};


static const rgb_color kWhiteColor = {255, 255, 255, 255};

static struct gui_window *window_list = 0;

static BString current_selection;
static BList current_selection_textruns;

static void nsbeos_window_expose_event(BView *view, gui_window *g, BMessage *message);
static void nsbeos_window_keypress_event(BView *view, gui_window *g, BMessage *event);
static void nsbeos_window_resize_event(BView *view, gui_window *g, BMessage *event);
static void nsbeos_window_moved_event(BView *view, gui_window *g, BMessage *event);
static void nsbeos_redraw_caret(struct gui_window *g);


NSBrowserFrameView::NSBrowserFrameView(BRect frame, struct gui_window *gui)
    : BView(frame, "NSBrowserFrameView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS),
      fGuiWindow(gui)
{
}


NSBrowserFrameView::~NSBrowserFrameView()
{
}


void NSBrowserFrameView::MessageReceived(BMessage *message)
{
    switch (message->what) {
    case B_SIMPLE_DATA:
    case B_ARGV_RECEIVED:
    case B_REFS_RECEIVED:
    case B_COPY:
    case B_CUT:
    case B_PASTE:
    case B_SELECT_ALL:
    case B_UI_SETTINGS_CHANGED:
    case B_NETPOSITIVE_OPEN_URL:
    case B_NETPOSITIVE_BACK:
    case B_NETPOSITIVE_FORWARD:
    case B_NETPOSITIVE_HOME:
    case B_NETPOSITIVE_RELOAD:
    case B_NETPOSITIVE_STOP:
    case B_NETPOSITIVE_DOWN:
    case B_NETPOSITIVE_UP:
    case 'back':
    case 'forw':
    case 'stop':
    case 'relo':
    case 'home':
    case 'urlc':
    case 'urle':
    case 'sear':
    case 'menu':
    case NO_ACTION:
    case HELP_OPEN_CONTENTS:
    case HELP_OPEN_GUIDE:
    case HELP_OPEN_INFORMATION:
    case HELP_OPEN_ABOUT:
    case HELP_LAUNCH_INTERACTIVE:
    case HISTORY_SHOW_LOCAL:
    case HISTORY_SHOW_GLOBAL:
    case HOTLIST_ADD_URL:
    case HOTLIST_SHOW:
    case COOKIES_SHOW:
    case COOKIES_DELETE:
    case BROWSER_PAGE:
    case BROWSER_PAGE_INFO:
    case BROWSER_PRINT:
    case BROWSER_NEW_WINDOW:
    case BROWSER_VIEW_SOURCE:
    case BROWSER_OBJECT:
    case BROWSER_OBJECT_INFO:
    case BROWSER_OBJECT_RELOAD:
    case BROWSER_OBJECT_SAVE:
    case BROWSER_OBJECT_EXPORT_SPRITE:
    case BROWSER_OBJECT_SAVE_URL_URI:
    case BROWSER_OBJECT_SAVE_URL_URL:
    case BROWSER_OBJECT_SAVE_URL_TEXT:
    case BROWSER_SAVE:
    case BROWSER_SAVE_COMPLETE:
    case BROWSER_EXPORT_DRAW:
    case BROWSER_EXPORT_TEXT:
    case BROWSER_SAVE_URL_URI:
    case BROWSER_SAVE_URL_URL:
    case BROWSER_SAVE_URL_TEXT:
    case HOTLIST_EXPORT:
    case HISTORY_EXPORT:
    case BROWSER_NAVIGATE_HOME:
    case BROWSER_NAVIGATE_BACK:
    case BROWSER_NAVIGATE_FORWARD:
    case BROWSER_NAVIGATE_UP:
    case BROWSER_NAVIGATE_RELOAD:
    case BROWSER_NAVIGATE_RELOAD_ALL:
    case BROWSER_NAVIGATE_STOP:
    case BROWSER_NAVIGATE_URL:
    case BROWSER_SCALE_VIEW:
    case BROWSER_FIND_TEXT:
    case BROWSER_IMAGES_FOREGROUND:
    case BROWSER_IMAGES_BACKGROUND:
    case BROWSER_BUFFER_ANIMS:
    case BROWSER_BUFFER_ALL:
    case BROWSER_SAVE_VIEW:
    case BROWSER_WINDOW_DEFAULT:
    case BROWSER_WINDOW_STAGGER:
    case BROWSER_WINDOW_COPY:
    case BROWSER_WINDOW_RESET:
    case TREE_NEW_FOLDER:
    case TREE_NEW_LINK:
    case TREE_EXPAND_ALL:
    case TREE_EXPAND_FOLDERS:
    case TREE_EXPAND_LINKS:
    case TREE_COLLAPSE_ALL:
    case TREE_COLLAPSE_FOLDERS:
    case TREE_COLLAPSE_LINKS:
    case TREE_SELECTION:
    case TREE_SELECTION_EDIT:
    case TREE_SELECTION_LAUNCH:
    case TREE_SELECTION_DELETE:
    case TREE_SELECT_ALL:
    case TREE_CLEAR_SELECTION:
    case TOOLBAR_BUTTONS:
    case TOOLBAR_ADDRESS_BAR:
    case TOOLBAR_THROBBER:
    case TOOLBAR_EDIT:
    case CHOICES_SHOW:
    case APPLICATION_QUIT:
        Window()->DetachCurrentMessage();
        nsbeos_pipe_message_top(message, NULL, fGuiWindow->scaffold);
        break;
    default:
        BView::MessageReceived(message);
    }
}


void NSBrowserFrameView::Draw(BRect updateRect)
{
    BMessage *message = new BMessage(_UPDATE_);
    message->AddRect("rect", updateRect);
    nsbeos_pipe_message(message, this, fGuiWindow);
}


void NSBrowserFrameView::FrameResized(float new_width, float new_height)
{
    BMessage *message = Window()->DetachCurrentMessage();
    atomic_add(&fGuiWindow->pending_resizes, 1);
    nsbeos_pipe_message(message, this, fGuiWindow);
    BView::FrameResized(new_width, new_height);
}


void NSBrowserFrameView::KeyDown(const char *bytes, int32 numBytes)
{
    BMessage *message = Window()->DetachCurrentMessage();
    nsbeos_pipe_message(message, this, fGuiWindow);
}


void NSBrowserFrameView::MouseDown(BPoint where)
{
    BMessage *message = Window()->DetachCurrentMessage();
    BPoint screenWhere;
    if (message->FindPoint("screen_where", &screenWhere) < B_OK) {
        screenWhere = ConvertToScreen(where);
        message->AddPoint("screen_where", screenWhere);
    }
    nsbeos_pipe_message(message, this, fGuiWindow);
}


void NSBrowserFrameView::MouseUp(BPoint where)
{
    BMessage *message = Window()->DetachCurrentMessage();
    BPoint screenWhere;
    if (message->FindPoint("screen_where", &screenWhere) < B_OK) {
        screenWhere = ConvertToScreen(where);
        message->AddPoint("screen_where", screenWhere);
    }
    nsbeos_pipe_message(message, this, fGuiWindow);
}


void NSBrowserFrameView::MouseMoved(BPoint where, uint32 transit, const BMessage *msg)
{
    if (transit != B_INSIDE_VIEW) {
        BView::MouseMoved(where, transit, msg);
        return;
    }
    BMessage *message = Window()->DetachCurrentMessage();
    nsbeos_pipe_message(message, this, fGuiWindow);
}


struct browser_window *nsbeos_get_browser_window(struct gui_window *g)
{
    return g->bw;
}

nsbeos_scaffolding *nsbeos_get_scaffold(struct gui_window *g)
{
    return g->scaffold;
}

struct browser_window *nsbeos_get_browser_for_gui(struct gui_window *g)
{
    return g->bw;
}

static struct gui_window *
gui_window_create(struct browser_window *bw, struct gui_window *existing, gui_window_create_flags flags)
{
    struct gui_window *g;

    g = (struct gui_window *)malloc(sizeof(*g));
    if (!g) {
        beos_warn_user("NoMemory", 0);
        return 0;
    }

    NSLOG(wisp, INFO, "Creating gui window %p for browser window %p", g, bw);

    g->bw = bw;
    g->mouse.state = 0;
    g->current_pointer = GUI_POINTER_DEFAULT;

    g->careth = 0;
    g->pending_resizes = 0;

    if (window_list)
        window_list->prev = g;
    g->next = window_list;
    g->prev = NULL;
    window_list = g;

    if (flags & GW_CREATE_TAB && existing) {
        g->scaffold = existing->scaffold;
        g->toplevel = false;
    } else {
        g->scaffold = nsbeos_new_scaffolding(g);
        g->toplevel = true;
    }

    if (!g->scaffold)
        return NULL;

    BRect frame(0, 0, -1, -1);
    g->view = new NSBrowserFrameView(frame, g);
    g->view->SetViewColor(B_TRANSPARENT_COLOR);
    g->view->SetLowColor(kWhiteColor);

    if (g->toplevel) {
        nsbeos_attach_toplevel_view(g->scaffold, g->view);
    }

    return g;
}

void nsbeos_dispatch_event(BMessage *message)
{
    struct gui_window *gui = NULL;
    NSBrowserFrameView *view = NULL;
    struct beos_scaffolding *scaffold = NULL;
    NSBrowserWindow *window = NULL;

    if (message->FindPointer("View", (void **)&view) < B_OK)
        view = NULL;
    if (message->FindPointer("gui_window", (void **)&gui) < B_OK)
        gui = NULL;
    if (message->FindPointer("Window", (void **)&window) < B_OK)
        window = NULL;
    if (message->FindPointer("scaffolding", (void **)&scaffold) < B_OK)
        scaffold = NULL;

    struct gui_window *z;
    for (z = window_list; z && gui && z != gui; z = z->next)
        continue;

    struct gui_window *y;
    for (y = window_list; y && scaffold && y->scaffold != scaffold; y = y->next)
        continue;

    if (gui && gui != z) {
        NSLOG(wisp, INFO, "discarding event for destroyed gui_window");
        delete message;
        return;
    }
    if (scaffold && (!y || scaffold != y->scaffold)) {
        NSLOG(wisp, INFO, "discarding event for destroyed scaffolding");
        delete message;
        return;
    }

    if (scaffold) {
        nsbeos_scaffolding_dispatch_event(scaffold, message);
        delete message;
        return;
    }

    switch (message->what) {
    case B_QUIT_REQUESTED:
        nsbeos_done = true;
        break;
    case NO_ACTION:
        delete message;
        return;
    case B_ABOUT_REQUESTED: {
        if (gui == NULL)
            gui = window_list;
        nsbeos_about(gui);
        break;
    }
    case _UPDATE_:
        if (gui && view)
            nsbeos_window_expose_event(view, gui, message);
        break;
    case B_MOUSE_MOVED: {
        if (gui == NULL || gui->bw == NULL)
            break;

        BPoint where;
        int32 mods;
        if (message->FindPoint("be:view_where", &where) < B_OK) {
            if (message->FindPoint("where", &where) < B_OK)
                break;
        }
        if (message->FindInt32("modifiers", &mods) < B_OK)
            mods = 0;


        if (gui->mouse.state & BROWSER_MOUSE_PRESS_1) {
            browser_window_mouse_click(gui->bw, BROWSER_MOUSE_DRAG_1, gui->mouse.pressed_x, gui->mouse.pressed_y);
            gui->mouse.state ^= (BROWSER_MOUSE_PRESS_1 | BROWSER_MOUSE_HOLDING_1);
            gui->mouse.state |= BROWSER_MOUSE_DRAG_ON;
        } else if (gui->mouse.state & BROWSER_MOUSE_PRESS_2) {
            browser_window_mouse_click(gui->bw, BROWSER_MOUSE_DRAG_2, gui->mouse.pressed_x, gui->mouse.pressed_y);
            gui->mouse.state ^= (BROWSER_MOUSE_PRESS_2 | BROWSER_MOUSE_HOLDING_2);
            gui->mouse.state |= BROWSER_MOUSE_DRAG_ON;
        }

        bool shift = mods & B_SHIFT_KEY;
        bool ctrl = mods & B_CONTROL_KEY;

        if (gui->mouse.state & BROWSER_MOUSE_MOD_1 && !shift)
            gui->mouse.state ^= BROWSER_MOUSE_MOD_1;
        if (gui->mouse.state & BROWSER_MOUSE_MOD_2 && !ctrl)
            gui->mouse.state ^= BROWSER_MOUSE_MOD_2;

        browser_window_mouse_track(gui->bw, (browser_mouse_state)gui->mouse.state, (int)(where.x), (int)(where.y));

        gui->last_x = (int)where.x;
        gui->last_y = (int)where.y;
        break;
    }
    case B_MOUSE_DOWN: {
        if (gui == NULL || gui->bw == NULL)
            break;

        BPoint where;
        int32 buttons;
        int32 mods;
        BPoint screenWhere;
        if (message->FindPoint("be:view_where", &where) < B_OK) {
            if (message->FindPoint("where", &where) < B_OK)
                break;
        }
        if (message->FindInt32("buttons", &buttons) < B_OK)
            break;
        if (message->FindPoint("screen_where", &screenWhere) < B_OK)
            break;
        if (message->FindInt32("modifiers", &mods) < B_OK)
            mods = 0;

        if (buttons & B_SECONDARY_MOUSE_BUTTON) {
            nsbeos_scaffolding_popup_menu(gui->scaffold, gui->bw, where, screenWhere);
            break;
        }

        gui->mouse.state = BROWSER_MOUSE_PRESS_1;

        if (buttons & B_TERTIARY_MOUSE_BUTTON)
            gui->mouse.state = BROWSER_MOUSE_PRESS_2;

        if (mods & B_SHIFT_KEY)
            gui->mouse.state |= BROWSER_MOUSE_MOD_1;
        if (mods & B_CONTROL_KEY)
            gui->mouse.state |= BROWSER_MOUSE_MOD_2;

        gui->mouse.pressed_x = where.x;
        gui->mouse.pressed_y = where.y;

        if (view && view->LockLooper()) {
            if (!view->IsFocus())
                view->MakeFocus();
            view->UnlockLooper();
        }

        browser_window_mouse_click(
            gui->bw, (browser_mouse_state)gui->mouse.state, gui->mouse.pressed_x, gui->mouse.pressed_y);

        break;
    }
    case B_MOUSE_UP: {
        if (gui == NULL || gui->bw == NULL)
            break;

        BPoint where;
        int32 buttons;
        int32 mods;
        BPoint screenWhere;
        if (message->FindPoint("be:view_where", &where) < B_OK) {
            if (message->FindPoint("where", &where) < B_OK)
                break;
        }
        if (message->FindInt32("buttons", &buttons) < B_OK)
            break;
        if (message->FindPoint("screen_where", &screenWhere) < B_OK)
            break;
        if (message->FindInt32("modifiers", &mods) < B_OK)
            mods = 0;

        if (gui->mouse.state & BROWSER_MOUSE_PRESS_1)
            gui->mouse.state ^= (BROWSER_MOUSE_PRESS_1 | BROWSER_MOUSE_CLICK_1);
        else if (gui->mouse.state & BROWSER_MOUSE_PRESS_2)
            gui->mouse.state ^= (BROWSER_MOUSE_PRESS_2 | BROWSER_MOUSE_CLICK_2);

        bool shift = mods & B_SHIFT_KEY;
        bool ctrl = mods & B_CONTROL_KEY;

        if (gui->mouse.state & BROWSER_MOUSE_MOD_1 && !shift)
            gui->mouse.state ^= BROWSER_MOUSE_MOD_1;
        if (gui->mouse.state & BROWSER_MOUSE_MOD_2 && !ctrl)
            gui->mouse.state ^= BROWSER_MOUSE_MOD_2;

        if (gui->mouse.state & (BROWSER_MOUSE_CLICK_1 | BROWSER_MOUSE_CLICK_2))
            browser_window_mouse_click(gui->bw, (browser_mouse_state)gui->mouse.state, where.x, where.y);
        else
            browser_window_mouse_track(gui->bw, (browser_mouse_state)0, where.x, where.y);

        gui->mouse.state = 0;

        break;
    }
    case B_KEY_DOWN:
        if (gui && view)
            nsbeos_window_keypress_event(view, gui, message);
        break;
    case B_VIEW_RESIZED:
        if (gui && view)
            nsbeos_window_resize_event(view, gui, message);
        break;
    case B_VIEW_MOVED:
        if (gui && view)
            nsbeos_window_moved_event(view, gui, message);
        break;
    case B_UI_SETTINGS_CHANGED:
        nsbeos_update_system_ui_colors();
        break;
    case 'nsLO':
    {
        nsurl *url;
        BString realm;
        BString username;
        BString password;
        void *cbpw;
        nserror (*cb)(const char *username, const char *password, void *pw);

        if (message->FindPointer("URL", (void **)&url) < B_OK)
            break;
        if (message->FindString("Realm", &realm) < B_OK)
            break;
        if (message->FindString("User", &username) < B_OK)
            break;
        if (message->FindString("Pass", &password) < B_OK)
            break;
        if (message->FindPointer("callback", (void **)&cb) < B_OK)
            break;
        if (message->FindPointer("callback_pw", (void **)&cbpw) < B_OK)
            break;
        cb(username.String(), password.String(), cbpw);
        break;
    }
    default:
        break;
    }
    delete message;
}

void nsbeos_window_expose_event(BView *view, gui_window *g, BMessage *message)
{
    BRect updateRect;
    struct redraw_context ctx = {true, true, &nsbeos_plotters, NULL};

    assert(g);
    assert(g->bw);

    if (g->pending_resizes > 1)
        return;

    if (message->FindRect("rect", &updateRect) < B_OK)
        return;

    if (browser_window_has_content(g->bw) == false)
        return;

    if (!view->LockLooper())
        return;
    nsbeos_current_gc_set(view);

    if (view->Window())
        view->Window()->BeginViewTransaction();

    /* Fixed-Tile Redraw Implementation */
    int tile_size = browser_get_tile_size();
    int rect_left = (int)updateRect.left;
    int rect_top = (int)updateRect.top;
    int rect_right = (int)updateRect.right + 1;
    int rect_bottom = (int)updateRect.bottom + 1;

    int x_start = rect_left - (rect_left % tile_size);
    int y_start = rect_top - (rect_top % tile_size);

    for (int ty = y_start; ty < rect_bottom; ty += tile_size) {
        int t_y0 = MAX(ty, rect_top);
        int t_y1 = MIN(ty + tile_size, rect_bottom);

        for (int tx = x_start; tx < rect_right; tx += tile_size) {
            struct rect tile_clip;
            tile_clip.x0 = MAX(tx, rect_left);
            tile_clip.y0 = t_y0;
            tile_clip.x1 = MIN(tx + tile_size, rect_right);
            tile_clip.y1 = t_y1;

            if (tile_clip.x0 >= tile_clip.x1 || tile_clip.y0 >= tile_clip.y1)
                continue;

            /* Push tile clip and redraw */
            BRegion region;
            region.Set(BRect(tile_clip.x0, tile_clip.y0, tile_clip.x1 - 1, tile_clip.y1 - 1));
            view->PushState();
            view->ConstrainClippingRegion(&region);

            browser_window_redraw(g->bw, 0, 0, &tile_clip, &ctx);

            view->PopState();
        }
    }

    if (g->careth != 0)
        nsbeos_plot_caret(g->caretx, g->carety, g->careth);

    if (view->Window())
        view->Window()->EndViewTransaction();

    nsbeos_current_gc_set(NULL);
    view->UnlockLooper();
}

void nsbeos_window_keypress_event(BView *view, gui_window *g, BMessage *event)
{
    const char *bytes;
    char buff[6];
    int numbytes = 0;
    uint32 mods;
    uint32 key;
    uint32 raw_char;
    uint32_t nskey;
    int i;

    if (event->FindInt32("modifiers", (int32 *)&mods) < B_OK)
        mods = modifiers();
    if (event->FindInt32("key", (int32 *)&key) < B_OK)
        key = 0;
    if (event->FindInt32("raw_char", (int32 *)&raw_char) < B_OK)
        raw_char = 0;
    for (i = 0; i < 5; i++) {
        buff[i] = '\0';
        if (event->FindInt8("byte", i, (int8 *)&buff[i]) < B_OK)
            break;
    }

    if (i) {
        bytes = buff;
        numbytes = i;
    } else if (event->FindString("bytes", &bytes) < B_OK)
        bytes = "";

    if (!numbytes)
        numbytes = strlen(bytes);

    NSLOG(wisp, INFO, "mods 0x%08" PRIx32 " key %" PRIu32 " raw %" PRIu32 " byte[0] %d", mods, key, raw_char, buff[0]);

    char byte;
    if (numbytes == 1) {
        byte = bytes[0];
        if (mods & B_CONTROL_KEY)
            byte = (char)raw_char;
        if (byte >= '!' && byte <= '~')
            nskey = (uint32_t)byte;
        else {
            switch (byte) {
            case B_BACKSPACE:
                nskey = NS_KEY_DELETE_LEFT;
                break;
            case B_TAB:
                nskey = NS_KEY_TAB;
                break;
            case B_ENTER:
                nskey = (uint32_t)10;
                break;
            case B_ESCAPE:
                nskey = (uint32_t)'\033';
                break;
            case B_SPACE:
                nskey = (uint32_t)' ';
                break;
            case B_DELETE:
                nskey = NS_KEY_DELETE_RIGHT;
                break;
            case B_HOME:
                nskey = NS_KEY_LINE_START;
                break;
            case B_END:
                nskey = NS_KEY_LINE_END;
                break;
            case B_PAGE_UP:
                nskey = NS_KEY_PAGE_UP;
                break;
            case B_PAGE_DOWN:
                nskey = NS_KEY_PAGE_DOWN;
                break;
            case B_LEFT_ARROW:
                nskey = NS_KEY_LEFT;
                break;
            case B_RIGHT_ARROW:
                nskey = NS_KEY_RIGHT;
                break;
            case B_UP_ARROW:
                nskey = NS_KEY_UP;
                break;
            case B_DOWN_ARROW:
                nskey = NS_KEY_DOWN;
                break;
            case 0:
                nskey = (uint32_t)0;
                break;
            default:
                nskey = (uint32_t)raw_char;
                break;
            }
        }
    } else {
        nskey = utf8_to_ucs4(bytes, numbytes);
    }

    if (browser_window_key_press(g->bw, nskey))
        return;

    float hdelta = 0.0f, vdelta = 0.0f;
    if (!g->view->LockLooper())
        return;
    BRect size = g->view->Bounds();
    switch (byte) {
    case B_HOME:
        g->view->ScrollTo(0.0f, 0.0f);
        break;
    case B_PAGE_UP:
        vdelta = -size.Height();
        break;
    case B_PAGE_DOWN:
        vdelta = size.Height();
        break;
    case B_LEFT_ARROW:
        hdelta = -10;
        break;
    case B_RIGHT_ARROW:
        hdelta = 10;
        break;
    case B_UP_ARROW:
        vdelta = -10;
        break;
    case B_DOWN_ARROW:
        vdelta = 10;
        break;
    }

    g->view->ScrollBy(hdelta, vdelta);
    g->view->UnlockLooper();
}


void nsbeos_window_resize_event(BView *view, gui_window *g, BMessage *event)
{
    if (atomic_add(&g->pending_resizes, -1) > 1)
        return;

    browser_window_schedule_reformat(g->bw);
}


void nsbeos_window_moved_event(BView *view, gui_window *g, BMessage *event)
{
    if (!view || !view->LockLooper())
        return;
    view->UnlockLooper();
}


void nsbeos_reflow_all_windows(void)
{
    for (struct gui_window *g = window_list; g; g = g->next) {
        browser_window_schedule_reformat(g->bw);
    }
}


void nsbeos_window_destroy_browser(struct gui_window *g)
{
    browser_window_destroy(g->bw);
}

static void gui_window_destroy(struct gui_window *g)
{
    if (!g)
        return;

    if (g->prev)
        g->prev->next = g->next;
    else
        window_list = g->next;

    if (g->next)
        g->next->prev = g->prev;


    NSLOG(wisp, INFO, "Destroying gui_window %p", g);

    if (g->view == NULL)
        return;
    if (!g->view->LockLooper())
        return;

    BLooper *looper = g->view->Looper();
    if (g->toplevel) {
        g->view->RemoveSelf();
        delete g->view;
        nsbeos_scaffolding_destroy(g->scaffold);
    } else {
        g->view->RemoveSelf();
        delete g->view;
        looper->Unlock();
    }

    free(g);
}

void nsbeos_redraw_caret(struct gui_window *g)
{
    if (g->careth == 0)
        return;

    if (g->view == NULL)
        return;
    if (!g->view->LockLooper())
        return;

    nsbeos_current_gc_set(g->view);
    g->view->Invalidate(BRect(g->caretx, g->carety, g->caretx, g->carety + g->careth));
    nsbeos_current_gc_set(NULL);
    g->view->UnlockLooper();
}

static nserror beos_window_invalidate_area(struct gui_window *g, const struct rect *rect)
{
    if (browser_window_has_content(g->bw) == false) {
        return NSERROR_OK;
    }

    if (g->view == NULL) {
        return NSERROR_OK;
    }

    if (!g->view->LockLooper()) {
        return NSERROR_OK;
    }

    if (rect != NULL) {
        g->view->Invalidate(BRect(rect->x0, rect->y0, rect->x1 - 1, rect->y1 - 1));
    } else {
        g->view->Invalidate();
    }

    g->view->UnlockLooper();

    return NSERROR_OK;
}

static bool gui_window_get_scroll(struct gui_window *g, int *sx, int *sy)
{
    if (g->view == NULL)
        return false;
    if (!g->view->LockLooper())
        return false;

    if (g->view->ScrollBar(B_HORIZONTAL))
        *sx = (int)g->view->ScrollBar(B_HORIZONTAL)->Value();
    if (g->view->ScrollBar(B_VERTICAL))
        *sy = (int)g->view->ScrollBar(B_VERTICAL)->Value();

    g->view->UnlockLooper();
    return true;
}

static nserror gui_window_set_scroll(struct gui_window *g, const struct rect *rect)
{
    if (g->view == NULL) {
        return NSERROR_BAD_PARAMETER;
    }
    if (!g->view->LockLooper()) {
        return NSERROR_BAD_PARAMETER;
    }

    if (g->view->ScrollBar(B_HORIZONTAL)) {
        g->view->ScrollBar(B_HORIZONTAL)->SetValue(rect->x0);
    }
    if (g->view->ScrollBar(B_VERTICAL)) {
        g->view->ScrollBar(B_VERTICAL)->SetValue(rect->y0);
    }

    g->view->UnlockLooper();

    return NSERROR_OK;
}


static void gui_window_update_extent(struct gui_window *g)
{
    nserror err;
    if (browser_window_has_content(g->bw) == false)
        return;

    if (g->view == NULL)
        return;
    if (!g->view->LockLooper())
        return;

    int x_max, y_max;

    err = browser_window_get_extents(g->bw, true, &x_max, &y_max);
    if (err != NSERROR_OK) {
        g->view->UnlockLooper();
        return;
    }

    float x_prop = g->view->Bounds().Width() / x_max;
    float y_prop = g->view->Bounds().Height() / y_max;
    x_max -= (int)g->view->Bounds().Width() + 1;
    y_max -= (int)g->view->Bounds().Height() + 1;

    if (g->view->ScrollBar(B_HORIZONTAL)) {
        g->view->ScrollBar(B_HORIZONTAL)->SetRange(0, x_max);
        g->view->ScrollBar(B_HORIZONTAL)->SetProportion(x_prop);
        g->view->ScrollBar(B_HORIZONTAL)->SetSteps(10, 50);
    }
    if (g->view->ScrollBar(B_VERTICAL)) {
        g->view->ScrollBar(B_VERTICAL)->SetRange(0, y_max);
        g->view->ScrollBar(B_VERTICAL)->SetProportion(y_prop);
        g->view->ScrollBar(B_VERTICAL)->SetSteps(10, 50);
    }

    g->view->UnlockLooper();
}

static BCursorID gui_haiku_pointer(gui_pointer_shape shape)
{
    switch (shape) {
    case GUI_POINTER_POINT:
        return B_CURSOR_ID_FOLLOW_LINK;
    case GUI_POINTER_CARET:
        return B_CURSOR_ID_I_BEAM;
    case GUI_POINTER_MENU:
        return B_CURSOR_ID_CONTEXT_MENU;
    case GUI_POINTER_UP:
        return B_CURSOR_ID_RESIZE_NORTH;
    case GUI_POINTER_DOWN:
        return B_CURSOR_ID_RESIZE_SOUTH;
    case GUI_POINTER_LEFT:
        return B_CURSOR_ID_RESIZE_WEST;
    case GUI_POINTER_RIGHT:
        return B_CURSOR_ID_RESIZE_EAST;
    case GUI_POINTER_RU:
        return B_CURSOR_ID_RESIZE_NORTH_EAST;
    case GUI_POINTER_LD:
        return B_CURSOR_ID_RESIZE_SOUTH_WEST;
    case GUI_POINTER_LU:
        return B_CURSOR_ID_RESIZE_NORTH_WEST;
    case GUI_POINTER_RD:
        return B_CURSOR_ID_RESIZE_SOUTH_EAST;
    case GUI_POINTER_CROSS:
        return B_CURSOR_ID_CROSS_HAIR;
    case GUI_POINTER_MOVE:
        return B_CURSOR_ID_MOVE;
    case GUI_POINTER_WAIT:
    case GUI_POINTER_PROGRESS:
        return B_CURSOR_ID_PROGRESS;
    case GUI_POINTER_NO_DROP:
    case GUI_POINTER_NOT_ALLOWED:
        return B_CURSOR_ID_NOT_ALLOWED;
    case GUI_POINTER_HELP:
        return B_CURSOR_ID_HELP;
    case GUI_POINTER_DEFAULT:
    default:
        break;
    }
    return B_CURSOR_ID_SYSTEM_DEFAULT;
}

static void gui_window_set_pointer(struct gui_window *g, gui_pointer_shape shape)
{
    if (g->current_pointer == shape)
        return;

    g->current_pointer = shape;

    BCursor cursor(gui_haiku_pointer(shape));

    if (g->view && g->view->LockLooper()) {
        g->view->SetViewCursor(&cursor);
        g->view->UnlockLooper();
    }
}

static void gui_window_place_caret(struct gui_window *g, int x, int y, int height, const struct rect *clip)
{
    if (g->view == NULL)
        return;
    if (!g->view->LockLooper())
        return;

    nsbeos_redraw_caret(g);

    g->caretx = x;
    g->carety = y + 1;
    g->careth = height - 2;

    nsbeos_redraw_caret(g);
    g->view->MakeFocus();

    g->view->UnlockLooper();
}

static void gui_window_remove_caret(struct gui_window *g)
{
    int oh = g->careth;

    if (oh == 0)
        return;

    g->careth = 0;

    if (g->view == NULL)
        return;
    if (!g->view->LockLooper())
        return;

    nsbeos_current_gc_set(g->view);
    g->view->Invalidate(BRect(g->caretx, g->carety, g->caretx, g->carety + oh));
    nsbeos_current_gc_set(NULL);
    g->view->UnlockLooper();
}

static void gui_window_new_content(struct gui_window *g)
{
    if (!g->toplevel)
        return;

    if (g->view == NULL)
        return;
    if (!g->view->LockLooper())
        return;

    g->view->ScrollTo(0, 0);
    g->view->UnlockLooper();
}

static void gui_start_selection(struct gui_window *g)
{
    if (!g->view->LockLooper())
        return;

    g->view->MakeFocus();
    g->view->UnlockLooper();
}

static void gui_get_clipboard(char **buffer, size_t *length)
{
    BMessage *clip;
    *length = 0;
    *buffer = NULL;

    if (be_clipboard->Lock()) {
        clip = be_clipboard->Data();
        if (clip) {
            const char *text;
            ssize_t textlen;
            if (clip->FindData("text/plain", B_MIME_TYPE, (const void **)&text, &textlen) >= B_OK) {
                *buffer = (char *)malloc(textlen);
                *length = textlen;
                memcpy(*buffer, text, textlen);
            }
        }
        be_clipboard->Unlock();
    }
}

static void gui_set_clipboard(const char *buffer, size_t length, nsclipboard_styles styles[], int n_styles)
{
    BMessage *clip;

    if (be_clipboard->Lock()) {
        be_clipboard->Clear();
        clip = be_clipboard->Data();
        if (clip) {
            clip->AddData("text/plain", B_MIME_TYPE, buffer, length);

            int arraySize = sizeof(text_run_array) + n_styles * sizeof(text_run);
            text_run_array *array = (text_run_array *)malloc(arraySize);
            array->count = n_styles;
            for (int i = 0; i < n_styles; i++) {
                BFont font;
                nsbeos_style_to_font(font, &styles[i].style);
                array->runs[i].offset = styles[i].start;
                array->runs[i].font = font;
                array->runs[i].color = nsbeos_rgb_colour(styles[i].style.foreground);
            }
            clip->AddData("application/x-vnd.Be-text_run_array", B_MIME_TYPE, array, arraySize);
            free(array);
            be_clipboard->Commit();
        }
        be_clipboard->Unlock();
    }
}

static struct gui_clipboard_table clipboard_table = {
    gui_get_clipboard,
    gui_set_clipboard,
};

struct gui_clipboard_table *beos_clipboard_table = &clipboard_table;

static nserror gui_window_get_dimensions(struct gui_window *g, int *width, int *height)
{
    if (g->view && g->view->LockLooper()) {
        *width = (int)g->view->Bounds().Width() + 1;
        *height = (int)g->view->Bounds().Height() + 1;
        g->view->UnlockLooper();
    }
    return NSERROR_OK;
}

static nserror gui_window_get_scrollbar_width(struct gui_window *g, int *width)
{
    *width = (int)B_V_SCROLL_BAR_WIDTH;
    return NSERROR_OK;
}

static nserror gui_window_event(struct gui_window *gw, enum gui_window_event event)
{
    switch (event) {
    case GW_EVENT_UPDATE_EXTENT:
        gui_window_update_extent(gw);
        break;
    case GW_EVENT_REMOVE_CARET:
        gui_window_remove_caret(gw);
        break;
    case GW_EVENT_NEW_CONTENT:
        gui_window_new_content(gw);
        break;
    case GW_EVENT_START_SELECTION:
        gui_start_selection(gw);
        break;
    case GW_EVENT_START_THROBBER:
        gui_window_start_throbber(gw);
        break;
    case GW_EVENT_STOP_THROBBER:
        gui_window_stop_throbber(gw);
        break;
    default:
        break;
    }
    return NSERROR_OK;
}


static struct gui_window_table window_table = {
    .create = gui_window_create,
    .destroy = gui_window_destroy,
    .invalidate = beos_window_invalidate_area,
    .get_scroll = gui_window_get_scroll,
    .set_scroll = gui_window_set_scroll,
    .get_dimensions = gui_window_get_dimensions,
    .get_scrollbar_width = gui_window_get_scrollbar_width,
    .event = gui_window_event,

    .set_title = gui_window_set_title,
    .set_url = gui_window_set_url,
    .set_icon = gui_window_set_icon,
    .set_status = gui_window_set_status,
    .set_pointer = gui_window_set_pointer,
    .place_caret = gui_window_place_caret,
    .drag_start = NULL,
    .save_link = NULL,
    .create_form_select_menu = NULL,
    .file_gadget_open = NULL,
    .drag_save_object = NULL,
    .drag_save_selection = NULL,
    .console_log = NULL
};

struct gui_window_table *beos_window_table = &window_table;
