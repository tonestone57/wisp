#include <check.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#include "utils/filename.h"
#include "utils/file.h"
#include "desktop/gui_table.h"
#include "utils/log.h"
#include "utils/utils.h"

extern struct wisp_table *guit;
bool verbose_log = true;

static void setup_mock_gui(void) {
    static struct gui_file_table file_ops;
    file_ops.mkdir_all = default_file_table->mkdir_all;
    file_ops.mkpath = default_file_table->mkpath;

    static struct wisp_table gui;
    gui.file = &file_ops;

    guit = &gui;
}

static void teardown_mock_gui(void) {
    /* Clean up the directory created by our test */
    wisp_recursive_rm("/tmp/WWW");
    guit = NULL;
}

START_TEST(filename_initialise_test)
{
    setup_mock_gui();

    system("mkdir -p " TEMP_FILENAME_PREFIX);

    filename_initialise();

    struct stat sb;
    ck_assert_int_eq(stat(TEMP_FILENAME_PREFIX, &sb), 0);
    ck_assert(S_ISDIR(sb.st_mode));

    teardown_mock_gui();
}
END_TEST

START_TEST(filename_request_test)
{
    setup_mock_gui();

    system("mkdir -p " TEMP_FILENAME_PREFIX);
    filename_initialise();

    const char *name1 = filename_request();
    ck_assert_ptr_nonnull(name1);
    char buf1[256];
    strncpy(buf1, name1, sizeof(buf1));

    const char *name2 = filename_request();
    ck_assert_ptr_nonnull(name2);
    char buf2[256];
    strncpy(buf2, name2, sizeof(buf2));

    ck_assert_str_ne(buf1, buf2);

    teardown_mock_gui();
}
END_TEST

START_TEST(filename_claim_release_test)
{
    setup_mock_gui();

    system("mkdir -p " TEMP_FILENAME_PREFIX);
    filename_initialise();

    ck_assert_int_eq(filename_claim(NULL), false);
    ck_assert_int_eq(filename_claim("short"), false);

    /* Test valid format '01/23/45/XX' */
    ck_assert_int_eq(filename_claim("01/23/45/00"), true);
    ck_assert_int_eq(filename_claim("01/23/45/00"), false); /* Cannot claim twice */

    filename_release("01/23/45/00");
    ck_assert_int_eq(filename_claim("01/23/45/00"), true); /* Can claim after release */

    teardown_mock_gui();
}
END_TEST

START_TEST(filename_flush_test)
{
    setup_mock_gui();

    system("mkdir -p " TEMP_FILENAME_PREFIX);
    filename_initialise();

    filename_flush(); /* Shouldn't crash */

    teardown_mock_gui();
}
END_TEST

static Suite *filename_suite_create(void)
{
    Suite *s;
    TCase *tc;

    s = suite_create("Filename");
    tc = tcase_create("Operations");

    tcase_add_test(tc, filename_initialise_test);
    tcase_add_test(tc, filename_request_test);
    tcase_add_test(tc, filename_claim_release_test);
    tcase_add_test(tc, filename_flush_test);

    suite_add_tcase(s, tc);

    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    SRunner *sr;

    sr = srunner_create(filename_suite_create());

    srunner_run_all(sr, CK_ENV);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
