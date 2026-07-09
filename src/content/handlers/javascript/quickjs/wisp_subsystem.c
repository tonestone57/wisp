#include "wisp_subsystem.h"
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
    while (__atomic_load_n(&worker->running, __ATOMIC_RELAXED)) {
        js_task_t *task = NULL;
        bool has_task = false;
#ifdef _WIN32
        EnterCriticalSection(&pool->lock);
        while (pool->head == NULL && __atomic_load_n(&worker->running, __ATOMIC_RELAXED) && !pool->stop) {
            BOOL wait_res = SleepConditionVariableCS(&pool->cond, &pool->lock, 5000);
            if (!wait_res && GetLastError() == ERROR_TIMEOUT) {
                if (pool->head == NULL && pool->active_workers > 1 && !pool->stop) {
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
        if (pool->head != NULL && __atomic_load_n(&worker->running, __ATOMIC_RELAXED)) {
            task = pool->head; pool->head = task->next;
            if (pool->head == NULL) pool->tail = NULL;
            pool->count--; has_task = true; pool->busy_workers++;
        }
        LeaveCriticalSection(&pool->lock);
#else
        pthread_mutex_lock(&pool->lock);
        while (pool->head == NULL && __atomic_load_n(&worker->running, __ATOMIC_RELAXED) && !pool->stop) {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 5;
            int wait_res = pthread_cond_timedwait(&pool->cond, &pool->lock, &ts);
            if (wait_res == ETIMEDOUT) {
                if (pool->head == NULL && pool->active_workers > 1 && !pool->stop) {
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
        if (pool->head != NULL && __atomic_load_n(&worker->running, __ATOMIC_RELAXED)) {
            task = pool->head; pool->head = task->next;
            if (pool->head == NULL) pool->tail = NULL;
            pool->count--; has_task = true; pool->busy_workers++;
        }
        pthread_mutex_unlock(&pool->lock);
#endif
        if (has_task && task) {
            if (task->script) {
                JSValue val = js_eval_with_aot_cache(worker->ctx, (const uint8_t *)task->script, strlen(task->script), "<eval>", JS_EVAL_TYPE_GLOBAL);
                JS_FreeValue(worker->ctx, val);
                JSContext *ctx1;
                while (JS_ExecutePendingJob(JS_GetRuntime(worker->ctx), &ctx1) > 0);
            }
            if (task->function) task->function(task->arg);
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
    if (!pool) return false;
    js_task_t *new_task = malloc(sizeof(js_task_t));
    if (!new_task) return false;
    new_task->next = NULL;
    new_task->script = script ? strdup(script) : NULL;
    new_task->function = func;
    new_task->arg = arg;
    new_task->priority = priority;
    nsu_getmonotonic_ms(&new_task->entry_time);
    if (script && !new_task->script) { free(new_task); return false; }
    int worker_to_start = -1;
#ifdef _WIN32
    EnterCriticalSection(&pool->lock);
#else
    pthread_mutex_lock(&pool->lock);
#endif
    if (pool->count < pool->capacity) {
        uint64_t now; nsu_getmonotonic_ms(&now);
        js_task_t *prev = NULL;
        js_task_t *curr = pool->head;
        while (curr != NULL) {
            float curr_priority = curr->priority;
            uint64_t age = now - curr->entry_time;
            if (age > 5000) curr_priority += (float)(age - 5000) / 1000.0f;
            if (curr_priority < priority) break;
            prev = curr; curr = curr->next;
        }
        new_task->next = curr;
        if (prev == NULL) pool->head = new_task; else prev->next = new_task;
        if (curr == NULL) pool->tail = new_task;
        pool->count++;
        if (pool->busy_workers == pool->active_workers && pool->active_workers < pool->worker_count) {
            for (int i=0; i<pool->worker_count; i++) {
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
        WakeConditionVariable(&pool->cond); LeaveCriticalSection(&pool->lock);
#else
        pthread_cond_signal(&pool->cond); pthread_mutex_unlock(&pool->lock);
#endif
        if (worker_to_start != -1) start_worker(pool, worker_to_start);
        return true;
    } else {
#ifdef _WIN32
        LeaveCriticalSection(&pool->lock);
#else
        pthread_mutex_unlock(&pool->lock);
#endif
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
                memcpy(req->out_buffer, data, req->out_len);
                req->success = true;
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
    if (qjs_init_worker_thread(h, &t) != NSERROR_OK) { h->running = false; return NULL; }
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
            JSValue stack = JS_GetPropertyStr(t->ctx, exc, "stack");
            const char *msg_str = JS_ToCString(t->ctx, exc);
            WispMessage *errMsg = calloc(1, sizeof(*errMsg));
            errMsg->type = WISP_MSG_TYPE_ERROR;
            errMsg->error_message = strdup(msg_str ? msg_str : "Unknown error");
            wisp_message_queue_push(&h->from_worker, errMsg);
            wisp_worker_notify_main_thread(h);
            JS_FreeCString(t->ctx, msg_str); JS_FreeValue(t->ctx, stack); JS_FreeValue(t->ctx, exc);
        }
        JS_FreeValue(t->ctx, res);
        free(req.out_buffer);
    }
    pthread_mutex_destroy(&req.mutex); pthread_cond_destroy(&req.cond);

    h->running = true;
    while (h->running && !h->terminated) {
        JSContext *ctx1;
        while (JS_ExecutePendingJob(JS_GetRuntime(t->ctx), &ctx1) > 0);
        WispMessage *msg = wisp_message_queue_pop(&h->to_worker, 100);
        if (msg) {
            JSValue global = JS_GetGlobalObject(t->ctx);
            JSValue msg_data = JS_ReadObject(t->ctx, msg->data, msg->size, JS_READ_OBJ_SAB | JS_READ_OBJ_REFERENCE);
            extern JSValue qjs_new_messageevent_manual(JSContext *ctx, JSValue data);
            JSValue event = qjs_new_messageevent_manual(t->ctx, msg_data);
            JSValue onmessage = JS_GetPropertyStr(t->ctx, global, "onmessage");
            if (JS_IsFunction(t->ctx, onmessage)) {
                JSValue ret = JS_Call(t->ctx, onmessage, global, 1, &event);
                if (JS_IsException(ret)) {
                    JSValue exc = JS_GetException(t->ctx);
                    const char *exc_str = JS_ToCString(t->ctx, exc);
                    WispMessage *errMsg = calloc(1, sizeof(*errMsg));
                    errMsg->type = WISP_MSG_TYPE_ERROR;
                    errMsg->error_message = strdup(exc_str ? exc_str : "Unknown error");
                    wisp_message_queue_push(&h->from_worker, errMsg);
                    wisp_worker_notify_main_thread(h);
                    JS_FreeCString(t->ctx, exc_str); JS_FreeValue(t->ctx, exc);
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
    if (!h) return NULL;
    wisp_message_queue_init(&h->to_worker);
    wisp_message_queue_init(&h->from_worker);
    h->script_url = strdup(script_url);
#ifdef _WIN32
    h->thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)wisp_web_worker_routine, h, 0, NULL);
#else
    pthread_create(&h->thread, NULL, wisp_web_worker_routine, h);
#endif
    return h;
}
