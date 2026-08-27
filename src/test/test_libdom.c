#include <check.h>
#include <stdlib.h>
#include <stdbool.h>

#include <libwapcaplet/libwapcaplet.h>
#include <dom/dom.h>
#include <dom/html/html_input_element.h>
#include <dom/html/html_option_element.h>
#include "wisp/utils/errors.h"
#include "wisp/utils/nsurl.h"
#include "content/handlers/css/select.h"
#include "wisp/content/handlers/html/private.h"
#include "utils/libdom.h"
#include "utils/corestrings.h"
#include "content/handlers/html/box_construct.h"

// Context for testing callbacks
struct test_ctx {
    int count;
    nserror error_to_return;
};

// Callback for libdom_iterate_child_elements
static nserror test_callback(dom_node *node, void *ctx)
{
    struct test_ctx *tc = (struct test_ctx *)ctx;
    tc->count++;
    return tc->error_to_return;
}

static void setup(void)
{
    corestrings_init();
}

static void teardown(void)
{
    corestrings_fini();
}

START_TEST(test_libdom_iterate_null_parent)
{
    struct test_ctx tc = { 0, NSERROR_OK };
    nserror err = libdom_iterate_child_elements(NULL, test_callback, &tc);

    // Should fail gracefully, dom_node_get_child_nodes returns DOM_BAD_CAST_ERR or similar
    ck_assert_int_ne(err, NSERROR_OK);
    ck_assert_int_eq(tc.count, 0);
}
END_TEST

