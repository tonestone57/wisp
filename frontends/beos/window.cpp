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
#include <Button.h>
#include <CheckBox.h>
#include <Clipboard.h>
#include <Cursor.h>
#include <DirectWindow.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <FilePanel.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <RadioButton.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <String.h>
#include <TextControl.h>
#include <TextView.h>
#include <View.h>
#include <Window.h>
#include <assert.h>
#include <stdlib.h>

#include <map>
#include <new>

extern "C" {
#include <dom/html/html_form_element.h>
#include <dom/html/html_input_element.h>
#include "utils/log.h"
#include "utils/nsoption.h"
#include "utils/nsurl.h"
#include "utils/utf8.h"
#include "utils/utils.h"
#include "wisp/browser.h"
#include "wisp/browser_window.h"
#include "wisp/clipboard.h"
#include "wisp/content_type.h"
#include "wisp/form.h"
#include "wisp/ns_inttypes.h"
#include "wisp/keypress.h"
#include "wisp/mouse.h"
#include "wisp/plotters.h"
#include "wisp/url_db.h"
#include "wisp/window.h"
#include "wisp/content/hlcache.h"
#include "wisp/content/handlers/html/form_internal.h"
#include "wisp/utils/task_queue.h"
#include "wisp/desktop/plot_blend2d.h"
}

#include "beos/about.h"
#include "beos/font.h"
#include "beos/gui.h"
#include "beos/plotters.h"
#include "beos/scaffolding.h"
#include "beos/window.h"
#include "desktop/tile_pool.h"
#include "content/handlers/javascript/quickjs/wisp_subsystem.h"
#include "wisp/content.h"
#ifdef WITH_BLEND2D
#include <blend2d/blend2d.h>
#endif

class NSBrowserFrameView;
class BBitmap;

static bool nsbeos_gui_window_exists(struct gui_window *g);
static BBitmap *nsbeos_blit_bitmap;

class NSTextView : public BTextView {
public:
    NSTextView(BRect frame, const char *name, BRect textRect, uint32 resizeMask, uint32 flags, struct form_control *control, struct gui_window *g, BHandler *target)
        : BTextView(frame, name, textRect, resizeMask, flags), fControl(control), fGui(g), fTarget(target) {}

    virtual void InsertText(const char *text, int32 length, int32 offset, const text_run_array *runs) {
        BTextView::InsertText(text, length, offset, runs);
        Notify();
    }

    virtual void DeleteText(int32 fromOffset, int32 toOffset) {
        BTextView::DeleteText(fromOffset, toOffset);
        Notify();
    }

private:
    void Notify() {
        BMessage msg('gmod');
        msg.AddPointer("control", fControl);
        msg.AddPointer("gui_window", fGui);
        if (Window() && fTarget) {
            Window()->PostMessage(&msg, fTarget);
        }
    }
    struct form_control *fControl;
    struct gui_window *fGui;
    BHandler *fTarget;
};

class NSFileWidget : public BView {
public:
    NSFileWidget(BRect frame, struct form_control *control, struct gui_window *g)
        : BView(frame, "NSFileWidget", B_FOLLOW_NONE, B_WILL_DRAW), fControl(control), fGui(g) {
        SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

        BRect r = Bounds();
        float btnWidth = StringWidth("Browse...") + 20;
        BRect btnRect = r;
        btnRect.left = btnRect.right - btnWidth;

        BRect textRect = r;
        textRect.right = btnRect.left - 5;

        fText = new BTextControl(textRect, "file_path", "", "", NULL, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_V_CENTER);
        fText->SetEnabled(false);
        AddChild(fText);

        BMessage *msg = new BMessage('fbrw');
        msg->AddPointer("control", fControl);
        msg->AddPointer("gui_window", fGui);
        fBrowse = new BButton(btnRect, "browse", "Browse...", msg, B_FOLLOW_RIGHT | B_FOLLOW_V_CENTER);
        AddChild(fBrowse);
    }

    void SetTarget(BHandler *handler) {
        fBrowse->SetTarget(handler);
    }

    void SetText(const char *text) {
        fText->SetText(text);
    }

private:
    BTextControl *fText;
    BButton *fBrowse;
    struct form_control *fControl;
    struct gui_window *fGui;
};

#ifdef WITH_BLEND2D
struct beos_tile_task_t {
    struct gui_window *g;
    struct hlcache_handle *h;
    struct rect tile_clip;
    NSBrowserFrameView *view;
    void *buffer;
    int tile_size;
    int scrollx, scrolly;
    float priority;
};

static void nsbeos_tile_raster_complete(void *arg);

