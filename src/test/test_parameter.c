#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "utils/http/parameter.h"
#include "utils/http/parameter_internal.h"
#include <wisp/utils/corestrings.h>

START_TEST(test_http_parse_parameter)
{
    const char *input = "charset=utf-8";
    const char *pos = input;
    http__item *param = NULL;
    nserror err;

    err = http__parse_parameter(&pos, &param);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(param);

    lwc_string *name = NULL;
    lwc_string *val = NULL;

    const http_parameter *iter = http_parameter_list_iterate((const http_parameter *)param, &name, &val);
    ck_assert_ptr_null(iter);

    ck_assert_ptr_nonnull(name);
    ck_assert_ptr_nonnull(val);

    ck_assert_int_eq(lwc_string_length(name), 7);
    ck_assert_int_eq(strncmp(lwc_string_data(name), "charset", 7), 0);

    ck_assert_int_eq(lwc_string_length(val), 5);
    ck_assert_int_eq(strncmp(lwc_string_data(val), "utf-8", 5), 0);

    lwc_string_unref(name);
    lwc_string_unref(val);

    http_parameter_list_destroy((http_parameter *)param);
    param = NULL;

    // Test with quotes
    input = "name=\"value with spaces\", next";
    pos = input;
    err = http__parse_parameter(&pos, &param);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(param);
    ck_assert_str_eq(pos, ", next");

    name = NULL;
    val = NULL;
    iter = http_parameter_list_iterate((const http_parameter *)param, &name, &val);
    ck_assert_ptr_null(iter);

    ck_assert_int_eq(lwc_string_length(name), 4);
    ck_assert_int_eq(strncmp(lwc_string_data(name), "name", 4), 0);

    ck_assert_int_eq(lwc_string_length(val), 17);
    ck_assert_int_eq(strncmp(lwc_string_data(val), "value with spaces", 17), 0);

    lwc_string_unref(name);
    lwc_string_unref(val);
    http_parameter_list_destroy((http_parameter *)param);
    param = NULL;

    // Test with escaped characters inside quoted string
    input = "title=\"hello \\\"world\\\"\"";
    pos = input;
    err = http__parse_parameter(&pos, &param);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(param);

    name = NULL;
    val = NULL;
    iter = http_parameter_list_iterate((const http_parameter *)param, &name, &val);
    ck_assert_ptr_null(iter);

    ck_assert_int_eq(lwc_string_length(name), 5);
    ck_assert_int_eq(strncmp(lwc_string_data(name), "title", 5), 0);

    ck_assert_int_eq(lwc_string_length(val), 13);
    ck_assert_int_eq(strncmp(lwc_string_data(val), "hello \"world\"", 13), 0);

    lwc_string_unref(name);
    lwc_string_unref(val);
    http_parameter_list_destroy((http_parameter *)param);
    param = NULL;

    // Test with LWS, parameter parse expects pos to point directly to token
    input = "foo \t =  bar  ";
    pos = input;
    err = http__parse_parameter(&pos, &param);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(param);

    name = NULL;
    val = NULL;
    iter = http_parameter_list_iterate((const http_parameter *)param, &name, &val);
    ck_assert_ptr_null(iter);

    ck_assert_int_eq(lwc_string_length(name), 3);
    ck_assert_int_eq(strncmp(lwc_string_data(name), "foo", 3), 0);

    ck_assert_int_eq(lwc_string_length(val), 3);
    ck_assert_int_eq(strncmp(lwc_string_data(val), "bar", 3), 0);

    lwc_string_unref(name);
    lwc_string_unref(val);
    http_parameter_list_destroy((http_parameter *)param);
    param = NULL;

    // Test invalid missing =
    input = "foo bar";
    pos = input;
    err = http__parse_parameter(&pos, &param);
    ck_assert_int_eq(err, NSERROR_NOT_FOUND);

    // Test invalid missing value
    input = "foo=";
    pos = input;
    err = http__parse_parameter(&pos, &param);
    ck_assert_int_eq(err, NSERROR_NOT_FOUND);

    // Test unclosed quoted string
    input = "key=\"unclosed";
    pos = input;
    err = http__parse_parameter(&pos, &param);
    ck_assert_int_ne(err, NSERROR_OK);

    // Test invalid token start (starts with '=')
    input = "=value";
    pos = input;
    err = http__parse_parameter(&pos, &param);
    ck_assert_int_ne(err, NSERROR_OK);

    // Test invalid token start (starts with '@')
    input = "@key=value";
    pos = input;
    err = http__parse_parameter(&pos, &param);
    ck_assert_int_ne(err, NSERROR_OK);
}
END_TEST

