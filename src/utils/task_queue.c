#include <wisp/utils/task_queue.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <wisp/desktop/gui_internal.h>
#include <wisp/misc.h>

typedef struct task_entry {
    void (*fn)(void *);
    void *arg;
    struct task_entry *next;
} task_entry_t;

static struct {
    pthread_mutex_t lock;
    task_entry_t *head;
    task_entry_t *tail;
    bool initialized;
} global_task_queue;

bool task_queue_init(void) {
    if (global_task_queue.initialized) return true;

    if (pthread_mutex_init(&global_task_queue.lock, NULL) != 0) {
        return false;
    }

    global_task_queue.head = NULL;
    global_task_queue.tail = NULL;
    global_task_queue.initialized = true;
    return true;
}

void task_queue_destroy(void) {
    if (!global_task_queue.initialized) return;

    pthread_mutex_lock(&global_task_queue.lock);
    task_entry_t *current = global_task_queue.head;
    while (current != NULL) {
        task_entry_t *next = current->next;
        free(current);
        current = next;
    }
    global_task_queue.head = NULL;
    global_task_queue.tail = NULL;
    pthread_mutex_unlock(&global_task_queue.lock);

    pthread_mutex_destroy(&global_task_queue.lock);
    global_task_queue.initialized = false;
}

bool task_queue_post(void (*fn)(void *), void *arg) {
    if (!global_task_queue.initialized || !fn) return false;

    task_entry_t *task = (task_entry_t *)malloc(sizeof(task_entry_t));
    if (!task) return false;

    task->fn = fn;
    task->arg = arg;
    task->next = NULL;

    pthread_mutex_lock(&global_task_queue.lock);
    bool was_empty = (global_task_queue.head == NULL);
    if (global_task_queue.tail == NULL) {
        global_task_queue.head = task;
        global_task_queue.tail = task;
    } else {
        global_task_queue.tail->next = task;
        global_task_queue.tail = task;
    }
    pthread_mutex_unlock(&global_task_queue.lock);

    if (was_empty && guit && guit->misc && guit->misc->task_queue_wake) {
        guit->misc->task_queue_wake();
    }

    return true;
}

void task_queue_execute_pending(void) {
    if (!global_task_queue.initialized) return;

    pthread_mutex_lock(&global_task_queue.lock);
    task_entry_t *local_list = global_task_queue.head;
    global_task_queue.head = NULL;
    global_task_queue.tail = NULL;
    pthread_mutex_unlock(&global_task_queue.lock);

    while (local_list != NULL) {
        task_entry_t *next = local_list->next;
        local_list->fn(local_list->arg);
        free(local_list);
        local_list = next;
    }
}
