#include <check.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <wisp/utils/shm_dom.h>

// Do NOT define the global variables here because they are already defined in libwisp.so

extern bool wisp_is_js_process;
extern shm_dom_t *wisp_shm_dom;
extern uint32_t wisp_shm_capacity;


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

START_TEST(test_shm_dom_create_long_name_truncation)
{
    // Test safe truncation and NUL termination of long shm_name strings via snprintf
    const char *long_name = "/test_shm_dom_create_with_an_extremely_long_name_that_exceeds_sixty_four_bytes_buffer_capacity_limit";
    uint32_t capacity = 100;
    bool is_server = true;

    shm_dom_t *shm = shm_dom_create(long_name, capacity, is_server);

    ck_assert_ptr_nonnull(shm);
    ck_assert_int_eq(shm->is_server, is_server);
    ck_assert_int_eq(shm->node_capacity, capacity);
    ck_assert_int_eq(strlen(shm->shm_name), sizeof(shm->shm_name) - 1);
    ck_assert_int_eq(shm->shm_name[sizeof(shm->shm_name) - 1], '\0');
    ck_assert_int_eq(strncmp(shm->shm_name, long_name, sizeof(shm->shm_name) - 1), 0);

    shm_dom_destroy(shm, long_name, is_server);
}
END_TEST

START_TEST(test_shm_dom_destroy_null)
{
    // Test that destroying a NULL pointer doesn't crash
    shm_dom_destroy(NULL, "/wisp_test_shm_dom", true);
}
END_TEST

START_TEST(test_shm_dom_lock_read)
{
    const char *test_name = "/test_shm_dom_lock_read";
    shm_dom_t *shm = shm_dom_create(test_name, 100, true);
    ck_assert_ptr_nonnull(shm);

    // Ensure initial lock state is 0
    ck_assert_int_eq(shm->lock, 0);

    // Test a read lock increments the counter
    shm_dom_lock_read(shm);
    ck_assert_int_eq(shm->lock, 1);

    // Test a second read lock increments it again
    shm_dom_lock_read(shm);
    ck_assert_int_eq(shm->lock, 2);

    // Test a read unlock decrements the counter
    shm_dom_unlock_read(shm);
    ck_assert_int_eq(shm->lock, 1);

    // Final unlock
    shm_dom_unlock_read(shm);
    ck_assert_int_eq(shm->lock, 0);

    shm_dom_destroy(shm, test_name, true);

    // Test NULL does not crash
    shm_dom_lock_read(NULL);
    shm_dom_unlock_read(NULL);
}
END_TEST


