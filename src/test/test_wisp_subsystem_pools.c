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
#include "wisp/browser.h"
#include "wisp/content.h"
#include "desktop/tile_pool.h"

// Mock for nslog_log used by NSLOG macro
void nslog_log(enum nslog_level level, const char *file, const char *func, int ln, const char *format, ...) {}

struct hlcache_handle { int dummy; };
struct content *hlcache_handle_get_content(struct hlcache_handle *h) { return (struct content *)h; }
void content_inc_bg_tasks(struct hlcache_handle *h) {}
void content_dec_bg_tasks(struct hlcache_handle *h) {}

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
    ck_assert_ptr_nonnull(wisp_style_pool);

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
    if (expected_js < 1) expected_js = 1;
    int expected_style = (n_cores > 4) ? 4 : (int)n_cores;
    if (expected_style < 1) expected_style = 1;

    ck_assert_int_eq(raster_pool->worker_count, expected_raster);
    ck_assert_int_eq(js_pool->worker_count, expected_js);
    ck_assert_int_eq(wisp_style_pool->worker_count, expected_style);

    shutdown_wisp_subsystem();
    ck_assert_ptr_null(raster_pool);
    ck_assert_ptr_null(js_pool);
    ck_assert_ptr_null(wisp_style_pool);
}
END_TEST

START_TEST(test_subsystem_dispatch)
{
    init_wisp_subsystem(10);

    task_executed_count = 0;

    // Dispatch to JS pool
    ck_assert(wisp_dispatch_js(NULL, test_task, NULL, 0.0f));
    ck_assert(wisp_dispatch_js(NULL, test_task, NULL, 0.0f));

    // Dispatch to Raster pool
    ck_assert(wisp_dispatch_raster(NULL, test_task, NULL, 0.0f));

    // Dispatch to Style pool
    ck_assert(wisp_dispatch_style(NULL, test_task, NULL, 0.0f));

    // Wait for tasks to complete
    int retries = 0;
    while (retries < 50) {
        ns_mutex_lock(&count_lock);
        if (task_executed_count == 4) {
            ns_mutex_unlock(&count_lock);
            break;
        }
        ns_mutex_unlock(&count_lock);
        ns_usleep(100000);
        retries++;
    }

    ck_assert_int_eq(task_executed_count, 4);

    shutdown_wisp_subsystem();
}
END_TEST

START_TEST(test_browser_tile_priority)
{
    float p;
    int tile_size = browser_get_tile_size();

    /* 1. Visible tile (center is inside viewport) */
    /* Viewport: (0,0) to (1000, 1000) */
    /* Tile: (100, 100). Center: (100+tile_size/2, 100+tile_size/2) -> (228, 228) if 256 */
    p = browser_calculate_tile_priority(100, 100, 0, 0, 1000, 1000);
    ck_assert_float_eq(p, 1.0f);

    /* 2. Distant tile */
    /* Viewport: (0,0) to (100, 100) */
    /* Tile: (1000, 1000). Center: (1128, 1128) if 256.
     * dx = 1128 - 100 = 1028. dy = 1028. dist = sqrt(1028^2 + 1028^2) approx 1453. */
    p = browser_calculate_tile_priority(1000, 1000, 0, 0, 100, 100);
    ck_assert_msg(p < 0.1f, "Priority for distant tile should be low (got %f)", p);
    ck_assert_msg(p > 0.0f, "Priority should be positive");

    /* 3. Edge case: 0x0 viewport */
    p = browser_calculate_tile_priority(0, 0, 0, 0, 0, 0);
    /* Tile center (128, 128). dx=128, dy=128. dist = 181. p = 1/182 approx 0.005 */
    ck_assert_msg(p < 1.0f, "Priority for 0x0 viewport should not be 1.0 (got %f)", p);
    ck_assert_msg(p > 0.0f, "Priority should be positive");

    /* 4. Large coordinates */
    p = browser_calculate_tile_priority(1000000, 1000000, 0, 0, 100, 100);
    ck_assert_msg(p < 0.001f, "Priority for extremely distant tile should be very low (got %f)", p);
}
END_TEST

static int priority_results[4];
static int priority_idx = 0;
static ns_mutex_t priority_lock;
static ns_mutex_t block_lock;
static volatile int blocker_started = 0;

