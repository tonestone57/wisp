#include "wisp_subsystem.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdatomic.h>
#include "wisp/utils/log.h"
#include "wisp/utils/utils.h"
#include "qjs_internal.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#endif

/* Global pools */
WispPool raster_pool = {0};
WispPool js_pool = {0};

/* Internal helper to start a worker thread in a pool.
 * Caller must ensure worker->running is set to true before calling. */
static void start_worker_in_pool(WispPool *pool, int i)
{
    pool->workers[i].worker_id = i;
    pool->workers[i].pool = pool;

    if (pool->is_js) {
        pool->workers[i].rt = JS_NewRuntime();
        pool->workers[i].ctx = JS_NewContext(pool->workers[i].rt);
        /* Basic globals for worker tasks */
        qjs_init_console(pool->workers[i].ctx);
        qjs_init_crypto(pool->workers[i].ctx);
    } else {
        pool->workers[i].rt = NULL;
        pool->workers[i].ctx = NULL;
    }

#ifdef _WIN32
    pool->workers[i].thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)wisp_worker_routine, &pool->workers[i], 0, NULL);
#else
    pthread_create(&pool->workers[i].thread, NULL, wisp_worker_routine, &pool->workers[i]);
#endif

#ifdef _WIN32
    EnterCriticalSection(&pool->lock);
    pool->active_workers++;
    LeaveCriticalSection(&pool->lock);
#else
    pthread_mutex_lock(&pool->lock);
    pool->active_workers++;
    pthread_mutex_unlock(&pool->lock);
#endif
}

/* Internal helper to initialize a pool */
static void init_pool(WispPool *pool, int worker_count, int queue_size, bool is_js)
{
    memset(pool, 0, sizeof(WispPool));
    pool->worker_count = worker_count;
    pool->capacity = queue_size;
    pool->is_js = is_js;

#ifdef _WIN32
    InitializeCriticalSection(&pool->lock);
    InitializeConditionVariable(&pool->cond);
#else
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->cond, NULL);
#endif

    if (worker_count > 0) {
        pool->workers = calloc(worker_count, sizeof(WispWorker));
        /* The first worker is always started to handle initial tasks */
        pool->workers[0].running = true;
        start_worker_in_pool(pool, 0);
    }
}

/* Internal helper to shutdown a pool */
static void shutdown_pool(WispPool *pool)
{
    if (pool->workers == NULL && pool->worker_count == 0) {
        return;
    }

#ifdef _WIN32
    EnterCriticalSection(&pool->lock);
    pool->stop = true;
    for (int i = 0; i < pool->worker_count; i++) {
        pool->workers[i].running = false;
    }
    WakeAllConditionVariable(&pool->cond);
    LeaveCriticalSection(&pool->lock);
#else
    pthread_mutex_lock(&pool->lock);
    pool->stop = true;
    for (int i = 0; i < pool->worker_count; i++) {
        pool->workers[i].running = false;
    }
    pthread_cond_broadcast(&pool->cond);
    pthread_mutex_unlock(&pool->lock);
#endif

    for (int i = 0; i < pool->worker_count; i++) {
#ifdef _WIN32
        if (pool->workers[i].thread) {
            WaitForSingleObject(pool->workers[i].thread, INFINITE);
            CloseHandle(pool->workers[i].thread);
            pool->workers[i].thread = NULL;
        }
#else
        pthread_t null_thread;
        memset(&null_thread, 0, sizeof(pthread_t));
        if (memcmp(&pool->workers[i].thread, &null_thread, sizeof(pthread_t)) != 0) {
            pthread_join(pool->workers[i].thread, NULL);
            pool->workers[i].thread = null_thread;
        }
#endif
        if (pool->workers[i].ctx != NULL) {
            JS_FreeContext(pool->workers[i].ctx);
            pool->workers[i].ctx = NULL;
        }
        if (pool->workers[i].rt != NULL) {
            JS_RunGC(pool->workers[i].rt);
            JS_FreeRuntime(pool->workers[i].rt);
            pool->workers[i].rt = NULL;
        }
    }
    free(pool->workers);
    pool->workers = NULL;

    js_task_t *task = pool->head;
    while (task) {
        js_task_t *next = task->next;
        if (task->script) {
            free(task->script);
        }
        free(task);
        task = next;
    }
    pool->head = NULL;
    pool->tail = NULL;
    pool->count = 0;

#ifdef _WIN32
    DeleteCriticalSection(&pool->lock);
#else
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->cond);
#endif

    int is_js = pool->is_js;
    memset(pool, 0, sizeof(WispPool));
    pool->is_js = is_js;
}

