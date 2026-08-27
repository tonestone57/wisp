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
extern bool verbose_log;

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

    wisp_mkdir_all(TEMP_FILENAME_PREFIX "/dummy");

    filename_initialise();

    struct stat sb;
    ck_assert_int_eq(stat(TEMP_FILENAME_PREFIX, &sb), 0);
    ck_assert(S_ISDIR(sb.st_mode));

    teardown_mock_gui();
}
END_TEST

START_TEST(filename_request_rollover_test)
{
    setup_mock_gui();

    wisp_mkdir_all(TEMP_FILENAME_PREFIX "/dummy");
    filename_initialise();

    char first_dir_files[64][32];

    /* Fill all 64 slots in the first directory */
    for (int i = 0; i < 64; i++) {
        const char *req = filename_request();
        ck_assert_ptr_nonnull(req);
        strncpy(first_dir_files[i], req, sizeof(first_dir_files[i]));

        /* Verify all 64 files share the exact same 9-char directory prefix */
        ck_assert_int_eq(strncmp(first_dir_files[i], first_dir_files[0], 9), 0);
    }

    /* Request 65th file: should create a new directory prefix */
    const char *req65 = filename_request();
    ck_assert_ptr_nonnull(req65);
    char file65[32];
    strncpy(file65, req65, sizeof(file65));

    /* Verify file65 prefix is different from first directory */
    ck_assert_str_ne(file65, first_dir_files[0]);

    /* Test slot reuse: release slot 15 from first directory */
    filename_release(first_dir_files[15]);

    /* Next request should reuse slot 15 in first directory */
    const char *req_reused = filename_request();
    ck_assert_ptr_nonnull(req_reused);
    ck_assert_str_eq(req_reused, first_dir_files[15]);

    /* Test out-of-order claims for directory sorting */
    ck_assert_int_eq(filename_claim("00/00/05/00"), true);
    ck_assert_int_eq(filename_claim("00/00/03/00"), true);

    teardown_mock_gui();
}
END_TEST

START_TEST(filename_claim_release_boundary_test)
{
    setup_mock_gui();

    wisp_mkdir_all(TEMP_FILENAME_PREFIX "/dummy");
    filename_initialise();

    /* Input validation tests */
    ck_assert_int_eq(filename_claim(NULL), false);
    ck_assert_int_eq(filename_claim(""), false);
    ck_assert_int_eq(filename_claim("01/23/45"), false); /* 8 chars < 11 */

    /* Releasing invalid or non-existent filenames should be safe (no-op) */
    filename_release(NULL);
    filename_release("short");
    filename_release("99/99/99/00");

    /* Low bitfield boundary claims (slots 0 and 31) */
    ck_assert_int_eq(filename_claim("02/00/00/00"), true);
    ck_assert_int_eq(filename_claim("02/00/00/00"), false); /* duplicate claim */
    ck_assert_int_eq(filename_claim("02/00/00/31"), true);
    ck_assert_int_eq(filename_claim("02/00/00/31"), false); /* duplicate claim */

    /* High bitfield boundary claims (slots 32 and 63) */
    ck_assert_int_eq(filename_claim("02/00/00/32"), true);
    ck_assert_int_eq(filename_claim("02/00/00/32"), false); /* duplicate claim */
    ck_assert_int_eq(filename_claim("02/00/00/63"), true);
    ck_assert_int_eq(filename_claim("02/00/00/63"), false); /* duplicate claim */

    /* Release high bitfield slot and re-claim */
    filename_release("02/00/00/32");
    ck_assert_int_eq(filename_claim("02/00/00/32"), true);

    /* Release low bitfield slot and re-claim */
    filename_release("02/00/00/00");
    ck_assert_int_eq(filename_claim("02/00/00/00"), true);

    teardown_mock_gui();
}
END_TEST

