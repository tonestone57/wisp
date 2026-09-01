#include "wisp/plotters.h"
/*
 * Copyright 2004-2010 James Bursa <bursa@users.sourceforge.net>
 * Copyright 2010-2016 Vincent Sanders <vince@netsurf-browser.org>
 * Copyright 2004-2009 John-Mark Bell <jmb@netsurf-browser.org>
 * Copyright 2009 Paul Blokus <paul_pl@users.sourceforge.net>
 * Copyright 2006-2009 Daniel Silverstone <dsilvers@netsurf-browser.org>
 * Copyright 2006-2008 Rob Kendrick <rjek@netsurf-browser.org>
 * Copyright 2008 John Tytgat <joty@netsurf-browser.org>
 * Copyright 2008 Adam Blokus <adamblokus@gmail.com>
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

#include <gtk/gtk.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <wisp/bitmap.h>
#include <wisp/browser.h>
#include <wisp/browser_window.h>
#include <wisp/content/backing_store.h>
#include <wisp/content/fetch.h>
#include <wisp/cookie_db.h>
#include <wisp/desktop/hotlist.h>
#include <wisp/desktop/save_complete.h>
#include <wisp/desktop/searchweb.h>
#include <wisp/keypress.h>
#include <wisp/wisp.h>
#include <wisp/url_db.h>
#include <wisp/utils/file.h>
#include <wisp/utils/filepath.h>
#include <wisp/utils/log.h>
#include <wisp/utils/messages.h>
#include <wisp/utils/nsoption.h>
#include <wisp/utils/utils.h>

#include "gtk/accelerator.h"
#include "gtk/bitmap.h"
#include "gtk/compat.h"
#include "gtk/completion.h"
#include "gtk/cookies.h"
#include "gtk/corewindow.h"
#include "gtk/download.h"
#include "gtk/fetch.h"
#include "gtk/global_history.h"
#include "gtk/gui.h"
#include "gtk/hotlist.h"
#include "gtk/layout_pango.h"
#include "gtk/local_history.h"
#include "gtk/misc.h"
#include "gtk/resources.h"
#include "gtk/plotters.h"
#include "wisp/plotters.h"
#include "gtk/plotters.h"
#include "gtk/plotters.h"
#include "gtk/scaffolding.h"
#include "gtk/schedule.h"
#include "gtk/search.h"
#include <wisp/utils/task_queue.h>
#include <unistd.h>
#include <fcntl.h>
#include "gtk/selection.h"
#include "gtk/throbber.h"
#include "gtk/toolbar_items.h"
#include "gtk/warn.h"
#include "gtk/window.h"
extern bool qjs_execute_pending_all(void);
#ifdef __APPLE__
#include <wisp/audio.h>
extern struct gui_audio_table *macos_audio_table;
#endif

volatile bool nsgtk_complete = false;

/* exported global defined in gtk/gui.h */
char *nsgtk_config_home;

/** favicon default pixbuf */
GdkPixbuf *favicon_pixbuf;

/** default window icon pixbuf */
GdkPixbuf *win_default_icon_pixbuf;

GtkBuilder *warning_builder;

/** resource search path vector */
char **respaths;


/* exported function documented in gtk/warn.h */
nserror nsgtk_warning(const char *warning, const char *detail)
{
    char buf[300]; /* 300 is the size the RISC OS GUI uses */
    static GtkWindow *nsgtk_warning_window;
    GtkLabel *WarningLabel;

    NSLOG(wisp, INFO, "%s %s", warning, detail ? detail : "");
    fflush(stdout);

    nsgtk_warning_window = GTK_WINDOW(gtk_builder_get_object(warning_builder, "wndWarning"));
    WarningLabel = GTK_LABEL(gtk_builder_get_object(warning_builder, "labelWarning"));

    snprintf(buf, sizeof(buf), "%s %s", messages_get(warning), detail ? detail : "");
    buf[sizeof(buf) - 1] = 0;

    gtk_label_set_text(WarningLabel, buf);

    gtk_widget_show_all(GTK_WIDGET(nsgtk_warning_window));

    return NSERROR_OK;
}


