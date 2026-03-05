#include "wisp/ipc/task_queue.h"
#include <stdlib.h>

nserror task_queue_init(struct task_queue **queue_out) {
    struct task_queue *queue = calloc(1, sizeof(struct task_queue));
    if (!queue) return NSERROR_NOMEM;

    if (pthread_mutex_init(&queue->lock, NULL) != 0) {
        free(queue);
        return NSERROR_INIT_FAILED;
    }
    if (pthread_cond_init(&queue->cond, NULL) != 0) {
        pthread_mutex_destroy(&queue->lock);
        free(queue);
        return NSERROR_INIT_FAILED;
    }

    *queue_out = queue;
    return NSERROR_OK;
}

nserror task_queue_push(struct task_queue *queue, task_func func, void *ctx) {
    struct task *t = calloc(1, sizeof(struct task));
    if (!t) return NSERROR_NOMEM;
    t->func = func;
    t->ctx = ctx;

    pthread_mutex_lock(&queue->lock);
    if (queue->stop) {
        pthread_mutex_unlock(&queue->lock);
        free(t);
        return NSERROR_STOPPED;
    }

    if (queue->tail) {
        queue->tail->next = t;
    } else {
        queue->head = t;
    }
    queue->tail = t;

    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->lock);

    return NSERROR_OK;
}

nserror task_queue_pop(struct task_queue *queue, task_func *func, void **ctx) {
    pthread_mutex_lock(&queue->lock);

    while (!queue->head && !queue->stop) {
        pthread_cond_wait(&queue->cond, &queue->lock);
    }

    if (queue->stop && !queue->head) {
        pthread_mutex_unlock(&queue->lock);
        return NSERROR_EOF;
    }

    struct task *t = queue->head;
    queue->head = t->next;
    if (!queue->head) {
        queue->tail = NULL;
    }

    pthread_mutex_unlock(&queue->lock);

    *func = t->func;
    *ctx = t->ctx;
    free(t);

    return NSERROR_OK;
}

void task_queue_stop(struct task_queue *queue) {
    if (!queue) return;
    pthread_mutex_lock(&queue->lock);
    queue->stop = true;
    pthread_cond_broadcast(&queue->cond);
    pthread_mutex_unlock(&queue->lock);
}

void task_queue_destroy(struct task_queue *queue) {
    if (!queue) return;
    struct task *t = queue->head;
    while (t) {
        struct task *next = t->next;
        free(t);
        t = next;
    }
    pthread_mutex_destroy(&queue->lock);
    pthread_cond_destroy(&queue->cond);
    free(queue);
}
