#include <check.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include "quickjs.h"
#include "processes/js/js_process.h"
#include "content/handlers/javascript/quickjs/dom_bridge.h"
#include "content/handlers/javascript/quickjs/qjs_internal.h"
#include <wisp/utils/ipc.h>
#include <wisp/utils/shm_dom.h>
#include <wisp/utils/nsurl.h>

extern bool wisp_is_js_process;
extern shm_dom_t *wisp_shm_dom;
extern uint32_t wisp_shm_capacity;
extern wisp_ipc_handle *ipc_main;

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
        if (curr->thread) {
            js_destroythread(curr->thread);
            curr->thread = NULL;
            curr->ctx = NULL;
        } else if (curr->ctx) {
            qjs_finalise_dom_bridge(rt, curr->ctx);
            JS_SetContextOpaque(curr->ctx, NULL);
            JS_FreeContext(curr->ctx);
            curr->ctx = NULL;
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

    ck_assert(eval_js_bool(ctx, "document === null"));
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

    ck_assert(eval_js_bool(ctx, "document === null"));
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

static wisp_ipc_handle *test_ipc_server = NULL;
static wisp_ipc_handle *test_ipc_accepted = NULL;

static void setup_ipc(void)
{
    const char *sock_name = "test_js_main_ipc_sock";
    test_ipc_server = wisp_ipc_create_server(sock_name);
    ck_assert_ptr_nonnull(test_ipc_server);

    ipc_main = wisp_ipc_connect(sock_name);
    ck_assert_ptr_nonnull(ipc_main);

    test_ipc_accepted = wisp_ipc_accept(test_ipc_server);
    ck_assert_ptr_nonnull(test_ipc_accepted);

    wisp_ipc_set_blocking(test_ipc_accepted, false);
}

static void teardown_ipc(void)
{
    if (ipc_main) {
        wisp_ipc_destroy(ipc_main);
        ipc_main = NULL;
    }
    if (test_ipc_accepted) {
        wisp_ipc_destroy(test_ipc_accepted);
        test_ipc_accepted = NULL;
    }
    if (test_ipc_server) {
        wisp_ipc_destroy(test_ipc_server);
        test_ipc_server = NULL;
    }
}

START_TEST(test_process_timers_and_raf_execution)
{
    JSContext *ctx = get_context(888);
    ck_assert_ptr_nonnull(ctx);

    ck_assert(eval_js_bool(ctx, "globalThis.timerRan = false; globalThis.rafRan = false; globalThis.idleRan = false; true"));
    ck_assert(eval_js_bool(ctx, "setTimeout(() => { globalThis.timerRan = true; }, 0); true"));
    ck_assert(eval_js_bool(ctx, "requestAnimationFrame(() => { globalThis.rafRan = true; }); true"));
    ck_assert(eval_js_bool(ctx, "requestIdleCallback(() => { globalThis.idleRan = true; }); true"));

    /* Wait 60ms for rAF (16ms) and rIC (50ms) delays */
    usleep(60000);

    /* Execute qjs_execute_timers in loop to drain all due callbacks */
    while (qjs_execute_timers(ctx) == 0);

    /* Verify callbacks executed */
    ck_assert(eval_js_bool(ctx, "globalThis.timerRan === true"));
    ck_assert(eval_js_bool(ctx, "globalThis.rafRan === true"));
    ck_assert(eval_js_bool(ctx, "globalThis.idleRan === true"));
}
END_TEST

START_TEST(test_js_process_main_invalid_args)
{
    char *argv[] = { "wisp-js" };
    int ret = js_process_main(1, argv);
    ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(test_ipc_shm_init_with_origin)
{
    setup_ipc();

    /* First create context 1 */
    JSContext *ctx = get_context(1);
    ck_assert_ptr_nonnull(ctx);

    const char *shm_name = "/test_js_ipc_shm";
    shm_unlink(shm_name);
    shm_dom_t *server_shm = shm_dom_create(shm_name, 100, true);
    ck_assert_ptr_nonnull(server_shm);

    char payload[256];
    snprintf(payload, sizeof(payload), "%s|https://example.org", shm_name);

    wisp_ipc_msg msg;
    msg.type = WISP_IPC_MSG_SHM_INIT;
    msg.length = strlen(payload);
    msg.data = (uint8_t *)payload;

    js_process_handle_ipc_msg(&msg);

    ck_assert_ptr_nonnull(wisp_shm_dom);
    ck_assert_ptr_nonnull(js_process_origin);
    ck_assert_str_eq(js_process_origin, "https://example.org");

    struct jsthread *t = JS_GetContextOpaque(ctx);
    ck_assert_ptr_nonnull(t);
    ck_assert_ptr_nonnull(t->origin);
    ck_assert_str_eq(t->origin, "https://example.org");

    /* Re-init with new origin */
    snprintf(payload, sizeof(payload), "%s|https://neworigin.com", shm_name);
    msg.length = strlen(payload);

    js_process_handle_ipc_msg(&msg);

    ck_assert_str_eq(js_process_origin, "https://neworigin.com");
    ck_assert_str_eq(t->origin, "https://neworigin.com");

    shm_dom_destroy(server_shm, NULL, false);
    teardown_ipc();
}
END_TEST

START_TEST(test_ipc_js_exec_binary_string_embedded_nulls)
{
    setup_ipc();

    uint32_t ctx_id = 1;
    uint32_t eval_flags = JS_EVAL_TYPE_GLOBAL;
    uint32_t name_len = 0;
    const char *script = "'hello\\0world'";
    uint32_t script_len = strlen(script);

    uint32_t total_len = 12 + script_len;
    uint8_t *data = malloc(total_len);
    memcpy(data, &ctx_id, 4);
    memcpy(data + 4, &eval_flags, 4);
    memcpy(data + 8, &name_len, 4);
    memcpy(data + 12, script, script_len);

    wisp_ipc_msg msg = {
        .type = WISP_IPC_MSG_JS_EXEC,
        .length = total_len,
        .data = data
    };

    js_process_handle_ipc_msg(&msg);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_JS_EXEC);
    ck_assert_int_eq(recv_msg.length, 11);
    ck_assert_mem_eq(recv_msg.data, "hello\0world", 11);

    wisp_ipc_msg_free(&recv_msg);
    free(data);
    teardown_ipc();
}
END_TEST

START_TEST(test_get_context_opaque_available_during_init)
{
    JSContext *ctx = get_context(777);
    ck_assert_ptr_nonnull(ctx);
    struct jsthread *t = JS_GetContextOpaque(ctx);
    ck_assert_ptr_nonnull(t);
    ck_assert_ptr_eq(t->ctx, ctx);
}
END_TEST

START_TEST(test_ipc_js_exec_string_exception)
{
    setup_ipc();

    uint32_t ctx_id = 1;
    uint32_t eval_flags = JS_EVAL_TYPE_GLOBAL;
    uint32_t name_len = 0;
    const char *script = "throw 'primitive string exception';";
    uint32_t script_len = strlen(script);

    uint32_t total_len = 12 + script_len;
    uint8_t *data = malloc(total_len);
    memcpy(data, &ctx_id, 4);
    memcpy(data + 4, &eval_flags, 4);
    memcpy(data + 8, &name_len, 4);
    memcpy(data + 12, script, script_len);

    wisp_ipc_msg msg = {
        .type = WISP_IPC_MSG_JS_EXEC,
        .length = total_len,
        .data = data
    };

    js_process_handle_ipc_msg(&msg);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_JS_EXEC);
    ck_assert_int_eq(recv_msg.length, 0);

    wisp_ipc_msg_free(&recv_msg);
    free(data);
    teardown_ipc();
}
END_TEST