void init_wisp_subsystem(int queue_size)
{
    if (raster_pool.worker_count > 0 || js_pool.worker_count > 0 || raster_pool.workers != NULL) {
        return;
    }

    long n_cores = 1;
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    n_cores = sysinfo.dwNumberOfProcessors;
#else
    n_cores = sysconf(_SC_NPROCESSORS_ONLN);
#endif

    if (n_cores < 1) {
        n_cores = 1;
    }

    int p_raster = (int)(n_cores > 1 ? n_cores - 1 : 0);
    int p_js = (int)(n_cores < 4 ? n_cores : 4);

    init_pool(&raster_pool, p_raster, queue_size, false);
    init_pool(&js_pool, p_js, queue_size, true);
}

void shutdown_wisp_subsystem(void)
{
    shutdown_pool(&raster_pool);
    shutdown_pool(&js_pool);
}

void* wisp_worker_routine(void *arg)
{
    WispWorker *worker = (WispWorker *)arg;
    WispPool *pool = worker->pool;

    while (worker->running) {
        js_task_t *task = NULL;
        bool has_task = false;

#ifdef _WIN32
        EnterCriticalSection(&pool->lock);
        while (pool->head == NULL && worker->running && !pool->stop) {
            BOOL wait_res = SleepConditionVariableCS(&pool->cond, &pool->lock, 5000);
            if (!wait_res && GetLastError() == ERROR_TIMEOUT) {
                if (pool->head == NULL && pool->active_workers > 1) {
                    worker->running = false;
                    pool->active_workers--;
                    if (worker->ctx) {
                        JS_FreeContext(worker->ctx);
                    }
                    if (worker->rt) {
                        JS_RunGC(worker->rt);
                        JS_FreeRuntime(worker->rt);
                    }
                    worker->ctx = NULL;
                    worker->rt = NULL;
                    HANDLE h = worker->thread;
                    worker->thread = NULL;
                    LeaveCriticalSection(&pool->lock);
                    if (h) {
                        CloseHandle(h);
                    }
                    return NULL;
                }
            }
        }
        if (pool->head != NULL && worker->running) {
            task = pool->head;
            pool->head = task->next;
            if (pool->head == NULL) {
                pool->tail = NULL;
            }
            pool->count--;
            has_task = true;
            pool->busy_workers++;
        }
        LeaveCriticalSection(&pool->lock);
#else
        pthread_mutex_lock(&pool->lock);
        while (pool->head == NULL && worker->running && !pool->stop) {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 5;
            int wait_res = pthread_cond_timedwait(&pool->cond, &pool->lock, &ts);
            if (wait_res != 0) {
                if (pool->head == NULL && pool->active_workers > 1) {
                    worker->running = false;
                    pool->active_workers--;
                    if (worker->ctx) {
                        JS_FreeContext(worker->ctx);
                    }
                    if (worker->rt) {
                        JS_RunGC(worker->rt);
                        JS_FreeRuntime(worker->rt);
                    }
                    worker->ctx = NULL;
                    worker->rt = NULL;
                    pthread_t null_thread;
                    memset(&null_thread, 0, sizeof(pthread_t));
                    worker->thread = null_thread;
                    pthread_mutex_unlock(&pool->lock);
                    pthread_detach(pthread_self());
                    return NULL;
                }
            }
        }
        if (pool->head != NULL && worker->running) {
            task = pool->head;
            pool->head = task->next;
            if (pool->head == NULL) {
                pool->tail = NULL;
            }
            pool->count--;
            has_task = true;
            pool->busy_workers++;
        }
        pthread_mutex_unlock(&pool->lock);
#endif

        if (has_task && task) {
            if (task->function) {
                task->function(task->arg);
            } else if (task->script && worker->ctx) {
                JSValue res = JS_Eval(worker->ctx, task->script, strlen(task->script), "<worker>", JS_EVAL_TYPE_GLOBAL);
                if (JS_IsException(res)) {
                    JSValue exc = JS_GetException(worker->ctx);
                    const char *exc_str = JS_ToCString(worker->ctx, exc);
                    if (exc_str) {
                        NSLOG(wisp, WARNING, "Worker JS Error: %s", exc_str);
                        JS_FreeCString(worker->ctx, exc_str);
                    }
                    JS_FreeValue(worker->ctx, exc);
                }
                JS_FreeValue(worker->ctx, res);
                JS_RunGC(worker->rt);
            }
            if (task->script) {
                free(task->script);
            }
            free(task);

#ifdef _WIN32
            EnterCriticalSection(&pool->lock);
            pool->busy_workers--;
            LeaveCriticalSection(&pool->lock);
#else
            pthread_mutex_lock(&pool->lock);
            pool->busy_workers--;
            pthread_mutex_unlock(&pool->lock);
#endif
        }
    }
    return NULL;
}

