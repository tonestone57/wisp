#ifndef WISP_UTILS_ARENA_H
#define WISP_UTILS_ARENA_H

#include <stddef.h>

struct arena;

struct arena *arena_create(size_t chunk_size);
void *arena_alloc(struct arena *a, size_t size);
void arena_register_destructor(struct arena *a, void *ptr, void (*fn)(void *));
void arena_destroy(struct arena *a);

char *arena_strdup(struct arena *a, const char *s);
char *arena_strndup(struct arena *a, const char *s, size_t n);
void *arena_memdup(struct arena *a, const void *ptr, size_t size);

#endif