/* exported interface documented in gtk/gui.h */
uint32_t gtk_gui_gdkkey_to_nskey(GdkEventKey *key)
{
    /* this function will need to become much more complex to support
     * everything that the RISC OS version does.  But this will do for
     * now.  I hope.
     */
    switch (key->keyval) {

    case GDK_KEY(Tab):
        return NS_KEY_TAB;

    case GDK_KEY(BackSpace):
        if (key->state & GDK_SHIFT_MASK)
            return NS_KEY_DELETE_LINE_START;
        else if (key->state & GDK_CONTROL_MASK)
            return NS_KEY_DELETE_WORD_LEFT;
        else
            return NS_KEY_DELETE_LEFT;

    case GDK_KEY(Delete):
        if (key->state & GDK_SHIFT_MASK)
            return NS_KEY_DELETE_LINE_END;
        else if (key->state & GDK_CONTROL_MASK)
            return NS_KEY_DELETE_WORD_RIGHT;
        else
            return NS_KEY_DELETE_RIGHT;

    case GDK_KEY(Linefeed):
        return 13;

    case GDK_KEY(Return):
        return 10;

    case GDK_KEY(Left):
    case GDK_KEY(KP_Left):
        if (key->state & GDK_CONTROL_MASK)
            return NS_KEY_WORD_LEFT;
        return NS_KEY_LEFT;

    case GDK_KEY(Right):
    case GDK_KEY(KP_Right):
        if (key->state & GDK_CONTROL_MASK)
            return NS_KEY_WORD_RIGHT;
        return NS_KEY_RIGHT;

    case GDK_KEY(Up):
    case GDK_KEY(KP_Up):
        return NS_KEY_UP;

    case GDK_KEY(Down):
    case GDK_KEY(KP_Down):
        return NS_KEY_DOWN;

    case GDK_KEY(Home):
    case GDK_KEY(KP_Home):
        if (key->state & GDK_CONTROL_MASK)
            return NS_KEY_LINE_START;
        else
            return NS_KEY_TEXT_START;

    case GDK_KEY(End):
    case GDK_KEY(KP_End):
        if (key->state & GDK_CONTROL_MASK)
            return NS_KEY_LINE_END;
        else
            return NS_KEY_TEXT_END;

    case GDK_KEY(Page_Up):
    case GDK_KEY(KP_Page_Up):
        return NS_KEY_PAGE_UP;

    case GDK_KEY(Page_Down):
    case GDK_KEY(KP_Page_Down):
        return NS_KEY_PAGE_DOWN;

    case 'a':
        if (key->state & GDK_CONTROL_MASK)
            return NS_KEY_SELECT_ALL;
        return gdk_keyval_to_unicode(key->keyval);

    case 'u':
        if (key->state & GDK_CONTROL_MASK)
            return NS_KEY_DELETE_LINE;
        return gdk_keyval_to_unicode(key->keyval);

    case 'c':
        if (key->state & GDK_CONTROL_MASK)
            return NS_KEY_COPY_SELECTION;
        return gdk_keyval_to_unicode(key->keyval);

    case 'v':
        if (key->state & GDK_CONTROL_MASK)
            return NS_KEY_PASTE;
        return gdk_keyval_to_unicode(key->keyval);

    case 'x':
        if (key->state & GDK_CONTROL_MASK)
            return NS_KEY_CUT_SELECTION;
        return gdk_keyval_to_unicode(key->keyval);

    case 'Z':
    case 'y':
        if (key->state & GDK_CONTROL_MASK)
            return NS_KEY_REDO;
        return gdk_keyval_to_unicode(key->keyval);

    case 'z':
        if (key->state & GDK_CONTROL_MASK)
            return NS_KEY_UNDO;
        return gdk_keyval_to_unicode(key->keyval);

    case GDK_KEY(Escape):
        return NS_KEY_ESCAPE;

        /* Modifiers - do nothing for now */
    case GDK_KEY(Shift_L):
    case GDK_KEY(Shift_R):
    case GDK_KEY(Control_L):
    case GDK_KEY(Control_R):
    case GDK_KEY(Caps_Lock):
    case GDK_KEY(Shift_Lock):
    case GDK_KEY(Meta_L):
    case GDK_KEY(Meta_R):
    case GDK_KEY(Alt_L):
    case GDK_KEY(Alt_R):
    case GDK_KEY(Super_L):
    case GDK_KEY(Super_R):
    case GDK_KEY(Hyper_L):
    case GDK_KEY(Hyper_R):
        return 0;
    }
    return gdk_keyval_to_unicode(key->keyval);
}


/**
 * Create an array of valid paths to search for resources.
 *
 * The idea is that all the complex path computation to find resources
 * is performed here, once, rather than every time a resource is
 * searched for.
 */
static char **nsgtk_init_resource_path(const char *config_home)
{
    char *resource_path;
    int resource_path_len;
    const gchar *const *langv;
    char **pathv; /* resource path string vector */
    char **respath; /* resource paths vector */

    char self_path[1024];
    char exe_res_path[2048] = "";
    char exe_share_path[2048] = "";
#ifdef _WIN32
    if (GetModuleFileNameA(NULL, self_path, sizeof(self_path)) > 0) {
        char *last_backslash = strrchr(self_path, '\\');
        if (last_backslash) {
            *last_backslash = '\0';
            snprintf(exe_res_path, sizeof(exe_res_path), "%s\\res", self_path);
            snprintf(exe_share_path, sizeof(exe_share_path), "%s\\..\\share\\wisp-gtk", self_path);
        }
    }
#else
    ssize_t len = readlink("/proc/self/exe", self_path, sizeof(self_path) - 1);
    if (len != -1) {
        self_path[len] = '\0';
        char *last_slash = strrchr(self_path, '/');
        if (last_slash) {
            *last_slash = '\0';
            snprintf(exe_res_path, sizeof(exe_res_path), "%s/res", self_path);
            snprintf(exe_share_path, sizeof(exe_share_path), "%s/../share/wisp-gtk", self_path);
        }
    }
#endif

    if (config_home != NULL) {
        if (exe_res_path[0] != '\0') {
            resource_path_len = snprintf(NULL, 0, "%s:%s:%s:${WISPRES}:%s", config_home, exe_res_path, exe_share_path, GTK_RESPATH);
            resource_path = malloc(resource_path_len + 1);
            if (resource_path == NULL) {
                return NULL;
            }
            snprintf(resource_path, resource_path_len + 1, "%s:%s:%s:${WISPRES}:%s", config_home, exe_res_path, exe_share_path, GTK_RESPATH);
        } else {
            resource_path_len = snprintf(NULL, 0, "%s:${WISPRES}:%s", config_home, GTK_RESPATH);
            resource_path = malloc(resource_path_len + 1);
            if (resource_path == NULL) {
                return NULL;
            }
            snprintf(resource_path, resource_path_len + 1, "%s:${WISPRES}:%s", config_home, GTK_RESPATH);
        }
    } else {
        if (exe_res_path[0] != '\0') {
            resource_path_len = snprintf(NULL, 0, "%s:%s:${WISPRES}:%s", exe_res_path, exe_share_path, GTK_RESPATH);
            resource_path = malloc(resource_path_len + 1);
            if (resource_path == NULL) {
                return NULL;
            }
            snprintf(resource_path, resource_path_len + 1, "%s:%s:${WISPRES}:%s", exe_res_path, exe_share_path, GTK_RESPATH);
        } else {
            resource_path_len = snprintf(NULL, 0, "${WISPRES}:%s", GTK_RESPATH);
            resource_path = malloc(resource_path_len + 1);
            if (resource_path == NULL) {
                return NULL;
            }
            snprintf(resource_path, resource_path_len + 1, "${WISPRES}:%s", GTK_RESPATH);
        }
    }

    pathv = filepath_path_to_strvec(resource_path);

    langv = g_get_language_names();

    respath = filepath_generate(pathv, langv);

    filepath_free_strvec(pathv);

    free(resource_path);

    return respath;
}


