/*
 * Full runner for html5test.co in Wisp QuickJS environment.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include <stdbool.h>
#include <unistd.h>

#include "content/handlers/javascript/js.h"
#include "qjs_internal.h"
#include <wisp/utils/corestrings.h>
#include <dom/core/implementation.h>
#include <dom/core/document.h>
#include <dom/core/node.h>
#include <dom/core/element.h>

static char *read_file_contents(const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

static dom_document *create_html5test_document(void)
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

    dom_string *head_s;
    struct dom_element *head_el;
    dom_string_create_interned((const uint8_t *)"head", 4, &head_s);
    dom_document_create_element(doc, head_s, &head_el);
    dom_node_append_child((dom_node *)html_el, (dom_node *)head_el, NULL);
    dom_string_unref(head_s);
    dom_node_unref((dom_node *)head_el);

    dom_string *body_s;
    struct dom_element *body_el;
    dom_string_create_interned((const uint8_t *)"body", 4, &body_s);
    dom_document_create_element(doc, body_s, &body_el);
    dom_node_append_child((dom_node *)html_el, (dom_node *)body_el, NULL);
    dom_string_unref(body_s);

    /* create divs required by html5test.co */
    const char *div_ids[] = { "score", "results", "index", "contents", "loading", "warning" };
    for (size_t i = 0; i < sizeof(div_ids)/sizeof(div_ids[0]); i++) {
        dom_string *div_s, *id_attr, *id_val;
        struct dom_element *div_el;
        dom_string_create_interned((const uint8_t *)"div", 3, &div_s);
        dom_document_create_element(doc, div_s, &div_el);
        dom_string_create_interned((const uint8_t *)"id", 2, &id_attr);
        dom_string_create_interned((const uint8_t *)div_ids[i], strlen(div_ids[i]), &id_val);
        dom_element_set_attribute(div_el, id_attr, id_val);
        dom_node_append_child((dom_node *)body_el, (dom_node *)div_el, NULL);
        dom_string_unref(div_s);
        dom_string_unref(id_attr);
        dom_string_unref(id_val);
        dom_node_unref((dom_node *)div_el);
    }

    dom_node_unref((dom_node *)body_el);
    dom_node_unref((dom_node *)html_el);

    return doc;
}

