#include <check.h>
#include <stdlib.h>
#include <unistd.h>
#include "content/handlers/javascript/quickjs/wisp_subsystem.h"
#include "wisp/utils/log.h"

// Mock for nslog_log used by NSLOG macro
void nslog_log(enum nslog_level level, const char *file, const char *func, int ln, const char *format, ...) {}

static int task_executed_count = 0;
static pthread_mutex_t count_lock = PTHREAD_MUTEX_INITIALIZER;

static void test_task(void *arg) {
    pthread_mutex_lock(&count_lock);
    task_executed_count++;
    pthread_mutex_unlock(&count_lock);
}

START_TEST(test_subsystem_init_shutdown)
{
    init_wisp_subsystem(10);
    ck_assert_ptr_nonnull(raster_pool);
    ck_assert_ptr_nonnull(js_pool);

    // Check sizes based on logical cores (assuming >= 1)
    long n_cores = sysconf(_SC_NPROCESSORS_ONLN);
    int expected_raster = (n_cores > 1) ? (int)(n_cores - 1) : 0;
    int expected_js = (n_cores > 4) ? 4 : (int)n_cores;

    ck_assert_int_eq(raster_pool->worker_count, expected_raster);
    ck_assert_int_eq(js_pool->worker_count, expected_js);

    shutdown_wisp_subsystem();
    ck_assert_ptr_null(raster_pool);
    ck_assert_ptr_null(js_pool);
}
END_TEST

START_TEST(test_subsystem_dispatch)
{
    init_wisp_subsystem(10);

    task_executed_count = 0;

    // Dispatch to JS pool
    wisp_dispatch_js(NULL, test_task, NULL);
    wisp_dispatch_js(NULL, test_task, NULL);

    // Dispatch to Raster pool
    wisp_dispatch_raster(NULL, test_task, NULL);

    // Wait for tasks to complete
    int retries = 0;
    while (retries < 50) {
        pthread_mutex_lock(&count_lock);
        if (task_executed_count == 3) {
            pthread_mutex_unlock(&count_lock);
            break;
        }
        pthread_mutex_unlock(&count_lock);
        usleep(100000);
        retries++;
    }

    ck_assert_int_eq(task_executed_count, 3);

    shutdown_wisp_subsystem();
}
END_TEST

Suite *subsystem_suite(void)
{
    Suite *s = suite_create("WispSubsystem");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_subsystem_init_shutdown);
    tcase_add_test(tc_core, test_subsystem_dispatch);

    suite_add_tcase(s, tc_core);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = subsystem_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
