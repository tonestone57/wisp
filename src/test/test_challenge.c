#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "utils/corestrings.h"
#include "utils/http/challenge.h"
#include "utils/http/challenge_internal.h"
#include "utils/errors.h"

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
    http_challenge *challenge = NULL;
    nserror error;
    lwc_string *scheme = NULL;
    http_parameter *params = NULL;
    lwc_string *name = NULL;
    lwc_string *val = NULL;

    /* Test 1: "Basic realm=\"WallyWorld\"" */
    const char *input1 = "Basic realm=\"WallyWorld\"";
    const char *pos = input1;
    error = http__parse_challenge(&pos, &challenge);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(challenge);

    const http_challenge *cur = http_challenge_list_iterate(challenge, &scheme, &params);
    ck_assert_ptr_null(cur); /* only one challenge expected */
    ck_assert_ptr_nonnull(scheme);
    ck_assert_int_eq(lwc_string_length(scheme), 5);
    ck_assert_int_eq(strncmp(lwc_string_data(scheme), "Basic", 5), 0);
    lwc_string_unref(scheme);

    ck_assert_ptr_nonnull(params);
    const http_parameter *pcur = http_parameter_list_iterate(params, &name, &val);
    ck_assert_ptr_null(pcur);
    ck_assert_ptr_nonnull(name);
    ck_assert_ptr_nonnull(val);
    ck_assert_int_eq(strncmp(lwc_string_data(name), "realm", 5), 0);
    ck_assert_int_eq(strncmp(lwc_string_data(val), "WallyWorld", 10), 0);
    lwc_string_unref(name);
    lwc_string_unref(val);

    http_challenge_list_destroy(challenge);

    /* Test 2: Multiple parameters */
    const char *input2 = "Digest realm=\"testrealm@host.com\", qop=\"auth,auth-int\", nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\"";
    pos = input2;
    error = http__parse_challenge(&pos, &challenge);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(challenge);

    cur = http_challenge_list_iterate(challenge, &scheme, &params);
    ck_assert_ptr_nonnull(scheme);
    ck_assert_int_eq(strncmp(lwc_string_data(scheme), "Digest", 6), 0);
    lwc_string_unref(scheme);

    ck_assert_ptr_nonnull(params);
    pcur = http_parameter_list_iterate(params, &name, &val);
    ck_assert_ptr_nonnull(pcur);

    /* order of parameters is not guaranteed since it's a linked list parsed left to right or right to left? */
    /* actually it's left to right */

    /* realm */
    if (strncmp(lwc_string_data(name), "realm", 5) == 0) {
        ck_assert_int_eq(strncmp(lwc_string_data(val), "testrealm@host.com", 18), 0);
    } else if (strncmp(lwc_string_data(name), "qop", 3) == 0) {
        ck_assert_int_eq(strncmp(lwc_string_data(val), "auth,auth-int", 13), 0);
    } else if (strncmp(lwc_string_data(name), "nonce", 5) == 0) {
        ck_assert_int_eq(strncmp(lwc_string_data(val), "dcd98b7102dd2f0e8b11d0f600bfb0c093", 34), 0);
    } else {
        ck_abort_msg("Unexpected parameter name: %.*s", (int)lwc_string_length(name), lwc_string_data(name));
    }
    lwc_string_unref(name);
    lwc_string_unref(val);

    pcur = http_parameter_list_iterate(pcur, &name, &val);
    ck_assert_ptr_nonnull(pcur);
    if (strncmp(lwc_string_data(name), "realm", 5) == 0) {
        ck_assert_int_eq(strncmp(lwc_string_data(val), "testrealm@host.com", 18), 0);
    } else if (strncmp(lwc_string_data(name), "qop", 3) == 0) {
        ck_assert_int_eq(strncmp(lwc_string_data(val), "auth,auth-int", 13), 0);
    } else if (strncmp(lwc_string_data(name), "nonce", 5) == 0) {
        ck_assert_int_eq(strncmp(lwc_string_data(val), "dcd98b7102dd2f0e8b11d0f600bfb0c093", 34), 0);
    } else {
        ck_abort_msg("Unexpected parameter name: %.*s", (int)lwc_string_length(name), lwc_string_data(name));
    }
    lwc_string_unref(name);
    lwc_string_unref(val);

    pcur = http_parameter_list_iterate(pcur, &name, &val);
    ck_assert_ptr_null(pcur);
    if (strncmp(lwc_string_data(name), "realm", 5) == 0) {
        ck_assert_int_eq(strncmp(lwc_string_data(val), "testrealm@host.com", 18), 0);
    } else if (strncmp(lwc_string_data(name), "qop", 3) == 0) {
        ck_assert_int_eq(strncmp(lwc_string_data(val), "auth,auth-int", 13), 0);
    } else if (strncmp(lwc_string_data(name), "nonce", 5) == 0) {
        ck_assert_int_eq(strncmp(lwc_string_data(val), "dcd98b7102dd2f0e8b11d0f600bfb0c093", 34), 0);
    } else {
        ck_abort_msg("Unexpected parameter name: %.*s", (int)lwc_string_length(name), lwc_string_data(name));
    }
    lwc_string_unref(name);
    lwc_string_unref(val);

    http_challenge_list_destroy(challenge);
}
END_TEST

START_TEST(test_parse_edge_cases)
{
    http_challenge *challenge = NULL;
    nserror error;
    const char *pos;

    /* Test 1: Empty string */
    pos = "";
    error = http__parse_challenge(&pos, &challenge);
    ck_assert_int_ne(error, NSERROR_OK);

    /* Test 2: Only spaces */
    pos = "   ";
    error = http__parse_challenge(&pos, &challenge);
    ck_assert_int_ne(error, NSERROR_OK);

    /* Test 3: Missing space after scheme */
    pos = "BasicRealm";
    error = http__parse_challenge(&pos, &challenge);
    ck_assert_int_ne(error, NSERROR_OK);

    /* Test 4: Missing parameters */
    pos = "Basic ";
    error = http__parse_challenge(&pos, &challenge);
    ck_assert_int_ne(error, NSERROR_OK);
}
END_TEST

static Suite *challenge_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("challenge");

    tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_parse_happy_paths);
    tcase_add_test(tc_core, test_parse_edge_cases);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = challenge_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
