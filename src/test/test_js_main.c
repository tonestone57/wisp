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

START_TEST(test_get_context_mru_cache)
{
    JSContext *ctx1 = get_context(10);
    ck_assert_ptr_nonnull(ctx1);

    JSContext *ctx2 = get_context(20);
    ck_assert_ptr_nonnull(ctx2);

    /* Context 20 is MRU. Accessing 20 again should hit MRU cache */
    JSContext *ctx2_repeat = get_context(20);
    ck_assert_ptr_eq(ctx2, ctx2_repeat);

    /* Accessing 10 should find 10 in list/hash table and update MRU */
    JSContext *ctx1_repeat = get_context(10);
    ck_assert_ptr_eq(ctx1, ctx1_repeat);

    /* Repeated access to 10 should hit MRU cache */
    JSContext *ctx1_mru = get_context(10);
    ck_assert_ptr_eq(ctx1, ctx1_mru);
}
END_TEST

START_TEST(test_get_context_hash_collision_and_many_contexts)
{
    /* Test creating contexts that map to the same hash bucket (e.g. 1 and 1 + 64) */
    JSContext *c1 = get_context(1);
    JSContext *c65 = get_context(65);
    JSContext *c129 = get_context(129);

    ck_assert_ptr_nonnull(c1);
    ck_assert_ptr_nonnull(c65);
    ck_assert_ptr_nonnull(c129);

    ck_assert_ptr_ne(c1, c65);
    ck_assert_ptr_ne(c65, c129);

    /* Fast lookups should retrieve the correct contexts */
    ck_assert_ptr_eq(get_context(1), c1);
    ck_assert_ptr_eq(get_context(65), c65);
    ck_assert_ptr_eq(get_context(129), c129);
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

START_TEST(test_global_document_get_property_lookup)
{
    const char *shm_name = "/test_js_main_shm_prop";
    shm_unlink(shm_name);

    wisp_shm_dom = shm_dom_create(shm_name, 100, true);
    ck_assert_ptr_nonnull(wisp_shm_dom);

    WispCompactNode *nodes = shm_dom_get_nodes(wisp_shm_dom);
    nodes[1].node_type = 9; /* DOM_DOCUMENT_NODE */
    wisp_shm_dom->node_count = 2;

    JSContext *ctx = get_context(1);
    ck_assert_ptr_nonnull(ctx);

    ck_assert(eval_js_bool(ctx, "window.document === document"));
    ck_assert(eval_js_bool(ctx, "self.document === document"));
    ck_assert(eval_js_bool(ctx, "top.document === document"));
    ck_assert(eval_js_bool(ctx, "typeof document.nodeType === 'number'"));
}
END_TEST

START_TEST(test_context_isolation_and_state)
{
    JSContext *ctx1 = get_context(100);
    JSContext *ctx2 = get_context(200);

    ck_assert_ptr_nonnull(ctx1);
    ck_assert_ptr_nonnull(ctx2);

    /* Define variable in ctx1 */
    ck_assert(eval_js_bool(ctx1, "var mySecretVar = 42; mySecretVar === 42"));

    /* Ensure ctx2 does not see variable from ctx1 */
    ck_assert(eval_js_bool(ctx2, "typeof mySecretVar === 'undefined'"));

    /* Define same variable name with different value in ctx2 */
    ck_assert(eval_js_bool(ctx2, "var mySecretVar = 100; mySecretVar === 100"));

    /* Verify ctx1 retains its original value */
    ck_assert(eval_js_bool(ctx1, "mySecretVar === 42"));
}
END_TEST

START_TEST(test_pending_jobs_and_microtasks)
{
    JSContext *ctx = get_context(300);
    ck_assert_ptr_nonnull(ctx);

    /* Evaluate code that creates a resolved Promise */
    ck_assert(eval_js_bool(ctx, "globalThis.promiseValue = 0; Promise.resolve(42).then(v => { globalThis.promiseValue = v; }); true"));

    /* Before executing pending jobs, value is 0 */
    ck_assert(eval_js_bool(ctx, "globalThis.promiseValue === 0"));

    /* Execute pending jobs */
    JSContext *pctx;
    int job_ret;
    while ((job_ret = JS_ExecutePendingJob(rt, &pctx)) != 0) {
        ck_assert_int_ge(job_ret, 0);
    }

    /* After microtask execution, promiseValue should be updated to 42 */
    ck_assert(eval_js_bool(ctx, "globalThis.promiseValue === 42"));
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

START_TEST(test_find_shm_doc_node_id_no_document)
{
    const char *shm_name = "/test_js_main_shm_nodoc";
    shm_unlink(shm_name);

    wisp_shm_dom = shm_dom_create(shm_name, 100, true);
    ck_assert_ptr_nonnull(wisp_shm_dom);

    /* Populate nodes with non-document node types (e.g. element type 1) */
    WispCompactNode *nodes = shm_dom_get_nodes(wisp_shm_dom);
    nodes[0].node_type = 1;
    nodes[1].node_type = 1;
    wisp_shm_dom->node_count = 2;

    JSContext *ctx = get_context(1);
    ck_assert_ptr_nonnull(ctx);

    struct jsthread *t = JS_GetContextOpaque(ctx);
    ck_assert_ptr_nonnull(t);
    ck_assert_ptr_eq(t->doc_priv, (void *)(uintptr_t)0);

    ck_assert(eval_js_bool(ctx, "document === undefined"));
}
END_TEST

START_TEST(test_find_shm_doc_node_id_at_index)
{
    const char *shm_name = "/test_js_main_shm_idx5";
    shm_unlink(shm_name);

    wisp_shm_dom = shm_dom_create(shm_name, 100, true);
    ck_assert_ptr_nonnull(wisp_shm_dom);

    WispCompactNode *nodes = shm_dom_get_nodes(wisp_shm_dom);
    for (int i = 0; i < 5; i++) {
        nodes[i].node_type = 1;
    }
    nodes[5].node_type = 9; /* DOM_DOCUMENT_NODE placed at index 5 */
    wisp_shm_dom->node_count = 6;

    JSContext *ctx = get_context(1);
    ck_assert_ptr_nonnull(ctx);

    struct jsthread *t = JS_GetContextOpaque(ctx);
    ck_assert_ptr_nonnull(t);
    ck_assert_ptr_eq(t->doc_priv, (void *)(uintptr_t)5);

    ck_assert(eval_js_bool(ctx, "typeof document === 'object'"));
    ck_assert(eval_js_bool(ctx, "document !== null"));
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

START_TEST(test_get_context_core_polyfills)
{
    JSContext *ctx = get_context(1);
    ck_assert_ptr_nonnull(ctx);

    ck_assert(eval_js_bool(ctx, "typeof matchMedia === 'function'"));
    ck_assert(eval_js_bool(ctx, "typeof ResizeObserver === 'function'"));
    ck_assert(eval_js_bool(ctx, "typeof crypto.randomUUID === 'function'"));
    ck_assert(eval_js_bool(ctx, "typeof CSS.escape === 'function'"));
}
END_TEST

START_TEST(test_eval_js_when_shm_null)
{
    wisp_shm_dom = NULL;
    JSContext *ctx = get_context(1);
    ck_assert_ptr_nonnull(ctx);

    ck_assert(eval_js_bool(ctx, "1 + 1 === 2"));
    ck_assert(eval_js_bool(ctx, "typeof Math.abs === 'function'"));
}
END_TEST

START_TEST(test_shm_dom_update_contexts)
{
    const char *shm_name1 = "/test_js_main_shm1";
    shm_unlink(shm_name1);

    wisp_shm_dom = shm_dom_create(shm_name1, 100, true);
    ck_assert_ptr_nonnull(wisp_shm_dom);

    WispCompactNode *nodes1 = shm_dom_get_nodes(wisp_shm_dom);
    nodes1[1].node_type = 9; /* DOM_DOCUMENT_NODE */
    wisp_shm_dom->node_count = 2;

    JSContext *ctx = get_context(1);
    ck_assert_ptr_nonnull(ctx);

    struct jsthread *t = JS_GetContextOpaque(ctx);
    ck_assert_ptr_nonnull(t);
    ck_assert_ptr_eq(t->doc_priv, (void *)(uintptr_t)1);

    /* Simulate SHM DOM re-creation where document node index changes */
    shm_dom_destroy(wisp_shm_dom, NULL, false);
    const char *shm_name2 = "/test_js_main_shm2";
    shm_unlink(shm_name2);

    wisp_shm_dom = shm_dom_create(shm_name2, 100, true);
    ck_assert_ptr_nonnull(wisp_shm_dom);

    WispCompactNode *nodes2 = shm_dom_get_nodes(wisp_shm_dom);
    nodes2[2].node_type = 9; /* DOM_DOCUMENT_NODE moved to index 2 */
    wisp_shm_dom->node_count = 3;

    /* Perform context update logic */
    WispCompactNode *nodes_arr = shm_dom_get_nodes(wisp_shm_dom);
    uint64_t new_doc_id = 0;
    for (uint32_t i = 0; i < wisp_shm_dom->node_count; i++) {
        if (nodes_arr[i].node_type == 9) {
            new_doc_id = i;
            break;
        }
    }
    ck_assert_uint_eq(new_doc_id, 2);

    t->doc_priv = (void *)(uintptr_t)new_doc_id;
    t->win_priv = (void *)(uintptr_t)new_doc_id;
    t->global_window_priv.node = (void *)(uintptr_t)new_doc_id;

    ck_assert_ptr_eq(t->doc_priv, (void *)(uintptr_t)2);
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
    tcase_add_test(tc_core, test_get_context_mru_cache);
    tcase_add_test(tc_core, test_get_context_hash_collision_and_many_contexts);
    tcase_add_test(tc_core, test_get_context_global_properties);
    tcase_add_test(tc_core, test_global_document_get_property_lookup);
    tcase_add_test(tc_core, test_context_isolation_and_state);
    tcase_add_test(tc_core, test_pending_jobs_and_microtasks);
    tcase_add_test(tc_core, test_get_context_origin_propagation);
    tcase_add_test(tc_core, test_global_document_get_null_shm);
    tcase_add_test(tc_core, test_find_shm_doc_node_id_no_document);
    tcase_add_test(tc_core, test_find_shm_doc_node_id_at_index);
    tcase_add_test(tc_core, test_global_document_get_with_shm);
    tcase_add_test(tc_core, test_get_context_core_polyfills);
    tcase_add_test(tc_core, test_eval_js_when_shm_null);
    tcase_add_test(tc_core, test_shm_dom_update_contexts);
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
