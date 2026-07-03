#ifndef WISP_SUBSYSTEM_H
#define WISP_SUBSYSTEM_H

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include "quickjs.h"

#ifdef _WIN32
#include <windows.h>
#endif

typedef struct js_task_t {
    struct js_task_t *next;
    char *script;
    void (*function)(void*);
    void *arg;
} js_task_t;

struct WispPool;

typedef struct {
#ifdef _WIN32
    HANDLE thread;
#else
    pthread_t thread;
#endif
    int worker_id;
    atomic_bool running;
    JSRuntime *rt;
    JSContext *ctx;
    struct WispPool *pool;
} WispWorker;

typedef struct WispPool {
    js_task_t *head;
    js_task_t *tail;
    int count;
    int capacity;
    bool stop;
#ifdef _WIN32
    CRITICAL_SECTION lock;
    CONDITION_VARIABLE cond;
#else
    pthread_mutex_t lock;
    pthread_cond_t cond;
#endif
    WispWorker *workers;
    int worker_count;
    int active_workers;
    int busy_workers;
} WispPool;

/* Global pools */
extern WispPool *raster_pool;
extern WispPool *js_pool;

/* Subsystem management */
void init_wisp_subsystem(int queue_size);
void shutdown_wisp_subsystem(void);

/* Task dispatching */
void wisp_dispatch_raster(char *script, void (*func)(void*), void *arg);
void wisp_dispatch_js(char *script, void (*func)(void*), void *arg);
/* Deprecated/Compatibility wrapper */
void wisp_dispatch(char *script, void (*func)(void*), void *arg);

/* Internal worker routine */
void* wisp_worker_routine(void *arg);

#endif // WISP_SUBSYSTEM_H
