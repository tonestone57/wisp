#include <check.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "quickjs.h"
#include "processes/js/js_process.h"
#include "content/handlers/javascript/quickjs/dom_bridge.h"
#include "content/handlers/javascript/quickjs/qjs_internal.h"
#include <wisp/utils/shm_dom.h>

extern bool wisp_is_js_process;
extern shm_dom_t *wisp_shm_dom;

static bool eval_js_bool(JSContext *ctx, const char *code)
{
    JSValue val = JS_Eval(ctx, code, strlen(code), "<test>", JS_EVAL_TYPE_GLOBAL);
    bool result = JS_ToBool(ctx, val);
    JS_FreeValue(ctx, val);
    return result;
}

static void setup(void)
{
    wisp_is_js_process = true;
    rt = JS_NewRuntime();
}

static void teardown(void)
{
    if (js_process_origin) {
        free(js_process_origin);
        js_process_origin = NULL;
    }
    if (wisp_shm_dom) {
        shm_dom_destroy(wisp_shm_dom, NULL, false);
        wisp_shm_dom = NULL;
    }
    struct js_context_node *curr = contexts;
    while (curr) {
        struct js_context_node *next = curr->next;
        if (curr->ctx) {
            qjs_finalise_dom_bridge(rt, curr->ctx);
            JS_SetContextOpaque(curr->ctx, NULL);
            JS_FreeContext(curr->ctx);
        }
        if (curr->thread) {
            if (curr->thread->origin) {
                free(curr->thread->origin);
            }
            free(curr->thread);
        }
        free(curr);
        curr = next;
    }
    contexts = NULL;
    if (rt) {
        qjs_bridge_cleanup(rt);
        JS_FreeRuntime(rt);
        rt = NULL;
    }
}

START_TEST(test_get_context_creates_new)
{
    JSContext *ctx = get_context(1);
    ck_assert_ptr_nonnull(ctx);
    ck_assert_ptr_nonnull(contexts);
    ck_assert_int_eq(contexts->id, 1);
    ck_assert_ptr_eq(contexts->ctx, ctx);
}
END_TEST

START_TEST(test_get_context_returns_existing)
{
    JSContext *ctx1 = get_context(1);
    ck_assert_ptr_nonnull(ctx1);

    JSContext *ctx2 = get_context(1);
    ck_assert_ptr_eq(ctx1, ctx2);

    // contexts should only have one item since we didn't add more
    ck_assert_ptr_null(contexts->next);
}
END_TEST

START_TEST(test_get_context_creates_multiple)
{
    JSContext *ctx1 = get_context(1);
    ck_assert_ptr_nonnull(ctx1);

    JSContext *ctx2 = get_context(2);
    ck_assert_ptr_nonnull(ctx2);
    ck_assert_ptr_ne(ctx1, ctx2);

    ck_assert_ptr_nonnull(contexts);
    ck_assert_int_eq(contexts->id, 2);
    ck_assert_ptr_nonnull(contexts->next);
    ck_assert_int_eq(contexts->next->id, 1);
}
END_TEST

START_TEST(test_get_context_global_properties)
{
    JSContext *ctx = get_context(1);
    ck_assert_ptr_nonnull(ctx);

    ck_assert(eval_js_bool(ctx, "globalThis.__wisp_is_js_process === true"));
    ck_assert(eval_js_bool(ctx, "window === globalThis"));
    ck_assert(eval_js_bool(ctx, "self === globalThis"));
    ck_assert(eval_js_bool(ctx, "parent === globalThis"));
    ck_assert(eval_js_bool(ctx, "top === globalThis"));
    ck_assert(eval_js_bool(ctx, "frames === globalThis"));
}
END_TEST

START_TEST(test_get_context_origin_propagation)
{
    js_process_origin = strdup("https://example.com");
    JSContext *ctx = get_context(1);
    ck_assert_ptr_nonnull(ctx);

    struct jsthread *t = JS_GetContextOpaque(ctx);
    ck_assert_ptr_nonnull(t);
    ck_assert_ptr_nonnull(t->origin);
    ck_assert_str_eq(t->origin, "https://example.com");
}
END_TEST

START_TEST(test_global_document_get_null_shm)
{
    wisp_shm_dom = NULL;
    JSContext *ctx = get_context(1);
    ck_assert_ptr_nonnull(ctx);

    ck_assert(eval_js_bool(ctx, "document === undefined"));
}
END_TEST

START_TEST(test_global_document_get_with_shm)
{
    const char *shm_name = "/test_js_main_shm";
    shm_unlink(shm_name);

    wisp_shm_dom = shm_dom_create(shm_name, 100, true);
    ck_assert_ptr_nonnull(wisp_shm_dom);

    WispCompactNode *nodes = shm_dom_get_nodes(wisp_shm_dom);
    nodes[1].node_type = 9; /* DOM_DOCUMENT_NODE */
    wisp_shm_dom->node_count = 2;

    JSContext *ctx = get_context(1);
    ck_assert_ptr_nonnull(ctx);

    struct jsthread *t = JS_GetContextOpaque(ctx);
    ck_assert_ptr_nonnull(t);
    ck_assert_ptr_eq(t->doc_priv, (void *)(uintptr_t)1);

    ck_assert(eval_js_bool(ctx, "typeof document === 'object'"));
    ck_assert(eval_js_bool(ctx, "document !== null"));
}
END_TEST

Suite *js_main_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("JS Main");

    tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_get_context_creates_new);
    tcase_add_test(tc_core, test_get_context_returns_existing);
    tcase_add_test(tc_core, test_get_context_creates_multiple);
    tcase_add_test(tc_core, test_get_context_global_properties);
    tcase_add_test(tc_core, test_get_context_origin_propagation);
    tcase_add_test(tc_core, test_global_document_get_null_shm);
    tcase_add_test(tc_core, test_global_document_get_with_shm);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = js_main_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
