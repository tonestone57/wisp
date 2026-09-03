/*
 * Copyright 2009 Mark Benjamin <netsurf-browser.org.MarkBenjamin@dfgh.net>
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

/**
 * \file
 * windows frontend download implementation
 */

#include <wisp/ns_inttypes.h>
#include "wisp/utils/inet.h" /* get correct winsock ordering */
#include <limits.h>
#include <shlobj.h>
#include <windows.h>

#include <sys/time.h>
#include "wisp/content/fetch.h"
#include "wisp/desktop/download.h"
#include "wisp/download.h"
#include "wisp/utils/log.h"
#include "wisp/utils/messages.h"
#include "wisp/utils/nsoption.h"
#include "wisp/utils/nsurl.h"
#include "wisp/utils/string.h"
#include "wisp/utils/utils.h"

#include "windows/download.h"
#include "windows/gui.h"
#include "windows/resourceid.h"
#include "windows/schedule.h"
#include "windows/window.h"

struct gui_download_window {
    HWND hwnd;
    download_context *ctx;
    struct gui_window *window;
    FILE *file;

    char *title;
    char *filename;
    char *domain;
    char *total_size_str;

    unsigned long long size;
    unsigned long long downloaded;
    unsigned int progress;
    int time_remaining;
    struct timeval start_time;
    download_status status;
};

static void nsws_download_update_label(void *p);
static void nsws_download_update_progress(void *p);

static void nsws_download_destroy(struct gui_download_window *w)
{
    if (w == NULL)
        return;

    win32_schedule(-1, nsws_download_update_progress, (void *)w);
    win32_schedule(-1, nsws_download_update_label, (void *)w);

    if (w->hwnd != NULL) {
        HWND hwnd = w->hwnd;
        w->hwnd = NULL;
        SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)NULL);
        DestroyWindow(hwnd);
    }

    if (w->file != NULL) {
        fclose(w->file);
        w->file = NULL;
    }

    if (w->ctx != NULL) {
        download_context_destroy(w->ctx);
        w->ctx = NULL;
    }

    free(w->title);
    free(w->filename);
    free(w->domain);
    free(w->total_size_str);
    free(w);
}

static void nsws_download_update_label(void *p)
{
    struct gui_download_window *w = p;
    if (w == NULL || w->hwnd == NULL) {
        return;
    }

    HWND sub = GetDlgItem(w->hwnd, IDC_DOWNLOAD_LABEL);
    if (sub == NULL)
        return;

    char *size_downloaded_str = human_friendly_bytesize((int)w->downloaded);
    char time_left_str[64];

    if (w->status == DOWNLOAD_COMPLETE) {
        snprintf(time_left_str, sizeof(time_left_str), "%s",
            messages_get("gtkComplete") ? messages_get("gtkComplete") : "Complete");
    } else if (w->status == DOWNLOAD_ERROR) {
        snprintf(time_left_str, sizeof(time_left_str), "%s",
            messages_get("gtkError") ? messages_get("gtkError") : "Error");
    } else if (w->status == DOWNLOAD_CANCELED) {
        snprintf(time_left_str, sizeof(time_left_str), "%s",
            messages_get("gtkCanceled") ? messages_get("gtkCanceled") : "Canceled");
    } else if (w->time_remaining < 0) {
        snprintf(time_left_str, sizeof(time_left_str), "%s", messages_get("UnknownSize"));
    } else if (w->time_remaining > 3600) {
        snprintf(time_left_str, sizeof(time_left_str), "%d h %d m",
            w->time_remaining / 3600, (w->time_remaining % 3600) / 60);
    } else if (w->time_remaining > 60) {
        snprintf(time_left_str, sizeof(time_left_str), "%d m %d s",
            w->time_remaining / 60, w->time_remaining % 60);
    } else {
        snprintf(time_left_str, sizeof(time_left_str), "%d s", w->time_remaining);
    }

    char label[1024];
    snprintf(label, sizeof(label),
        "download %s  from %s to %s\n[%s\t/\t%s] [%u%%]\n"
        "estimate of time remaining %s",
        w->title ? w->title : "",
        w->domain ? w->domain : "",
        w->filename ? w->filename : "",
        size_downloaded_str ? size_downloaded_str : "",
        w->total_size_str ? w->total_size_str : "",
        w->progress / 100,
        time_left_str);

    SendMessage(sub, WM_SETTEXT, (WPARAM)0, (LPARAM)label);

    if (w->status == DOWNLOAD_WORKING) {
        win32_schedule(500, nsws_download_update_label, p);
    }
}

