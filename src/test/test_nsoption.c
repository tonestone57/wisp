#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wisp/utils/nsoption.h"
#include "wisp/utils/errors.h"

static int setup_called = 0;

static void setup(void) {
    /* Initialize global options by passing NULL for popts and pdefs */
    nsoption_init(NULL, NULL, NULL);
    setup_called = 1;
}

static void teardown(void) {
    nsoption_finalise(NULL, NULL);
}

START_TEST(test_nsoption_init_finalise)
{
    ck_assert_int_eq(setup_called, 1);
    ck_assert_ptr_ne(nsoptions, NULL);
    ck_assert_ptr_ne(nsoptions_default, NULL);
}
END_TEST

START_TEST(test_nsoption_set_get_bool)
{
    nsoption_set_bool(http_proxy, true);
    ck_assert_int_eq(nsoption_bool(http_proxy), true);

    nsoption_set_bool(http_proxy, false);
    ck_assert_int_eq(nsoption_bool(http_proxy), false);
}
END_TEST

START_TEST(test_nsoption_set_get_int)
{
    nsoption_set_int(http_proxy_port, 8081);
    ck_assert_int_eq(nsoption_int(http_proxy_port), 8081);
}
END_TEST

START_TEST(test_nsoption_set_get_charp)
{
    nsoption_set_charp(http_proxy_host, strdup("localhost"));
    ck_assert_str_eq(nsoption_charp(http_proxy_host), "localhost");
}
END_TEST

static TCase *nsoption_case_create(void)
{
    TCase *tc;
    tc = tcase_create("Nsoption");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, test_nsoption_init_finalise);
    tcase_add_test(tc, test_nsoption_set_get_bool);
    tcase_add_test(tc, test_nsoption_set_get_int);
    tcase_add_test(tc, test_nsoption_set_get_charp);
    return tc;
}

static Suite *nsoption_suite_create(void)
{
    Suite *s;
    s = suite_create("Nsoption Utils");
    suite_add_tcase(s, nsoption_case_create());
    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    SRunner *sr;
    sr = srunner_create(nsoption_suite_create());
    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
