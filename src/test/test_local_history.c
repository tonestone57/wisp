/*
 * Copyright 2026 Wisp Contributors
 *
 * This file is part of Wisp.
 *
 * Wisp is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

/**
 * \file
 * Unit tests for local_history functions in src/desktop/local_history.c.
 */

#include <check.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <wisp/bitmap.h>
#include <wisp/browser_window.h>
#include <wisp/core_window.h>
#include <wisp/keypress.h>
#include <wisp/layout.h>
#include <wisp/plotters.h>
#include <wisp/desktop/gui_internal.h>
#include <wisp/desktop/local_history.h>
#include <wisp/utils/corestrings.h>
#include <wisp/utils/errors.h>
#include <wisp/utils/messages.h>
#include <wisp/utils/nsoption.h>
#include <wisp/utils/nsurl.h>

#include "desktop/browser_private.h"
#include "desktop/local_history_private.h"
#include "desktop/system_colour.h"
#include "content/urldb.h"

static nserror mock_layout_position(
    const struct plot_font_style *fstyle, const char *string, size_t length, int width, size_t *actual_x, int *actual_y)
{
    (void)fstyle;
    (void)string;
    (void)width;
    if (actual_x) *actual_x = length;
    if (actual_y) *actual_y = (int)(length * 8);
    return NSERROR_OK;
}

static struct gui_layout_table mock_layout_table = {
    .position = mock_layout_position,
};

static nserror mock_corewindow_set_extent(struct core_window *cw, int width, int height)
{
    (void)cw;
    (void)width;
    (void)height;
    return NSERROR_OK;
}

static nserror mock_corewindow_invalidate(struct core_window *cw, const struct rect *r)
{
    (void)cw;
    (void)r;
    return NSERROR_OK;
}

static nserror mock_corewindow_get_scroll(const struct core_window *cw, int *x, int *y)
{
    (void)cw;
    if (x) *x = 0;
    if (y) *y = 0;
    return NSERROR_OK;
}

static nserror mock_corewindow_set_scroll(struct core_window *cw, int x, int y)
{
    (void)cw;
    (void)x;
    (void)y;
    return NSERROR_OK;
}

static nserror mock_corewindow_get_dimensions(const struct core_window *cw, int *width, int *height)
{
    (void)cw;
    if (width) *width = 800;
    if (height) *height = 600;
    return NSERROR_OK;
}

static struct core_window_table mock_corewindow_table = {
    .set_extent = mock_corewindow_set_extent,
    .invalidate = mock_corewindow_invalidate,
    .get_scroll = mock_corewindow_get_scroll,
    .set_scroll = mock_corewindow_set_scroll,
    .get_dimensions = mock_corewindow_get_dimensions,
};

static struct wisp_table mock_guit = {
    .layout = &mock_layout_table,
    .corewindow = &mock_corewindow_table,
};

extern struct wisp_table *guit;

static nserror mock_plot_rectangle(const struct redraw_context *ctx, const plot_style_t *style, const struct rect *rect)
{
    (void)ctx;
    (void)style;
    (void)rect;
    return NSERROR_OK;
}

static nserror mock_plot_line(const struct redraw_context *ctx, const plot_style_t *style, const struct rect *rect)
{
    (void)ctx;
    (void)style;
    (void)rect;
    return NSERROR_OK;
}

static nserror mock_plot_text(const struct redraw_context *ctx, const plot_font_style_t *fstyle, int x, int y, const char *text, size_t length)
{
    (void)ctx;
    (void)fstyle;
    (void)x;
    (void)y;
    (void)text;
    (void)length;
    return NSERROR_OK;
}

static nserror mock_plot_clip(const struct redraw_context *ctx, const struct rect *clip)
{
    (void)ctx;
    (void)clip;
    return NSERROR_OK;
}

static nserror mock_plot_bitmap(const struct redraw_context *ctx, struct bitmap *bitmap, int x, int y, int width, int height, colour bg, bitmap_flags_t flags)
{
    (void)ctx;
    (void)bitmap;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)bg;
    (void)flags;
    return NSERROR_OK;
}

static struct plotter_table mock_plotters = {
    .rectangle = mock_plot_rectangle,
    .line = mock_plot_line,
    .text = mock_plot_text,
    .clip = mock_plot_clip,
    .bitmap = mock_plot_bitmap,
};

static struct redraw_context mock_redraw_ctx = {
    .plot = &mock_plotters,
};

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

START_TEST(test_local_history_init_and_fini_null_bw)
{
    setup_env();

    void *dummy_cw_handle = (void *)0x5678;
    struct local_history_session *session = NULL;

    /* Test init with NULL browser_window */
    nserror err = local_history_init(dummy_cw_handle, NULL, &session);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(session);

    /* Test get_url and mouse_action with NULL bw */
    ck_assert_int_eq(local_history_get_url(session, 10, 10, NULL), NSERROR_BAD_PARAMETER);
    ck_assert_int_eq(local_history_mouse_action(session, BROWSER_MOUSE_PRESS_1, 10, 10), NSERROR_BAD_PARAMETER);

    /* Test redraw with NULL bw */
    struct rect clip = { .x0 = 0, .y0 = 0, .x1 = 100, .y1 = 100 };
    ck_assert_int_eq(local_history_redraw(session, 0, 0, &clip, &mock_redraw_ctx), NSERROR_OK);

    /* Clean up */
    err = local_history_fini(session);
    ck_assert_int_eq(err, NSERROR_OK);

    teardown_env();
}
END_TEST

