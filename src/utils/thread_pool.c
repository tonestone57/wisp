#include <wisp/utils/thread_pool.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

struct thread_pool_task {
    thread_pool_task_fn function;
    void *argument;
    struct thread_pool_task *next;
};

struct thread_pool {
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
    struct thread_pool_task *queue_head;
    struct thread_pool_task *queue_tail;
    int thread_count;
    bool shutdown;
    int started;
};

static void *thread_pool_worker(void *threadpool)
{
    thread_pool_t *pool = (thread_pool_t *)threadpool;
    struct thread_pool_task *task;

    while (1) {
        pthread_mutex_lock(&pool->lock);

        while (pool->queue_head == NULL && !pool->shutdown) {
            pthread_cond_wait(&pool->notify, &pool->lock);
        }

        if (pool->shutdown && pool->queue_head == NULL) {
            break;
        }

        task = pool->queue_head;
        if (task != NULL) {
            pool->queue_head = task->next;
            if (pool->queue_head == NULL) {
                pool->queue_tail = NULL;
            }
        }
        pthread_mutex_unlock(&pool->lock);

        if (task != NULL) {
            (*(task->function))(task->argument);
            free(task);
        } else {
            /* Should not reach here if logic holds, but handle safely */
            continue;
        }
    }

    pool->started--;
    pthread_mutex_unlock(&pool->lock);
    pthread_exit(NULL);
    return NULL;
}

thread_pool_t *thread_pool_create(int num_threads)
{
    thread_pool_t *pool;
    int i;

    if (num_threads <= 0) {
        return NULL;
    }

    pool = calloc(1, sizeof(thread_pool_t));
    if (pool == NULL) {
        return NULL;
    }

    pool->thread_count = 0;
    pool->queue_head = NULL;
    pool->queue_tail = NULL;
    pool->shutdown = false;
    pool->started = 0;

    pool->threads = calloc((size_t)num_threads, sizeof(pthread_t));
    if (pool->threads == NULL) {
        free(pool);
        return NULL;
    }

    if (pthread_mutex_init(&pool->lock, NULL) != 0 ||
        pthread_cond_init(&pool->notify, NULL) != 0) {
        free(pool->threads);
        free(pool);
        return NULL;
    }

    for (i = 0; i < num_threads; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, thread_pool_worker, (void *)pool) == 0) {
            pool->thread_count++;
            pool->started++;
        } else {
            thread_pool_destroy(pool);
            return NULL;
        }
    }

    return pool;
}

bool thread_pool_add_task(thread_pool_t *pool, thread_pool_task_fn function, void *argument)
{
    struct thread_pool_task *new_task;

    if (pool == NULL || function == NULL) {
        return false;
    }

    new_task = malloc(sizeof(struct thread_pool_task));
    if (new_task == NULL) {
        return false;
    }

    new_task->function = function;
    new_task->argument = argument;
    new_task->next = NULL;

    if (pthread_mutex_lock(&pool->lock) != 0) {
        free(new_task);
        return false;
    }

    if (pool->shutdown) {
        free(new_task);
        pthread_mutex_unlock(&pool->lock);
        return false;
    }

    if (pool->queue_tail == NULL) {
        pool->queue_head = new_task;
        pool->queue_tail = new_task;
    } else {
        pool->queue_tail->next = new_task;
        pool->queue_tail = new_task;
    }

    if (pthread_cond_signal(&pool->notify) != 0) {
        pthread_mutex_unlock(&pool->lock);
        return false;
    }

    pthread_mutex_unlock(&pool->lock);
    return true;
}

void thread_pool_destroy(thread_pool_t *pool)
{
    int i;
    struct thread_pool_task *task, *next_task;

    if (pool == NULL) {
        return;
    }

    if (pthread_mutex_lock(&pool->lock) != 0) {
        return;
    }

    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->lock);
        return;
    }

    pool->shutdown = true;

    if (pthread_cond_broadcast(&pool->notify) != 0 ||
        pthread_mutex_unlock(&pool->lock) != 0) {
        return;
    }

    for (i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    task = pool->queue_head;
    while (task != NULL) {
        next_task = task->next;
        free(task);
        task = next_task;
    }

    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->notify);
    free(pool->threads);
    free(pool);
}