/**
 * create directory name and check it is acessible and a directory.
 */
static nserror check_dirname(const char *path, const char *leaf, char **dirname_out)
{
    nserror ret;
    char *dirname = NULL;
    struct stat dirname_stat;

    ret = wisp_mkpath(&dirname, NULL, 2, path, leaf);
    if (ret != NSERROR_OK) {
        return ret;
    }

    /* ensure access is possible and the entry is actualy
     * a directory.
     */
    if (stat(dirname, &dirname_stat) == 0) {
        if (S_ISDIR(dirname_stat.st_mode)) {
            if (access(dirname, R_OK | W_OK) == 0) {
                *dirname_out = dirname;
                return NSERROR_OK;
            } else {
                ret = NSERROR_PERMISSION;
            }
        } else {
            ret = NSERROR_NOT_DIRECTORY;
        }
    } else {
        ret = NSERROR_NOT_FOUND;
    }

    free(dirname);

    return ret;
}


/**
 * Get the path to the config directory.
 *
 * @param config_home_out Path to configuration directory.
 * @return NSERROR_OK on sucess and \a config_home_out updated else error code.
 */
static nserror get_config_home(char **config_home_out)
{
    nserror ret;
    char *home_dir;
    char *xdg_config_dir;
    char *config_home;

    home_dir = getenv("HOME");

    /* The old $HOME/.neosurf/ directory should be used if it
     * exists and is accessible.
     */
    if (home_dir != NULL) {
        ret = check_dirname(home_dir, ".wisp", &config_home);
        if (ret == NSERROR_OK) {
            NSLOG(wisp, INFO, "\"%s\"", config_home);
            *config_home_out = config_home;
            return ret;
        }
    }

    /* $XDG_CONFIG_HOME defines the base directory
     * relative to which user specific configuration files
     * should be stored.
     */
    xdg_config_dir = getenv("XDG_CONFIG_HOME");

    if ((xdg_config_dir == NULL) || (*xdg_config_dir == 0)) {
        /* If $XDG_CONFIG_HOME is either not set or empty, a
         * default equal to $HOME/.config should be used.
         */

        /** @todo the meaning of empty is never defined so I
         * am assuming it is a zero length string but is it
         * supposed to mean "whitespace" and if so what counts
         * as whitespace? (are tabs etc. counted or should
         * isspace() be used)
         */

        /* the HOME envvar is required */
        if (home_dir == NULL) {
            return NSERROR_NOT_DIRECTORY;
        }

        ret = check_dirname(home_dir, ".config/wisp", &config_home);
        if (ret != NSERROR_OK) {
            return ret;
        }
    } else {
        ret = check_dirname(xdg_config_dir, "wisp", &config_home);
        if (ret != NSERROR_OK) {
            return ret;
        }
    }

    NSLOG(wisp, INFO, "\"%s\"", config_home);

    *config_home_out = config_home;
    return NSERROR_OK;
}


static nserror create_config_home(char **config_home_out)
{
    char *config_home = NULL;
    char *home_dir;
    char *xdg_config_dir;
    nserror ret;

    NSLOG(wisp, INFO, "Attempting to create configuration directory");

    /* $XDG_CONFIG_HOME defines the base directory
     * relative to which user specific configuration files
     * should be stored.
     */
    xdg_config_dir = getenv("XDG_CONFIG_HOME");

    if ((xdg_config_dir == NULL) || (*xdg_config_dir == 0)) {
        home_dir = getenv("HOME");

        if ((home_dir == NULL) || (*home_dir == 0)) {
            return NSERROR_NOT_DIRECTORY;
        }

        ret = wisp_mkpath(&config_home, NULL, 4, home_dir, ".config", "wisp", "/");
        if (ret != NSERROR_OK) {
            return ret;
        }
    } else {
        ret = wisp_mkpath(&config_home, NULL, 3, xdg_config_dir, "wisp", "/");
        if (ret != NSERROR_OK) {
            return ret;
        }
    }

    /* ensure all elements of path exist (the trailing / is required) */
    ret = wisp_mkdir_all(config_home);
    if (ret != NSERROR_OK) {
        free(config_home);
        return ret;
    }

    /* strip the trailing separator */
    config_home[strlen(config_home) - 1] = 0;

    NSLOG(wisp, INFO, "\"%s\"", config_home);

    *config_home_out = config_home;

    return NSERROR_OK;
}


/**
 * Ensures output logging stream is correctly configured
 */
static bool nslog_stream_configure(FILE *fptr)
{
    /* set log stream to be non-buffering */
    setbuf(fptr, NULL);

    return true;
}


/**
 * Set option defaults for gtk frontend.
 *
 * \param defaults The option table to update.
 * \return error status.
 */
