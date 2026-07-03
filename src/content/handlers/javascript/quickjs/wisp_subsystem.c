#include "wisp_subsystem.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "wisp/utils/log.h"
#include "wisp/utils/utils.h"

#ifdef _WIN32
#else
#include <sys/sysinfo.h>
#include <sys/time.h>
#endif

WispPool *raster_pool = NULL;
WispPool *js_pool = NULL;

static void start_worker(WispPool *pool, int i) {
    pool->workers[i].worker_id = i;
    pool->workers[i].rt = JS_NewRuntime();
    pool->workers[i].ctx = JS_NewContext(pool->workers[i].rt);
    pool->workers[i].pool = pool;

#ifdef _WIN32
    pool->workers[i].thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)wisp_worker_routine, &pool->workers[i], 0, NULL);
#else
    pthread_create(&pool->workers[i].thread, NULL, wisp_worker_routine, &pool->workers[i]);
#endif

#ifdef _WIN32
    EnterCriticalSection(&pool->lock);
#else
    pthread_mutex_lock(&pool->lock);
#endif
    pool->active_workers++;
#ifdef _WIN32
    LeaveCriticalSection(&pool->lock);
#else
    pthread_mutex_unlock(&pool->lock);
#endif
}

static WispPool* init_pool(int worker_count, int queue_size) {
    if (worker_count < 0) worker_count = 0;

    WispPool *pool = calloc(1, sizeof(WispPool));
    if (!pool) return NULL;

    pool->worker_count = worker_count;
    pool->capacity = queue_size;
    pool->stop = false;

#ifdef _WIN32
    InitializeCriticalSection(&pool->lock);
    InitializeConditionVariable(&pool->cond);
#else
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->cond, NULL);
#endif

    if (worker_count > 0) {
        pool->workers = calloc(worker_count, sizeof(WispWorker));
#ifndef _WIN32
        pthread_t null_thread;
        memset(&null_thread, 0, sizeof(pthread_t));
        for (int i=0; i<worker_count; i++) {
            pool->workers[i].thread = null_thread;
        }
#endif
        // Spawn the first thread as a seed if pool is not empty
        pool->workers[0].running = true;
        start_worker(pool, 0);
    }

    return pool;
}

static void shutdown_pool(WispPool *pool) {
    if (!pool) return;

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
        HANDLE h;
        EnterCriticalSection(&pool->lock);
        h = pool->workers[i].thread;
        pool->workers[i].thread = NULL;
        LeaveCriticalSection(&pool->lock);

        if (h) {
            WaitForSingleObject(h, INFINITE);
            CloseHandle(h);
        }
#else
        pthread_t t;
        pthread_t null_thread;
        memset(&null_thread, 0, sizeof(pthread_t));

        pthread_mutex_lock(&pool->lock);
        t = pool->workers[i].thread;
        pool->workers[i].thread = null_thread;
        pthread_mutex_unlock(&pool->lock);

        if (memcmp(&t, &null_thread, sizeof(pthread_t)) != 0) {
            pthread_join(t, NULL);
        }
#endif
        if (pool->workers[i].ctx != NULL) {
            JS_FreeContext(pool->workers[i].ctx);
            pool->workers[i].ctx = NULL;
        }
        if (pool->workers[i].rt != NULL) {
            JS_FreeRuntime(pool->workers[i].rt);
            pool->workers[i].rt = NULL;
        }
    }

    js_task_t *task = pool->head;
    while (task) {
        js_task_t *next = task->next;
        if (task->script) free(task->script);
        free(task);
        task = next;
    }

#ifdef _WIN32
    DeleteCriticalSection(&pool->lock);
#else
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->cond);
#endif

    if (pool->workers) free(pool->workers);
    free(pool);
}

void init_wisp_subsystem(int queue_size) {
    if (raster_pool != NULL || js_pool != NULL) return;

    long n_cores;
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    n_cores = sysinfo.dwNumberOfProcessors;
#else
    n_cores = sysconf(_SC_NPROCESSORS_ONLN);
#endif

    int raster_workers = (n_cores > 1) ? (int)(n_cores - 1) : 0;
    int js_workers = (n_cores > 4) ? 4 : (int)n_cores;

    raster_pool = init_pool(raster_workers, queue_size);
    js_pool = init_pool(js_workers, queue_size);

    NSLOG(wisp, INFO, "Wisp Subsystem Initialized: Raster Pool (%d), JS Pool (%d)", raster_workers, js_workers);
}

