/*
 * Copyright 2024 Neosurf Contributors
 *
 * This file is part of NeoSurf.
 *
 * NeoSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

/**
 * \file
 * Unit tests for QuickJS-ng JavaScript engine integration.
 */

#include <check.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "content/handlers/javascript/js.h"
#include <wisp/utils/corestrings.h>
#include <dom/core/implementation.h>
#include <dom/core/document.h>
#include <dom/core/node.h>
#include <dom/core/element.h>

/* Include QuickJS directly for console binding tests */
#include "content/handlers/javascript/quickjs/dom_bridge.h"
#include "content/handlers/javascript/quickjs/qjs_internal.h"
#include "quickjs.h"
#include "utils/hashmap.h"
#include "wisp/desktop/gui_table.h"
#include "wisp/misc.h"
extern struct wisp_table *guit;

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

    dom_string *head_s;
    struct dom_element *head_el;
    dom_string_create_interned((const uint8_t *)"head", 4, &head_s);
    dom_document_create_element(doc, head_s, &head_el);
    printf("DEBUG: after create head_el refcnt=%u\n", ((dom_node *)head_el)->refcnt);
    dom_node_append_child((dom_node *)html_el, (dom_node *)head_el, NULL);
    printf("DEBUG: after append head_el refcnt=%u\n", ((dom_node *)head_el)->refcnt);
    dom_node_unref((dom_node *)head_el);
    printf("DEBUG: after unref head_el refcnt=%u\n", ((dom_node *)head_el)->refcnt);
    dom_string_unref(head_s);

    dom_string *body_s;
    struct dom_element *body_el;
    dom_string_create_interned((const uint8_t *)"body", 4, &body_s);
    dom_document_create_element(doc, body_s, &body_el);
    dom_node_append_child((dom_node *)html_el, (dom_node *)body_el, NULL);
    dom_string_unref(body_s);

    dom_string *div_s;
    struct dom_element *div_el;
    dom_string_create_interned((const uint8_t *)"div", 3, &div_s);
    dom_document_create_element(doc, div_s, &div_el);
    dom_node_append_child((dom_node *)body_el, (dom_node *)div_el, NULL);
    dom_node_unref((dom_node *)div_el);
    dom_string_unref(div_s);

    dom_string *p_s;
    struct dom_element *p_el;
    dom_string_create_interned((const uint8_t *)"p", 1, &p_s);
    dom_document_create_element(doc, p_s, &p_el);
    dom_node_append_child((dom_node *)body_el, (dom_node *)p_el, NULL);
    dom_node_unref((dom_node *)p_el);
    dom_string_unref(p_s);

    dom_node_unref((dom_node *)body_el);
    dom_node_unref((dom_node *)html_el);

    return doc;
}

START_TEST(test_quickjs_init_finalise)
{
    js_initialise();
    js_finalise();
}
END_TEST

START_TEST(test_quickjs_css_style_declaration)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    bool result;

    js_initialise();
    js_newheap(5, &heap);
    dom_document *doc = create_test_document();
    js_newthread(heap, (void*)doc, doc, &thread);
    dom_node_unref((dom_node *)doc);
    doc = NULL;

    const char *code =
        "var el = document.createElement('div');\n"
        "// 1. Check style exists and has expected prototype/class\n"
        "if (typeof el.style !== 'object') throw 'style missing';\n"
        "if (typeof CSSStyleDeclaration !== 'undefined' && !(el.style instanceof CSSStyleDeclaration)) throw 'not an instance of CSSStyleDeclaration';\n"
        "\n"
        "// 2. Test initial style parsing from attribute\n"
        "el.setAttribute('style', 'color: red; background-color: blue;');\n"
        "var style = el.style;\n"
        "if (style.color !== 'red') throw 'color get fail';\n"
        "if (style.backgroundColor !== 'blue') throw 'backgroundColor camelCase get fail';\n"
        "if (style['background-color'] !== 'blue') throw 'background-color kebab-case get fail';\n"
        "\n"
        "// 3. Test standard methods: getPropertyValue, setProperty, removeProperty\n"
        "if (style.getPropertyValue('color') !== 'red') throw 'getPropertyValue fail';\n"
        "style.setProperty('font-size', '16px');\n"
        "if (style.fontSize !== '16px') throw 'setProperty camelCase sync fail';\n"
        "if (style.getPropertyValue('font-size') !== '16px') throw 'setProperty getPropertyValue fail';\n"
        "\n"
        "var removed = style.removeProperty('color');\n"
        "if (removed !== 'red') throw 'removeProperty return value fail';\n"
        "if (style.color !== '') throw 'removeProperty target value fail';\n"
        "\n"
        "// 4. Test cssText getter and setter\n"
        "style.cssText = 'display: inline-block; opacity: 0.5;';\n"
        "if (style.display !== 'inline-block') throw 'cssText setter fail';\n"
        "if (style.opacity !== '0.5') throw 'cssText setter sync fail';\n"
        "\n"
        "// 5. Test length and index-based enumeration\n"
        "if (style.length !== 2) throw 'length fail: ' + style.length;\n"
        "if (style.item(0) !== 'display') throw 'item(0) fail: ' + style.item(0);\n"
        "if (style[1] !== 'opacity') throw 'style[1] index access fail: ' + style[1];\n"
        "1;";

    result = js_exec(thread, (const uint8_t *)code, strlen(code), "test_css_style_declaration");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

START_TEST(test_quickjs_node_stubs)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    bool result;

    js_initialise();
    js_newheap(5, &heap);
    dom_document *doc = create_test_document();
    js_newthread(heap, (void*)doc, doc, &thread);
    dom_node_unref((dom_node *)doc);
    doc = NULL;

    const char *script =
        "var parent = document.createElement('div');\n"
        "var child1 = document.createElement('span');\n"
        "var child2 = document.createElement('p');\n"
        "parent.appendChild(child1);\n"
        "parent.appendChild(child2);\n"
        "var children = parent.childNodes;\n"
        "var childrenOk = children.length === 2 && children[0] === child1 && children[1] === child2;\n"
        "var baseURIOk = parent.baseURI !== null;\n"
        "var lookupOk = typeof parent.lookupPrefix === 'function' && typeof parent.lookupNamespaceURI === 'function' && typeof parent.isDefaultNamespace === 'function';\n"
        "childrenOk && baseURIOk && lookupOk;";

    result = js_exec(thread, (const uint8_t *)script, strlen(script), "test_node_stubs");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

