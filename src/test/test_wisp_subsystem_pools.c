#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "content/handlers/javascript/quickjs/wisp_subsystem.h"
#include <wisp/utils/errors.h>

#ifdef _WIN32
#include <windows.h>
#define SLEEP_MS(ms) Sleep(ms)
#else
#include <unistd.h>
#define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

static int raster_task_count = 0;
static void test_raster_task(void *arg) {
    raster_task_count++;
}

START_TEST(test_wisp_subsystem_pools_init)
{
    init_wisp_subsystem(64);
    ck_assert(raster_pool.worker_count >= 0);
    ck_assert(js_pool.worker_count >= 1);
    shutdown_wisp_subsystem();
}
END_TEST

START_TEST(test_wisp_subsystem_pools_dispatch)
{
    init_wisp_subsystem(64);
    raster_task_count = 0;
    for (int i=0; i<10; i++) {
        wisp_dispatch_raster(test_raster_task, NULL);
    }
    /* Allow some time for workers to process */
    SLEEP_MS(100);
    ck_assert_int_ge(raster_task_count, 1);
    shutdown_wisp_subsystem();
}
END_TEST

Suite *wisp_subsystem_pools_suite(void)
{
    Suite *s = suite_create("WispSubsystemPools");
    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_wisp_subsystem_pools_init);
    tcase_add_test(tc_core, test_wisp_subsystem_pools_dispatch);
    suite_add_tcase(s, tc_core);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = wisp_subsystem_pools_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