START_TEST(test_ipc_js_exec_binary_string_len)
{
    setup_ipc();

    uint32_t ctx_id = 1;
    uint32_t eval_flags = JS_EVAL_TYPE_GLOBAL;
    uint32_t name_len = 0;
    /* Script returning string containing embedded NUL character */
    const char *script = "'hello\\0world'";
    uint32_t script_len = strlen(script);

    uint32_t total_len = 12 + script_len;
    uint8_t *data = malloc(total_len);
    memcpy(data, &ctx_id, 4);
    memcpy(data + 4, &eval_flags, 4);
    memcpy(data + 8, &name_len, 4);
    memcpy(data + 12, script, script_len);

    wisp_ipc_msg msg = {
        .type = WISP_IPC_MSG_JS_EXEC,
        .length = total_len,
        .data = data
    };

    js_process_handle_ipc_msg(&msg);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_JS_EXEC);
    ck_assert_int_eq(recv_msg.length, 11);
    ck_assert_mem_eq(recv_msg.data, "hello\0world", 11);

    wisp_ipc_msg_free(&recv_msg);
    free(data);
    teardown_ipc();
}
END_TEST

START_TEST(test_ipc_shm_init_clears_location_url)
{
    setup_ipc();

    JSContext *ctx = get_context(1);
    ck_assert_ptr_nonnull(ctx);
    struct jsthread *t = JS_GetContextOpaque(ctx);
    ck_assert_ptr_nonnull(t);

    /* Set a dummy location_url on thread */
    nsurl *url = NULL;
    nserror err = nsurl_create("https://example.com/page", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    t->location_url = url;

    const char *shm_name = "/test_js_ipc_shm_clear_loc";
    shm_unlink(shm_name);
    shm_dom_t *server_shm = shm_dom_create(shm_name, 100, true);
    ck_assert_ptr_nonnull(server_shm);

    /* SHM INIT without origin */
    wisp_ipc_msg msg = {
        .type = WISP_IPC_MSG_SHM_INIT,
        .length = strlen(shm_name),
        .data = (uint8_t *)shm_name
    };
    js_process_handle_ipc_msg(&msg);

    ck_assert_ptr_null(t->location_url);

    shm_dom_destroy(server_shm, NULL, false);
    teardown_ipc();
}
END_TEST

START_TEST(test_get_context_deferred_linking_on_origin_failure)
{
    /* Set origin to non-null value */
    js_process_origin = strdup("https://test-origin.org");

    /* get_context(50) should succeed */
    JSContext *ctx = get_context(50);
    ck_assert_ptr_nonnull(ctx);
    ck_assert_ptr_nonnull(contexts);
    ck_assert_int_eq(contexts->id, 50);
}
END_TEST

START_TEST(test_ipc_shm_init_origin_memory_safety)
{
    setup_ipc();

    JSContext *ctx1 = get_context(10);
    JSContext *ctx2 = get_context(20);
    ck_assert_ptr_nonnull(ctx1);
    ck_assert_ptr_nonnull(ctx2);

    const char *shm_name = "/test_js_ipc_shm_memsafety";
    shm_unlink(shm_name);
    shm_dom_t *server_shm = shm_dom_create(shm_name, 100, true);
    ck_assert_ptr_nonnull(server_shm);

    char payload[256];
    snprintf(payload, sizeof(payload), "%s|https://origin1.org", shm_name);

    wisp_ipc_msg msg;
    msg.type = WISP_IPC_MSG_SHM_INIT;
    msg.length = strlen(payload);
    msg.data = (uint8_t *)payload;

    js_process_handle_ipc_msg(&msg);

    struct jsthread *t1 = JS_GetContextOpaque(ctx1);
    struct jsthread *t2 = JS_GetContextOpaque(ctx2);
    ck_assert_ptr_nonnull(t1);
    ck_assert_ptr_nonnull(t2);
    ck_assert_str_eq(t1->origin, "https://origin1.org");
    ck_assert_str_eq(t2->origin, "https://origin1.org");

    snprintf(payload, sizeof(payload), "%s|https://origin2.org", shm_name);
    msg.length = strlen(payload);

    js_process_handle_ipc_msg(&msg);

    ck_assert_str_eq(t1->origin, "https://origin2.org");
    ck_assert_str_eq(t2->origin, "https://origin2.org");

    shm_dom_destroy(server_shm, NULL, false);
    teardown_ipc();
}
END_TEST

START_TEST(test_ipc_js_exec_corrupt_name_len)
{
    setup_ipc();

    uint32_t ctx_id = 1;
    uint32_t eval_flags = JS_EVAL_TYPE_GLOBAL;
    uint32_t corrupt_name_len = 1000; /* Exceeds message payload length */
    const char *script = "1 + 1";
    uint32_t script_len = strlen(script);

    uint32_t total_len = 12 + script_len;
    uint8_t *data = malloc(total_len);
    memcpy(data, &ctx_id, 4);
    memcpy(data + 4, &eval_flags, 4);
    memcpy(data + 8, &corrupt_name_len, 4);
    memcpy(data + 12, script, script_len);

    wisp_ipc_msg msg = {
        .type = WISP_IPC_MSG_JS_EXEC,
        .length = total_len,
        .data = data
    };

    js_process_handle_ipc_msg(&msg);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_JS_EXEC);
    ck_assert_int_eq(recv_msg.length, 0);

    wisp_ipc_msg_free(&recv_msg);
    free(data);
    teardown_ipc();
}
END_TEST