START_TEST(test_quickjs_dom_parser)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;

    js_initialise();
    corestrings_init();

    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_test_document();

    err = js_newthread(heap, (void*)doc, doc, &thread);

    dom_node_unref((dom_node *)doc);
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    /* Test DOMParser constructor, XML parsing, HTML parsing, error handling and MIME validation */
    const char *code =
        "try {\n"
        "  if (typeof DOMParser !== 'function') throw 'DOMParser missing';\n"
        "  var parser = new DOMParser();\n"
        "  if (!parser) throw 'failed to instantiate';\n"
        "\n"
        "  // 1. Well-formed XML parsing\n"
        "  var xmlDoc = parser.parseFromString('<root><child id=\"c1\">hello</child></root>', 'text/xml');\n"
        "  if (!xmlDoc) throw 'failed to parse XML';\n"
        "  var child = xmlDoc.getElementById('c1');\n"
        "  if (!child) throw 'getElementById failed on XML';\n"
        "  if (child.tagName !== 'child') throw 'incorrect child tag';\n"
        "\n"
        "  // 2. Spec-compliant XML parsing error handling (<parsererror>)\n"
        "  var badXmlDoc = parser.parseFromString('<root><unclosed></root>', 'text/xml');\n"
        "  if (!badXmlDoc) throw 'failed to parse bad XML';\n"
        "  var parsererror = badXmlDoc.documentElement;\n"
        "  if (!parsererror || parsererror.tagName !== 'parsererror') throw 'parsererror element missing';\n"
        "\n"
        "  // 3. Successful HTML parsing\n"
        "  var htmlDoc = parser.parseFromString('<html><body><div id=\"h1\">world</div></body></html>', 'text/html');\n"
        "  if (!htmlDoc) throw 'failed to parse HTML';\n"
        "  var div = htmlDoc.getElementById('h1');\n"
        "  if (!div) throw 'getElementById failed on HTML';\n"
        "  if (div.tagName.toLowerCase() !== 'div') throw 'incorrect HTML tag';\n"
        "\n"
        "  // 4. Rejection of unsupported MIME types\n"
        "  var threw = false;\n"
        "  try {\n"
        "    parser.parseFromString('hello', 'image/png');\n"
        "  } catch (e) {\n"
        "    threw = true;\n"
        "  }\n"
        "  if (!threw) throw 'unsupported MIME type did not throw';\n"
        "} catch(e) {\n"
        "  console.log('TEST_ERROR:', e);\n"
        "  throw e;\n"
        "}\n"
        "1;";
    JSValue val = js_eval_with_aot_cache(thread->ctx, (const uint8_t *)code, strlen(code), "test_DOMParser", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(val)) {
        JSValue exc = JS_GetException(thread->ctx);
        const char *exc_str = JS_ToCString(thread->ctx, exc);
        fprintf(stderr, "\\n--- EXCEPTION: %s ---\\n\\n", exc_str ? exc_str : "unknown");
        if (exc_str) JS_FreeCString(thread->ctx, exc_str);
        JS_FreeValue(thread->ctx, exc);
    }
    ck_assert(!JS_IsException(val));
    JS_FreeValue(thread->ctx, val);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    corestrings_fini();
    js_finalise();
}
END_TEST

START_TEST(test_quickjs_aot_cache)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();

    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_test_document();

    err = js_newthread(heap, (void*)doc, doc, &thread);

    dom_node_unref((dom_node *)doc);
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    /* Force a clean environment for bytecode storage testing */
    unlink("/tmp/wisp-bytecode-cache/cb23c5dc9286beade7a95d92d41c5c78b878e77ec88c26adb5a5bf5c8e3ce21c.bin");

    const char *code = "var aot_test_var = 123 + 456; aot_test_var === 579;";

    /* First run - compiles and saves cache */
    result = js_exec(thread, (const uint8_t *)code, strlen(code), "test_aot_cache_first");
    ck_assert(result == true);

    /* Verify cache file exists (SHA256 for the string above is expected to be stable) */
    FILE *f = fopen("/tmp/wisp-bytecode-cache/cb23c5dc9286beade7a95d92d41c5c78b878e77ec88c26adb5a5bf5c8e3ce21c.bin", "rb");
    ck_assert_ptr_nonnull(f);
    fclose(f);

    /* Second run - loads directly from raw cached bytecode */
    result = js_exec(thread, (const uint8_t *)code, strlen(code), "test_aot_cache_second");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

/**
 * Test structural JSON pre-parsing with SSE2/NEON/RVV 1.0 SIMD loops.
 */
START_TEST(test_quickjs_json_simd)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();

    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_test_document();

    err = js_newthread(heap, (void*)doc, doc, &thread);

    dom_node_unref((dom_node *)doc);
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    /* 1. Test nested JSON objects and arrays */
    const char *code1 = "var obj = JSON.parse('{\"a\": {\"b\": {\"c\": 42}}, \"arr\": [1, 2, [3, 4]]}'); obj.a.b.c === 42 && obj.arr[2][1] === 4;";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_json_nested");
    ck_assert(result == true);

    /* 2. Test JSON with heavy whitespaces, tabs, carriage returns, and newlines */
    const char *code2 = "var obj = JSON.parse(' \\n\\r\\t { \\n\\r\\t \"x\" \\n\\r\\t : \\n\\r\\t 999 \\n\\r\\t } \\n\\r\\t '); obj.x === 999;";
    result = js_exec(thread, (const uint8_t *)code2, strlen(code2), "test_json_whitespace");
    ck_assert(result == true);

    /* 3. Test JSON with long string containing spaces, quotes, and escape characters */
    const char *code3 = "var obj = JSON.parse('{\"desc\": \"A very long description with spaces, quotes like \\\\\\\"hello\\\\\\\" and trailing text.\"}'); obj.desc.includes('hello');";
    result = js_exec(thread, (const uint8_t *)code3, strlen(code3), "test_json_long_string");
    ck_assert(result == true);

    /* 4. Test error handling for invalid JSON syntax */
    const char *code4 = "var pass = false; try { JSON.parse('{\"broken\": '); } catch (e) { pass = true; } pass === true;";
    result = js_exec(thread, (const uint8_t *)code4, strlen(code4), "test_json_syntax_error");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

/**
 * Test EventTarget full functionality.
 */
START_TEST(test_quickjs_event_target_full)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();
    corestrings_init();

    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_test_document();

    err = js_newthread(heap, (void*)doc, doc, &thread);

    dom_node_unref((dom_node *)doc);
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    /* Test adding and dispatching on window */
    const char *code1 = "window.testGlobal = 0;\n"
                        "function onTestEvent() { window.testGlobal = 1; }\n"
                        "window.addEventListener('testEvent', onTestEvent);\n"
                        "window.dispatchEvent(new Event('testEvent'));\n"
                        "window.testGlobal === 1;";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_dispatchEvent_window");
    ck_assert(result == true);

    /* Test removing on window */
    const char *code2 = "window.testGlobal = 0;\n"
                        "window.removeEventListener('testEvent', onTestEvent);\n"
                        "window.dispatchEvent(new Event('testEvent'));\n"
                        "window.testGlobal === 0;";
    result = js_exec(thread, (const uint8_t *)code2, strlen(code2), "test_removeEventListener_window");
    ck_assert(result == true);

    /* Test adding and dispatching on document element */
    const char *code3 = "var el = document.createElement('div');\n"
                        "el.testValue = 0;\n"
                        "function onElEvent() { el.testValue = 42; }\n"
                        "el.addEventListener('click', onElEvent);\n"
                        "el.dispatchEvent(new Event('click'));\n"
                        "el.testValue === 42;";
    result = js_exec(thread, (const uint8_t *)code3, strlen(code3), "test_dispatchEvent_element");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

