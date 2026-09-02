#include <check.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <wisp/bitmap.h>
#include <wisp/core_window.h>
#include <wisp/layout.h>
#include <wisp/plot_style.h>
#include <wisp/desktop/global_history.h>
#include <wisp/desktop/gui_internal.h>
#include <wisp/utils/corestrings.h>
#include <wisp/utils/errors.h>
#include <wisp/utils/messages.h>
#include <wisp/utils/nsoption.h>
#include <wisp/utils/nsurl.h>
#include "content/urldb.h"

struct mock_bitmap {
    int width;
    int height;
    size_t stride;
    unsigned char *buffer;
};

static nserror mock_layout_width(const struct plot_font_style *fstyle, const char *str, size_t len, int *width)
{
    (void)fstyle;
    (void)str;
    if (width != NULL) {
        *width = (int)(len * 8);
    }
    return NSERROR_OK;
}

static struct gui_layout_table mock_layout_table = {
    .width = mock_layout_width,
};

static nserror mock_corewindow_invalidate(struct core_window *cw, const struct rect *r)
{
    (void)cw;
    (void)r;
    return NSERROR_OK;
}

static nserror mock_corewindow_get_dimensions(const struct core_window *cw, int *width, int *height)
{
    (void)cw;
    if (width) *width = 800;
    if (height) *height = 600;
    return NSERROR_OK;
}

static nserror mock_corewindow_set_extent(struct core_window *cw, int width, int height)
{
    (void)cw;
    (void)width;
    (void)height;
    return NSERROR_OK;
}

static nserror mock_corewindow_drag_status(struct core_window *cw, core_window_drag_status ds)
{
    (void)cw;
    (void)ds;
    return NSERROR_OK;
}

static struct core_window_table mock_corewindow_table = {
    .invalidate = mock_corewindow_invalidate,
    .get_dimensions = mock_corewindow_get_dimensions,
    .set_extent = mock_corewindow_set_extent,
    .drag_status = mock_corewindow_drag_status,
};

static void *mock_bitmap_create(int width, int height, enum gui_bitmap_flags flags)
{
    (void)flags;
    struct mock_bitmap *bm = malloc(sizeof(struct mock_bitmap));
    if (bm == NULL) return NULL;
    bm->width = width;
    bm->height = height;
    bm->stride = (size_t)width * 4;
    size_t size = bm->stride * (size_t)height;
    bm->buffer = calloc(1, size ? size : 1);
    if (bm->buffer == NULL) {
        free(bm);
        return NULL;
    }
    return (void *)bm;
}

static void mock_bitmap_destroy(void *bitmap)
{
    struct mock_bitmap *bm = (struct mock_bitmap *)bitmap;
    if (bm != NULL) {
        free(bm->buffer);
        free(bm);
    }
}

static unsigned char *mock_bitmap_get_buffer(void *bitmap)
{
    struct mock_bitmap *bm = (struct mock_bitmap *)bitmap;
    return bm ? bm->buffer : NULL;
}

static size_t mock_bitmap_get_rowstride(void *bitmap)
{
    struct mock_bitmap *bm = (struct mock_bitmap *)bitmap;
    return bm ? bm->stride : 0;
}

static int mock_bitmap_get_width(void *bitmap)
{
    struct mock_bitmap *bm = (struct mock_bitmap *)bitmap;
    return bm ? bm->width : 0;
}

static int mock_bitmap_get_height(void *bitmap)
{
    struct mock_bitmap *bm = (struct mock_bitmap *)bitmap;
    return bm ? bm->height : 0;
}

static void mock_bitmap_modified(void *bitmap)
{
    (void)bitmap;
}

static struct gui_bitmap_table mock_bitmap_table = {
    .create = mock_bitmap_create,
    .destroy = mock_bitmap_destroy,
    .get_buffer = mock_bitmap_get_buffer,
    .get_rowstride = mock_bitmap_get_rowstride,
    .get_width = mock_bitmap_get_width,
    .get_height = mock_bitmap_get_height,
    .modified = mock_bitmap_modified,
};

static struct wisp_table mock_guit = {
    .layout = &mock_layout_table,
    .corewindow = &mock_corewindow_table,
    .bitmap = &mock_bitmap_table,
};

