#include <check.h>
#ifdef WISP_DISABLE_LOGGING
#undef WISP_DISABLE_LOGGING
#endif
#define WISP_DISABLE_LOGGING 0
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

static void cleanup_split_logs(void) {
    const char *files[] = {
        "wisp-logs/ns-deepdebug.txt",
        "wisp-logs/ns-debug.txt",
        "wisp-logs/ns-verbose.txt",
        "wisp-logs/ns-info.txt",
        "wisp-logs/ns-warning.txt",
        "wisp-logs/ns-error.txt",
        "wisp-logs/ns-critical.txt",
    };
    for (int i = 0; i < 7; i++) {
        remove(files[i]);
    }
    remove("wisp-logs");
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


START_TEST(test_nslog_init_invalid_file)
{
    int argc = 3;
    char *argv[] = {"wisp", "-V", "/tmp/nonexistent1/nonexistent2/log.txt", NULL};
    nserror err;

    err = nslog_init(NULL, &argc, argv);
    ck_assert_int_eq(err, NSERROR_NOT_FOUND);
    ck_assert_int_eq(argc, 1);
    ck_assert(!verbose_log);

    nslog_finalise();
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
    cleanup_split_logs();
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



START_TEST(test_nslog_log_output)
{
    char file_path[] = "/tmp/ns_test_XXXXXX";
    int fd = mkstemp(file_path);
    ck_assert_int_ge(fd, 0);
    close(fd);

    int argc = 3;
    char *argv[] = {"wisp", "-V", file_path, NULL};
    nserror err;

    err = nslog_init(test_ensure, &argc, argv);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert(verbose_log);

    NSLOG(wisp, INFO, "Test output %s", "working");

    nslog_finalise();

    FILE *f = fopen(file_path, "r");
    ck_assert_ptr_nonnull(f);

    /* Ensure log output is flushed from stdio buffer */
    fflush(NULL);

    char buffer[1024] = {0};
    size_t read_bytes = fread(buffer, 1, sizeof(buffer) - 1, f);
    ck_assert_int_gt(read_bytes, 0);

    ck_assert_ptr_nonnull(strstr(buffer, "Test output working"));

    fclose(f);
    unlink(file_path);
}
END_TEST

START_TEST(test_nslog_log_output_split)
{
    int argc = 2;
    char *argv[] = {"wisp", "-split-logs", NULL};
    nserror err;

    err = nslog_init(NULL, &argc, argv);
    ck_assert_int_eq(err, NSERROR_OK);

    NSLOG(wisp, INFO, "Test output split %s", "working");

    nslog_finalise();

    FILE *f = fopen("wisp-logs/ns-info.txt", "r");
    ck_assert_ptr_nonnull(f);

    /* Ensure log output is flushed from stdio buffer */
    fflush(NULL);

    char buffer[1024] = {0};
    size_t read_bytes = fread(buffer, 1, sizeof(buffer) - 1, f);
    ck_assert_int_gt(read_bytes, 0);

    ck_assert_ptr_nonnull(strstr(buffer, "Test output split working"));

    fclose(f);
    cleanup_split_logs();
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

    // Test error conditions
#ifdef WITH_NSLOG
    err = nslog_set_filter("invalid_syntax!");
    ck_assert_int_eq(err, NSERROR_INVALID);
#else
    err = nslog_set_filter("invalid_syntax!");
    ck_assert_int_eq(err, NSERROR_OK);
#endif
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



START_TEST(test_nslog_init_verbose_file_invalid)
{
    int argc = 3;
    char *argv[] = {"wisp", "-V", "/invalid/path/that/cannot/be/written.txt", NULL};
    nserror err;

    err = nslog_init(NULL, &argc, argv);
    ck_assert_int_eq(err, NSERROR_NOT_FOUND);
    ck_assert(!verbose_log);

    // Ensure we can still log without a crash due to stderr fallback
    NSLOG(wisp, INFO, "Test fallback logging %s", "working");

    nslog_finalise();
}
END_TEST

START_TEST(test_nslog_init_args_shift)
{
    int argc = 4;
    char *argv[] = {"wisp", "-v", "extra_arg", "another_arg", NULL};
    nserror err;

    err = nslog_init(NULL, &argc, argv);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(argc, 3);
    ck_assert_str_eq(argv[0], "wisp");
    ck_assert_str_eq(argv[1], "extra_arg");
    ck_assert_str_eq(argv[2], "another_arg");

    nslog_finalise();
}
END_TEST

START_TEST(test_nslog_init_args_shift_V)
{
    int argc = 5;
    char *argv[] = {"wisp", "-V", "test_log_output.txt", "extra_arg", "another_arg", NULL};
    nserror err;

    err = nslog_init(NULL, &argc, argv);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(argc, 3);
    ck_assert_str_eq(argv[0], "wisp");
    ck_assert_str_eq(argv[1], "extra_arg");
    ck_assert_str_eq(argv[2], "another_arg");

    nslog_finalise();
    unlink("test_log_output.txt");
}
END_TEST

START_TEST(test_nslog_init_args_shift_split)
{
    int argc = 4;
    char *argv[] = {"wisp", "-split-logs", "extra_arg", "another_arg", NULL};
    nserror err;

    err = nslog_init(NULL, &argc, argv);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(argc, 3);
    ck_assert_str_eq(argv[0], "wisp");
    ck_assert_str_eq(argv[1], "extra_arg");
    ck_assert_str_eq(argv[2], "another_arg");

    nslog_finalise();
    cleanup_split_logs();
}
END_TEST

START_TEST(test_nslog_set_filter_by_options_verbose)
{
    nserror err;
    int argc = 2;
    char *argv[] = {"wisp", "-v", NULL};

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
    tcase_add_test(tc_core, test_nslog_init_invalid_file);
    tcase_add_test(tc_core, test_nslog_init_verbose_file_ensure_fail);
    tcase_add_test(tc_core, test_nslog_init_split_logs);
    tcase_add_test(tc_core, test_nslog_log_macro);
    tcase_add_test(tc_core, test_nslog_log_output);
    tcase_add_test(tc_core, test_nslog_log_output_split);

    tcase_add_test(tc_core, test_nslog_set_filter);
    tcase_add_test(tc_core, test_nslog_set_filter_by_options);
    tcase_add_test(tc_core, test_nslog_set_filter_by_options_verbose);
    tcase_add_test(tc_core, test_nslog_init_verbose_file_invalid);
    tcase_add_test(tc_core, test_nslog_init_args_shift);
    tcase_add_test(tc_core, test_nslog_init_args_shift_V);
    tcase_add_test(tc_core, test_nslog_init_args_shift_split);


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