START_TEST(test_quickjs_trusted_types)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();
    js_newheap(5, &heap);
    dom_document *doc = create_test_document();

    err = js_newthread(heap, (void*)doc, doc, &thread);
    dom_node_unref((dom_node *)doc);
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    /* Test 1: trustedTypes exists and contains classes/methods */
    const char *code1 =
        "typeof trustedTypes === 'object' && "
        "typeof TrustedHTML === 'function' && "
        "typeof TrustedScript === 'function' && "
        "typeof TrustedScriptURL === 'function';";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_tt_exists");
    ck_assert(result == true);

    /* Test 2: Create a policy and generate Trusted Types */
    const char *code2 =
        "var policy = trustedTypes.createPolicy('my-policy', {\n"
        "  createHTML: (s) => s + ' [safe]',\n"
        "  createScript: (s) => s + ' // safe'\n"
        "});\n"
        "var html = policy.createHTML('<div>hello</div>');\n"
        "var script = policy.createScript('console.log(1)');\n"
        "trustedTypes.isHTML(html) === true && "
        "html.toString() === '<div>hello</div> [safe]' && "
        "trustedTypes.isScript(script) === true && "
        "script.toString() === 'console.log(1) // safe';";
    result = js_exec(thread, (const uint8_t *)code2, strlen(code2), "test_tt_create_policy");
    ck_assert(result == true);

    /* Test 3: Sinks accept raw strings when TT is not required/enforced */
    const char *code3 =
        "var threwTypeError = false;\n"
        "try {\n"
        "  var div = document.createElement('div');\n"
        "  div.innerHTML = '<span>not enforced yet</span>';\n"
        "} catch (e) {\n"
        "  if (e instanceof TypeError && e.message.includes('requires TrustedHTML')) threwTypeError = true;\n"
        "}\n"
        "threwTypeError === false;";
    result = js_exec(thread, (const uint8_t *)code3, strlen(code3), "test_tt_not_required");
    ck_assert(result == true);

    /* Test 4: Default policy configuration and fallback works */
    const char *code4 =
        "try {\n"
        "  var defPolicy = trustedTypes.createPolicy('default', {\n"
        "    createHTML: (s) => s + ' [default]'\n"
        "  });\n"
        "  if (trustedTypes.defaultPolicy !== defPolicy) {\n"
        "    throw new Error('defaultPolicy mismatch: ' + trustedTypes.defaultPolicy + ' vs ' + defPolicy);\n"
        "  }\n"
        "  true;\n"
        "} catch (e) {\n"
        "  throw new Error('Error in default_policy test: ' + e.message + '\\n' + e.stack);\n"
        "}";
    result = js_exec(thread, (const uint8_t *)code4, strlen(code4), "test_tt_default_policy");
    ck_assert(result == true);

    /* Test 5: Sinks throw TypeError when Trusted Types are required and a raw string is assigned (with defaultPolicy fallback disabled) */
    const char *code5 =
        "__trustedTypesSetRequiredForTesting(true);\n"
        "// Temporarily clear defaultPolicy to test raw string rejection\n"
        "var savedDefault = trustedTypes.defaultPolicy;\n"
        "trustedTypes.defaultPolicy = null;\n"
        "var threw = false;\n"
        "try {\n"
        "  var div = document.createElement('div');\n"
        "  div.innerHTML = 'unsafe string';\n"
        "} catch (e) {\n"
        "  if (e instanceof TypeError && e.message.includes('requires TrustedHTML')) threw = true;\n"
        "}\n"
        "trustedTypes.defaultPolicy = savedDefault;\n"
        "__trustedTypesSetRequiredForTesting(false);\n"
        "threw === true;";
    result = js_exec(thread, (const uint8_t *)code5, strlen(code5), "test_tt_throws");
    ck_assert(result == true);

    /* Test 6: Default policy conversion applies when Trusted Types are required and a raw string is assigned */
    const char *code6 =
        "__trustedTypesSetRequiredForTesting(true);\n"
        "var div = document.createElement('div');\n"
        "var threwTypeError = false;\n"
        "try {\n"
        "  div.innerHTML = 'test-string';\n"
        "} catch (e) {\n"
        "  if (e instanceof TypeError && e.message.includes('requires TrustedHTML')) threwTypeError = true;\n"
        "}\n"
        "__trustedTypesSetRequiredForTesting(false);\n"
        "threwTypeError === false;";
    result = js_exec(thread, (const uint8_t *)code6, strlen(code6), "test_tt_default_conversion");
    ck_assert(result == true);

    /* Test 7: Sinks allow explicitly wrapped Trusted Types even when required */
    const char *code7 =
        "__trustedTypesSetRequiredForTesting(true);\n"
        "var div = document.createElement('div');\n"
        "var p = trustedTypes.createPolicy('p', { createHTML: s => s });\n"
        "var threwTypeError = false;\n"
        "try {\n"
        "  div.innerHTML = p.createHTML('safe trusted html');\n"
        "} catch (e) {\n"
        "  if (e instanceof TypeError && e.message.includes('requires TrustedHTML')) threwTypeError = true;\n"
        "}\n"
        "__trustedTypesSetRequiredForTesting(false);\n"
        "threwTypeError === false;";
    result = js_exec(thread, (const uint8_t *)code7, strlen(code7), "test_tt_allow_trusted");
    ck_assert(result == true);

    /* Test 8: Script sinks (like eval) throw TypeError when raw strings are used under required mode */
    const char *code8 =
        "__trustedTypesSetRequiredForTesting(true);\n"
        "var savedDefault = trustedTypes.defaultPolicy;\n"
        "trustedTypes.defaultPolicy = null;\n"
        "var threw = false;\n"
        "try {\n"
        "  eval('1 + 1');\n"
        "} catch (e) {\n"
        "  if (e instanceof TypeError && e.message.includes('requires TrustedScript')) threw = true;\n"
        "}\n"
        "trustedTypes.defaultPolicy = savedDefault;\n"
        "__trustedTypesSetRequiredForTesting(false);\n"
        "threw === true;";
    result = js_exec(thread, (const uint8_t *)code8, strlen(code8), "test_tt_eval_throws");
    ck_assert(result == true);

    /* Test 9: Policy creation checking is enforced based on allowed policies set */
    const char *code9 =
        "__trustedTypesSetAllowedPoliciesForTesting(['policy-a', 'policy-b']);\n"
        "var threw = false;\n"
        "try {\n"
        "  trustedTypes.createPolicy('policy-c', {});\n"
        "} catch (e) {\n"
        "  if (e instanceof TypeError) threw = true;\n"
        "}\n"
        "__trustedTypesSetAllowedPoliciesForTesting(null);\n"
        "threw === true;";
    result = js_exec(thread, (const uint8_t *)code9, strlen(code9), "test_tt_policy_allowlist");
    ck_assert(result == true);

    /* Test 10: Built-in DOM-based auto-sanitization of HTML, Script and URL sinks */
    const char *code10 =
        "__trustedTypesSetRequiredForTesting(true);\n"
        "// Save and clear user-defined default policy to force built-in fallback\n"
        "var savedDefault = trustedTypes.defaultPolicy;\n"
        "trustedTypes.defaultPolicy = null;\n"
        "var div = document.createElement('div');\n"
        "// HTML auto-sanitization: strip script tags but allow text, strip on* handlers\n"
        "div.innerHTML = '<p>hello</p><script>alert(1)</script>';\n"
        "var htmlClean = div.innerHTML === '<p>hello</p>';\n"
        "var div2 = document.createElement('div');\n"
        "div2.innerHTML = '<a href=\"javascript:alert(1)\" onclick=\"evil()\">click</a>';\n"
        "var htmlClean2 = div2.innerHTML.includes('click') && !div2.innerHTML.includes('javascript') && !div2.innerHTML.includes('onclick');\n"
        "// Script URL auto-sanitization\n"
        "var scriptEl = document.createElement('script');\n"
        "scriptEl.src = 'javascript:alert(1)';\n"
        "var urlClean = scriptEl.src === 'about:blank';\n"
        "// Restore default policy\n"
        "trustedTypes.defaultPolicy = savedDefault;\n"
        "__trustedTypesSetRequiredForTesting(false);\n"
        "htmlClean && htmlClean2 && urlClean;";
    result = js_exec(thread, (const uint8_t *)code10, strlen(code10), "test_tt_builtin_sanitization");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

