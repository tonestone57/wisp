/*
 * Test suite for core web search facilities in src/desktop/searchweb.c
 */

#include <check.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <wisp/utils/errors.h>
#include <wisp/utils/corestrings.h>
#include <wisp/utils/nsurl.h>
#include <wisp/utils/file.h>
#include <wisp/utils/nsoption.h>
#include <wisp/fetch.h>
#include <wisp/misc.h>
#include <wisp/content/hlcache.h>
#include <wisp/content/backing_store.h>
#include <wisp/content/fetch.h>
#include <wisp/desktop/searchweb.h>
#include <wisp/desktop/gui_table.h>
#include "content/fetchers.h"
#include "content/urldb.h"

extern struct wisp_table *guit;
extern bool fetch_use_ipc;

static char last_provider_update[128];
static void (*scheduled_cb)(void *p) = NULL;
static void *scheduled_p = NULL;

static nserror mock_provider_update(const char *provider_name, struct bitmap *ico_bitmap)
{
    (void)ico_bitmap;
    if (provider_name != NULL) {
        strncpy(last_provider_update, provider_name, sizeof(last_provider_update) - 1);
        last_provider_update[sizeof(last_provider_update) - 1] = '\0';
    }
    return NSERROR_OK;
}

static nserror mock_schedule(int t, void (*cb)(void *p), void *p)
{
    if (t >= 0) {
        scheduled_cb = cb;
        scheduled_p = p;
    } else {
        if (scheduled_cb == cb) {
            scheduled_cb = NULL;
            scheduled_p = NULL;
        }
    }
    return NSERROR_OK;
}

static void pump_scheduled(void)
{
    int safety = 0;
    while (scheduled_cb != NULL && safety++ < 10) {
        void (*cb)(void *p) = scheduled_cb;
        void *p = scheduled_p;
        scheduled_cb = NULL;
        scheduled_p = NULL;
        cb(p);
    }
}

