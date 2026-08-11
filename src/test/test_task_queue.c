#include <check.h>
#include <stdlib.h>
#include <stdbool.h>
#include <wisp/utils/task_queue.h>

START_TEST(test_task_queue_init)
{
    bool result = task_queue_init();
    ck_assert_int_eq(result, true);

    /* Call it again to test that repeated calls return true */
    result = task_queue_init();
    ck_assert_int_eq(result, true);

    /* Clean up */
    task_queue_destroy();
}
END_TEST

static void dummy_task(void *arg) {
    (void)arg;
}

START_TEST(test_task_queue_destroy)
{
    /* Test destroying uninitialized queue (should be no-op) */
    task_queue_destroy();

    /* Test destroying empty initialized queue */
    ck_assert_int_eq(task_queue_init(), true);
    task_queue_destroy();

    /* Test destroying queue with pending tasks */
    ck_assert_int_eq(task_queue_init(), true);
    ck_assert_int_eq(task_queue_post(dummy_task, NULL), true);
    ck_assert_int_eq(task_queue_post(dummy_task, NULL), true);
    task_queue_destroy();

    /* Ensure it resets to uninitialized state */
    ck_assert_int_eq(task_queue_post(dummy_task, NULL), false);
}
END_TEST

static int execute_count = 0;
static int execute_order[10];

static void order_task(void *arg) {
    int val = *(int*)arg;
    if (execute_count < 10) {
        execute_order[execute_count++] = val;
    }
}

START_TEST(test_task_queue_execute_pending)
{
    /* Reset state */
    execute_count = 0;

    /* 1. Test uninitialized queue execution (should not crash) */
    task_queue_destroy(); /* Ensure uninitialized */
    task_queue_execute_pending();

    /* 2. Test empty initialized queue execution (should not crash) */
    ck_assert_int_eq(task_queue_init(), true);
    task_queue_execute_pending();

    /* 3. Test execution of pending tasks in FIFO order */
    int val1 = 1, val2 = 2, val3 = 3;
    ck_assert_int_eq(task_queue_post(order_task, &val1), true);
    ck_assert_int_eq(task_queue_post(order_task, &val2), true);
    ck_assert_int_eq(task_queue_post(order_task, &val3), true);

    task_queue_execute_pending();

    /* Verify execution */
    ck_assert_int_eq(execute_count, 3);
    ck_assert_int_eq(execute_order[0], 1);
    ck_assert_int_eq(execute_order[1], 2);
    ck_assert_int_eq(execute_order[2], 3);

    /* Verify queue is empty after execution */
    execute_count = 0;
    task_queue_execute_pending();
    ck_assert_int_eq(execute_count, 0);

    task_queue_destroy();
}
END_TEST

static Suite *task_queue_suite(void)
{
    Suite *s = suite_create("task_queue");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_task_queue_init);
    tcase_add_test(tc_core, test_task_queue_destroy);
    tcase_add_test(tc_core, test_task_queue_execute_pending);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = task_queue_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
