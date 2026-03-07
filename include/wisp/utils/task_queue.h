#ifndef WISP_TASK_QUEUE_H
#define WISP_TASK_QUEUE_H

#include <stdbool.h>

/**
 * Initialize the global task queue.
 * Must be called from the main thread during initialization.
 *
 * \return true on success, false on failure.
 */
bool task_queue_init(void);

/**
 * Destroy the global task queue.
 * Must be called from the main thread during shutdown.
 */
void task_queue_destroy(void);

/**
 * Post a task to be executed on the main thread.
 * Thread-safe. Can be called from any background thread.
 *
 * \param fn The function to execute.
 * \param arg The argument to pass to the function.
 * \return true on success, false if the task queue is not initialized.
 */
bool task_queue_post(void (*fn)(void *), void *arg);

/**
 * Execute all pending tasks in the queue.
 * MUST only be called from the main thread.
 */
void task_queue_execute_pending(void);

#endif