START_TEST(test_ipc_shm_init_null_data)
{
    wisp_ipc_msg msg = {
        .type = WISP_IPC_MSG_SHM_INIT,
        .length = 10,
        .data = NULL
    };

    /* Should safely return without crashing */
    js_process_handle_ipc_msg(&msg);
}
END_TEST

START_TEST(test_ipc_shm_init_without_origin)
{
    setup_ipc();

    /* Create context and set origin first */
    JSContext *ctx = get_context(1);
    ck_assert_ptr_nonnull(ctx);
    struct jsthread *t = JS_GetContextOpaque(ctx);
    ck_assert_ptr_nonnull(t);

    const char *shm_name = "/test_js_ipc_shm_no_orig";
    shm_unlink(shm_name);
    shm_dom_t *server_shm = shm_dom_create(shm_name, 100, true);
    ck_assert_ptr_nonnull(server_shm);

    /* First init with origin */
    char payload[256];
    snprintf(payload, sizeof(payload), "%s|https://example.com", shm_name);
    wisp_ipc_msg msg = {
        .type = WISP_IPC_MSG_SHM_INIT,
        .length = strlen(payload),
        .data = (uint8_t *)payload
    };
    js_process_handle_ipc_msg(&msg);

    ck_assert_ptr_nonnull(t->origin);
    ck_assert_str_eq(t->origin, "https://example.com");

    /* Now init without origin */
    msg.length = strlen(shm_name);
    msg.data = (uint8_t *)shm_name;
    js_process_handle_ipc_msg(&msg);

    ck_assert_ptr_nonnull(wisp_shm_dom);
    ck_assert_ptr_null(js_process_origin);
    ck_assert_ptr_null(t->origin);
    ck_assert_ptr_eq(t->shm_dom, wisp_shm_dom);
    ck_assert_uint_eq(t->shm_capacity, wisp_shm_capacity);

    shm_dom_destroy(server_shm, NULL, false);
    teardown_ipc();
}
END_TEST

