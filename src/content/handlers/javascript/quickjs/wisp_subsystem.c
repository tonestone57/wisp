#include "wisp_subsystem.h"

#ifdef __SANITIZE_THREAD__
#undef __atomic_thread_fence
#define __atomic_thread_fence(x) ((void)0)
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <nsutils/time.h>
#include "wisp/utils/log.h"
#include "wisp/utils/utils.h"
#include "content/handlers/javascript/js.h"
#include "qjs_internal.h"
#include "dom_bridge.h"
#include "wisp/desktop/gui_table.h"
#include "wisp/misc.h"
#include "wisp/content.h"
#include "wisp/content/hlcache.h"

#ifdef _WIN32
#elif defined(__HAIKU__)
#include <OS.h>
#include <sys/time.h>
#else
#include <sys/sysinfo.h>
#include <sys/time.h>
#endif

WispPool *raster_pool = NULL;
WispPool *js_pool = NULL;

/* Lock-safe multi-producer Chase-Lev double-ended queue (deque) implementation */
static inline void wisp_deque_init(wisp_deque_t *q) {
    __atomic_store_n(&q->head, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&q->tail, 0, __ATOMIC_RELAXED);
    for (int i = 0; i < WISP_DEQUE_SIZE; i++) {
        __atomic_store_n(&q->tasks[i], NULL, __ATOMIC_RELAXED);
    }
#ifdef _WIN32
    InitializeCriticalSection(&q->lock);
#else
    pthread_mutex_init(&q->lock, NULL);
#endif
}

static inline void wisp_deque_deinit(wisp_deque_t *q) {
#ifdef _WIN32
    DeleteCriticalSection(&q->lock);
#else
    pthread_mutex_destroy(&q->lock);
#endif
}

static inline bool wisp_deque_push(wisp_deque_t *q, js_task_t *task) {
    bool result = false;
#ifdef _WIN32
    EnterCriticalSection(&q->lock);
#else
    pthread_mutex_lock(&q->lock);
#endif
    int64_t t = __atomic_load_n(&q->tail, __ATOMIC_RELAXED);
    int64_t h = __atomic_load_n(&q->head, __ATOMIC_ACQUIRE);
    if (t - h < WISP_DEQUE_SIZE) {
        __atomic_store_n(&q->tasks[t % WISP_DEQUE_SIZE], task, __ATOMIC_RELAXED);
        __atomic_thread_fence(__ATOMIC_RELEASE);
        __atomic_store_n(&q->tail, t + 1, __ATOMIC_RELEASE);
        result = true;
    }
#ifdef _WIN32
    LeaveCriticalSection(&q->lock);
#else
    pthread_mutex_unlock(&q->lock);
#endif
    return result;
}

static inline js_task_t* wisp_deque_pop(wisp_deque_t *q) {
    js_task_t *task = NULL;
#ifdef _WIN32
    EnterCriticalSection(&q->lock);
#else
    pthread_mutex_lock(&q->lock);
#endif
    int64_t t = __atomic_load_n(&q->tail, __ATOMIC_RELAXED);
    if (t > 0) {
        t = t - 1;
        __atomic_store_n(&q->tail, t, __ATOMIC_RELAXED);
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        int64_t h = __atomic_load_n(&q->head, __ATOMIC_ACQUIRE);
        if (h <= t) {
            task = (js_task_t *)__atomic_load_n(&q->tasks[t % WISP_DEQUE_SIZE], __ATOMIC_RELAXED);
            if (h == t) {
                int64_t expected_h = h;
                if (!__atomic_compare_exchange_n(&q->head, &expected_h, h + 1, false, __ATOMIC_SEQ_CST, __ATOMIC_RELAXED)) {
                    task = NULL;
                }
                __atomic_store_n(&q->tail, h + 1, __ATOMIC_RELAXED);
            }
        } else {
            __atomic_store_n(&q->tail, h, __ATOMIC_RELAXED);
        }
    }
#ifdef _WIN32
    LeaveCriticalSection(&q->lock);
#else
    pthread_mutex_unlock(&q->lock);
#endif
    return task;
}

