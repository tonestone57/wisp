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


extern bool wisp_is_js_process;
extern shm_dom_t *wisp_shm_dom;
extern uint32_t wisp_shm_capacity;

START_TEST(test_bbmq_flush_empty)
{
    // Ensure wisp_shm_dom is NULL initially
    wisp_shm_dom = NULL;
    // Calling bbmq_flush with wisp_shm_dom == NULL shouldn't crash
    bbmq_flush();

    // Now create a valid shm_dom but no pending mutations
    const char *test_name = "/test_shm_dom_bbmq_empty";
    wisp_shm_dom = shm_dom_create(test_name, 100, true);
    ck_assert_ptr_nonnull(wisp_shm_dom);

    // bbmq_size is 0, so it should just return
    bbmq_flush();

    ck_assert_int_eq(wisp_shm_dom->mutation_queue.head, 0);
    ck_assert_int_eq(wisp_shm_dom->mutation_queue.tail, 0);

    shm_dom_destroy(wisp_shm_dom, test_name, true);
    wisp_shm_dom = NULL;
}
END_TEST

START_TEST(test_bbmq_flush_normal)
{
    const char *test_name = "/test_shm_dom_bbmq_normal";
    wisp_shm_dom = shm_dom_create(test_name, 100, true);
    ck_assert_ptr_nonnull(wisp_shm_dom);
    wisp_shm_capacity = 100;

    // We act as JS process to populate bbmq
    wisp_is_js_process = true;

    // Ensure queue starts empty
    ck_assert_int_eq(wisp_shm_dom->mutation_queue.head, 0);

    // Enqueue a single mutation
    shm_mutation_enqueue(wisp_shm_dom, 1 /* SHM_MUTATION_SET_ATTRIBUTE */, 42, 0, 0, "id", "test");

    // Flush it
    bbmq_flush();

    // Verify it was moved to mutation_queue
    ck_assert_int_eq(wisp_shm_dom->mutation_queue.head, 1);
    ck_assert_int_eq(wisp_shm_dom->mutation_queue.queue[0].target_id, 42);
    ck_assert_int_eq(wisp_shm_dom->mutation_queue.queue[0].type, 1);

    // After flush, bbmq is empty, another flush shouldn't change head
    bbmq_flush();
    ck_assert_int_eq(wisp_shm_dom->mutation_queue.head, 1);

    wisp_is_js_process = false;
    shm_dom_destroy(wisp_shm_dom, test_name, true);
    wisp_shm_dom = NULL;
}
END_TEST

START_TEST(test_bbmq_flush_full)
{
    const char *test_name = "/test_shm_dom_bbmq_full";
    wisp_shm_dom = shm_dom_create(test_name, 100, true);
    ck_assert_ptr_nonnull(wisp_shm_dom);
    wisp_shm_capacity = 100;

    wisp_is_js_process = true;

    // We mock that the shared mutation queue is fully consumed and wrap around is needed
    // or just start from 0 and fill it up.

    // Enqueue SHM_MUTATION_QUEUE_SIZE + 5 items to BBMQ
    int total_items = SHM_MUTATION_QUEUE_SIZE + 5;
    for (int i = 0; i < total_items; i++) {
        shm_mutation_enqueue(wisp_shm_dom, 1, i, 0, 0, "test", "test");
    }

    // Since our bbmq_size can grow, we should have enqueued all
    // Now flush to shared mutation queue.
    // However, shared mutation queue has a fixed capacity SHM_MUTATION_QUEUE_SIZE
    bbmq_flush();

    // The shared queue should only be able to accept SHM_MUTATION_QUEUE_SIZE items
    // before head - tail >= SHM_MUTATION_QUEUE_SIZE.
    ck_assert_int_eq(wisp_shm_dom->mutation_queue.head, SHM_MUTATION_QUEUE_SIZE);

    // bbmq_flush should reset bbmq_size to 0, which means the overflowing items are lost.
    // That's current behavior, we just verify it doesn't overflow `mutation_queue`

    wisp_is_js_process = false;
    shm_dom_destroy(wisp_shm_dom, test_name, true);
    wisp_shm_dom = NULL;
}
END_TEST

static Suite *shm_dom_suite(void)
{
    Suite *s = suite_create("shm_dom");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_shm_dom_create);
    tcase_add_test(tc_core, test_shm_dom_destroy_null);
    tcase_add_test(tc_core, test_bbmq_flush_empty);
    tcase_add_test(tc_core, test_bbmq_flush_normal);
    tcase_add_test(tc_core, test_bbmq_flush_full);
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