static nserror set_defaults(struct nsoption_s *defaults)
{
    char *fname;
    GtkSettings *settings;
    GtkIconSize tooliconsize;
    GtkToolbarStyle toolbarstyle;

    /* cookie file default */
    fname = NULL;
    wisp_mkpath(&fname, NULL, 2, nsgtk_config_home, "Cookies");
    if (fname != NULL) {
        nsoption_setnull_tbl_charp(defaults, NSOPTION_cookie_file, fname);
    }

    /* cookie jar default */
    fname = NULL;
    wisp_mkpath(&fname, NULL, 2, nsgtk_config_home, "Cookies");
    if (fname != NULL) {
        nsoption_setnull_tbl_charp(defaults, NSOPTION_cookie_jar, fname);
    }

    /* url database default */
    fname = NULL;
    wisp_mkpath(&fname, NULL, 2, nsgtk_config_home, "URLs");
    if (fname != NULL) {
        nsoption_setnull_tbl_charp(defaults, NSOPTION_url_file, fname);
    }

    /* bookmark database default */
    fname = NULL;
    wisp_mkpath(&fname, NULL, 2, nsgtk_config_home, "Hotlist");
    if (fname != NULL) {
        nsoption_setnull_tbl_charp(defaults, NSOPTION_hotlist_path, fname);
    }

    /* download directory default */
    fname = getenv("HOME");
    if (fname != NULL) {
        nsoption_setnull_tbl_charp(defaults, NSOPTION_downloads_directory, strdup(fname));
    }

    if (((defaults[NSOPTION_cookie_file].value.s) == NULL) || ((defaults[NSOPTION_cookie_jar].value.s) == NULL) ||
        ((defaults[NSOPTION_url_file].value.s) == NULL) || ((defaults[NSOPTION_hotlist_path].value.s) == NULL) ||
        ((defaults[NSOPTION_downloads_directory].value.s) == NULL)) {
        NSLOG(wisp, INFO, "Failed initialising default resource paths");
        return NSERROR_BAD_PARAMETER;
    }

    /* set default font names */
    nsoption_set_tbl_charp(defaults, NSOPTION_font_sans, strdup("Sans"));
    nsoption_set_tbl_charp(defaults, NSOPTION_font_serif, strdup("Serif"));
    nsoption_set_tbl_charp(defaults, NSOPTION_font_mono, strdup("Monospace"));
    nsoption_set_tbl_charp(defaults, NSOPTION_font_cursive, strdup("Serif"));
    nsoption_set_tbl_charp(defaults, NSOPTION_font_fantasy, strdup("Serif"));

    /* Default toolbar button type to system defaults */

    settings = gtk_settings_get_default();
    g_object_get(settings, "gtk-toolbar-icon-size", &tooliconsize, "gtk-toolbar-style", &toolbarstyle, NULL);

    switch (toolbarstyle) {
    case GTK_TOOLBAR_ICONS:
        if (tooliconsize == GTK_ICON_SIZE_SMALL_TOOLBAR) {
            nsoption_set_tbl_int(defaults, NSOPTION_button_type, 1);
        } else {
            nsoption_set_tbl_int(defaults, NSOPTION_button_type, 2);
        }
        break;

    case GTK_TOOLBAR_TEXT:
        nsoption_set_tbl_int(defaults, NSOPTION_button_type, 4);
        break;

    case GTK_TOOLBAR_BOTH:
    case GTK_TOOLBAR_BOTH_HORIZ:
        /* no labels in default configuration */
    default:
        /* No system default, so use large icons */
        nsoption_set_tbl_int(defaults, NSOPTION_button_type, 2);
        break;
    }

    /* set default items in toolbar */
    nsoption_set_tbl_charp(defaults, NSOPTION_toolbar_items, strdup("back/history/forward/reloadstop/url_bar/websearch/openmenu"));

    /* set default for menu and tool bar visibility */
    nsoption_set_tbl_charp(defaults, NSOPTION_bar_show, strdup("tool"));

    return NSERROR_OK;
}


/**
 * Initialise user options
 *
 * Initialise the browser configuration options. These are set by:
 *  - set generic defaults suitable for the gtk frontend
 *  - user choices loaded from Choices file
 *  - command line parameters
 */
static nserror nsgtk_option_init(int *pargc, char **argv)
{
    nserror ret;
    char *choices = NULL;

    /* user options setup */
    ret = nsoption_init(set_defaults, &nsoptions, &nsoptions_default);
    if (ret != NSERROR_OK) {
        return ret;
    }

    /* Attempt to load the user choices */
    ret = wisp_mkpath(&choices, NULL, 2, nsgtk_config_home, "Choices");
    if (ret == NSERROR_OK) {
        nsoption_read(choices, nsoptions, NULL);
        free(choices);
    }

    /* overide loaded options with those from commandline */
    nsoption_commandline(pargc, argv, nsoptions);

    /* ensure all options fall within sensible bounds */

    /* Attempt to handle nonsense status bar widths.  These may exist
     * in people's Choices as the GTK front end used to abuse the
     * status bar width option by using it for an absolute value in px.
     * The GTK front end now correctly uses it as a proportion of window
     * width.  Here we assume that a value of less than 15% is wrong
     * and set to the default two thirds. */
    if (nsoption_int(toolbar_status_size) < 1500) {
        nsoption_set_int(toolbar_status_size, 6667);
    }

    return NSERROR_OK;
}


/**
 * initialise message translation
 */
static nserror nsgtk_messages_init(char **respaths)
{
    const char *messages;
    nserror ret;
    const uint8_t *data;
    size_t data_size;

    ret = nsgtk_data_from_resname("Messages", &data, &data_size);
    if (ret == NSERROR_OK) {
        ret = messages_add_from_inline(data, data_size);
    } else {
        /* Obtain path to messages */
        ret = nsgtk_path_from_resname("Messages", &messages);
        if (ret == NSERROR_OK) {
            ret = messages_add_from_file(messages);
        }
    }
    return ret;
}


/**
 * Get the path to the cache directory.
 *
 * @param cache_home_out Path to cache directory.
 * @return NSERROR_OK on sucess and \a cache_home_out updated else error code.
 */
static nserror get_cache_home(char **cache_home_out)
{
    nserror ret;
    char *xdg_cache_dir;
    char *cache_home;
    char *home_dir;

    /* $XDG_CACHE_HOME defines the base directory relative to
     * which user specific non-essential data files should be
     * stored.
     */
    xdg_cache_dir = getenv("XDG_CACHE_HOME");

    if ((xdg_cache_dir == NULL) || (*xdg_cache_dir == 0)) {
        /* If $XDG_CACHE_HOME is either not set or empty, a
         * default equal to $HOME/.cache should be used.
         */

        home_dir = getenv("HOME");

        /* the HOME envvar is required */
        if (home_dir == NULL) {
            return NSERROR_NOT_DIRECTORY;
        }

        ret = check_dirname(home_dir, ".cache/wisp", &cache_home);
        if (ret != NSERROR_OK) {
            return ret;
        }
    } else {
        ret = check_dirname(xdg_cache_dir, "wisp", &cache_home);
        if (ret != NSERROR_OK) {
            return ret;
        }
    }

    NSLOG(wisp, INFO, "\"%s\"", cache_home);

    *cache_home_out = cache_home;
    return NSERROR_OK;
}