static void nsws_download_update_progress(void *p)
{
    struct gui_download_window *w = p;
    if (w == NULL || w->hwnd == NULL) {
        return;
    }

    HWND sub = GetDlgItem(w->hwnd, IDC_DOWNLOAD_PROGRESS);
    if (sub != NULL) {
        SendMessage(sub, PBM_SETPOS, (WPARAM)(w->progress / 100), 0);
    }

    if (w->status == DOWNLOAD_WORKING) {
        win32_schedule(500, nsws_download_update_progress, p);
    }
}

static INT_PTR CALLBACK nsws_download_event_callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    struct gui_download_window *w = (struct gui_download_window *)GetWindowLongPtr(hwnd, DWLP_USER);

    switch (msg) {
    case WM_INITDIALOG:
        w = (struct gui_download_window *)lparam;
        SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)w);
        if (w != NULL) {
            w->hwnd = hwnd;
            nsws_download_update_label(w);
            nsws_download_update_progress(w);
        }
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wparam)) {
        case IDOK:
            if (w != NULL) {
                if (w->status == DOWNLOAD_WORKING) {
                    return TRUE;
                }
                nsws_download_destroy(w);
            } else {
                EndDialog(hwnd, IDOK);
            }
            return TRUE;

        case IDCANCEL:
            if (w != NULL) {
                if (w->status == DOWNLOAD_WORKING && w->ctx != NULL) {
                    w->status = DOWNLOAD_CANCELED;
                    download_context_abort(w->ctx);
                }
                nsws_download_destroy(w);
            } else {
                EndDialog(hwnd, IDCANCEL);
            }
            return TRUE;
        }
        break;

    case WM_CLOSE:
        if (w != NULL) {
            if (w->status == DOWNLOAD_WORKING && w->ctx != NULL) {
                w->status = DOWNLOAD_CANCELED;
                download_context_abort(w->ctx);
            }
            nsws_download_destroy(w);
        } else {
            EndDialog(hwnd, IDCANCEL);
        }
        return TRUE;
    }

    return FALSE;
}

static bool nsws_download_window_up(struct gui_download_window *w)
{
    w->hwnd = CreateDialogParam(
        hinst, MAKEINTRESOURCE(IDD_DOWNLOAD), gui_window_main_window(w->window),
        nsws_download_event_callback, (LPARAM)w);
    if (w->hwnd == NULL) {
        return false;
    }
    ShowWindow(w->hwnd, SW_SHOW);
    return true;
}

static struct gui_download_window *gui_download_window_create(download_context *ctx, struct gui_window *gui)
{
    struct gui_download_window *w = calloc(1, sizeof(struct gui_download_window));
    if (w == NULL) {
        win32_warning(messages_get("NoMemory"), 0);
        return NULL;
    }

    w->ctx = ctx;
    w->window = gui;
    w->size = download_context_get_total_length(ctx);

    const char *suggested_filename = download_context_get_filename(ctx);
    char *filename = NULL;
    if (suggested_filename != NULL && suggested_filename[0] != '\0') {
        filename = strdup(suggested_filename);
    } else {
        nsurl *url = download_context_get_url(ctx);
        if (url == NULL || nsurl_nice(url, &filename, false) != NSERROR_OK) {
            filename = strdup(messages_get("UnknownFile"));
        }
    }
    if (filename == NULL) {
        win32_warning(messages_get("NoMemory"), 0);
        free(w);
        return NULL;
    }

    char *domain = NULL;
    nsurl *url = download_context_get_url(ctx);
    if (url != NULL && nsurl_has_component(url, NSURL_HOST)) {
        domain = strdup(lwc_string_data(nsurl_get_component(url, NSURL_HOST)));
    } else {
        domain = strdup(messages_get("UnknownHost"));
    }
    if (domain == NULL) {
        win32_warning(messages_get("NoMemory"), 0);
        free(filename);
        free(w);
        return NULL;
    }

    bool unknown_size = (w->size == 0);
    const char *size_str = unknown_size ? messages_get("UnknownSize") : human_friendly_bytesize((int)w->size);
    char *total_size_str = strdup(size_str);
    if (total_size_str == NULL) {
        win32_warning(messages_get("NoMemory"), 0);
        free(domain);
        free(filename);
        free(w);
        return NULL;
    }