static bool nsbeos_tile_direct_blit(NSBrowserWindow *window, BView *view, void *buffer, int tile_size, const struct rect *tile_clip)
{
    if (!window->fDirectActive || !window->fDirectInfo)
        return false;

    bool success = false;
    if (window->_LockDirect()) {
        direct_buffer_info *info = window->fDirectInfo;
        if ((info->buffer_state & B_DIRECT_MODE_MASK) == B_DIRECT_STOP) {
            window->_UnlockDirect();
            return false;
        }

        /* Calculate view offset relative to screen */
        BPoint view_origin = view->ConvertToScreen(BPoint(0, 0));
        int vx = (int)view_origin.x;
        int vy = (int)view_origin.y;

        /* Source tile coordinate in the raster buffer (relative to tile origin) */
        int tx = tile_clip->x0 - (tile_clip->x0 % tile_size);
        int ty = tile_clip->y0 - (tile_clip->y0 % tile_size);

        /* Absolute destination coordinates on screen */
        int dx0 = vx + tile_clip->x0;
        int dy0 = vy + tile_clip->y0;
        int dx1 = vx + tile_clip->x1;
        int dy1 = vy + tile_clip->y1;

        uint8 *bits = (uint8 *)info->bits;
        int32 bpr = info->bytes_per_row;
        int32 bpp = info->bits_per_pixel / 8;

        /* Skip if bpp is unexpected (Blend2D uses 4bpp/PRGB32) */
        if (bpp != 4) {
            window->_UnlockDirect();
            return false;
        }

        /* Iterate through clipping rects provided by app_server */
        for (uint32 i = 0; i < info->clip_list_count; i++) {
            clipping_rect r = info->clip_list[i];

            /* Intersect tile destination with clipping rect */
            int ix0 = (dx0 > r.left) ? dx0 : r.left;
            int iy0 = (dy0 > r.top) ? dy0 : r.top;
            int ix1 = (dx1 < r.right + 1) ? dx1 : r.right + 1;
            int iy1 = (dy1 < r.bottom + 1) ? dy1 : r.bottom + 1;

            if (ix0 >= ix1 || iy0 >= iy1)
                continue;

            /* Blit line by line */
            for (int y = iy0; y < iy1; y++) {
                uint8 *dst = bits + (y * bpr) + (ix0 * bpp);
                /* src offset must be relative to tile buffer start:
                 * (y - vy - ty) is the vertical offset within the tile buffer
                 * (ix0 - vx - tx) is the horizontal pixel offset within the tile buffer
                 */
                uint8 *src = (uint8 *)buffer + ((y - vy - ty) * tile_size * bpp) + ((ix0 - vx - tx) * bpp);
                memcpy(dst, src, (ix1 - ix0) * bpp);
            }
        }

        success = true;
        window->_UnlockDirect();
    }
    return success;
}

extern "C" void beos_tile_redraw_worker(void *arg)
{
    struct beos_tile_task_t *task = (struct beos_tile_task_t *)arg;
    BLContextCore bl_ctx;
    BLImageCore img;

    /* Initialize Blend2D image from pooled buffer */
    bl_image_init_as_from_data(&img, task->tile_size, task->tile_size, BL_FORMAT_PRGB32, task->buffer, (intptr_t)task->tile_size * 4, BL_DATA_ACCESS_RW, NULL, NULL);
    bl_context_init_as(&bl_ctx, &img, NULL);

    /* Clear buffer to white initially */
    bl_context_set_fill_style_rgba32(&bl_ctx, 0xFFFFFFFF);
    bl_context_fill_all(&bl_ctx);

    struct blend2d_context b2d_ctx = {
        .bl_ctx = &bl_ctx,
        .native_ctx = NULL,
        .native_text_handler = NULL
    };

    struct redraw_context ctx = {true, true, &blend2d_plotters, &b2d_ctx};

    /* Adjust drawing coordinates so (0,0) is the top-left of the tile.
     * We calculate the tile's top-left origin by aligning with the tile grid. */
    BLMatrix2D m;
    bl_matrix2d_set_identity(&m);
    int tile_x0 = task->tile_clip.x0 - (task->tile_clip.x0 % task->tile_size);
    int tile_y0 = task->tile_clip.y0 - (task->tile_clip.y0 % task->tile_size);
    double tl_data[2] = {-(double)tile_x0, -(double)tile_y0};
    bl_matrix2d_apply_op(&m, BL_TRANSFORM_OP_TRANSLATE, tl_data);
    bl_context_apply_transform_op(&bl_ctx, BL_TRANSFORM_OP_POST_TRANSFORM, &m);

    /* Render content into tile buffer.
     * We pass -task->scrollx, -task->scrolly to ensure content is rendered
     * at the correct position within the view-space tile clip. */
    browser_window_redraw(task->g->bw, -task->scrollx, -task->scrolly, &task->tile_clip, &ctx);

    bl_context_end(&bl_ctx);
    bl_image_destroy(&img);

    /* Dispatch completion callback to main thread */
    if (!task_queue_post(nsbeos_tile_raster_complete, task)) {
        content_dec_bg_tasks(task->h);
        hlcache_handle_release(task->h);
        tile_pool_return(task->buffer);
        free(task);
    }
}

