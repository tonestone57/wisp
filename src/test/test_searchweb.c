/*
 * Copyright 2026 Wisp Contributors
 *
 * This file is part of Wisp.
 *
 * Wisp is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

/**
 * \file
 * Unit tests and benchmark suite for searchweb functions in src/desktop/searchweb.c.
 */

#include <check.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <wisp/bitmap.h>
#include <wisp/content/backing_store.h>
#include <wisp/content/hlcache.h>
#include <wisp/content/llcache.h>
#include <wisp/desktop/gui_table.h>
#include <wisp/desktop/searchweb.h>
#include <wisp/fetch.h>
#include <wisp/misc.h>
#include <wisp/utils/corestrings.h>
#include <wisp/utils/errors.h>
#include <wisp/utils/messages.h>
#include <wisp/utils/nsoption.h>
#include <wisp/utils/nsurl.h>

#include "content/fetchers.h"
#include "content/urldb.h"

static char last_provider_updated[128] = {0};

static nserror mock_provider_update(const char *name, struct bitmap *bitmap)
{
    (void)bitmap;
    if (name) {
        strncpy(last_provider_updated, name, sizeof(last_provider_updated) - 1);
        last_provider_updated[sizeof(last_provider_updated) - 1] = '\0';
    }
    return NSERROR_OK;
}

static struct gui_search_web_table mock_search_web_table = {
    .provider_update = mock_provider_update,
};

static nserror mock_schedule(int t, void (*callback)(void *p), void *p)
{
    (void)t;
    (void)callback;
    (void)p;
    return NSERROR_OK;
}

static nserror mock_fetch_get_resource_data(const char *path, const uint8_t **data, size_t *data_len)
{
    static const uint8_t dummy_ico_data[] = { 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 32, 0, 0, 0 };
    if (data) *data = dummy_ico_data;
    if (data_len) *data_len = sizeof(dummy_ico_data);
    return NSERROR_OK;
}

static struct gui_fetch_table mock_fetch_table = {
    .get_resource_data = mock_fetch_get_resource_data,
};

static struct gui_misc_table mock_misc_table = {
    .schedule = mock_schedule,
};

static struct wisp_table mock_guit = {
    .search_web = &mock_search_web_table,
    .misc = &mock_misc_table,
    .fetch = &mock_fetch_table,
};

extern struct wisp_table *guit;
extern bool fetch_use_ipc;

static void setup_env(void)
{
    guit = &mock_guit;
    guit->llcache = filesystem_llcache_table;
    ck_assert_int_eq(corestrings_init(), NSERROR_OK);
    ck_assert_int_eq(nsoption_init(NULL, NULL, NULL), NSERROR_OK);
    ck_assert_int_eq(messages_add_from_file("src/test/data/Messages"), NSERROR_OK);
    urldb_init();

    struct hlcache_parameters hlcache_params = {
        .bg_clean_time = 10,
    };
    hlcache_params.llcache.store.path = "/tmp";

    ck_assert_int_eq(hlcache_initialise(&hlcache_params), NSERROR_OK);
    fetch_use_ipc = false;
    ck_assert_int_eq(fetcher_init(), NSERROR_OK);
}

static void teardown_env(void)
{
    fetcher_quit();
    hlcache_finalise();
    urldb_destroy();
    nsoption_finalise(NULL, NULL);
    corestrings_fini();
}

START_TEST(test_search_web_init_and_iterate)
{
    setup_env();

    /* Test initialization with default providers */
    ck_assert_int_eq(search_web_init(NULL), NSERROR_OK);

    /* Test provider iteration */
    const char *name = NULL;
    ssize_t iter = search_web_iterate_providers(-1, &name);
    ck_assert_int_ge(iter, 0);
    ck_assert_ptr_nonnull(name);
    ck_assert_str_eq(name, "DuckDuckGo");

    /* End of iteration */
    iter = search_web_iterate_providers(iter, &name);
    ck_assert_int_eq(iter, -1);

    /* Test select provider */
    ck_assert_int_eq(search_web_select_provider("DuckDuckGo"), NSERROR_OK);
    ck_assert_str_eq(last_provider_updated, "DuckDuckGo");

    /* Test bitmap retrieval */
    struct bitmap *bmp = NULL;
    ck_assert_int_eq(search_web_get_provider_bitmap(&bmp), NSERROR_OK);

    ck_assert_int_eq(search_web_finalise(), NSERROR_OK);

    teardown_env();
}
END_TEST

START_TEST(test_search_web_omni)
{
    setup_env();

    ck_assert_int_eq(search_web_init(NULL), NSERROR_OK);

    /* Test valid URL term */
    struct nsurl *url = NULL;
    nserror err = search_web_omni("https://www.example.com/", SEARCH_WEB_OMNI_NONE, &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(url);
    ck_assert_str_eq(nsurl_access(url), "https://www.example.com/");
    nsurl_unref(url);

    /* Test search query term */
    url = NULL;
    err = search_web_omni("wisp browser fast performance", SEARCH_WEB_OMNI_SEARCHONLY, &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(url);
    ck_assert_str_eq(nsurl_access(url), "https://www.duckduckgo.com/html/?q=wisp+browser+fast+performance");
    nsurl_unref(url);

    ck_assert_int_eq(search_web_finalise(), NSERROR_OK);

    teardown_env();
}
END_TEST

START_TEST(test_search_web_benchmark)
{
    setup_env();

    ck_assert_int_eq(search_web_init(NULL), NSERROR_OK);

    const char *benchmark_terms[] = {
        "wisp browser fast performance optimization",
        "caching strlen outside of loop body",
        "cURL DNS prefetch preconnect async thread pool",
        "BBMQ dual-stage ring-buffer auto-scaling architecture"
    };
    const size_t num_terms = sizeof(benchmark_terms) / sizeof(benchmark_terms[0]);
    const int iterations = 100000;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < iterations; i++) {
        const char *term = benchmark_terms[i % num_terms];
        struct nsurl *url = NULL;
        nserror err = search_web_omni(term, SEARCH_WEB_OMNI_SEARCHONLY, &url);
        ck_assert_int_eq(err, NSERROR_OK);
        ck_assert_ptr_nonnull(url);
        nsurl_unref(url);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("\n[BENCHMARK] search_web_omni (%d iterations): %.4f seconds (%.2f ns/op)\n",
           iterations, elapsed, (elapsed / iterations) * 1e9);

    ck_assert_int_eq(search_web_finalise(), NSERROR_OK);

    teardown_env();
}
END_TEST

static Suite *searchweb_suite(void)
{
    Suite *s = suite_create("searchweb");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_search_web_init_and_iterate);
    tcase_add_test(tc_core, test_search_web_omni);
    tcase_add_test(tc_core, test_search_web_benchmark);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;

    Suite *s = searchweb_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
