#include "tile_pool.h"
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "wisp/utils/log.h"

#define TILE_BUFFER_SIZE (512 * 512 * 4) // 1MB

struct tile_pool {
    void **buffers;
    size_t capacity;
    size_t count;
    pthread_mutex_t lock;
};

static struct tile_pool *global_pool = NULL;

bool tile_pool_init(size_t pool_size) {
    if (global_pool != NULL) return true;

    global_pool = calloc(1, sizeof(struct tile_pool));
    if (!global_pool) return false;

    global_pool->buffers = malloc(sizeof(void *) * pool_size);
    if (!global_pool->buffers) {
        free(global_pool);
        global_pool = NULL;
        return false;
    }

    pthread_mutex_init(&global_pool->lock, NULL);
    global_pool->capacity = pool_size;
    global_pool->count = 0;

    for (size_t i = 0; i < pool_size; i++) {
        global_pool->buffers[i] = malloc(TILE_BUFFER_SIZE);
        if (!global_pool->buffers[i]) {
            // Cleanup on failure
            for (size_t j = 0; j < i; j++) {
                free(global_pool->buffers[j]);
            }
            free(global_pool->buffers);
            pthread_mutex_destroy(&global_pool->lock);
            free(global_pool);
            global_pool = NULL;
            return false;
        }
        global_pool->count++;
    }

    NSLOG(wisp, INFO, "Tile Pool Initialized: %zu buffers of %d KB", pool_size, TILE_BUFFER_SIZE / 1024);
    return true;
}

void tile_pool_fini(void) {
    if (!global_pool) return;

    pthread_mutex_lock(&global_pool->lock);
    for (size_t i = 0; i < global_pool->count; i++) {
        free(global_pool->buffers[i]);
    }
    free(global_pool->buffers);
    pthread_mutex_unlock(&global_pool->lock);
    pthread_mutex_destroy(&global_pool->lock);
    free(global_pool);
    global_pool = NULL;
}

void *tile_pool_checkout(void) {
    if (!global_pool) return NULL;

    void *buffer = NULL;
    pthread_mutex_lock(&global_pool->lock);
    if (global_pool->count > 0) {
        buffer = global_pool->buffers[--global_pool->count];
    }
    pthread_mutex_unlock(&global_pool->lock);

    if (!buffer) {
        // Fallback to heap if pool is empty
        buffer = malloc(TILE_BUFFER_SIZE);
        NSLOG(wisp, DEBUG, "Tile pool empty, allocated temporary buffer");
    }

    return buffer;
}

void tile_pool_return(void *buffer) {
    if (!global_pool || !buffer) {
        free(buffer);
        return;
    }

    pthread_mutex_lock(&global_pool->lock);
    if (global_pool->count < global_pool->capacity) {
        global_pool->buffers[global_pool->count++] = buffer;
        pthread_mutex_unlock(&global_pool->lock);
    } else {
        pthread_mutex_unlock(&global_pool->lock);
        free(buffer);
    }
}

size_t tile_pool_get_buffer_size(void) {
    return TILE_BUFFER_SIZE;
}
