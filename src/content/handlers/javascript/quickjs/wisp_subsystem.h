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

#define WISP_DEQUE_SIZE 4096

typedef struct wisp_deque_t {
    volatile int64_t head;
    volatile int64_t tail;
    js_task_t * volatile tasks[WISP_DEQUE_SIZE];
#ifdef _WIN32
    CRITICAL_SECTION lock;
#else
    pthread_mutex_t lock;
#endif
} wisp_deque_t;

#ifdef _MSC_VER
#include <windows.h>
#define __ATOMIC_RELAXED 0
#define __ATOMIC_CONSUME 1
#define __ATOMIC_ACQUIRE 2
#define __ATOMIC_RELEASE 3
#define __ATOMIC_ACQ_REL 4
#define __ATOMIC_SEQ_CST 5

static inline int64_t msvc_atomic_load_64(volatile void *ptr, int memorder) {
    return InterlockedOr64((volatile int64_t *)ptr, 0);
}
static inline int32_t msvc_atomic_load_32(volatile void *ptr, int memorder) {
    return InterlockedOr((volatile int32_t *)ptr, 0);
}
static inline int16_t msvc_atomic_load_16(volatile void *ptr, int memorder) {
    return InterlockedOr16((volatile short *)ptr, 0);
}
static inline int8_t msvc_atomic_load_8(volatile void *ptr, int memorder) {
    return InterlockedOr8((volatile char *)ptr, 0);
}

static inline void msvc_atomic_store_64(volatile void *ptr, int64_t val, int memorder) {
    InterlockedExchange64((volatile int64_t *)ptr, val);
}
static inline void msvc_atomic_store_32(volatile void *ptr, int32_t val, int memorder) {
    InterlockedExchange((volatile int32_t *)ptr, val);
}
static inline void msvc_atomic_store_16(volatile void *ptr, int16_t val, int memorder) {
    InterlockedExchange16((volatile short *)ptr, val);
}
static inline void msvc_atomic_store_8(volatile void *ptr, int8_t val, int memorder) {
    InterlockedExchange8((volatile char *)ptr, val);
}

static inline int64_t msvc_atomic_add_fetch_64(volatile void *ptr, int64_t val, int memorder) {
    return InterlockedAdd64((volatile int64_t *)ptr, val);
}
static inline int32_t msvc_atomic_add_fetch_32(volatile void *ptr, int32_t val, int memorder) {
    return InterlockedExchangeAdd((volatile int32_t *)ptr, val) + val;
}

static inline int64_t msvc_atomic_sub_fetch_64(volatile void *ptr, int64_t val, int memorder) {
    return InterlockedAdd64((volatile int64_t *)ptr, -val);
}
static inline int32_t msvc_atomic_sub_fetch_32(volatile void *ptr, int32_t val, int memorder) {
    return InterlockedExchangeAdd((volatile int32_t *)ptr, -val) - val;
}

static inline int64_t msvc_atomic_fetch_add_64(volatile void *ptr, int64_t val, int memorder) {
    return InterlockedExchangeAdd64((volatile int64_t *)ptr, val);
}
static inline int32_t msvc_atomic_fetch_add_32(volatile void *ptr, int32_t val, int memorder) {
    return InterlockedExchangeAdd((volatile int32_t *)ptr, val);
}

static inline int64_t msvc_atomic_exchange_64(volatile void *ptr, int64_t val, int memorder) {
    return InterlockedExchange64((volatile int64_t *)ptr, val);
}
static inline int32_t msvc_atomic_exchange_32(volatile void *ptr, int32_t val, int memorder) {
    return InterlockedExchange((volatile int32_t *)ptr, val);
}

