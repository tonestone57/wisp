/*
 * Copyright 2012 Vincent Sanders <vince@kyllikki.org>
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

/** \file
 * implementation for javascript scheme fetcher
 *
 * This fetcher implements
 * http://www.whatwg.org/specs/web-apps/current-work/multipage/browsers.html#javascript-protocol
 */

#include <libwapcaplet/libwapcaplet.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <wisp/content/fetch.h>
#include <wisp/ns_inttypes.h>
#include <wisp/utils/corestrings.h>
#include <wisp/utils/log.h>
#include <wisp/utils/nsurl.h>
#include <wisp/utils/utils.h>
#include "utils/ring.h"
#include "utils/url.h"
#include "content/fetchers.h"

#include "content/handlers/javascript/fetcher.h"

/** Context for an resource fetch */
struct fetch_javascript_context {
    struct fetch_javascript_context *r_next, *r_prev;

    struct fetch *fetchh; /**< Handle for this fetch */

    bool aborted; /**< Flag indicating fetch has been aborted */
    bool locked; /**< Flag indicating entry is already entered */

    nsurl *url; /**< The URL being fetched */
};

static struct fetch_javascript_context *ring = NULL;


/** issue fetch callbacks with locking */
static inline bool fetch_javascript_send_callback(const fetch_msg *msg, struct fetch_javascript_context *ctx)
{
    ctx->locked = true;
    fetch_send_callback(msg, ctx->fetchh);
    ctx->locked = false;

    return ctx->aborted;
}

static bool fetch_javascript_send_header(struct fetch_javascript_context *ctx, const char *fmt, ...)
{
    fetch_msg msg;
    char header[256];
    va_list ap;
    int len;

    va_start(ap, fmt);
    len = vsnprintf(header, sizeof(header), fmt, ap);
    va_end(ap);

    if (len >= (int)sizeof(header) || len < 0) {
        return true; /* Treat format overflow as error/abort */
    }

    msg.type = FETCH_HEADER;
    msg.data.header_or_data.buf = (const uint8_t *)header;
    msg.data.header_or_data.len = len;

    return fetch_javascript_send_callback(&msg, ctx);
}

/**
 * called from poll to progress fetch.
 */
static bool fetch_javascript_handler(struct fetch_javascript_context *ctx)
{
    fetch_msg msg;
    nserror res;
    const char *url_str;
    const char *script_code;
    char *unescaped = NULL;
    size_t unescaped_len = 0;

    url_str = nsurl_access(ctx->url);
    if (url_str != NULL && strncasecmp(url_str, "javascript:", SLEN("javascript:")) == 0) {
        script_code = url_str + SLEN("javascript:");
    } else {
        script_code = "";
    }

    res = url_unescape(script_code, 0, &unescaped_len, &unescaped);
    if (res != NSERROR_OK || unescaped == NULL) {
        msg.type = FETCH_ERROR;
        msg.data.error = "Unable to URL decode javascript: URL";
        fetch_javascript_send_callback(&msg, ctx);
        return false;
    }

    fetch_set_http_code(ctx->fetchh, 200);

    if (fetch_javascript_send_header(ctx, "Content-Type: text/html; charset=utf-8")) {
        free(unescaped);
        return true;
    }

    if (fetch_javascript_send_header(ctx, "Content-Length: %" PRIsizet, unescaped_len)) {
        free(unescaped);
        return true;
    }

    if (fetch_javascript_send_header(ctx, "Cache-Control: no-cache")) {
        free(unescaped);
        return true;
    }

    if (ctx->aborted == false) {
        msg.type = FETCH_DATA;
        msg.data.header_or_data.buf = (const uint8_t *)unescaped;
        msg.data.header_or_data.len = unescaped_len;
        fetch_javascript_send_callback(&msg, ctx);
    }

    if (ctx->aborted == false) {
        msg.type = FETCH_FINISHED;
        fetch_javascript_send_callback(&msg, ctx);
    }

    free(unescaped);
    return true;
}


/** callback to initialise the resource fetcher. */
static bool fetch_javascript_initialise(lwc_string *scheme)
{
    return true;
}

/** callback to finalise the resource fetcher. */
static void fetch_javascript_finalise(lwc_string *scheme)
{
}

static bool fetch_javascript_can_fetch(const nsurl *url)
{
    return true;
}

/** callback to set up a resource fetch context. */
static nserror fetch_javascript_setup(struct fetch *fetchh, nsurl *url, bool only_2xx, bool downgrade_tls,
    const struct fetch_postdata *postdata, const char **headers, void **handle_out)
{
    struct fetch_javascript_context *ctx;

    ctx = calloc(1, sizeof(*ctx));
    if (ctx == NULL)
        return NSERROR_NOMEM;

    ctx->url = nsurl_ref(url);

    ctx->fetchh = fetchh;

    RING_INSERT(ring, ctx);

    *handle_out = ctx;
    return NSERROR_OK;
}

/** callback to free a resource fetch */
static void fetch_javascript_free(void *ctx)
{
    struct fetch_javascript_context *c = ctx;
    if (c == NULL) return;
    if (c->url != NULL) {
        nsurl_unref(c->url);
    }
    if (c->r_next != NULL || c->r_prev != NULL || ring == c) {
        RING_REMOVE(ring, c);
    }
    free(ctx);
}

/** callback to start a resource fetch */
static bool fetch_javascript_start(void *ctx)
{
    return true;
}

/** callback to abort a resource fetch */
static void fetch_javascript_abort(void *ctx)
{
    struct fetch_javascript_context *c = ctx;

    /* To avoid the poll loop having to deal with the fetch context
     * disappearing from under it, we simply flag the abort here.
     * The poll loop itself will perform the appropriate cleanup.
     */
    c->aborted = true;
}


/** callback to poll for additional resource fetch contents */
static void fetch_javascript_poll(lwc_string *scheme)
{
    struct fetch_javascript_context *c, *save_ring = NULL;

    /* Iterate over ring, processing each pending fetch */
    while (ring != NULL) {
        /* Take the first entry from the ring */
        c = ring;
        RING_REMOVE(ring, c);

        /* Ignore fetches that have been flagged as locked.
         * This allows safe re-entrant calls to this function.
         * Re-entrancy can occur if, as a result of a callback,
         * the interested party causes fetch_poll() to be called
         * again.
         */
        if (c->locked == true) {
            RING_INSERT(save_ring, c);
            continue;
        }

        /* Only process non-aborted fetches */
        if (c->aborted == false) {
            /* resource fetches can be processed in one go */
            fetch_javascript_handler(c);
        }

        /* And now finish */
        fetch_remove_from_queues(c->fetchh);
        fetch_free(c->fetchh);
    }

    /* Finally, if we saved any fetches which were locked, put them back
     * into the ring for next time
     */
    ring = save_ring;
}

/**
 * Register javascript scheme fetcher with fetcher factory.
 *
 * \return NSERROR_OK on success or appropriate error code on faliure.
 */
nserror fetch_javascript_register(void)
{
    lwc_string *scheme = lwc_string_ref(corestring_lwc_javascript);
    const struct fetcher_operation_table fetcher_ops = {.initialise = fetch_javascript_initialise,
        .acceptable = fetch_javascript_can_fetch,
        .setup = fetch_javascript_setup,
        .start = fetch_javascript_start,
        .abort = fetch_javascript_abort,
        .free = fetch_javascript_free,
        .poll = fetch_javascript_poll,
        .finalise = fetch_javascript_finalise};

    return fetcher_add(scheme, &fetcher_ops);
}