extern struct wisp_table *guit;

static void setup_env(void)
{
    guit = &mock_guit;
    ck_assert_int_eq(nsoption_init(NULL, NULL, NULL), NSERROR_OK);
    ck_assert_int_eq(corestrings_init(), NSERROR_OK);
    ck_assert_int_eq(messages_add_from_file("src/test/data/Messages"), NSERROR_OK);
    urldb_init();
}

static void teardown_env(void)
{
    urldb_destroy();
    corestrings_fini();
    nsoption_finalise(NULL, NULL);
}

START_TEST(test_global_history_init_and_fini)
{
    setup_env();

    void *dummy_cw_handle = (void *)0x1234;

    /* Initialize global history */
    nserror err = global_history_init(dummy_cw_handle);
    ck_assert_int_eq(err, NSERROR_OK);

    /* Test double initialization safety (fini then re-init) */
    err = global_history_fini();
    ck_assert_int_eq(err, NSERROR_OK);

    /* Re-initialization should succeed */
    err = global_history_init(dummy_cw_handle);
    ck_assert_int_eq(err, NSERROR_OK);

    err = global_history_fini();
    ck_assert_int_eq(err, NSERROR_OK);

    teardown_env();
}
END_TEST

START_TEST(test_global_history_add_and_selection)
{
    setup_env();

    void *dummy_cw_handle = (void *)0x1234;

    /* Add URL to urldb first */
    nsurl *url1 = NULL;
    ck_assert_int_eq(nsurl_create("http://example.com/test1", &url1), NSERROR_OK);
    urldb_add_url(url1);
    urldb_set_url_title(url1, "Example Title 1");
    urldb_update_url_visit_data(url1);

    /* Test adding entry before global_history_init (gh_ctx.tree is NULL) */
    ck_assert_int_eq(global_history_add(url1), NSERROR_OK);

    /* Init global history */
    ck_assert_int_eq(global_history_init(dummy_cw_handle), NSERROR_OK);

    /* Add another URL after init */
    nsurl *url2 = NULL;
    ck_assert_int_eq(nsurl_create("http://example.com/test2", &url2), NSERROR_OK);
    urldb_add_url(url2);
    urldb_set_url_title(url2, "Example Title 2");
    urldb_update_url_visit_data(url2);

    ck_assert_int_eq(global_history_add(url2), NSERROR_OK);

    /* Test adding URL non-existent in urldb returns NSERROR_BAD_PARAMETER */
    nsurl *url_missing = NULL;
    ck_assert_int_eq(nsurl_create("http://example.com/missing", &url_missing), NSERROR_OK);
    ck_assert_int_eq(global_history_add(url_missing), NSERROR_BAD_PARAMETER);
    nsurl_unref(url_missing);

    /* Verify selection functions */
    ck_assert_int_eq(global_history_has_selection(), false);

    nsurl *sel_url = NULL;
    const char *sel_title = NULL;
    ck_assert_int_eq(global_history_get_selection(&sel_url, &sel_title), false);
    ck_assert_ptr_null(sel_url);
    ck_assert_ptr_null(sel_title);

    /* Test tree view expand/contract */
    ck_assert_int_eq(global_history_expand(true), NSERROR_OK);
    ck_assert_int_eq(global_history_expand(false), NSERROR_OK);
    ck_assert_int_eq(global_history_contract(true), NSERROR_OK);
    ck_assert_int_eq(global_history_contract(false), NSERROR_OK);

    /* Test export */
    char temp_path[] = "/tmp/gh_export_XXXXXX";
    int fd = mkstemp(temp_path);
    if (fd >= 0) {
        close(fd);
        ck_assert_int_eq(global_history_export(temp_path, "Test History"), NSERROR_OK);
        unlink(temp_path);
    }

    /* Finalize */
    ck_assert_int_eq(global_history_fini(), NSERROR_OK);

    nsurl_unref(url2);
    nsurl_unref(url1);

    teardown_env();
}
END_TEST

static Suite *global_history_suite(void)
{
    Suite *s = suite_create("global_history");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_global_history_init_and_fini);
    tcase_add_test(tc_core, test_global_history_add_and_selection);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;

    Suite *s = global_history_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