static inline js_task_t* wisp_deque_steal(wisp_deque_t *q) {
    js_task_t *task = NULL;
#ifdef _WIN32
    EnterCriticalSection(&q->lock);
#else
    pthread_mutex_lock(&q->lock);
#endif
    while (1) {
        int64_t h = __atomic_load_n(&q->head, __ATOMIC_SEQ_CST);
        int64_t t = __atomic_load_n(&q->tail, __ATOMIC_ACQUIRE);
        if (h >= t) {
            break;
        }
        task = (js_task_t *)__atomic_load_n(&q->tasks[h % WISP_DEQUE_SIZE], __ATOMIC_ACQUIRE);
        int64_t expected_h = h;
        if (__atomic_compare_exchange_n(&q->head, &expected_h, h + 1, false, __ATOMIC_SEQ_CST, __ATOMIC_RELAXED)) {
            break;
        }
        task = NULL;
    }
#ifdef _WIN32
    LeaveCriticalSection(&q->lock);
#else
    pthread_mutex_unlock(&q->lock);
#endif
    return task;
}

extern struct wisp_table *guit;
static int active_web_workers = 0;
#ifdef _WIN32
static CRITICAL_SECTION web_worker_lock;
#else
static pthread_mutex_t web_worker_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

void wisp_message_queue_init(WispMessageQueue *q) {
    memset(q, 0, sizeof(*q));
#ifdef _WIN32
    InitializeCriticalSection(&q->lock);
    InitializeConditionVariable(&q->cond);
#else
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->cond, NULL);
#endif
}

void wisp_message_queue_deinit(WispMessageQueue *q) {
    WispMessage *msg = q->head;
    while (msg) {
        WispMessage *next = msg->next;
        free(msg->data);
        free(msg->error_message);
        free(msg->filename);
        free(msg);
        msg = next;
    }
#ifdef _WIN32
    DeleteCriticalSection(&q->lock);
#else
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->cond);
#endif
}

void wisp_message_queue_push(WispMessageQueue *q, WispMessage *msg) {
    if (!msg) return;
    msg->next = NULL;

#ifdef _WIN32
    EnterCriticalSection(&q->lock);
#else
    pthread_mutex_lock(&q->lock);
#endif
    if (q->tail) {
        q->tail->next = msg;
        q->tail = msg;
    } else {
        q->head = q->tail = msg;
    }
#ifdef _WIN32
    WakeConditionVariable(&q->cond);
    LeaveCriticalSection(&q->lock);
#else
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->lock);
#endif
}

WispMessage* wisp_message_queue_pop(WispMessageQueue *q, int timeout_ms) {
#ifdef _WIN32
    EnterCriticalSection(&q->lock);
    if (!q->head && timeout_ms != 0) {
        SleepConditionVariableCS(&q->cond, &q->lock, timeout_ms > 0 ? (DWORD)timeout_ms : INFINITE);
    }
#else
    pthread_mutex_lock(&q->lock);
    if (!q->head && timeout_ms != 0) {
        if (timeout_ms < 0) {
            pthread_cond_wait(&q->cond, &q->lock);
        } else {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += timeout_ms / 1000;
            ts.tv_nsec += (timeout_ms % 1000) * 1000000;
            if (ts.tv_nsec >= 1000000000) {
                ts.tv_sec++;
                ts.tv_nsec -= 1000000000;
            }
            pthread_cond_timedwait(&q->cond, &q->lock, &ts);
        }
    }
#endif
    WispMessage *msg = q->head;
    if (msg) {
        q->head = msg->next;
        if (!q->head) q->tail = NULL;
    }
#ifdef _WIN32
    LeaveCriticalSection(&q->lock);
#else
    pthread_mutex_unlock(&q->lock);
#endif
    return msg;
}

extern void wisp_dispatch_message_to_worker_object(WispWorkerHandle *h, WispMessage *msg);

static void wisp_worker_flush_to_main_cb(void *p) {
    WispWorkerHandle *h = p;
    WispMessage *msg;
    while ((msg = wisp_message_queue_pop(&h->from_worker, 0)) != NULL) {
        wisp_dispatch_message_to_worker_object(h, msg);
        free(msg->data);
        free(msg->error_message);
        free(msg->filename);
        free(msg);
    }
    __atomic_store_n(&h->main_thread_notified, false, __ATOMIC_RELAXED);
}

void wisp_worker_notify_main_thread(WispWorkerHandle *h) {
    if (!__atomic_exchange_n(&h->main_thread_notified, true, __ATOMIC_RELAXED)) {
        if (guit && guit->misc && guit->misc->schedule) {
            guit->misc->schedule(0, wisp_worker_flush_to_main_cb, h);
        }
    }
}

