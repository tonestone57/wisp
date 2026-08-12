#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "utils/http/primitives.h"
#include <wisp/ns_inttypes.h>
#include "utils/corestrings.h"
#include "test/log.c"

START_TEST(test_http_skip_LWS)
{
    const char *input = "   \t  hello";
    const char *pos = input;

    http__skip_LWS(&pos);

    ck_assert_str_eq(pos, "hello");

    input = "hello";
    pos = input;
    http__skip_LWS(&pos);
    ck_assert_str_eq(pos, "hello");

    input = "";
    pos = input;
    http__skip_LWS(&pos);
    ck_assert_str_eq(pos, "");

    input = " \t ";
    pos = input;
    http__skip_LWS(&pos);
    ck_assert_str_eq(pos, "");
}
END_TEST

START_TEST(test_http_parse_token)
{
    const char *input = "token123, next";
    const char *pos = input;
    lwc_string *value = NULL;
    nserror err;

    err = http__parse_token(&pos, &value);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_ne(value, NULL);

    size_t len = lwc_string_length(value);
    const char *data = lwc_string_data(value);
    ck_assert_int_eq(len, 8);
    ck_assert_int_eq(strncmp(data, "token123", 8), 0);

    lwc_string_unref(value);
    ck_assert_str_eq(pos, ", next");

    // Invalid token start
    input = ",token";
    pos = input;
    err = http__parse_token(&pos, &value);
    ck_assert_int_eq(err, NSERROR_NOT_FOUND);
    ck_assert_str_eq(pos, ",token");
}
END_TEST

START_TEST(test_http_parse_quoted_string)
{
    const char *input = "\"quoted string\" next";
    const char *pos = input;
    lwc_string *value = NULL;
    nserror err;

    err = http__parse_quoted_string(&pos, &value);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_ne(value, NULL);

    size_t len = lwc_string_length(value);
    const char *data = lwc_string_data(value);
    ck_assert_int_eq(len, 13);
    ck_assert_int_eq(strncmp(data, "quoted string", 13), 0);

    lwc_string_unref(value);
    ck_assert_str_eq(pos, " next");

    // With escapes
    input = "\"escaped \\\" string\" next";
    pos = input;
    err = http__parse_quoted_string(&pos, &value);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_ne(value, NULL);

    len = lwc_string_length(value);
    data = lwc_string_data(value);
    ck_assert_int_eq(len, 16);
    ck_assert_int_eq(strncmp(data, "escaped \" string", 16), 0);

    lwc_string_unref(value);
    ck_assert_str_eq(pos, " next");

    // Invalid start
    input = "not quoted";
    pos = input;
    err = http__parse_quoted_string(&pos, &value);
    ck_assert_int_eq(err, NSERROR_NOT_FOUND);
    ck_assert_str_eq(pos, "not quoted");

    // Unclosed quote
    input = "\"unclosed string";
    pos = input;
    err = http__parse_quoted_string(&pos, &value);
    ck_assert_int_eq(err, NSERROR_NOT_FOUND);
}
END_TEST

static Suite *http_primitives_suite(void)
{
    Suite *s = suite_create("http_primitives");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_http_skip_LWS);
    tcase_add_test(tc_core, test_http_parse_token);
    tcase_add_test(tc_core, test_http_parse_quoted_string);

    suite_add_tcase(s, tc_core);
    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    if (argc != 1) {
        printf("Usage: %s\n", argv[0]);
        return EXIT_FAILURE;
    }

    corestrings_init();

    s = http_primitives_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    corestrings_fini();

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
