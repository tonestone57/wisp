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

#include "content/handlers/javascript/js.h"
#include <dom/core/implementation.h>
#include <dom/core/document.h>
#include <dom/core/node.h>
#include <dom/core/element.h>

/* Include QuickJS directly for console binding tests */
#include "content/handlers/javascript/quickjs/dom_bridge.h"
#include "quickjs.h"
#include "utils/hashmap.h"

static dom_document *create_test_document(void)
{
    dom_document *doc;
    dom_exception err;
    err = dom_implementation_create_document(DOM_IMPLEMENTATION_XML, NULL, (const uint8_t *)"html", NULL, NULL, NULL, &doc);
    if (err != DOM_NO_ERR) return NULL;

    dom_string *body_s;
    struct dom_element *el;
    dom_string_create((const uint8_t *)"body", 4, &body_s);
    dom_document_create_element(doc, body_s, &el);
    dom_node_append_child((dom_node *)doc, (dom_node *)el, NULL);
    dom_node_unref((dom_node *)el);
    dom_string_unref(body_s);

    return doc;
}

START_TEST(test_quickjs_init_finalise)
{
    js_initialise();
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
    err = js_newheap(5, &heap);
    ck_assert_int_eq(err, NSERROR_OK);
    dom_document *doc = create_test_document();
    err = js_newthread(heap, (void*)doc, doc, &thread);
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
        "div.appendChild(document.createTextNode('hello'));\n"
        "span.appendChild(document.createTextNode('world'));\n"
        "span.firstChild.data = 'new world';\n"
        "// At this point records should still be empty because it's a microtask\n"
        "if (records.length !== 0) throw new Error('Not asynchronous');\n"
        "var taken = observer.takeRecords();\n"
        "// div: attr, childList. span: childList (for 'world'), characterData (for 'new world')\n"
        "taken.length === 4 && \n"
        "taken[0].type === 'attributes' && taken[0].target === div &&\n"
        "taken[1].type === 'childList' && taken[1].target === div &&\n"
        "taken[2].type === 'childList' && taken[2].target === span &&\n"
        "taken[3].type === 'characterData' && taken[3].target === span.firstChild;";

    result = js_exec(thread, (const uint8_t *)code, strlen(code), "test_mutation_observer_e2e");
    ck_assert(result == true);

    js_closethread(thread);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

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
    err = js_closethread(thread);
    ck_assert_int_eq(err, NSERROR_OK);
    const char *code = "1 + 1";
    result = js_exec(thread, (const uint8_t *)code, strlen(code), "test");
    ck_assert(result == false);
    js_destroythread(thread);
    js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc);
    js_finalise();
}
END_TEST

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
    dom_document *doc1 = create_test_document();
    err = js_newthread(heap, (void*)doc1, doc1, &thread1);
    dom_document *doc2 = create_test_document();
    err = js_newthread(heap, (void*)doc2, doc2, &thread2);
    const char *code = "var x = 1;";
    result = js_exec(thread1, (const uint8_t *)code, strlen(code), "test1");
    ck_assert(result == true);
    result = js_exec(thread2, (const uint8_t *)code, strlen(code), "test2");
    ck_assert(result == true);
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

START_TEST(test_quickjs_console_init)
{
    JSRuntime *rt; JSContext *ctx; int ret;
    rt = JS_NewRuntime(); ctx = JS_NewContext(rt);
    ret = qjs_init_dom_bridge(ctx); qjs_init_console(ctx);
    ck_assert_int_eq(ret, 0);
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue console = JS_GetPropertyStr(ctx, global, "console");
    ck_assert(JS_IsObject(console));
    JS_FreeValue(ctx, console); JS_FreeValue(ctx, global);
    JS_FreeContext(ctx); qjs_bridge_cleanup(rt); JS_FreeRuntime(rt);
}
END_TEST

START_TEST(test_quickjs_console_log)
{
    JSRuntime *rt; JSContext *ctx; JSValue result;
    rt = JS_NewRuntime(); ctx = JS_NewContext(rt);
    qjs_init_dom_bridge(ctx); qjs_init_console(ctx);
    const char *code = "console.log('Hello from QuickJS!');";
    result = JS_Eval(ctx, code, strlen(code), "test", JS_EVAL_TYPE_GLOBAL);
    ck_assert(!JS_IsException(result));
    JS_FreeValue(ctx, result); JS_FreeContext(ctx); qjs_bridge_cleanup(rt); JS_FreeRuntime(rt);
}
END_TEST