START_TEST(test_local_history_init_with_history_and_operations)
{
    setup_env();

    void *dummy_cw_handle = (void *)0x5678;

    /* Build mock history tree */
    struct history hist;
    memset(&hist, 0, sizeof(hist));
    hist.width = 300;
    hist.height = 200;

    struct history_entry entry_root;
    memset(&entry_root, 0, sizeof(entry_root));
    entry_root.x = 10;
    entry_root.y = 10;
    entry_root.page.title = "Root Page";

    nsurl *root_url = NULL;
    ck_assert_int_eq(nsurl_create("http://example.com/root", &root_url), NSERROR_OK);
    entry_root.page.url = root_url;

    struct history_entry entry_child;
    memset(&entry_child, 0, sizeof(entry_child));
    entry_child.x = 100;
    entry_child.y = 10;
    entry_child.page.title = "Child Page";
    entry_child.back = &entry_root;

    nsurl *child_url = NULL;
    ck_assert_int_eq(nsurl_create("http://example.com/child", &child_url), NSERROR_OK);
    entry_child.page.url = child_url;

    entry_root.forward = &entry_child;
    entry_root.forward_pref = &entry_child;
    entry_root.forward_last = &entry_child;
    entry_root.children = 1;

    hist.start = &entry_root;
    hist.current = &entry_root;

    struct browser_window bw;
    memset(&bw, 0, sizeof(bw));
    bw.history = &hist;

    struct local_history_session *session = NULL;

    /* Test init with valid bw */
    nserror err = local_history_init(dummy_cw_handle, &bw, &session);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(session);

    /* Test get_size */
    int width = 0, height = 0;
    ck_assert_int_eq(local_history_get_size(session, &width, &height), NSERROR_OK);
    ck_assert_int_eq(width, 320);
    ck_assert_int_eq(height, 220);

    /* Test get_url hit */
    nsurl *out_url = NULL;
    /* entry_root bounding box: x in [10, 10+LOCAL_HISTORY_WIDTH], y in [10, 10+LOCAL_HISTORY_HEIGHT] */
    err = local_history_get_url(session, 15, 15, &out_url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(out_url);
    ck_assert(nsurl_compare(out_url, root_url, NSURL_COMPLETE));
    nsurl_unref(out_url);

    /* Test get_url miss */
    out_url = NULL;
    err = local_history_get_url(session, 999, 999, &out_url);
    ck_assert_int_eq(err, NSERROR_NOT_FOUND);

    /* Test scroll_to_cursor */
    ck_assert_int_eq(local_history_scroll_to_cursor(session), NSERROR_OK);

    /* Test redraw */
    struct rect clip = { .x0 = 0, .y0 = 0, .x1 = 500, .y1 = 500 };
    ck_assert_int_eq(local_history_redraw(session, 0, 0, &clip, &mock_redraw_ctx), NSERROR_OK);

    /* Test mouse actions */
    /* Mouse click on current entry (root) -> NSERROR_PERMISSION */
    ck_assert_int_eq(local_history_mouse_action(session, BROWSER_MOUSE_PRESS_1, 15, 15), NSERROR_PERMISSION);

    /* Mouse click with no press flag -> NSERROR_NOT_IMPLEMENTED */
    ck_assert_int_eq(local_history_mouse_action(session, BROWSER_MOUSE_HOVER, 15, 15), NSERROR_NOT_IMPLEMENTED);

    /* Mouse click empty space -> NSERROR_NOT_FOUND */
    ck_assert_int_eq(local_history_mouse_action(session, BROWSER_MOUSE_PRESS_1, 999, 999), NSERROR_NOT_FOUND);

    /* Test keypress navigation */
    /* Right key -> moves cursor to preferred child */
    ck_assert_msg(local_history_keypress(session, NS_KEY_RIGHT), "Expected NS_KEY_RIGHT to be handled");

    /* Left key -> moves cursor back to parent */
    ck_assert_msg(local_history_keypress(session, NS_KEY_LEFT), "Expected NS_KEY_LEFT to be handled");

    /* Down / Up key handling */
    ck_assert_msg(local_history_keypress(session, NS_KEY_DOWN), "Expected NS_KEY_DOWN to be handled");
    ck_assert_msg(local_history_keypress(session, NS_KEY_UP), "Expected NS_KEY_UP to be handled");

    /* Enter key when cursor is current -> false / unhandled navigation */
    ck_assert_msg(local_history_keypress(session, NS_KEY_CR), "Expected NS_KEY_CR to be handled");

    /* Unhandled key */
    ck_assert_msg(!local_history_keypress(session, 'a'), "Expected unhandled key to return false");

    /* Clean up */
    err = local_history_fini(session);
    ck_assert_int_eq(err, NSERROR_OK);

    nsurl_unref(child_url);
    nsurl_unref(root_url);

    teardown_env();
}
END_TEST

static Suite *local_history_suite(void)
{
    Suite *s = suite_create("local_history");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_local_history_init_and_fini_null_bw);
    tcase_add_test(tc_core, test_local_history_init_with_history_and_operations);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;

    Suite *s = local_history_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