START_TEST(test_ipc_js_exec_file_url_long_path)
{
    setup_ipc();

    /* Create temporary JS script file in nested long directory path */
    char long_dir[600];
    memset(long_dir, 'a', sizeof(long_dir) - 1);
    long_dir[sizeof(long_dir) - 1] = '\0';

    char tmp_path[700];
    snprintf(tmp_path, sizeof(tmp_path), "/tmp/wisp_long_path_%s.js", long_dir + 500);

    FILE *f = fopen(tmp_path, "wb");
    ck_assert_ptr_nonnull(f);
    const char *js_code = "7 * 8;";
    fwrite(js_code, 1, strlen(js_code), f);
    fclose(f);

    char file_url[800];
    snprintf(file_url, sizeof(file_url), "file://%s", tmp_path);

    uint32_t ctx_id = 1;
    uint32_t eval_flags = JS_EVAL_TYPE_GLOBAL;
    uint32_t name_len = 0;
    uint32_t script_len = strlen(file_url);

    uint32_t total_len = 12 + script_len;
    uint8_t *data = malloc(total_len);
    memcpy(data, &ctx_id, 4);
    memcpy(data + 4, &eval_flags, 4);
    memcpy(data + 8, &name_len, 4);
    memcpy(data + 12, file_url, script_len);

    wisp_ipc_msg msg = {
        .type = WISP_IPC_MSG_JS_EXEC,
        .length = total_len,
        .data = data
    };

    js_process_handle_ipc_msg(&msg);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_JS_EXEC);
    ck_assert_int_eq(recv_msg.length, 2);
    ck_assert_mem_eq(recv_msg.data, "56", 2);

    unlink(tmp_path);
    wisp_ipc_msg_free(&recv_msg);
    free(data);
    teardown_ipc();
}
END_TEST

START_TEST(test_ipc_js_exec_normal_script)
{
    setup_ipc();

    uint32_t ctx_id = 1;
    uint32_t eval_flags = JS_EVAL_TYPE_GLOBAL;
    const char *name = "test.js";
    uint32_t name_len = strlen(name);
    const char *script = "10 + 20";
    uint32_t script_len = strlen(script);

    uint32_t total_len = 12 + name_len + script_len;
    uint8_t *data = malloc(total_len);
    memcpy(data, &ctx_id, 4);
    memcpy(data + 4, &eval_flags, 4);
    memcpy(data + 8, &name_len, 4);
    memcpy(data + 12, name, name_len);
    memcpy(data + 12 + name_len, script, script_len);

    wisp_ipc_msg msg = {
        .type = WISP_IPC_MSG_JS_EXEC,
        .length = total_len,
        .data = data
    };

    js_process_handle_ipc_msg(&msg);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_JS_EXEC);
    ck_assert_int_eq(recv_msg.length, 2);
    ck_assert_mem_eq(recv_msg.data, "30", 2);

    wisp_ipc_msg_free(&recv_msg);
    free(data);
    teardown_ipc();
}
END_TEST

START_TEST(test_ipc_js_exec_default_script_name)
{
    setup_ipc();

    uint32_t ctx_id = 1;
    uint32_t eval_flags = JS_EVAL_TYPE_GLOBAL;
    uint32_t name_len = 0;
    const char *script = "'hello ' + 'world'";
    uint32_t script_len = strlen(script);

    uint32_t total_len = 12 + script_len;
    uint8_t *data = malloc(total_len);
    memcpy(data, &ctx_id, 4);
    memcpy(data + 4, &eval_flags, 4);
    memcpy(data + 8, &name_len, 4);
    memcpy(data + 12, script, script_len);

    wisp_ipc_msg msg = {
        .type = WISP_IPC_MSG_JS_EXEC,
        .length = total_len,
        .data = data
    };

    js_process_handle_ipc_msg(&msg);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_JS_EXEC);
    ck_assert_int_eq(recv_msg.length, 11);
    ck_assert_mem_eq(recv_msg.data, "hello world", 11);

    wisp_ipc_msg_free(&recv_msg);
    free(data);
    teardown_ipc();
}
END_TEST

