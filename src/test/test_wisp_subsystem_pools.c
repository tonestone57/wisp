#include <check.h>
#include <stdlib.h>

#ifndef _WIN32
#include <unistd.h>
#include <pthread.h>
#define ns_mutex_t pthread_mutex_t
#define ns_mutex_init(m) pthread_mutex_init(m, NULL)
#define ns_mutex_lock(m) pthread_mutex_lock(m)
#define ns_mutex_unlock(m) pthread_mutex_unlock(m)
#define ns_mutex_destroy(m) pthread_mutex_destroy(m)
#define ns_usleep(us) usleep(us)
#else
#include <windows.h>
#define ns_mutex_t CRITICAL_SECTION
#define ns_mutex_init(m) InitializeCriticalSection(m)
#define ns_mutex_lock(m) EnterCriticalSection(m)
#define ns_mutex_unlock(m) LeaveCriticalSection(m)
#define ns_mutex_destroy(m) DeleteCriticalSection(m)
#define ns_usleep(us) Sleep((us)/1000)
#endif

#include "content/handlers/javascript/quickjs/wisp_subsystem.h"
#include "wisp/utils/log.h"

// Mock for nslog_log used by NSLOG macro
void nslog_log(enum nslog_level level, const char *file, const char *func, int ln, const char *format, ...) {}

static int task_executed_count = 0;
static ns_mutex_t count_lock;

static void test_task(void *arg) {
    ns_mutex_lock(&count_lock);
    task_executed_count++;
    ns_mutex_unlock(&count_lock);
}

START_TEST(test_subsystem_init_shutdown)
{
    init_wisp_subsystem(10);
    ck_assert_ptr_nonnull(raster_pool);
    ck_assert_ptr_nonnull(js_pool);

    // Sizing verification
    long n_cores;
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    n_cores = sysinfo.dwNumberOfProcessors;
#else
    n_cores = sysconf(_SC_NPROCESSORS_ONLN);
#endif
    if (n_cores <= 0) n_cores = 1;
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
        ns_mutex_lock(&count_lock);
        if (task_executed_count == 3) {
            ns_mutex_unlock(&count_lock);
            break;
        }
        ns_mutex_unlock(&count_lock);
        ns_usleep(100000);
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

    ns_mutex_init(&count_lock);
    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    ns_mutex_destroy(&count_lock);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
