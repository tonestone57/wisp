#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "utils/corestrings.h"
#include "utils/http/content-type.h"
#include "wisp/utils/errors.h"

static void setup(void)
{
    corestrings_init();
}

static void teardown(void)
{
    corestrings_fini();
}

START_TEST(test_parse_happy_paths)
{
    http_content_type *ct = NULL;
    nserror error;
    lwc_string *key;
    lwc_string *val;

    /* Test 1: simple "text/html" */
    error = http_parse_content_type("text/html", &ct);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(ct);
    ck_assert_ptr_nonnull(ct->media_type);
    ck_assert_int_eq(lwc_string_length(ct->media_type), 9);
    ck_assert_int_eq(strncmp(lwc_string_data(ct->media_type), "text/html", 9), 0);
    ck_assert_ptr_null(ct->parameters);
    http_content_type_destroy(ct);

    /* Test 2: "text/html; charset=utf-8" */
    error = http_parse_content_type("text/html; charset=utf-8", &ct);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(ct);
    ck_assert_int_eq(lwc_string_length(ct->media_type), 9);
    ck_assert_int_eq(strncmp(lwc_string_data(ct->media_type), "text/html", 9), 0);
    ck_assert_ptr_nonnull(ct->parameters);

    lwc_intern_string("charset", 7, &key);
    error = http_parameter_list_find_item(ct->parameters, key, &val);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_int_eq(lwc_string_length(val), 5);
    ck_assert_int_eq(strncmp(lwc_string_data(val), "utf-8", 5), 0);
    lwc_string_unref(key);
    lwc_string_unref(val);

    http_content_type_destroy(ct);

    /* Test 3: "application/json; charset=\"utf-8\"; foo=bar" */
    error = http_parse_content_type("application/json; charset=\"utf-8\"; foo=bar", &ct);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(ct);
    ck_assert_int_eq(lwc_string_length(ct->media_type), 16);
    ck_assert_int_eq(strncmp(lwc_string_data(ct->media_type), "application/json", 16), 0);
    ck_assert_ptr_nonnull(ct->parameters);

    lwc_intern_string("charset", 7, &key);
    error = http_parameter_list_find_item(ct->parameters, key, &val);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_int_eq(lwc_string_length(val), 5);
    ck_assert_int_eq(strncmp(lwc_string_data(val), "utf-8", 5), 0);
    lwc_string_unref(key);
    lwc_string_unref(val);

    lwc_intern_string("foo", 3, &key);
    error = http_parameter_list_find_item(ct->parameters, key, &val);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_int_eq(lwc_string_length(val), 3);
    ck_assert_int_eq(strncmp(lwc_string_data(val), "bar", 3), 0);
    lwc_string_unref(key);
    lwc_string_unref(val);

    http_content_type_destroy(ct);
}
END_TEST

START_TEST(test_parse_edge_cases)
{
    http_content_type *ct = NULL;
    nserror error;

    /* Test 1: Empty string */
    error = http_parse_content_type("", &ct);
    ck_assert_int_ne(error, NSERROR_OK);

    /* Test 2: Only spaces */
    error = http_parse_content_type("   ", &ct);
    ck_assert_int_ne(error, NSERROR_OK);

    /* Test 3: Missing subtype */
    error = http_parse_content_type("text", &ct);
    ck_assert_int_ne(error, NSERROR_OK);

    /* Test 4: Missing subtype after slash */
    error = http_parse_content_type("text/", &ct);
    ck_assert_int_ne(error, NSERROR_OK);

    /* Test 5: Malformed parameter */
    /* NetSurf currently treats malformed parameters (where no item could be parsed)
     * by ignoring the error (returning NSERROR_NOT_FOUND which is not an error)
     * and continuing to create the content type without parameters. */
    error = http_parse_content_type("text/html; charset=", &ct);
    ck_assert_int_eq(error, NSERROR_OK);
    if (error == NSERROR_OK) {
        ck_assert_ptr_nonnull(ct);
        ck_assert_ptr_null(ct->parameters);
        http_content_type_destroy(ct);
    }

    /* Test 6: Missing type before slash */
    error = http_parse_content_type("/html", &ct);
    ck_assert_int_ne(error, NSERROR_OK);
    if (error == NSERROR_OK) {
        http_content_type_destroy(ct);
    }

    /* Test 7: Trailing whitespace */
    error = http_parse_content_type("text/html   ", &ct);
    ck_assert_int_eq(error, NSERROR_OK);
    if (error == NSERROR_OK) {
        ck_assert_ptr_nonnull(ct);
        ck_assert_int_eq(lwc_string_length(ct->media_type), 9);
        ck_assert_int_eq(strncmp(lwc_string_data(ct->media_type), "text/html", 9), 0);
        http_content_type_destroy(ct);
    }

    /* Test 8: Whitespace around slash */
    error = http_parse_content_type("text / html", &ct);
    ck_assert_int_eq(error, NSERROR_OK);
    if (error == NSERROR_OK) {
        ck_assert_ptr_nonnull(ct);
        ck_assert_int_eq(lwc_string_length(ct->media_type), 9);
        ck_assert_int_eq(strncmp(lwc_string_data(ct->media_type), "text/html", 9), 0);
        http_content_type_destroy(ct);
    }
}
END_TEST

static Suite *test_suite(void)
{
    Suite *s = suite_create("content-type");
    TCase *tc_core = tcase_create("Core");

    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_parse_happy_paths);
    tcase_add_test(tc_core, test_parse_edge_cases);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = test_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