START_TEST(test_quickjs_mutation_observer_e2e)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();
    js_newheap(5, &heap);
    dom_document *doc = create_test_document();
    js_newthread(heap, (void*)doc, doc, &thread);
    dom_node_unref((dom_node *)doc);
    doc = NULL;

    const char *code =
        "var records = [];\n"
        "var observer = new MutationObserver(function(muts) {\n"
        "  records = records.concat(muts);\n"
        "});\n"
        "var div = document.createElement('div');\n"
        "var span = document.createElement('span');\n"
        "observer.observe(div, { childList: true, attributes: true });\n"
        "observer.observe(span, { childList: true, characterData: true, subtree: true });\n"
        "div.setAttribute('id', 'test');\n"
        "var text = document.createTextNode('hello');\n"
        "div.appendChild(text);\n"
        "div.removeChild(text);\n"
        "span.appendChild(document.createTextNode('world'));\n"
        "span.firstChild.data = 'new world';\n"
        "// At this point records should still be empty because it's a microtask\n"
        "if (records.length !== 0) throw new Error('Not asynchronous');\n"
        "var taken = observer.takeRecords();\n"
        "// div: attr, childList (add), childList (remove). span: childList (add), characterData\n"
        "taken.length === 5 && \n"
        "taken[0].type === 'attributes' && taken[0].target === div &&\n"
        "taken[1].type === 'childList' && taken[1].addedNodes.length === 1 && taken[1].addedNodes[0] === text &&\n"
        "taken[2].type === 'childList' && taken[2].removedNodes.length === 1 && taken[2].removedNodes[0] === text &&\n"
        "taken[3].type === 'childList' && taken[3].target === span &&\n"
        "taken[4].type === 'characterData' && taken[4].target === span.firstChild;";

    result = js_exec(thread, (const uint8_t *)code, strlen(code), "test_mutation_observer_e2e");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

/**
 * Test creating and destroying a heap.
 */
START_TEST(test_quickjs_heap_create_destroy)
{
    jsheap *heap = NULL;
    nserror err;

    js_initialise();

    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(heap);

    js_destroyheap(heap);
    js_finalise();
}
END_TEST

/**
 * Test creating and destroying a thread.
 */
START_TEST(test_quickjs_thread_create_destroy)
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
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(thread);

    err = js_closethread(thread);
    ck_assert_int_eq(err, NSERROR_OK);

    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

/**
 * Test executing simple JavaScript code.
 */
START_TEST(test_quickjs_exec_simple)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();

    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_test_document();

    err = js_newthread(heap, (void*)doc, doc, &thread);

    dom_node_unref((dom_node *)doc);
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    /* Test simple expression */
    const char *code = "1 + 1";
    result = js_exec(thread, (const uint8_t *)code, strlen(code), "test");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

/**
 * Test executing JavaScript with syntax error.
 */
START_TEST(test_quickjs_exec_syntax_error)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();

    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_test_document();

    err = js_newthread(heap, (void*)doc, doc, &thread);

    dom_node_unref((dom_node *)doc);
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    /* Test syntax error - should return false */
    const char *code = "function broken_syntax() { return 1;";
    result = js_exec(thread, (const uint8_t *)code, strlen(code), "test_error");
    ck_assert(result == false);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

/**
 * Test executing JavaScript that creates objects.
 */
START_TEST(test_quickjs_exec_objects)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();

    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_test_document();

    err = js_newthread(heap, (void*)doc, doc, &thread);

    dom_node_unref((dom_node *)doc);
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    /* Test creating objects and arrays */
    const char *code = "var obj = { name: 'test', value: 42 };\n"
                       "var arr = [1, 2, 3];\n"
                       "obj.name + arr.length;";
    result = js_exec(thread, (const uint8_t *)code, strlen(code), "test_objects");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

/**
 * Test console.log via js.h API (integration test).
 * This verifies the console binding is automatically initialized.
 */
START_TEST(test_quickjs_exec_console_log)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();

    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_test_document();

    err = js_newthread(heap, (void*)doc, doc, &thread);

    dom_node_unref((dom_node *)doc);
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    /* Test console.log - should work now that it's auto-initialized */
    const char *code = "console.log('Integration test: console works via js.h!');";
    result = js_exec(thread, (const uint8_t *)code, strlen(code), "test_console");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

/**
 * Test that execution on closed thread fails gracefully.
 */
START_TEST(test_quickjs_exec_closed_thread)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();

    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_test_document();

    err = js_newthread(heap, (void*)doc, doc, &thread);

    dom_node_unref((dom_node *)doc);
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    /* Close the thread first */
    err = js_closethread(thread);
    ck_assert_int_eq(err, NSERROR_OK);

    /* Try to execute - should fail gracefully */
    const char *code = "1 + 1";
    result = js_exec(thread, (const uint8_t *)code, strlen(code), "test");
    ck_assert(result == false);

    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