static void start_worker(WispPool *pool, int i) {
#ifdef _WIN32
    EnterCriticalSection(&pool->lock);
#else
    pthread_mutex_lock(&pool->lock);
#endif

    if (pool->stop) {
#ifdef _WIN32
        LeaveCriticalSection(&pool->lock);
#else
        pthread_mutex_unlock(&pool->lock);
#endif
        return;
    }

    pool->workers[i].worker_id = i;
    pool->workers[i].rt = JS_NewRuntime();
    if (pool->workers[i].rt != NULL) {
        JS_SetMaxStackSize(pool->workers[i].rt, 8192 * 1024);
    }
    pool->workers[i].ctx = JS_NewContext(pool->workers[i].rt);
    pool->workers[i].pool = pool;

#ifdef _WIN32
    pool->workers[i].thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)wisp_worker_routine, &pool->workers[i], 0, NULL);
    if (pool->workers[i].thread != NULL) {
        pool->active_workers++;
    } else {
        pool->workers[i].running = false;
        JS_FreeContext(pool->workers[i].ctx); pool->workers[i].ctx = NULL;
        JS_FreeRuntime(pool->workers[i].rt); pool->workers[i].rt = NULL;
    }
    LeaveCriticalSection(&pool->lock);
#else
    int ret = pthread_create(&pool->workers[i].thread, NULL, wisp_worker_routine, &pool->workers[i]);
    if (ret == 0) {
        pool->active_workers++;
    } else {
        pool->workers[i].running = false;
        pthread_t null_thread; memset(&null_thread, 0, sizeof(pthread_t));
        pool->workers[i].thread = null_thread;
        JS_FreeContext(pool->workers[i].ctx); pool->workers[i].ctx = NULL;
        JS_FreeRuntime(pool->workers[i].rt); pool->workers[i].rt = NULL;
    }
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
    __atomic_store_n(&pool->dispatch_idx, 0, __ATOMIC_RELAXED);
#ifdef _WIN32
    InitializeCriticalSection(&pool->lock);
    InitializeConditionVariable(&pool->cond);
