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
#include "utils/nsurl.h"
#include "utils/ring.h"
#include "utils/url.h"
#include "utils/utils.h"
#include "content/fetch.h"
#include "content/llcache.h"

/******************************************************************************
 * Things that we'd reasonably expect to have to implement                    *
 ******************************************************************************/

bool verbose_log;

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

#include <wisp/misc.h>
#include <wisp/desktop/gui_internal.h>
#include <wisp/content/backing_store.h>
#include <wisp/utils/nsoption.h>

struct mock_task {
    void (*callback)(void *p);
    void *param;
    struct mock_task *next;
};

static struct mock_task *mock_tasks = NULL;

static nserror mock_schedule(int t, void (*cb)(void *p), void *p)
{
    if (t < 0) {
        struct mock_task **prev = &mock_tasks;
        struct mock_task *curr = mock_tasks;
        while (curr) {
            if (curr->callback == cb && curr->param == p) {
                *prev = curr->next;
                free(curr);
                return NSERROR_OK;
            }
            prev = &curr->next;
            curr = curr->next;
        }
        return NSERROR_NOT_FOUND;
    }

    struct mock_task *task = malloc(sizeof(*task));
    task->callback = cb;
    task->param = p;
    task->next = NULL;

    if (mock_tasks == NULL) {
        mock_tasks = task;
    } else {
        struct mock_task *tail = mock_tasks;
        while (tail->next) tail = tail->next;
        tail->next = task;
    }

    return NSERROR_OK;
}

static void pump_scheduled(void)
{
    int safety = 0;
    while (mock_tasks != NULL && safety++ < 100) {
        struct mock_task *curr = mock_tasks;
        mock_tasks = curr->next;
        curr->callback(curr->param);
        free(curr);
    }
}

static struct gui_misc_table mock_misc = {
    .schedule = mock_schedule,
};

static struct wisp_table mock_gui_table = {
    .misc = &mock_misc,
    .llcache = NULL,
};

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
    /* Nothing to do */
    return true;
}

bool test_can_fetch(const nsurl *url)
{
    /* Nothing to do */
    return true;
}

void test_finalise(lwc_string *scheme)
{
    /* Nothing to do */
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
    /* Nothing to do */
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

    ctx->locked = true;

    fetch_set_http_code(ctx->parent, 200);

    msg.type = FETCH_HEADER;
    msg.data.header_or_data.buf = (const uint8_t *)"Content-Type: text/plain";
    msg.data.header_or_data.len = strlen("Content-Type: text/plain");
    fetch_send_callback(&msg, ctx->parent);

    msg.type = FETCH_HEADER;
    msg.data.header_or_data.buf = (const uint8_t *)"Cache-Control: max-age=3600";
    msg.data.header_or_data.len = strlen("Cache-Control: max-age=3600");
    fetch_send_callback(&msg, ctx->parent);

    if (ctx->aborted == false) {
        msg.type = FETCH_DATA;
        msg.data.header_or_data.buf = (const uint8_t *)"test data";
        msg.data.header_or_data.len = strlen("test data");
        fetch_send_callback(&msg, ctx->parent);
    }

    if (ctx->aborted == false) {
        msg.type = FETCH_FINISHED;
        fetch_send_callback(&msg, ctx->parent);
    }

    ctx->locked = false;
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

#include "content/fetchers.h"

nserror event_handler(llcache_handle *handle, const llcache_event *event, void *pw)
{
    static char *event_names[] = {
        "GOT_CERTS", "HAD_HEADERS", "HAD_DATA", "DONE", "ERROR", "PROGRESS", "REDIRECT"
    };
    bool *done = pw;

    if (event->type != LLCACHE_EVENT_PROGRESS)
        fprintf(stdout, "%p : %s\n", handle, event_names[event->type]);

    /* Inform main() that the fetch completed */
    if (event->type == LLCACHE_EVENT_DONE)
        *done = true;

    return NSERROR_OK;
}

int main(int argc, char **argv)
{
    nserror error;
    llcache_handle *handle;
    llcache_handle *handle2;
    lwc_string *scheme;
    nsurl *url;
    bool done = false;

    /* Initialise options & core strings */
    if (nsoption_init(NULL, NULL, NULL) != NSERROR_OK) {
        fprintf(stderr, "Failed to initialize nsoptions\n");
        return 1;
    }

    if (corestrings_init() != NSERROR_OK) {
        fprintf(stderr, "Failed to initialize corestrings\n");
        return 1;
    }

    /* Initialise subsystems */
    if (lwc_intern_string("http", SLEN("http"), &scheme) != lwc_error_ok) {
        fprintf(stderr, "Failed to intern \"http\"\n");
        return 1;
    }

    static const struct fetcher_operation_table test_fetcher_ops = {
        .initialise = test_initialise,
        .acceptable = test_can_fetch,
        .setup = test_setup_fetch,
        .start = test_start_fetch,
        .abort = test_abort_fetch,
        .free = test_free_fetch,
        .poll = test_poll,
        .finalise = test_finalise,
    };

    if (fetcher_add(scheme, &test_fetcher_ops) != NSERROR_OK) {
        fprintf(stderr, "Failed to add fetcher\n");
        return 1;
    }

    mock_gui_table.llcache = filesystem_llcache_table;
    guit = &mock_gui_table;

    /* Initialise low-level cache */
    struct llcache_parameters prm = {
        .limit = 1024 * 1024,
    };
    error = llcache_initialise(&prm);
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
    }

    if (nsurl_create("http://www.wispbrowser.com", &url) != NSERROR_OK) {
        fprintf(stderr, "Failed creating url\n");
        return 1;
    }

    /* Retrieve an URL from the low-level cache (may trigger fetch) */
    error = llcache_handle_retrieve(url, LLCACHE_RETRIEVE_VERIFIABLE, NULL, NULL, event_handler, &done, &handle);
    if (error != NSERROR_OK) {
        fprintf(stderr, "llcache_handle_retrieve: %d\n", error);
        return 1;
    }

    /* Poll relevant components */
    while (done == false) {
        pump_scheduled();
    }

    done = false;
    error = llcache_handle_retrieve(url, LLCACHE_RETRIEVE_VERIFIABLE, NULL, NULL, event_handler, &done, &handle2);
    if (error != NSERROR_OK) {
        fprintf(stderr, "llcache_handle_retrieve: %d\n", error);
        return 1;
    }

    while (done == false) {
        pump_scheduled();
    }

    fprintf(stdout, "%p, %p -> %d\n", handle, handle2, llcache_handle_references_same_object(handle, handle2));

    /* Cleanup */
    llcache_handle_release(handle2);
    llcache_handle_release(handle);

    return 0;
}