/**
 * Test multiple threads in one heap.
 */
START_TEST(test_quickjs_multiple_threads)
{
    jsheap *heap = NULL;
    jsthread *thread1 = NULL;
    jsthread *thread2 = NULL;
    nserror err;
    bool result;

    js_initialise();

    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    /* Create two threads */
    dom_document *doc1 = create_test_document();
    err = js_newthread(heap, (void*)doc1, doc1, &thread1);
    dom_node_unref((dom_node *)doc1);
    doc1 = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc2 = create_test_document();
    err = js_newthread(heap, (void*)doc2, doc2, &thread2);
    dom_node_unref((dom_node *)doc2);
    doc2 = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    /* Execute in both */
    const char *code = "var x = 1;";
    result = js_exec(thread1, (const uint8_t *)code, strlen(code), "test1");
    ck_assert(result == true);

    result = js_exec(thread2, (const uint8_t *)code, strlen(code), "test2");
    ck_assert(result == true);

    /* Clean up */
    js_closethread(thread1);
    js_closethread(thread2);
    js_destroythread(thread1);
    js_destroythread(thread2);
    js_destroyheap(heap);
    if (doc1) dom_node_unref((dom_node *)doc1);
    if (doc2) dom_node_unref((dom_node *)doc2);
    js_finalise();
}
END_TEST


/*
 * Console binding tests - test the QuickJS console directly
 */

/**
 * Test initializing the console binding.
 */
START_TEST(test_quickjs_console_init)
{
    JSRuntime *rt;
    JSContext *ctx;
    int ret;

    rt = JS_NewRuntime();
    ck_assert_ptr_nonnull(rt);

    ctx = JS_NewContext(rt);
    ck_assert_ptr_nonnull(ctx);

    /* Initialize console binding */
    ret = qjs_init_dom_bridge(ctx); qjs_init_console(ctx);
    ck_assert_int_eq(ret, 0);

    /* Verify console object exists */
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue console = JS_GetPropertyStr(ctx, global, "console");
    ck_assert(JS_IsObject(console));

    JS_FreeValue(ctx, console);
    JS_FreeValue(ctx, global);
    JS_FreeContext(ctx);
    qjs_bridge_cleanup(rt);
    JS_RunGC(rt);
    JS_FreeRuntime(rt);
}
END_TEST

/**
 * Test console.log() execution.
 */
START_TEST(test_quickjs_console_log)
{
    JSRuntime *rt;
    JSContext *ctx;
    JSValue result;

    rt = JS_NewRuntime();
    ctx = JS_NewContext(rt);
    qjs_init_dom_bridge(ctx); qjs_init_console(ctx);

    /* Execute console.log - should not throw */
    const char *code = "console.log('Hello from QuickJS!');";
    result = JS_Eval(ctx, code, strlen(code), "test", JS_EVAL_TYPE_GLOBAL);

    ck_assert(!JS_IsException(result));

    JS_FreeValue(ctx, result);
    qjs_console_cleanup(ctx);
    JS_FreeContext(ctx);
    qjs_bridge_cleanup(rt);
    JS_RunGC(rt);
    JS_FreeRuntime(rt);
}
END_TEST

/**
 * Test console.error() execution.
 */
START_TEST(test_quickjs_console_error)
{
    JSRuntime *rt;
    JSContext *ctx;
    JSValue result;

    rt = JS_NewRuntime();
    ctx = JS_NewContext(rt);
    qjs_init_dom_bridge(ctx); qjs_init_console(ctx);

    /* Execute console.error - should not throw */
    const char *code = "console.error('Test error message');";
    result = JS_Eval(ctx, code, strlen(code), "test", JS_EVAL_TYPE_GLOBAL);

    ck_assert(!JS_IsException(result));

    JS_FreeValue(ctx, result);
    qjs_console_cleanup(ctx);
    JS_FreeContext(ctx);
    qjs_bridge_cleanup(rt);
    JS_RunGC(rt);
    JS_FreeRuntime(rt);
}
END_TEST

/**
 * Test console.warn() execution.
 */
START_TEST(test_quickjs_console_warn)
{
    JSRuntime *rt;
    JSContext *ctx;
    JSValue result;

    rt = JS_NewRuntime();
    ctx = JS_NewContext(rt);
    qjs_init_dom_bridge(ctx); qjs_init_console(ctx);

    /* Execute console.warn - should not throw */
    const char *code = "console.warn('Test warning');";
    result = JS_Eval(ctx, code, strlen(code), "test", JS_EVAL_TYPE_GLOBAL);

    ck_assert(!JS_IsException(result));

    JS_FreeValue(ctx, result);
    qjs_console_cleanup(ctx);
    JS_FreeContext(ctx);
    qjs_bridge_cleanup(rt);
    JS_RunGC(rt);
    JS_FreeRuntime(rt);
}
END_TEST

/**
 * Test console with multiple arguments.
 */
START_TEST(test_quickjs_console_multiple_args)
{
    JSRuntime *rt;
    JSContext *ctx;
    JSValue result;

    rt = JS_NewRuntime();
    ctx = JS_NewContext(rt);
    qjs_init_dom_bridge(ctx); qjs_init_console(ctx);

    /* Execute console.log with multiple arguments */
    const char *code = "console.log('Value:', 42, 'Name:', 'test');";
    result = JS_Eval(ctx, code, strlen(code), "test", JS_EVAL_TYPE_GLOBAL);

    ck_assert(!JS_IsException(result));

    JS_FreeValue(ctx, result);
    qjs_console_cleanup(ctx);
    JS_FreeContext(ctx);
    qjs_bridge_cleanup(rt);
    JS_RunGC(rt);
    JS_FreeRuntime(rt);
}
END_TEST

/**
 * Test console.group() and console.groupEnd().
 */
START_TEST(test_quickjs_console_group)
{
    JSRuntime *rt;
    JSContext *ctx;
    JSValue result;

    rt = JS_NewRuntime();
    ctx = JS_NewContext(rt);
    qjs_init_dom_bridge(ctx); qjs_init_console(ctx);

    /* Execute grouping */
    const char *code = "console.group();\n"
                       "console.log('Grouped message');\n"
                       "console.groupEnd();";
    result = JS_Eval(ctx, code, strlen(code), "test", JS_EVAL_TYPE_GLOBAL);

    ck_assert(!JS_IsException(result));

    JS_FreeValue(ctx, result);
    qjs_console_cleanup(ctx);
    JS_FreeContext(ctx);
    qjs_bridge_cleanup(rt);
    JS_RunGC(rt);
    JS_FreeRuntime(rt);
}
END_TEST

