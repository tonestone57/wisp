#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "content/fetch.h"
#include "content/fetchers.h"
#include "content/fetchers/curl.h"
#include "content/urldb.h"
#include <unistd.h>
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

START_TEST(test_fetch_set_cookie_verifiable)
{
    setup_test_fetcher();
    nsurl *url;
    nsurl_create("http://example.com/cookie_verifiable", &url);

    struct fetch *f = NULL;
    nserror err = fetch_start(url, NULL, test_fetch_callback, NULL, false, NULL, true, false, NULL, &f);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_ne(f, NULL);

    fetch_set_cookie(f, "vcookie=val1; Path=/; Domain=example.com");

    char *cookie = urldb_get_cookie(url, true);
    ck_assert_ptr_ne(cookie, NULL);
    ck_assert_ptr_ne(strstr(cookie, "vcookie=val1"), NULL);
    free(cookie);

    fetch_msg msg = { .type = FETCH_FINISHED };
    fetch_send_callback(&msg, f);
    fetch_remove_from_queues(f);
    fetch_free(f);
    nsurl_unref(url);
}
END_TEST

START_TEST(test_fetch_set_cookie_unverifiable_matching)
{
    setup_test_fetcher();
    nsurl *url, *referer;
    nsurl_create("http://example.com/subresource", &url);
    nsurl_create("http://example.com/parent", &referer);

    struct fetch *f = NULL;
    nserror err = fetch_start(url, referer, test_fetch_callback, NULL, false, NULL, false, false, NULL, &f);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_ne(f, NULL);

    fetch_set_cookie(f, "ucookie=val2; Path=/; Domain=example.com");

    char *cookie = urldb_get_cookie(url, true);
    ck_assert_ptr_ne(cookie, NULL);
    ck_assert_ptr_ne(strstr(cookie, "ucookie=val2"), NULL);
    free(cookie);

    fetch_msg msg = { .type = FETCH_FINISHED };
    fetch_send_callback(&msg, f);
    fetch_remove_from_queues(f);
    fetch_free(f);
    nsurl_unref(referer);
    nsurl_unref(url);
}
END_TEST

START_TEST(test_fetch_set_cookie_unverifiable_no_referer)
{
    setup_test_fetcher();
    nsurl *url;
    nsurl_create("http://noreferer.com/subresource", &url);

    struct fetch *f = NULL;
    nserror err = fetch_start(url, NULL, test_fetch_callback, NULL, false, NULL, false, false, NULL, &f);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_ne(f, NULL);

    fetch_set_cookie(f, "nocookie=val3; Path=/; Domain=noreferer.com");

    char *cookie = urldb_get_cookie(url, true);
    ck_assert_ptr_eq(cookie, NULL);

    fetch_msg msg = { .type = FETCH_FINISHED };
    fetch_send_callback(&msg, f);
    fetch_remove_from_queues(f);
    fetch_free(f);
    nsurl_unref(url);
}
END_TEST

static int redirect_test_header_count = 0;
static int redirect_test_redirect_count = 0;

static void redirect_test_callback(const fetch_msg *msg, void *p)
{
    if (msg->type == FETCH_HEADER) {
        redirect_test_header_count++;
    } else if (msg->type == FETCH_REDIRECT) {
        redirect_test_redirect_count++;
    }
}

START_TEST(test_fetch_immediate_redirect_processing)
{
    setup_test_fetcher();
    nsurl *url;
    nsurl_create("http://example.com/redir_immediate", &url);

    struct fetch *f = NULL;
    nserror err = fetch_start(url, NULL, redirect_test_callback, NULL, false, NULL, true, false, NULL, &f);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_ne(f, NULL);

    redirect_test_header_count = 0;
    redirect_test_redirect_count = 0;

    /* Set HTTP status code 301 on the fetch */
    fetch_set_http_code(f, 301);

    /* Simulate header callback delivering HTTP 301 header line followed by Location: header */
    fetch_msg hmsg1 = { .type = FETCH_HEADER };
    fetch_send_callback(&hmsg1, f);

    /* Simulate Location: header dispatching FETCH_REDIRECT */
    nsurl *target_url = NULL;
    nsurl_create("https://example.com/target", &target_url);
    fetch_msg rmsg = { .type = FETCH_REDIRECT, .data = { .redirect = target_url } };
    fetch_send_callback(&rmsg, f);

    ck_assert_int_eq(redirect_test_header_count, 1);
    ck_assert_int_eq(redirect_test_redirect_count, 1);

    nsurl_unref(target_url);
    fetch_remove_from_queues(f);
    fetch_free(f);
    nsurl_unref(url);
}
END_TEST

START_TEST(test_fetch_multipart_data_destroy_null)
{
    /* Passing NULL should execute safely without crashing */
    fetch_multipart_data_destroy(NULL);
}
END_TEST