/**
 * create a cache directory
 */
static nserror create_cache_home(char **cache_home_out)
{
    char *cache_home = NULL;
    char *home_dir;
    char *xdg_cache_dir;
    nserror ret;

    NSLOG(wisp, INFO, "Attempting to create cache directory");

    /* $XDG_CACHE_HOME defines the base directory
     * relative to which user specific cache files
     * should be stored.
     */
    xdg_cache_dir = getenv("XDG_CACHE_HOME");

    if ((xdg_cache_dir == NULL) || (*xdg_cache_dir == 0)) {
        home_dir = getenv("HOME");

        if ((home_dir == NULL) || (*home_dir == 0)) {
            return NSERROR_NOT_DIRECTORY;
        }

        ret = wisp_mkpath(&cache_home, NULL, 4, home_dir, ".cache", "wisp", "/");
        if (ret != NSERROR_OK) {
            return ret;
        }
    } else {
        ret = wisp_mkpath(&cache_home, NULL, 3, xdg_cache_dir, "wisp", "/");
        if (ret != NSERROR_OK) {
            return ret;
        }
    }

    /* ensure all elements of path exist (the trailing / is required) */
    ret = wisp_mkdir_all(cache_home);
    if (ret != NSERROR_OK) {
        free(cache_home);
        return ret;
    }

    /* strip the trailing separator */
    cache_home[strlen(cache_home) - 1] = 0;

    NSLOG(wisp, INFO, "\"%s\"", cache_home);

    *cache_home_out = cache_home;

    return NSERROR_OK;
}


/**
 * GTK specific initialisation
 */
static nserror nsgtk_init(int *pargc, char ***pargv, char **cache_home)
{
    nserror ret;
    void wisp_gui_pump_events(void);
    extern void (*wisp_gui_pump_events_hook)(void);
    wisp_gui_pump_events_hook = wisp_gui_pump_events;

    /* Locate the correct user configuration directory path */
    ret = get_config_home(&nsgtk_config_home);
    if (ret == NSERROR_NOT_FOUND) {
        /* no config directory exists yet so try to create one */
        ret = create_config_home(&nsgtk_config_home);
    }
    if (ret != NSERROR_OK) {
        NSLOG(wisp, INFO, "Unable to locate a configuration directory.");
        nsgtk_config_home = NULL;
    }

    /* Initialise gtk */
    gtk_init(pargc, pargv);

    /* initialise logging. Not fatal if it fails but not much we
     * can do about it either.
     */
    nslog_init(nslog_stream_configure, pargc, *pargv);

    /* build the common resource path list */
    respaths = nsgtk_init_resource_path(nsgtk_config_home);
    if (respaths == NULL) {
        fprintf(stderr, "Unable to locate resources\n");
        return 1;
    }

    /* initialise the gtk resource handling */
    ret = nsgtk_init_resources(respaths);
    if (ret != NSERROR_OK) {
        fprintf(stderr, "GTK resources failed to initialise (%s)\n", messages_get_errorcode(ret));
        return ret;
    }

    /* Initialise user options */
    ret = nsgtk_option_init(pargc, *pargv);
    if (ret != NSERROR_OK) {
        fprintf(stderr, "Options failed to initialise (%s)\n", messages_get_errorcode(ret));
        return ret;
    }

    /* Initialise translated messages */
    ret = nsgtk_messages_init(respaths);
    if (ret != NSERROR_OK) {
        fprintf(stderr, "Unable to load translated messages (%s)\n", messages_get_errorcode(ret));
        NSLOG(wisp, WARNING, "Unable to load translated messages");
    }

    /* Locate the correct user cache directory path */
    ret = get_cache_home(cache_home);
    if (ret == NSERROR_NOT_FOUND) {
        /* no cache directory exists yet so try to create one */
        ret = create_cache_home(cache_home);
    }
    if (ret != NSERROR_OK) {
        NSLOG(wisp, INFO, "Unable to locate a cache directory.");
    }


    return NSERROR_OK;
}


static gboolean nsgtk_js_event_loop_cb(gpointer user_data)
{
    qjs_execute_pending_all();
    return TRUE; /* G_SOURCE_CONTINUE */
}

#if GTK_CHECK_VERSION(3, 14, 0)

/**
 * adds named icons into gtk theme
 */
static nserror nsgtk_add_named_icons_to_theme(void)
{
    gtk_icon_theme_add_resource_path(gtk_icon_theme_get_default(), "/org/wisp/icons");
    return NSERROR_OK;
}

#else

static nserror add_builtin_icon(const char *prefix, const char *name, int x, int y)
{
    GdkPixbuf *pixbuf;
    nserror res;
    char *resname;
    int resnamelen;

    /* resource name string length allowing for / .png and termination */
    resnamelen = strlen(prefix) + strlen(name) + 5 + 1 + 4 + 1;
    resname = malloc(resnamelen);
    if (resname == NULL) {
        return NSERROR_NOMEM;
    }
    snprintf(resname, resnamelen, "icons%s/%s.png", prefix, name);

    res = nsgdk_pixbuf_new_from_resname(resname, &pixbuf);
    NSLOG(wisp, DEEPDEBUG, "%d %s", res, resname);
    free(resname);
    if (res != NSERROR_OK) {
        pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, false, 8, x, y);
    }
    gtk_icon_theme_add_builtin_icon(name, y, pixbuf);

    return NSERROR_OK;
}


/**
 * adds named icons into gtk theme
 */
