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

static Suite *task_queue_suite(void)
{
    Suite *s = suite_create("task_queue");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_task_queue_init);
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