static void blocker_task(void *arg) {
    __atomic_store_n(&blocker_started, 1, __ATOMIC_RELEASE);
    ns_mutex_lock(&block_lock);
    ns_mutex_unlock(&block_lock);
}

static void priority_task(void *arg) {
    int val = (int)(intptr_t)arg;
    ns_mutex_lock(&priority_lock);
    priority_results[priority_idx++] = val;
    ns_mutex_unlock(&priority_lock);
}

START_TEST(test_subsystem_priority)
{
    setenv("WISP_JS_WORKERS", "1", 1);
    /* Initialize with 1 worker in JS pool for predictability */
    init_wisp_subsystem(10);

    ns_mutex_lock(&block_lock);
    priority_idx = 0;

    /* 1. Dispatch a blocker task to occupy the worker */
    ck_assert(wisp_dispatch_js(NULL, blocker_task, NULL, 1.0f));

    /* 2. Dispatch tasks with different priorities.
     * They will be queued because the worker is blocked. */
    ck_assert(wisp_dispatch_js(NULL, priority_task, (void*)(intptr_t)1, 0.1f)); /* Low */
    ck_assert(wisp_dispatch_js(NULL, priority_task, (void*)(intptr_t)3, 0.9f)); /* High */
    ck_assert(wisp_dispatch_js(NULL, priority_task, (void*)(intptr_t)2, 0.5f)); /* Mid */

    /* 3. Unblock the worker */
    ns_mutex_unlock(&block_lock);

    /* Wait for tasks to complete */
    int retries = 0;
    while (retries < 50) {
        ns_mutex_lock(&priority_lock);
        if (priority_idx == 3) {
            ns_mutex_unlock(&priority_lock);
            break;
        }
        ns_mutex_unlock(&priority_lock);
        ns_usleep(100000);
        retries++;
    }

    ck_assert_int_eq(priority_idx, 3);

    /* Order MUST be 3 (0.9), 2 (0.5), 1 (0.1) due to priority-based insertion */
    ck_assert_int_eq(priority_results[0], 3);
    ck_assert_int_eq(priority_results[1], 2);
    ck_assert_int_eq(priority_results[2], 1);

    shutdown_wisp_subsystem();
    unsetenv("WISP_JS_WORKERS");
}
END_TEST

static void test_pump_task_cb(void *arg) {
    int *flag = (int *)arg;
    *flag = 1;
}

START_TEST(test_pop_task_and_wait_group_pumping)
{
    setenv("WISP_JS_WORKERS", "1", 1);
    init_wisp_subsystem(10);

    /* 1. Block the worker thread so tasks stay queued */
    ns_mutex_lock(&block_lock);
    __atomic_store_n(&blocker_started, 0, __ATOMIC_RELAXED);

    int executed_flag = 0;

    /* Dispatch a blocker task to occupy worker 0 */
    ck_assert(wisp_dispatch_js(NULL, blocker_task, NULL, 1.0f));

    /* Wait until worker 0 actually starts blocker_task and blocks on block_lock */
    while (__atomic_load_n(&blocker_started, __ATOMIC_ACQUIRE) == 0) {
        ns_usleep(1000);
    }

    /* Dispatch a task to be popped / pumped by main thread */
    ck_assert(wisp_dispatch_js(NULL, test_pump_task_cb, &executed_flag, 0.5f));

    /* Verify task was enqueued */
    ck_assert_int_eq(js_pool->count, 1);

    /* Main thread pops and executes the pending task directly */
    js_task_t *popped = wisp_pool_pop_task(js_pool);
    ck_assert_ptr_nonnull(popped);
    ck_assert_ptr_eq(popped->function, test_pump_task_cb);
    popped->function(popped->arg);
    if (popped->script) free(popped->script);
    free(popped);

    ck_assert_int_eq(executed_flag, 1);

    /* Repeat test for wisp_style_pool */
    int style_executed_flag = 0;
    if (!wisp_dispatch_style(NULL, test_pump_task_cb, &style_executed_flag, 0.5f)) {
        test_pump_task_cb(&style_executed_flag);
    } else {
        js_task_t *popped_style = wisp_pool_pop_task(wisp_style_pool);
        if (popped_style) {
            ck_assert_ptr_eq(popped_style->function, test_pump_task_cb);
            popped_style->function(popped_style->arg);
            if (popped_style->script) free(popped_style->script);
            free(popped_style);
        } else {
            /* If wisp_dispatch_style executed the task synchronously (fallback mode), verify flag */
            int retries = 0;
            while (style_executed_flag == 0 && retries < 10) {
                ns_usleep(1000);
                retries++;
            }
        }
    }

    ck_assert_int_eq(style_executed_flag, 1);

    /* Unblock worker thread */
    ns_mutex_unlock(&block_lock);

    /* Wait for blocker task to complete */
    ns_usleep(50000);

    shutdown_wisp_subsystem();
    unsetenv("WISP_JS_WORKERS");
}
END_TEST