static nserror nsgtk_add_named_icons_to_theme(void)
{
    /* these must also be in gtk/resources.c pixbuf_resource *and*
     * gtk/res/netsurf.gresource.xml
     */
    add_builtin_icon("", "local-history", 8, 32);
    add_builtin_icon("", "show-cookie", 24, 24);
    add_builtin_icon("/24x24/actions", "page-info-insecure", 24, 24);
    add_builtin_icon("/24x24/actions", "page-info-internal", 24, 24);
    add_builtin_icon("/24x24/actions", "page-info-local", 24, 24);
    add_builtin_icon("/24x24/actions", "page-info-secure", 24, 24);
    add_builtin_icon("/24x24/actions", "page-info-warning", 24, 24);
    add_builtin_icon("/48x48/actions", "page-info-insecure", 48, 48);
    add_builtin_icon("/48x48/actions", "page-info-internal", 48, 48);
    add_builtin_icon("/48x48/actions", "page-info-local", 48, 48);
    add_builtin_icon("/48x48/actions", "page-info-secure", 48, 48);
    add_builtin_icon("/48x48/actions", "page-info-warning", 48, 48);

    return NSERROR_OK;
}

#endif


/**
 * setup GTK specific parts of the browser.
 *
 * \param argc The number of arguments on the command line
 * \param argv A string vector of command line arguments.
 * \respath A string vector of the path elements of resources
 */
static nserror nsgtk_setup(int argc, char **argv, char **respath)
{
    char buf[PATH_MAX];
    char *resource_filename;
    char *addr = NULL;
    nsurl *url;
    nserror res;

    /* Initialise gtk accelerator table */
    res = nsgtk_accelerator_init(respaths);
    if (res != NSERROR_OK) {
        NSLOG(wisp, INFO, "Unable to load gtk accelerator configuration");
        /* not fatal if this does not load */
    }

    /* initialise warning dialog */
    res = nsgtk_builder_new_from_resname("warning", &warning_builder);
    if (res != NSERROR_OK) {
        NSLOG(wisp, INFO, "Unable to initialise warning dialog");
        return res;
    }

    gtk_builder_connect_signals(warning_builder, NULL);

    /* set default icon if its available */
    res = nsgdk_pixbuf_new_from_resname("wisp.xpm", &win_default_icon_pixbuf);
    if (res == NSERROR_OK) {
        NSLOG(wisp, INFO, "Setting default window icon");
        gtk_window_set_default_icon(win_default_icon_pixbuf);
    }

    /* Search engine sources */
    resource_filename = filepath_find(respath, "SearchEngines");
    search_web_init(resource_filename);
    if (resource_filename != NULL) {
        NSLOG(wisp, INFO, "Using '%s' as Search Engines file", resource_filename);
        free(resource_filename);
    }
    search_web_select_provider(nsoption_charp(search_web_provider));

    /* Default favicon */
    res = nsgdk_pixbuf_new_from_resname("favicon.png", &favicon_pixbuf);
    if (res == NSERROR_OK && favicon_pixbuf != NULL) {
        GdkPixbuf *scaled = gdk_pixbuf_scale_simple(favicon_pixbuf, 16, 16, GDK_INTERP_BILINEAR);
        if (scaled != NULL) {
            g_object_unref(favicon_pixbuf);
            favicon_pixbuf = scaled;
        }
    } else if (favicon_pixbuf == NULL) {
        favicon_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, false, 8, 16, 16);
    }

    /* add named icons to gtk theme */
    res = nsgtk_add_named_icons_to_theme();
    if (res != NSERROR_OK) {
        NSLOG(wisp, INFO, "Unable to add named icons to GTK theme.");
        return res;
    }

    /* initialise throbber */
    res = nsgtk_throbber_init();
    if (res != NSERROR_OK) {
        NSLOG(wisp, INFO, "Unable to initialise throbber.");
        return res;
    }

    /* Initialise completions - cannot fail */
    nsgtk_completion_init();

    /* The tree view system needs to know the screen's DPI, so we
     * find that out here, rather than when we create a first browser
     * window.
     */
    browser_set_dpi(gdk_screen_get_resolution(gdk_screen_get_default()));
    NSLOG(wisp, INFO, "Set CSS DPI to %d", browser_get_dpi());

    bitmap_set_format(&(bitmap_fmt_t){
        .layout = BITMAP_LAYOUT_ARGB8888,
        .pma = true,
    });

    struct stat statbuf;
    if (stat("/etc/mime.types", &statbuf) == 0 && S_ISREG(statbuf.st_mode)) {
        strncpy(buf, "/etc/mime.types", PATH_MAX);
        buf[PATH_MAX - 1] = '\0';
    } else {
        filepath_sfinddef(respath, buf, "mime.types", "/etc/");
    }
    gtk_fetch_filetype_init(buf);

    save_complete_init();

    urldb_load(nsoption_charp(url_file));
    urldb_load_cookies(nsoption_charp(cookie_file));
    hotlist_init(nsoption_charp(hotlist_path), nsoption_charp(hotlist_path));

    /* Initialise top level UI elements */
    res = nsgtk_download_init();
    if (res != NSERROR_OK) {
        NSLOG(wisp, INFO, "Unable to initialise download window.");
        return res;
    }

    /* If there is a url specified on the command line use it */
    if (argc > 1) {
        struct stat fs;
        if (stat(argv[1], &fs) == 0) {
            size_t addrlen;
            char *rp = realpath(argv[1], NULL);
            assert(rp != NULL);

            /* calculate file url length including terminator */
            addrlen = SLEN("file://") + strlen(rp) + 1;
            addr = malloc(addrlen);
            assert(addr != NULL);
            snprintf(addr, addrlen, "file://%s", rp);
            free(rp);
        } else {
            addr = strdup(argv[1]);
        }
    }
    if (addr != NULL) {
        /* managed to set up based on local launch */
    } else if (nsoption_charp(homepage_url) != NULL) {
        addr = strdup(nsoption_charp(homepage_url));
    } else {
        addr = strdup(WISP_HOMEPAGE);
    }

    /* create an initial browser window */
    res = nsurl_create(addr, &url);
    if (res != NSERROR_OK) {
        res = search_web_omni(addr, SEARCH_WEB_OMNI_NONE, &url);
    }
    if (res == NSERROR_OK) {
        res = browser_window_create(BW_CREATE_HISTORY, url, NULL, NULL, NULL);
        nsurl_unref(url);
    }

    free(addr);

    /* Hook QuickJS Event Loop and Timer processing to GTK event loop */
    g_timeout_add(10, nsgtk_js_event_loop_cb, NULL);

    return res;
}


