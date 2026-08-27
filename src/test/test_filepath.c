#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>

#include <wisp/utils/filepath.h>

START_TEST(filepath_free_strvec_test)
{
    char **strvec;

    strvec = calloc(4, sizeof(char *));
    ck_assert_ptr_ne(strvec, NULL);

    strvec[0] = strdup("/usr/share/wisp");
    strvec[1] = strdup("/etc/wisp");
    strvec[2] = strdup("~/.wisp");
    strvec[3] = NULL;

    filepath_free_strvec(strvec);

    /* We can't strictly check for memory leaks here but ASAN will catch it
     * and we ensure no segmentation fault occurred. */
}
END_TEST

START_TEST(filepath_free_strvec_empty_test)
{
    char **strvec;

    strvec = calloc(1, sizeof(char *));
    ck_assert_ptr_ne(strvec, NULL);

    strvec[0] = NULL;

    filepath_free_strvec(strvec);
}
END_TEST

START_TEST(filepath_findfile_existing_test)
{
    char tmp_template[] = "/tmp/wisp_filepath_test_XXXXXX";
    int fd = mkstemp(tmp_template);
    ck_assert_int_ge(fd, 0);
    close(fd);

    char expected_realpath[PATH_MAX];
    ck_assert_ptr_ne(realpath(tmp_template, expected_realpath), NULL);

    /* Split filename and path to test variadic formatting */
    char *dir = strdup(tmp_template);
    char *last_slash = strrchr(dir, '/');
    ck_assert_ptr_ne(last_slash, NULL);
    *last_slash = '\0';
    const char *filename = last_slash + 1;

    char *result = filepath_findfile("%s/%s", dir, filename);
    ck_assert_ptr_ne(result, NULL);
    ck_assert_str_eq(result, expected_realpath);

    free(result);
    free(dir);
    unlink(tmp_template);
}
END_TEST

START_TEST(filepath_findfile_variadic_args_test)
{
    char expected_realpath[PATH_MAX];
    char path_buf[PATH_MAX];
    snprintf(path_buf, sizeof(path_buf), "/tmp/wisp_test_%d_%s.txt", (int)getpid(), "file");
    int fd = open(path_buf, O_CREAT | O_WRONLY, 0644);
    ck_assert_int_ge(fd, 0);
    close(fd);

    ck_assert_ptr_ne(realpath(path_buf, expected_realpath), NULL);

    char *result = filepath_findfile("%s/%s_%d_%s.%s", "/tmp", "wisp_test", (int)getpid(), "file", "txt");
    ck_assert_ptr_ne(result, NULL);
    ck_assert_str_eq(result, expected_realpath);

    free(result);
    unlink(path_buf);
}
END_TEST

START_TEST(filepath_findfile_nonexistent_test)
{
    char *result = filepath_findfile("%s/%s_%d.tmp", "/tmp", "non_existent_file_wisp_test", 99999);
    ck_assert_ptr_null(result);
}
END_TEST

START_TEST(filepath_findfile_unreadable_test)
{
    /* Root user bypasses file permission checks for R_OK, so skip test if running as root */
    if (geteuid() == 0) {
        return;
    }

    char tmp_template[] = "/tmp/wisp_filepath_unreadable_XXXXXX";
    int fd = mkstemp(tmp_template);
    ck_assert_int_ge(fd, 0);
    close(fd);

    /* Remove read permissions */
    ck_assert_int_eq(chmod(tmp_template, 0000), 0);

    char *result = filepath_findfile("%s", tmp_template);
    ck_assert_ptr_null(result);

    /* Restore permission for deletion */
    chmod(tmp_template, 0600);
    unlink(tmp_template);
}
END_TEST

static TCase *filepath_case_create(void)
{
    TCase *tc;
    tc = tcase_create("Filepath");

    tcase_add_test(tc, filepath_free_strvec_test);
    tcase_add_test(tc, filepath_free_strvec_empty_test);
    tcase_add_test(tc, filepath_findfile_existing_test);
    tcase_add_test(tc, filepath_findfile_variadic_args_test);
    tcase_add_test(tc, filepath_findfile_nonexistent_test);
    tcase_add_test(tc, filepath_findfile_unreadable_test);

    return tc;
}

static Suite *filepath_suite_create(void)
{
    Suite *s;
    s = suite_create("Filepath Utils");

    suite_add_tcase(s, filepath_case_create());

    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    SRunner *sr;

    sr = srunner_create(filepath_suite_create());

    srunner_run_all(sr, CK_ENV);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