START_TEST(filename_request_test)
{
    setup_mock_gui();

    wisp_mkdir_all(TEMP_FILENAME_PREFIX "/dummy");
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

    wisp_mkdir_all(TEMP_FILENAME_PREFIX "/dummy");
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

static void create_dummy_file(const char *path)
{
    FILE *f = fopen(path, "w");
    if (f) {
        fputs("test content", f);
        fclose(f);
    }
}

START_TEST(filename_flush_test)
{
    setup_mock_gui();

    wisp_mkdir_all(TEMP_FILENAME_PREFIX "/dummy");
    filename_initialise();

    filename_flush(); /* Shouldn't crash */

    teardown_mock_gui();
}
END_TEST

START_TEST(filename_flush_disk_test)
{
    setup_mock_gui();

    wisp_mkdir_all(TEMP_FILENAME_PREFIX "/dummy");
    filename_initialise();

    /* 1. Claim a valid file */
    ck_assert_int_eq(filename_claim("03/00/00/00"), true);

    /* 2. Create disk structure under TEMP_FILENAME_PREFIX */
    wisp_mkdir_all(TEMP_FILENAME_PREFIX "/03/00/00/dummy");
    wisp_mkdir_all(TEMP_FILENAME_PREFIX "/03/00/00/unexpected_dir/dummy");
    wisp_mkdir_all(TEMP_FILENAME_PREFIX "/99/dummy");

    /* Claimed file (should be retained) */
    char claimed_path[256];
    snprintf(claimed_path, sizeof(claimed_path), "%s/03/00/00/00", TEMP_FILENAME_PREFIX);
    create_dummy_file(claimed_path);

    /* Unclaimed file (should be deleted) */
    char unclaimed_path[256];
    snprintf(unclaimed_path, sizeof(unclaimed_path), "%s/03/00/00/01", TEMP_FILENAME_PREFIX);
    create_dummy_file(unclaimed_path);

    /* Invalid file name (should be deleted) */
    char invalid_path[256];
    snprintf(invalid_path, sizeof(invalid_path), "%s/03/00/00/invalid.txt", TEMP_FILENAME_PREFIX);
    create_dummy_file(invalid_path);

    /* Unexpected directory (should be deleted) */
    char unexp_dir_path[256];
    snprintf(unexp_dir_path, sizeof(unexp_dir_path), "%s/03/00/00/unexpected_dir", TEMP_FILENAME_PREFIX);

    /* Unexpected top directory (should be deleted) */
    char unexp_top_path[256];
    snprintf(unexp_top_path, sizeof(unexp_top_path), "%s/99", TEMP_FILENAME_PREFIX);

    /* Verify files exist before flush */
    struct stat sb;
    ck_assert_int_eq(stat(claimed_path, &sb), 0);
    ck_assert_int_eq(stat(unclaimed_path, &sb), 0);
    ck_assert_int_eq(stat(invalid_path, &sb), 0);
    ck_assert_int_eq(stat(unexp_dir_path, &sb), 0);
    ck_assert_int_eq(stat(unexp_top_path, &sb), 0);

    /* 3. Execute filename_flush */
    filename_flush();

    /* 4. Assert post-conditions */
    /* Claimed file must remain */
    ck_assert_int_eq(stat(claimed_path, &sb), 0);

    /* Unclaimed, invalid, and unexpected paths must be deleted */
    ck_assert_int_eq(stat(unclaimed_path, &sb), -1);
    ck_assert_int_eq(errno, ENOENT);

    ck_assert_int_eq(stat(invalid_path, &sb), -1);
    ck_assert_int_eq(errno, ENOENT);

    ck_assert_int_eq(stat(unexp_dir_path, &sb), -1);
    ck_assert_int_eq(errno, ENOENT);

    ck_assert_int_eq(stat(unexp_top_path, &sb), -1);
    ck_assert_int_eq(errno, ENOENT);

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
    tcase_add_test(tc, filename_request_rollover_test);
    tcase_add_test(tc, filename_claim_release_test);
    tcase_add_test(tc, filename_claim_release_boundary_test);
    tcase_add_test(tc, filename_flush_test);
    tcase_add_test(tc, filename_flush_disk_test);

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