static void nsbeos_tile_raster_complete(void *arg)
{
    struct beos_tile_task_t *task = (struct beos_tile_task_t *)arg;
    struct gui_window *g = task->g;
    NSBrowserFrameView *view = task->view;
    bool blitted = false;

    /* Safety Check: Verify window still exists in the global list */
    if (nsbeos_gui_window_exists(g)) {
        NSBrowserWindow *window = dynamic_cast<NSBrowserWindow *>(view->Window());
        if (window) {
            blitted = nsbeos_tile_direct_blit(window, view, task->buffer, task->tile_size, &task->tile_clip);
        }
    }

    if (!blitted && nsbeos_gui_window_exists(g) && view->LockLooper()) {
        /* Atomic Blit: Reuse a global BBitmap to prevent area/heap fragmentation. */
        if (nsbeos_blit_bitmap != NULL && (nsbeos_blit_bitmap->Bounds().Width() + 1 != task->tile_size)) {
            delete nsbeos_blit_bitmap;
            nsbeos_blit_bitmap = NULL;
        }
        if (nsbeos_blit_bitmap == NULL) {
            BRect frame(0, 0, task->tile_size - 1, task->tile_size - 1);
            nsbeos_blit_bitmap = new BBitmap(frame, 0, B_RGBA32);
        }

        BBitmap *b = nsbeos_blit_bitmap;

        if (b && b->InitCheck() == B_OK) {
            view->Sync();
            if (b->ImportBits(task->buffer, task->tile_size * task->tile_size * 4, task->tile_size * 4, 0, B_RGBA32) == B_OK) {
                int tx = task->tile_clip.x0 - (task->tile_clip.x0 % task->tile_size);
                int ty = task->tile_clip.y0 - (task->tile_clip.y0 % task->tile_size);

                /* Only blit the part of the tile that is actually within the clip.
                 * This prevents overpainting adjacent tiles with the clear color. */
                BRect srcRect(task->tile_clip.x0 - tx, task->tile_clip.y0 - ty,
                              task->tile_clip.x1 - tx - 1, task->tile_clip.y1 - ty - 1);
                BRect dstRect(task->tile_clip.x0, task->tile_clip.y0,
                              task->tile_clip.x1 - 1, task->tile_clip.y1 - 1);
                view->DrawBitmap(b, srcRect, dstRect);

                /* Caret Persistence: Redraw caret if it was overpainted by this tile */
                if (g->careth != 0) {
                    BRect caretRect(g->caretx, g->carety, g->caretx, g->carety + g->careth);
                    if (caretRect.Intersects(dstRect)) {
                        nsbeos_current_gc_set(view);
                        nsbeos_plot_caret(g->caretx, g->carety, g->careth);
                        nsbeos_current_gc_set(NULL);
                    }
                }
            }
        }

        view->UnlockLooper();
    }

    /* Update active task count for content lifecycle management */
    content_dec_bg_tasks(task->h);
    hlcache_handle_release(task->h);

    /* Save rendered buffer to cache instead of immediately returning it to pool */
    int tx = task->tile_clip.x0 - (task->tile_clip.x0 % task->tile_size);
    int ty = task->tile_clip.y0 - (task->tile_clip.y0 % task->tile_size);
    tile_pool_put_cached(task->g, tx, ty, task->tile_size, task->buffer, task->priority);

    free(task);
}
#endif




static const rgb_color kWhiteColor = {255, 255, 255, 255};

static struct gui_window *window_list = 0;
static BBitmap *nsbeos_blit_bitmap = NULL;

static BString current_selection;
static BList current_selection_textruns;

