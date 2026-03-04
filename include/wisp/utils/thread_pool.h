#ifndef WISP_UTILS_THREAD_POOL_H
#define WISP_UTILS_THREAD_POOL_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct thread_pool;
typedef struct thread_pool thread_pool_t;

/**
 * Task function prototype.
 *
 * @param arg Pointer to task-specific arguments.
 */
typedef void (*thread_pool_task_fn)(void *arg);

/**
 * Creates a new thread pool.
 *
 * @param num_threads The number of threads to create in the pool.
 * @return A pointer to the newly created thread pool, or NULL on failure.
 */
thread_pool_t *thread_pool_create(int num_threads);

/**
 * Destroys a thread pool, waiting for all currently executing tasks to finish.
 * Pending tasks in the queue might be discarded or completed depending on the implementation.
 *
 * @param pool The thread pool to destroy.
 */
void thread_pool_destroy(thread_pool_t *pool);

/**
 * Adds a new task to the thread pool queue.
 *
 * @param pool The thread pool.
 * @param function The task function to execute.
 * @param argument The argument to pass to the task function.
 * @return true if the task was successfully added, false otherwise.
 */
bool thread_pool_add_task(thread_pool_t *pool, thread_pool_task_fn function, void *argument);

#ifdef __cplusplus
}
#endif

#endif /* WISP_UTILS_THREAD_POOL_H */
