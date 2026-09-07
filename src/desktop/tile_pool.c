#include "tile_pool.h"
#include <stdlib.h>
#include <string.h>
#include <wisp/utils/log.h>
#include "wisp/browser.h"
#include <lz4.h>

#ifdef _WIN32
#include <windows.h>
#define ns_mutex_t CRITICAL_SECTION
#define ns_mutex_init(m) InitializeCriticalSection(m)
#define ns_mutex_lock(m) EnterCriticalSection(m)
#define ns_mutex_unlock(m) LeaveCriticalSection(m)
#define ns_mutex_destroy(m) DeleteCriticalSection(m)
#else
#include <pthread.h>
#define ns_mutex_t pthread_mutex_t
#define ns_mutex_init(m) pthread_mutex_init(m, NULL)
#define ns_mutex_lock(m) pthread_mutex_lock(m)
#define ns_mutex_unlock(m) pthread_mutex_unlock(m)
#define ns_mutex_destroy(m) pthread_mutex_destroy(m)
#endif

#define MAX_CACHED_TILES 64
#define PRIORITY_COMPRESS_THRESHOLD 0.2f
#define PRIORITY_EVICT_THRESHOLD 0.01f

struct tile_cache_entry {
    void *owner;             // e.g. gui_window
    int doc_x;               // document-space x
    int doc_y;               // document-space y
    int tile_size;           // size of tile
    void *raw_buffer;        // 1MB raw buffer, or NULL if compressed
    void *compressed_buffer; // dynamic lz4 compressed buffer
    size_t compressed_size;  // size of compressed data
    float priority;          // last calculated priority
    uint64_t last_used;      // timestamp/monotonic counter for LRU
    bool active;             // true if entry is active
};

struct tile_pool {
    void **buffers;
    size_t capacity;
    size_t count;
    ns_mutex_t lock;
    struct tile_cache_entry cache[MAX_CACHED_TILES];
    uint64_t next_lru;       // counter to track LRU order
};

static struct tile_pool *global_pool = NULL;

static void return_buffer_locked(void *buffer) {
    if (!buffer) return;
    if (global_pool->count < global_pool->capacity) {
        global_pool->buffers[global_pool->count++] = buffer;
    } else {
        free(buffer);
    }
}

static void *checkout_buffer_locked(void) {
    void *buffer = NULL;
    if (global_pool->count > 0) {
        buffer = global_pool->buffers[--global_pool->count];
    }
    if (!buffer) {
        buffer = malloc(TILE_BUFFER_SIZE);
        if (buffer) {
            NSLOG(wisp, DEBUG, "Tile pool empty, allocated temporary buffer (internal)");
        }
    }
    return buffer;
}

static void *compress_tile(const void *src, size_t src_len, size_t *out_len) {
    int max_dst_size = LZ4_compressBound((int)src_len);
    void *dst = malloc(max_dst_size);
    if (!dst) return NULL;

    int compressed_size = LZ4_compress_default((const char *)src, (char *)dst, (int)src_len, max_dst_size);
    if (compressed_size <= 0) {
        free(dst);
        return NULL;
    }

    *out_len = (size_t)compressed_size;
    void *reallocated = realloc(dst, *out_len);
    if (reallocated) {
        return reallocated;
    }
    return dst;
}

static bool decompress_tile(const void *src, size_t src_len, void *dst, size_t dst_capacity) {
    int decompressed_size = LZ4_decompress_safe((const char *)src, (char *)dst, (int)src_len, (int)dst_capacity);
    return (decompressed_size == (int)dst_capacity);
}

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

    ns_mutex_init(&global_pool->lock);
    global_pool->capacity = pool_size;
    global_pool->count = 0;
    global_pool->next_lru = 0;

    for (size_t i = 0; i < pool_size; i++) {
        global_pool->buffers[i] = malloc(TILE_BUFFER_SIZE);
        if (!global_pool->buffers[i]) {
            // Cleanup on failure
            for (size_t j = 0; j < i; j++) {
                free(global_pool->buffers[j]);
            }
            free(global_pool->buffers);
            ns_mutex_destroy(&global_pool->lock);
            free(global_pool);
            global_pool = NULL;
            return false;
        }
        global_pool->count++;
    }

    // Initialize tile cache entries
    for (int i = 0; i < MAX_CACHED_TILES; i++) {
        global_pool->cache[i].active = false;
        global_pool->cache[i].raw_buffer = NULL;
        global_pool->cache[i].compressed_buffer = NULL;
    }

    NSLOG(wisp, INFO, "Tile Pool & Compressed Cache Initialized: %zu buffers, cache size %d", pool_size, MAX_CACHED_TILES);
    return true;
}