static void wisp_dispatch_internal(WispPool *pool, char *script, void (*func)(void*), void *arg)
{
    if (pool->worker_count == 0) {
        if (func) {
            func(arg);
        }
        if (script) {
            free(script);
        }
        return;
    }

    js_task_t *new_task = malloc(sizeof(js_task_t));
    if (!new_task) {
        if (script) {
            free(script);
        }
        return;
    }
    new_task->next = NULL;
    new_task->script = script;
    new_task->function = func;
    new_task->arg = arg;

#ifdef _WIN32
    EnterCriticalSection(&pool->lock);
    if (!pool->stop && pool->count < pool->capacity) {
        if (pool->tail == NULL) {
            pool->head = new_task;
            pool->tail = new_task;
        } else {
            pool->tail->next = new_task;
            pool->tail = new_task;
        }
        pool->count++;

        int worker_to_start = -1;
        if (pool->busy_workers == pool->active_workers && pool->active_workers < pool->worker_count) {
            for (int i=0; i<pool->worker_count; i++) {
                if (!pool->workers[i].running) {
                    worker_to_start = i;
                    pool->workers[i].running = true; /* Claim worker inside lock */
                    break;
                }
            }
        }

        WakeConditionVariable(&pool->cond);
        LeaveCriticalSection(&pool->lock);
        if (worker_to_start != -1) {
            start_worker_in_pool(pool, worker_to_start);
        }
    } else {
        LeaveCriticalSection(&pool->lock);
        if (new_task->script) {
            free(new_task->script);
        }
        free(new_task);
    }
#else
    pthread_mutex_lock(&pool->lock);
    if (!pool->stop && pool->count < pool->capacity) {
        if (pool->tail == NULL) {
            pool->head = new_task;
            pool->tail = new_task;
        } else {
            pool->tail->next = new_task;
            pool->tail = new_task;
        }
        pool->count++;

        int worker_to_start = -1;
        if (pool->busy_workers == pool->active_workers && pool->active_workers < pool->worker_count) {
            pthread_t null_thread;
            memset(&null_thread, 0, sizeof(pthread_t));
            for (int i=0; i<pool->worker_count; i++) {
                if (!pool->workers[i].running && memcmp(&pool->workers[i].thread, &null_thread, sizeof(pthread_t)) == 0) {
                    worker_to_start = i;
                    pool->workers[i].running = true; /* Claim worker inside lock */
                    break;
                }
            }
        }

        pthread_cond_signal(&pool->cond);
        pthread_mutex_unlock(&pool->lock);
        if (worker_to_start != -1) {
            start_worker_in_pool(pool, worker_to_start);
        }
    } else {
        pthread_mutex_unlock(&pool->lock);
        if (new_task->script) {
            free(new_task->script);
        }
        free(new_task);
    }
#endif
}

void wisp_dispatch_raster(void (*func)(void*), void *arg)
{
    wisp_dispatch_internal(&raster_pool, NULL, func, arg);
}

void wisp_dispatch_js(char *script, void (*func)(void*), void *arg)
{
    wisp_dispatch_internal(&js_pool, script, func, arg);
}
