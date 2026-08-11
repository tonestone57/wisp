#include <check.h>
#include <stdlib.h>
#include <stdbool.h>
#include <wisp/utils/shm_dom.h>

// Do NOT define the global variables here because they are already defined in libwisp.so

START_TEST(test_shm_dom_create)
{
    const char *test_name = "/test_shm_dom_create";
    uint32_t capacity = 100;
    bool is_server = true;

    shm_dom_t *shm = shm_dom_create(test_name, capacity, is_server);

    ck_assert_ptr_nonnull(shm);
    ck_assert_int_eq(shm->is_server, is_server);
    ck_assert_int_eq(shm->node_capacity, capacity);
    ck_assert_str_eq(shm->shm_name, test_name);

    shm_dom_destroy(shm, test_name, is_server);
}
END_TEST

START_TEST(test_shm_dom_destroy_null)
{
    // Test that destroying a NULL pointer doesn't crash
    shm_dom_destroy(NULL, "/wisp_test_shm_dom", true);
}
END_TEST

START_TEST(test_shm_mutation_enqueue_native)
{
    const char *test_name = "/test_shm_mutation_enqueue_native";
    uint32_t capacity = 100;
    shm_dom_t *shm = shm_dom_create(test_name, capacity, true);
    ck_assert_ptr_nonnull(shm);

    extern bool wisp_is_js_process;
    bool saved_js_process = wisp_is_js_process;
    wisp_is_js_process = false;

    // Must setup node to be found
    shm->node_count = 2; // Node ID 1 exists
    shm->string_heap_top = 1; // Reserve offset 0 for empty string reference

    // Add mutation
    shm_mutation_enqueue(shm, SHM_MUTATION_SET_ATTRIBUTE, 1, 0, 0, "class", "test-class");

    // Check mutation queue
    shm_mutation_queue_t *mq = &shm->mutation_queue;
    ck_assert_int_eq(mq->head, 1);
    ck_assert_int_eq(mq->tail, 0);
    ck_assert_int_eq(mq->queue[0].type, SHM_MUTATION_SET_ATTRIBUTE);
    ck_assert_int_eq(mq->queue[0].target_id, 1);
    ck_assert_str_eq(wisp_string_ref_data(shm, mq->queue[0].name), "class");
    ck_assert_str_eq(wisp_string_ref_data(shm, mq->queue[0].value), "test-class");

    wisp_is_js_process = saved_js_process;
    shm_dom_destroy(shm, test_name, true);
}
END_TEST

START_TEST(test_shm_mutation_enqueue_js)
{
    const char *test_name = "/test_shm_mutation_enqueue_js";
    uint32_t capacity = 100;
    shm_dom_t *shm = shm_dom_create(test_name, capacity, true);
    ck_assert_ptr_nonnull(shm);

    extern bool wisp_is_js_process;
    extern shm_dom_t *wisp_shm_dom;
    extern uint32_t wisp_shm_capacity;

    bool saved_js_process = wisp_is_js_process;
    shm_dom_t *saved_shm = wisp_shm_dom;
    uint32_t saved_capacity = wisp_shm_capacity;

    wisp_is_js_process = true;
    wisp_shm_dom = shm;
    wisp_shm_capacity = capacity;

    shm->node_count = 2; // Node ID 1 exists
    shm->string_heap_top = 1; // Reserve offset 0 for empty string reference

    // Add mutation
    shm_mutation_enqueue(shm, SHM_MUTATION_SET_ATTRIBUTE, 1, 0, 0, "id", "test-id");

    // It should go into bbmq_buffer, not the native queue directly
    ck_assert_int_eq(shm->mutation_queue.head, 0);
    ck_assert(bbmq_has_pending_for_node(1));

    // Flush bbmq
    bbmq_flush();

    // Now it should be in the native queue
    shm_mutation_queue_t *mq = &shm->mutation_queue;
    ck_assert_int_eq(mq->head, 1);
    ck_assert_int_eq(mq->tail, 0);
    ck_assert_int_eq(mq->queue[0].type, SHM_MUTATION_SET_ATTRIBUTE);
    ck_assert_int_eq(mq->queue[0].target_id, 1);
    ck_assert_str_eq(wisp_string_ref_data(shm, mq->queue[0].name), "id");
    ck_assert_str_eq(wisp_string_ref_data(shm, mq->queue[0].value), "test-id");

    wisp_is_js_process = saved_js_process;
    wisp_shm_dom = saved_shm;
    wisp_shm_capacity = saved_capacity;
    shm_dom_destroy(shm, test_name, true);
}
END_TEST

static Suite *shm_dom_suite(void)
{
    Suite *s = suite_create("shm_dom");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_shm_dom_create);
    tcase_add_test(tc_core, test_shm_dom_destroy_null);
    tcase_add_test(tc_core, test_shm_mutation_enqueue_native);
    tcase_add_test(tc_core, test_shm_mutation_enqueue_js);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = shm_dom_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