typedef struct {
    int fd;
    guint watch_id;
    GIOCondition cond;
} nsgtk_fetch_watch_t;

#define MAX_FETCH_WATCHES 128
static nsgtk_fetch_watch_t fetch_watches[MAX_FETCH_WATCHES];
static size_t fetch_watch_count = 0;

static gboolean nsgtk_fetch_io_cb(GIOChannel *source, GIOCondition condition, gpointer data)
{
    fetch_poll_all();
    schedule_run();
    qjs_execute_pending_all();
    return TRUE; /* Keep watching */
}

static void nsgtk_update_fetch_watches(fd_set *read_fds, fd_set *write_fds, fd_set *exc_fds, int max_fd)
{
    bool kept[MAX_FETCH_WATCHES] = { false };

    if (max_fd >= 0 && read_fds && write_fds && exc_fds) {
        for (int i = 0; i <= max_fd; i++) {
            GIOCondition cond = 0;
            if (FD_ISSET(i, read_fds))  cond |= G_IO_IN | G_IO_HUP | G_IO_ERR;
            if (FD_ISSET(i, write_fds)) cond |= G_IO_OUT | G_IO_ERR;
            if (FD_ISSET(i, exc_fds))   cond |= G_IO_ERR;

            if (cond != 0) {
                int found_idx = -1;
                for (size_t w = 0; w < fetch_watch_count; w++) {
                    if (fetch_watches[w].fd == i) {
                        found_idx = (int)w;
                        break;
                    }
                }

                if (found_idx >= 0) {
                    if (fetch_watches[found_idx].cond == cond) {
                        kept[found_idx] = true;
                    } else {
                        g_source_remove(fetch_watches[found_idx].watch_id);
                        GIOChannel *chan = g_io_channel_unix_new(i);
                        guint wid = g_io_add_watch(chan, cond, nsgtk_fetch_io_cb, NULL);
                        g_io_channel_unref(chan);
                        fetch_watches[found_idx].watch_id = wid;
                        fetch_watches[found_idx].cond = cond;
                        kept[found_idx] = true;
                    }
                } else if (fetch_watch_count < MAX_FETCH_WATCHES) {
                    GIOChannel *chan = g_io_channel_unix_new(i);
                    guint wid = g_io_add_watch(chan, cond, nsgtk_fetch_io_cb, NULL);
                    g_io_channel_unref(chan);
                    fetch_watches[fetch_watch_count].fd = i;
                    fetch_watches[fetch_watch_count].watch_id = wid;
                    fetch_watches[fetch_watch_count].cond = cond;
                    kept[fetch_watch_count] = true;
                    fetch_watch_count++;
                }
            }
        }
    }

    size_t write_idx = 0;
    for (size_t r = 0; r < fetch_watch_count; r++) {
        if (!kept[r]) {
            g_source_remove(fetch_watches[r].watch_id);
        } else {
            if (write_idx != r) {
                fetch_watches[write_idx] = fetch_watches[r];
            }
            write_idx++;
        }
    }
    fetch_watch_count = write_idx;
}

/**
 * Run the gtk event loop.
 *
 * Uses GLib GIOChannel watches for active fetch file descriptors so
 * the main context can block cleanly on I/O events without busy-waiting.
 */
static void nsgtk_main(void)
{
    fd_set read_fd_set, write_fd_set, exc_fd_set;
    int max_fd;

    while (!nsgtk_complete) {
        while (gtk_events_pending())
            gtk_main_iteration_do(TRUE);

        schedule_run();

        fetch_fdset(&read_fd_set, &write_fd_set, &exc_fd_set, &max_fd);
        nsgtk_update_fetch_watches(&read_fd_set, &write_fd_set, &exc_fd_set, max_fd);

        gtk_main_iteration();

        qjs_execute_pending_all();
    }

    nsgtk_update_fetch_watches(NULL, NULL, NULL, -1);
}


/**
 * finalise the browser
 */
int nsgtk_wake_pipe[2] = {-1, -1};

static gboolean nsgtk_task_queue_wake_cb(GIOChannel *source, GIOCondition condition, gpointer data)
{
    char buf[1];
    if (read(nsgtk_wake_pipe[0], buf, 1) > 0) {
        task_queue_execute_pending();
    }
    return TRUE;
}

void nsgtk_task_queue_wake(void)
{
    if (nsgtk_wake_pipe[1] != -1) {
        char buf[1] = {'w'};
        if (write(nsgtk_wake_pipe[1], buf, 1) == -1) {
            /* Handle warning? */
        }
    }
}

void wisp_gui_pump_events(void)
{
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }
    qjs_execute_pending_all();
    schedule_run();
}