START_TEST(test_html5test_full_execution)
{
#ifndef HTML5TEST_DATA_DIR
#define HTML5TEST_DATA_DIR "src/test/data/html5test"
#endif

    char base_path[512], engine_path[512], data_path[512];
    snprintf(base_path, sizeof(base_path), "%s/base.js", HTML5TEST_DATA_DIR);
    snprintf(engine_path, sizeof(engine_path), "%s/engine.js", HTML5TEST_DATA_DIR);
    snprintf(data_path, sizeof(data_path), "%s/data.js", HTML5TEST_DATA_DIR);

    char *base_js = read_file_contents(base_path);
    char *engine_js = read_file_contents(engine_path);
    char *data_js = read_file_contents(data_path);

    ck_assert_msg(base_js != NULL, "Failed to load %s", base_path);
    ck_assert_msg(engine_js != NULL, "Failed to load %s", engine_path);
    ck_assert_msg(data_js != NULL, "Failed to load %s", data_path);

    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;

    corestrings_init();
    js_initialise();
    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_html5test_document();
    err = js_newthread(heap, (void*)doc, doc, &thread);
    dom_node_unref((dom_node *)doc);
    ck_assert_int_eq(err, NSERROR_OK);

    /* Polyfills/Shims required by WhichBrowser / engine in headless test */
    const char *shim_code =
        "var Browsers = {};\n"
        "function WhichBrowser(opts) {\n"
        "  return {\n"
        "    isDevice: function() { return false; },\n"
        "    isOs: function() { return false; },\n"
        "    isBrowser: function() { return false; },\n"
        "    isType: function(t) { return t === 'desktop'; }\n"
        "  };\n"
        "}\n"
        "window.loadWhichBrowser = function(cb) { if (cb) cb(); };\n";

    js_exec(thread, (const uint8_t *)base_js, strlen(base_js), "base.js");
    js_exec(thread, (const uint8_t *)engine_js, strlen(engine_js), "engine.js");
    js_exec(thread, (const uint8_t *)data_js, strlen(data_js), "data.js");
    js_exec(thread, (const uint8_t *)shim_code, strlen(shim_code), "shim");

    const char *run_code =
        "window.html5testFinished = false;\n"
        "window.html5testScore = 0;\n"
        "window.html5testErrors = [];\n"
        "window.html5testError = null;\n"
        "window.html5testStep = 'start';\n"
        "\n"
        "if (typeof Test === 'function') {\n"
        "  Test.prototype.startBackground = function(id) {};\n"
        "  Test.prototype.stopBackground = function(id) {};\n"
        "  Test.prototype.waitForBackground = function() {\n"
        "    var that = this;\n"
        "    window.setTimeout(function() { that.checkForBackground(); }, 1);\n"
        "  };\n"
        "}\n"
        "\n"
        "\n"
        "try {\n"
        "  loadWhichBrowser(function() {\n"
        "    window.html5testStep = 'whichbrowser_cb';\n"
        "    Browsers = new WhichBrowser({});\n"
        "    try {\n"
        "      new Test(function(r) {\n"
        "        window.html5testStep = 'test_success_cb';\n"
        "        try {\n"
        "          var m = new Metadata(tests);\n"
        "          var c = new Calculate(r, m.data);\n"
        "          window.html5testScore = c.score;\n"
        "          window.html5testMaximum = c.maximum;\n"
        "          window.html5testResults = c.points;\n"
        "          window.html5testCalc = c;\n"
        "          window.html5testRawResults = r;\n"
        "          window.html5testFinished = true;\n"
        "        } catch (calcErr) {\n"
        "          window.html5testError = 'Calc error: ' + (calcErr.stack || calcErr.message || String(calcErr));\n"
        "          window.html5testFinished = true;\n"
        "        }\n"
        "      }, function(err) {\n"
        "        window.html5testStep = 'test_error_cb';\n"
        "        window.html5testScore = 0;\n"
        "        var errMsg = (err && err.message) ? err.message : String(err);\n"
        "        var errStack = (err && err.stack) ? err.stack : 'no stack';\n"
        "        window.html5testError = 'ErrorCb: ' + errMsg + ' Stack:\\n' + errStack;\n"
        "        window.html5testFinished = true;\n"
        "      });\n"
        "    } catch (testErr) {\n"
        "      window.html5testError = 'TestConstruct error: ' + testErr.message + '\\n' + testErr.stack;\n"
        "      window.html5testFinished = true;\n"
        "    }\n"
        "  });\n"
        "} catch (e) {\n"
        "  window.html5testScore = 0;\n"
        "  window.html5testError = 'Outer error: ' + (e.stack || e.message || String(e));\n"
        "  window.html5testFinished = true;\n"
        "}\n";

    bool res = js_exec(thread, (const uint8_t *)run_code, strlen(run_code), "run_html5test");
    ck_assert(res == true);

    /* Run timer loop until finished or max ticks */
    extern uint64_t qjs_execute_timers(JSContext *ctx);
    for (int i = 0; i < 1500; i++) {
        qjs_execute_timers(thread->ctx);
        usleep(10000); /* 10ms */
        JSValue global_obj = JS_GetGlobalObject(thread->ctx);
        JSValue is_done_val = JS_GetPropertyStr(thread->ctx, global_obj, "html5testFinished");
        bool is_done = JS_ToBool(thread->ctx, is_done_val);
        JS_FreeValue(thread->ctx, is_done_val);
        JS_FreeValue(thread->ctx, global_obj);
        if (is_done) {
            break;
        }
    }

    const char *print_err =
        "var report = [];\n"
        "function walk(data) {\n"
        "  if (!data) return;\n"
        "  for (var i = 0; i < data.length; i++) {\n"
        "    if (data[i].key) {\n"
        "      var res = 0;\n"
        "      try {\n"
        "        if (window.html5testCalc) {\n"
        "          res = window.html5testCalc.getResult(data[i].key);\n"
        "        }\n"
        "      } catch(e) { res = e.message; }\n"
        "      report.push(data[i].key + ': value=' + JSON.stringify(data[i].value) + ' res=' + res);\n"
        "    }\n"
        "    if (data[i].items) walk(data[i].items);\n"
        "  }\n"
        "}\n"
        "if (typeof tests !== 'undefined') walk(tests);\n"
        "var res = 'Finished: ' + window.html5testFinished + ', Score: ' + window.html5testScore + ' / ' + window.html5testMaximum + '\\nPoints details: ' + JSON.stringify(window.html5testResults) + '\\nREPORT:\\n' + report.join('\\n');\n"
        "res;\n";

    JSValue val = JS_Eval(thread->ctx, print_err, strlen(print_err), "print_err", JS_EVAL_TYPE_GLOBAL);
    const char *str = JS_ToCString(thread->ctx, val);
    if (str) {
        fprintf(stderr, "\n========== RESULT ==========\n%s\n============================\n\n", str);
        JS_FreeCString(thread->ctx, str);
    }
    JS_FreeValue(thread->ctx, val);

    const char *eval_score =
        "if (window.html5testError) {\n"
        "  throw new Error('HTML5Test error: ' + window.html5testError);\n"
        "}\n"
        "if (!window.html5testFinished) {\n"
        "  throw new Error('HTML5Test did not finish');\n"
        "}\n"
        "true;\n";

    res = js_exec(thread, (const uint8_t *)eval_score, strlen(eval_score), "eval_score");

    free(base_js);
    free(engine_js);
    free(data_js);

    ck_assert(res == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    js_finalise();
}
END_TEST

Suite *html5test_full_suite(void)
{
    Suite *s = suite_create("HTML5TestFull");
    TCase *tc = tcase_create("FullExecution");
    tcase_set_timeout(tc, 60);
    tcase_add_test(tc, test_html5test_full_execution);
    suite_add_tcase(s, tc);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = html5test_full_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
