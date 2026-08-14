#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utils/corestrings.h"
#include "utils/http/challenge.h"
#include "utils/http/challenge_internal.h"
#include "wisp/utils/errors.h"


static void setup(void)
{
    corestrings_init();
}

static void teardown(void)
{
    corestrings_fini();
}

START_TEST(test_parse_happy_path)
{
    http_challenge *challenge = NULL;
    nserror error;
    const char *input = "Basic realm=\"example\"";
    const char *pos = input;

    error = http__parse_challenge(&pos, &challenge);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(challenge);

    lwc_string *scheme = NULL;
    http_parameter *params = NULL;
    const http_challenge *next_cur;

    next_cur = http_challenge_list_iterate(challenge, &scheme, &params);
    ck_assert_ptr_nonnull(scheme);
    ck_assert_int_eq(lwc_string_length(scheme), 5);
    ck_assert_int_eq(strncmp(lwc_string_data(scheme), "Basic", 5), 0);
    lwc_string_unref(scheme);

    ck_assert_ptr_nonnull(params);

    lwc_string *key;
    lwc_string *val;
    lwc_intern_string("realm", 5, &key);
    error = http_parameter_list_find_item(params, key, &val);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(val);
    ck_assert_int_eq(lwc_string_length(val), 7);
    ck_assert_int_eq(strncmp(lwc_string_data(val), "example", 7), 0);
    lwc_string_unref(key);
    lwc_string_unref(val);

    http_challenge_list_destroy(challenge);
}
END_TEST
START_TEST(test_parse_multiple_parameters)
{
    http_challenge *challenge = NULL;
    nserror error;
    const char *input = "Digest realm=\"testrealm\", qop=\"auth\"";
    const char *pos = input;

    error = http__parse_challenge(&pos, &challenge);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(challenge);

    lwc_string *scheme = NULL;
    http_parameter *params = NULL;

    http_challenge_list_iterate(challenge, &scheme, &params);
    ck_assert_ptr_nonnull(scheme);
    ck_assert_int_eq(lwc_string_length(scheme), 6);
    ck_assert_int_eq(strncmp(lwc_string_data(scheme), "Digest", 6), 0);
    lwc_string_unref(scheme);

    ck_assert_ptr_nonnull(params);

    lwc_string *key;
    lwc_string *val;

    /* Check for realm */
    lwc_intern_string("realm", 5, &key);
    error = http_parameter_list_find_item(params, key, &val);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(val);
    ck_assert_int_eq(lwc_string_length(val), 9);
    ck_assert_int_eq(strncmp(lwc_string_data(val), "testrealm", 9), 0);
    lwc_string_unref(key);
    lwc_string_unref(val);

    /* Check for qop */
    lwc_intern_string("qop", 3, &key);
    error = http_parameter_list_find_item(params, key, &val);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(val);
    ck_assert_int_eq(lwc_string_length(val), 4);
    ck_assert_int_eq(strncmp(lwc_string_data(val), "auth", 4), 0);
    lwc_string_unref(key);
    lwc_string_unref(val);

    http_challenge_list_destroy(challenge);
}
END_TEST
START_TEST(test_parse_edge_cases)
{
    http_challenge *challenge = NULL;
    nserror error;
    const char *input;
    const char *pos;

    /* Missing space after scheme */
    input = "Basicrealm=\"example\"";
    pos = input;
    error = http__parse_challenge(&pos, &challenge);
    ck_assert_int_eq(error, NSERROR_NOT_FOUND);
    ck_assert_ptr_null(challenge);

    /* Empty string */
    input = "";
    pos = input;
    error = http__parse_challenge(&pos, &challenge);
    ck_assert_int_ne(error, NSERROR_OK);
    ck_assert_ptr_null(challenge);

    /* Only spaces */
    input = "   ";
    pos = input;
    error = http__parse_challenge(&pos, &challenge);
    ck_assert_int_ne(error, NSERROR_OK);
    ck_assert_ptr_null(challenge);

    /* Malformed parameter */
    input = "Basic realm=";
    pos = input;
    error = http__parse_challenge(&pos, &challenge);
    ck_assert_int_ne(error, NSERROR_OK);
}
END_TEST



static Suite *test_suite(void)
{
    Suite *s = suite_create("http-challenge");
    TCase *tc_core = tcase_create("Core");

    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_parse_happy_path);
    tcase_add_test(tc_core, test_parse_multiple_parameters);
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
