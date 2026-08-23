#include <check.h>
#include <stdlib.h>
#include <stdbool.h>

#include <libwapcaplet/libwapcaplet.h>
#include <dom/dom.h>
#include "wisp/utils/errors.h"
#include "utils/libdom.h"
#include "utils/corestrings.h"

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
