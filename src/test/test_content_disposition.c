#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "utils/corestrings.h"
#include "utils/http/content-disposition.h"
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
    http_content_disposition *cd = NULL;
    nserror error;
    lwc_string *key;
    lwc_string *val;

    /* Test 1: simple "attachment" */
    error = http_parse_content_disposition("attachment", &cd);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(cd);
    ck_assert_ptr_nonnull(cd->disposition_type);
    ck_assert_int_eq(lwc_string_length(cd->disposition_type), 10);
    ck_assert_int_eq(strncmp(lwc_string_data(cd->disposition_type), "attachment", 10), 0);
    ck_assert_ptr_null(cd->parameters);
    http_content_disposition_destroy(cd);

    /* Test 2: "inline" */
    error = http_parse_content_disposition("inline", &cd);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(cd);
    ck_assert_int_eq(lwc_string_length(cd->disposition_type), 6);
    ck_assert_int_eq(strncmp(lwc_string_data(cd->disposition_type), "inline", 6), 0);
    ck_assert_ptr_null(cd->parameters);
    http_content_disposition_destroy(cd);

    /* Test 3: "attachment; filename=\"foo.html\"" */
    error = http_parse_content_disposition("attachment; filename=\"foo.html\"", &cd);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(cd);
    ck_assert_int_eq(lwc_string_length(cd->disposition_type), 10);
    ck_assert_int_eq(strncmp(lwc_string_data(cd->disposition_type), "attachment", 10), 0);
    ck_assert_ptr_nonnull(cd->parameters);

    lwc_intern_string("filename", 8, &key);
    error = http_parameter_list_find_item(cd->parameters, key, &val);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_int_eq(lwc_string_length(val), 8);
    ck_assert_int_eq(strncmp(lwc_string_data(val), "foo.html", 8), 0);
    lwc_string_unref(key);
    lwc_string_unref(val);

    http_content_disposition_destroy(cd);

    /* Test 4: form-data */
    error = http_parse_content_disposition("form-data; name=\"fieldName\"; filename=\"filename.jpg\"", &cd);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(cd);
    ck_assert_int_eq(lwc_string_length(cd->disposition_type), 9);
    ck_assert_int_eq(strncmp(lwc_string_data(cd->disposition_type), "form-data", 9), 0);
    ck_assert_ptr_nonnull(cd->parameters);

    lwc_intern_string("name", 4, &key);
    error = http_parameter_list_find_item(cd->parameters, key, &val);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_int_eq(lwc_string_length(val), 9);
    ck_assert_int_eq(strncmp(lwc_string_data(val), "fieldName", 9), 0);
    lwc_string_unref(key);
    lwc_string_unref(val);

    lwc_intern_string("filename", 8, &key);
    error = http_parameter_list_find_item(cd->parameters, key, &val);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_int_eq(lwc_string_length(val), 12);
    ck_assert_int_eq(strncmp(lwc_string_data(val), "filename.jpg", 12), 0);
    lwc_string_unref(key);
    lwc_string_unref(val);

    http_content_disposition_destroy(cd);
}
END_TEST

START_TEST(test_parse_edge_cases)
{
    http_content_disposition *cd = NULL;
    nserror error;

    /* Test 1: Empty string */
    error = http_parse_content_disposition("", &cd);
    ck_assert_int_ne(error, NSERROR_OK);

    /* Test 2: Only spaces */
    error = http_parse_content_disposition("   ", &cd);
    ck_assert_int_ne(error, NSERROR_OK);

    /* Test 3: Malformed parameter */
    error = http_parse_content_disposition("attachment; filename=", &cd);
    if (error == NSERROR_OK) {
        http_content_disposition_destroy(cd);
    }
}
END_TEST

static Suite *test_suite(void)
{
    Suite *s = suite_create("content-disposition");
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
