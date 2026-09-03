/*
 * Copyright 2026 Wisp Project
 *
 * Test form charset extraction (form_acceptable_charset and form_dom_to_data).
 */

#include <check.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dom/dom.h>

#include "wisp/utils/corestrings.h"
#include "wisp/utils/errors.h"
#include "wisp/utils/nsurl.h"

#include "content/handlers/html/form.c"

/* Dummy implementations of stubs required by form.c */
void html__redraw_a_box(struct html_content *html, struct box *box) { (void)html; (void)box; }
void browser_window_set_drag_type(struct browser_window *bw, browser_drag_type type, const struct rect *rect) { (void)bw; (void)type; (void)rect; }
nserror browser_window_navigate(struct browser_window *bw, struct nsurl *url, struct nsurl *referrer,
    enum browser_window_nav_flags flags, char *post_urlenc, struct fetch_multipart_data *post_multipart,
    struct hlcache_handle *parent)
{
    (void)bw; (void)url; (void)referrer; (void)flags; (void)post_urlenc; (void)post_multipart; (void)parent;
    return NSERROR_OK;
}
void content__request_redraw(struct content *c, int x, int y, int width, int height) { (void)c; (void)x; (void)y; (void)width; (void)height; }
const char *messages_get(const char *key) { return key; }
void textarea_destroy(struct textarea *ta) { (void)ta; }
bool textarea_set_text(struct textarea *ta, const char *text) { (void)ta; (void)text; return true; }
nserror scrollbar_create(bool horizontal, int length, int full_size, int visible_size, void *client_data,
    scrollbar_client_callback callback, struct scrollbar **s)
{
    (void)horizontal; (void)length; (void)full_size; (void)visible_size; (void)client_data; (void)callback;
    if (s) *s = NULL;
    return NSERROR_OK;
}
void scrollbar_destroy(struct scrollbar *s) { (void)s; }
int scrollbar_get_offset(struct scrollbar *s) { (void)s; return 0; }
nserror scrollbar_redraw(struct scrollbar *s, int x, int y, const struct rect *clip, float scale,
    const struct redraw_context *ctx)
{
    (void)s; (void)x; (void)y; (void)clip; (void)scale; (void)ctx;
    return NSERROR_OK;
}
scrollbar_mouse_status scrollbar_mouse_action(struct scrollbar *s, browser_mouse_state mouse, int x, int y)
{
    (void)s; (void)mouse; (void)x; (void)y;
    return 0;
}
const char *scrollbar_mouse_status_to_message(scrollbar_mouse_status status) { (void)status; return ""; }
void scrollbar_mouse_drag_end(struct scrollbar *s, browser_mouse_state mouse, int x, int y) { (void)s; (void)mouse; (void)x; (void)y; }
void font_plot_style_from_css(const css_unit_ctx *ctx, const css_computed_style *style,
    plot_font_style_t *fstyle) { (void)ctx; (void)style; (void)fstyle; }
void box_bounds(struct box *box, struct rect *r) { (void)box; (void)r; }
void box_coords(struct box *box, int *x, int *y) { (void)box; if (x) *x = 0; if (y) *y = 0; }

static void setup(void)
{
    corestrings_init();
}

static void teardown(void)
{
    corestrings_fini();
}

START_TEST(test_form_dom_to_data_doc_charset)
{
    struct form *f = form_new(NULL, "http://example.com/submit", NULL, method_GET, "UTF-8");
    ck_assert_ptr_nonnull(f);

    struct fetch_multipart_data *data = NULL;
    char *charset = NULL;

    /* Testing with document_charset set and node NULL */
    nserror err = form_dom_to_data(f, NULL, &data, &charset);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_null(data);
    ck_assert_ptr_nonnull(charset);
    ck_assert_str_eq(charset, "UTF-8");

    free(charset);
    form_free(f);
}
END_TEST

START_TEST(test_form_acceptable_charset_fallback)
{
    struct form *f_utf8 = form_new(NULL, "http://example.com/submit", NULL, method_GET, "UTF-8");
    ck_assert_ptr_nonnull(f_utf8);

    char *c1 = form_acceptable_charset(f_utf8);
    ck_assert_ptr_nonnull(c1);
    ck_assert_str_eq(c1, "UTF-8");
    free(c1);

    form_free(f_utf8);

    struct form *f_default = form_new(NULL, "http://example.com/submit", NULL, method_GET, NULL);
    ck_assert_ptr_nonnull(f_default);

    char *c2 = form_acceptable_charset(f_default);
    ck_assert_ptr_nonnull(c2);
    ck_assert_str_eq(c2, "ISO-8859-1");
    free(c2);

    form_free(f_default);
}
END_TEST

START_TEST(test_form_dom_to_data_null_charset_out)
{
    struct form *f = form_new(NULL, "http://example.com/submit", NULL, method_GET, "UTF-8");
    ck_assert_ptr_nonnull(f);

    struct fetch_multipart_data *data = NULL;

    /* Test form_dom_to_data with NULL charset_out parameter */
    nserror err = form_dom_to_data(f, NULL, &data, NULL);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_null(data);

    form_free(f);
}
END_TEST

static Suite *form_charset_suite(void)
{
    Suite *s = suite_create("Form Charset");
    TCase *tc_core = tcase_create("Core");

    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_form_dom_to_data_doc_charset);
    tcase_add_test(tc_core, test_form_acceptable_charset_fallback);
    tcase_add_test(tc_core, test_form_dom_to_data_null_charset_out);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = form_charset_suite();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
