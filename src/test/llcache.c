/*
 * Copyright 2011 John Mark Bell <jmb@netsurf-browser.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/corestrings.h"
#include "utils/messages.h"
#include "utils/nsurl.h"
#include "utils/ring.h"
#include "utils/url.h"
#include "utils/utils.h"
#include "utils/file.h"
#include "utils/nsoption.h"
#include "desktop/gui_table.h"
#include "desktop/gui_internal.h"
#include "wisp/misc.h"
#include "wisp/content/backing_store.h"
#include "content/fetch.h"
#include "content/fetchers.h"
#include "content/llcache.h"

/******************************************************************************
 * Things that we'd reasonably expect to have to implement                    *
 ******************************************************************************/

/* utils/utils.h */
char *filename_from_path(char *path)
{
    char *leafname;

    leafname = strrchr(path, '/');
    if (!leafname)
        leafname = path;
    else
        leafname += 1;

    return strdup(leafname);
}

typedef struct schedule_entry {
    int time;
    void (*cb)(void *p);
    void *pw;
    struct schedule_entry *next;
} schedule_entry;

static schedule_entry *schedule_list = NULL;

static nserror mock_schedule(int t, void (*cb)(void *p), void *pw)
{
    if (t < 0) {
        schedule_entry **curr = &schedule_list;
        while (*curr != NULL) {
            if ((*curr)->cb == cb && (*curr)->pw == pw) {
                schedule_entry *tmp = *curr;
                *curr = tmp->next;
                free(tmp);
            } else {
                curr = &(*curr)->next;
            }
        }
    } else {
        schedule_entry *entry = malloc(sizeof(schedule_entry));
        if (entry != NULL) {
            entry->time = t;
            entry->cb = cb;
            entry->pw = pw;
            entry->next = schedule_list;
            schedule_list = entry;
        }
    }
    return NSERROR_OK;
}

static void pump_scheduled(void)
{
    int safety = 0;
    while (schedule_list != NULL && safety++ < 100) {
        schedule_entry *entry = schedule_list;
        schedule_list = entry->next;
        void (*cb)(void *p) = entry->cb;
        void *pw = entry->pw;
        free(entry);
        cb(pw);
    }
}

static struct gui_misc_table mock_misc = {
    .schedule = mock_schedule,
};

extern struct wisp_table *guit;

/* content/fetch.h */
const char *fetch_filetype(const char *unix_path)
{
    return NULL;
}

/* content/fetch.h */
char *fetch_mimetype(const char *ro_path)
{
    return NULL;
}

/* utils/url.h */
char *path_to_url(const char *path)
{
    int urllen = strlen(path) + FILE_SCHEME_PREFIX_LEN + 1;
    char *url = malloc(urllen);

    if (url == NULL) {
        return NULL;
    }

    if (*path == '/') {
        path++; /* file: paths are already absolute */
    }

    snprintf(url, urllen, "%s%s", FILE_SCHEME_PREFIX, path);

    return url;
}

/* utils/url.h */
char *url_to_path(const char *url)
{
    char *url_path;
    char *path = NULL;

    if (url_unescape(url, 0, NULL, &url_path) == NSERROR_OK) {
        /* return the absolute path including leading / */
        path = strdup(url_path + (FILE_SCHEME_PREFIX_LEN - 1));
        free(url_path);
    }

    return path;
}

/******************************************************************************
 * Things that are absolutely not reasonable, and should disappear            *
 ******************************************************************************/

#include "desktop/cookie_manager.h"

/* desktop/cookie_manager.h -- used by urldb
 *
 * URLdb should have a cookies update event + handler registration
 */
bool cookie_manager_add(const struct cookie_data *data)
{
    return true;
}

/* desktop/cookie_manager.h -- used by urldb
 *
 * URLdb should have a cookies removal handler registration
 */
void cookie_manager_remove(const struct cookie_data *data)
{
}


/* content/fetchers/fetch_file.h -- used by fetcher core
 *
 * Simpler to stub this than haul in all the file fetcher's dependencies
 */
