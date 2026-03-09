#include "wisp_subsystem.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "wisp/utils/log.h"

#ifdef _WIN32
#else
#include <sys/sysinfo.h>
#include <sys/time.h>
#endif

static WispWorker *wisp_worker_pool = NULL;
static WispQueue wisp_queue;
static int wisp_worker_count = 0;

static int active_workers = 0;
static int busy_workers = 0;

static void start_worker(int i) {
    wisp_worker_pool[i].worker_id = i;
    wisp_worker_pool[i].rt = JS_NewRuntime();
    wisp_worker_pool[i].ctx = JS_NewContext(wisp_worker_pool[i].rt);

#ifdef _WIN32
    wisp_worker_pool[i].thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)wisp_worker_routine, &wisp_worker_pool[i], 0, NULL);
#else
    pthread_create(&wisp_worker_pool[i].thread, NULL, wisp_worker_routine, &wisp_worker_pool[i]);
#endif

#ifdef _WIN32
    EnterCriticalSection(&wisp_queue.lock);
#else
    pthread_mutex_lock(&wisp_queue.lock);
#endif
    active_workers++;
#ifdef _WIN32
    LeaveCriticalSection(&wisp_queue.lock);
#else
    pthread_mutex_unlock(&wisp_queue.lock);
#endif
}

void init_wisp_subsystem(int queue_size) {
    if (wisp_worker_pool != NULL) {
        return; // Already initialized
    }

    // 1. Determine dynamic worker max count
    long n_cores;
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    n_cores = sysinfo.dwNumberOfProcessors;
#else
    n_cores = sysconf(_SC_NPROCESSORS_ONLN);
#endif

    wisp_worker_count = (int)(n_cores - 1);

    // Manual Clamp: min 1, max 7
    if (wisp_worker_count < 1) wisp_worker_count = 1;
    if (wisp_worker_count > 7) wisp_worker_count = 7;

    // 2. Allocate the worker pool for max workers (not all spawned yet)
    wisp_worker_pool = calloc(wisp_worker_count, sizeof(WispWorker));

#ifndef _WIN32
    pthread_t null_thread;
    memset(&null_thread, 0, sizeof(pthread_t));
    for (int i=0; i<wisp_worker_count; i++) {
        wisp_worker_pool[i].thread = null_thread;
    }
#endif

    // 3. Initialize the Queue
    wisp_queue.capacity = queue_size;
    wisp_queue.head = NULL;
    wisp_queue.tail = NULL;
    wisp_queue.count = 0;
    wisp_queue.stop = false;

#ifdef _WIN32
    InitializeCriticalSection(&wisp_queue.lock);
    InitializeConditionVariable(&wisp_queue.cond);
#else
    pthread_mutex_init(&wisp_queue.lock, NULL);
    pthread_cond_init(&wisp_queue.cond, NULL);
#endif

    // 4. Spawn the seed thread
    wisp_worker_pool[0].running = true;
    start_worker(0);
}

