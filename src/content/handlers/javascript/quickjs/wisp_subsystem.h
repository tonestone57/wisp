#ifndef WISP_SUBSYSTEM_H
#define WISP_SUBSYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"

#ifdef _WIN32
#include <windows.h>
#endif

typedef struct js_task_t {
    struct js_task_t *next;
    char *script;
    void (*function)(void*);
    void *arg;
    float priority;
    uint64_t entry_time;
} js_task_t;

typedef enum {
    WISP_MSG_TYPE_DATA,  /* Standard postMessage payload */
    WISP_MSG_TYPE_ERROR  /* Unhandled exception details */
} WispMsgType;

typedef struct WispMessage {
    struct WispMessage *next;
    WispMsgType type;
    uint8_t *data;
    size_t size;
    /* Error details (only used if type == WISP_MSG_TYPE_ERROR) */
    char *error_message;
    char *filename;
    int line_number;
    int col_number;
} WispMessage;

typedef struct WispMessageQueue {
    WispMessage *head;
    WispMessage *tail;
#ifdef _WIN32
    CRITICAL_SECTION lock;
    CONDITION_VARIABLE cond;
#else
    pthread_mutex_t lock;
    pthread_cond_t cond;
#endif
} WispMessageQueue;

typedef struct WispWorkerHandle {
#ifdef _WIN32
    HANDLE thread;
#else
    pthread_t thread;
#endif
    volatile bool running;
    volatile bool terminated;
    volatile bool main_thread_notified;
    WispMessageQueue to_worker;
    WispMessageQueue from_worker;
    char *script_url;
    void *worker_priv; /* Pointer to QJSWorkerPrivate (main thread side) */
    void *worker_js_thread; /* Pointer to worker's jsthread */
    int ref_count;
} WispWorkerHandle;

void wisp_worker_handle_ref(WispWorkerHandle *h);
void wisp_worker_handle_unref(WispWorkerHandle *h);

struct WispPool;

typedef struct {
#ifdef _WIN32
    HANDLE thread;
#else
    pthread_t thread;
#endif
    int worker_id;
    bool running;
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

WispWorkerHandle* wisp_subsystem_spawn_worker(const char *script_url);

/* Task dispatching */
bool wisp_dispatch_raster(const char *script, void (*func)(void*), void *arg, float priority);
bool wisp_dispatch_js(const char *script, void (*func)(void*), void *arg, float priority);
/* Deprecated/Compatibility wrapper */
void wisp_dispatch(char *script, void (*func)(void*), void *arg);

/* Internal worker routine */
void* wisp_worker_routine(void *arg);

/* Web Worker support */
void wisp_message_queue_init(WispMessageQueue *q);
void wisp_message_queue_deinit(WispMessageQueue *q);
void wisp_message_queue_push(WispMessageQueue *q, WispMessage *msg);
WispMessage* wisp_message_queue_pop(WispMessageQueue *q, int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // WISP_SUBSYSTEM_H