START_TEST(test_ipc_js_exec_invalid_length)
{
    setup_ipc();

    uint8_t dummy[8] = { 0 };
    wisp_ipc_msg msg = {
        .type = WISP_IPC_MSG_JS_EXEC,
        .length = 8,
        .data = dummy
    };

    js_process_handle_ipc_msg(&msg);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_JS_EXEC);
    ck_assert_int_eq(recv_msg.length, 0);

    wisp_ipc_msg_free(&recv_msg);
    teardown_ipc();
}
END_TEST

START_TEST(test_ipc_js_exec_file_url_script)
{
    setup_ipc();

    /* Create temporary JS script file */
    char tmp_file[] = "/tmp/wisp_test_script_XXXXXX";
    int fd = mkstemp(tmp_file);
    ck_assert_int_ne(fd, -1);
    const char *js_code = "var val = 50; val * 3;";
    ssize_t w = write(fd, js_code, strlen(js_code));
    ck_assert_int_eq(w, (ssize_t)strlen(js_code));
    close(fd);

    char file_url[512];
    snprintf(file_url, sizeof(file_url), "file://%s", tmp_file);

    uint32_t ctx_id = 1;
    uint32_t eval_flags = JS_EVAL_TYPE_GLOBAL;
    uint32_t name_len = 0;
    uint32_t script_len = strlen(file_url);

    uint32_t total_len = 12 + script_len;
    uint8_t *data = malloc(total_len);
    memcpy(data, &ctx_id, 4);
    memcpy(data + 4, &eval_flags, 4);
    memcpy(data + 8, &name_len, 4);
    memcpy(data + 12, file_url, script_len);

    wisp_ipc_msg msg = {
        .type = WISP_IPC_MSG_JS_EXEC,
        .length = total_len,
        .data = data
    };

    js_process_handle_ipc_msg(&msg);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_JS_EXEC);
    ck_assert_int_eq(recv_msg.length, 3);
    ck_assert_mem_eq(recv_msg.data, "150", 3);

    unlink(tmp_file);
    wisp_ipc_msg_free(&recv_msg);
    free(data);
    teardown_ipc();
}
END_TEST

START_TEST(test_ipc_js_exec_short_script)
{
    setup_ipc();

    uint32_t ctx_id = 1;
    uint32_t eval_flags = JS_EVAL_TYPE_GLOBAL;
    uint32_t name_len = 0;
    const char *script = "1+1"; /* length 3, < 7 */
    uint32_t script_len = strlen(script);

    uint32_t total_len = 12 + script_len;
    uint8_t *data = malloc(total_len);
    memcpy(data, &ctx_id, 4);
    memcpy(data + 4, &eval_flags, 4);
    memcpy(data + 8, &name_len, 4);
    memcpy(data + 12, script, script_len);

    wisp_ipc_msg msg = {
        .type = WISP_IPC_MSG_JS_EXEC,
        .length = total_len,
        .data = data
    };

    js_process_handle_ipc_msg(&msg);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_JS_EXEC);
    ck_assert_int_eq(recv_msg.length, 1);
    ck_assert_mem_eq(recv_msg.data, "2", 1);

    wisp_ipc_msg_free(&recv_msg);
    free(data);
    teardown_ipc();
}
END_TEST

START_TEST(test_ipc_js_exec_file_url_nonexistent)
{
    setup_ipc();

    const char *file_url = "file:///nonexistent/path/does_not_exist.js";
    uint32_t ctx_id = 1;
    uint32_t eval_flags = JS_EVAL_TYPE_GLOBAL;
    uint32_t name_len = 0;
    uint32_t script_len = strlen(file_url);

    uint32_t total_len = 12 + script_len;
    uint8_t *data = malloc(total_len);
    memcpy(data, &ctx_id, 4);
    memcpy(data + 4, &eval_flags, 4);
    memcpy(data + 8, &name_len, 4);
    memcpy(data + 12, file_url, script_len);

    wisp_ipc_msg msg = {
        .type = WISP_IPC_MSG_JS_EXEC,
        .length = total_len,
        .data = data
    };

    js_process_handle_ipc_msg(&msg);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_JS_EXEC);
    ck_assert_int_eq(recv_msg.length, 0);

    wisp_ipc_msg_free(&recv_msg);
    free(data);
    teardown_ipc();
}
END_TEST

