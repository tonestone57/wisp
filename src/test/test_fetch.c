#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "content/fetch.h"
#include "content/fetchers.h"
#include "utils/log.h"
#include "utils/nsurl.h"
#include "utils/nsoption.h"
#include "wisp/utils/inet.h"
#include "wisp/utils/config.h"
#include "utils/messages.h"
#include "utils/utils.h"
#include "utils/corestrings.h"

static bool test_fetcher_initialise(lwc_string *scheme) { return true; }
static bool test_fetcher_acceptable(const struct nsurl *url) { return true; }

static nserror test_fetcher_setup(struct fetch *parent_fetch, struct nsurl *url, bool only_2xx, bool downgrade_tls,
        const struct fetch_postdata *postdata, const char **headers, void **handle_out) {
    *handle_out = (void *)0x1234; // Dummy handle
    return NSERROR_OK;
}

static bool test_fetcher_start(void *fetch) { return true; }
static bool test_fetcher_abort_called = false;
static void test_fetcher_abort(void *fetch) { test_fetcher_abort_called = true; }

static bool test_fetcher_free_called = false;
static void test_fetcher_free(void *fetch) {
    test_fetcher_free_called = true;
}

static void test_fetcher_poll(lwc_string *scheme) {}
static void test_fetcher_finalise(lwc_string *scheme) {}

static const struct fetcher_operation_table test_fetcher_ops = {
    .initialise = test_fetcher_initialise,
    .acceptable = test_fetcher_acceptable,
    .setup = test_fetcher_setup,
    .start = test_fetcher_start,
    .abort = test_fetcher_abort,
    .free = test_fetcher_free,
    .poll = test_fetcher_poll,
    .finalise = test_fetcher_finalise
};


static bool test_callback_called = false;
static fetch_msg_type test_callback_msg_type = FETCH_PROGRESS;

static void test_fetch_callback(const fetch_msg *msg, void *p) {
    test_callback_called = true;
    test_callback_msg_type = msg->type;
}

static void setup_test_fetcher() {
    static bool setup = false;
    if (setup) return;

    // Corestrings and logging initialisation needed for messages?

    lwc_string *scheme;
    lwc_intern_string("http", 4, &scheme);

    // Add our test fetcher
    nserror err = fetcher_add(scheme, &test_fetcher_ops);
    ck_assert_int_eq(err, NSERROR_OK);

    lwc_string_unref(scheme);
    setup = true;
}

static void setup_mock_options(void) {
    nsoption_set_int(max_fetchers, 10);
}

START_TEST(test_fetch_free_failure)
{
    setup_test_fetcher();
    // Setup lwc and nsurl first
    nsurl *url;
    nsurl_create("http://example.com", &url);

    // Create a fetch object
    struct fetch *f = NULL;
    nserror err = fetch_start(url, NULL, test_fetch_callback, NULL, false, NULL, true, false, NULL, &f);

    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_ne(f, NULL);

    // Now test fetch_free when it didn't finish
    test_callback_called = false;
    test_fetcher_free_called = false;

    // We are simulating an aborted or unfinished fetch
    fetch_remove_from_queues(f);
    fetch_free(f);

    ck_assert_int_eq(test_callback_called, true);
    ck_assert_int_eq(test_callback_msg_type, FETCH_ERROR);
    ck_assert_int_eq(test_fetcher_free_called, true);

    nsurl_unref(url);
}
END_TEST

START_TEST(test_fetch_free_success)
{
    setup_test_fetcher();
    // Setup lwc and nsurl first
    nsurl *url;
    nsurl_create("http://example.com/2", &url);

    // Create a fetch object
    struct fetch *f = NULL;
    nserror err = fetch_start(url, NULL, test_fetch_callback, NULL, false, NULL, true, false, NULL, &f);

    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_ne(f, NULL);

    // Mock completing the fetch
    // FETCH_MIN_FINISHED_MSG is FETCH_FINISHED
    fetch_msg msg = { .type = FETCH_FINISHED };
    // normally fetch_send_callback sets f->last_msg
    fetch_send_callback(&msg, f);

    // Now test fetch_free when it did finish
    test_callback_called = false;
    test_fetcher_free_called = false;

    fetch_remove_from_queues(f);
    fetch_free(f);

    ck_assert_int_eq(test_callback_called, false); // Shouldn't be called on free if finished
    ck_assert_int_eq(test_fetcher_free_called, true);

    nsurl_unref(url);
}
END_TEST


START_TEST(test_fetch_abort_success)
{
    setup_test_fetcher();
    // Setup lwc and nsurl first
    nsurl *url;
    nsurl_create("http://example.com/abort", &url);

    // Create a fetch object
    struct fetch *f = NULL;
    nserror err = fetch_start(url, NULL, test_fetch_callback, NULL, false, NULL, true, false, NULL, &f);

    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_ne(f, NULL);

    test_fetcher_abort_called = false;
    fetch_abort(f);

    ck_assert_int_eq(test_fetcher_abort_called, true);
    // ck_assert_int_eq(f->last_msg, FETCH__INTERNAL_ABORTED); struct fetch is opaque here

    fetch_remove_from_queues(f);
    fetch_free(f);

    nsurl_unref(url);
}
END_TEST

Suite *fetch_suite(void)
{
    Suite *s = suite_create("Fetch");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_fetch_free_failure);
    tcase_add_test(tc_core, test_fetch_free_success);
    tcase_add_test(tc_core, test_fetch_abort_success);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    nsoption_init(NULL, NULL, NULL);
    corestrings_init();
    setup_mock_options();

    int number_failed;
    Suite *s = fetch_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    corestrings_fini();
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