START_TEST(test_quickjs_console_error)
{
    JSRuntime *rt; JSContext *ctx; JSValue result;
    rt = JS_NewRuntime(); ctx = JS_NewContext(rt);
    qjs_init_dom_bridge(ctx); qjs_init_console(ctx);
    const char *code = "console.error('Test error message');";
    result = JS_Eval(ctx, code, strlen(code), "test", JS_EVAL_TYPE_GLOBAL);
    ck_assert(!JS_IsException(result));
    JS_FreeValue(ctx, result); JS_FreeContext(ctx); qjs_bridge_cleanup(rt); JS_FreeRuntime(rt);
}
END_TEST

START_TEST(test_quickjs_console_warn)
{
    JSRuntime *rt; JSContext *ctx; JSValue result;
    rt = JS_NewRuntime(); ctx = JS_NewContext(rt);
    qjs_init_dom_bridge(ctx); qjs_init_console(ctx);
    const char *code = "console.warn('Test warning');";
    result = JS_Eval(ctx, code, strlen(code), "test", JS_EVAL_TYPE_GLOBAL);
    ck_assert(!JS_IsException(result));
    JS_FreeValue(ctx, result); JS_FreeContext(ctx); qjs_bridge_cleanup(rt); JS_FreeRuntime(rt);
}
END_TEST

START_TEST(test_quickjs_console_multiple_args)
{
    JSRuntime *rt; JSContext *ctx; JSValue result;
    rt = JS_NewRuntime(); ctx = JS_NewContext(rt);
    qjs_init_dom_bridge(ctx); qjs_init_console(ctx);
    const char *code = "console.log('Value:', 42, 'Name:', 'test');";
    result = JS_Eval(ctx, code, strlen(code), "test", JS_EVAL_TYPE_GLOBAL);
    ck_assert(!JS_IsException(result));
    JS_FreeValue(ctx, result); JS_FreeContext(ctx); qjs_bridge_cleanup(rt); JS_FreeRuntime(rt);
}
END_TEST

START_TEST(test_quickjs_console_group)
{
    JSRuntime *rt; JSContext *ctx; JSValue result;
    rt = JS_NewRuntime(); ctx = JS_NewContext(rt);
    qjs_init_dom_bridge(ctx); qjs_init_console(ctx);
    const char *code = "console.group();\nconsole.log('Grouped message');\nconsole.groupEnd();";
    result = JS_Eval(ctx, code, strlen(code), "test", JS_EVAL_TYPE_GLOBAL);
    ck_assert(!JS_IsException(result));
    JS_FreeValue(ctx, result); JS_FreeContext(ctx); qjs_bridge_cleanup(rt); JS_FreeRuntime(rt);
}
END_TEST

START_TEST(test_quickjs_window_global)
{
    jsheap *heap = NULL; jsthread *thread = NULL; nserror err; bool result;
    js_initialise(); err = js_newheap(5, &heap); dom_document *doc = create_test_document();
    err = js_newthread(heap, (void*)doc, doc, &thread);
    const char *code1 = "typeof window !== 'undefined'";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_window1");
    ck_assert(result == true);
    const char *code2 = "window.self === window";
    result = js_exec(thread, (const uint8_t *)code2, strlen(code2), "test_window2");
    ck_assert(result == true);
    const char *code3 = "window.window === window";
    result = js_exec(thread, (const uint8_t *)code3, strlen(code3), "test_window3");
    ck_assert(result == true);
    js_closethread(thread); js_destroythread(thread); js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc); js_finalise();
}
END_TEST

START_TEST(test_quickjs_window_methods)
{
    jsheap *heap = NULL; jsthread *thread = NULL; nserror err; bool result;
    js_initialise(); err = js_newheap(5, &heap); dom_document *doc = create_test_document();
    err = js_newthread(heap, (void*)doc, doc, &thread);
    const char *code1 = "typeof window.alert === 'function'";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_alert");
    ck_assert(result == true);
    js_closethread(thread); js_destroythread(thread); js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc); js_finalise();
}
END_TEST

START_TEST(test_quickjs_timers)
{
    jsheap *heap = NULL; jsthread *thread = NULL; nserror err; bool result;
    js_initialise(); err = js_newheap(5, &heap); dom_document *doc = create_test_document();
    err = js_newthread(heap, (void*)doc, doc, &thread);
    const char *code1 = "typeof setTimeout === 'function'";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_setTimeout");
    ck_assert(result == true);
    const char *code2 = "typeof clearTimeout === 'function'";
    result = js_exec(thread, (const uint8_t *)code2, strlen(code2), "test_clearTimeout");
    ck_assert(result == true);
    js_closethread(thread); js_destroythread(thread); js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc); js_finalise();
}
END_TEST