static void nsbeos_window_expose_event(BView *view, gui_window *g, BMessage *message);
static void nsbeos_window_keypress_event(BView *view, gui_window *g, BMessage *event);
static void nsbeos_window_resize_event(BView *view, gui_window *g, BMessage *event);
static void nsbeos_window_moved_event(BView *view, gui_window *g, BMessage *event);
static void nsbeos_redraw_caret(struct gui_window *g);
static void gui_window_cleanup_widgets(struct gui_window *g);


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
    case 'slct':
    case 'fsel':
    case 'fbrw':
    case 'gdgt':
    case 'gmod':
        Window()->DetachCurrentMessage();
        nsbeos_pipe_message(message, this, fGuiWindow);
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

    g = new (std::nothrow) struct gui_window();
    if (!g) {
        beos_warn_user("NoMemory", 0);
        return 0;
    }

    NSLOG(wisp, INFO, "Creating gui window %p for browser window %p", g, bw);

    g->bw = bw;
    g->current_pointer = GUI_POINTER_DEFAULT;

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

    if (gui && !nsbeos_gui_window_exists(gui)) {
        NSLOG(wisp, INFO, "discarding event for destroyed gui_window");
        delete message;
        return;
    }

    struct gui_window *y;
    for (y = window_list; y && scaffold && y->scaffold != scaffold; y = y->next)
        continue;
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
    case 'slct': {
        struct form_control *control;
        int32 index;
        if (gui != NULL && message->FindPointer("control", (void **)&control) == B_OK &&
            message->FindInt32("index", &index) == B_OK) {
            form_select_process_selection(control, (int)index);
        }
        break;
    }
    case 'fbrw': {
        struct form_control *control;
        if (gui != NULL && message->FindPointer("control", (void **)&control) == B_OK) {
            gui_window_file_gadget_open(gui, NULL, control);
        }
        break;
    }
    case 'fsel': {
        entry_ref ref;
        struct form_control *gadget;
        if (gui != NULL && message->FindRef("refs", &ref) == B_OK &&
            message->FindPointer("gadget", (void **)&gadget) == B_OK) {
            BPath path(&ref);
            browser_window_set_gadget_filename(gui->bw, gadget, path.Path());
        }
        break;
    }
    case 'gmod':
    case 'gdgt': {
        struct form_control *control;
        if (message->FindPointer("control", (void **)&control) == B_OK) {
            if (gui == NULL) break;

            if (gui->view && gui->view->LockLooper()) {
                switch (control->type) {
                case GADGET_SUBMIT:
                    if (message->what == 'gdgt') {
                        form_submit(content_get_url(browser_window_get_content(gui->bw)), gui->bw, control->form, control);
                    }
                    break;
                case GADGET_RESET:
                    if (message->what == 'gdgt' && control->form && control->form->node) {
                        dom_html_form_element_reset((dom_html_form_element *)control->form->node);
                    }
                    break;
                case GADGET_BUTTON:
                    break;
                case GADGET_CHECKBOX: {
                    std::map<struct form_control *, BView *>::iterator it = gui->widgets.find(control);
                    BCheckBox *cb = (it != gui->widgets.end()) ? dynamic_cast<BCheckBox *>(it->second) : NULL;
                    if (cb) {
                        control->selected = (cb->Value() == B_CONTROL_ON);
                        dom_html_input_element_set_checked((dom_html_input_element *)control->node, control->selected);
                    }
                    break;
                }
                case GADGET_TEXTAREA: {
                    std::map<struct form_control *, BView *>::iterator it = gui->widgets.find(control);
                    BScrollView *sv = (it != gui->widgets.end()) ? dynamic_cast<BScrollView *>(it->second) : NULL;
                    NSTextView *tv = sv ? dynamic_cast<NSTextView *>(sv->Target()) : NULL;
                    if (tv) {
                        form_gadget_update_value(control, tv->Text());
                    }
                    break;
                }
                case GADGET_RADIO: {
                    std::map<struct form_control *, BView *>::iterator it = gui->widgets.find(control);
                    BRadioButton *rb = (it != gui->widgets.end()) ? dynamic_cast<BRadioButton *>(it->second) : NULL;
                    if (rb && rb->Value() == B_CONTROL_ON && control->selected == false) {
                        form_radio_set(control);
                    }
                    break;
                }
                case GADGET_TEXTBOX:
                case GADGET_PASSWORD: {
                    std::map<struct form_control *, BView *>::iterator it = gui->widgets.find(control);
                    BTextControl *tc = (it != gui->widgets.end()) ? dynamic_cast<BTextControl *>(it->second) : NULL;
                    if (tc) {
                        form_gadget_update_value(control, tc->Text());
                    }
                    break;
                }
                default:
                    break;
                }
                gui->view->UnlockLooper();
            }
        }
        break;
    }
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
    struct redraw_context ctx = {true, true, &nsbeos_plotters, g};

    assert(g);
    assert(g->bw);

    if (g->pending_resizes > 1)
        return;

    if (message->FindRect("rect", &updateRect) < B_OK)
        return;

    if (browser_window_has_content(g->bw) == false)
        return;

    int backend = nsoption_int(render_backend);
    bool use_blend2d = (backend == OPTION_RENDER_BACKEND_BLEND2D);
    /* For Haiku, OPTION_RENDER_BACKEND_AUTO remains Native */

    if (!use_blend2d) {
        if (view->LockLooper()) {
            struct rect clip = {(int)updateRect.left, (int)updateRect.top, (int)updateRect.right + 1, (int)updateRect.bottom + 1};
            nsbeos_current_gc_set(view);
            browser_window_redraw(g->bw, 0, 0, &clip, &ctx);
            nsbeos_current_gc_set(NULL);
            view->UnlockLooper();
        }
        return;
    }

