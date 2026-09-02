#include <check.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>

#include "wisp/utils/file.h"
#include "wisp/utils/errors.h"
#include "wisp/utils/corestrings.h"
#include "wisp/utils/nsurl.h"
#include "desktop/gui_table.h"
#include "windows/file.h"

extern struct wisp_table *guit;

START_TEST(wisp_mkpath_test)
{
    char *path = NULL;
    nserror err;

    struct gui_file_table file_ops = {
        .mkpath = default_file_table->mkpath,
    };
    struct wisp_table gui = {
        .file = &file_ops,
    };
    guit = &gui;

    err = wisp_mkpath(&path, NULL, 2, "foo", "bar");
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(path, "foo/bar");

    free(path);
    guit = NULL;
}
END_TEST

START_TEST(wisp_mkpath_three_args_test)
{
    char *path = NULL;
    nserror err;

    struct gui_file_table file_ops = {
        .mkpath = default_file_table->mkpath,
    };
    struct wisp_table gui = {
        .file = &file_ops,
    };
    guit = &gui;

    err = wisp_mkpath(&path, NULL, 3, "foo", "bar", "baz");
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(path, "foo/bar/baz");

    free(path);
    guit = NULL;
}
END_TEST

START_TEST(wisp_mkdir_all_test)
{
    nserror err;
    struct stat sb;
    struct gui_file_table file_ops = {
        .mkdir_all = default_file_table->mkdir_all,
    };
    struct wisp_table gui = {
        .file = &file_ops,
    };
    guit = &gui;

    /* Test 1: No separators (just a filename) */
    err = wisp_mkdir_all("filename.txt");
    ck_assert_int_eq(err, NSERROR_OK);

    /* Test 2: File exists where directory should be */
    char path_template[] = "/tmp/ns_mkdir_XXXXXX";
    int fd = mkstemp(path_template);
    ck_assert_int_ge(fd, 0);
    close(fd);
    char test_file_path[256];
    snprintf(test_file_path, sizeof(test_file_path), "%s/file.txt", path_template);
    err = wisp_mkdir_all(test_file_path);
    ck_assert_int_eq(err, NSERROR_NOT_DIRECTORY);
    unlink(path_template);

    /* Test 3: Nested directories creation */
    char dir_template[] = "/tmp/ns_mkdir_dir_XXXXXX";
    char *tmp_dir = mkdtemp(dir_template);
    ck_assert_ptr_nonnull(tmp_dir);
    snprintf(test_file_path, sizeof(test_file_path), "%s/dir1/dir2/file.txt", tmp_dir);
    err = wisp_mkdir_all(test_file_path);
    ck_assert_int_eq(err, NSERROR_OK);
    char check_dir[256];
    snprintf(check_dir, sizeof(check_dir), "%s/dir1/dir2", tmp_dir);
    ck_assert_int_eq(stat(check_dir, &sb), 0);
    ck_assert(S_ISDIR(sb.st_mode));
    rmdir(check_dir);
    snprintf(check_dir, sizeof(check_dir), "%s/dir1", tmp_dir);
    rmdir(check_dir);
    rmdir(tmp_dir);

    /* Test 4: Existing directory */
    char dir_template2[] = "/tmp/ns_mkdir_dir2_XXXXXX";
    char *tmp_dir2 = mkdtemp(dir_template2);
    ck_assert_ptr_nonnull(tmp_dir2);
    snprintf(test_file_path, sizeof(test_file_path), "%s/file.txt", tmp_dir2);
    err = wisp_mkdir_all(test_file_path);
    ck_assert_int_eq(err, NSERROR_OK);
    rmdir(tmp_dir2);

    /* Test 5: Grandparent is file */
    char file_template[] = "/tmp/ns_mkdir_file_XXXXXX";
    fd = mkstemp(file_template);
    ck_assert_int_ge(fd, 0);
    close(fd);
    snprintf(test_file_path, sizeof(test_file_path), "%s/dir1/file.txt", file_template);
    err = wisp_mkdir_all(test_file_path);
    ck_assert_int_eq(err, NSERROR_NOT_DIRECTORY);
    unlink(file_template);

    /* Test 6: Multiple slashes */
    char dir_template3[] = "/tmp/ns_mkdir_dir3_XXXXXX";
    char *tmp_dir3 = mkdtemp(dir_template3);
    ck_assert_ptr_nonnull(tmp_dir3);
    snprintf(test_file_path, sizeof(test_file_path), "%s//dir1//file.txt", tmp_dir3);
    err = wisp_mkdir_all(test_file_path);
    ck_assert_int_eq(err, NSERROR_OK);
    snprintf(check_dir, sizeof(check_dir), "%s/dir1", tmp_dir3);
    ck_assert_int_eq(stat(check_dir, &sb), 0);
    ck_assert(S_ISDIR(sb.st_mode));
    rmdir(check_dir);
    rmdir(tmp_dir3);

    guit = NULL;
}
END_TEST