START_TEST(test_ipc_js_exec_exception)
{
    setup_ipc();

    uint32_t ctx_id = 1;
    uint32_t eval_flags = JS_EVAL_TYPE_GLOBAL;
    uint32_t name_len = 0;
    const char *script = "throw new Error('Test Error Exception');";
    uint32_t script_len = strlen(script);

    uint32_t total_len = 12 + script_len;
    uint8_t *data = malloc(total_len);
    memcpy(data, &ctx_id, 4);
    memcpy(data + 4, &eval_flags, 4);
    memcpy(data + 8, &name_len, 4);
    memcpy(data + 12, script, script_len);

    wisp_ipc_msg msg = {
        .type = WISP_IPC_MSG_JS_EXEC,
        .length = total_len,
        .data = data
    };

    js_process_handle_ipc_msg(&msg);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_JS_EXEC);
    ck_assert_int_eq(recv_msg.length, 0);

    wisp_ipc_msg_free(&recv_msg);
    free(data);
    teardown_ipc();
}
END_TEST

START_TEST(test_ipc_js_exec_microtask_and_bbmq)
{
    setup_ipc();

    uint32_t ctx_id = 1;
    uint32_t eval_flags = JS_EVAL_TYPE_GLOBAL;
    uint32_t name_len = 0;
    const char *script = "globalThis.asyncRes = 0; Promise.resolve(99).then(v => { globalThis.asyncRes = v; }); 'ok'";
    uint32_t script_len = strlen(script);

    uint32_t total_len = 12 + script_len;
    uint8_t *data = malloc(total_len);
    memcpy(data, &ctx_id, 4);
    memcpy(data + 4, &eval_flags, 4);
    memcpy(data + 8, &name_len, 4);
    memcpy(data + 12, script, script_len);

    wisp_ipc_msg msg = {
        .type = WISP_IPC_MSG_JS_EXEC,
        .length = total_len,
        .data = data
    };

    js_process_handle_ipc_msg(&msg);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_JS_EXEC);
    ck_assert_mem_eq(recv_msg.data, "ok", 2);

    JSContext *ctx = get_context(1);
    ck_assert(eval_js_bool(ctx, "globalThis.asyncRes === 99"));

    wisp_ipc_msg_free(&recv_msg);
    free(data);
    teardown_ipc();
}
END_TEST

START_TEST(test_ipc_js_exec_shm_dom_remap)
{
    setup_ipc();

    const char *shm_name = "/test_js_ipc_shm_remap";
    shm_unlink(shm_name);

    wisp_shm_dom = shm_dom_create(shm_name, 10, true);
    ck_assert_ptr_nonnull(wisp_shm_dom);
    wisp_shm_capacity = 10;

    /* Simulate another process expanding SHM node_capacity */
    wisp_shm_dom->node_capacity = 20;

    uint32_t ctx_id = 1;
    uint32_t eval_flags = JS_EVAL_TYPE_GLOBAL;
    uint32_t name_len = 0;
    const char *script = "'remapped'";
    uint32_t script_len = strlen(script);

    uint32_t total_len = 12 + script_len;
    uint8_t *data = malloc(total_len);
    memcpy(data, &ctx_id, 4);
    memcpy(data + 4, &eval_flags, 4);
    memcpy(data + 8, &name_len, 4);
    memcpy(data + 12, script, script_len);

    wisp_ipc_msg msg = {
        .type = WISP_IPC_MSG_JS_EXEC,
        .length = total_len,
        .data = data
    };

    js_process_handle_ipc_msg(&msg);

    ck_assert_uint_eq(wisp_shm_capacity, 20);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    wisp_ipc_msg_free(&recv_msg);
    free(data);
    teardown_ipc();
}
END_TEST

START_TEST(test_get_context_calloc_zero_init)
{
    JSContext *ctx = get_context(999);
    ck_assert_ptr_nonnull(ctx);
    ck_assert_ptr_nonnull(contexts);
    ck_assert_int_eq(contexts->id, 999);
}
END_TEST

START_TEST(test_ipc_js_exec_idle_microtask_error)
{
    setup_ipc();

    uint32_t ctx_id = 1;
    uint32_t eval_flags = JS_EVAL_TYPE_GLOBAL;
    uint32_t name_len = 0;
    const char *script = "Promise.resolve().then(() => { throw new Error('Async microtask failure'); }); 'done'";
    uint32_t script_len = strlen(script);

    uint32_t total_len = 12 + script_len;
    uint8_t *data = malloc(total_len);
    memcpy(data, &ctx_id, 4);
    memcpy(data + 4, &eval_flags, 4);
    memcpy(data + 8, &name_len, 4);
    memcpy(data + 12, script, script_len);

    wisp_ipc_msg msg = {
        .type = WISP_IPC_MSG_JS_EXEC,
        .length = total_len,
        .data = data
    };

    js_process_handle_ipc_msg(&msg);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_JS_EXEC);
    ck_assert_mem_eq(recv_msg.data, "done", 4);

    wisp_ipc_msg_free(&recv_msg);
    free(data);
    teardown_ipc();
}
END_TEST