#ifdef WITH_BLEND2D
    /* Fixed-Tile Redraw Implementation with Worker Offloading */
    int tile_size = browser_get_tile_size();

    /* Safety check: ensure tile size doesn't exceed pooled buffer capacity */
    if (tile_size > TILE_WIDTH) {
        NSLOG(wisp, WARNING, "Tile size %d exceeds pool capacity, clamping to %d", tile_size, TILE_WIDTH);
        tile_size = TILE_WIDTH;
    }
    int rect_left = (int)updateRect.left;
    int rect_top = (int)updateRect.top;
    int rect_right = (int)updateRect.right + 1;
    int rect_bottom = (int)updateRect.bottom + 1;

    int x_start = rect_left - (rect_left % tile_size);
    int y_start = rect_top - (rect_top % tile_size);

    BRect view_bounds = view->Bounds();
    int v_x = (int)view_bounds.left;
    int v_y = (int)view_bounds.top;
    int v_w = (int)view_bounds.Width() + 1;
    int v_h = (int)view_bounds.Height() + 1;

    /* Compress non-visible tiles or evict distant tiles before rendering */
    tile_pool_manage_cache(g, v_x, v_y, v_w, v_h);

    for (int ty = y_start; ty < rect_bottom; ty += tile_size) {
        int t_y0 = (ty > rect_top) ? ty : rect_top;
        int t_y1 = (ty + tile_size < rect_bottom) ? ty + tile_size : rect_bottom;

        for (int tx = x_start; tx < rect_right; tx += tile_size) {
            struct rect tile_clip;
            tile_clip.x0 = (tx > rect_left) ? tx : rect_left;
            tile_clip.y0 = t_y0;
            tile_clip.x1 = (tx + tile_size < rect_right) ? tx + tile_size : rect_right;
            tile_clip.y1 = t_y1;

            if (tile_clip.x0 >= tile_clip.x1 || tile_clip.y0 >= tile_clip.y1)
                continue;

            /* Calculate priority based on distance to viewport frustum */
            float priority = browser_calculate_tile_priority(tx, ty, v_x, v_y, v_w, v_h);

            /* Check if the tile is already in the cache (either raw or compressed) */
            bool from_cache = false;
            void *cached_buf = tile_pool_get_cached(g, tx, ty, tile_size, &from_cache);
            if (from_cache && cached_buf != NULL) {
                bool blitted = false;
                if (nsbeos_gui_window_exists(g)) {
                    NSBrowserWindow *window = dynamic_cast<NSBrowserWindow *>(view->Window());
                    if (window) {
                        blitted = nsbeos_tile_direct_blit(window, view, cached_buf, tile_size, &tile_clip);
                    }
                }

                if (!blitted && nsbeos_gui_window_exists(g) && view->LockLooper()) {
                    if (nsbeos_blit_bitmap != NULL && (nsbeos_blit_bitmap->Bounds().Width() + 1 != tile_size)) {
                        delete nsbeos_blit_bitmap;
                        nsbeos_blit_bitmap = NULL;
                    }
                    if (nsbeos_blit_bitmap == NULL) {
                        BRect frame(0, 0, tile_size - 1, tile_size - 1);
                        nsbeos_blit_bitmap = new BBitmap(frame, 0, B_RGBA32);
                    }

                    BBitmap *b = nsbeos_blit_bitmap;
                    if (b && b->InitCheck() == B_OK) {
                        view->Sync();
                        if (b->ImportBits(cached_buf, tile_size * tile_size * 4, tile_size * 4, 0, B_RGBA32) == B_OK) {
                            BRect srcRect(tile_clip.x0 - tx, tile_clip.y0 - ty,
                                          tile_clip.x1 - tx - 1, tile_clip.y1 - ty - 1);
                            BRect dstRect(tile_clip.x0, tile_clip.y0,
                                          tile_clip.x1 - 1, tile_clip.y1 - 1);
                            view->DrawBitmap(b, srcRect, dstRect);

                            if (g->careth != 0) {
                                BRect caretRect(g->caretx, g->carety, g->caretx, g->carety + g->careth);
                                if (caretRect.Intersects(dstRect)) {
                                    nsbeos_current_gc_set(view);
                                    nsbeos_plot_caret(g->caretx, g->carety, g->careth);
                                    nsbeos_current_gc_set(NULL);
                                }
                            }
                        }
                    }
                    view->UnlockLooper();
                }

                /* Keep it in cache and update priority */
                tile_pool_put_cached(g, tx, ty, tile_size, cached_buf, priority);
                continue;
            }

            /* Checkout buffer and dispatch raster task */
            void *buf = tile_pool_checkout();
            bool dispatched = false;
            if (buf != NULL) {
                struct hlcache_handle *h = browser_window_get_content(g->bw);
                if (h != NULL) {
                    struct beos_tile_task_t *task = (struct beos_tile_task_t *)malloc(sizeof(struct beos_tile_task_t));
                    if (task) {
                        int sx = 0, sy = 0;
                        if (view->ScrollBar(B_HORIZONTAL)) sx = (int)view->ScrollBar(B_HORIZONTAL)->Value();
                        if (view->ScrollBar(B_VERTICAL)) sy = (int)view->ScrollBar(B_VERTICAL)->Value();

                        task->g = g;
                        task->tile_clip = tile_clip;
                        task->view = (NSBrowserFrameView *)view;
                        task->buffer = buf;
                        task->tile_size = tile_size;
                        task->scrollx = sx;
                        task->scrolly = sy;
                        task->priority = priority;

                        /* Clone handle to ensure it remains valid during background task */
                        if (hlcache_handle_clone(h, &task->h) == NSERROR_OK) {
                            /* Increment active background tasks to prevent content destruction while rendering */
                            content_inc_bg_tasks(task->h);
                            dispatched = wisp_dispatch_raster(NULL, beos_tile_redraw_worker, task, priority);
                            if (!dispatched) {
                                content_dec_bg_tasks(task->h);
                                hlcache_handle_release(task->h);
                                free(task);
                            }
                        } else {
                            free(task);
                        }
                    }
                }
            }

            if (!dispatched) {
                if (buf) {
                    tile_pool_return(buf);
                }
                /* Synchronous fallback if pool/dispatch fails */
                struct redraw_context ctx = {true, true, &nsbeos_plotters, NULL};
                if (view->LockLooper()) {
                    nsbeos_current_gc_set(view);
                    browser_window_redraw(g->bw, 0, 0, &tile_clip, &ctx);
                    nsbeos_current_gc_set(NULL);
                    view->UnlockLooper();
                }
            }
        }
    }