/**
 * Test Window global object basics.
 */
START_TEST(test_quickjs_window_global)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();

    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_test_document();

    err = js_newthread(heap, (void*)doc, doc, &thread);

    dom_node_unref((dom_node *)doc);
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    /* Test 1: window global exists */
    const char *code1 = "typeof window !== 'undefined'";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_window1");
    ck_assert(result == true);

    /* Test 2: window.self === window (self-reference) */
    const char *code2 = "window.self === window";
    result = js_exec(thread, (const uint8_t *)code2, strlen(code2), "test_window2");
    ck_assert(result == true);

    /* Test 3: window.window === window (self-reference) */
    const char *code3 = "window.window === window";
    result = js_exec(thread, (const uint8_t *)code3, strlen(code3), "test_window3");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

START_TEST(test_quickjs_window_methods)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();

    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_test_document();

    err = js_newthread(heap, (void*)doc, doc, &thread);

    dom_node_unref((dom_node *)doc);
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    /* Test that alert is a function (from Window interface) */
    const char *code1 = "typeof window.alert === 'function'";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_alert");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

START_TEST(test_quickjs_timers)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();

    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_test_document();

    err = js_newthread(heap, (void*)doc, doc, &thread);

    dom_node_unref((dom_node *)doc);
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    /* Test setTimeout exists and returns a number */
    const char *code1 = "typeof setTimeout === 'function'";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_setTimeout");
    ck_assert(result == true);

    /* Test clearTimeout exists */
    const char *code2 = "typeof clearTimeout === 'function'";
    result = js_exec(thread, (const uint8_t *)code2, strlen(code2), "test_clearTimeout");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

START_TEST(test_quickjs_navigator)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();

    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_test_document();

    err = js_newthread(heap, (void*)doc, doc, &thread);

    dom_node_unref((dom_node *)doc);
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    /* Test UserAgent */
    const char *code1 = "typeof navigator === 'object' && navigator.userAgent.length > 0";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_userAgent");
    ck_assert(result == true);

    /* Test cookieEnabled */
    const char *code2 = "navigator.cookieEnabled === true";
    result = js_exec(thread, (const uint8_t *)code2, strlen(code2), "test_cookieEnabled");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

START_TEST(test_quickjs_location)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();

    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_test_document();

    err = js_newthread(heap, (void*)doc, doc, &thread);

    dom_node_unref((dom_node *)doc);
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    /* Test location exists */
    const char *code1 = "typeof location === 'object' && typeof window.location === 'object'";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_location1");
    ck_assert(result == true);

    /* Test href */
    const char *code2 = "typeof location.href === 'string'";
    result = js_exec(thread, (const uint8_t *)code2, strlen(code2), "test_location2");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

START_TEST(test_quickjs_document)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();

    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_test_document();

    err = js_newthread(heap, (void*)doc, doc, &thread);

    dom_node_unref((dom_node *)doc);
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    /* Test document exists */
    const char *code1 = "typeof document === 'object' && typeof window.document === 'object'";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_document1");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

START_TEST(test_quickjs_storage)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();

    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_test_document();

    err = js_newthread(heap, (void*)doc, doc, &thread);

    dom_node_unref((dom_node *)doc);
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    /* Test localStorage operations */
    const char *code1 =
        "try {\n"
        "  if (typeof localStorage !== 'object' || typeof localStorage.getItem !== 'function') throw 'localStorage missing';\n"
        "  localStorage.setItem('mykey', 'myvalue');\n"
        "  if (localStorage.getItem('mykey') !== 'myvalue') throw 'getItem fail';\n"
        "  if (localStorage.length !== 1) throw 'length fail: ' + localStorage.length;\n"
        "  if (localStorage.key(0) !== 'mykey') throw 'key fail';\n"
        "  localStorage.removeItem('mykey');\n"
        "  if (localStorage.getItem('mykey') !== null) throw 'removeItem fail';\n"
        "  if (localStorage.length !== 0) throw 'length empty fail';\n"
        "  localStorage.setItem('k1', 'v1');\n"
        "  localStorage.setItem('k2', 'v2');\n"
        "  localStorage.clear();\n"
        "  if (localStorage.length !== 0) throw 'clear fail';\n"
        "} catch(e) {\n"
        "  console.log('TEST_ERROR:', e);\n"
        "  throw e;\n"
        "}\n"
        "1;";
    JSValue val_storage = js_eval_with_aot_cache(thread->ctx, (const uint8_t *)code1, strlen(code1), "test_localStorage", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(val_storage)) {
        JSValue exc = JS_GetException(thread->ctx);
        const char *exc_str = JS_ToCString(thread->ctx, exc);
        fprintf(stderr, "\n--- EXCEPTION: %s ---\n\n", exc_str ? exc_str : "unknown");
        if (exc_str) JS_FreeCString(thread->ctx, exc_str);
        JS_FreeValue(thread->ctx, exc);
    }
    ck_assert(!JS_IsException(val_storage));
    JS_FreeValue(thread->ctx, val_storage);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

START_TEST(test_quickjs_event_target_basic)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();

    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_test_document();

    err = js_newthread(heap, (void*)doc, doc, &thread);

    dom_node_unref((dom_node *)doc);
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    /* Test addEventListener exists on window */
    const char *code1 = "typeof window.addEventListener === 'function'";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_addEventListener");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

START_TEST(test_quickjs_xhr)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    corestrings_init();
    js_initialise();

    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_test_document();

    err = js_newthread(heap, (void*)doc, doc, &thread);

    dom_node_unref((dom_node *)doc);
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    /* Test XMLHttpRequest constructor and basic state */
    const char *code1 =
        "var xhr = new XMLHttpRequest();\n"
        "var states = [];\n"
        "xhr.onreadystatechange = function() { states.push(xhr.readyState); };\n"
        "xhr.open('GET', 'about:blank');\n"
        "xhr.readyState === 1 && states.length === 1 && states[0] === 1;";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_xhr_basic");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    corestrings_fini();
    js_finalise();
}
END_TEST

START_TEST(test_quickjs_dom_identity)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();
    js_newheap(5, &heap);
    dom_document *doc = create_test_document();
    js_newthread(heap, (void*)doc, doc, &thread);
    dom_node_unref((dom_node *)doc);
    doc = NULL;

    const char *code = "var body1 = document.body; var body2 = document.body; body1 === body2;";
    result = js_exec(thread, (const uint8_t *)code, strlen(code), "test_dom_identity");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

START_TEST(test_quickjs_crypto)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();

    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_test_document();

    err = js_newthread(heap, (void*)doc, doc, &thread);

    dom_node_unref((dom_node *)doc);
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    /* Test crypto object exists */
    const char *code1 = "typeof crypto === 'object' && typeof crypto.subtle === 'object'";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_crypto_exists");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

