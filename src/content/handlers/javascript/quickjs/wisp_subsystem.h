#ifndef WISP_SUBSYSTEM_H
#define WISP_SUBSYSTEM_H

#include "quickjs.h"
#include <stdbool.h>
#include <stdatomic.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

typedef struct js_task_t {
    struct js_task_t *next;
    char *script;
    void (*function)(void*);
    void *arg;
} js_task_t;

typedef struct WispPool WispPool;

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
    WispPool *pool;
} WispWorker;

struct WispPool {
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
    bool is_js;
};

extern WispPool raster_pool;
extern WispPool js_pool;

void init_wisp_subsystem(int queue_size);
void shutdown_wisp_subsystem(void);
void* wisp_worker_routine(void *arg);
void wisp_dispatch_raster(void (*func)(void*), void *arg);
void wisp_dispatch_js(char *script, void (*func)(void*), void *arg);

#endif