#else
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->cond, NULL);
#endif
    if (worker_count > 0) {
        pool->workers = calloc(worker_count, sizeof(WispWorker));
        if (!pool->workers) {
#ifdef _WIN32
            DeleteCriticalSection(&pool->lock);
#else
            pthread_mutex_destroy(&pool->lock);
            pthread_cond_destroy(&pool->cond);
#endif
            free(pool);
            return NULL;
        }
#ifndef _WIN32
        pthread_t null_thread;
        memset(&null_thread, 0, sizeof(pthread_t));
        for (int i=0; i<worker_count; i++) pool->workers[i].thread = null_thread;
#endif
        for (int i = 0; i < worker_count; i++) {
            for (int j = 0; j < 4; j++) {
                wisp_deque_init(&pool->workers[i].deques[j]);
                __atomic_store_n(&pool->workers[i].deficit[j], 0, __ATOMIC_RELAXED);
            }
        }
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
    for (int i = 0; i < pool->worker_count; i++) __atomic_store_n(&pool->workers[i].running, false, __ATOMIC_RELAXED);
    WakeAllConditionVariable(&pool->cond);
    LeaveCriticalSection(&pool->lock);
#else
    pthread_mutex_lock(&pool->lock);
    pool->stop = true;
    for (int i = 0; i < pool->worker_count; i++) __atomic_store_n(&pool->workers[i].running, false, __ATOMIC_RELAXED);
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
        if (h) { WaitForSingleObject(h, INFINITE); CloseHandle(h); }
#else
        pthread_t t;
        pthread_t null_thread;
        memset(&null_thread, 0, sizeof(pthread_t));
        pthread_mutex_lock(&pool->lock);
        t = pool->workers[i].thread;
        pool->workers[i].thread = null_thread;
        pthread_mutex_unlock(&pool->lock);
        if (memcmp(&t, &null_thread, sizeof(pthread_t)) != 0) pthread_join(t, NULL);
#endif
        if (pool->workers[i].ctx != NULL) { JS_FreeContext(pool->workers[i].ctx); pool->workers[i].ctx = NULL; }
        if (pool->workers[i].rt != NULL) { JS_FreeRuntime(pool->workers[i].rt); pool->workers[i].rt = NULL; }
    }
    for (int i = 0; i < pool->worker_count; i++) {
        for (int j = 0; j < 4; j++) {
            js_task_t *task;
            while ((task = wisp_deque_pop(&pool->workers[i].deques[j])) != NULL) {
                if (task->script) free(task->script);
                free(task);
            }
            wisp_deque_deinit(&pool->workers[i].deques[j]);
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
#elif defined(__HAIKU__)
    system_info info;
    get_system_info(&info);
    n_cores = info.cpu_count;
#else
    n_cores = sysconf(_SC_NPROCESSORS_ONLN);
#endif
    if (n_cores <= 0) n_cores = 1;
    int raster_workers = (n_cores > 1) ? (int)(n_cores - 1) : 0;
    int js_workers = (n_cores > 4) ? 4 : (int)n_cores;
    char *env_workers = getenv("WISP_JS_WORKERS");
    if (env_workers != NULL) {
        int val = atoi(env_workers);
        if (val > 0) js_workers = val;
    }
    if (js_workers < 1) js_workers = 1;
    raster_pool = init_pool(raster_workers, queue_size);
    js_pool = init_pool(js_workers, queue_size);
#ifdef _WIN32
    InitializeCriticalSection(&web_worker_lock);
#endif
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

#ifdef __HAIKU__
    rename_thread(find_thread(NULL), "Wisp Worker");
#endif

    const int64_t quanta[4] = { 10, 20, 50, 100 }; /* Quantums (in milliseconds) for tiers 0..3 */

    while (__atomic_load_n(&worker->running, __ATOMIC_RELAXED)) {
        js_task_t *task = NULL;
        int active_tier = -1;

        /* Retrospective Deficit Round Robin (R-DRR) local scheduling */
        for (int j = 3; j >= 0; j--) {
            int64_t h = __atomic_load_n(&worker->deques[j].head, __ATOMIC_RELAXED);
            int64_t t = __atomic_load_n(&worker->deques[j].tail, __ATOMIC_RELAXED);
            if (h >= t) {
                /* Queue is empty. Clamp positive accumulated credit to 0, but preserve negative penalty. */
                if (__atomic_load_n(&worker->deficit[j], __ATOMIC_RELAXED) > 0) {
                    __atomic_store_n(&worker->deficit[j], 0, __ATOMIC_RELAXED);
                }
                continue;
            }

            int64_t def = __atomic_load_n(&worker->deficit[j], __ATOMIC_RELAXED);
            if (def < 0) {
                /* Frozen due to prior over-execution. Boost deficit back up to allow eventual recovery. */
                __atomic_store_n(&worker->deficit[j], def + quanta[j], __ATOMIC_RELAXED);
                continue;
            }
            /* Add credit to deficit counter */
            __atomic_store_n(&worker->deficit[j], def + quanta[j], __ATOMIC_RELAXED);

            task = wisp_deque_pop(&worker->deques[j]);
            if (task != NULL) {
                active_tier = j;
                break;
            }
        }

        /* Decentralized work-stealing if local deques are empty */
        if (task == NULL) {
            /* 1. Try to steal from siblings in the same pool */
            for (int i = 0; i < pool->worker_count; i++) {
                if (i == worker->worker_id) continue;
                for (int j = 3; j >= 0; j--) {
                    task = wisp_deque_steal(&pool->workers[i].deques[j]);
                    if (task != NULL) {
                        active_tier = j;
                        break;
                    }
                }
                if (task != NULL) break;
            }
        }

        if (task == NULL && pool == js_pool && raster_pool != NULL) {
            /* 2. Asymmetric crossing over: Background JS workers steal from Raster workers (Tier 3 Viewport tiles) */
            for (int i = 0; i < raster_pool->worker_count; i++) {
                task = wisp_deque_steal(&raster_pool->workers[i].deques[3]);
                if (task != NULL) {
                    active_tier = 3;
                    break;
                }
            }
        }

        if (task == NULL) {
            /* No tasks found. Block/wait for new tasks. */
#ifdef _WIN32
            EnterCriticalSection(&pool->lock);
            while (__atomic_load_n(&pool->count, __ATOMIC_RELAXED) == 0 && __atomic_load_n(&worker->running, __ATOMIC_RELAXED) && !pool->stop) {
                BOOL wait_res = SleepConditionVariableCS(&pool->cond, &pool->lock, 5000);
                if (!wait_res && GetLastError() == ERROR_TIMEOUT) {
                    if (__atomic_load_n(&pool->count, __ATOMIC_RELAXED) == 0 && pool->active_workers > 1 && !pool->stop) {
                        __atomic_store_n(&worker->running, false, __ATOMIC_RELAXED);
                        pool->active_workers--;
                        JS_FreeContext(worker->ctx); JS_FreeRuntime(worker->rt);
                        worker->ctx = NULL; worker->rt = NULL;
                        HANDLE h = worker->thread; worker->thread = NULL;
                        LeaveCriticalSection(&pool->lock);
                        if (h) CloseHandle(h);
                        return NULL;
                    }
                }
            }
            LeaveCriticalSection(&pool->lock);
#else
            pthread_mutex_lock(&pool->lock);
            while (__atomic_load_n(&pool->count, __ATOMIC_RELAXED) == 0 && __atomic_load_n(&worker->running, __ATOMIC_RELAXED) && !pool->stop) {
                struct timespec ts;
                clock_gettime(CLOCK_REALTIME, &ts);
                ts.tv_sec += 5;
                int wait_res = pthread_cond_timedwait(&pool->cond, &pool->lock, &ts);
                if (wait_res == ETIMEDOUT) {
                    if (__atomic_load_n(&pool->count, __ATOMIC_RELAXED) == 0 && pool->active_workers > 1 && !pool->stop) {
                        __atomic_store_n(&worker->running, false, __ATOMIC_RELAXED);
                        pool->active_workers--;
                        JS_FreeContext(worker->ctx); JS_FreeRuntime(worker->rt);
                        worker->ctx = NULL; worker->rt = NULL;
                        pthread_t null_thread; memset(&null_thread, 0, sizeof(pthread_t));
                        worker->thread = null_thread;
                        pthread_mutex_unlock(&pool->lock);
                        pthread_detach(pthread_self());
                        return NULL;
                    }
                } else if (wait_res != 0) break;
            }
            pthread_mutex_unlock(&pool->lock);
#endif
            continue; /* Restart loop to re-poll deques */
        }

        /* Execute task */
        if (task) {
#ifdef _WIN32
            EnterCriticalSection(&pool->lock); pool->busy_workers++; LeaveCriticalSection(&pool->lock);
#else
            pthread_mutex_lock(&pool->lock); pool->busy_workers++; pthread_mutex_unlock(&pool->lock);
#endif
            __atomic_sub_fetch(&pool->count, 1, __ATOMIC_SEQ_CST);

            uint64_t start_time;
            nsu_getmonotonic_ms(&start_time);

            if (task->script) {
                JSValue val = js_eval_with_aot_cache(worker->ctx, (const uint8_t *)task->script, strlen(task->script), "<eval>", JS_EVAL_TYPE_GLOBAL);
                JS_FreeValue(worker->ctx, val);
                JSContext *ctx1;
                int job_ret;
                while ((job_ret = JS_ExecutePendingJob(JS_GetRuntime(worker->ctx), &ctx1)) != 0) {
                    if (job_ret < 0) {
                        JSValue exc = JS_GetException(ctx1);
                        const char *exc_str = JS_ToCString(ctx1, exc);
                        NSLOG(wisp, WARNING, "JS Error in microtask: %s", exc_str ? exc_str : "unknown");
                        if (exc_str) JS_FreeCString(ctx1, exc_str);
                        JS_FreeValue(ctx1, exc);
                    }
                }
            }
            if (task->function) task->function(task->arg);

            uint64_t end_time;
            nsu_getmonotonic_ms(&end_time);
            int64_t t_exec = (int64_t)(end_time - start_time);

            /* Retroactively penalize active tier's deficit balance */
            int64_t current_def = __atomic_load_n(&worker->deficit[active_tier], __ATOMIC_RELAXED);
            __atomic_store_n(&worker->deficit[active_tier], current_def - t_exec, __ATOMIC_RELAXED);

            if (task->script) free(task->script);
            free(task);

#ifdef _WIN32
            EnterCriticalSection(&pool->lock); pool->busy_workers--; LeaveCriticalSection(&pool->lock);
#else
            pthread_mutex_lock(&pool->lock); pool->busy_workers--; pthread_mutex_unlock(&pool->lock);
#endif
        }
    }
    return NULL;
}

static bool wisp_dispatch_internal(WispPool *pool, const char *script, void (*func)(void*), void *arg, float priority) {
    if (!pool || pool->worker_count == 0) return false;
    js_task_t *new_task = malloc(sizeof(js_task_t));
    if (!new_task) return false;
    new_task->next = NULL;
    new_task->script = script ? strdup(script) : NULL;
    new_task->function = func;
    new_task->arg = arg;
    new_task->priority = priority;
    nsu_getmonotonic_ms(&new_task->entry_time);
    if (script && !new_task->script) { free(new_task); return false; }

    int tier = 0;
    if (priority >= 0.75f) {
        tier = 3;
    } else if (priority >= 0.5f) {
        tier = 2;
    } else if (priority >= 0.25f) {
        tier = 1;
    } else {
        tier = 0;
    }

    int start_idx = (int)(__atomic_fetch_add(&pool->dispatch_idx, 1, __ATOMIC_RELAXED) % pool->worker_count);
    bool pushed = false;
    for (int i = 0; i < pool->worker_count; i++) {
        int idx = (start_idx + i) % pool->worker_count;
        if (wisp_deque_push(&pool->workers[idx].deques[tier], new_task)) {
            pushed = true;
            break;
        }
    }

    if (pushed) {
        int worker_to_start = -1;
#ifdef _WIN32
        EnterCriticalSection(&pool->lock);
#else
        pthread_mutex_lock(&pool->lock);
#endif
        __atomic_add_fetch(&pool->count, 1, __ATOMIC_SEQ_CST);
        if (pool->busy_workers == pool->active_workers && pool->active_workers < pool->worker_count) {
            for (int i = 0; i < pool->worker_count; i++) {
#ifdef _WIN32
                if (!pool->workers[i].running && pool->workers[i].thread == NULL)
#else
                pthread_t null_thread; memset(&null_thread, 0, sizeof(pthread_t));
                if (!pool->workers[i].running && memcmp(&pool->workers[i].thread, &null_thread, sizeof(pthread_t)) == 0)
#endif
                {
                    worker_to_start = i; pool->workers[i].running = true; break;
                }
            }
        }
#ifdef _WIN32
        WakeAllConditionVariable(&pool->cond); LeaveCriticalSection(&pool->lock);
#else
        pthread_cond_broadcast(&pool->cond); pthread_mutex_unlock(&pool->lock);
#endif
        if (worker_to_start != -1) start_worker(pool, worker_to_start);
        return true;
    } else {
        if (new_task->script) free(new_task->script);
        free(new_task);
        return false;
    }
}

bool wisp_dispatch_raster(const char *script, void (*func)(void*), void *arg, float priority) {
    if (raster_pool && raster_pool->worker_count > 0) return wisp_dispatch_internal(raster_pool, script, func, arg, priority);
    if (script) NSLOG(wisp, WARNING, "Synchronous raster fallback: JS script execution not supported without dedicated thread context.");
    if (func) func(arg);
    return true;
}

bool wisp_dispatch_js(const char *script, void (*func)(void*), void *arg, float priority) {
    return wisp_dispatch_internal(js_pool, script, func, arg, priority);
}

void wisp_dispatch(char *script, void (*func)(void*), void *arg) {
    wisp_dispatch_js(script, func, arg, 0.0f);
    if (script) free(script);
}

static int js_worker_interrupt_handler(JSRuntime *rt, void *opaque) {
    WispWorkerHandle *h = opaque;
    if (h->terminated) return 1;
    return 0;
}

extern nserror qjs_init_worker_thread(WispWorkerHandle *h, jsthread **thread_out);

typedef struct WispFetchRequest {
    const char *url;
    uint8_t *out_buffer;
    size_t out_len;
    bool success;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool completed;
} WispFetchRequest;

void wisp_worker_fetch_cb(void *p) {
    WispFetchRequest *req = p;
    nsurl *url;
    if (nsurl_create(req->url, &url) == NSERROR_OK) {
        hlcache_handle *h;
        nserror err = hlcache_handle_retrieve(url, 0, NULL, NULL, NULL, NULL, NULL, CONTENT_ANY, &h);
        if (err == NSERROR_OK) {
            const uint8_t *data = content_get_source_data(h, &req->out_len);
            if (data) {
                req->out_buffer = malloc(req->out_len);
                if (req->out_buffer) {
                    memcpy(req->out_buffer, data, req->out_len);
                    req->success = true;
                } else {
                    req->success = false;
                }
            }
            hlcache_handle_release(h);
        }
        nsurl_unref(url);
    }
    pthread_mutex_lock(&req->mutex);
    req->completed = true;
    pthread_cond_signal(&req->cond);
    pthread_mutex_unlock(&req->mutex);
}

void* wisp_web_worker_routine(void *arg) {
    WispWorkerHandle *h = (WispWorkerHandle *)arg;
    jsthread *t = NULL;

#ifdef __HAIKU__
    rename_thread(find_thread(NULL), "Wisp Web Worker");
#endif
    if (qjs_init_worker_thread(h, &t) != NSERROR_OK) {
        h->running = false;
#ifdef _WIN32
        EnterCriticalSection(&web_worker_lock);
#else
        pthread_mutex_lock(&web_worker_lock);
#endif
        active_web_workers--;
#ifdef _WIN32
        LeaveCriticalSection(&web_worker_lock);
#else
        pthread_mutex_unlock(&web_worker_lock);
#endif
        wisp_worker_handle_unref(h);
        return NULL;
    }
    JS_SetInterruptHandler(JS_GetRuntime(t->ctx), js_worker_interrupt_handler, h);

    WispFetchRequest req;
    memset(&req, 0, sizeof(req));
    req.url = h->script_url;
    pthread_mutex_init(&req.mutex, NULL);
    pthread_cond_init(&req.cond, NULL);
    guit->misc->schedule(0, wisp_worker_fetch_cb, &req);
    pthread_mutex_lock(&req.mutex);
    while (!req.completed) pthread_cond_wait(&req.cond, &req.mutex);
    pthread_mutex_unlock(&req.mutex);

    if (req.success && req.out_buffer) {
        JSValue res = js_eval_with_aot_cache(t->ctx, req.out_buffer, req.out_len, h->script_url, JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(res)) {
            JSValue exc = JS_GetException(t->ctx);
            JSValue stack = JS_UNDEFINED;
            if (JS_IsObject(exc)) {
                stack = JS_GetPropertyStr(t->ctx, exc, "stack");
            }
            const char *msg_str = JS_ToCString(t->ctx, exc);
            WispMessage *errMsg = calloc(1, sizeof(*errMsg));
            if (errMsg) {
                errMsg->type = WISP_MSG_TYPE_ERROR;
                errMsg->error_message = strdup(msg_str ? msg_str : "Unknown error");
                wisp_message_queue_push(&h->from_worker, errMsg);
                wisp_worker_notify_main_thread(h);
            }
            if (msg_str) JS_FreeCString(t->ctx, msg_str); JS_FreeValue(t->ctx, stack); JS_FreeValue(t->ctx, exc);
        }
        JS_FreeValue(t->ctx, res);
        free(req.out_buffer);
    }
    pthread_mutex_destroy(&req.mutex); pthread_cond_destroy(&req.cond);

    h->running = true;
    while (h->running && !h->terminated) {
        JSContext *ctx1;
        int job_ret;
        while ((job_ret = JS_ExecutePendingJob(JS_GetRuntime(t->ctx), &ctx1)) != 0) {
            if (job_ret < 0) {
                JSValue exc = JS_GetException(ctx1);
                const char *exc_str = JS_ToCString(ctx1, exc);
                NSLOG(wisp, WARNING, "JS Error in microtask: %s", exc_str ? exc_str : "unknown");
                if (exc_str) JS_FreeCString(ctx1, exc_str);
                JS_FreeValue(ctx1, exc);
            }
        }
        WispMessage *msg = wisp_message_queue_pop(&h->to_worker, 100);
        if (msg) {
            JSValue global = JS_GetGlobalObject(t->ctx);
            JSValue json = JS_GetPropertyStr(t->ctx, global, "JSON");
            JSValue parse = JS_GetPropertyStr(t->ctx, json, "parse");
            JSValue json_str_val = JS_NewStringLen(t->ctx, (const char *)msg->data, msg->size);
            JSValue msg_data = JS_Call(t->ctx, parse, json, 1, &json_str_val);
            JS_FreeValue(t->ctx, json_str_val);
            JS_FreeValue(t->ctx, parse);
            JS_FreeValue(t->ctx, json);
            extern JSValue qjs_new_messageevent_manual(JSContext *ctx, JSValue data);
            JSValue event = qjs_new_messageevent_manual(t->ctx, msg_data);
            JSValue onmessage = JS_GetPropertyStr(t->ctx, global, "onmessage");
            if (JS_IsFunction(t->ctx, onmessage)) {
                JSValue ret = JS_Call(t->ctx, onmessage, global, 1, &event);
                if (JS_IsException(ret)) {
                    JSValue exc = JS_GetException(t->ctx);
                    const char *exc_str = JS_ToCString(t->ctx, exc);
                    WispMessage *errMsg = calloc(1, sizeof(*errMsg));
                    if (errMsg) {
                        errMsg->type = WISP_MSG_TYPE_ERROR;
                        errMsg->error_message = strdup(exc_str ? exc_str : "Unknown error");
                        wisp_message_queue_push(&h->from_worker, errMsg);
                        wisp_worker_notify_main_thread(h);
                    }
                    if (exc_str) JS_FreeCString(t->ctx, exc_str); JS_FreeValue(t->ctx, exc);
                }
                JS_FreeValue(t->ctx, ret);
            }
            JS_FreeValue(t->ctx, onmessage); JS_FreeValue(t->ctx, event);
            JS_FreeValue(t->ctx, msg_data); JS_FreeValue(t->ctx, global);
            free(msg->data); free(msg);
        }
    }

    jsheap *heap = t->heap;
    js_destroythread(t);
    js_destroyheap(heap);
#ifdef _WIN32
    EnterCriticalSection(&web_worker_lock);
#else
    pthread_mutex_lock(&web_worker_lock);
#endif
    active_web_workers--;
#ifdef _WIN32
    LeaveCriticalSection(&web_worker_lock);
#else
    pthread_mutex_unlock(&web_worker_lock);
#endif
    wisp_worker_handle_unref(h);
    return NULL;
}

WispWorkerHandle* wisp_subsystem_spawn_worker(const char *script_url) {
#ifdef _WIN32
    EnterCriticalSection(&web_worker_lock);
#else
    pthread_mutex_lock(&web_worker_lock);
#endif
    if (active_web_workers >= 4) {
#ifdef _WIN32
        LeaveCriticalSection(&web_worker_lock);
#else
        pthread_mutex_unlock(&web_worker_lock);
#endif
        NSLOG(wisp, WARNING, "Web Worker limit reached (%d)", 4);
        return NULL;
    }
    active_web_workers++;
#ifdef _WIN32
    LeaveCriticalSection(&web_worker_lock);
#else
    pthread_mutex_unlock(&web_worker_lock);
#endif
    WispWorkerHandle *h = calloc(1, sizeof(WispWorkerHandle));
    if (!h) {
#ifdef _WIN32
        EnterCriticalSection(&web_worker_lock);
#else
        pthread_mutex_lock(&web_worker_lock);
#endif
        active_web_workers--;
#ifdef _WIN32
        LeaveCriticalSection(&web_worker_lock);
#else
        pthread_mutex_unlock(&web_worker_lock);
#endif
        return NULL;
    }
    wisp_message_queue_init(&h->to_worker);
    wisp_message_queue_init(&h->from_worker);
    h->script_url = strdup(script_url);
    if (!h->script_url) {
        wisp_message_queue_deinit(&h->to_worker);
        wisp_message_queue_deinit(&h->from_worker);
        free(h);
#ifdef _WIN32
        EnterCriticalSection(&web_worker_lock);
#else
        pthread_mutex_lock(&web_worker_lock);
#endif
        active_web_workers--;
#ifdef _WIN32
        LeaveCriticalSection(&web_worker_lock);
#else
        pthread_mutex_unlock(&web_worker_lock);
#endif
        return NULL;
    }
    h->ref_count = 2; /* 1 for JS main thread object, 1 for web worker thread */
#ifdef _WIN32
    h->thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)wisp_web_worker_routine, h, 0, NULL);
    if (h->thread == NULL) {
        free(h->script_url);
        wisp_message_queue_deinit(&h->to_worker);
        wisp_message_queue_deinit(&h->from_worker);
        free(h);
        EnterCriticalSection(&web_worker_lock);
        active_web_workers--;
        LeaveCriticalSection(&web_worker_lock);
        return NULL;
    }
