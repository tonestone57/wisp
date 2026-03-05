#ifndef WISP_IPC_TASK_QUEUE_H_
#define WISP_IPC_TASK_QUEUE_H_

#include <stdbool.h>
#include <pthread.h>
#include "wisp/utils/errors.h"

typedef void (*task_func)(void *ctx);

struct task {
    task_func func;
    void *ctx;
    struct task *next;
};

struct task_queue {
    struct task *head;
    struct task *tail;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    bool stop;
};

nserror task_queue_init(struct task_queue **queue_out);
nserror task_queue_push(struct task_queue *queue, task_func func, void *ctx);
nserror task_queue_pop(struct task_queue *queue, task_func *func, void **ctx);
void task_queue_stop(struct task_queue *queue);
void task_queue_destroy(struct task_queue *queue);

#endif
