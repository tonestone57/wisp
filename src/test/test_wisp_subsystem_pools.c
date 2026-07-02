#include <check.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "content/handlers/javascript/quickjs/wisp_subsystem.h"

static int raster_task_done = 0;
static int js_task_done = 0;

static void raster_task(void *arg) {
    int *done = (int *)arg;
    (*done)++;
}

static void js_task(void *arg) {
    int *done = (int *)arg;
    (*done)++;
}

START_TEST(test_wisp_subsystem_pools_init)
{
    init_wisp_subsystem(10);

    // Basic verification of pool counts based on environment
    // We can't know N here easily but we can check if they were initialized
    ck_assert(js_pool.worker_count > 0);

    shutdown_wisp_subsystem();
}
END_TEST

START_TEST(test_wisp_subsystem_pools_dispatch)
{
    init_wisp_subsystem(10);

    raster_task_done = 0;
    js_task_done = 0;

    wisp_dispatch_raster(raster_task, &raster_task_done);
    wisp_dispatch_js(NULL, js_task, &js_task_done);

    // Wait a bit for tasks to complete
    int retries = 100;
    while ((raster_task_done == 0 || js_task_done == 0) && retries-- > 0) {
        usleep(10000);
    }

    ck_assert_int_eq(raster_task_done, 1);
    ck_assert_int_eq(js_task_done, 1);

    shutdown_wisp_subsystem();
}
END_TEST

Suite *wisp_subsystem_pools_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("WispSubsystemPools");

    tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_wisp_subsystem_pools_init);
    tcase_add_test(tc_core, test_wisp_subsystem_pools_dispatch);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = wisp_subsystem_pools_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    sr = NULL; // srunner_free is handled in some check versions, but let's be safe
    // srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