static inline bool msvc_atomic_compare_exchange_64(volatile void *ptr, int64_t *expected, int64_t desired) {
    int64_t old = InterlockedCompareExchange64((volatile int64_t *)ptr, desired, *expected);
    if (old == *expected) {
        return true;
    } else {
        *expected = old;
        return false;
    }
}
static inline bool msvc_atomic_compare_exchange_32(volatile void *ptr, int32_t *expected, int32_t desired) {
    int32_t old = InterlockedCompareExchange((volatile int32_t *)ptr, desired, *expected);
    if (old == *expected) {
        return true;
    } else {
        *expected = old;
        return false;
    }
}

static inline void msvc_atomic_thread_fence(int memorder) {
    MemoryBarrier();
}

#define __atomic_load_n(ptr, memorder) \
    ((sizeof(*(ptr)) == 8) ? msvc_atomic_load_64((volatile void *)(ptr), memorder) : \
     (sizeof(*(ptr)) == 4) ? msvc_atomic_load_32((volatile void *)(ptr), memorder) : \
     (sizeof(*(ptr)) == 2) ? msvc_atomic_load_16((volatile void *)(ptr), memorder) : \
     msvc_atomic_load_8((volatile void *)(ptr), memorder))

#define __atomic_store_n(ptr, val, memorder) \
    ((sizeof(*(ptr)) == 8) ? msvc_atomic_store_64((volatile void *)(ptr), (int64_t)(val), memorder) : \
     (sizeof(*(ptr)) == 4) ? msvc_atomic_store_32((volatile void *)(ptr), (int32_t)(val), memorder) : \
     (sizeof(*(ptr)) == 2) ? msvc_atomic_store_16((volatile void *)(ptr), (int16_t)(val), memorder) : \
     msvc_atomic_store_8((volatile void *)(ptr), (int8_t)(val), memorder))

#define __atomic_add_fetch(ptr, val, memorder) \
    ((sizeof(*(ptr)) == 8) ? msvc_atomic_add_fetch_64((volatile void *)(ptr), (int64_t)(val), memorder) : \
     msvc_atomic_add_fetch_32((volatile void *)(ptr), (int32_t)(val), memorder))

#define __atomic_sub_fetch(ptr, val, memorder) \
    ((sizeof(*(ptr)) == 8) ? msvc_atomic_sub_fetch_64((volatile void *)(ptr), (int64_t)(val), memorder) : \
     msvc_atomic_sub_fetch_32((volatile void *)(ptr), (int32_t)(val), memorder))

#define __atomic_fetch_add(ptr, val, memorder) \
    ((sizeof(*(ptr)) == 8) ? msvc_atomic_fetch_add_64((volatile void *)(ptr), (int64_t)(val), memorder) : \
     msvc_atomic_fetch_add_32((volatile void *)(ptr), (int32_t)(val), memorder))

#define __atomic_exchange_n(ptr, val, memorder) \
    ((sizeof(*(ptr)) == 8) ? msvc_atomic_exchange_64((volatile void *)(ptr), (int64_t)(val), memorder) : \
     (sizeof(*(ptr)) == 4) ? msvc_atomic_exchange_32((volatile void *)(ptr), (int32_t)(val), memorder) : \
     *(ptr))

#define __atomic_compare_exchange_n(ptr, expected, desired, weak, memorder1, memorder2) \
    ((sizeof(*(ptr)) == 8) ? msvc_atomic_compare_exchange_64((volatile void *)(ptr), (int64_t *)(expected), (int64_t)(desired)) : \
     msvc_atomic_compare_exchange_32((volatile void *)(ptr), (int32_t *)(expected), (int32_t)(desired)))

#define __atomic_thread_fence(memorder) msvc_atomic_thread_fence(memorder)
#endif

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
    volatile int64_t main_thread_notified;
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
    volatile int64_t running;
    JSRuntime *rt;
    JSContext *ctx;
    struct WispPool *pool;
    wisp_deque_t deques[4];
    volatile int64_t deficit[4];
} WispWorker;

typedef struct WispPool {
    js_task_t *head;
    js_task_t *tail;
    volatile int64_t count;
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
    volatile int64_t dispatch_idx;
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
bool wisp_execute_pending_task(void);
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