START_TEST(wisp_recursive_rm_test)
{
    nserror err;
    struct stat sb;

    /* Setup mock GUI table for wisp_mkpath */
    struct gui_file_table file_ops = {
        .mkpath = default_file_table->mkpath,
    };
    struct wisp_table gui = {
        .file = &file_ops,
    };
    guit = &gui;

    /* Test 1: Non-existent path */
    err = wisp_recursive_rm("/tmp/nonexistent1/nonexistent2/rm_test");
    ck_assert_int_eq(err, NSERROR_NOT_FOUND);

    /* Test 2: Normal empty directory */
    char dir_template[] = "/tmp/ns_rm_empty_XXXXXX";
    char *tmp_dir = mkdtemp(dir_template);
    ck_assert_ptr_nonnull(tmp_dir);
    err = wisp_recursive_rm(tmp_dir);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(stat(tmp_dir, &sb), -1);

    /* Test 3: Nested directories and files */
    char dir_template2[] = "/tmp/ns_rm_nested_XXXXXX";
    char *tmp_dir2 = mkdtemp(dir_template2);
    ck_assert_ptr_nonnull(tmp_dir2);

    char nested_dir[256];
    snprintf(nested_dir, sizeof(nested_dir), "%s/subdir", tmp_dir2);
    mkdir(nested_dir, 0755);

    char file1[256];
    snprintf(file1, sizeof(file1), "%s/file1.txt", tmp_dir2);
    FILE *f1 = fopen(file1, "w");
    if (f1) fclose(f1);

    char file2[256];
    snprintf(file2, sizeof(file2), "%s/subdir/file2.txt", tmp_dir2);
    FILE *f2 = fopen(file2, "w");
    if (f2) fclose(f2);

    err = wisp_recursive_rm(tmp_dir2);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(stat(tmp_dir2, &sb), -1);

    /* Test 4: Attempting to delete a regular file */
    char file_template[] = "/tmp/ns_rm_file_XXXXXX";
    int fd = mkstemp(file_template);
    ck_assert_int_ge(fd, 0);
    close(fd);
    err = wisp_recursive_rm(file_template);
    /* Attempting to opendir a file returns ENOTDIR, leading to NSERROR_UNKNOWN in wisp_recursive_rm */
    ck_assert_int_eq(err, NSERROR_UNKNOWN);
    unlink(file_template);

    guit = NULL;
}
END_TEST

START_TEST(win32_nsurl_to_path_test)
{
    nserror err;
    nsurl *url = NULL;
    char *path = NULL;

    ck_assert_int_eq(corestrings_init(), NSERROR_OK);

    /* Test 1: Drive letter path */
    err = nsurl_create("file:///C:/path/file.txt", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    err = win32_file_table->nsurl_to_path(url, &path);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(path, "C:\\path\\file.txt");
    free(path);
    path = NULL;
    nsurl_unref(url);
    url = NULL;

    /* Test 2: Pipe drive letter path */
    err = nsurl_create("file:///C|/path/file.txt", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    err = win32_file_table->nsurl_to_path(url, &path);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(path, "C:\\path\\file.txt");
    free(path);
    path = NULL;
    nsurl_unref(url);
    url = NULL;

    /* Test 3: Path without drive letter */
    err = nsurl_create("file:///path/file.txt", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    err = win32_file_table->nsurl_to_path(url, &path);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(path, "\\path\\file.txt");
    free(path);
    path = NULL;
    nsurl_unref(url);
    url = NULL;

    /* Test 4: UNC path without drive letter */
    err = nsurl_create("file:////server/share/file.txt", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    err = win32_file_table->nsurl_to_path(url, &path);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(path, "\\\\server\\share\\file.txt");
    free(path);
    path = NULL;
    nsurl_unref(url);
    url = NULL;

    /* Test 5: Short path without drive letter */
    err = nsurl_create("file:///a", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    err = win32_file_table->nsurl_to_path(url, &path);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(path, "\\a");
    free(path);
    path = NULL;
    nsurl_unref(url);
    url = NULL;

    corestrings_fini();
}
END_TEST

static Suite *file_suite_create(void)
{
    Suite *s;
    TCase *tc;
    TCase *tc_mkdir;

    s = suite_create("File");
    tc = tcase_create("Paths");

    tcase_add_test(tc, wisp_mkpath_test);
    tcase_add_test(tc, wisp_mkpath_three_args_test);

    suite_add_tcase(s, tc);

    tc_mkdir = tcase_create("MkdirAll");
    tcase_add_test(tc_mkdir, wisp_mkdir_all_test);
    suite_add_tcase(s, tc_mkdir);

    TCase *tc_rm = tcase_create("RecursiveRm");
    tcase_add_test(tc_rm, wisp_recursive_rm_test);
    suite_add_tcase(s, tc_rm);

    TCase *tc_win32 = tcase_create("Win32Path");
    tcase_add_test(tc_win32, win32_nsurl_to_path_test);
    suite_add_tcase(s, tc_win32);

    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    SRunner *sr;

    sr = srunner_create(file_suite_create());

    srunner_run_all(sr, CK_ENV);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
