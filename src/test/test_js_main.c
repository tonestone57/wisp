#include <check.h>
#include <stdbool.h>
#include <stdlib.h>
#include "quickjs.h"
#include "processes/js/js_process.h"
#include "content/handlers/javascript/quickjs/dom_bridge.h"

extern bool wisp_is_js_process;

static void setup(void) {
    wisp_is_js_process = true;
    rt = JS_NewRuntime();
}

static void teardown(void) {
    struct js_context_node *curr = contexts;
    while (curr) {
        struct js_context_node *next = curr->next;
        if (curr->ctx) {
            qjs_finalise_dom_bridge(rt, curr->ctx);
            JS_SetContextOpaque(curr->ctx, NULL);
            JS_FreeContext(curr->ctx);
        }
        if (curr->thread) {
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