void tile_pool_fini(void) {
    if (!global_pool) return;

    ns_mutex_lock(&global_pool->lock);
    // Free lookaside list buffers
    for (size_t i = 0; i < global_pool->count; i++) {
        free(global_pool->buffers[i]);
    }
    free(global_pool->buffers);

    // Free all active cache entries
    for (int i = 0; i < MAX_CACHED_TILES; i++) {
        if (global_pool->cache[i].active) {
            if (global_pool->cache[i].raw_buffer) {
                free(global_pool->cache[i].raw_buffer);
            }
            if (global_pool->cache[i].compressed_buffer) {
                free(global_pool->cache[i].compressed_buffer);
            }
        }
    }

    ns_mutex_unlock(&global_pool->lock);
    ns_mutex_destroy(&global_pool->lock);
    free(global_pool);
    global_pool = NULL;
}

void *tile_pool_checkout(void) {
    if (!global_pool) return NULL;

    void *buffer = NULL;
    ns_mutex_lock(&global_pool->lock);
    if (global_pool->count > 0) {
        buffer = global_pool->buffers[--global_pool->count];
    }
    ns_mutex_unlock(&global_pool->lock);

    if (!buffer) {
        // Fallback to heap if pool is empty
        buffer = malloc(TILE_BUFFER_SIZE);
        if (buffer) {
            NSLOG(wisp, DEBUG, "Tile pool empty, allocated temporary buffer");
        } else {
            NSLOG(wisp, ERROR, "Failed to allocate fallback tile buffer (OOM)");
        }
    }

    return buffer;
}

void tile_pool_return(void *buffer) {
    if (!global_pool || !buffer) {
        free(buffer);
        return;
    }

    ns_mutex_lock(&global_pool->lock);
    if (global_pool->count < global_pool->capacity) {
        global_pool->buffers[global_pool->count++] = buffer;
        ns_mutex_unlock(&global_pool->lock);
    } else {
        ns_mutex_unlock(&global_pool->lock);
        free(buffer);
    }
}

size_t tile_pool_get_buffer_size(void) {
    return TILE_BUFFER_SIZE;
}

void *tile_pool_get_cached(void *owner, int doc_x, int doc_y, int tile_size, bool *out_from_cache) {
    *out_from_cache = false;
    if (!global_pool) return NULL;

    ns_mutex_lock(&global_pool->lock);
    for (int i = 0; i < MAX_CACHED_TILES; i++) {
        struct tile_cache_entry *entry = &global_pool->cache[i];
        if (entry->active && entry->owner == owner && entry->doc_x == doc_x && entry->doc_y == doc_y && entry->tile_size == tile_size) {
            // Cache Hit!
            entry->last_used = ++global_pool->next_lru;
            *out_from_cache = true;

            if (entry->raw_buffer) {
                // Return existing uncompressed raw buffer
                ns_mutex_unlock(&global_pool->lock);
                return entry->raw_buffer;
            } else if (entry->compressed_buffer) {
                // Decompress on demand
                void *raw = checkout_buffer_locked();
                if (raw) {
                    if (decompress_tile(entry->compressed_buffer, entry->compressed_size, raw, TILE_BUFFER_SIZE)) {
                        free(entry->compressed_buffer);
                        entry->compressed_buffer = NULL;
                        entry->compressed_size = 0;
                        entry->raw_buffer = raw;
                        NSLOG(wisp, DEBUG, "Decompressed tile (%d,%d) instantaneously", doc_x, doc_y);
                        ns_mutex_unlock(&global_pool->lock);
                        return raw;
                    } else {
                        return_buffer_locked(raw);
                        *out_from_cache = false;
                        NSLOG(wisp, ERROR, "Failed to decompress tile (%d,%d)", doc_x, doc_y);
                    }
                } else {
                    *out_from_cache = false;
                }
            }
            break;
        }
    }
    ns_mutex_unlock(&global_pool->lock);
    return NULL;
}

