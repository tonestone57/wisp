/*
 * Copyright 2026 Wisp Contributors
 *
 * This file is part of Wisp, http://www.netsurf-browser.org/
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wisp/utils/errors.h>
#include <wisp/utils/log.h>
#include <wisp/utils/nsurl.h>
#include "content/fetch.h"

static bool finished = false;

static void test_callback(const struct fetch_response *res, void *p)
{
    if (res == NULL) {
        printf("FAIL: Fetch pipeline returned error\n");
    } else {
        printf("PASS: Fetch pipeline returned response, code %ld\n", res->http_code);
        printf("Response body length: %zu\n", res->data_len);
    }
    finished = true;
}

int main(int argc, char **argv)
{
    nsurl *url;
    struct fetch_request req = {0};
    struct fetch *f = NULL;
    nserror res;

    res = nsurl_create("http://www.google.com", &url);
    if (res != NSERROR_OK) return 1;

    req.url = url;
    req.method = "POST";
    req.no_cache = true;

    printf("Starting fetch pipeline test...\n");
    res = fetch_pipeline_start(&req, test_callback, NULL, &f);
    if (res != NSERROR_OK) {
        printf("FAIL: fetch_pipeline_start failed with error %d\n", res);
        return 1;
    }

    if (f == NULL) {
        printf("FAIL: No fetch handle returned\n");
        return 1;
    }

    printf("fetch_pipeline_start called successfully, handle %p returned\n", f);

    nsurl_unref(url);
    return 0;
}