void fetch_file_register(void)
{
}


/******************************************************************************
 * test: protocol handler                                                     *
 ******************************************************************************/

typedef struct test_context {
    struct fetch *parent;

    bool aborted;
    bool locked;

    struct test_context *r_prev;
    struct test_context *r_next;
} test_context;

static test_context *ring;

bool test_initialise(lwc_string *scheme)
{
    return true;
}

bool test_can_fetch(const nsurl *url)
{
    return true;
}

void test_finalise(lwc_string *scheme)
{
}

nserror test_setup_fetch(struct fetch *parent, nsurl *url, bool only_2xx, bool downgrade_tls,
    const struct fetch_postdata *postdata, const char **headers, void **handle_out)
{
    test_context *ctx = calloc(1, sizeof(test_context));

    if (ctx == NULL)
        return NSERROR_NOMEM;

    ctx->parent = parent;

    RING_INSERT(ring, ctx);

    *handle_out = ctx;
    return NSERROR_OK;
}

bool test_start_fetch(void *handle)
{
    return true;
}

void test_abort_fetch(void *handle)
{
    test_context *ctx = handle;

    ctx->aborted = true;
}

void test_free_fetch(void *handle)
{
    test_context *ctx = handle;

    RING_REMOVE(ring, ctx);

    free(ctx);
}

void test_process(test_context *ctx)
{
    fetch_msg msg;

    fetch_set_http_code(ctx->parent, 200);

    msg.type = FETCH_HEADER;
    msg.data.header_or_data.buf = (const uint8_t *)"Content-Type: text/html\r\n";
    msg.data.header_or_data.len = strlen("Content-Type: text/html\r\n");
    fetch_send_callback(&msg, ctx->parent);

    msg.type = FETCH_HEADER;
    msg.data.header_or_data.buf = (const uint8_t *)"Cache-Control: max-age=3600\r\n";
    msg.data.header_or_data.len = strlen("Cache-Control: max-age=3600\r\n");
    fetch_send_callback(&msg, ctx->parent);

    msg.type = FETCH_DATA;
    msg.data.header_or_data.buf = (const uint8_t *)"<html><body>Test</body></html>";
    msg.data.header_or_data.len = strlen("<html><body>Test</body></html>");
    fetch_send_callback(&msg, ctx->parent);

    msg.type = FETCH_FINISHED;
    fetch_send_callback(&msg, ctx->parent);
}

static void pump_all(void)
{
    for (int i = 0; i < 20; i++) {
        fetch_poll_all();
        pump_scheduled();
    }
}

void test_poll(lwc_string *scheme)
{
    test_context *ctx, *next;

    if (ring == NULL)
        return;

    ctx = ring;
    do {
        next = ctx->r_next;

        if (ctx->locked)
            continue;

        if (ctx->aborted == false) {
            test_process(ctx);
        }

        fetch_remove_from_queues(ctx->parent);
        fetch_free(ctx->parent);
    } while ((ctx = next) != ring && ring != NULL);
}

/******************************************************************************
 * The actual test code                                                       *
 ******************************************************************************/

nserror event_handler(llcache_handle *handle, const llcache_event *event, void *pw)
{
    static char *event_names[] = {"GOT_CERTS", "HAD_HEADERS", "HAD_DATA", "DONE", "ERROR", "PROGRESS", "REDIRECT"};
    bool *done = pw;

    if (event->type != LLCACHE_EVENT_PROGRESS)
        fprintf(stdout, "%p : event type %d (%s)\n", handle, event->type, event_names[event->type]);

    /* Inform main() that the fetch completed */
    if (event->type == LLCACHE_EVENT_DONE)
        *done = true;

    return NSERROR_OK;
}

