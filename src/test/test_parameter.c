#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "utils/corestrings.h"
#include "utils/http/parameter_internal.h"
#include "wisp/utils/errors.h"
#include "utils/http/generics.h"
#include "utils/http/parameter.h"
#include "utils/http/primitives.h"
#include "utils/http.h"

static void setup(void)
{
    corestrings_init();
}

static void teardown(void)
{
    corestrings_fini();
}

START_TEST(test_parse_parameter_happy)
{
    const char *input = "charset=utf-8";
    const char *pos = input;
    http__item *param = NULL;
    nserror error;

    error = http__parse_parameter(&pos, &param);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(param);

    lwc_string *name;
    lwc_string *value;

    const http_parameter *cur = http_parameter_list_iterate((http_parameter *)param, &name, &value);
    ck_assert_ptr_nonnull(name);
    ck_assert_ptr_nonnull(value);

    ck_assert_int_eq(lwc_string_length(name), 7);
    ck_assert_int_eq(strncmp(lwc_string_data(name), "charset", 7), 0);

    ck_assert_int_eq(lwc_string_length(value), 5);
    ck_assert_int_eq(strncmp(lwc_string_data(value), "utf-8", 5), 0);

    lwc_string_unref(name);
    lwc_string_unref(value);

    ck_assert_ptr_null(cur);

    http_parameter_list_destroy((http_parameter *)param);
}
END_TEST

START_TEST(test_parse_parameter_quoted)
{
    const char *input = "filename=\"test file.txt\"";
    const char *pos = input;
    http__item *param = NULL;
    nserror error;

    error = http__parse_parameter(&pos, &param);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(param);

    lwc_string *name;
    lwc_string *value;

    const http_parameter *cur = http_parameter_list_iterate((http_parameter *)param, &name, &value);
    ck_assert_ptr_nonnull(name);
    ck_assert_ptr_nonnull(value);

    ck_assert_int_eq(lwc_string_length(name), 8);
    ck_assert_int_eq(strncmp(lwc_string_data(name), "filename", 8), 0);

    ck_assert_int_eq(lwc_string_length(value), 13);
    ck_assert_int_eq(strncmp(lwc_string_data(value), "test file.txt", 13), 0);

    lwc_string_unref(name);
    lwc_string_unref(value);

    ck_assert_ptr_null(cur);

    http_parameter_list_destroy((http_parameter *)param);
}
END_TEST

START_TEST(test_parse_parameter_spaces)
{
    const char *input = "charset  =  utf-8";
    const char *pos = input;
    http__item *param = NULL;
    nserror error;

    error = http__parse_parameter(&pos, &param);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(param);

    lwc_string *name;
    lwc_string *value;

    const http_parameter *cur = http_parameter_list_iterate((http_parameter *)param, &name, &value);
    ck_assert_ptr_nonnull(name);
    ck_assert_ptr_nonnull(value);

    ck_assert_int_eq(lwc_string_length(name), 7);
    ck_assert_int_eq(strncmp(lwc_string_data(name), "charset", 7), 0);

    ck_assert_int_eq(lwc_string_length(value), 5);
    ck_assert_int_eq(strncmp(lwc_string_data(value), "utf-8", 5), 0);

    lwc_string_unref(name);
    lwc_string_unref(value);

    ck_assert_ptr_null(cur);

    http_parameter_list_destroy((http_parameter *)param);
}
END_TEST


START_TEST(test_parse_parameter_no_equals)
{
    const char *input = "charset";
    const char *pos = input;
    http__item *param = NULL;
    nserror error;

    error = http__parse_parameter(&pos, &param);
    ck_assert_int_eq(error, NSERROR_NOT_FOUND);
    ck_assert_ptr_null(param);
}
END_TEST

START_TEST(test_parse_parameter_no_value)
{
    const char *input = "charset=";
    const char *pos = input;
    http__item *param = NULL;
    nserror error;

    error = http__parse_parameter(&pos, &param);
    /* Should fail to parse token / quoted string */
    ck_assert_int_ne(error, NSERROR_OK);
    ck_assert_ptr_null(param);
}
END_TEST

START_TEST(test_parameter_list_find)
{
    const char *input1 = "charset=utf-8";
    const char *pos = input1;
    http__item *param1 = NULL;
    nserror error;

    error = http__parse_parameter(&pos, &param1);
    ck_assert_int_eq(error, NSERROR_OK);

    const char *input2 = "filename=\"test.txt\"";
    pos = input2;
    http__item *param2 = NULL;

    error = http__parse_parameter(&pos, &param2);
    ck_assert_int_eq(error, NSERROR_OK);

    param1->next = param2;

    lwc_string *key;
    lwc_string *val;

    lwc_intern_string("charset", 7, &key);
    error = http_parameter_list_find_item((http_parameter *)param1, key, &val);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_int_eq(lwc_string_length(val), 5);
    ck_assert_int_eq(strncmp(lwc_string_data(val), "utf-8", 5), 0);
    lwc_string_unref(key);
    lwc_string_unref(val);

    lwc_intern_string("filename", 8, &key);
    error = http_parameter_list_find_item((http_parameter *)param1, key, &val);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_int_eq(lwc_string_length(val), 8);
    ck_assert_int_eq(strncmp(lwc_string_data(val), "test.txt", 8), 0);
    lwc_string_unref(key);
    lwc_string_unref(val);

    lwc_intern_string("notfound", 8, &key);
    error = http_parameter_list_find_item((http_parameter *)param1, key, &val);
    ck_assert_int_eq(error, NSERROR_NOT_FOUND);
    lwc_string_unref(key);

    http_parameter_list_destroy((http_parameter *)param1);
}
END_TEST


static Suite *test_suite(void)
{
    Suite *s = suite_create("http-parameter");
    TCase *tc_core = tcase_create("Core");

    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_parse_parameter_happy);
    tcase_add_test(tc_core, test_parse_parameter_quoted);
    tcase_add_test(tc_core, test_parse_parameter_spaces);
    tcase_add_test(tc_core, test_parse_parameter_no_equals);
    tcase_add_test(tc_core, test_parse_parameter_no_value);
    tcase_add_test(tc_core, test_parameter_list_find);
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