START_TEST(test_quickjs_dom_attributes)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();
    js_newheap(5, &heap);
    dom_document *doc = create_test_document();
    js_newthread(heap, (void*)doc, doc, &thread);
    dom_node_unref((dom_node *)doc);
    doc = NULL;

    const char *code = "var el = document.createElement('div'); el.className = 'test-class'; el.setAttribute('id', 'test-id'); el.className === 'test-class' && el.getAttribute('id') === 'test-id';";
    result = js_exec(thread, (const uint8_t *)code, strlen(code), "test_dom_attributes");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST


START_TEST(test_quickjs_canvas_imagedata)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    bool result;

    js_initialise();
    js_newheap(5, &heap);
    dom_document *doc = create_test_document();
    js_newthread(heap, (void*)doc, doc, &thread);
    dom_node_unref((dom_node *)doc);
    doc = NULL;

    const char *script = "if (typeof ImageData === 'undefined') throw 'ImageData missing';\n"
                         "let id = new ImageData(10, 10);\n"
                         "if (id.width !== 10) throw 'width fail';\n"
                         "if (id.height !== 10) throw 'height fail';\n"
                         "if (!(id.data instanceof Uint8ClampedArray)) throw 'data fail';\n"
                         "if (id.data.length !== 400) throw 'length fail: ' + id.data.length;\n"
                         "1;";

    result = js_exec(thread, (const uint8_t *)script, strlen(script), "test_canvas_imagedata");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

START_TEST(test_quickjs_observers)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();
    js_newheap(5, &heap);
    dom_document *doc = create_test_document();
    js_newthread(heap, (void*)doc, doc, &thread);
    dom_node_unref((dom_node *)doc);
    doc = NULL;

    /* Test MutationObserver existence and constructor */
    const char *code1 = "typeof MutationObserver === 'function' && typeof (new MutationObserver(() => {})) === 'object'";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_mutation_observer");
    ck_assert(result == true);

    /* Test IntersectionObserver existence and constructor */
    const char *code2 = "typeof IntersectionObserver === 'function' && typeof (new IntersectionObserver(() => {})) === 'object'";
    result = js_exec(thread, (const uint8_t *)code2, strlen(code2), "test_intersection_observer");
    ck_assert(result == true);

    /* Test IntersectionObserver full spec options, getters, threshold validation, and sorting */
    const char *code3 =
        "var options = {\n"
        "  root: document.body,\n"
        "  rootMargin: '10px 20px 30px 40px',\n"
        "  threshold: [0.5, 0.0, 1.0, 0.25]\n"
        "};\n"
        "var observer = new IntersectionObserver(function(entries) {}, options);\n"
        "var rootOk = observer.root === document.body;\n"
        "var marginOk = observer.rootMargin === '10px 20px 30px 40px';\n"
        "var th = observer.thresholds;\n"
        "var thresholdOk = th.length === 4 && th[0] === 0 && th[1] === 0.25 && th[2] === 0.5 && th[3] === 1.0;\n"
        "rootOk && marginOk && thresholdOk;";
    result = js_exec(thread, (const uint8_t *)code3, strlen(code3), "test_intersection_observer_options");
    ck_assert(result == true);

    /* Test threshold validation throwing RangeError */
    const char *code4 =
        "try {\n"
        "  new IntersectionObserver(() => {}, { threshold: [0.5, 1.5] });\n"
        "  false;\n"
        "} catch (e) {\n"
        "  e instanceof RangeError;\n"
        "}";
    result = js_exec(thread, (const uint8_t *)code4, strlen(code4), "test_intersection_observer_range_error");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

START_TEST(test_quickjs_site_isolation)
{
    jsheap *heap = NULL;
    jsthread *thread1 = NULL;
    jsthread *thread2 = NULL;
    nserror err;

    js_initialise();

    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc1 = create_test_document();
    err = js_newthread(heap, (void*)doc1, doc1, &thread1);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc2 = create_test_document();
    err = js_newthread(heap, (void*)doc2, doc2, &thread2);
    ck_assert_int_eq(err, NSERROR_OK);

    ck_assert_ptr_nonnull(thread1->origin);
    ck_assert_ptr_nonnull(thread2->origin);
    ck_assert_str_ne(thread1->origin, thread2->origin);

    js_closethread(thread1);
    js_closethread(thread2);
    js_destroythread(thread1);
    js_destroythread(thread2);
    js_destroyheap(heap);
    if (doc1) dom_node_unref((dom_node *)doc1);
    if (doc2) dom_node_unref((dom_node *)doc2);
    js_finalise();
}
END_TEST

struct mock_task {
    int delay;
    void (*callback)(void *p);
    void *param;
    struct mock_task *next;
};
static struct mock_task *mock_tasks = NULL;

static nserror mock_schedule(int delay, void (*callback)(void *p), void *param)
{
    if (delay < 0) {
        struct mock_task **prev = &mock_tasks;
        struct mock_task *curr = mock_tasks;
        while (curr) {
            if (curr->callback == callback && curr->param == param) {
                *prev = curr->next;
                free(curr);
                return NSERROR_OK;
            }
            prev = &curr->next;
            curr = curr->next;
        }
        return NSERROR_NOT_FOUND;
    }

    struct mock_task *task = malloc(sizeof(*task));
    task->delay = delay;
    task->callback = callback;
    task->param = param;
    task->next = mock_tasks;
    mock_tasks = task;
    return NSERROR_OK;
}

static void run_mock_tasks(void)
{
    struct mock_task *curr = mock_tasks;
    mock_tasks = NULL;
    while (curr) {
        struct mock_task *next = curr->next;
        curr->callback(curr->param);
        free(curr);
        curr = next;
    }
}

static struct gui_misc_table mock_misc = {
    .schedule = mock_schedule
};
static struct wisp_table mock_guit_data = {
    .misc = &mock_misc
};

START_TEST(test_quickjs_queue_microtask_order)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();
    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_test_document();
    err = js_newthread(heap, (void*)doc, doc, &thread);
    dom_node_unref((dom_node *)doc);
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    const char *code1 = "window.order = [];\n"
                        "window.order.push(1);\n"
                        "queueMicrotask(function() { window.order.push(3); });\n"
                        "window.order.push(2);";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_microtask_enqueue");
    ck_assert(result == true);

    const char *code2 = "window.order.length === 3 && window.order[0] === 1 && window.order[1] === 2 && window.order[2] === 3;";
    result = js_exec(thread, (const uint8_t *)code2, strlen(code2), "test_microtask_order");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    js_finalise();
}
END_TEST