START_TEST(test_quickjs_navigator)
{
    jsheap *heap = NULL; jsthread *thread = NULL; nserror err; bool result;
    js_initialise(); err = js_newheap(5, &heap); dom_document *doc = create_test_document();
    err = js_newthread(heap, (void*)doc, doc, &thread);
    const char *code1 = "typeof navigator === 'object' && navigator.userAgent.length > 0";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_userAgent");
    ck_assert(result == true);
    const char *code2 = "navigator.cookieEnabled === true";
    result = js_exec(thread, (const uint8_t *)code2, strlen(code2), "test_cookieEnabled");
    ck_assert(result == true);
    js_closethread(thread); js_destroythread(thread); js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc); js_finalise();
}
END_TEST

START_TEST(test_quickjs_location)
{
    jsheap *heap = NULL; jsthread *thread = NULL; nserror err; bool result;
    js_initialise(); err = js_newheap(5, &heap); dom_document *doc = create_test_document();
    err = js_newthread(heap, (void*)doc, doc, &thread);
    const char *code1 = "typeof location === 'object' && typeof window.location === 'object'";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_location1");
    ck_assert(result == true);
    const char *code2 = "typeof location.href === 'string'";
    result = js_exec(thread, (const uint8_t *)code2, strlen(code2), "test_location2");
    ck_assert(result == true);
    js_closethread(thread); js_destroythread(thread); js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc); js_finalise();
}
END_TEST

START_TEST(test_quickjs_document)
{
    jsheap *heap = NULL; jsthread *thread = NULL; nserror err; bool result;
    js_initialise(); err = js_newheap(5, &heap); dom_document *doc = create_test_document();
    err = js_newthread(heap, (void*)doc, doc, &thread);
    const char *code1 = "typeof document === 'object' && typeof window.document === 'object'";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_document1");
    ck_assert(result == true);
    js_closethread(thread); js_destroythread(thread); js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc); js_finalise();
}
END_TEST

START_TEST(test_quickjs_storage)
{
    jsheap *heap = NULL; jsthread *thread = NULL; nserror err; bool result;
    js_initialise(); err = js_newheap(5, &heap); dom_document *doc = create_test_document();
    err = js_newthread(heap, (void*)doc, doc, &thread);
    const char *code1 = "typeof localStorage === 'object' && typeof localStorage.getItem === 'function'";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_localStorage");
    ck_assert(result == true);
    js_closethread(thread); js_destroythread(thread); js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc); js_finalise();
}
END_TEST

START_TEST(test_quickjs_event_target_basic)
{
    jsheap *heap = NULL; jsthread *thread = NULL; nserror err; bool result;
    js_initialise(); err = js_newheap(5, &heap); dom_document *doc = create_test_document();
    err = js_newthread(heap, (void*)doc, doc, &thread);
    const char *code1 = "typeof window.addEventListener === 'function'";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_addEventListener");
    ck_assert(result == true);
    js_closethread(thread); js_destroythread(thread); js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc); js_finalise();
}
END_TEST

START_TEST(test_quickjs_xhr)
{
    jsheap *heap = NULL; jsthread *thread = NULL; nserror err; bool result;
    js_initialise(); err = js_newheap(5, &heap); dom_document *doc = create_test_document();
    err = js_newthread(heap, (void*)doc, doc, &thread);
    const char *code1 = "typeof XMLHttpRequest === 'function'";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_xhr_ctor");
    ck_assert(result == true);
    js_closethread(thread); js_destroythread(thread); js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc); js_finalise();
}
END_TEST

START_TEST(test_quickjs_dom_identity)
{
    jsheap *heap = NULL; jsthread *thread = NULL; nserror err; bool result;
    js_initialise(); js_newheap(5, &heap); dom_document *doc = create_test_document();
    js_newthread(heap, (void*)doc, doc, &thread);
    const char *code = "var body1 = document.body; var body2 = document.body; body1 === body2;";
    result = js_exec(thread, (const uint8_t *)code, strlen(code), "test_dom_identity");
    ck_assert(result == true);
    js_closethread(thread); js_destroythread(thread); js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc); js_finalise();
}
END_TEST

START_TEST(test_quickjs_crypto)
{
    jsheap *heap = NULL; jsthread *thread = NULL; nserror err; bool result;
    js_initialise(); err = js_newheap(5, &heap); dom_document *doc = create_test_document();
    err = js_newthread(heap, (void*)doc, doc, &thread);
    const char *code1 = "typeof crypto === 'object' && typeof crypto.subtle === 'object'";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_crypto_exists");
    ck_assert(result == true);
    js_closethread(thread); js_destroythread(thread); js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc); js_finalise();
}
END_TEST