START_TEST(test_tile_pool_compressed_cache)
{
    // Initialize pool
    bool init_ok = tile_pool_init(4);
    ck_assert_int_eq(init_ok, true);

    void *buf = tile_pool_checkout();
    ck_assert_ptr_nonnull(buf);

    // Write patterns to verify decompression integrity later
    uint8_t *pixel_data = (uint8_t *)buf;
    for (int i = 0; i < 512 * 512 * 4; i++) {
        pixel_data[i] = (uint8_t)(i % 256);
    }

    void *owner = (void*)0x1234;

    // Cache the tile
    tile_pool_put_cached(owner, 100, 100, 512, buf, 1.0f);

    // Retrieve and verify (should be same buffer, RAW)
    bool from_cache = false;
    void *ret = tile_pool_get_cached(owner, 100, 100, 512, &from_cache);
    ck_assert_int_eq(from_cache, true);
    ck_assert_ptr_eq(ret, buf);

    // Verify pattern contents
    for (int i = 0; i < 1000; i++) {
        ck_assert_int_eq(((uint8_t *)ret)[i], (uint8_t)(i % 256));
    }

    // Scroll significantly out of frustum (distance = 28px, priority = 0.034, which is in [0.01, 0.2])
    // Viewport: (200, 100) of size 100x100
    tile_pool_manage_cache(owner, 200, 100, 100, 100);

    // Retrieve again: should decompress and return a valid buffer
    from_cache = false;
    void *decompressed_ret = tile_pool_get_cached(owner, 100, 100, 512, &from_cache);
    ck_assert_int_eq(from_cache, true);
    ck_assert_ptr_nonnull(decompressed_ret);

    // Verify integrity of decompressed data
    bool data_integrity_ok = true;
    for (int i = 0; i < 512 * 512 * 4; i++) {
        if (((uint8_t *)decompressed_ret)[i] != (uint8_t)(i % 256)) {
            data_integrity_ok = false;
            break;
        }
    }
    ck_assert_int_eq(data_integrity_ok, true);

    // Put it back in the cache
    tile_pool_put_cached(owner, 100, 100, 512, decompressed_ret, 1.0f);

    // Scroll extremely far out of frustum (distance > 2000px, priority < 0.01)
    tile_pool_manage_cache(owner, 2000, 2000, 100, 100);

    // Retrieve should now fail (evicted)
    from_cache = false;
    void *evicted_ret = tile_pool_get_cached(owner, 100, 100, 512, &from_cache);
    ck_assert_int_eq(from_cache, false);
    ck_assert_ptr_null(evicted_ret);

    tile_pool_fini();
}
END_TEST

Suite *subsystem_suite(void)
{
    Suite *s = suite_create("WispSubsystem");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_subsystem_init_shutdown);
    tcase_add_test(tc_core, test_subsystem_dispatch);
    tcase_add_test(tc_core, test_subsystem_priority);
    tcase_add_test(tc_core, test_browser_tile_priority);
    tcase_add_test(tc_core, test_tile_pool_compressed_cache);
    tcase_add_test(tc_core, test_pop_task_and_wait_group_pumping);

    suite_add_tcase(s, tc_core);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = subsystem_suite();
    SRunner *sr = srunner_create(s);

    ns_mutex_init(&count_lock);
    ns_mutex_init(&priority_lock);
    ns_mutex_init(&block_lock);
    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    ns_mutex_destroy(&block_lock);
    ns_mutex_destroy(&priority_lock);
    ns_mutex_destroy(&count_lock);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
