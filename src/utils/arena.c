#ifndef _ISOC11_SOURCE
#define _ISOC11_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "arena.h"

static inline size_t arena_align_up(size_t val, size_t align)
{
    size_t res = val + align - 1;
    if (res < val) return SIZE_MAX & ~(align - 1);
    return res & ~(align - 1);
}

#define ALIGN_UP(val, align) arena_align_up(val, align)

typedef struct arena_destructor {
    void (*fn)(void *);
    void *ptr;
    struct arena_destructor *next;
} arena_destructor;

typedef struct arena_chunk {
    struct arena_chunk *next;
    size_t size;
    size_t used;
    char data[] __attribute__((aligned(64)));
} arena_chunk;

struct arena {
    arena_chunk *head;
    size_t default_chunk_size;
    arena_destructor *destructors;
    pthread_mutex_t lock;
};

struct arena *arena_create(size_t chunk_size) {
    if (chunk_size == 0) chunk_size = 64 * 1024;
    /* chunk_size must be 64-byte aligned for data alignment (AVX-512) */
    chunk_size = ALIGN_UP(chunk_size, 64);
    if (chunk_size == (SIZE_MAX & ~(size_t)63)) return NULL;

    struct arena *a = aligned_alloc(64, ALIGN_UP(sizeof(struct arena), 64));
    if (!a) return NULL;
    a->head = NULL;
    a->default_chunk_size = chunk_size;
    a->destructors = NULL;
    pthread_mutex_init(&a->lock, NULL);
    return a;
}

static void *arena_alloc_internal(struct arena *a, size_t size) {
    size_t req_size = (size == 0) ? 1 : size;
    size_t alloc_size = ALIGN_UP(req_size, 64);
    if (!a->head || ALIGN_UP(a->head->used, 64) + alloc_size > a->head->size) {
        size_t chunk_alloc = alloc_size > a->default_chunk_size ? alloc_size : a->default_chunk_size;
        arena_chunk *chunk = aligned_alloc(64, ALIGN_UP(sizeof(arena_chunk) + chunk_alloc, 64));
        if (!chunk) return NULL;
        chunk->size = chunk_alloc;
        chunk->used = 0;
        chunk->next = a->head;
        a->head = chunk;
    }
    size_t current_used = ALIGN_UP(a->head->used, 64);
    void *ptr = a->head->data + current_used;
    a->head->used = current_used + alloc_size;
    memset(ptr, 0, size);
    return ptr;
}

#ifndef _WIN32
__thread struct arena *wisp_worker_local_arena = NULL;
#else
__declspec(thread) struct arena *wisp_worker_local_arena = NULL;
#endif

void *arena_alloc(struct arena *a, size_t size) {
    if (wisp_worker_local_arena != NULL) {
        a = wisp_worker_local_arena;
    }
    if (!a || ((uintptr_t)a & 63) != 0) return NULL;

    pthread_mutex_lock(&a->lock);
    void *ptr = arena_alloc_internal(a, size);
    pthread_mutex_unlock(&a->lock);
    return ptr;
}

void arena_register_destructor(struct arena *a, void *ptr, void (*fn)(void *)) {
    if (wisp_worker_local_arena != NULL) {
        a = wisp_worker_local_arena;
    }
    if (!a || !fn) return;

    pthread_mutex_lock(&a->lock);
    arena_destructor *d = arena_alloc_internal(a, sizeof(arena_destructor));
    if (d) {
        d->ptr = ptr;
        d->fn = fn;
        d->next = a->destructors;
        a->destructors = d;
    }
    pthread_mutex_unlock(&a->lock);
}

void arena_destroy(struct arena *a) {
    if (!a || ((uintptr_t)a & 63) != 0) return;

    arena_destructor *d = a->destructors;
    while (d) {
        d->fn(d->ptr);
        d = d->next;
    }

    arena_chunk *chunk = a->head;
    while (chunk) {
        arena_chunk *next = chunk->next;
        free(chunk);
        chunk = next;
    }
    pthread_mutex_destroy(&a->lock);
    free(a);
}

char *arena_strdup(struct arena *a, const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *res = arena_alloc(a, len + 1);
    if (res) memcpy(res, s, len + 1);
    return res;
}

char *arena_strndup(struct arena *a, const char *s, size_t n) {
    if (!s) return NULL;
    size_t len = 0;
    while (len < n && s[len]) len++;
    char *res = arena_alloc(a, len + 1);
    if (res) {
        memcpy(res, s, len);
        res[len] = '\0';
    }
    return res;
}

void *arena_memdup(struct arena *a, const void *ptr, size_t size) {
    if (!ptr) return NULL;
    void *res = arena_alloc(a, size);
    if (res) memcpy(res, ptr, size);
    return res;
}

void arena_merge(struct arena *m, struct arena *w) {
    if (!m || !w) return;
    if (m == w) return;

    if (m < w) {
        pthread_mutex_lock(&m->lock);
        pthread_mutex_lock(&w->lock);
    } else {
        pthread_mutex_lock(&w->lock);
        pthread_mutex_lock(&m->lock);
    }

    /* Merge destructors */
    if (w->destructors != NULL) {
        /* Find the last destructor in w's list */
        arena_destructor *last_d = w->destructors;
        while (last_d->next != NULL) {
            last_d = last_d->next;
        }
        /* Point its next to m's destructors */
        last_d->next = m->destructors;
        m->destructors = w->destructors;
        w->destructors = NULL;
    }

    /* Merge chunks */
    if (w->head != NULL) {
        /* Find the last chunk in w's list */
        arena_chunk *last_c = w->head;
        while (last_c->next != NULL) {
            last_c = last_c->next;
        }
        /* Point its next to m's chunks */
        last_c->next = m->head;
        m->head = w->head;
        w->head = NULL;
    }

    pthread_mutex_unlock(&w->lock);
    pthread_mutex_unlock(&m->lock);
}