static void nsgtk_finalise(void)
{
    nserror res;

    NSLOG(wisp, INFO, "Quitting GUI");
    nsgtk_plotters.finalise(NULL);

    /* Ensure all scaffoldings and browser windows are destroyed before we go into exit */
    nsgtk_scaffolding_destroy_all();
    while (window_list != NULL) {
        struct gui_window *gw = window_list;
        browser_window_destroy(nsgtk_get_browser_window(gw));
    }

    /* Drain scheduled tasks (such as deferred content_actually_destroy callbacks) */
    schedule_run();
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }

    while (gtk_events_pending()) {
        gtk_main_iteration();
    }
    nsgtk_download_destroy();
    urldb_save_cookies(nsoption_charp(cookie_jar));
    urldb_save(nsoption_charp(url_file));

    res = nsgtk_cookies_destroy();
    if (res != NSERROR_OK) {
        NSLOG(wisp, INFO, "Error finalising cookie viewer: %s", messages_get_errorcode(res));
    }

    res = nsgtk_local_history_destroy();
    if (res != NSERROR_OK) {
        NSLOG(wisp, INFO, "Error finalising local history viewer: %s", messages_get_errorcode(res));
    }

    res = nsgtk_global_history_destroy();
    if (res != NSERROR_OK) {
        NSLOG(wisp, INFO, "Error finalising global history viewer: %s", messages_get_errorcode(res));
    }

    res = nsgtk_hotlist_destroy();
    if (res != NSERROR_OK) {
        NSLOG(wisp, INFO, "Error finalising hotlist viewer: %s", messages_get_errorcode(res));
    }

    res = hotlist_fini();
    if (res != NSERROR_OK) {
        NSLOG(wisp, INFO, "Error finalising hotlist: %s", messages_get_errorcode(res));
    }

    res = save_complete_finalise();
    if (res != NSERROR_OK) {
        NSLOG(wisp, INFO, "Error finalising save complete: %s", messages_get_errorcode(res));
    }

    free(nsgtk_config_home);
    if (respaths != NULL) {
        filepath_free_strvec(respaths);
        respaths = NULL;
    }

    /* common finalisation */
    wisp_exit();

    gtk_fetch_filetype_fin();
    nsgtk_free_resources();

    /* Clean up scheduled callbacks */
    nsgtk_schedule_finalise();

    /* finalise options */
    nsoption_finalise(nsoptions, nsoptions_default);

    if (favicon_pixbuf != NULL) {
        g_object_unref(favicon_pixbuf);
        favicon_pixbuf = NULL;
    }
    if (win_default_icon_pixbuf != NULL) {
        g_object_unref(win_default_icon_pixbuf);
        win_default_icon_pixbuf = NULL;
    }
    if (warning_builder != NULL) {
        g_object_unref(warning_builder);
        warning_builder = NULL;
    }

    while (gtk_events_pending()) {
        gtk_main_iteration();
    }

    /* finalise logging */
    nslog_finalise();
}


#include <signal.h>

static void nsgtk_signal_handler(int sig)
{
    nsgtk_complete = true;
    if (nsgtk_wake_pipe[1] != -1) {
        char buf[1] = {'w'};
        (void)write(nsgtk_wake_pipe[1], buf, 1);
    }
    g_main_context_wakeup(NULL);
}

/**
 * Main entry point from OS.
 */
int main(int argc, char **argv)
{
    signal(SIGINT, nsgtk_signal_handler);
    signal(SIGTERM, nsgtk_signal_handler);

    nserror res;
    char *cache_home = NULL;
    extern struct gui_audio_table *nsgtk_audio_table;
    struct wisp_table nsgtk_table = {
        .misc = nsgtk_misc_table,
        .window = nsgtk_window_table,
        .corewindow = nsgtk_core_window_table,
        .clipboard = nsgtk_clipboard_table,
        .download = nsgtk_download_table,
        .fetch = nsgtk_fetch_table,
        .llcache = filesystem_llcache_table,
        .search = nsgtk_search_table,
        .search_web = nsgtk_search_web_table,
        .bitmap = nsgtk_bitmap_table,
        .layout = nsgtk_layout_table,
        #ifdef __APPLE__
        .audio = macos_audio_table,
#else
        .audio = nsgtk_audio_table,
#endif
    };

    res = wisp_register(&nsgtk_table);
    if (res != NSERROR_OK) {
        fprintf(stderr, "Wisp operation table failed registration (%s)\n", messages_get_errorcode(res));
        return 1;
    }

    /* gtk specific initialisation */
    res = nsgtk_init(&argc, &argv, &cache_home);
    if (res != NSERROR_OK) {
        fprintf(stderr, "Wisp gtk failed to initialise (%s)\n", messages_get_errorcode(res));
        return 2;
    }

    /* core initialisation */
    res = wisp_init(cache_home);
    free(cache_home);
    if (res != NSERROR_OK) {
        fprintf(stderr, "Wisp core failed to initialise (%s)\n", messages_get_errorcode(res));
        return 3;
    }

    /* gtk specific initalisation and main run loop */
    res = nsgtk_setup(argc, argv, respaths);
    if (res != NSERROR_OK) {
        nsgtk_finalise();
        fprintf(stderr, "Wisp gtk setup failed (%s)\n", messages_get_errorcode(res));
        return 4;
    }

    if (pipe(nsgtk_wake_pipe) == 0) {
        int flags = fcntl(nsgtk_wake_pipe[0], F_GETFL, 0);
        fcntl(nsgtk_wake_pipe[0], F_SETFL, flags | O_NONBLOCK);
        flags = fcntl(nsgtk_wake_pipe[1], F_GETFL, 0);
        fcntl(nsgtk_wake_pipe[1], F_SETFL, flags | O_NONBLOCK);

        GIOChannel *chan = g_io_channel_unix_new(nsgtk_wake_pipe[0]);
        g_io_add_watch(chan, G_IO_IN, nsgtk_task_queue_wake_cb, NULL);
        g_io_channel_unref(chan);
    }

    nsgtk_main();

    nsgtk_finalise();

    if (nsgtk_wake_pipe[0] != -1) {
        close(nsgtk_wake_pipe[0]);
        close(nsgtk_wake_pipe[1]);
        nsgtk_wake_pipe[0] = -1;
        nsgtk_wake_pipe[1] = -1;
    }

    return 0;
}

#if defined(__clang__) || defined(__GNUC__)
__attribute__((used))
#endif
const char *__lsan_default_suppressions(void)
{
    return "leak:libfontconfig\n"
           "leak:libglib\n"
           "leak:libpango\n"
           "leak:libgtk\n"
           "leak:libcairo\n"
           "leak:libgobject\n"
           "leak:gdk_pango\n";
}
