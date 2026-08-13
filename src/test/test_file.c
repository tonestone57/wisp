#include <check.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#include "wisp/utils/file.h"
#include "wisp/utils/errors.h"
#include "desktop/gui_table.h"

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

static Suite *file_suite_create(void)
{
    Suite *s;
    TCase *tc;

    s = suite_create("File");
    tc = tcase_create("Paths");

    tcase_add_test(tc, wisp_mkpath_test);
    tcase_add_test(tc, wisp_mkpath_three_args_test);

    suite_add_tcase(s, tc);

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
