/*
 * Copyright 2015 Adrián Arroyo Calle <adrian.arroyocalle@gmail.com>
 * Copyright 2008 François Revol <mmu_man@users.sourceforge.net>
 * Copyright 2005 James Bursa <bursa@users.sourceforge.net>
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
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <Alert.h>
#include <Application.h>
#include <BeBuild.h>
#include <FindDirectory.h>
#include <Mime.h>
#include <Path.h>
#include <PathFinder.h>
#include <Resources.h>
#include <Roster.h>
#include <Screen.h>
#include <String.h>
#ifdef __HAIKU__
#include <LocaleRoster.h>
#endif

extern "C" {

#include "utils/corestrings.h"
#include "utils/filename.h"
#include "utils/log.h"
#include "utils/messages.h"
#include "utils/nsoption.h"
#include "utils/nsurl.h"
#include "utils/url.h"
#include "utils/utf8.h"
#include "utils/utils.h"
#include "utils/task_queue.h"
#include "wisp/browser_window.h"
#include "wisp/clipboard.h"
#include "wisp/content.h"
#include "wisp/cookie_db.h"
#include "wisp/fetch.h"
#include "wisp/misc.h"
#include "wisp/wisp.h"
#include "wisp/search.h"
#include "wisp/url_db.h"
#include "content/fetch.h"
}

#include "beos/gui.h"
#include "beos/gui_options.h"
#include "beos/bitmap.h"
#include "beos/download.h"
#include "beos/fetch_rsrc.h"
#include "beos/filetype.h"
#include "beos/font.h"
#include "beos/scaffolding.h"
#include "beos/schedule.h"
#include "beos/throbber.h"
#include "beos/window.h"

#define USE_RESOURCES 1

bool nsbeos_done = false;

bool replicated = false;

char *options_file_location;

struct gui_window *search_current_window = 0;

BWindow *wndAbout;
BWindow *wndWarning;
BWindow *wndTooltip;

static thread_id sBAppThreadID;

static BMessage *gFirstRefsReceived = NULL;

static int sEventPipe[2];


nserror beos_warn_user(const char *warning, const char *detail)
{
    NSLOG(wisp, INFO, "warn_user: %s (%s)", warning, detail);
    BAlert *alert;
    BString text(warning);
    if (detail)
        text << ":\n" << detail;

    alert = new BAlert("Wisp Warning", text.String(), "Debug", "Ok", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
    if (alert->Go() < 1) {
        debugger("warn_user");
    }

    return NSERROR_OK;
}

NSBrowserApplication::NSBrowserApplication() : BApplication("application/x-vnd.Wisp")
{
}


NSBrowserApplication::~NSBrowserApplication()
{
}


void NSBrowserApplication::MessageReceived(BMessage *message)
{
    switch (message->what) {
    case B_REFS_RECEIVED:
    case B_UI_SETTINGS_CHANGED:
    case 'back':
    case 'forw':
    case 'stop':
    case 'relo':
    case 'home':
    case 'urlc':
    case 'urle':
    case 'sear':
    case 'menu':
    case B_NETPOSITIVE_OPEN_URL:
    case B_NETPOSITIVE_BACK:
    case B_NETPOSITIVE_FORWARD:
    case B_NETPOSITIVE_HOME:
    case B_NETPOSITIVE_RELOAD:
    case B_NETPOSITIVE_STOP:
    case B_NETPOSITIVE_DOWN:
    case B_NETPOSITIVE_UP:
        break;
    default:
        BApplication::MessageReceived(message);
    }
}


void NSBrowserApplication::ArgvReceived(int32 argc, char **argv)
{
    NSBrowserWindow *win = nsbeos_find_last_window();
    if (!win) {
        return;
    }
    win->Unlock();
    BMessage *message = DetachCurrentMessage();
    nsbeos_pipe_message_top(message, win, win->Scaffolding());
}


void NSBrowserApplication::RefsReceived(BMessage *message)
{
    DetachCurrentMessage();
    NSBrowserWindow *win = nsbeos_find_last_window();
    if (!win) {
        gFirstRefsReceived = message;
        return;
    }
    win->Unlock();
    nsbeos_pipe_message_top(message, win, win->Scaffolding());
}


void NSBrowserApplication::AboutRequested()
{
    nsbeos_pipe_message(new BMessage(B_ABOUT_REQUESTED), NULL, NULL);
}


bool NSBrowserApplication::QuitRequested()
{
    nsbeos_pipe_message(new BMessage(B_QUIT_REQUESTED), NULL, NULL);
    return false;
}


#if !defined(__HAIKU__) && !defined(B_BEOS_VERSION_DANO)
extern "C" char *realpath(const char *f, char *buf);
char *realpath(const char *f, char *buf)
{
    BPath path(f, NULL, true);
    if (path.InitCheck() < 0) {
        strncpy(buf, f, MAXPATHLEN);
        return NULL;
    }
    strncpy(buf, path.Path(), MAXPATHLEN);
    return buf;
}
#endif

image_id nsbeos_find_app_path(char *path)
{
    image_info info;
    int32 cookie = 0;
    while (get_next_image_info(0, &cookie, &info) == B_OK) {
        if (((char *)&nsbeos_find_app_path >= (char *)info.text) &&
            ((char *)&nsbeos_find_app_path < (char *)info.text + info.text_size)) {
            if (path) {
                memset(path, 0, B_PATH_NAME_LENGTH);
                strncpy(path, info.name, B_PATH_NAME_LENGTH - 1);
            }
            return info.id;
        }
    }
    return B_ERROR;
}

char *find_resource(char *buf, const char *filename, const char *def)
{
    const char *cdir = NULL;
    status_t err;
    BPath path;
    char t[PATH_MAX];

    err = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
    path.Append("Wisp");
    if (err >= B_OK)
        cdir = path.Path();
    if (cdir != NULL) {
        strcpy(t, cdir);
        strcat(t, "/");
        strcat(t, filename);
        realpath(t, buf);
        if (access(buf, R_OK) == 0)
            return buf;
    }

    cdir = getenv("HOME");
    if (cdir != NULL) {
        strcpy(t, cdir);
        strcat(t, "/.wisp/");
        strcat(t, filename);
        realpath(t, buf);
        if (access(buf, R_OK) == 0)
            return buf;
    }

    cdir = getenv("WISPRES");

    if (cdir != NULL) {
        realpath(cdir, buf);
        strcat(buf, "/");
        strcat(buf, filename);
        if (access(buf, R_OK) == 0)
            return buf;
    }


    BPathFinder f((void *)find_resource);

    BPath p;
    if (f.FindPath(B_FIND_PATH_APPS_DIRECTORY, "wisp/res", p) == B_OK) {
        strcpy(t, p.Path());
        strcat(t, filename);
        realpath(t, buf);
        if (access(buf, R_OK) == 0)
            return buf;
    }

    if (def[0] == '%') {
        snprintf(t, PATH_MAX, "%s%s", path.Path(), def + 1);
        if (realpath(t, buf) == NULL) {
            strcpy(buf, t);
        }
    } else if (def[0] == '~') {
        snprintf(t, PATH_MAX, "%s%s", getenv("HOME"), def + 1);
        if (realpath(t, buf) == NULL) {
            strcpy(buf, t);
        }
    } else {
        if (realpath(def, buf) == NULL) {
            strcpy(buf, def);
        }
    }

    return buf;
}

static void check_homedir(void)
{
    status_t err;

    BPath path;
    err = find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);

    if (err < B_OK) {
        NSLOG(wisp, INFO, "Can't find user settings directory - nowhere to store state!");
        die("Wisp needs to find the user settings directory in order to run.\n");
    }

    path.Append("Wisp");
    err = create_directory(path.Path(), 0644);
    if (err < B_OK) {
        NSLOG(wisp, INFO, "Unable to create %s", path.Path());
        die("Wisp could not create its settings directory.\n");
    }
}

static int32 bapp_thread(void *arg)
{
    be_app->Lock();
    be_app->Run();
    return 0;
}

static nsurl *gui_get_resource_url(const char *path)
{
    nsurl *url = NULL;
    BString u("rsrc:///");

    if (strcmp(path, "default.css") == 0)
        path = "beosdefault.css";

    if (strcmp(path, "favicon.ico") == 0)
        path = "favicon.png";

    u << path;
    NSLOG(wisp, INFO, "(%s) -> '%s'\n", path, u.String());
    nsurl_create(u.String(), &url);
    return url;
}


#if !defined(__HAIKU__) && !defined(B_BEOS_VERSION_DANO)
#define B_PANEL_TEXT_COLOR ((color_which)10)
#define B_DOCUMENT_BACKGROUND_COLOR ((color_which)11)
#define B_DOCUMENT_TEXT_COLOR ((color_which)12)
#define B_CONTROL_BACKGROUND_COLOR ((color_which)13)
#define B_CONTROL_TEXT_COLOR ((color_which)14)
#define B_CONTROL_BORDER_COLOR ((color_which)15)
#define B_CONTROL_HIGHLIGHT_COLOR ((color_which)16)
#define B_NAVIGATION_BASE_COLOR ((color_which)4)
#define B_NAVIGATION_PULSE_COLOR ((color_which)17)
#define B_SHINE_COLOR ((color_which)18)
#define B_SHADOW_COLOR ((color_which)19)
#define B_MENU_SELECTED_BORDER_COLOR ((color_which)9)
#define B_TOOL_TIP_BACKGROUND_COLOR ((color_which)20)
#define B_TOOL_TIP_TEXT_COLOR ((color_which)21)
#define B_SUCCESS_COLOR ((color_which)100)
#define B_FAILURE_COLOR ((color_which)101)
#define B_MENU_SELECTED_BACKGROUND_COLOR B_MENU_SELECTION_BACKGROUND_COLOR
#define B_RANDOM_COLOR ((color_which)0x80000000)
#define B_MICHELANGELO_FAVORITE_COLOR ((color_which)0x80000001)
#define B_DSANDLER_FAVORITE_SKY_COLOR ((color_which)0x80000002)
#define B_DSANDLER_FAVORITE_INK_COLOR ((color_which)0x80000003)
#define B_DSANDLER_FAVORITE_SHOES_COLOR ((color_which)0x80000004)
#define B_DAVE_BROWN_FAVORITE_COLOR ((color_which)0x80000005)
#endif
#if defined(B_BEOS_VERSION_DANO)
#define B_TOOL_TIP_BACKGROUND_COLOR B_TOOLTIP_BACKGROUND_COLOR
#define B_TOOL_TIP_TEXT_COLOR B_TOOLTIP_TEXT_COLOR
#endif
#define NOCOL ((color_which)0)

static nserror set_colour_from_ui(struct nsoption_s *opts, color_which ui, enum nsoption_e option, colour def_colour)
{
    if (ui != NOCOL) {
        rgb_color c;
        if (ui == B_DESKTOP_COLOR) {
            BScreen s;
            c = s.DesktopColor();
        } else {
            c = ui_color(ui);
        }

        def_colour = ((((uint32_t)c.blue << 16) & 0xff0000) | ((c.green << 8) & 0x00ff00) | ((c.red) & 0x0000ff));
    }

    opts[option].value.c = def_colour;

    return NSERROR_OK;
}

static nserror set_option_defaults(struct nsoption_s *defaults)
{
    struct {
        color_which ui;
        colour dflt;
        enum nsoption_e option;
    } entries[] = {
        {B_DOCUMENT_TEXT_COLOR, 0x00000000, NSOPTION_sys_colour_AccentColor},
        {B_CONTROL_HIGHLIGHT_COLOR, 0x00000000, NSOPTION_sys_colour_AccentColorText},
        {B_SHINE_COLOR, 0x00000000, NSOPTION_sys_colour_ActiveText},
        {B_CONTROL_BORDER_COLOR, 0x00000000, NSOPTION_sys_colour_ButtonBorder},
        {B_CONTROL_BACKGROUND_COLOR, 0x00aaaaaa, NSOPTION_sys_colour_ButtonFace},
        {B_CONTROL_TEXT_COLOR, 0x00000000, NSOPTION_sys_colour_ButtonText},
        {B_DOCUMENT_BACKGROUND_COLOR, 0x00aaaaaa, NSOPTION_sys_colour_Canvas},
        {B_DOCUMENT_TEXT_COLOR, 0x00000000, NSOPTION_sys_colour_CanvasText},
        {B_CONTROL_BACKGROUND_COLOR, 0x00000000, NSOPTION_sys_colour_Field},
        {B_CONTROL_TEXT_COLOR, 0x00000000, NSOPTION_sys_colour_FieldText},
        {NOCOL, 0x00777777, NSOPTION_sys_colour_GrayText},
        {NOCOL, 0x00ee0000, NSOPTION_sys_colour_Highlight},
        {NOCOL, 0x00000000, NSOPTION_sys_colour_HighlightText},
        {B_LINK_TEXT_COLOR, 0x00000000, NSOPTION_sys_colour_LinkText},
        {B_CONTROL_MARK_COLOR, 0x00000000, NSOPTION_sys_colour_Mark},
        {B_CONTROL_TEXT_COLOR, 0x00000000, NSOPTION_sys_colour_MarkText},
        {B_TOOL_TIP_BACKGROUND_COLOR, 0x00000000, NSOPTION_sys_colour_SelectedItem},
        {B_TOOL_TIP_TEXT_COLOR, 0x00000000, NSOPTION_sys_colour_SelectedItemText},
        {B_LINK_VISITED_COLOR, 0x00000000, NSOPTION_sys_colour_VisitedText},
        {NOCOL, 0x00000000, NSOPTION_LISTEND},
    };

    int idx;

    for (idx = 0; entries[idx].option != NSOPTION_LISTEND; idx++) {
        set_colour_from_ui(defaults, entries[idx].ui, entries[idx].option, entries[idx].dflt);
    }

    return NSERROR_OK;
}

void nsbeos_update_system_ui_colors(void)
{
    set_option_defaults(nsoptions);
}

static bool nslog_stream_configure(FILE *fptr)
{
    setbuf(fptr, NULL);
    return true;
}

static BPath get_messages_path()
{
    BPathFinder f((void *)get_messages_path);

    BPath p;
    f.FindPath(B_FIND_PATH_APPS_DIRECTORY, "wisp/res", p);
    BString lang;
#ifdef __HAIKU__
    BMessage preferredLangs;
    if (BLocaleRoster::Default()->GetPreferredLanguages(&preferredLangs) == B_OK) {
        preferredLangs.FindString("language", 0, &lang);
        lang.Truncate(2);
    }
#endif
    if (lang.Length() < 1) {
        lang.SetTo(getenv("LC_MESSAGES"));
        lang.Truncate(2);
    }
    BDirectory d(p.Path());
    if (!d.Contains(lang.String(), B_DIRECTORY_NODE))
        lang = "en";
    p.Append(lang.String());
    p.Append("Messages");
    return p;
}


static void gui_init(int argc, char **argv)
{
    const char *addr;
    nsurl *url;
    nserror error;
    char buf[PATH_MAX];

    if (pipe(sEventPipe) < 0)
        return;
    if (!replicated) {
        sBAppThreadID = spawn_thread(
            bapp_thread, "BApplication(Wisp)", B_NORMAL_PRIORITY, (void *)find_thread(NULL));
        if (sBAppThreadID < B_OK)
            return;
        if (resume_thread(sBAppThreadID) < B_OK)
            return;
    }

    nsbeos_update_system_ui_colors();
    fetch_rsrc_register();
    check_homedir();
    create_directory(TEMP_FILENAME_PREFIX, 0700);

    {
#define STROF(n) #n
#define FIND_THROB(n) filenames[(n)] = "throbber/throbber" STROF(n) ".png";
        const char *filenames[9];
        FIND_THROB(0);
        FIND_THROB(1);
        FIND_THROB(2);
        FIND_THROB(3);
        FIND_THROB(4);
        FIND_THROB(5);
        FIND_THROB(6);
        FIND_THROB(7);
        FIND_THROB(8);
        nsbeos_throbber_initialise_from_png(9, filenames[0], filenames[1], filenames[2], filenames[3], filenames[4],
            filenames[5], filenames[6], filenames[7], filenames[8]);
#undef FIND_THROB
#undef STROF
    }

    if (nsbeos_throbber == NULL)
        die("Unable to load throbber image.\n");

    find_resource(buf, "Choices", "%/Choices");
    NSLOG(wisp, INFO, "Using '%s' as Preferences file", buf);
    options_file_location = strdup(buf);
    nsoption_read(buf, NULL);


#define SETFONTDEFAULT(OPTION, y)                                                                                      \
    if (nsoption_charp(OPTION) == NULL)                                                                                \
    nsoption_set_charp(OPTION, strdup((y)))

#ifdef __HAIKU__
    SETFONTDEFAULT(font_sans, "DejaVu Sans");
    SETFONTDEFAULT(font_serif, "DejaVu Serif");
    SETFONTDEFAULT(font_mono, "DejaVu Mono");
    SETFONTDEFAULT(font_cursive, "DejaVu Sans");
    SETFONTDEFAULT(font_fantasy, "DejaVu Sans");
#else
    SETFONTDEFAULT(font_sans, "Bitstream Vera Sans");
    SETFONTDEFAULT(font_serif, "Bitstream Vera Serif");
    SETFONTDEFAULT(font_mono, "Bitstream Vera Sans Mono");
    SETFONTDEFAULT(font_cursive, "Bitstream Vera Serif");
    SETFONTDEFAULT(font_fantasy, "Bitstream Vera Serif");
#endif

    nsbeos_options_init();
    nsoption_set_bool(core_select_menu, true);

    if (nsoption_charp(cookie_file) == NULL) {
        find_resource(buf, "Cookies", "%/Cookies");
        nsoption_set_charp(cookie_file, strdup(buf));
    }
    if (nsoption_charp(cookie_jar) == NULL) {
        find_resource(buf, "Cookies", "%/Cookies");
        nsoption_set_charp(cookie_jar, strdup(buf));
    }
    if (nsoption_charp(url_file) == NULL) {
        find_resource(buf, "URLs", "%/URLs");
        nsoption_set_charp(url_file, strdup(buf));
    }
    if (nsoption_charp(ca_path) == NULL) {
        find_resource(buf, "certs", "/etc/ssl/certs");
        nsoption_set_charp(ca_path, strdup(buf));
    }

    beos_fetch_filetype_init();
    urldb_load(nsoption_charp(url_file));
    urldb_load_cookies(nsoption_charp(cookie_file));

    if (!replicated)
        be_app->Unlock();

    if (argc > 1) {
        addr = argv[1];
    } else if (nsoption_charp(homepage_url) != NULL) {
        addr = nsoption_charp(homepage_url);
    } else {
        addr = WISP_HOMEPAGE;
    }

    error = nsurl_create(addr, &url);
    if (error == NSERROR_OK) {
        error = browser_window_create(BW_CREATE_HISTORY, url, NULL, NULL, NULL);
        nsurl_unref(url);
    }
    if (error != NSERROR_OK) {
        beos_warn_user(messages_get_errorcode(error), 0);
    }

    if (gFirstRefsReceived) {
        be_app_messenger.SendMessage(gFirstRefsReceived);
        delete gFirstRefsReceived;
        gFirstRefsReceived = NULL;
    }
}


void nsbeos_pipe_message(BMessage *message, BView *_this, struct gui_window *gui)
{
    if (message == NULL) return;
    if (_this) message->AddPointer("View", _this);
    if (gui) message->AddPointer("gui_window", gui);
    write(sEventPipe[1], &message, sizeof(void *));
}


void nsbeos_pipe_message_top(BMessage *message, BWindow *_this, struct beos_scaffolding *scaffold)
{
    if (message == NULL) return;
    if (_this) message->AddPointer("Window", _this);
    if (scaffold) message->AddPointer("scaffolding", scaffold);
    write(sEventPipe[1], &message, sizeof(void *));
}


void nsbeos_gui_poll(void)
{
    fd_set read_fd_set, write_fd_set, exc_fd_set;
    int max_fd;
    struct timeval timeout;
    unsigned int fd_count = 0;
    bigtime_t next_schedule = 0;

    schedule_run();
    fetch_fdset(&read_fd_set, &write_fd_set, &exc_fd_set, &max_fd);
    FD_SET(sEventPipe[0], &read_fd_set);
    max_fd = MAX(max_fd, sEventPipe[0]) + 1;

    if (earliest_callback_timeout != B_INFINITE_TIMEOUT) {
        next_schedule = earliest_callback_timeout - system_time();
    } else {
        next_schedule = earliest_callback_timeout;
    }

    if (next_schedule < 0) next_schedule = 0;

    timeout.tv_sec = (long)(next_schedule / 1000000LL);
    timeout.tv_usec = (long)(next_schedule % 1000000LL);

    fd_count = select(max_fd, &read_fd_set, &write_fd_set, &exc_fd_set, &timeout);

    if (fd_count > 0 && FD_ISSET(sEventPipe[0], &read_fd_set)) {
        BMessage *message;
        int len = read(sEventPipe[0], &message, sizeof(void *));
        if (len == sizeof(void *)) {
            nsbeos_dispatch_event(message);
        }
    }
    task_queue_execute_pending();
}


static void gui_quit(void)
{
    urldb_save_cookies(nsoption_charp(cookie_jar));
    urldb_save(nsoption_charp(url_file));
    free(nsoption_charp(cookie_file));
    free(nsoption_charp(cookie_jar));
    beos_fetch_filetype_fin();
    fetch_rsrc_unregister();
}

static char *url_to_path(const char *url)
{
    char *url_path;
    char *path = NULL;

    if (url_unescape(url, 0, NULL, &url_path) == NSERROR_OK) {
        path = strdup(url_path + (FILE_SCHEME_PREFIX_LEN - 1));
        free(url_path);
    }
    return path;
}

void nsbeos_gui_view_source(struct hlcache_handle *content)
{
    char *temp_name;
    bool done = false;
    BPath path;
    status_t err;
    size_t size;
    const uint8_t *source;

    source = content_get_source_data(content, &size);

    if (!content || !source) {
        beos_warn_user("MiscError", "No document source");
        return;
    }

    temp_name = url_to_path(nsurl_access(hlcache_handle_get_url(content)));
    if (temp_name) {
        path.SetTo(temp_name);
        BEntry entry;
        if (entry.SetTo(path.Path()) >= B_OK && entry.Exists() && entry.IsFile())
            done = true;
    }
    if (!done) {
        BString filename(filename_request());
        if (filename.IsEmpty()) {
            beos_warn_user("NoMemory", 0);
            return;
        }

        lwc_string *mime = content_get_mime_type(content);

        if (mime) {
            BMimeType type(lwc_string_data(mime));
            BMessage extensions;
            if (type.GetFileExtensions(&extensions) == B_OK) {
                BString ext;
                if (extensions.FindString("extensions", &ext) == B_OK)
                    filename << "." << ext;
            }
        }

        path.SetTo(TEMP_FILENAME_PREFIX);
        path.Append(filename.String());
        BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
        err = file.InitCheck();
        if (err < B_OK) {
            beos_warn_user("IOError", strerror(err));
            return;
        }
        err = file.Write(source, size);
        if (err < B_OK) {
            beos_warn_user("IOError", strerror(err));
            return;
        }

        if (mime) {
            file.WriteAttr("BEOS:TYPE", B_MIME_STRING_TYPE, 0LL, lwc_string_data(mime), lwc_string_length(mime) + 1);
            lwc_string_unref(mime);
        }
    }

    entry_ref ref;
    if (get_ref_for_path(path.Path(), &ref) < B_OK)
        return;

    BMessage m(B_REFS_RECEIVED);
    m.AddRef("refs", &ref);

    const char *editorSigs[] = {"text/x-source-code", "application/x-vnd.beunited.pe", "application/x-vnd.XEmacs",
        "application/x-vnd.Haiku-StyledEdit", "application/x-vnd.Be-STEE", "application/x-vnd.yT-STEE", NULL};
    int i;
    for (i = 0; editorSigs[i]; i++) {
        team_id team = -1;
        {
            BMessenger msgr(editorSigs[i], team);
            if (msgr.SendMessage(&m) >= B_OK)
                break;
        }

        err = be_roster->Launch(editorSigs[i], (BMessage *)&m, &team);
        if (err >= B_OK || err == B_ALREADY_RUNNING)
            break;
    }
}

static nserror gui_launch_url(struct nsurl *url)
{
    status_t status;
    BString mimeType = "application/x-vnd.Be.URL.";
    BString arg(nsurl_access(url));

    mimeType.Append(arg, arg.FindFirst(":"));

    if (arg.IFindFirst("mailto:") == 0)
        mimeType = "text/x-email";

    if (!BMimeType::IsValid(mimeType.String()))
        return NSERROR_NO_FETCH_HANDLER;
    char *args[2] = {(char *)nsurl_access(url), NULL};
    status = be_roster->Launch(mimeType.String(), 1, args);
    if (status < B_OK)
        beos_warn_user("Cannot launch url", strerror(status));
    return NSERROR_OK;
}


void die(const char *const error)
{
    fprintf(stderr, "%s", error);
    BAlert *alert;
    BString text("Cannot continue:\n");
    text << error;

    alert = new BAlert("Wisp Error", text.String(), "Debug", "Ok", NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
    if (alert->Go() < 1)
        debugger("die");

    exit(EXIT_FAILURE);
}


static struct gui_fetch_table beos_fetch_table = {
    .fetch_filetype = fetch_filetype,
    .get_resource_url = gui_get_resource_url
};

static struct gui_misc_table beos_misc_table = {
    .schedule = beos_schedule,
    .quit = gui_quit,
    .launch_url = gui_launch_url
};


int main(int argc, char **argv)
{
    nserror ret;
    BPath options;
    extern struct gui_audio_table *beos_audio_table;
    struct wisp_table beos_table = {
        .misc = &beos_misc_table,
        .window = beos_window_table,
        .download = beos_download_table,
        .clipboard = beos_clipboard_table,
        .fetch = &beos_fetch_table,
        .bitmap = beos_bitmap_table,
        .layout = beos_layout_table,
        .audio = beos_audio_table
    };

    ret = wisp_register(&beos_table);
    if (ret != NSERROR_OK) {
        die("Wisp operation table failed registration");
    }

    if (find_directory(B_USER_SETTINGS_DIRECTORY, &options, true) == B_OK) {
        options.Append("x-vnd.Wisp");
    }

    if (!replicated) {
        new NSBrowserApplication;
    }

    nslog_init(nslog_stream_configure, &argc, argv);

    ret = nsoption_init(set_option_defaults, &nsoptions, &nsoptions_default);
    if (ret != NSERROR_OK) {
        die("Options failed to initialise");
    }
    nsoption_read(options.Path(), NULL);
    nsoption_commandline(&argc, argv, NULL);

    BResources resources;
    resources.SetToImage((const void *)main);
    size_t size = 0;

    BString lang;
#ifdef __HAIKU__
    BMessage preferredLangs;
    if (BLocaleRoster::Default()->GetPreferredLanguages(&preferredLangs) == B_OK) {
        preferredLangs.FindString("language", 0, &lang);
    }
#endif
    if (lang.Length() < 1)
        lang.SetTo(getenv("LC_MESSAGES"));

    char path[12];
    snprintf(path, sizeof(path), "%.2s/Messages", lang.String());

    const uint8_t *res = (const uint8_t *)resources.LoadResource('data', path, &size);
    if (size > 0 && res != NULL) {
        ret = messages_add_from_inline(res, size);
    } else {
        BPath messages = get_messages_path();
        ret = messages_add_from_file(messages.Path());
    }

    ret = wisp_init(NULL);
    if (ret != NSERROR_OK) {
        die("Wisp failed to initialise");
    }

    gui_init(argc, argv);

    while (!nsbeos_done) {
        nsbeos_gui_poll();
    }

    nsbeos_plotters.finalise(NULL);
    nsbeos_window_finalise();
    wisp_exit();

    nsoption_finalise(nsoptions, nsoptions_default);
    nslog_finalise();

    return 0;
}

int gui_init_replicant(int argc, char **argv)
{
    nserror ret;
    BPath options;
    struct wisp_table beos_table = {
        .misc = &beos_misc_table,
        .window = beos_window_table,
        .download = beos_download_table,
        .clipboard = beos_clipboard_table,
        .fetch = &beos_fetch_table,
        .bitmap = beos_bitmap_table,
        .layout = beos_layout_table
    };

    ret = wisp_register(&beos_table);
    if (ret != NSERROR_OK) {
        die("Wisp operation table failed registration");
    }

    if (find_directory(B_USER_SETTINGS_DIRECTORY, &options, true) == B_OK) {
        options.Append("x-vnd.Wisp");
    }

    nslog_init(nslog_stream_configure, &argc, argv);

    ret = nsoption_init(set_option_defaults, &nsoptions, &nsoptions_default);
    if (ret != NSERROR_OK) {
        die("Options failed to initialise");
    }
    nsoption_read(options.Path(), NULL);
    nsoption_commandline(&argc, argv, NULL);

    BPath messages = get_messages_path();
    ret = messages_add_from_file(messages.Path());

    ret = wisp_init(NULL);
    if (ret != NSERROR_OK) {
        die("Wisp failed to initialise");
    }

    gui_init(argc, argv);

    return 0;
}