START_TEST(test_quickjs_raf)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();
    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_test_document();
    err = js_newthread(heap, (void*)doc, doc, &thread);
    dom_node_unref((dom_node *)doc);
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    struct wisp_table *saved_guit = guit;
    guit = &mock_guit_data;

    const char *code1 = "window.rafTime = 0;\n"
                        "window.rafId = requestAnimationFrame(function(t) { window.rafTime = t; });\n"
                        "window.rafId > 0 && window.rafTime === 0;";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_raf_schedule");
    ck_assert(result == true);

    run_mock_tasks();

    const char *code2 = "window.rafTime > 0;";
    result = js_exec(thread, (const uint8_t *)code2, strlen(code2), "test_raf_executed");
    ck_assert(result == true);

    const char *code3 = "window.rafTime2 = 0;\n"
                        "var id2 = requestAnimationFrame(function(t) { window.rafTime2 = t; });\n"
                        "cancelAnimationFrame(id2);";
    result = js_exec(thread, (const uint8_t *)code3, strlen(code3), "test_raf_cancel");
    ck_assert(result == true);

    run_mock_tasks();

    const char *code4 = "window.rafTime2 === 0;";
    result = js_exec(thread, (const uint8_t *)code4, strlen(code4), "test_raf_not_executed");
    ck_assert(result == true);

    guit = saved_guit;

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    js_finalise();
}
END_TEST

START_TEST(test_quickjs_ric)
{
    jsheap *heap = NULL;
    jsthread *thread = NULL;
    nserror err;
    bool result;

    js_initialise();
    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);

    dom_document *doc = create_test_document();
    err = js_newthread(heap, (void*)doc, doc, &thread);
    dom_node_unref((dom_node *)doc);
    doc = NULL;
    ck_assert_int_eq(err, NSERROR_OK);

    struct wisp_table *saved_guit = guit;
    guit = &mock_guit_data;

    const char *code1 = "window.idleRun = false;\n"
                        "window.didTimeout = null;\n"
                        "window.remaining = null;\n"
                        "window.ricId = requestIdleCallback(function(deadline) {\n"
                        "  window.idleRun = true;\n"
                        "  window.didTimeout = deadline.didTimeout;\n"
                        "  window.remaining = deadline.timeRemaining();\n"
                        "});\n"
                        "window.ricId > 0 && window.idleRun === false;";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_ric_schedule");
    ck_assert(result == true);

    run_mock_tasks();

    const char *code2 = "window.idleRun === true && window.didTimeout === false && window.remaining <= 50;";
    result = js_exec(thread, (const uint8_t *)code2, strlen(code2), "test_ric_executed");
    ck_assert(result == true);

    const char *code3 = "window.idleRun2 = false;\n"
                        "var id2 = requestIdleCallback(function(deadline) { window.idleRun2 = true; });\n"
                        "cancelIdleCallback(id2);";
    result = js_exec(thread, (const uint8_t *)code3, strlen(code3), "test_ric_cancel");
    ck_assert(result == true);

    run_mock_tasks();

    const char *code4 = "window.idleRun2 === false;";
    result = js_exec(thread, (const uint8_t *)code4, strlen(code4), "test_ric_not_executed");
    ck_assert(result == true);

    guit = saved_guit;

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    js_finalise();
}
END_TEST

Suite *quickjs_suite(void)
{
    Suite *s;
    TCase *tc_core;
    TCase *tc_exec;
    TCase *tc_window;
    TCase *tc_console;

    s = suite_create("QuickJS");

    /* Core test case */
    tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_quickjs_init_finalise);
    tcase_add_test(tc_core, test_quickjs_heap_create_destroy);
    tcase_add_test(tc_core, test_quickjs_thread_create_destroy);
    tcase_add_test(tc_core, test_quickjs_multiple_threads);
    tcase_add_test(tc_core, test_quickjs_site_isolation);
    suite_add_tcase(s, tc_core);

    /* Execution test case */
    tc_exec = tcase_create("Execution");
    tcase_add_test(tc_exec, test_quickjs_exec_simple);
    tcase_add_test(tc_exec, test_quickjs_aot_cache);
    tcase_add_test(tc_exec, test_quickjs_json_simd);
    tcase_add_test(tc_exec, test_quickjs_exec_syntax_error);
    tcase_add_test(tc_exec, test_quickjs_exec_objects);
    tcase_add_test(tc_exec, test_quickjs_exec_console_log);
    tcase_add_test(tc_exec, test_quickjs_exec_closed_thread);
    suite_add_tcase(s, tc_exec);

    /* Console binding test case */
    tc_console = tcase_create("Console");
    tcase_add_test(tc_console, test_quickjs_console_init);
    tcase_add_test(tc_console, test_quickjs_console_log);
    tcase_add_test(tc_console, test_quickjs_console_error);
    tcase_add_test(tc_console, test_quickjs_console_warn);
    tcase_add_test(tc_console, test_quickjs_console_multiple_args);
    tcase_add_test(tc_console, test_quickjs_console_group);
    suite_add_tcase(s, tc_console);

    /* Window binding test case */
    tc_window = tcase_create("Window");
    tcase_add_test(tc_window, test_quickjs_window_global);
    tcase_add_test(tc_window, test_quickjs_window_methods);
    tcase_add_test(tc_window, test_quickjs_timers);
    tcase_add_test(tc_window, test_quickjs_navigator);
    tcase_add_test(tc_window, test_quickjs_location);
    tcase_add_test(tc_window, test_quickjs_document);
    tcase_add_test(tc_window, test_quickjs_storage);
    tcase_add_test(tc_window, test_quickjs_dom_parser);
    tcase_add_test(tc_window, test_quickjs_event_target_basic);
    tcase_add_test(tc_window, test_quickjs_event_target_full);
    tcase_add_test(tc_window, test_quickjs_xhr);
    tcase_set_timeout(tc_window, 10);
    tcase_add_test(tc_window, test_quickjs_crypto);
    tcase_add_test(tc_window, test_quickjs_dom_identity);
    tcase_add_test(tc_window, test_quickjs_dom_attributes);
    tcase_add_test(tc_window, test_quickjs_node_stubs);
    tcase_add_test(tc_window, test_quickjs_css_style_declaration);
    tcase_add_test(tc_window, test_quickjs_canvas_imagedata);
    tcase_add_test(tc_window, test_quickjs_observers);
    tcase_add_test(tc_window, test_quickjs_trusted_types);
    suite_add_tcase(s, tc_window);

    /* MutationObserver test case */
    TCase *tc_mutation = tcase_create("MutationObserver");
    tcase_add_test(tc_mutation, test_quickjs_mutation_observer_e2e);
    suite_add_tcase(s, tc_mutation);

    /* Event Loop & Microtask Queue Resolution test case */
    TCase *tc_event_loop = tcase_create("EventLoop");
    tcase_add_test(tc_event_loop, test_quickjs_queue_microtask_order);
    tcase_add_test(tc_event_loop, test_quickjs_raf);
    tcase_add_test(tc_event_loop, test_quickjs_ric);
    suite_add_tcase(s, tc_event_loop);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = quickjs_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
