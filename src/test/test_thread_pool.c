#include <check.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdatomic.h>
#include <limits.h>

#include "wisp/utils/thread_pool.h"

const char *__attribute__((weak)) __asan_default_options(void) {
    return "allocator_may_return_null=1";
}

static atomic_int counter;

static void worker_func(void *arg) {
    (void)arg;
    atomic_fetch_add(&counter, 1);
    usleep(10000); // 10ms
}

START_TEST(test_thread_pool_create_invalid_threads) {
    ck_assert_ptr_eq(thread_pool_create(0), NULL);
    ck_assert_ptr_eq(thread_pool_create(-1), NULL);
    ck_assert_ptr_eq(thread_pool_create(-100), NULL);
}
END_TEST

START_TEST(test_thread_pool_create_excessive_threads) {
    ck_assert_ptr_eq(thread_pool_create(INT_MAX), NULL);
}
END_TEST

START_TEST(test_thread_pool_destroy_null) {
    thread_pool_destroy(NULL);
    // Should not crash
}
END_TEST

START_TEST(test_thread_pool_destroy_empty) {
    thread_pool_t *pool = thread_pool_create(2);
    ck_assert_ptr_ne(pool, NULL);
    thread_pool_destroy(pool);
    // Should not crash and should clean up memory
}
END_TEST

START_TEST(test_thread_pool_destroy_with_tasks) {
    atomic_init(&counter, 0);
    thread_pool_t *pool = thread_pool_create(2);
    ck_assert_ptr_ne(pool, NULL);

    for (int i = 0; i < 5; i++) {
        bool added = thread_pool_add_task(pool, worker_func, NULL);
        ck_assert_int_eq(added, true);
    }

    thread_pool_destroy(pool);
    // Tasks should be cleaned up by destroy (either run or discarded)
}
END_TEST

START_TEST(test_thread_pool_add_task_null_pool) {
    bool added = thread_pool_add_task(NULL, worker_func, NULL);
    ck_assert_int_eq(added, false);
}
END_TEST

START_TEST(test_thread_pool_add_task_null_func) {
    thread_pool_t *pool = thread_pool_create(2);
    ck_assert_ptr_ne(pool, NULL);
    bool added = thread_pool_add_task(pool, NULL, NULL);
    ck_assert_int_eq(added, false);
    thread_pool_destroy(pool);
}
END_TEST

START_TEST(test_thread_pool_add_task_valid) {
    atomic_init(&counter, 0);
    thread_pool_t *pool = thread_pool_create(2);
    ck_assert_ptr_ne(pool, NULL);

    bool added = thread_pool_add_task(pool, worker_func, NULL);
    ck_assert_int_eq(added, true);

    // Wait a bit for task to finish
    usleep(20000);

    int final_count = atomic_load(&counter);
    ck_assert_int_eq(final_count, 1);

    thread_pool_destroy(pool);
}
END_TEST

Suite *thread_pool_suite(void) {
    Suite *s = suite_create("Thread Pool");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_thread_pool_create_invalid_threads);
    tcase_add_test(tc_core, test_thread_pool_create_excessive_threads);
    tcase_add_test(tc_core, test_thread_pool_destroy_null);
    tcase_add_test(tc_core, test_thread_pool_destroy_empty);
    tcase_add_test(tc_core, test_thread_pool_destroy_with_tasks);

    tcase_add_test(tc_core, test_thread_pool_add_task_null_pool);
    tcase_add_test(tc_core, test_thread_pool_add_task_null_func);
    tcase_add_test(tc_core, test_thread_pool_add_task_valid);

    suite_add_tcase(s, tc_core);
    return s;
}

int main(void) {
    int number_failed;
    Suite *s = thread_pool_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