#else
    int ret = pthread_create(&h->thread, NULL, wisp_web_worker_routine, h);
    if (ret != 0) {
        free(h->script_url);
        wisp_message_queue_deinit(&h->to_worker);
        wisp_message_queue_deinit(&h->from_worker);
        free(h);
        pthread_mutex_lock(&web_worker_lock);
        active_web_workers--;
        pthread_mutex_unlock(&web_worker_lock);
        return NULL;
    }
#endif
    return h;
}

void wisp_worker_handle_ref(WispWorkerHandle *h) {
    if (!h) return;
#ifdef _WIN32
    EnterCriticalSection(&web_worker_lock);
    h->ref_count++;
    LeaveCriticalSection(&web_worker_lock);
#else
    pthread_mutex_lock(&web_worker_lock);
    h->ref_count++;
    pthread_mutex_unlock(&web_worker_lock);
#endif
}

void wisp_worker_handle_unref(WispWorkerHandle *h) {
    if (!h) return;
    bool should_free = false;
#ifdef _WIN32
    EnterCriticalSection(&web_worker_lock);
    h->ref_count--;
    if (h->ref_count == 0) {
        should_free = true;
    }
    LeaveCriticalSection(&web_worker_lock);
#else
    pthread_mutex_lock(&web_worker_lock);
    h->ref_count--;
    if (h->ref_count == 0) {
        should_free = true;
    }
    pthread_mutex_unlock(&web_worker_lock);
#endif

    if (should_free) {
#ifdef _WIN32
        if (h->thread) {
            CloseHandle(h->thread);
        }
#else
        pthread_detach(h->thread);
#endif
        wisp_message_queue_deinit(&h->to_worker);
        wisp_message_queue_deinit(&h->from_worker);
        free(h->script_url);
        free(h);
    }
}