START_TEST(test_fetch_multipart_data_destroy_single_kv)
{
    struct fetch_multipart_data *list = NULL;
    nserror err = fetch_multipart_data_new_kv(&list, "field_name", "field_value");
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_ne(list, NULL);

    fetch_multipart_data_destroy(list);
}
END_TEST

START_TEST(test_fetch_multipart_data_new_kv_invalid_params)
{
    struct fetch_multipart_data *list = NULL;

    /* Verify NULL name returns NSERROR_BAD_PARAMETER */
    nserror err = fetch_multipart_data_new_kv(&list, NULL, "value");
    ck_assert_int_eq(err, NSERROR_BAD_PARAMETER);
    ck_assert_ptr_eq(list, NULL);

    /* Verify NULL value returns NSERROR_BAD_PARAMETER */
    err = fetch_multipart_data_new_kv(&list, "name", NULL);
    ck_assert_int_eq(err, NSERROR_BAD_PARAMETER);
    ck_assert_ptr_eq(list, NULL);

    /* Safe cleanup on NULL list */
    fetch_multipart_data_destroy(list);
}
END_TEST

START_TEST(test_fetch_multipart_data_destroy_file)
{
    struct fetch_multipart_data *item = calloc(1, sizeof(*item));
    ck_assert_ptr_ne(item, NULL);

    item->name = strdup("upload");
    item->value = strdup("file.txt");
    item->rawfile = strdup("/tmp/file.txt");
    item->file = true;
    item->next = NULL;

    fetch_multipart_data_destroy(item);
}
END_TEST

START_TEST(test_fetch_multipart_data_destroy_chain)
{
    struct fetch_multipart_data *list = NULL;

    /* Add key-value node 1 */
    nserror err = fetch_multipart_data_new_kv(&list, "username", "alice");
    ck_assert_int_eq(err, NSERROR_OK);

    /* Add key-value node 2 */
    err = fetch_multipart_data_new_kv(&list, "action", "upload");
    ck_assert_int_eq(err, NSERROR_OK);

    /* Add file node 3 manually */
    struct fetch_multipart_data *file_item = calloc(1, sizeof(*file_item));
    ck_assert_ptr_ne(file_item, NULL);

    file_item->name = strdup("file_attachment");
    file_item->value = strdup("document.pdf");
    file_item->rawfile = strdup("/path/to/document.pdf");
    file_item->file = true;
    file_item->next = list;
    list = file_item;

    fetch_multipart_data_destroy(list);
}
END_TEST

START_TEST(test_fetch_multipart_data_destroy_file_null_rawfile)
{
    struct fetch_multipart_data *item = calloc(1, sizeof(*item));
    ck_assert_ptr_ne(item, NULL);

    item->name = strdup("upload_no_raw");
    item->value = strdup("file_no_raw.txt");
    item->rawfile = NULL;
    item->file = true;
    item->next = NULL;

    fetch_multipart_data_destroy(item);
}
END_TEST

START_TEST(test_fetch_multipart_data_destroy_null_fields)
{
    struct fetch_multipart_data *item = calloc(1, sizeof(*item));
    ck_assert_ptr_ne(item, NULL);

    item->name = NULL;
    item->value = NULL;
    item->rawfile = NULL;
    item->file = false;

    struct fetch_multipart_data *item2 = calloc(1, sizeof(*item2));
    ck_assert_ptr_ne(item2, NULL);
    item2->name = strdup("valid_name");
    item2->value = NULL;
    item2->file = true;
    item2->rawfile = NULL;

    item->next = item2;

    fetch_multipart_data_destroy(item);
}
END_TEST

START_TEST(test_fetch_multipart_data_destroy_cloned)
{
    struct fetch_multipart_data *orig = NULL;
    nserror err = fetch_multipart_data_new_kv(&orig, "key1", "val1");
    ck_assert_int_eq(err, NSERROR_OK);

    struct fetch_multipart_data *file_item = calloc(1, sizeof(*file_item));
    ck_assert_ptr_ne(file_item, NULL);
    file_item->name = strdup("attachment");
    file_item->value = strdup("notes.txt");
    file_item->rawfile = strdup("/tmp/notes.txt");
    file_item->file = true;
    file_item->next = orig;
    orig = file_item;

    struct fetch_multipart_data *clone = fetch_multipart_data_clone(orig);
    ck_assert_ptr_ne(clone, NULL);

    /* Verify search finding elements in original and clone */
    ck_assert_str_eq(fetch_multipart_data_find(orig, "key1"), "val1");
    ck_assert_str_eq(fetch_multipart_data_find(clone, "key1"), "val1");
    ck_assert_str_eq(fetch_multipart_data_find(orig, "attachment"), "notes.txt");
    ck_assert_str_eq(fetch_multipart_data_find(clone, "attachment"), "notes.txt");

    /* Destroy original and clone independently */
    fetch_multipart_data_destroy(orig);
    fetch_multipart_data_destroy(clone);
}
END_TEST