START_TEST(test_http_parameter_list_find)
{
    const char *pos = "charset=utf-8";
    http__item *param1 = NULL;
    nserror err = http__parse_parameter(&pos, &param1);
    ck_assert_int_eq(err, NSERROR_OK);

    pos = "name=\"test file\"";
    http__item *param2 = NULL;
    err = http__parse_parameter(&pos, &param2);
    ck_assert_int_eq(err, NSERROR_OK);

    // Link them together
    param1->next = param2;

    // Now test find
    lwc_string *name_to_find = NULL;
    lwc_string *found_val = NULL;

    // Find first item
    lwc_intern_string("charset", 7, &name_to_find);
    err = http_parameter_list_find_item((http_parameter *)param1, name_to_find, &found_val);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(found_val);
    ck_assert_int_eq(strncmp(lwc_string_data(found_val), "utf-8", 5), 0);
    lwc_string_unref(found_val);
    lwc_string_unref(name_to_find);
    found_val = NULL;
    name_to_find = NULL;

    // Find second item (case insensitive)
    lwc_intern_string("NAME", 4, &name_to_find);
    err = http_parameter_list_find_item((http_parameter *)param1, name_to_find, &found_val);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(found_val);
    ck_assert_int_eq(strncmp(lwc_string_data(found_val), "test file", 9), 0);
    lwc_string_unref(found_val);
    lwc_string_unref(name_to_find);
    found_val = NULL;
    name_to_find = NULL;

    // Find non-existent
    lwc_intern_string("missing", 7, &name_to_find);
    err = http_parameter_list_find_item((http_parameter *)param1, name_to_find, &found_val);
    ck_assert_int_eq(err, NSERROR_NOT_FOUND);
    lwc_string_unref(name_to_find);

    // Find on NULL list
    lwc_intern_string("charset", 7, &name_to_find);
    err = http_parameter_list_find_item(NULL, name_to_find, &found_val);
    ck_assert_int_eq(err, NSERROR_NOT_FOUND);
    lwc_string_unref(name_to_find);

    http_parameter_list_destroy((http_parameter *)param1);
}
END_TEST

START_TEST(test_http_parameter_list_iterate)
{
    const char *pos = "first=1";
    http__item *param1 = NULL;
    nserror err = http__parse_parameter(&pos, &param1);
    ck_assert_int_eq(err, NSERROR_OK);

    pos = "second=2";
    http__item *param2 = NULL;
    err = http__parse_parameter(&pos, &param2);
    ck_assert_int_eq(err, NSERROR_OK);

    param1->next = param2;

    lwc_string *name = NULL;
    lwc_string *val = NULL;

    const http_parameter *iter = http_parameter_list_iterate((const http_parameter *)param1, &name, &val);
    ck_assert_ptr_nonnull(iter);
    ck_assert_int_eq(strncmp(lwc_string_data(name), "first", 5), 0);
    ck_assert_int_eq(strncmp(lwc_string_data(val), "1", 1), 0);
    lwc_string_unref(name);
    lwc_string_unref(val);

    const http_parameter *iter2 = http_parameter_list_iterate(iter, &name, &val);
    ck_assert_ptr_null(iter2);
    ck_assert_int_eq(strncmp(lwc_string_data(name), "second", 6), 0);
    ck_assert_int_eq(strncmp(lwc_string_data(val), "2", 1), 0);
    lwc_string_unref(name);
    lwc_string_unref(val);

    // Iterate on NULL pointer
    const http_parameter *null_iter = http_parameter_list_iterate(NULL, &name, &val);
    ck_assert_ptr_null(null_iter);

    http_parameter_list_destroy((http_parameter *)param1);
}
END_TEST

START_TEST(test_http_parameter_list_destroy_null)
{
    // Test that destroying NULL list does not crash
    http_parameter_list_destroy(NULL);
}
END_TEST

static Suite *parameter_suite_create(void)
{
    Suite *s;
    TCase *tc;

    s = suite_create("http-parameter");
    tc = tcase_create("Core");

    tcase_add_test(tc, test_http_parse_parameter);
    tcase_add_test(tc, test_http_parameter_list_find);
    tcase_add_test(tc, test_http_parameter_list_iterate);
    tcase_add_test(tc, test_http_parameter_list_destroy_null);

    suite_add_tcase(s, tc);

    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    SRunner *sr;

    if (corestrings_init() != NSERROR_OK)
        return EXIT_FAILURE;

    sr = srunner_create(parameter_suite_create());
    srunner_run_all(sr, CK_ENV);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    corestrings_fini();

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