void shutdown_wisp_subsystem(void) {
    if (wisp_worker_pool == NULL) return;

    // 1. Signal workers to stop
#ifdef _WIN32
    EnterCriticalSection(&wisp_queue.lock);
    wisp_queue.stop = true;
    for (int i = 0; i < wisp_worker_count; i++) {
        wisp_worker_pool[i].running = false;
    }
    WakeAllConditionVariable(&wisp_queue.cond);
    LeaveCriticalSection(&wisp_queue.lock);
#else
    pthread_mutex_lock(&wisp_queue.lock);
    wisp_queue.stop = true;
    for (int i = 0; i < wisp_worker_count; i++) {
        wisp_worker_pool[i].running = false;
    }
    pthread_cond_broadcast(&wisp_queue.cond);
    pthread_mutex_unlock(&wisp_queue.lock);
#endif

    // 2. Join threads and free contexts
    for (int i = 0; i < wisp_worker_count; i++) {
#ifdef _WIN32
        if (wisp_worker_pool[i].thread) {
            WaitForSingleObject(wisp_worker_pool[i].thread, INFINITE);
            CloseHandle(wisp_worker_pool[i].thread);
        }
#else
        pthread_t null_thread;
        memset(&null_thread, 0, sizeof(pthread_t));
        if (memcmp(&wisp_worker_pool[i].thread, &null_thread, sizeof(pthread_t)) != 0) {
            pthread_join(wisp_worker_pool[i].thread, NULL);
        }
#endif
        if (wisp_worker_pool[i].ctx != NULL) {
            JS_FreeContext(wisp_worker_pool[i].ctx);
        }
        if (wisp_worker_pool[i].rt != NULL) {
            JS_FreeRuntime(wisp_worker_pool[i].rt);
        }
    }

    // 3. Free queue tasks
    js_task_t *task = wisp_queue.head;
    while (task) {
        js_task_t *next = task->next;
        if (task->script) free(task->script);
        free(task);
        task = next;
    }
    wisp_queue.head = NULL;
    wisp_queue.tail = NULL;
    wisp_queue.count = 0;

#ifdef _WIN32
    DeleteCriticalSection(&wisp_queue.lock);
#else
    pthread_mutex_destroy(&wisp_queue.lock);
    pthread_cond_destroy(&wisp_queue.cond);
#endif

    free(wisp_worker_pool);
    wisp_worker_pool = NULL;
    active_workers = 0;
    busy_workers = 0;
}

void* wisp_worker_routine(void *arg) {
    WispWorker *worker = (WispWorker *)arg;

    while (worker->running) {
        js_task_t *task = NULL;
        bool has_task = false;

#ifdef _WIN32
        EnterCriticalSection(&wisp_queue.lock);
        while (wisp_queue.head == NULL && worker->running && !wisp_queue.stop) {
            BOOL wait_res = SleepConditionVariableCS(&wisp_queue.cond, &wisp_queue.lock, 5000); // 5 sec TTL
            if (!wait_res && GetLastError() == ERROR_TIMEOUT) {
                if (wisp_queue.head == NULL && active_workers > 1) {
                    // Time to live expired, scale down
                    worker->running = false;
                    active_workers--;
                    LeaveCriticalSection(&wisp_queue.lock);
                    // Free context
                    JS_FreeContext(worker->ctx);
                    JS_FreeRuntime(worker->rt);
                    worker->ctx = NULL;
                    worker->rt = NULL;
                    worker->thread = NULL;
                    return NULL;
                }
            }
        }

        if (wisp_queue.head != NULL && worker->running) {
            task = wisp_queue.head;
            wisp_queue.head = task->next;
            if (wisp_queue.head == NULL) {
                wisp_queue.tail = NULL;
            }
            wisp_queue.count--;
            has_task = true;
            busy_workers++;
        }
        LeaveCriticalSection(&wisp_queue.lock);
#else
        pthread_mutex_lock(&wisp_queue.lock);
        while (wisp_queue.head == NULL && worker->running && !wisp_queue.stop) {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 5; // 5 sec TTL

            int wait_res = pthread_cond_timedwait(&wisp_queue.cond, &wisp_queue.lock, &ts);
            if (wait_res != 0) { // Timeout or error
                if (wisp_queue.head == NULL && active_workers > 1) {
                    // Time to live expired, scale down
                    worker->running = false;
                    active_workers--;

                    pthread_t null_thread;
                    memset(&null_thread, 0, sizeof(pthread_t));
                    worker->thread = null_thread;
                    pthread_mutex_unlock(&wisp_queue.lock);

                    // Detach so resources are freed immediately upon exit
                    pthread_detach(pthread_self());

                    // Free context
                    JS_FreeContext(worker->ctx);
                    JS_FreeRuntime(worker->rt);
                    worker->ctx = NULL;
                    worker->rt = NULL;
                    return NULL;
                }
            }
        }

        if (wisp_queue.head != NULL && worker->running) {
            task = wisp_queue.head;
            wisp_queue.head = task->next;
            if (wisp_queue.head == NULL) {
                wisp_queue.tail = NULL;
            }
            wisp_queue.count--;
            has_task = true;
            busy_workers++;
        }
        pthread_mutex_unlock(&wisp_queue.lock);
#endif

        if (has_task && task) {
            if (task->function) {
                task->function(task->arg);
            }
            if (task->script) free(task->script);
            free(task);

#ifdef _WIN32
            EnterCriticalSection(&wisp_queue.lock);
            busy_workers--;
            LeaveCriticalSection(&wisp_queue.lock);
#else
            pthread_mutex_lock(&wisp_queue.lock);
            busy_workers--;
            pthread_mutex_unlock(&wisp_queue.lock);
#endif
        }
    }

    return NULL;
}

