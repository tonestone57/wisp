#include "arena.h"
#include <stdlib.h>
#include <string.h>

#define ALIGN_UP(val, align) (((val) + ((align) - 1)) & ~((align) - 1))

typedef struct arena_destructor {
    void (*fn)(void *);
    void *ptr;
    struct arena_destructor *next;
} arena_destructor;

typedef struct arena_chunk {
    struct arena_chunk *next;
    size_t size;
    size_t used;
    char data[];
} arena_chunk;

struct arena {
    arena_chunk *head;
    size_t default_chunk_size;
    arena_destructor *destructors;
};

struct arena *arena_create(size_t chunk_size) {
    if (chunk_size == 0) chunk_size = 64 * 1024;
    struct arena *a = malloc(sizeof(struct arena));
    if (!a) return NULL;
    a->head = NULL;
    a->default_chunk_size = chunk_size;
    a->destructors = NULL;
    return a;
}

void *arena_alloc(struct arena *a, size_t size) {
    if (!a) return NULL;
    size_t alloc_size = ALIGN_UP(size, 8);
    if (!a->head || a->head->used + alloc_size > a->head->size) {
        size_t chunk_alloc = alloc_size > a->default_chunk_size ? alloc_size : a->default_chunk_size;
        arena_chunk *chunk = malloc(sizeof(arena_chunk) + chunk_alloc);
        if (!chunk) return NULL;
        chunk->size = chunk_alloc;
        chunk->used = 0;
        chunk->next = a->head;
        a->head = chunk;
    }
    void *ptr = a->head->data + a->head->used;
    a->head->used += alloc_size;
    return ptr;
}

void arena_register_destructor(struct arena *a, void *ptr, void (*fn)(void *)) {
    if (!a || !fn) return;
    arena_destructor *d = arena_alloc(a, sizeof(arena_destructor));
    if (!d) return;
    d->ptr = ptr;
    d->fn = fn;
    d->next = a->destructors;
    a->destructors = d;
}

void arena_destroy(struct arena *a) {
    if (!a) return;

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
