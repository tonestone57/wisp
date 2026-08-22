/*
 * Unit test for HTML5Test runner JavaScript features in QuickJS.
 */

#include <check.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "content/handlers/javascript/js.h"
#include <wisp/utils/corestrings.h>
#include <dom/core/implementation.h>
#include <dom/core/document.h>
#include <dom/core/node.h>
#include <dom/core/element.h>

static dom_document *create_test_document(void)
{
    dom_document *doc;
    dom_exception err;
    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML, NULL, NULL, NULL, NULL, NULL, &doc);
    if (err != DOM_NO_ERR) return NULL;

    dom_string *html_s;
    struct dom_element *html_el;
    dom_string_create_interned((const uint8_t *)"html", 4, &html_s);
    dom_document_create_element(doc, html_s, &html_el);
    dom_node_append_child((dom_node *)doc, (dom_node *)html_el, NULL);
    dom_string_unref(html_s);

    dom_string *body_s;
    struct dom_element *body_el;
    dom_string_create_interned((const uint8_t *)"body", 4, &body_s);
    dom_document_create_element(doc, body_s, &body_el);
    dom_node_append_child((dom_node *)html_el, (dom_node *)body_el, NULL);
    dom_string_unref(body_s);

    dom_node_unref((dom_node *)body_el);
    dom_node_unref((dom_node *)html_el);

    return doc;
}

START_TEST(test_html5test_features)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;

    js_initialise();
    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_test_document();
    err = js_newthread(heap, (void*)doc, doc, &thread);
    dom_node_unref((dom_node *)doc);
    ck_assert_int_eq(err, NSERROR_OK);

    const char *code =
        "try {\n"
        "  var element = document.createElement('canvas');\n"
        "  var hasCanvas = !!(element.getContext && element.getContext('2d'));\n"
        "  var hasText = element.getContext && typeof element.getContext('2d').fillText === 'function';\n"
        "  var div = document.createElement('div');\n"
        "  div.innerHTML = '<div><span>test</span></div>';\n"
        "  var hasInnerHTML = div.childNodes.length > 0;\n"
        "  var hasCustomElements = 'customElements' in window;\n"
        "  var hasShadowDOM = 'attachShadow' in document.createElement('div');\n"
        "  if (!hasCanvas || !hasText || !hasInnerHTML || !hasCustomElements || !hasShadowDOM) {\n"
        "    throw new Error('HTML5Test core features missing');\n"
        "  }\n"
        "  window.html5testRes = 'OK';\n"
        "} catch(e) {\n"
        "  window.html5testRes = e.message;\n"
        "}\n"
        "window.html5testRes === 'OK';";

    bool result = js_exec(thread, (const uint8_t *)code, strlen(code), "test_html5test_runner");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    js_finalise();
}
END_TEST

Suite *html5test_runner_suite(void)
{
    Suite *s = suite_create("HTML5TestRunner");
    TCase *tc = tcase_create("Features");
    tcase_add_test(tc, test_html5test_features);
    suite_add_tcase(s, tc);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = html5test_runner_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