void tile_pool_put_cached(void *owner, int doc_x, int doc_y, int tile_size, void *buffer, float priority) {
    if (!global_pool || !buffer) return;

    ns_mutex_lock(&global_pool->lock);

    int existing_idx = -1;
    int free_idx = -1;
    int lru_idx = -1;
    uint64_t min_lru = ~(uint64_t)0;

    for (int i = 0; i < MAX_CACHED_TILES; i++) {
        struct tile_cache_entry *entry = &global_pool->cache[i];
        if (entry->active) {
            if (entry->owner == owner && entry->doc_x == doc_x && entry->doc_y == doc_y && entry->tile_size == tile_size) {
                existing_idx = i;
                break;
            }
            if (entry->last_used < min_lru) {
                min_lru = entry->last_used;
                lru_idx = i;
            }
        } else {
            if (free_idx == -1) {
                free_idx = i;
            }
        }
    }

    if (existing_idx != -1) {
        // Update existing entry
        struct tile_cache_entry *entry = &global_pool->cache[existing_idx];
        if (entry->raw_buffer && entry->raw_buffer != buffer) {
            return_buffer_locked(entry->raw_buffer);
        }
        if (entry->compressed_buffer) {
            free(entry->compressed_buffer);
            entry->compressed_buffer = NULL;
            entry->compressed_size = 0;
        }
        entry->raw_buffer = buffer;
        entry->priority = priority;
        entry->last_used = ++global_pool->next_lru;
    } else {
        // Insert new entry
        int target_idx = -1;
        if (free_idx != -1) {
            target_idx = free_idx;
        } else if (lru_idx != -1) {
            // Evict LRU
            struct tile_cache_entry *evicted = &global_pool->cache[lru_idx];
            if (evicted->raw_buffer) {
                return_buffer_locked(evicted->raw_buffer);
            }
            if (evicted->compressed_buffer) {
                free(evicted->compressed_buffer);
            }
            evicted->active = false;
            target_idx = lru_idx;
        }

        if (target_idx != -1) {
            struct tile_cache_entry *entry = &global_pool->cache[target_idx];
            entry->owner = owner;
            entry->doc_x = doc_x;
            entry->doc_y = doc_y;
            entry->tile_size = tile_size;
            entry->raw_buffer = buffer;
            entry->compressed_buffer = NULL;
            entry->compressed_size = 0;
            entry->priority = priority;
            entry->last_used = ++global_pool->next_lru;
            entry->active = true;
        }
    }

    ns_mutex_unlock(&global_pool->lock);
}

void tile_pool_manage_cache(void *owner, int viewport_x, int viewport_y, int viewport_width, int viewport_height) {
    if (!global_pool) return;

    ns_mutex_lock(&global_pool->lock);

    for (int i = 0; i < MAX_CACHED_TILES; i++) {
        struct tile_cache_entry *entry = &global_pool->cache[i];
        if (entry->active && entry->owner == owner) {
            // Recalculate priority based on distance to new viewport
            float priority = browser_calculate_tile_priority(entry->doc_x, entry->doc_y, viewport_x, viewport_y, viewport_width, viewport_height);
            entry->priority = priority;

            if (priority < PRIORITY_EVICT_THRESHOLD) {
                // Scrolled extremely far out of the active frustum -> evict entirely
                if (entry->raw_buffer) {
                    return_buffer_locked(entry->raw_buffer);
                }
                if (entry->compressed_buffer) {
                    free(entry->compressed_buffer);
                }
                entry->active = false;
                NSLOG(wisp, DEBUG, "Evicted extremely distant tile at (%d,%d) priority %f", entry->doc_x, entry->doc_y, priority);
            } else if (priority < PRIORITY_COMPRESS_THRESHOLD) {
                // Scrolled significantly out of the active frustum -> compress using LZ4
                if (entry->raw_buffer && !entry->compressed_buffer) {
                    size_t comp_len = 0;
                    void *comp_buf = compress_tile(entry->raw_buffer, TILE_BUFFER_SIZE, &comp_len);
                    if (comp_buf) {
                        return_buffer_locked(entry->raw_buffer);
                        entry->raw_buffer = NULL;
                        entry->compressed_buffer = comp_buf;
                        entry->compressed_size = comp_len;
                        NSLOG(wisp, DEBUG, "Compressed non-visible tile (%d,%d): 1MB -> %zu KB (ratio: %.1f:1)",
                              entry->doc_x, entry->doc_y, comp_len / 1024, (double)TILE_BUFFER_SIZE / comp_len);
                    }
                }
            }
        }
    }

    ns_mutex_unlock(&global_pool->lock);
}
