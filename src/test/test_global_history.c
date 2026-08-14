#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "utils/corestrings.h"
#include "utils/nsurl.h"
#include "content/urldb.h"
#include "desktop/global_history.h"
#include "utils/messages.h"
#include "desktop/gui_table.h"
#include "utils/errors.h"
#include "utils/nsoption.h"
#include "content/hlcache.h"
#include "content/backing_store.h"
#include "desktop/gui_internal.h"
#include "wisp/misc.h"
#include "wisp/bitmap.h"
#include "wisp/utf8.h"
#include "wisp/layout.h"
#include "message_data_inline.h"
#include "utils/file.h"
#include "desktop/treeview.h"

// Set up guit pointer for tests
extern struct wisp_table *guit;
extern bool verbose_log;
extern struct gui_file_table *default_file_table;

static nserror mock_schedule(int t, void (*callback)(void *p), void *p)
{
    return NSERROR_OK;
}

static struct gui_misc_table mock_misc_table = {
    .schedule = mock_schedule,
};

static unsigned char mock_bmp_buf[40 * 10] = {0};

static void *mock_bitmap_create(int width, int height, enum gui_bitmap_flags flags) { return (void *)1; }
static void mock_bitmap_set_opaque(void *bitmap, bool opaque) {}
static bool mock_bitmap_get_opaque(void *bitmap) { return true; }
static int mock_bitmap_get_width(void *bitmap) { return 10; }
static int mock_bitmap_get_height(void *bitmap) { return 10; }
static size_t mock_bitmap_get_rowstride(void *bitmap) { return 40; }
static void mock_bitmap_destroy(void *bitmap) {}
static void mock_bitmap_modified(void *bitmap) {}
static unsigned char *mock_bitmap_get_buffer(void *bitmap) { return mock_bmp_buf; }
static nserror mock_bitmap_render(struct bitmap *bitmap, struct hlcache_handle *content) { return NSERROR_OK; }

static struct gui_bitmap_table mock_bitmap_table = {
    .create = mock_bitmap_create,
    .destroy = mock_bitmap_destroy,
    .set_opaque = mock_bitmap_set_opaque,
    .get_opaque = mock_bitmap_get_opaque,
    .get_buffer = mock_bitmap_get_buffer,
    .get_rowstride = mock_bitmap_get_rowstride,
    .get_width = mock_bitmap_get_width,
    .get_height = mock_bitmap_get_height,
    .modified = mock_bitmap_modified,
    .render = mock_bitmap_render,
};

static nserror mock_layout_width(const struct plot_font_style *fstyle,
                              const char *string, size_t length,
                              int *width)
{
    *width = length * 10;
    return NSERROR_OK;
}

static struct gui_layout_table mock_layout_table = {
    .width = mock_layout_width,
};

static struct gui_utf8_table mock_utf8_table = {
    .utf8_to_local = NULL,
    .local_to_utf8 = NULL,
};

static struct wisp_table test_guit = {
    .llcache = NULL,
    .misc = &mock_misc_table,
    .file = NULL,
    .utf8 = &mock_utf8_table,
    .layout = &mock_layout_table,
    .bitmap = &mock_bitmap_table,
};

// Expose tree_g internally to test environment to avoid hlcache failure
struct treeview_global {
    int initialised;
    int step_width;
    int furniture_width;
    int line_height;
};
extern struct treeview_global tree_g;

static void setup(void)
{
    nserror res;

    test_guit.llcache = filesystem_llcache_table;
    test_guit.file = default_file_table;
    guit = &test_guit;

    res = corestrings_init();
    ck_assert_int_eq(res, NSERROR_OK);

    urldb_init();

    res = nsoption_init(NULL, NULL, NULL);
    ck_assert_int_eq(res, NSERROR_OK);

    res = messages_add_from_inline(test_data_Messages, test_data_Messages_len);
    ck_assert_int_eq(res, NSERROR_OK);

    wisp_recursive_rm("test_cache_gh");
    mkdir("test_cache_gh", 0755);

    struct hlcache_parameters hlcache_params = {
        .bg_clean_time = 0,
        .llcache = {
            .limit = 1024 * 1024,
            .hysteresis = 0,
            .minimum_lifetime = 0,
            .minimum_bandwidth = 0,
            .maximum_bandwidth = 0,
            .time_quantum = 0,
            .fetch_attempts = 0,
            .store = {
                .path = "test_cache_gh",
                .limit = 1024 * 1024,
                .hysteresis = 128 * 1024,
            }
        }
    };
    res = hlcache_initialise(&hlcache_params);
    ck_assert_int_eq(res, NSERROR_OK);
}

static void teardown(void)
{
    hlcache_stop();
    hlcache_finalise();
    nsoption_finalise(NULL, NULL);
    urldb_destroy();
    corestrings_fini();
    wisp_recursive_rm("test_cache_gh");
    guit = NULL;
}

START_TEST(global_history_init_test)
{
    nserror err;

    err = global_history_init(NULL);
    ck_assert_int_eq(err, NSERROR_OK);

    // Bypass cleanup since the test isn't performing async cache fetching
    // which treeview requires.
    // By resetting `tree_g.initialised`, `global_history_fini()` will clean up `gh_ctx`
    // but when it calls `treeview_fini()` it will safely early exit.
    tree_g.initialised = 0;

    err = global_history_fini();
    ck_assert_int_eq(err, NSERROR_OK);
}
END_TEST

START_TEST(global_history_add_test)
{
    nserror err;
    nsurl *url;

    err = nsurl_create("http://example.com/test", &url);
    ck_assert_int_eq(err, NSERROR_OK);

    ck_assert_int_eq(urldb_add_url(url), true);

    err = global_history_init(NULL);
    ck_assert_int_eq(err, NSERROR_OK);

    err = global_history_add(url);
    ck_assert_int_eq(err, NSERROR_OK);

    tree_g.initialised = 0;

    err = global_history_fini();
    ck_assert_int_eq(err, NSERROR_OK);

    nsurl_unref(url);
}
END_TEST

START_TEST(global_history_export_test)
{
    nserror err;
    nsurl *url;
    char file_path[] = "/tmp/ns_test_gh_XXXXXX";
    int fd;
    FILE *fp;
    char buffer[256];

    err = nsurl_create("http://example.com/export_test", &url);
    ck_assert_int_eq(err, NSERROR_OK);

    ck_assert_int_eq(urldb_add_url(url), true);

    err = global_history_init(NULL);
    ck_assert_int_eq(err, NSERROR_OK);

    err = global_history_add(url);
    ck_assert_int_eq(err, NSERROR_OK);

    fd = mkstemp(file_path);
    ck_assert_int_ne(fd, -1);
    close(fd); // global_history_export opens it again

    err = global_history_export(file_path, "Test History");
    ck_assert_int_eq(err, NSERROR_OK);

    fp = fopen(file_path, "r");
    ck_assert_ptr_ne(fp, NULL);

    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        ck_assert_str_eq(buffer, "<!DOCTYPE html PUBLIC \"//W3C/DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n");
    } else {
        ck_abort_msg("Failed to read from export file");
    }

    fclose(fp);
    unlink(file_path);

    tree_g.initialised = 0;

    err = global_history_fini();
    ck_assert_int_eq(err, NSERROR_OK);

    nsurl_unref(url);
}
END_TEST

static Suite *global_history_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("global_history");

    tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, global_history_init_test);
    tcase_add_test(tc_core, global_history_add_test);
    tcase_add_test(tc_core, global_history_export_test);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    SRunner *sr;

    sr = srunner_create(global_history_suite());
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