void wisp_dispatch(char *script, void (*func)(void*), void *arg) {
    js_task_t *new_task = malloc(sizeof(js_task_t));
    if (!new_task) return;
    new_task->next = NULL;
    new_task->script = script;
    new_task->function = func;
    new_task->arg = arg;

#ifdef _WIN32
    EnterCriticalSection(&wisp_queue.lock);
    if (wisp_queue.count < wisp_queue.capacity) {
        if (wisp_queue.tail == NULL) {
            wisp_queue.head = new_task;
            wisp_queue.tail = new_task;
        } else {
            wisp_queue.tail->next = new_task;
            wisp_queue.tail = new_task;
        }
        wisp_queue.count++;

        // Scale up if all workers are busy and we haven't reached max
        int worker_to_start = -1;
        if (busy_workers == active_workers && active_workers < wisp_worker_count) {
            for (int i=0; i<wisp_worker_count; i++) {
                if (!wisp_worker_pool[i].running) {
                    worker_to_start = i;
                    wisp_worker_pool[i].running = true; // Mark as running so another thread doesn't pick it up
                    break;
                }
            }
        }

        WakeConditionVariable(&wisp_queue.cond);
        LeaveCriticalSection(&wisp_queue.lock);

        if (worker_to_start != -1) {
            start_worker(worker_to_start);
        }
    } else {
        LeaveCriticalSection(&wisp_queue.lock);
        if (new_task->script) {
            free(new_task->script);
        }
        free(new_task); // Queue full
    }
#else
    pthread_mutex_lock(&wisp_queue.lock);
    if (wisp_queue.count < wisp_queue.capacity) {
        if (wisp_queue.tail == NULL) {
            wisp_queue.head = new_task;
            wisp_queue.tail = new_task;
        } else {
            wisp_queue.tail->next = new_task;
            wisp_queue.tail = new_task;
        }
        wisp_queue.count++;

        // Scale up if all workers are busy and we haven't reached max
        int worker_to_start = -1;
        if (busy_workers == active_workers && active_workers < wisp_worker_count) {
            pthread_t null_thread;
            memset(&null_thread, 0, sizeof(pthread_t));
            for (int i=0; i<wisp_worker_count; i++) {
                if (!wisp_worker_pool[i].running && memcmp(&wisp_worker_pool[i].thread, &null_thread, sizeof(pthread_t)) == 0) {
                    worker_to_start = i;
                    wisp_worker_pool[i].running = true; // Mark as running so another thread doesn't pick it up
                    break;
                }
            }
        }

        pthread_cond_signal(&wisp_queue.cond);
        pthread_mutex_unlock(&wisp_queue.lock);

        if (worker_to_start != -1) {
            start_worker(worker_to_start);
        }
    } else {
        pthread_mutex_unlock(&wisp_queue.lock);
        if (new_task->script) {
            free(new_task->script);
        }
        free(new_task); // Queue full
    }
#endif
}