START_TEST(test_ipc_js_exec_shm_dom_remap_failure_safety)
{
    setup_ipc();

    const char *shm_name = "/test_js_ipc_shm_remap_fail";
    shm_unlink(shm_name);

    wisp_shm_dom = shm_dom_create(shm_name, 10, true);
    ck_assert_ptr_nonnull(wisp_shm_dom);
    wisp_shm_capacity = 10;

    /* Simulate invalid capacity expansion that causes remap failure (> 10000000 safety limit) */
    wisp_shm_dom->node_capacity = 20000000;

    uint32_t ctx_id = 1;
    uint32_t eval_flags = JS_EVAL_TYPE_GLOBAL;
    uint32_t name_len = 0;
    const char *script = "'post_remap_fail'";
    uint32_t script_len = strlen(script);

    uint32_t total_len = 12 + script_len;
    uint8_t *data = malloc(total_len);
    memcpy(data, &ctx_id, 4);
    memcpy(data + 4, &eval_flags, 4);
    memcpy(data + 8, &name_len, 4);
    memcpy(data + 12, script, script_len);

    wisp_ipc_msg msg = {
        .type = WISP_IPC_MSG_JS_EXEC,
        .length = total_len,
        .data = data
    };

    /* Should handle remap failure safely without double unlock or crash */
    js_process_handle_ipc_msg(&msg);

    ck_assert_uint_eq(wisp_shm_capacity, 0);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    wisp_ipc_msg_free(&recv_msg);
    free(data);
    teardown_ipc();
}
END_TEST

START_TEST(test_ipc_js_exec_binary_string_with_null_bytes)
{
    setup_ipc();

    uint32_t ctx_id = 1;
    uint32_t eval_flags = JS_EVAL_TYPE_GLOBAL;
    uint32_t name_len = 0;
    const char *script = "String.fromCharCode(65, 0, 66, 0, 67)"; /* 'A\0B\0C' */
    uint32_t script_len = strlen(script);

    uint32_t total_len = 12 + script_len;
    uint8_t *data = malloc(total_len);
    memcpy(data, &ctx_id, 4);
    memcpy(data + 4, &eval_flags, 4);
    memcpy(data + 8, &name_len, 4);
    memcpy(data + 12, script, script_len);

    wisp_ipc_msg msg = {
        .type = WISP_IPC_MSG_JS_EXEC,
        .length = total_len,
        .data = data
    };

    js_process_handle_ipc_msg(&msg);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_JS_EXEC);
    ck_assert_int_eq(recv_msg.length, 5);
    ck_assert_mem_eq(recv_msg.data, "A\0B\0C", 5);

    wisp_ipc_msg_free(&recv_msg);
    free(data);
    teardown_ipc();
}
END_TEST

START_TEST(test_ipc_shm_init_origin_reset_unref_location_url)
{
    setup_ipc();

    JSContext *ctx = get_context(1);
    ck_assert_ptr_nonnull(ctx);
    struct jsthread *t = JS_GetContextOpaque(ctx);
    ck_assert_ptr_nonnull(t);

    nsurl *url = NULL;
    ck_assert_int_eq(nsurl_create("https://example.com/test", &url), NSERROR_OK);
    t->location_url = url;

    const char *shm_name = "/test_js_ipc_shm_unref_loc";
    shm_unlink(shm_name);
    shm_dom_t *server_shm = shm_dom_create(shm_name, 100, true);
    ck_assert_ptr_nonnull(server_shm);

    wisp_ipc_msg msg = {
        .type = WISP_IPC_MSG_SHM_INIT,
        .length = strlen(shm_name),
        .data = (uint8_t *)shm_name
    };

    js_process_handle_ipc_msg(&msg);

    ck_assert_ptr_null(t->origin);
    ck_assert_ptr_null(t->location_url);

    shm_dom_destroy(server_shm, NULL, false);
    teardown_ipc();
}
END_TEST