START_TEST(test_quickjs_dom_attributes)
{
    jsheap *heap = NULL; jsthread *thread = NULL; nserror err; bool result;
    js_initialise(); js_newheap(5, &heap); dom_document *doc = create_test_document();
    js_newthread(heap, (void*)doc, doc, &thread);
    const char *code = "var el = document.createElement('div'); el.className = 'test-class'; el.setAttribute('id', 'test-id'); el.className === 'test-class' && el.getAttribute('id') === 'test-id';";
    result = js_exec(thread, (const uint8_t *)code, strlen(code), "test_dom_attributes");
    ck_assert(result == true);
    js_closethread(thread); js_destroythread(thread); js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc); js_finalise();
}
END_TEST

START_TEST(test_quickjs_observers)
{
    jsheap *heap = NULL; jsthread *thread = NULL; nserror err; bool result;
    js_initialise(); js_newheap(5, &heap); dom_document *doc = create_test_document();
    js_newthread(heap, (void*)doc, doc, &thread);
    const char *code1 = "typeof MutationObserver === 'function' && typeof (new MutationObserver(() => {})) === 'object'";
    result = js_exec(thread, (const uint8_t *)code1, strlen(code1), "test_mutation_observer");
    ck_assert(result == true);
    const char *code2 = "typeof IntersectionObserver === 'function' && typeof (new IntersectionObserver(() => {})) === 'object'";
    result = js_exec(thread, (const uint8_t *)code2, strlen(code2), "test_intersection_observer");
    ck_assert(result == true);
    js_closethread(thread); js_destroythread(thread); js_destroyheap(heap);
    if (doc) dom_node_unref((dom_node *)doc); js_finalise();
}
END_TEST

Suite *quickjs_suite(void)
{
    Suite *s; TCase *tc_core; TCase *tc_exec; TCase *tc_window; TCase *tc_console;
    s = suite_create("QuickJS");
    tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_quickjs_init_finalise);
    tcase_add_test(tc_core, test_quickjs_heap_create_destroy);
    tcase_add_test(tc_core, test_quickjs_thread_create_destroy);
    tcase_add_test(tc_core, test_quickjs_multiple_threads);
    suite_add_tcase(s, tc_core);
    tc_exec = tcase_create("Execution");
    tcase_add_test(tc_exec, test_quickjs_exec_simple);
    tcase_add_test(tc_exec, test_quickjs_exec_syntax_error);
    tcase_add_test(tc_exec, test_quickjs_exec_objects);
    tcase_add_test(tc_exec, test_quickjs_exec_console_log);
    tcase_add_test(tc_exec, test_quickjs_exec_closed_thread);
    suite_add_tcase(s, tc_exec);
    tc_console = tcase_create("Console");
    tcase_add_test(tc_console, test_quickjs_console_init);
    tcase_add_test(tc_console, test_quickjs_console_log);
    tcase_add_test(tc_console, test_quickjs_console_error);
    tcase_add_test(tc_console, test_quickjs_console_warn);
    tcase_add_test(tc_console, test_quickjs_console_multiple_args);
    tcase_add_test(tc_console, test_quickjs_console_group);
    suite_add_tcase(s, tc_console);
    tc_window = tcase_create("Window");
    tcase_add_test(tc_window, test_quickjs_window_global);
    tcase_add_test(tc_window, test_quickjs_window_methods);
    tcase_add_test(tc_window, test_quickjs_timers);
    tcase_add_test(tc_window, test_quickjs_navigator);
    tcase_add_test(tc_window, test_quickjs_location);
    tcase_add_test(tc_window, test_quickjs_document);
    tcase_add_test(tc_window, test_quickjs_storage);
    tcase_add_test(tc_window, test_quickjs_event_target_basic);
    tcase_add_test(tc_window, test_quickjs_event_target_full);
    tcase_add_test(tc_window, test_quickjs_xhr);
    tcase_add_test(tc_window, test_quickjs_crypto);
    tcase_add_test(tc_window, test_quickjs_dom_identity);
    tcase_add_test(tc_window, test_quickjs_dom_attributes);
    tcase_add_test(tc_window, test_quickjs_observers);
    suite_add_tcase(s, tc_window);
    TCase *tc_mutation = tcase_create("MutationObserver");
    tcase_add_test(tc_mutation, test_quickjs_mutation_observer_e2e);
    suite_add_tcase(s, tc_mutation);
    return s;
}

int main(void)
{
    int number_failed; Suite *s; SRunner *sr;
    s = quickjs_suite(); sr = srunner_create(s);
    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