static nserror mock_get_resource_data(const char *path, const uint8_t **data, size_t *data_len)
{
    (void)path;
    static const uint8_t dummy_png[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    *data = dummy_png;
    *data_len = sizeof(dummy_png);
    return NSERROR_OK;
}

static void setup_mock_gui(void)
{
    static struct gui_search_web_table search_web_ops;
    search_web_ops.provider_update = mock_provider_update;

    static struct gui_misc_table misc_ops;
    misc_ops.schedule = mock_schedule;

    static struct gui_fetch_table fetch_ops;
    fetch_ops.get_resource_data = mock_get_resource_data;

    static struct wisp_table gui;
    memset(&gui, 0, sizeof(gui));
    gui.search_web = &search_web_ops;
    gui.misc = &misc_ops;
    gui.fetch = &fetch_ops;
    gui.llcache = filesystem_llcache_table;
    gui.file = default_file_table;

    guit = &gui;
    last_provider_update[0] = '\0';
    scheduled_cb = NULL;
    scheduled_p = NULL;

    ck_assert_int_eq(nsoption_init(NULL, NULL, NULL), NSERROR_OK);
    ck_assert_int_eq(corestrings_init(), NSERROR_OK);
    urldb_init();

    fetch_use_ipc = false;
    ck_assert_int_eq(fetcher_init(), NSERROR_OK);

    struct hlcache_parameters params = {
        .bg_clean_time = 10000,
        .llcache = {
            .limit = 1024 * 1024,
            .store = {
                .path = "/tmp",
            },
        },
    };
    ck_assert_int_eq(hlcache_initialise(&params), NSERROR_OK);
}

static void teardown_mock_gui(void)
{
    pump_scheduled();
    hlcache_finalise();
    fetcher_quit();
    urldb_destroy();
    corestrings_fini();
    nsoption_finalise(NULL, NULL);
    guit = NULL;
}

START_TEST(test_search_web_init_and_iterate)
{
    setup_mock_gui();

    nserror res = search_web_init(NULL);
    ck_assert_int_eq(res, NSERROR_OK);
    pump_scheduled();

    ssize_t iter = -1;
    const char *name = NULL;
    int count = 0;

    iter = search_web_iterate_providers(iter, &name);
    while (iter != -1) {
        ck_assert_ptr_nonnull(name);
        count++;
        iter = search_web_iterate_providers(iter, &name);
    }

    ck_assert_int_gt(count, 0);

    res = search_web_finalise();
    ck_assert_int_eq(res, NSERROR_OK);

    teardown_mock_gui();
}
END_TEST

START_TEST(test_search_web_omni_substitution)
{
    setup_mock_gui();

    nserror res = search_web_init(NULL);
    ck_assert_int_eq(res, NSERROR_OK);
    pump_scheduled();

    nsurl *url = NULL;
    res = search_web_omni("c++ performance", SEARCH_WEB_OMNI_SEARCHONLY, &url);
    ck_assert_int_eq(res, NSERROR_OK);
    ck_assert_ptr_nonnull(url);

    const char *url_str = nsurl_access(url);
    ck_assert_ptr_nonnull(url_str);
    ck_assert_ptr_nonnull(strstr(url_str, "c%2B%2B"));

    nsurl_unref(url);

    res = search_web_finalise();
    ck_assert_int_eq(res, NSERROR_OK);

    teardown_mock_gui();
}
END_TEST

START_TEST(test_search_web_select_provider)
{
    setup_mock_gui();

    nserror res = search_web_init(NULL);
    ck_assert_int_eq(res, NSERROR_OK);
    pump_scheduled();

    res = search_web_select_provider("DuckDuckGo");
    ck_assert_int_eq(res, NSERROR_OK);
    ck_assert_str_eq(last_provider_update, "DuckDuckGo");

    /* Selecting non-existent provider should default to index 0 */
    res = search_web_select_provider("NonExistentProvider");
    ck_assert_int_eq(res, NSERROR_OK);

    res = search_web_finalise();
    ck_assert_int_eq(res, NSERROR_OK);

    teardown_mock_gui();
}
END_TEST

START_TEST(test_search_web_https_urls)
{
    setup_mock_gui();

    nserror res = search_web_init("src/resources/SearchEngines");
    ck_assert_int_eq(res, NSERROR_OK);
    pump_scheduled();

    ssize_t iter = -1;
    const char *name = NULL;
    int count = 0;

    iter = search_web_iterate_providers(iter, &name);
    while (iter != -1) {
        ck_assert_ptr_nonnull(name);
        count++;

        res = search_web_select_provider(name);
        ck_assert_int_eq(res, NSERROR_OK);

        nsurl *url = NULL;
        res = search_web_omni("security_test", SEARCH_WEB_OMNI_SEARCHONLY, &url);
        ck_assert_int_eq(res, NSERROR_OK);
        ck_assert_ptr_nonnull(url);

        const char *url_str = nsurl_access(url);
        ck_assert_ptr_nonnull(url_str);
        ck_assert_msg(strncmp(url_str, "https://", 8) == 0,
                      "Provider %s search URL is not HTTPS: %s", name, url_str);

        nsurl_unref(url);

        iter = search_web_iterate_providers(iter, &name);
    }

    ck_assert_int_gt(count, 0);

    res = search_web_finalise();
    ck_assert_int_eq(res, NSERROR_OK);

    teardown_mock_gui();
}
END_TEST

static Suite *searchweb_suite_create(void)
{
    Suite *s;
    TCase *tc;

    s = suite_create("SearchWeb");
    tc = tcase_create("Core");

    tcase_add_test(tc, test_search_web_init_and_iterate);
    tcase_add_test(tc, test_search_web_omni_substitution);
    tcase_add_test(tc, test_search_web_select_provider);
    tcase_add_test(tc, test_search_web_https_urls);

    suite_add_tcase(s, tc);

    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    SRunner *sr;

    sr = srunner_create(searchweb_suite_create());

    srunner_run_all(sr, CK_ENV);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