void shutdown_wisp_subsystem(void) {
    shutdown_pool(raster_pool);
    shutdown_pool(js_pool);
    raster_pool = NULL;
    js_pool = NULL;
}

void* wisp_worker_routine(void *arg) {
    WispWorker *worker = (WispWorker *)arg;
    WispPool *pool = worker->pool;

    while (worker->running) {
        js_task_t *task = NULL;
        bool has_task = false;

#ifdef _WIN32
        EnterCriticalSection(&pool->lock);
        while (pool->head == NULL && worker->running && !pool->stop) {
            BOOL wait_res = SleepConditionVariableCS(&pool->cond, &pool->lock, 5000); // 5 sec TTL
            if (!wait_res && GetLastError() == ERROR_TIMEOUT) {
                if (pool->head == NULL && pool->active_workers > 1) {
                    worker->running = false;
                    pool->active_workers--;
                    JS_FreeContext(worker->ctx);
                    JS_FreeRuntime(worker->rt);
                    worker->ctx = NULL;
                    worker->rt = NULL;
                    HANDLE h = worker->thread;
                    worker->thread = NULL;
                    LeaveCriticalSection(&pool->lock);
                    if (h) CloseHandle(h);
                    return NULL;
                }
            }
        }

        if (pool->head != NULL && worker->running) {
            task = pool->head;
            pool->head = task->next;
            if (pool->head == NULL) pool->tail = NULL;
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
                    JS_FreeContext(worker->ctx);
                    JS_FreeRuntime(worker->rt);
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
            if (pool->head == NULL) pool->tail = NULL;
            pool->count--;
            has_task = true;
            pool->busy_workers++;
        }
        pthread_mutex_unlock(&pool->lock);
#endif

        if (has_task && task) {
            if (task->function) task->function(task->arg);
            if (task->script) free(task->script);
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

static void wisp_dispatch_internal(WispPool *pool, char *script, void (*func)(void*), void *arg) {
    if (!pool) return;

    js_task_t *new_task = malloc(sizeof(js_task_t));
    if (!new_task) return;
    new_task->next = NULL;
    new_task->script = script ? strdup(script) : NULL;
    new_task->function = func;
    new_task->arg = arg;

#ifdef _WIN32
    EnterCriticalSection(&pool->lock);
    if (pool->count < pool->capacity) {
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
                    pool->workers[i].running = true;
                    break;
                }
            }
        }
        WakeConditionVariable(&pool->cond);
        LeaveCriticalSection(&pool->lock);
        if (worker_to_start != -1) start_worker(pool, worker_to_start);
    } else {
        LeaveCriticalSection(&pool->lock);
        if (new_task->script) free(new_task->script);
        free(new_task);
    }
#else
    pthread_mutex_lock(&pool->lock);
    if (pool->count < pool->capacity) {
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
                    pool->workers[i].running = true;
                    break;
                }
            }
        }
        pthread_cond_signal(&pool->cond);
        pthread_mutex_unlock(&pool->lock);
        if (worker_to_start != -1) start_worker(pool, worker_to_start);
    } else {
        pthread_mutex_unlock(&pool->lock);
        if (new_task->script) free(new_task->script);
        free(new_task);
    }
#endif
}

void wisp_dispatch_raster(char *script, void (*func)(void*), void *arg) {
    if (raster_pool && raster_pool->worker_count > 0) {
        wisp_dispatch_internal(raster_pool, script, func, arg);
    } else {
        // Synchronous execution for single-core or if pool initialization failed
        if (func) func(arg);
        // Fallback doesn't strdup script, so we don't free it either (as it belongs to the caller)
    }
}

void wisp_dispatch_js(char *script, void (*func)(void*), void *arg) {
    wisp_dispatch_internal(js_pool, script, func, arg);
}

void wisp_dispatch(char *script, void (*func)(void*), void *arg) {
    wisp_dispatch_js(script, func, arg);
}