static nserror redirect_abort_event_handler(llcache_handle *handle, const llcache_event *event, void *pw)
{
    bool *redirect_aborted = pw;
    if (event->type == LLCACHE_EVENT_REDIRECT) {
        *redirect_aborted = true;
        /* Simulate concurrent handle abort during redirect event dispatch */
        llcache_handle_abort(handle);
    }
    return NSERROR_OK;
}

int main(int argc, char **argv)
{
    nserror error;
    llcache_handle *handle;
    llcache_handle *handle2;
    nsurl *url;
    bool done = false;

    guit = calloc(1, sizeof(struct wisp_table));
    if (guit == NULL) return 1;

    guit->misc = &mock_misc;
    guit->llcache = filesystem_llcache_table;
    guit->file = default_file_table;

    if (nsoption_init(NULL, NULL, NULL) != NSERROR_OK) {
        fprintf(stderr, "Failed to initialize nsoption\n");
        return 1;
    }
    nsoption_set_int(max_fetchers, 10);
    nsoption_set_int(max_fetchers_per_host, 5);

    if (corestrings_init() != NSERROR_OK) {
        fprintf(stderr, "Failed to initialize corestrings\n");
        return 1;
    }

    messages_add_key_value("BadRedirect", "Bad redirect");
    messages_add_key_value("BadAuth", "Bad authentication");
    messages_add_key_value("SSLError", "SSL error");
    messages_add_key_value("MiscError", "Miscellaneous error");

    extern bool fetch_use_ipc;
    fetch_use_ipc = false;

    struct fetcher_operation_table fetcher_ops = {
        .initialise = test_initialise,
        .acceptable = test_can_fetch,
        .setup = test_setup_fetch,
        .start = test_start_fetch,
        .abort = test_abort_fetch,
        .free = test_free_fetch,
        .poll = test_poll,
        .finalise = test_finalise
    };

    fetcher_add(lwc_string_ref(corestring_lwc_https), &fetcher_ops);
    fetcher_add(lwc_string_ref(corestring_lwc_http), &fetcher_ops);

    /* Initialise subsystems */
    fetcher_init();

    struct llcache_parameters llcache_params = {
        .limit = 1024 * 1024,
    };

    /* Initialise low-level cache */
    error = llcache_initialise(&llcache_params);
    if (error != NSERROR_OK) {
        fprintf(stderr, "llcache_initialise: %d\n", error);
        return 1;
    }

    /* Test header parsing and parameter extraction */
    {
        nsurl *turl;
        llcache_handle *th;
        bool tdone = false;

        if (nsurl_create("wisp-inline://test-header", &turl) == NSERROR_OK) {
            error = llcache_handle_retrieve_buffer(turl, (const uint8_t *)"test", 4,
                "text/html; charset=utf-8; foo=\"bar baz\"", event_handler, &tdone, &th);
            if (error == NSERROR_OK) {
                const llcache_header_value *hdr = llcache_handle_get_header(th, LLCACHE_HEADER_CONTENT_TYPE);
                if (hdr != NULL && hdr->count == 1 &&
                    strcmp(hdr->entries[0].value, "text/html") == 0 &&
                    hdr->entries[0].num_params == 2 &&
                    strcmp(hdr->entries[0].params[0].key, "charset") == 0 &&
                    strcmp(hdr->entries[0].params[0].value, "utf-8") == 0 &&
                    strcmp(hdr->entries[0].params[1].key, "foo") == 0 &&
                    strcmp(hdr->entries[0].params[1].value, "bar baz") == 0) {
                    fprintf(stdout, "llcache_handle_get_header test PASSED\n");
                } else {
                    fprintf(stderr, "llcache_handle_get_header test FAILED\n");
                    return 1;
                }

                if (llcache_handle_get_header(th, LLCACHE_HEADER_CONTENT_DISPOSITION) != NULL) {
                    fprintf(stderr, "llcache_handle_get_header missing header test FAILED\n");
                    return 1;
                }

                llcache_handle_release(th);
            }
            nsurl_unref(turl);
        }

        /* Test format specifier security in MIME type string */
        if (nsurl_create("wisp-inline://test-format-string-security", &turl) == NSERROR_OK) {
            error = llcache_handle_retrieve_buffer(turl, (const uint8_t *)"test", 4,
                "text/html; charset=%s%x%d%p%n", event_handler, &tdone, &th);
            if (error == NSERROR_OK) {
                const llcache_header_value *hdr = llcache_handle_get_header(th, LLCACHE_HEADER_CONTENT_TYPE);
                if (hdr != NULL && hdr->count == 1 &&
                    strcmp(hdr->entries[0].value, "text/html") == 0 &&
                    hdr->entries[0].num_params == 1 &&
                    strcmp(hdr->entries[0].params[0].key, "charset") == 0 &&
                    strcmp(hdr->entries[0].params[0].value, "%s%x%d%p%n") == 0) {
                    fprintf(stdout, "llcache format specifier security test PASSED\n");
                } else {
                    fprintf(stderr, "llcache format specifier security test FAILED\n");
                    return 1;
                }
                llcache_handle_release(th);
            }
            nsurl_unref(turl);
        }
    }

    if (nsurl_create("https://www.wispbrowser.com", &url) != NSERROR_OK) {
        fprintf(stderr, "Failed creating url\n");
        return 1;
    }

    /* Retrieve an URL from the low-level cache (may trigger fetch) */
    error = llcache_handle_retrieve(url, 0, NULL, NULL, event_handler, &done, &handle);
    if (error != NSERROR_OK) {
        fprintf(stderr, "llcache_handle_retrieve: %d\n", error);
        return 1;
    }

    /* Poll relevant components */
    pump_all();

    if (!done) {
        fprintf(stderr, "First retrieve timed out\n");
        return 1;
    }

    done = false;
    error = llcache_handle_retrieve(url, 0, NULL, NULL, event_handler, &done, &handle2);
    if (error != NSERROR_OK) {
        fprintf(stderr, "llcache_handle_retrieve: %d\n", error);
        return 1;
    }

    pump_all();

    if (!done) {
        fprintf(stderr, "Second retrieve timed out\n");
        return 1;
    }

    bool same = llcache_handle_references_same_object(handle, handle2);
    fprintf(stdout, "%p, %p -> %d\n", handle, handle2, same);

    if (!same) {
        fprintf(stderr, "Expected handle and handle2 to reference the same cached object!\n");
        return 1;
    }

    /* Test concurrent llcache_handle_abort during redirect event */
    {
        nsurl *redirect_src_url;
        llcache_handle *redirect_handle;
        bool redirect_aborted = false;

        if (nsurl_create("https://www.wispbrowser.com/redirect-test", &redirect_src_url) == NSERROR_OK) {
            error = llcache_handle_retrieve(redirect_src_url, 0, NULL, NULL, redirect_abort_event_handler, &redirect_aborted, &redirect_handle);
            if (error == NSERROR_OK) {
                /* Simulate a 302 redirect from test fetcher */
                if (ring != NULL) {
                    fetch_msg rmsg;
                    fetch_set_http_code(ring->parent, 302);
                    nsurl *target_url;
                    if (nsurl_create("https://www.wispbrowser.com/redirect-target", &target_url) == NSERROR_OK) {
                        rmsg.type = FETCH_REDIRECT;
                        rmsg.data.redirect = target_url;
                        fetch_send_callback(&rmsg, ring->parent);
                        nsurl_unref(target_url);
                    }
                }
                pump_all();
                if (redirect_aborted) {
                    fprintf(stdout, "llcache_fetch_redirect abort safety test PASSED\n");
                }
                llcache_handle_release(redirect_handle);
            }
            nsurl_unref(redirect_src_url);
        }
    }

    /* Cleanup */
    llcache_handle_release(handle2);
    llcache_handle_release(handle);
    nsurl_unref(url);

    llcache_finalise();
    fetcher_quit();
    messages_destroy();
    corestrings_fini();
    nsoption_finalise(NULL, NULL);
    free(guit);

    return 0;
}