START_TEST(test_fetch_multipart_data_destroy_large_mixed_chain)
{
    struct fetch_multipart_data *list = NULL;

    for (int i = 0; i < 10; i++) {
        char name[32], val[32], raw[64];
        snprintf(name, sizeof(name), "field_%d", i);
        snprintf(val, sizeof(val), "value_%d", i);
        snprintf(raw, sizeof(raw), "/path/to/file_%d.dat", i);

        if (i % 3 == 0) {
            /* Key-value pair */
            nserror err = fetch_multipart_data_new_kv(&list, name, val);
            ck_assert_int_eq(err, NSERROR_OK);
        } else if (i % 3 == 1) {
            /* File item with rawfile */
            struct fetch_multipart_data *item = calloc(1, sizeof(*item));
            ck_assert_ptr_ne(item, NULL);
            item->name = strdup(name);
            item->value = strdup(val);
            item->rawfile = strdup(raw);
            item->file = true;
            item->next = list;
            list = item;
        } else {
            /* File item with NULL rawfile */
            struct fetch_multipart_data *item = calloc(1, sizeof(*item));
            ck_assert_ptr_ne(item, NULL);
            item->name = strdup(name);
            item->value = strdup(val);
            item->rawfile = NULL;
            item->file = true;
            item->next = list;
            list = item;
        }
    }

    /* Verify lookup works */
    ck_assert_str_eq(fetch_multipart_data_find(list, "field_0"), "value_0");
    ck_assert_str_eq(fetch_multipart_data_find(list, "field_9"), "value_9");
    ck_assert_ptr_eq(fetch_multipart_data_find(list, "nonexistent"), NULL);

    /* Destroy the whole chain */
    fetch_multipart_data_destroy(list);
}
END_TEST

START_TEST(test_fetch_curl_preconnect_null)
{
    /* Passing NULL url string should execute safely via guard check */
    fetch_curl_preconnect(NULL);
    fetch_curl_dns_prefetch(NULL);
}
END_TEST

START_TEST(test_fetch_curl_preconnect_valid)
{
    /* Register curl fetcher which initialises network_thread_pool */
    nserror err = fetch_curl_register();
    ck_assert_int_eq(err, NSERROR_OK);

    /* Test preconnect and DNS prefetch with valid URL string */
    fetch_curl_preconnect("http://127.0.0.1:9");
    fetch_curl_dns_prefetch("127.0.0.1");

    /* Allow background thread pool worker tasks to execute */
    usleep(50000);
}
END_TEST

START_TEST(test_fetch_curl_security_options)
{
    nserror err = fetch_curl_register();
    ck_assert_int_eq(err, NSERROR_OK);

    nsurl *url;
    nsurl_create("https://example.com/security_test", &url);

    struct fetch *f = NULL;
    err = fetch_start(url, NULL, test_fetch_callback, NULL, false, NULL, true, false, NULL, &f);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_ne(f, NULL);

    fetch_msg msg = { .type = FETCH_FINISHED };
    fetch_send_callback(&msg, f);
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

    TCase *tc_cookie = tcase_create("Cookie");
    tcase_add_test(tc_cookie, test_fetch_set_cookie_verifiable);
    tcase_add_test(tc_cookie, test_fetch_set_cookie_unverifiable_matching);
    tcase_add_test(tc_cookie, test_fetch_set_cookie_unverifiable_no_referer);
    tcase_add_test(tc_cookie, test_fetch_immediate_redirect_processing);
    suite_add_tcase(s, tc_cookie);

    TCase *tc_multipart = tcase_create("Multipart");
    tcase_add_test(tc_multipart, test_fetch_multipart_data_destroy_null);
    tcase_add_test(tc_multipart, test_fetch_multipart_data_destroy_single_kv);
    tcase_add_test(tc_multipart, test_fetch_multipart_data_destroy_file);
    tcase_add_test(tc_multipart, test_fetch_multipart_data_destroy_chain);
    tcase_add_test(tc_multipart, test_fetch_multipart_data_destroy_file_null_rawfile);
    tcase_add_test(tc_multipart, test_fetch_multipart_data_destroy_null_fields);
    tcase_add_test(tc_multipart, test_fetch_multipart_data_destroy_cloned);
    tcase_add_test(tc_multipart, test_fetch_multipart_data_destroy_large_mixed_chain);
    tcase_add_test(tc_multipart, test_fetch_multipart_data_new_kv_invalid_params);
    suite_add_tcase(s, tc_multipart);

    TCase *tc_curl = tcase_create("cURL");
    tcase_add_test(tc_curl, test_fetch_curl_preconnect_null);
    tcase_add_test(tc_curl, test_fetch_curl_preconnect_valid);
    tcase_add_test(tc_curl, test_fetch_curl_security_options);
    suite_add_tcase(s, tc_curl);

    return s;
}

int main(void)
{
    nsoption_init(NULL, NULL, NULL);
    corestrings_init();
    urldb_init();
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
