/*
 * Copyright 2026 Wisp Browser Project
 *
 * This file is part of Wisp.
 *
 * Wisp is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <libwapcaplet/libwapcaplet.h>
#include "wisp/desktop/gui_internal.h"
#include "wisp/desktop/ipc_sandbox.h"
#include "wisp/desktop/ipc_messages.h"
#include "wisp/fetch.h"
#include "wisp/utils/log.h"
#include "wisp/utils/corestrings.h"
#include "content/fetch.h"
#include "content/fetchers.h"
#include "content/fetchers/broker.h"

struct broker_fetch_info {
    struct fetch *fetch_handle;
    nsurl *url;
    int fetch_id;
    struct broker_fetch_info *next;
};

static struct broker_fetch_info *active_fetches = NULL;
static int next_fetch_id = 1;

static bool fetch_broker_initialise(lwc_string *scheme)
{
    NSLOG(wisp, INFO, "Initialise Broker fetcher for %s", lwc_string_data(scheme));
    return true;
}

static void fetch_broker_finalise(lwc_string *scheme)
{
    NSLOG(wisp, INFO, "Finalise Broker fetcher for %s", lwc_string_data(scheme));
}

static bool fetch_broker_can_fetch(const nsurl *url)
{
    lwc_string *scheme = nsurl_get_component(url, NSURL_SCHEME);
    bool can = false;
    if (lwc_string_caseless_isequal(scheme, corestring_lwc_http, &can) == lwc_error_ok && can) goto out;
    if (lwc_string_caseless_isequal(scheme, corestring_lwc_https, &can) == lwc_error_ok && can) goto out;
out:
    lwc_string_unref(scheme);
    return can;
}

static void *fetch_broker_setup(struct fetch *parent_fetch, nsurl *url, bool only_2xx, bool downgrade_tls,
    const struct fetch_postdata *postdata, const char **headers)
{
    struct broker_fetch_info *f = malloc(sizeof(*f));
    if (!f) return NULL;
    f->fetch_handle = parent_fetch;
    f->url = nsurl_ref(url);
    f->fetch_id = next_fetch_id++;
    f->next = active_fetches;
    active_fetches = f;
    return f;
}

static bool fetch_broker_start(void *vf)
{
    struct broker_fetch_info *f = vf;
    NSLOG(wisp, INFO, "Starting brokered fetch for %s (id: %d)", nsurl_access(f->url), f->fetch_id);

    if (guit->ipc_sandbox) {
        const char *url_str = nsurl_access(f->url);
        guit->ipc_sandbox->post_ipc_message(guit->ipc_sandbox->ui_process_pid, WISP_MSG_FETCH_REQUEST, (uint32_t)f->fetch_id, url_str, strlen(url_str) + 1);
    }

    return true;
}

static void fetch_broker_abort(void *vf)
{
    struct broker_fetch_info *f = vf;
    fetch_free(f->fetch_handle);
}

static void fetch_broker_free(void *vf)
{
    struct broker_fetch_info *f = vf;
    nsurl_unref(f->url);
    free(f);
}

static void fetch_broker_poll(lwc_string *scheme)
{
}

nserror fetch_broker_register(void)
{
    static const struct fetcher_operation_table fetcher_ops = {
        fetch_broker_initialise,
        fetch_broker_can_fetch,
        fetch_broker_setup,
        fetch_broker_start,
        fetch_broker_abort,
        fetch_broker_free,
        fetch_broker_poll,
        NULL,
        fetch_broker_finalise
    };

    fetcher_add(lwc_string_ref(corestring_lwc_http), &fetcher_ops);
    fetcher_add(lwc_string_ref(corestring_lwc_https), &fetcher_ops);

    return NSERROR_OK;
}

static struct broker_fetch_info *find_fetch(int fetch_id)
{
    struct broker_fetch_info *f = active_fetches;
    while (f && f->fetch_id != fetch_id) f = f->next;
    return f;
}

void fetch_broker_deliver_header(int fetch_id, const uint8_t *data, size_t len)
{
    struct broker_fetch_info *f = find_fetch(fetch_id);
    if (f) {
        fetch_msg msg;
        msg.type = FETCH_HEADER;
        msg.data.header_or_data.buf = data;
        msg.data.header_or_data.len = len;
        fetch_send_callback(&msg, f->fetch_handle);
    }
}

void fetch_broker_deliver_data(int fetch_id, const uint8_t *data, size_t len)
{
    struct broker_fetch_info *f = find_fetch(fetch_id);
    if (f) {
        fetch_msg msg;
        msg.type = FETCH_DATA;
        msg.data.header_or_data.buf = data;
        msg.data.header_or_data.len = len;
        fetch_send_callback(&msg, f->fetch_handle);
    }
}

void fetch_broker_deliver_done(int fetch_id)
{
    struct broker_fetch_info *f = find_fetch(fetch_id);
    if (f) {
        fetch_msg msg;
        msg.type = FETCH_FINISHED;
        fetch_send_callback(&msg, f->fetch_handle);
    }
}

void fetch_broker_deliver_error(int fetch_id)
{
    struct broker_fetch_info *f = find_fetch(fetch_id);
    if (f) {
        fetch_msg msg;
        msg.type = FETCH_ERROR;
        msg.data.error = "BrokerFetchError";
        fetch_send_callback(&msg, f->fetch_handle);
    }
}
