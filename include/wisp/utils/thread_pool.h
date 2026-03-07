#ifndef WISP_THREAD_POOL_H
#define WISP_THREAD_POOL_H

#include <stdbool.h>

typedef struct thread_pool thread_pool_t;

typedef void (*thread_pool_task_fn)(void *arg);

thread_pool_t *thread_pool_create(int num_threads);
void thread_pool_destroy(thread_pool_t *pool);
bool thread_pool_add_task(thread_pool_t *pool, thread_pool_task_fn func, void *arg);

#endif
