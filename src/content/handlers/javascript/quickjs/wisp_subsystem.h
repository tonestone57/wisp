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

typedef struct {
    js_task_t *tasks;
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
} WispQueue;

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
} WispWorker;

// The globals expected by qjs.c
// extern WispWorker *wisp_worker_pool;
// extern WispQueue wisp_queue;
// extern int wisp_worker_count;

void init_wisp_subsystem(int queue_size);
void shutdown_wisp_subsystem(void);
void* wisp_worker_routine(void *arg);
void wisp_dispatch(char *script, void (*func)(void*), void *arg);

#endif // WISP_SUBSYSTEM_H
