/*
 * Copyright 2026 NetSurf Project
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#include <check.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <wisp/utils/errors.h>
#include <wisp/utils/nsurl.h>
#include <wisp/misc.h>
#include <wisp/desktop/searchweb.h>
#include <wisp/desktop/gui_internal.h>
#include <wisp/utils/corestrings.h>
#include <wisp/utils/nsoption.h>
#include <wisp/content/hlcache.h>
#include "content/fetchers.h"
#include "content/fetch.h"
#include "content/urldb.h"

extern struct gui_llcache_table *filesystem_llcache_table;
extern bool fetch_use_ipc;

static nserror mock_schedule(int t, void (*cb)(void *p), void *p)
{
    return NSERROR_OK;
}

static nserror mock_provider_update(const char *name, struct bitmap *bitmap)
{
    return NSERROR_OK;
}

static struct gui_search_web_table mock_search_web_table = {
    .provider_update = mock_provider_update,
};

static struct gui_misc_table mock_misc_table = {
    .schedule = mock_schedule,
};

static struct wisp_table mock_gui_table;

START_TEST(test_search_web_init_and_iterate)
{
    guit = &mock_gui_table;
    guit->misc = &mock_misc_table;
    guit->search_web = &mock_search_web_table;
    guit->llcache = filesystem_llcache_table;

    corestrings_init();
    urldb_init();
    nsoption_init(NULL, NULL, NULL);

    fetch_use_ipc = false;
    fetcher_init();

    struct hlcache_parameters hlcache_params = {
        .bg_clean_time = 0,
        .llcache = {
            .store = {
                .path = "/tmp"
            }
        }
    };
    hlcache_initialise(&hlcache_params);

    nserror err = search_web_init(NULL);
    ck_assert_int_eq(err, NSERROR_OK);

    const char *name = NULL;
    ssize_t iter = search_web_iterate_providers(-1, &name);
    ck_assert_int_ge(iter, 0);
    ck_assert_ptr_ne(name, NULL);
    ck_assert_str_eq(name, "DuckDuckGo");

    /* iterate further should end */
    iter = search_web_iterate_providers(iter, &name);
    ck_assert_int_eq(iter, -1);

    search_web_finalise();
    hlcache_finalise();
    fetcher_quit();
    urldb_destroy();
    nsoption_finalise(NULL, NULL);
    corestrings_fini();
}
END_TEST

START_TEST(test_search_web_select)
{
    guit = &mock_gui_table;
    guit->misc = &mock_misc_table;
    guit->search_web = &mock_search_web_table;
    guit->llcache = filesystem_llcache_table;

    corestrings_init();
    urldb_init();
    nsoption_init(NULL, NULL, NULL);

    fetch_use_ipc = false;
    fetcher_init();

    struct hlcache_parameters hlcache_params = {
        .bg_clean_time = 0,
        .llcache = {
            .store = {
                .path = "/tmp"
            }
        }
    };
    hlcache_initialise(&hlcache_params);

    nserror err = search_web_init(NULL);
    ck_assert_int_eq(err, NSERROR_OK);

    err = search_web_select_provider("DuckDuckGo");
    ck_assert_int_eq(err, NSERROR_OK);

    err = search_web_select_provider("NonExistentProvider");
    ck_assert_int_eq(err, NSERROR_OK);

    search_web_finalise();
    hlcache_finalise();
    fetcher_quit();
    urldb_destroy();
    nsoption_finalise(NULL, NULL);
    corestrings_fini();
}
END_TEST

START_TEST(test_search_web_omni)
{
    guit = &mock_gui_table;
    guit->misc = &mock_misc_table;
    guit->search_web = &mock_search_web_table;
    guit->llcache = filesystem_llcache_table;

    corestrings_init();
    urldb_init();
    nsoption_init(NULL, NULL, NULL);

    fetch_use_ipc = false;
    fetcher_init();

    struct hlcache_parameters hlcache_params = {
        .bg_clean_time = 0,
        .llcache = {
            .store = {
                .path = "/tmp"
            }
        }
    };
    hlcache_initialise(&hlcache_params);

    nserror err = search_web_init(NULL);
    ck_assert_int_eq(err, NSERROR_OK);

    nsurl *url = NULL;
    err = search_web_omni("c language performance %s benchmark", SEARCH_WEB_OMNI_SEARCHONLY, &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_ne(url, NULL);

    const char *url_str = nsurl_access(url);
    ck_assert_ptr_ne(url_str, NULL);
    ck_assert_ptr_ne(strstr(url_str, "duckduckgo.com"), NULL);
    ck_assert_ptr_ne(strstr(url_str, "c+language+performance"), NULL);

    nsurl_unref(url);

    search_web_finalise();
    hlcache_finalise();
    fetcher_quit();
    urldb_destroy();
    nsoption_finalise(NULL, NULL);
    corestrings_fini();
}
END_TEST

static Suite *searchweb_suite_create(void)
{
    Suite *s = suite_create("SearchWeb");
    TCase *tc = tcase_create("Core");

    tcase_add_test(tc, test_search_web_init_and_iterate);
    tcase_add_test(tc, test_search_web_select);
    tcase_add_test(tc, test_search_web_omni);

    suite_add_tcase(s, tc);
    return s;
}

int main(void)
{
    int number_failed;
    SRunner *sr = srunner_create(searchweb_suite_create());
    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