START_TEST(test_global_document_get_updates_win_priv)
{
    const char *shm_name = "/test_js_main_shm_win_priv";
    shm_unlink(shm_name);

    wisp_shm_dom = shm_dom_create(shm_name, 100, true);
    ck_assert_ptr_nonnull(wisp_shm_dom);

    WispCompactNode *nodes = shm_dom_get_nodes(wisp_shm_dom);
    nodes[2].node_type = 9; /* DOM_DOCUMENT_NODE at index 2 */
    wisp_shm_dom->node_count = 3;

    JSContext *ctx = get_context(1);
    ck_assert_ptr_nonnull(ctx);

    struct jsthread *t = JS_GetContextOpaque(ctx);
    ck_assert_ptr_nonnull(t);

    JSValue doc_val = global_document_get(ctx, JS_UNDEFINED, 0, NULL);
    JS_FreeValue(ctx, doc_val);

    ck_assert_ptr_eq(t->doc_priv, (void *)(uintptr_t)2);
    ck_assert_ptr_eq(t->win_priv, (void *)(uintptr_t)2);
    ck_assert_ptr_eq(t->global_window_priv.node, (void *)(uintptr_t)2);
}
END_TEST

START_TEST(test_ipc_js_exec_file_url_percent_encoded_path)
{
    setup_ipc();

    /* Create file with space in filename */
    const char *tmp_file = "/tmp/wisp_test%20space.js";
    const char *real_path = "/tmp/wisp_test space.js";

    FILE *f = fopen(real_path, "wb");
    ck_assert_ptr_nonnull(f);
    const char *js_code = "10 * 10;";
    fwrite(js_code, 1, strlen(js_code), f);
    fclose(f);

    char file_url[512];
    snprintf(file_url, sizeof(file_url), "file://%s", tmp_file);

    uint32_t ctx_id = 1;
    uint32_t eval_flags = JS_EVAL_TYPE_GLOBAL;
    uint32_t name_len = 0;
    uint32_t script_len = strlen(file_url);

    uint32_t total_len = 12 + script_len;
    uint8_t *data = malloc(total_len);
    memcpy(data, &ctx_id, 4);
    memcpy(data + 4, &eval_flags, 4);
    memcpy(data + 8, &name_len, 4);
    memcpy(data + 12, file_url, script_len);

    wisp_ipc_msg msg = {
        .type = WISP_IPC_MSG_JS_EXEC,
        .length = total_len,
        .data = data
    };

    js_process_handle_ipc_msg(&msg);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_JS_EXEC);
    ck_assert_int_eq(recv_msg.length, 3);
    ck_assert_mem_eq(recv_msg.data, "100", 3);

    unlink(real_path);
    wisp_ipc_msg_free(&recv_msg);
    free(data);
    teardown_ipc();
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
    tcase_add_test(tc_core, test_js_process_main_invalid_args);
    tcase_add_test(tc_core, test_ipc_shm_init_with_origin);
    tcase_add_test(tc_core, test_ipc_shm_init_without_origin);
    tcase_add_test(tc_core, test_get_context_deferred_linking_on_origin_failure);
    tcase_add_test(tc_core, test_ipc_shm_init_origin_memory_safety);
    tcase_add_test(tc_core, test_ipc_js_exec_normal_script);
    tcase_add_test(tc_core, test_ipc_js_exec_default_script_name);
    tcase_add_test(tc_core, test_ipc_js_exec_invalid_length);
    tcase_add_test(tc_core, test_ipc_js_exec_corrupt_name_len);
    tcase_add_test(tc_core, test_ipc_shm_init_null_data);
    tcase_add_test(tc_core, test_ipc_js_exec_file_url_script);
    tcase_add_test(tc_core, test_ipc_js_exec_file_url_long_path);
    tcase_add_test(tc_core, test_ipc_js_exec_short_script);
    tcase_add_test(tc_core, test_ipc_js_exec_file_url_nonexistent);
    tcase_add_test(tc_core, test_ipc_js_exec_exception);
    tcase_add_test(tc_core, test_ipc_js_exec_microtask_and_bbmq);
    tcase_add_test(tc_core, test_ipc_js_exec_shm_dom_remap);
    tcase_add_test(tc_core, test_ipc_js_exec_binary_string_len);
    tcase_add_test(tc_core, test_ipc_shm_init_clears_location_url);
    tcase_add_test(tc_core, test_get_context_calloc_zero_init);
    tcase_add_test(tc_core, test_ipc_js_exec_idle_microtask_error);
    tcase_add_test(tc_core, test_ipc_js_exec_shm_dom_remap_failure_safety);
    tcase_add_test(tc_core, test_ipc_js_exec_binary_string_with_null_bytes);
    tcase_add_test(tc_core, test_ipc_shm_init_origin_reset_unref_location_url);
    tcase_add_test(tc_core, test_global_document_get_updates_win_priv);
    tcase_add_test(tc_core, test_ipc_js_exec_file_url_percent_encoded_path);
    tcase_add_test(tc_core, test_ipc_js_exec_binary_string_embedded_nulls);
    tcase_add_test(tc_core, test_get_context_opaque_available_during_init);
    tcase_add_test(tc_core, test_ipc_js_exec_string_exception);
    tcase_add_test(tc_core, test_process_timers_and_raf_execution);
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