    const char *download_dir = nsoption_charp(downloads_directory);
    char dest_dir[MAX_PATH];
    if (download_dir != NULL && download_dir[0] != '\0') {
        snprintf(dest_dir, sizeof(dest_dir), "%s", download_dir);
    } else if (FAILED(SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT, dest_dir))) {
        snprintf(dest_dir, sizeof(dest_dir), ".");
    }

    size_t dir_len = strlen(dest_dir);
    char destination[PATH_MAX];
    if (dir_len > 0 && (dest_dir[dir_len - 1] == '\\' || dest_dir[dir_len - 1] == '/')) {
        snprintf(destination, sizeof(destination), "%s%s", dest_dir, filename);
    } else {
        snprintf(destination, sizeof(destination), "%s\\%s", dest_dir, filename);
    }

    NSLOG(wisp, INFO, "download %s [%s] from %s to %s", filename, total_size_str, domain, destination);

    w->title = filename;
    w->domain = domain;
    w->total_size_str = total_size_str;
    w->filename = strdup(destination);
    if (w->filename == NULL) {
        win32_warning(messages_get("NoMemory"), 0);
        free(w->title);
        free(w->domain);
        free(w->total_size_str);
        free(w);
        return NULL;
    }

    w->downloaded = 0;
    gettimeofday(&(w->start_time), NULL);
    w->time_remaining = -1;
    w->status = DOWNLOAD_WORKING;
    w->progress = 0;

    w->file = fopen(destination, "wb");
    if (w->file == NULL) {
        win32_warning(messages_get("FileOpenWriteError"), destination);
        free(w->title);
        free(w->domain);
        free(w->total_size_str);
        free(w->filename);
        free(w);
        return NULL;
    }

    if (!nsws_download_window_up(w)) {
        win32_warning(messages_get("NoMemory"), 0);
        fclose(w->file);
        w->file = NULL;
        free(w->title);
        free(w->domain);
        free(w->total_size_str);
        free(w->filename);
        free(w);
        return NULL;
    }

    return w;
}

static nserror gui_download_window_data(struct gui_download_window *w, const uint8_t *data, unsigned int size)
{
    if (w == NULL || w->file == NULL || w->status != DOWNLOAD_WORKING)
        return NSERROR_SAVE_FAILED;

    size_t res = fwrite(data, 1, size, w->file);
    if (res != size) {
        NSLOG(wisp, INFO, "file write error %" PRIsizet " of %u", size - res, size);
    }
    w->downloaded += res;

    if (w->size > 0) {
        w->progress = (unsigned int)(((unsigned long long)w->downloaded * 10000) / w->size);
        if (w->progress > 10000)
            w->progress = 10000;
    } else {
        w->progress = 0;
    }

    struct timeval val;
    gettimeofday(&val, NULL);
    long elapsed = val.tv_sec - w->start_time.tv_sec;
    if (elapsed > 0 && w->downloaded > 0) {
        if (w->size > w->downloaded) {
            unsigned long long remaining_bytes = w->size - w->downloaded;
            w->time_remaining = (int)((remaining_bytes * (unsigned long long)elapsed) / w->downloaded);
        } else {
            w->time_remaining = 0;
        }
    } else {
        w->time_remaining = -1;
    }

    return NSERROR_OK;
}

static void gui_download_window_error(struct gui_download_window *w, const char *error_msg)
{
    NSLOG(wisp, INFO, "error %s", error_msg);
    if (w == NULL)
        return;

    w->status = DOWNLOAD_ERROR;
    nsws_download_update_label(w);
    nsws_download_update_progress(w);
}

static void gui_download_window_done(struct gui_download_window *w)
{
    if (w == NULL)
        return;

    if (w->file != NULL) {
        fclose(w->file);
        w->file = NULL;
    }

    w->status = DOWNLOAD_COMPLETE;
    w->progress = 10000;
    w->time_remaining = 0;
    nsws_download_update_label(w);
    nsws_download_update_progress(w);
}

static struct gui_download_table download_table = {
    .create = gui_download_window_create,
    .data = gui_download_window_data,
    .error = gui_download_window_error,
    .done = gui_download_window_done,
};

struct gui_download_table *win32_download_table = &download_table;
