#include <check.h>
#include <wisp/utils/log.h>
#include <stdio.h>
#include <stdbool.h>
#include <wisp/utils/nsoption.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool test_ensure(FILE *fptr) {
    if (fptr == NULL) return false;
    return true;
}

static bool failing_ensure(FILE *fptr) {
    return false;
}

START_TEST(test_nslog_init_basic)
{
    int argc = 1;
    char *argv[] = {"wisp", NULL};
    nserror err;

    err = nslog_init(NULL, &argc, argv);
    ck_assert_int_eq(err, NSERROR_OK);

    nslog_finalise();
}
END_TEST

START_TEST(test_nslog_init_verbose)
{
    int argc = 2;
    char *argv[] = {"wisp", "-v", NULL};
    nserror err;

    err = nslog_init(NULL, &argc, argv);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(argc, 1);
    ck_assert_str_eq(argv[0], "wisp");
    ck_assert(verbose_log);

    nslog_finalise();
}
END_TEST

START_TEST(test_nslog_init_verbose_file)
{
    int argc = 3;
    char *argv[] = {"wisp", "-V", "test_log_output.txt", NULL};
    nserror err;

    err = nslog_init(test_ensure, &argc, argv);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(argc, 1);
    ck_assert_str_eq(argv[0], "wisp");
    ck_assert(verbose_log);

    nslog_finalise();
    unlink("test_log_output.txt");
}
END_TEST

START_TEST(test_nslog_init_verbose_file_ensure_fail)
{
    int argc = 3;
    char *argv[] = {"wisp", "-V", "test_log_output_2.txt", NULL};
    nserror err;

    err = nslog_init(failing_ensure, &argc, argv);
    ck_assert_int_eq(err, NSERROR_INIT_FAILED);
    ck_assert_int_eq(argc, 1);
    ck_assert(!verbose_log);

    nslog_finalise();
    unlink("test_log_output_2.txt");
}
END_TEST

START_TEST(test_nslog_init_split_logs)
{
    int argc = 2;
    char *argv[] = {"wisp", "-split-logs", NULL};
    nserror err;

    err = nslog_init(NULL, &argc, argv);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(argc, 1);
    ck_assert_str_eq(argv[0], "wisp");

    nslog_finalise();

    // Check if wisp-logs directory was created and contains files
    ck_assert_int_eq(access("wisp-logs/ns-info.txt", F_OK), 0);
    system("rm -rf wisp-logs");
}
END_TEST

START_TEST(test_nslog_log_macro)
{
    int argc = 2;
    char *argv[] = {"wisp", "-v", NULL};
    nserror err;

    err = nslog_init(NULL, &argc, argv);
    ck_assert_int_eq(err, NSERROR_OK);

    // This won't do much if WITH_NSLOG is used (as wisp_render_log outputs it),
    // but tests it doesn't crash
    NSLOG(wisp, INFO, "Test log message %d", 42);

    nslog_finalise();
}
END_TEST


START_TEST(test_nslog_set_filter)
{
    nserror err;

    // Test with a valid filter
    err = nslog_set_filter("level~WARNING");
    ck_assert_int_eq(err, NSERROR_OK);

    // Test with another filter
    err = nslog_set_filter("level~DEBUG");
    ck_assert_int_eq(err, NSERROR_OK);
}
END_TEST

START_TEST(test_nslog_set_filter_by_options)
{
    nserror err;
    int argc = 1;
    char *argv[] = {"wisp", NULL};

    err = nsoption_init(NULL, NULL, NULL);
    ck_assert_int_eq(err, NSERROR_OK);

    err = nslog_init(NULL, &argc, argv);
    ck_assert_int_eq(err, NSERROR_OK);

    err = nslog_set_filter_by_options();
    ck_assert_int_eq(err, NSERROR_OK);

    nslog_finalise();
    nsoption_finalise(NULL, NULL);
}
END_TEST

Suite *log_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("log");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_nslog_init_basic);
    tcase_add_test(tc_core, test_nslog_init_verbose);
    tcase_add_test(tc_core, test_nslog_init_verbose_file);
    tcase_add_test(tc_core, test_nslog_init_verbose_file_ensure_fail);
    tcase_add_test(tc_core, test_nslog_init_split_logs);
    tcase_add_test(tc_core, test_nslog_log_macro);
    tcase_add_test(tc_core, test_nslog_set_filter);
    tcase_add_test(tc_core, test_nslog_set_filter_by_options);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = log_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