START_TEST(test_libdom_iterate_no_children)
{
    dom_document *doc = NULL;
    dom_element *root_el = NULL;
    dom_node *body = NULL;
    nserror err;
    dom_exception exc;

    err = libdom_parse_file("src/test/data/test_empty.html", "UTF-8", &doc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(doc);

    exc = dom_document_get_document_element(doc, &root_el);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    ck_assert_ptr_nonnull(root_el);

    exc = dom_node_get_first_child((dom_node *)root_el, &body);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    ck_assert_ptr_nonnull(body);

    struct test_ctx tc = { 0, NSERROR_OK };
    err = libdom_iterate_child_elements((dom_node *)body, test_callback, &tc);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_node_unref(body);
    dom_node_unref(root_el);
    dom_node_unref(doc);
}
END_TEST

START_TEST(test_libdom_iterate_with_children)
{
    dom_document *doc = NULL;
    dom_element *root_el = NULL;
    dom_node *body = NULL;
    nserror err;
    dom_exception exc;

    err = libdom_parse_file("src/test/data/test_mixed.html", "UTF-8", &doc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(doc);

    exc = dom_document_get_document_element(doc, &root_el);
    ck_assert_int_eq(exc, DOM_NO_ERR);

    struct test_ctx tc_root = { 0, NSERROR_OK };
    err = libdom_iterate_child_elements((dom_node *)root_el, test_callback, &tc_root);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(tc_root.count, 2);

    exc = dom_node_get_last_child((dom_node *)root_el, &body);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    ck_assert_ptr_nonnull(body);

    struct test_ctx tc = { 0, NSERROR_OK };
    err = libdom_iterate_child_elements(body, test_callback, &tc);

    ck_assert_int_eq(err, NSERROR_OK);
    // Should only iterate over elements (div and span), skipping text node
    ck_assert_int_eq(tc.count, 2);

    // Test early exit when callback returns error
    struct test_ctx tc_err = { 0, NSERROR_NOMEM };
    err = libdom_iterate_child_elements(body, test_callback, &tc_err);

    ck_assert_int_eq(err, NSERROR_NOMEM);
    // Should stop after first element
    ck_assert_int_eq(tc_err.count, 1);

    dom_node_unref(body);
    dom_node_unref(root_el);
    dom_node_unref(doc);
}
END_TEST

START_TEST(test_libdom_has_class_quirks_vs_standards)
{
    dom_document *doc = NULL;
    dom_element *root_el = NULL;
    dom_string *class_attr_name = NULL;
    dom_string *class_attr_val = NULL;
    lwc_string *target_exact = NULL;
    lwc_string *target_lower = NULL;
    nserror err;
    dom_exception exc;
    bool match = false;

    err = libdom_parse_file("src/test/data/test_empty.html", "UTF-8", &doc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(doc);

    exc = dom_document_get_document_element(doc, &root_el);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    ck_assert_ptr_nonnull(root_el);

    exc = dom_string_create_interned((const uint8_t *)"class", 5, &class_attr_name);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create_interned((const uint8_t *)"FooBar", 6, &class_attr_val);
    ck_assert_int_eq(exc, DOM_NO_ERR);

    exc = dom_element_set_attribute(root_el, class_attr_name, class_attr_val);
    ck_assert_int_eq(exc, DOM_NO_ERR);

    ck_assert_int_eq(lwc_intern_string("FooBar", 6, &target_exact), lwc_error_ok);
    ck_assert_int_eq(lwc_intern_string("foobar", 6, &target_lower), lwc_error_ok);

    /* Test Standards mode: case-sensitive matching */
    exc = dom_document_set_quirks_mode(doc, DOM_DOCUMENT_QUIRKS_MODE_NONE);
    ck_assert_int_eq(exc, DOM_NO_ERR);

    exc = dom_element_has_class(root_el, target_exact, &match);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    ck_assert_int_eq(match, true);

    exc = dom_element_has_class(root_el, target_lower, &match);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    ck_assert_int_eq(match, false);

    /* Test Quirks mode: case-insensitive matching */
    exc = dom_document_set_quirks_mode(doc, DOM_DOCUMENT_QUIRKS_MODE_FULL);
    ck_assert_int_eq(exc, DOM_NO_ERR);

    exc = dom_element_has_class(root_el, target_exact, &match);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    ck_assert_int_eq(match, true);

    exc = dom_element_has_class(root_el, target_lower, &match);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    ck_assert_int_eq(match, true);

    /* Cleanup */
    lwc_string_unref(target_exact);
    lwc_string_unref(target_lower);
    dom_string_unref(class_attr_name);
    dom_string_unref(class_attr_val);
    dom_node_unref(root_el);
    dom_node_unref(doc);
}
END_TEST

START_TEST(test_libdom_document_fragment_reinsert)
{
    dom_document *doc = NULL;
    dom_element *root_el = NULL;
    dom_document_fragment *frag = NULL;
    dom_element *div_el = NULL;
    dom_string *div_name = NULL;
    dom_node *res1 = NULL;
    dom_node *res2 = NULL;
    nserror err;
    dom_exception exc;

    err = libdom_parse_file("src/test/data/test_empty.html", "UTF-8", &doc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(doc);

    exc = dom_document_get_document_element(doc, &root_el);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    ck_assert_ptr_nonnull(root_el);

    exc = dom_document_create_document_fragment(doc, &frag);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    ck_assert_ptr_nonnull(frag);

    exc = dom_string_create_interned((const uint8_t *)"div", 3, &div_name);
    ck_assert_int_eq(exc, DOM_NO_ERR);

    exc = dom_document_create_element(doc, div_name, &div_el);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    ck_assert_ptr_nonnull(div_el);

    exc = dom_node_append_child((dom_node *)frag, (dom_node *)div_el, &res1);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    if (res1) dom_node_unref(res1);

    /* First insertion of DocumentFragment into root_el */
    exc = dom_node_append_child((dom_node *)root_el, (dom_node *)frag, &res2);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    if (res2) dom_node_unref(res2);

    /* Second insertion (re-insertion when fragment is empty or re-used) */
    exc = dom_node_append_child((dom_node *)root_el, (dom_node *)frag, &res2);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    if (res2) dom_node_unref(res2);

    /* Cleanup */
    dom_string_unref(div_name);
    dom_node_unref(div_el);
    dom_node_unref(frag);
    dom_node_unref(root_el);
    dom_node_unref(doc);
}
END_TEST

START_TEST(test_count_subtree_elements)
{
    dom_document *doc = NULL;
    dom_element *root_el = NULL;
    dom_element *child_el1 = NULL;
    dom_element *child_el2 = NULL;
    dom_element *grandchild = NULL;
    dom_string *div_name = NULL;
    dom_string *span_name = NULL;
    dom_node *res = NULL;
    nserror err;
    dom_exception exc;

    err = libdom_parse_file("src/test/data/test_empty.html", "UTF-8", &doc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(doc);

    exc = dom_document_get_document_element(doc, &root_el);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    ck_assert_ptr_nonnull(root_el);

    /* Test null root */
    ck_assert_int_eq(count_subtree_elements(NULL, 32), 0);

    /* Test document element sub-tree (root_el <html> has <head> and <body>) */
    ck_assert_int_eq(count_subtree_elements((dom_node *)root_el, 32), 3);

    exc = dom_string_create_interned((const uint8_t *)"div", 3, &div_name);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create_interned((const uint8_t *)"span", 4, &span_name);
    ck_assert_int_eq(exc, DOM_NO_ERR);

    exc = dom_document_create_element(doc, div_name, &child_el1);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_node_append_child((dom_node *)root_el, (dom_node *)child_el1, &res);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    if (res) dom_node_unref(res);

    exc = dom_document_create_element(doc, div_name, &child_el2);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_node_append_child((dom_node *)root_el, (dom_node *)child_el2, &res);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    if (res) dom_node_unref(res);

    exc = dom_document_create_element(doc, span_name, &grandchild);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_node_append_child((dom_node *)child_el1, (dom_node *)grandchild, &res);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    if (res) dom_node_unref(res);

    /* Total elements under root_el: <html>(1), <head>(2), <body>(3), child_el1(4), grandchild(5), child_el2(6) */
    ck_assert_int_eq(count_subtree_elements((dom_node *)root_el, 32), 6);

    /* Sub-tree at child_el1: child_el1 (1), grandchild (2) */
    ck_assert_int_eq(count_subtree_elements((dom_node *)child_el1, 32), 2);

    /* Limit test: limit = 2 on root_el should stop at 2 */
    ck_assert_int_eq(count_subtree_elements((dom_node *)root_el, 2), 2);

    /* Cleanup */
    dom_string_unref(div_name);
    dom_string_unref(span_name);
    dom_node_unref(child_el1);
    dom_node_unref(child_el2);
    dom_node_unref(grandchild);
    dom_node_unref(root_el);
    dom_node_unref(doc);
}
END_TEST

START_TEST(test_node_is_target)
{
    dom_document *doc = NULL;
    dom_element *div_el = NULL;
    dom_element *a_el = NULL;
    dom_string *str_div = NULL;
    dom_string *str_a = NULL;
    dom_string *str_id = NULL;
    dom_string *str_name = NULL;
    nsurl *url_with_frag = NULL;
    nsurl *url_no_frag = NULL;
    html_content html_c = { 0 };
    nscss_select_ctx ctx = { 0 };
    bool match = false;
    dom_exception exc;
    css_error cserr;

    exc = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML, NULL, NULL, NULL, NULL, NULL, &doc);
    ck_assert_int_eq(exc, DOM_NO_ERR);

    exc = dom_string_create_interned((const uint8_t *)"div", 3, &str_div);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create_interned((const uint8_t *)"a", 1, &str_a);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create_interned((const uint8_t *)"section1", 8, &str_id);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create_interned((const uint8_t *)"anchor1", 7, &str_name);
    ck_assert_int_eq(exc, DOM_NO_ERR);

    exc = dom_document_create_element(doc, str_div, &div_el);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_element_set_attribute(div_el, corestring_dom_id, str_id);
    ck_assert_int_eq(exc, DOM_NO_ERR);

    exc = dom_document_create_element(doc, str_a, &a_el);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_element_set_attribute(a_el, corestring_dom_name, str_name);
    ck_assert_int_eq(exc, DOM_NO_ERR);

    ck_assert_int_eq(nsurl_create("http://example.com/page.html#section1", &url_with_frag), NSERROR_OK);
    ck_assert_int_eq(nsurl_create("http://example.com/page.html", &url_no_frag), NSERROR_OK);

    ctx.c = &html_c;

    /* 1. Base URL has no fragment */
    html_c.base_url = url_no_frag;
    cserr = node_is_target(&ctx, div_el, &match);
    ck_assert_int_eq(cserr, CSS_OK);
    ck_assert_int_eq(match, false);

    /* 2. Base URL has fragment matching div id */
    html_c.base_url = url_with_frag;
    cserr = node_is_target(&ctx, div_el, &match);
    ck_assert_int_eq(cserr, CSS_OK);
    ck_assert_int_eq(match, true);

    /* 3. Base URL fragment does not match div id when looking at anchor */
    cserr = node_is_target(&ctx, a_el, &match);
    ck_assert_int_eq(cserr, CSS_OK);
    ck_assert_int_eq(match, false);

    /* 4. Base URL fragment matches anchor name */
    nsurl_unref(url_with_frag);
    ck_assert_int_eq(nsurl_create("http://example.com/page.html#anchor1", &url_with_frag), NSERROR_OK);
    html_c.base_url = url_with_frag;
    cserr = node_is_target(&ctx, a_el, &match);
    ck_assert_int_eq(cserr, CSS_OK);
    ck_assert_int_eq(match, true);

    /* Cleanup */
    nsurl_unref(url_with_frag);
    nsurl_unref(url_no_frag);
    dom_string_unref(str_div);
    dom_string_unref(str_a);
    dom_string_unref(str_id);
    dom_string_unref(str_name);
    dom_node_unref(div_el);
    dom_node_unref(a_el);
    dom_node_unref(doc);
}
END_TEST

START_TEST(test_node_is_checked)
{
    dom_document *doc = NULL;
    dom_element *input_el = NULL;
    dom_element *option_el = NULL;
    dom_element *div_el = NULL;
    dom_string *str_input = NULL;
    dom_string *str_option = NULL;
    dom_string *str_div = NULL;
    dom_string *str_checked = NULL;
    dom_string *str_selected = NULL;
    nscss_select_ctx ctx = { 0 };
    bool match = false;
    dom_exception exc;
    css_error cserr;

    exc = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML, NULL, NULL, NULL, NULL, NULL, &doc);
    ck_assert_int_eq(exc, DOM_NO_ERR);

    exc = dom_string_create_interned((const uint8_t *)"input", 5, &str_input);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create_interned((const uint8_t *)"option", 6, &str_option);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create_interned((const uint8_t *)"div", 3, &str_div);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create_interned((const uint8_t *)"checked", 7, &str_checked);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create_interned((const uint8_t *)"selected", 8, &str_selected);
    ck_assert_int_eq(exc, DOM_NO_ERR);

    exc = dom_document_create_element(doc, str_input, &input_el);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_document_create_element(doc, str_option, &option_el);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_document_create_element(doc, str_div, &div_el);
    ck_assert_int_eq(exc, DOM_NO_ERR);

    /* 1. Div element should never match checked */
    cserr = node_is_checked(&ctx, div_el, &match);
    ck_assert_int_eq(cserr, CSS_OK);
    ck_assert_int_eq(match, false);

    /* 2. Unchecked input element */
    cserr = node_is_checked(&ctx, input_el, &match);
    ck_assert_int_eq(cserr, CSS_OK);
    ck_assert_int_eq(match, false);

    /* 3. Input element with checked attribute */
    exc = dom_element_set_attribute(input_el, corestring_dom_checked, str_checked);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    cserr = node_is_checked(&ctx, input_el, &match);
    ck_assert_int_eq(cserr, CSS_OK);
    ck_assert_int_eq(match, true);

    /* 4. Input element with dom_html_input_element_set_checked (if HTML element) */
    exc = dom_html_input_element_set_checked((dom_html_input_element *)input_el, true);
    if (exc == DOM_NO_ERR) {
        cserr = node_is_checked(&ctx, input_el, &match);
        ck_assert_int_eq(cserr, CSS_OK);
        ck_assert_int_eq(match, true);

        /* Even with "checked" attribute present in DOM, set_checked(false) must result in match == false */
        exc = dom_html_input_element_set_checked((dom_html_input_element *)input_el, false);
        ck_assert_int_eq(exc, DOM_NO_ERR);
        cserr = node_is_checked(&ctx, input_el, &match);
        ck_assert_int_eq(cserr, CSS_OK);
        ck_assert_int_eq(match, false);

        exc = dom_element_remove_attribute(input_el, corestring_dom_checked);
        ck_assert_int_eq(exc, DOM_NO_ERR);
    }

    /* 5. Unselected option element */
    cserr = node_is_checked(&ctx, option_el, &match);
    ck_assert_int_eq(cserr, CSS_OK);
    ck_assert_int_eq(match, false);

    /* 6. Option element with selected attribute */
    exc = dom_element_set_attribute(option_el, corestring_dom_selected, str_selected);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    cserr = node_is_checked(&ctx, option_el, &match);
    ck_assert_int_eq(cserr, CSS_OK);
    ck_assert_int_eq(match, true);

    /* 7. Option element with dom_html_option_element_set_selected (if HTML element) */
    exc = dom_html_option_element_set_selected((dom_html_option_element *)option_el, true);
    if (exc == DOM_NO_ERR) {
        cserr = node_is_checked(&ctx, option_el, &match);
        ck_assert_int_eq(cserr, CSS_OK);
        ck_assert_int_eq(match, true);

        /* Even with "selected" attribute present in DOM, set_selected(false) must result in match == false */
        exc = dom_html_option_element_set_selected((dom_html_option_element *)option_el, false);
        ck_assert_int_eq(exc, DOM_NO_ERR);
        cserr = node_is_checked(&ctx, option_el, &match);
        ck_assert_int_eq(cserr, CSS_OK);
        ck_assert_int_eq(match, false);

        exc = dom_element_remove_attribute(option_el, corestring_dom_selected);
        ck_assert_int_eq(exc, DOM_NO_ERR);
    }

    /* Cleanup */
    dom_string_unref(str_input);
    dom_string_unref(str_option);
    dom_string_unref(str_div);
    dom_string_unref(str_checked);
    dom_string_unref(str_selected);
    dom_node_unref(input_el);
    dom_node_unref(option_el);
    dom_node_unref(div_el);
    dom_node_unref(doc);
}
END_TEST

static Suite *libdom_suite(void)
{
    Suite *s = suite_create("libdom");
    TCase *tc_core = tcase_create("Core");

    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_libdom_iterate_null_parent);
    tcase_add_test(tc_core, test_libdom_iterate_no_children);
    tcase_add_test(tc_core, test_libdom_iterate_with_children);
    tcase_add_test(tc_core, test_libdom_has_class_quirks_vs_standards);
    tcase_add_test(tc_core, test_libdom_document_fragment_reinsert);
    tcase_add_test(tc_core, test_count_subtree_elements);
    tcase_add_test(tc_core, test_node_is_target);
    tcase_add_test(tc_core, test_node_is_checked);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = libdom_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