#endif // WITH_BLEND2D

    if (g->careth != 0) {
        if (view->LockLooper()) {
            nsbeos_current_gc_set(view);
            nsbeos_plot_caret(g->caretx, g->carety, g->careth);
            nsbeos_current_gc_set(NULL);
            view->UnlockLooper();
        }
    }
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

void nsbeos_window_finalise(void)
{
    delete nsbeos_blit_bitmap;
    nsbeos_blit_bitmap = NULL;
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

    gui_window_cleanup_widgets(g);

    delete g->wndOpenFile;

    if (g->view != NULL && g->view->LockLooper()) {
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
    }

    delete g;
}

static void gui_window_cleanup_widgets(struct gui_window *g)
{
    if (g->view == NULL || !g->view->LockLooper())
        return;

    for (std::map<struct form_control *, BView *>::iterator it = g->widgets.begin(); it != g->widgets.end(); ++it) {
        it->second->RemoveSelf();
        delete it->second;
    }
    g->widgets.clear();

    g->view->UnlockLooper();
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

/**
 * Check if a gui_window exists in the global list.
 *
 * \param g  The window to check.
 * \return true if it exists, false otherwise.
 */
static bool nsbeos_gui_window_exists(struct gui_window *g)
{
    struct gui_window *z;
    for (z = window_list; z && z != g; z = z->next)
        continue;
    return (z != NULL);
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

static void gui_window_create_form_select_menu(struct gui_window *g, struct form_control *control)
{
    BPopUpMenu *menu = new BPopUpMenu("select_menu", false, false);
    struct form_option *option;
    int i = 0;

    while ((option = form_select_get_option(control, i)) != NULL) {
        BMessage *msg = new BMessage('slct');
        msg->AddInt32("index", i);
        msg->AddPointer("control", control);
        msg->AddPointer("gui_window", g);
        BMenuItem *item = new BMenuItem(option->text, msg);
        if (option->selected) {
            item->SetMarked(true);
        }
        menu->AddItem(item);
        i++;
    }

    if (!g->view->LockLooper()) {
        delete menu;
        return;
    }
    BPoint screen_pos = g->view->ConvertToScreen(BPoint(g->last_x, g->last_y));
    g->view->UnlockLooper();

    menu->SetTargetForItems(g->view);
    /* Go(..., false) makes the menu synchronous, allowing us to delete it immediately after.
     * The parameters are (where, deliversMessage, openAnyway, asynchronous). */
    menu->Go(screen_pos, true, false, false);
    delete menu;
}


static void gui_window_file_gadget_open(struct gui_window *g, struct hlcache_handle *hl, struct form_control *gadget)
{
    if (g->wndOpenFile == NULL) {
        g->wndOpenFile = new BFilePanel(B_OPEN_PANEL, NULL, NULL, B_FILE_NODE, false);
    }
    BMessage msg('fsel');
    msg.AddPointer("gui_window", g);
    msg.AddPointer("gadget", gadget);
    g->wndOpenFile->SetMessage(&msg);
    g->wndOpenFile->SetTarget(BMessenger(g->view));
    g->wndOpenFile->Show();
}


extern "C" nserror gui_window_draw_gadget(
    const struct redraw_context *ctx, int x, int y, int width, int height, struct form_control *control)
{
    struct gui_window *g = (struct gui_window *)ctx->priv;
    if (!g || !g->view)
        return NSERROR_NOT_IMPLEMENTED;

    BView *widget = NULL;

    if (g->view->LockLooper()) {
        std::map<struct form_control *, BView *>::iterator it = g->widgets.find(control);
        if (it != g->widgets.end()) {
            widget = it->second;
        }
        g->view->UnlockLooper();
    }

    if (widget == NULL) {
        BRect frame(x, y, x + width - 1, y + height - 1);
        BString label("");
        if (control->value && (control->type == GADGET_SUBMIT || control->type == GADGET_RESET ||
                                  control->type == GADGET_BUTTON || control->type == GADGET_CHECKBOX ||
                                  control->type == GADGET_RADIO)) {
            label = control->value;
        }

        BMessage *msg = new BMessage('gdgt');
        msg->AddPointer("control", control);
        msg->AddPointer("gui_window", g);

        switch (control->type) {
        case GADGET_SUBMIT:
        case GADGET_RESET:
        case GADGET_BUTTON:
            widget = new BButton(frame, "wisp_button", label.String(), msg);
            break;
        case GADGET_CHECKBOX:
            widget = new BCheckBox(frame, "wisp_checkbox", "", msg);
            ((BCheckBox *)widget)->SetValue(control->selected ? B_CONTROL_ON : B_CONTROL_OFF);
            break;
        case GADGET_RADIO:
            widget = new BRadioButton(frame, "wisp_radio", "", msg);
            ((BRadioButton *)widget)->SetValue(control->selected ? B_CONTROL_ON : B_CONTROL_OFF);
            break;
        case GADGET_TEXTBOX:
        case GADGET_PASSWORD:
            widget = new BTextControl(frame, "wisp_text", "", control->value ? control->value : "", msg);
            if (control->type == GADGET_PASSWORD) {
                ((BTextControl *)widget)->TextView()->HideTyping(true);
            }
            {
                BMessage *mod = new BMessage('gmod');
                mod->AddPointer("control", control);
                mod->AddPointer("gui_window", g);
                ((BTextControl *)widget)->SetModificationMessage(mod);
            }
            break;
        case GADGET_TEXTAREA: {
            /* Create scroller within the provided frame.
             * BScrollView will expand to include its scrollbars/borders.
             * We shrink the target view accordingly so the final scroller fits 'frame'. */
            BRect tvRect(0, 0, frame.Width() - B_V_SCROLL_BAR_WIDTH - 2, frame.Height() - B_H_SCROLL_BAR_HEIGHT - 2);

            NSTextView *tv = new NSTextView(tvRect, "wisp_textarea", tvRect.InsetByCopy(2, 2),
                                            B_FOLLOW_ALL, B_WILL_DRAW, control, g, g->view);
            tv->SetText(control->value ? control->value : "");
            widget = new BScrollView("wisp_textarea_scroller", tv, B_FOLLOW_NONE, 0, true, true);

            /* Position and size the BScrollView to exactly match the intended frame */
            widget->MoveTo(frame.left, frame.top);
            widget->ResizeTo(frame.Width(), frame.Height());
            delete msg;
            break;
        }
        case GADGET_SELECT: {
            BPopUpMenu *menu = new BPopUpMenu("wisp_select_menu");
            struct form_option *option;
            int i = 0;
            while ((option = form_select_get_option(control, i)) != NULL) {
                BMessage *m = new BMessage('slct');
                m->AddInt32("index", i);
                m->AddPointer("control", control);
                m->AddPointer("gui_window", g);
                BMenuItem *item = new BMenuItem(option->text, m);
                if (option->selected) item->SetMarked(true);
                menu->AddItem(item);
                i++;
            }
            widget = new BMenuField(frame, "wisp_select", NULL, menu);
            delete msg;
            break;
        }
        case GADGET_FILE: {
            widget = new NSFileWidget(frame, control, g);
            delete msg;
            break;
        }
        default:
            delete msg;
            return NSERROR_NOT_IMPLEMENTED;
        }

        if (widget) {
            if (g->view->LockLooper()) {
                g->view->AddChild(widget);
                BControl *c = dynamic_cast<BControl *>(widget);
                if (c) c->SetTarget(g->view);

                if (control->type == GADGET_SELECT) {
                    ((BMenuField *)widget)->Menu()->SetTargetForItems(g->view);
                } else if (control->type == GADGET_FILE) {
                    ((NSFileWidget *)widget)->SetTarget(g->view);
                }
                g->widgets[control] = widget;
                g->view->UnlockLooper();
            } else {
                delete widget;
                return NSERROR_NOMEM;
            }
        }
    }

    if (widget) {
        if (g->view->LockLooper()) {
            if (widget->Frame().left != x || widget->Frame().top != y) {
                widget->MoveTo(x, y);
            }
            if (widget->Bounds().Width() != (width - 1) || widget->Bounds().Height() != (height - 1)) {
                widget->ResizeTo(width - 1, height - 1);
            }

            /* Sync state from core to native widget */
            switch (control->type) {
            case GADGET_CHECKBOX: {
                BCheckBox *cb = dynamic_cast<BCheckBox *>(widget);
                int32 val = control->selected ? B_CONTROL_ON : B_CONTROL_OFF;
                if (cb && cb->Value() != val) {
                    cb->SetValue(val);
                }
                break;
            }
            case GADGET_RADIO: {
                BRadioButton *rb = dynamic_cast<BRadioButton *>(widget);
                int32 val = control->selected ? B_CONTROL_ON : B_CONTROL_OFF;
                if (rb && rb->Value() != val) {
                    rb->SetValue(val);
                }
                break;
            }
            case GADGET_TEXTBOX:
            case GADGET_PASSWORD: {
                BTextControl *tc = dynamic_cast<BTextControl *>(widget);
                if (tc) {
                    const char *core_val = control->value ? control->value : "";
                    if (strcmp(tc->Text(), core_val) != 0) {
                        tc->SetText(core_val);
                    }
                }
                break;
            }
            case GADGET_TEXTAREA: {
                BScrollView *sv = dynamic_cast<BScrollView *>(widget);
                NSTextView *tv = sv ? dynamic_cast<NSTextView *>(sv->Target()) : NULL;
                if (tv) {
                    const char *core_val = control->value ? control->value : "";
                    if (strcmp(tv->Text(), core_val) != 0) {
                        tv->SetText(core_val);
                    }
                }
                break;
            }
            case GADGET_SELECT: {
                BMenuField *mf = dynamic_cast<BMenuField *>(widget);
                if (mf && mf->Menu()) {
                    BMenu *menu = mf->Menu();
                    struct form_option *option;
                    int i = 0;
                    while ((option = form_select_get_option(control, i)) != NULL) {
                        BMenuItem *item = menu->ItemAt(i);
                        if (item && item->IsMarked() != option->selected) {
                            item->SetMarked(option->selected);
                        }
                        i++;
                    }
                }
                break;
            }
            case GADGET_FILE: {
                NSFileWidget *fw = dynamic_cast<NSFileWidget *>(widget);
                if (fw) {
                    fw->SetText(control->value ? control->value : "");
                }
                break;
            }
            default:
                break;
            }

            g->view->UnlockLooper();
        }
    }

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
        gui_window_cleanup_widgets(gw);
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
    .create_form_select_menu = gui_window_create_form_select_menu,
    .file_gadget_open = gui_window_file_gadget_open,
    .drag_save_object = NULL,
    .drag_save_selection = NULL,
    .console_log = NULL
};

struct gui_window_table *beos_window_table = &window_table;