START_TEST(test_shm_mutation_enqueue_native)
{
    const char *test_name = "/test_shm_mutation_enqueue_native";
    uint32_t capacity = 100;
    shm_dom_t *shm = shm_dom_create(test_name, capacity, true);
    ck_assert_ptr_nonnull(shm);

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

extern void drain_mutation_queue(shm_dom_t *shm, void *doc);

START_TEST(test_bbmq_flush_full)
{
    const char *test_name = "/test_shm_dom_bbmq_full";
    wisp_shm_dom = shm_dom_create(test_name, 100, true);
    ck_assert_ptr_nonnull(wisp_shm_dom);
    wisp_shm_capacity = 100;

    wisp_is_js_process = true;

    // Enqueue SHM_MUTATION_QUEUE_SIZE + 5 items to BBMQ
    int total_items = SHM_MUTATION_QUEUE_SIZE + 5;
    for (int i = 0; i < total_items; i++) {
        shm_mutation_enqueue(wisp_shm_dom, 1, i, 0, 0, "test", "test");
    }

    // Now flush to shared mutation queue.
    // With dynamic secondary chunks, primary accepts 1024 and secondary chunk accepts 5
    bbmq_flush();

    ck_assert_int_eq(wisp_shm_dom->mutation_queue.head, SHM_MUTATION_QUEUE_SIZE);
    ck_assert_int_eq(wisp_shm_dom->mutation_queue.secondary_chunk_count, 1);
    ck_assert_int_eq(wisp_shm_dom->mutation_queue.secondary_chunks[0].head, 5);

    wisp_is_js_process = false;
    shm_dom_destroy(wisp_shm_dom, test_name, true);
    wisp_shm_dom = NULL;
}
END_TEST

START_TEST(test_bbmq_secondary_chunks_auto_scale)
{
    const char *test_name = "/test_shm_dom_bbmq_sec_scale";
    wisp_shm_dom = shm_dom_create(test_name, 100, true);
    ck_assert_ptr_nonnull(wisp_shm_dom);
    wisp_shm_capacity = 100;

    wisp_is_js_process = true;

    // Enqueue 2500 mutations (Primary: 1024, Sec Chunk 0: 1024, Sec Chunk 1: 452)
    int total_items = 2500;
    for (int i = 0; i < total_items; i++) {
        shm_mutation_enqueue(wisp_shm_dom, 1, i + 1, 0, 0, "attr", "val");
    }

    bbmq_flush();

    ck_assert_int_eq(wisp_shm_dom->mutation_queue.head, SHM_MUTATION_QUEUE_SIZE);
    ck_assert_int_eq(wisp_shm_dom->mutation_queue.secondary_chunk_count, 2);
    ck_assert_int_eq(wisp_shm_dom->mutation_queue.secondary_chunks[0].head, SHM_MUTATION_CHUNK_CAPACITY);
    ck_assert_int_eq(wisp_shm_dom->mutation_queue.secondary_chunks[1].head, 2500 - 1024 - 1024);

    // Drain queue with NULL doc (verifies mapping, iteration, and cleanup without crash)
    drain_mutation_queue(wisp_shm_dom, NULL);

    ck_assert_int_eq(wisp_shm_dom->mutation_queue.tail, SHM_MUTATION_QUEUE_SIZE);
    ck_assert_int_eq(wisp_shm_dom->mutation_queue.secondary_chunk_count, 0);

    wisp_is_js_process = false;
    shm_dom_destroy(wisp_shm_dom, test_name, true);
    wisp_shm_dom = NULL;
}
END_TEST

START_TEST(test_shm_alloc_string_nulls)
{
    shm_dom_t *shm = shm_dom_create("/test_shm_alloc_string_nulls", 100, true);
    ck_assert_ptr_nonnull(shm);

    // Test null shm
    WispStringRef ref1 = wisp_shm_alloc_string(NULL, "test");
    ck_assert_int_eq(ref1, 0);

    // Test null str
    WispStringRef ref2 = wisp_shm_alloc_string(shm, NULL);
    ck_assert_int_eq(ref2, 0);

    // Test both null
    WispStringRef ref3 = wisp_shm_alloc_string(NULL, NULL);
    ck_assert_int_eq(ref3, 0);

    shm_dom_destroy(shm, "/test_shm_alloc_string_nulls", true);
}
END_TEST

START_TEST(test_shm_alloc_string_sso)
{
    shm_dom_t *shm = shm_dom_create("/test_shm_alloc_string_sso", 100, true);
    ck_assert_ptr_nonnull(shm);

    uint32_t initial_heap_top = shm->string_heap_top;

    // Length 0
    WispStringRef ref0 = wisp_shm_alloc_string(shm, "");
    ck_assert(wisp_string_ref_eq(shm, ref0, ""));
    ck_assert_int_eq(shm->string_heap_top, initial_heap_top);

    // Length 1
    WispStringRef ref1 = wisp_shm_alloc_string(shm, "A");
    ck_assert(wisp_string_ref_eq(shm, ref1, "A"));
    ck_assert_int_eq(shm->string_heap_top, initial_heap_top);

    // Length 2
    WispStringRef ref2 = wisp_shm_alloc_string(shm, "AB");
    ck_assert(wisp_string_ref_eq(shm, ref2, "AB"));
    ck_assert_int_eq(shm->string_heap_top, initial_heap_top);

    // Length 3
    WispStringRef ref3 = wisp_shm_alloc_string(shm, "ABC");
    ck_assert(wisp_string_ref_eq(shm, ref3, "ABC"));
    ck_assert_int_eq(shm->string_heap_top, initial_heap_top);

    shm_dom_destroy(shm, "/test_shm_alloc_string_sso", true);
}
END_TEST

START_TEST(test_shm_alloc_string_heap)
{
    shm_dom_t *shm = shm_dom_create("/test_shm_alloc_string_heap_tmp", 100, true);
    memset(shm->string_hash_table, 0, sizeof(shm->string_hash_table));
    ck_assert_ptr_nonnull(shm);

    uint32_t initial_heap_top = shm->string_heap_top;

    // Length > 3
    WispStringRef ref1 = wisp_shm_alloc_string(shm, "LongStringHere");
    ck_assert_int_ne(ref1, 0);
    ck_assert(wisp_string_ref_eq(shm, ref1, "LongStringHere"));

    uint32_t after_alloc_heap_top = shm->string_heap_top;
    ck_assert_int_gt(after_alloc_heap_top, initial_heap_top);

    // Allocate again, should intern
    WispStringRef ref2 = wisp_shm_alloc_string(shm, "LongStringHere");
    ck_assert_int_eq(ref1, ref2); // Should return same ref
    ck_assert_int_eq(shm->string_heap_top, after_alloc_heap_top); // Heap top should not increase

    shm_dom_destroy(shm, "/test_shm_alloc_string_heap_tmp", true);
}
END_TEST

START_TEST(test_shm_alloc_string_oom)
{
    shm_dom_t *shm = shm_dom_create("/test_shm_alloc_string_oom", 100, true);
    ck_assert_ptr_nonnull(shm);

    // Artificially restrict heap
    shm->string_heap_top = SHM_STRING_HEAP_SIZE - 2;

    // Try allocating string that needs > 2 bytes
    WispStringRef ref = wisp_shm_alloc_string(shm, "WillOOM");
    ck_assert_int_eq(ref, 0);

    shm_dom_destroy(shm, "/test_shm_alloc_string_oom", true);
}
END_TEST

START_TEST(test_shm_alloc_string_deduplication_and_linear_probing)
{
    shm_dom_t *shm = shm_dom_create("/test_shm_alloc_string_dedup", 100, true);
    ck_assert_ptr_nonnull(shm);

    const char *common_class = "flex items-center justify-between px-4 py-2 bg-white dark:bg-gray-800 rounded-lg shadow-md";

    // Allocate identical class attribute string multiple times
    uint32_t heap_top_before = shm->string_heap_top;
    WispStringRef ref1 = wisp_shm_alloc_string(shm, common_class);
    uint32_t heap_top_first = shm->string_heap_top;

    WispStringRef ref2 = wisp_shm_alloc_string(shm, common_class);
    WispStringRef ref3 = wisp_shm_alloc_string(shm, common_class);
    uint32_t heap_top_after = shm->string_heap_top;

    // References must be identical and heap memory usage must not increase
    ck_assert_int_eq(ref1, ref2);
    ck_assert_int_eq(ref2, ref3);
    ck_assert_int_gt(heap_top_first, heap_top_before);
    ck_assert_int_eq(heap_top_after, heap_top_first);
    ck_assert_str_eq(wisp_string_ref_data(shm, ref1), common_class);

    // Allocate many unique long strings to trigger linear probing collisions in hash table
    char str_buf[64];
    for (int i = 0; i < 50; i++) {
        snprintf(str_buf, sizeof(str_buf), "unique-attribute-value-string-%d", i);
        WispStringRef r1 = wisp_shm_alloc_string(shm, str_buf);
        ck_assert_int_ne(r1, 0);
        ck_assert_str_eq(wisp_string_ref_data(shm, r1), str_buf);

        // Allocating the exact same string again must retrieve the deduplicated reference
        WispStringRef r2 = wisp_shm_alloc_string(shm, str_buf);
        ck_assert_int_eq(r1, r2);
    }

    shm_dom_destroy(shm, "/test_shm_alloc_string_dedup", true);
}
END_TEST

static Suite *shm_dom_suite(void)
{
    Suite *s = suite_create("shm_dom");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_shm_dom_create);
    tcase_add_test(tc_core, test_shm_dom_create_long_name_truncation);
    tcase_add_test(tc_core, test_shm_dom_destroy_null);
    tcase_add_test(tc_core, test_shm_dom_lock_read);
    tcase_add_test(tc_core, test_shm_mutation_enqueue_native);
    tcase_add_test(tc_core, test_shm_mutation_enqueue_js);
    tcase_add_test(tc_core, test_bbmq_flush_empty);
    tcase_add_test(tc_core, test_bbmq_flush_normal);
    tcase_add_test(tc_core, test_bbmq_flush_full);
    tcase_add_test(tc_core, test_bbmq_secondary_chunks_auto_scale);
    tcase_add_test(tc_core, test_shm_alloc_string_nulls);
    tcase_add_test(tc_core, test_shm_alloc_string_sso);
    tcase_add_test(tc_core, test_shm_alloc_string_heap);
    tcase_add_test(tc_core, test_shm_alloc_string_oom);
    tcase_add_test(tc_core, test_shm_alloc_string_deduplication_and_linear_probing);

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
