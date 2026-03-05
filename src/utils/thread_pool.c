#include <wisp/utils/thread_pool.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct thread_pool_task {
    thread_pool_task_fn func;
    void *arg;
    struct thread_pool_task *next;
} thread_pool_task_t;

struct thread_pool {
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
    thread_pool_task_t *queue_head;
    thread_pool_task_t *queue_tail;
    int thread_count;
    int shutdown;
};

static void *thread_pool_worker(void *thread_pool) {
    thread_pool_t *pool = (thread_pool_t *)thread_pool;

    for (;;) {
        pthread_mutex_lock(&(pool->lock));

        while ((pool->queue_head == NULL) && (!pool->shutdown)) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        if (pool->shutdown) {
            pthread_mutex_unlock(&(pool->lock));
            pthread_exit(NULL);
        }

        thread_pool_task_t *task = pool->queue_head;
        pool->queue_head = pool->queue_head->next;
        if (pool->queue_head == NULL) {
            pool->queue_tail = NULL;
        }

        pthread_mutex_unlock(&(pool->lock));

        (*(task->func))(task->arg);
        free(task);
    }
    return NULL;
}

thread_pool_t *thread_pool_create(int num_threads) {
    if (num_threads <= 0) return NULL;

    thread_pool_t *pool = (thread_pool_t *)malloc(sizeof(thread_pool_t));
    if (pool == NULL) return NULL;

    pool->thread_count = num_threads;
    pool->shutdown = 0;
    pool->queue_head = NULL;
    pool->queue_tail = NULL;

    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);
    if (pool->threads == NULL) {
        free(pool);
        return NULL;
    }

    if (pthread_mutex_init(&(pool->lock), NULL) != 0 ||
        pthread_cond_init(&(pool->notify), NULL) != 0) {
        free(pool->threads);
        free(pool);
        return NULL;
    }

    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, thread_pool_worker, (void *)pool) != 0) {
            thread_pool_destroy(pool);
            return NULL;
        }
    }

    return pool;
}

void thread_pool_destroy(thread_pool_t *pool) {
    if (pool == NULL) return;

    pthread_mutex_lock(&(pool->lock));
    pool->shutdown = 1;
    pthread_cond_broadcast(&(pool->notify));
    pthread_mutex_unlock(&(pool->lock));

    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    thread_pool_task_t *task = pool->queue_head;
    while (task != NULL) {
        thread_pool_task_t *next = task->next;
        free(task);
        task = next;
    }

    pthread_mutex_destroy(&(pool->lock));
    pthread_cond_destroy(&(pool->notify));
    free(pool->threads);
    free(pool);
}

bool thread_pool_add_task(thread_pool_t *pool, thread_pool_task_fn func, void *arg) {
    if (pool == NULL || func == NULL) return false;

    thread_pool_task_t *task = (thread_pool_task_t *)malloc(sizeof(thread_pool_task_t));
    if (task == NULL) return false;

    task->func = func;
    task->arg = arg;
    task->next = NULL;

    pthread_mutex_lock(&(pool->lock));

    if (pool->shutdown) {
        pthread_mutex_unlock(&(pool->lock));
        free(task);
        return false;
    }

    if (pool->queue_tail == NULL) {
        pool->queue_head = task;
        pool->queue_tail = task;
    } else {
        pool->queue_tail->next = task;
        pool->queue_tail = task;
    }

    pthread_cond_signal(&(pool->notify));
    pthread_mutex_unlock(&(pool->lock));

    return true;
}
