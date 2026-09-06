#include "wisp/utils/shm_dom.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "wisp/utils/log.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

__thread char wisp_sso_decode_bufs[16][4];
__thread uint32_t wisp_sso_decode_idx = 0;

int peak_nodes_used = 0;
static bool shm_dom_metrics_registered = false;

static void shm_dom_log_final_peak(void) {
    NSLOG(wisp, INFO, "=========================================");
    NSLOG(wisp, INFO, "[SHM_DOM] BROWSER EXIT METRICS");
    NSLOG(wisp, INFO, "[SHM_DOM] Peak shared-memory nodes used: %d",
          peak_nodes_used);
    NSLOG(wisp, INFO, "=========================================");
}

#include <unistd.h>

void shm_dom_lock_read(shm_dom_t *shm) {
    if (!shm) return;
    while (1) {
        uint32_t val = __atomic_load_n(&shm->lock, __ATOMIC_RELAXED);
        /* Block new readers if write-locked (bit 31) or if any writer is waiting (bits 16..30) */
        if ((val & 0x80000000) == 0 && (val & 0x7FFF0000) == 0) {
            if (__atomic_compare_exchange_n(&shm->lock, &val, val + 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
                break;
            }
            continue;
        }
#ifdef _WIN32
        YieldProcessor();
#elif defined(__i386__) || defined(__x86_64__)
        __builtin_ia32_pause();
#elif defined(__arm__) || defined(__aarch64__)
        __asm__ __volatile__("yield" ::: "memory");
#else
        usleep(0);
#endif
    }
}

void shm_dom_unlock_read(shm_dom_t *shm) {
    if (!shm) return;
    __atomic_fetch_sub(&shm->lock, 1, __ATOMIC_RELEASE);
}

void shm_dom_lock_write(shm_dom_t *shm) {
    if (!shm) return;
    /* Register as pending writer (increment bits 16..30) */
    __atomic_fetch_add(&shm->lock, 0x00010000, __ATOMIC_ACQUIRE);

    while (1) {
        uint32_t val = __atomic_load_n(&shm->lock, __ATOMIC_RELAXED);
        /* Can acquire write lock if not write-locked (bit 31) and no active readers (bits 0..15) */
        if ((val & 0x8000FFFF) == 0) {
            uint32_t desired = (val - 0x00010000) | 0x80000000;
            if (__atomic_compare_exchange_n(&shm->lock, &val, desired, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
                break;
            }
            continue;
        }
#ifdef _WIN32
        YieldProcessor();
#elif defined(__i386__) || defined(__x86_64__)
        __builtin_ia32_pause();
#elif defined(__arm__) || defined(__aarch64__)
        __asm__ __volatile__("yield" ::: "memory");
#else
        usleep(0);
#endif
    }
}

void shm_dom_unlock_write(shm_dom_t *shm) {
    if (!shm) return;
    __atomic_fetch_and(&shm->lock, ~0x80000000, __ATOMIC_RELEASE);
}

size_t shm_dom_size(uint32_t capacity) {
    return sizeof(shm_dom_t) + capacity * (sizeof(WispCompactNode) + sizeof(WispShmLayoutCache) + sizeof(WispNodeStrings) + sizeof(uint64_t));
}

shm_dom_t* shm_dom_remap(shm_dom_t *old_shm, uint32_t old_capacity, uint32_t new_capacity) {
    if (!old_shm) return NULL;
    uint32_t old_cap = old_capacity;
    size_t old_size = shm_dom_size(old_cap);

    if (new_capacity > 10000000) {
        NSLOG(wisp, ERROR, "[SHM_DOM] shm_dom_remap: capacity %u exceeds safety limit", new_capacity);
#ifdef _WIN32
        UnmapViewOfFile(old_shm);
        HANDLE h = find_and_unregister_shm_handle(old_shm);
        if (h) CloseHandle(h);
#else
        munmap(old_shm, old_size);
#endif
        return NULL;
    }

    char name[64];
    strncpy(name, old_shm->shm_name, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
    bool is_server = old_shm->is_server;

    // Check if another process has already remapped the shared physical file and reorganized its layouts.
    // If so, we can skip allocating temporary buffers and copying/reorganizing elements,
    // and just perform the local unmap of the old capacity and mmap of the new capacity.
    bool already_remapped = (old_shm->node_capacity >= new_capacity);

    size_t new_size = shm_dom_size(new_capacity);

    WispCompactNode *temp_nodes = NULL;
    WispShmLayoutCache *temp_layout_cache = NULL;
    WispNodeStrings *temp_node_strings = NULL;
    uint64_t *temp_dom_ptrs = NULL;

    if (!already_remapped) {
        // 1. Allocate temporary buffers on the local heap to copy the old arrays
        temp_nodes = malloc(old_cap * sizeof(WispCompactNode));
        temp_layout_cache = malloc(old_cap * sizeof(WispShmLayoutCache));
        temp_node_strings = malloc(old_cap * sizeof(WispNodeStrings));
        temp_dom_ptrs = malloc(old_cap * sizeof(uint64_t));

        if (!temp_nodes || !temp_layout_cache || !temp_node_strings || !temp_dom_ptrs) {
            NSLOG(wisp, ERROR, "[SHM_DOM] shm_dom_remap: temporary buffer allocation failed!");
            free(temp_nodes);
            free(temp_layout_cache);
            free(temp_node_strings);
            free(temp_dom_ptrs);
            return NULL;
        }

        // 2. Copy the old elements from their old offsets before unmapping safely using old_cap
        WispCompactNode *old_nodes = (WispCompactNode*)((char*)old_shm + sizeof(shm_dom_t));
        WispShmLayoutCache *old_layout_cache = (WispShmLayoutCache*)((char*)old_nodes + old_cap * sizeof(WispCompactNode));
        WispNodeStrings *old_node_strings = (WispNodeStrings*)((char*)old_layout_cache + old_cap * sizeof(WispShmLayoutCache));
        uint64_t *old_dom_ptrs = (uint64_t*)((char*)old_node_strings + old_cap * sizeof(WispNodeStrings));

        memcpy(temp_nodes, old_nodes, old_cap * sizeof(WispCompactNode));
        memcpy(temp_layout_cache, old_layout_cache, old_cap * sizeof(WispShmLayoutCache));
        memcpy(temp_node_strings, old_node_strings, old_cap * sizeof(WispNodeStrings));
        memcpy(temp_dom_ptrs, old_dom_ptrs, old_cap * sizeof(uint64_t));
    }

    shm_dom_t *new_shm = NULL;

#ifdef _WIN32
    UnmapViewOfFile(old_shm);
    HANDLE h = find_and_unregister_shm_handle(old_shm);
    if (h) CloseHandle(h);

    HANDLE hMap = NULL;
    if (is_server) {
        SECURITY_ATTRIBUTES sa;
        SECURITY_DESCRIPTOR sd;
        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&sd, TRUE, (PACL)NULL, FALSE);
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = FALSE;

        hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, (DWORD)new_size, name);
    } else {
        hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name);
    }

    if (!hMap) {
        if (!already_remapped) {
            free(temp_nodes);
            free(temp_layout_cache);
            free(temp_node_strings);
            free(temp_dom_ptrs);
        }
        return NULL;
    }

    new_shm = (shm_dom_t *)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, new_size);
    if (new_shm) {
        register_shm_handle(new_shm, hMap);
    } else {
        CloseHandle(hMap);
        if (!already_remapped) {
            free(temp_nodes);
            free(temp_layout_cache);
            free(temp_node_strings);
            free(temp_dom_ptrs);
        }
        return NULL;
    }
#else
    int fd = shm_open(name, O_RDWR, 0666);
    if (fd < 0) {
        NSLOG(wisp, ERROR, "[SHM_DOM] shm_dom_remap: failed to shm_open %s", name);
        munmap(old_shm, old_size);
        if (!already_remapped) {
            free(temp_nodes);
            free(temp_layout_cache);
            free(temp_node_strings);
            free(temp_dom_ptrs);
        }
        return NULL;
    }

    if (is_server || new_capacity > old_cap) {
        if (ftruncate(fd, new_size) != 0) {
            NSLOG(wisp, ERROR, "[SHM_DOM] shm_dom_remap: ftruncate failed");
            close(fd);
            munmap(old_shm, old_size);
            if (!already_remapped) {
                free(temp_nodes);
                free(temp_layout_cache);
                free(temp_node_strings);
                free(temp_dom_ptrs);
            }
            return NULL;
        }
    }

    munmap(old_shm, old_size);

    new_shm = (shm_dom_t *)mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if (new_shm == MAP_FAILED) {
        NSLOG(wisp, ERROR, "[SHM_DOM] shm_dom_remap: mmap failed");
        if (!already_remapped) {
            free(temp_nodes);
            free(temp_layout_cache);
            free(temp_node_strings);
            free(temp_dom_ptrs);
        }
        return NULL;
    }
#endif

    if (!already_remapped) {
        // 3. Set the new capacity and copy the arrays back to their new offsets in new_shm
        new_shm->node_capacity = new_capacity;

        // Zero out the entire new array space to be absolutely safe
        memset(shm_dom_get_nodes(new_shm), 0, new_capacity * sizeof(WispCompactNode));
        memset(shm_dom_get_layout_cache(new_shm), 0, new_capacity * sizeof(WispShmLayoutCache));
        memset(shm_dom_get_node_strings(new_shm), 0, new_capacity * sizeof(WispNodeStrings));
        memset(shm_dom_get_dom_ptrs(new_shm), 0, new_capacity * sizeof(uint64_t));

        // Copy the old elements back to the beginning of the new arrays
        memcpy(shm_dom_get_nodes(new_shm), temp_nodes, old_cap * sizeof(WispCompactNode));
        memcpy(shm_dom_get_layout_cache(new_shm), temp_layout_cache, old_cap * sizeof(WispShmLayoutCache));
        memcpy(shm_dom_get_node_strings(new_shm), temp_node_strings, old_cap * sizeof(WispNodeStrings));
        memcpy(shm_dom_get_dom_ptrs(new_shm), temp_dom_ptrs, old_cap * sizeof(uint64_t));

        // 4. Free the temporary heap buffers
        free(temp_nodes);
        free(temp_layout_cache);
        free(temp_node_strings);
        free(temp_dom_ptrs);
    }

    return new_shm;
}

#ifdef _WIN32
typedef struct {
    void *ptr;
    HANDLE handle;
} shm_handle_entry_t;

static shm_handle_entry_t shm_handles[128];
static int shm_handles_count = 0;

static void register_shm_handle(void *ptr, HANDLE handle) {
    if (shm_handles_count < 128) {
        shm_handles[shm_handles_count++] = (shm_handle_entry_t){ ptr, handle };
    }
}

static HANDLE find_and_unregister_shm_handle(void *ptr) {
    for (int i = 0; i < shm_handles_count; i++) {
        if (shm_handles[i].ptr == ptr) {
            HANDLE h = shm_handles[i].handle;
            shm_handles[i] = shm_handles[--shm_handles_count];
            return h;
        }
    }
    return NULL;
}
#endif

shm_dom_t* shm_dom_create(const char *name, uint32_t capacity, bool is_server) {
    if (is_server && !shm_dom_metrics_registered) {
        atexit(shm_dom_log_final_peak);
        shm_dom_metrics_registered = true;
    }

    uint32_t use_cap = capacity;
    if (is_server && use_cap == 0) {
        use_cap = SHM_DOM_MAX_NODES;
    }

#ifdef _WIN32
    HANDLE hMap = NULL;
    size_t size = shm_dom_size(is_server ? use_cap : SHM_DOM_MAX_NODES);
    if (is_server) {
        SECURITY_ATTRIBUTES sa;
        SECURITY_DESCRIPTOR sd;
        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&sd, TRUE, (PACL)NULL, FALSE);
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = FALSE;

        hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, (DWORD)size, name);
    } else {
        hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name);
    }

    if (!hMap) return NULL;

    shm_dom_t *shm = NULL;
    if (is_server) {
        shm = (shm_dom_t *)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, size);
        if (shm) {
            memset(shm, 0, size);
            shm->node_capacity = use_cap;
            shm->is_server = true;
            shm->string_heap_top = 1;
            strncpy(shm->shm_name, name, sizeof(shm->shm_name) - 1);
            shm->shm_name[sizeof(shm->shm_name) - 1] = '\0';
            register_shm_handle(shm, hMap);
        } else {
            CloseHandle(hMap);
        }
    } else {
        shm_dom_t *temp = (shm_dom_t *)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(shm_dom_t));
        if (temp) {
            uint32_t cap = temp->node_capacity;
            if (cap == 0) {
                cap = (capacity != 0) ? capacity : SHM_DOM_MAX_NODES;
            }
            size = shm_dom_size(cap);
            UnmapViewOfFile(temp);
            shm = (shm_dom_t *)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, size);
            if (shm) {
                register_shm_handle(shm, hMap);
            } else {
                CloseHandle(hMap);
            }
        } else {
            CloseHandle(hMap);
        }
    }
    return shm;
#else
    int fd = -1;
    size_t size = shm_dom_size(is_server ? use_cap : SHM_DOM_MAX_NODES);
    if (is_server) {
        fd = shm_open(name, O_CREAT | O_RDWR, 0666);
        if (fd >= 0) {
            if (ftruncate(fd, size) != 0) {
                close(fd);
                shm_unlink(name);
                return NULL;
            }
        }
    } else {
        fd = shm_open(name, O_RDWR, 0666);
    }

    if (fd < 0) return NULL;

    shm_dom_t *shm = NULL;
    if (is_server) {
        shm = (shm_dom_t *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);
        if (shm == MAP_FAILED) {
            shm_unlink(name);
            return NULL;
        }
        memset(shm, 0, size);
        shm->node_capacity = use_cap;
        shm->is_server = true;
        shm->string_heap_top = 1;
        strncpy(shm->shm_name, name, sizeof(shm->shm_name) - 1);
        shm->shm_name[sizeof(shm->shm_name) - 1] = '\0';
    } else {
        shm_dom_t *temp = (shm_dom_t *)mmap(NULL, sizeof(shm_dom_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (temp == MAP_FAILED) {
            close(fd);
            return NULL;
        }
        uint32_t cap = temp->node_capacity;
        if (cap == 0) {
            cap = (capacity != 0) ? capacity : SHM_DOM_MAX_NODES;
        }
        size = shm_dom_size(cap);
        munmap(temp, sizeof(shm_dom_t));

        shm = (shm_dom_t *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);
        if (shm == MAP_FAILED) {
            return NULL;
        }
    }
    return shm;
#endif
}

shm_mutation_chunk_t* shm_mutation_chunk_create(const char *name, bool is_server) {
    size_t size = sizeof(shm_mutation_chunk_t);
#ifdef _WIN32
    HANDLE hMap = NULL;
    if (is_server) {
        SECURITY_ATTRIBUTES sa;
        SECURITY_DESCRIPTOR sd;
        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&sd, TRUE, (PACL)NULL, FALSE);
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = FALSE;
        hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, (DWORD)size, name);
    } else {
        hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name);
    }

    if (!hMap) return NULL;
    shm_mutation_chunk_t *chunk = (shm_mutation_chunk_t *)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (chunk) {
        register_shm_handle(chunk, hMap);
        if (is_server) {
            memset(chunk, 0, size);
        }
        chunk->capacity = SHM_MUTATION_CHUNK_CAPACITY;
    } else {
        CloseHandle(hMap);
    }
    return chunk;
#else
    int fd = -1;
    if (is_server) {
        fd = shm_open(name, O_CREAT | O_RDWR, 0666);
        if (fd >= 0) {
            if (ftruncate(fd, size) != 0) {
                close(fd);
                shm_unlink(name);
                return NULL;
            }
        }
    } else {
        fd = shm_open(name, O_RDWR, 0666);
    }

    if (fd < 0) return NULL;

    shm_mutation_chunk_t *chunk = (shm_mutation_chunk_t *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if (chunk == MAP_FAILED) {
        if (is_server) shm_unlink(name);
        return NULL;
    }

    if (is_server) {
        memset(chunk, 0, size);
    }
    chunk->capacity = SHM_MUTATION_CHUNK_CAPACITY;

    return chunk;
#endif
}

void shm_mutation_chunk_destroy(shm_mutation_chunk_t *chunk, const char *name, bool is_server) {
    if (!chunk) return;
    size_t size = sizeof(shm_mutation_chunk_t);
#ifdef _WIN32
    UnmapViewOfFile(chunk);
    HANDLE h = find_and_unregister_shm_handle(chunk);
    if (h) CloseHandle(h);
#else
    munmap(chunk, size);
    if (is_server && name) {
        shm_unlink(name);
    }
#endif
}

void shm_dom_destroy(shm_dom_t *shm, const char *name, bool is_server) {
    if (!shm) return;
    uint32_t cap = shm->node_capacity;
    if (cap == 0) cap = SHM_DOM_MAX_NODES;
    size_t size = shm_dom_size(cap);
#ifdef _WIN32
    UnmapViewOfFile(shm);
    HANDLE h = find_and_unregister_shm_handle(shm);
    if (h) CloseHandle(h);
#else
    munmap(shm, size);
    if (is_server && name) {
        shm_unlink(name);
    }
#endif
}

#define BBMQ_INITIAL_CAPACITY 256

extern bool wisp_is_js_process;
extern shm_dom_t *wisp_shm_dom;

static shm_mutation_t *bbmq_buffer = NULL;
static uint32_t bbmq_head = 0;
static uint32_t bbmq_tail = 0;
static uint32_t bbmq_size = 0;
static uint32_t bbmq_capacity = 0;

static shm_mutation_chunk_t *producer_sec_chunks[SHM_MAX_SECONDARY_CHUNKS] = {NULL};
static char producer_sec_names[SHM_MAX_SECONDARY_CHUNKS][64] = {{0}};

static void bbmq_cleanup_producer_chunks(void) {
    for (int i = 0; i < SHM_MAX_SECONDARY_CHUNKS; i++) {
        if (producer_sec_chunks[i]) {
            shm_mutation_chunk_destroy(producer_sec_chunks[i], producer_sec_names[i], true);
            producer_sec_chunks[i] = NULL;
            producer_sec_names[i][0] = '\0';
        }
    }
}

static void bbmq_cleanup(void) {
    if (bbmq_buffer) {
        free(bbmq_buffer);
        bbmq_buffer = NULL;
    }
    bbmq_cleanup_producer_chunks();
    bbmq_head = 0;
    bbmq_tail = 0;
    bbmq_size = 0;
    bbmq_capacity = 0;
}

static void bbmq_init(void) {
    bbmq_capacity = BBMQ_INITIAL_CAPACITY;
    bbmq_buffer = malloc(bbmq_capacity * sizeof(shm_mutation_t));
    if (!bbmq_buffer) {
        NSLOG(wisp, ERROR, "[BBMQ] Failed to allocate initial BBMQ buffer!");
        exit(1);
    }
    bbmq_head = 0;
    bbmq_tail = 0;
    bbmq_size = 0;
    atexit(bbmq_cleanup);
}

static void bbmq_resize(uint32_t new_capacity) {
    shm_mutation_t *new_buffer = malloc(new_capacity * sizeof(shm_mutation_t));
    if (!new_buffer) {
        NSLOG(wisp, ERROR, "[BBMQ] Failed to resize BBMQ buffer!");
        return;
    }

    for (uint32_t i = 0; i < bbmq_size; i++) {
        uint32_t idx = (bbmq_head + i) % bbmq_capacity;
        new_buffer[i] = bbmq_buffer[idx];
    }

    free(bbmq_buffer);
    bbmq_buffer = new_buffer;
    bbmq_head = 0;
    bbmq_tail = bbmq_size;
    bbmq_capacity = new_capacity;
}

WispStringRef wisp_shm_alloc_string(shm_dom_t *shm, const char *str) {
    if (!shm || !str) return 0;
    size_t len = strlen(str);
    if (len <= 3) {
        return wisp_make_string_ref(str, len, 0);
    }

    // String Interning Hash Lookup (FNV-1a)
    uint32_t hash = 2166136261U;
    for (size_t i = 0; i < len; i++) {
        hash = (hash ^ (uint8_t)str[i]) * 16777619ULL;
    }
    uint32_t bucket = hash % SHM_STRING_HASH_SIZE;

    // Open addressing / linear probing
    for (uint32_t i = 0; i < SHM_STRING_HASH_SIZE; i++) {
        uint32_t slot = (bucket + i) % SHM_STRING_HASH_SIZE;
        uint32_t offset = __atomic_load_n(&shm->string_hash_table[slot], __ATOMIC_ACQUIRE);

        while (offset == 0xFFFFFFFF) {
#ifdef _WIN32
            YieldProcessor();
#elif defined(__i386__) || defined(__x86_64__)
            __builtin_ia32_pause();
#elif defined(__arm__) || defined(__aarch64__)
            __asm__ __volatile__("yield" ::: "memory");
#else
            usleep(0);
#endif
            offset = __atomic_load_n(&shm->string_hash_table[slot], __ATOMIC_ACQUIRE);
        }

        if (offset == 0) {
            uint32_t expected = 0;
            if (__atomic_compare_exchange_n(&shm->string_hash_table[slot], &expected, 0xFFFFFFFF, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
                // Allocate new on shared heap top (Atomic Bump Allocator)
                uint32_t bytes_needed = len + 1;
                uint32_t cur_top = __atomic_fetch_add(&shm->string_heap_top, bytes_needed, __ATOMIC_SEQ_CST);
                if (cur_top + bytes_needed > SHM_STRING_HEAP_SIZE) {
                    NSLOG(wisp, ERROR, "[SHM_DOM] Shared string heap out of memory!");
                    __atomic_store_n(&shm->string_hash_table[slot], 0, __ATOMIC_RELEASE);
                    return 0;
                }
                memcpy(&shm->string_heap[cur_top], str, len);
                shm->string_heap[cur_top + len] = '\0';
                uint32_t stored_offset = cur_top + 1; // 1-based offset to distinguish from empty
                __atomic_store_n(&shm->string_hash_table[slot], stored_offset, __ATOMIC_RELEASE);
                return wisp_make_string_ref(str, len, cur_top);
            } else {
                // CAS failed, slot was modified by another thread/process.
                // Retry this slot iteration to evaluate the updated offset.
                i--;
                continue;
            }
        } else {
            uint32_t heap_offset = offset - 1;
            if (heap_offset < SHM_STRING_HEAP_SIZE && strcmp(&shm->string_heap[heap_offset], str) == 0) {
                return wisp_make_string_ref(str, len, heap_offset);
            }
        }
    }
    return 0;
}

void shm_mutation_enqueue(shm_dom_t *shm, uint32_t type, uint64_t target_id, uint64_t param1_id, uint64_t param2_id, const char *name, const char *value) {
    if (!shm) return;
    if (wisp_is_js_process && !wisp_shm_dom) return;

    if (wisp_is_js_process) {
        extern uint32_t wisp_shm_capacity;
        if (wisp_shm_dom && wisp_shm_capacity < wisp_shm_dom->node_capacity) {
            uint32_t new_cap = wisp_shm_dom->node_capacity;
            wisp_shm_dom = shm_dom_remap(wisp_shm_dom, wisp_shm_capacity, new_cap);
            if (wisp_shm_dom) {
                wisp_shm_capacity = new_cap;
            } else {
                wisp_shm_capacity = 0;
            }
        }
        shm = wisp_shm_dom;
        if (shm) {
            shm_dom_lock_write(shm);
        }
    } else {
        shm_dom_lock_write(shm);
    }

    if (!shm) {
        if (wisp_is_js_process && wisp_shm_dom) {
            shm_dom_unlock_write(wisp_shm_dom);
        }
        return;
    }

    shm->layout_dirty = true;
    WispCompactNode *sn = find_shm_node(shm, target_id);
    if (sn) {
        sn->layout_dirty = 1;
        if (sn->layout_index != 0) {
            shm_dom_get_layout_cache(shm)[sn->layout_index].layout_dirty = 1;
        }
    }

    WispStringRef name_ref = wisp_shm_alloc_string(shm, name);
    WispStringRef value_ref = wisp_shm_alloc_string(shm, value);

    if (sn) {
        WispNodeStrings *sns = &shm_dom_get_node_strings(shm)[target_id];
        if (type == SHM_MUTATION_SET_ATTRIBUTE && name) {
            bool found = false;
            uint32_t limit = sns->attr_count < WISP_SHM_MAX_ATTRIBUTES ? sns->attr_count : WISP_SHM_MAX_ATTRIBUTES;
            for (uint32_t i = 0; i < limit; i++) {
                if (wisp_string_ref_caseeq(shm, sns->attrs[i].name, name)) {
                    sns->attrs[i].value = value_ref;
                    found = true;
                    break;
                }
            }
            if (!found && sns->attr_count < WISP_SHM_MAX_ATTRIBUTES) {
                uint32_t idx = sns->attr_count++;
                sns->attrs[idx].name = name_ref;
                sns->attrs[idx].value = value_ref;
            }

            // Keep class hash up-to-date
            if (strcmp(name, "class") == 0) {
                uint32_t hash = 5381;
                if (value) {
                    const char *val_ptr = value;
                    int c;
                    while ((c = (unsigned char)*val_ptr++)) {
                        hash = ((hash << 5) + hash) + c;
                    }
                }
                sn->class_hash = hash;
            }
        } else if (type == SHM_MUTATION_REMOVE_ATTRIBUTE && name) {
            uint32_t limit = sns->attr_count < WISP_SHM_MAX_ATTRIBUTES ? sns->attr_count : WISP_SHM_MAX_ATTRIBUTES;
            for (uint32_t i = 0; i < limit; i++) {
                if (wisp_string_ref_caseeq(shm, sns->attrs[i].name, name)) {
                    sns->attrs[i] = sns->attrs[--sns->attr_count];
                    break;
                }
            }
        } else if (type == SHM_MUTATION_SET_NODE_VALUE || type == SHM_MUTATION_SET_TEXT_CONTENT) {
            sns->value = value_ref;
        }
    }

    if (wisp_is_js_process) {
        if (!bbmq_buffer) {
            bbmq_init();
        }
        if (bbmq_size == bbmq_capacity) {
            bbmq_resize(bbmq_capacity * 2);
        }

        shm_mutation_t *m = &bbmq_buffer[bbmq_tail];
        m->type = type;
        m->target_id = target_id;
        m->param1_id = param1_id;
        m->param2_id = param2_id;
        m->name = name_ref;
        m->value = value_ref;
        bbmq_tail = (bbmq_tail + 1) % bbmq_capacity;
        bbmq_size++;
        shm_dom_unlock_write(wisp_shm_dom);
        return;
    }

    shm_mutation_queue_t *mq = &shm->mutation_queue;
    uint32_t head = mq->head;
    uint32_t tail = mq->tail;
    if (head - tail >= SHM_MUTATION_QUEUE_SIZE) {
        shm_dom_unlock_write(shm);
        return;
    }
    uint32_t idx = head % SHM_MUTATION_QUEUE_SIZE;
    shm_mutation_t *m = &mq->queue[idx];
    m->type = type;
    m->target_id = target_id;
    m->param1_id = param1_id;
    m->param2_id = param2_id;
    m->name = name_ref;
    m->value = value_ref;

#ifdef _WIN32
    MemoryBarrier();
#else
    __sync_synchronize();
#endif
    mq->head = head + 1;
    shm_dom_unlock_write(shm);
}

bool bbmq_has_pending_for_node(uint64_t target_id) {
    if (!bbmq_buffer) return false;
    for (uint32_t i = 0; i < bbmq_size; i++) {
        uint32_t idx = (bbmq_head + i) % bbmq_capacity;
        if (bbmq_buffer[idx].target_id == target_id) {
            return true;
        }
    }
    return false;
}

static uint64_t bbmq_chunk_seq = 0;

void bbmq_flush(void) {
    if (!wisp_shm_dom) return;
    if (bbmq_size == 0) return;

    shm_mutation_queue_t *mq = &wisp_shm_dom->mutation_queue;
    uint32_t head = __atomic_load_n(&mq->head, __ATOMIC_ACQUIRE);
    uint32_t tail = __atomic_load_n(&mq->tail, __ATOMIC_ACQUIRE);

    uint32_t sec_count = __atomic_load_n(&mq->secondary_chunk_count, __ATOMIC_ACQUIRE);
    if (sec_count == 0) {
        bbmq_cleanup_producer_chunks();
    }

    uint32_t processed = 0;
    while (processed < bbmq_size) {
        sec_count = __atomic_load_n(&mq->secondary_chunk_count, __ATOMIC_ACQUIRE);
        if (sec_count == 0 && (head - tail < SHM_MUTATION_QUEUE_SIZE)) {
            uint32_t src_idx = (bbmq_head + processed) % bbmq_capacity;
            uint32_t idx = head % SHM_MUTATION_QUEUE_SIZE;
            mq->queue[idx] = bbmq_buffer[src_idx];
            head++;
            processed++;
        } else {
            /* Primary 64KB page is full. Create or select dynamic secondary SHM page chunk */
            sec_count = __atomic_load_n(&mq->secondary_chunk_count, __ATOMIC_ACQUIRE);
            if (sec_count > SHM_MAX_SECONDARY_CHUNKS) {
                sec_count = SHM_MAX_SECONDARY_CHUNKS;
            }

            shm_mutation_chunk_t *sec_chunk = NULL;
            int active_idx = -1;

            if (sec_count > 0 && sec_count <= SHM_MAX_SECONDARY_CHUNKS) {
                active_idx = (int)sec_count - 1;
                shm_mutation_chunk_desc_t *desc = &mq->secondary_chunks[active_idx];
                uint32_t chead = __atomic_load_n(&desc->head, __ATOMIC_ACQUIRE);
                uint32_t ctail = __atomic_load_n(&desc->tail, __ATOMIC_ACQUIRE);
                if (chead - ctail < desc->capacity && desc->shm_name[0] != '\0' && producer_sec_chunks[active_idx] != NULL) {
                    sec_chunk = producer_sec_chunks[active_idx];
                }
            }

            if (!sec_chunk) {
                uint32_t cur_sec_count = __atomic_load_n(&mq->secondary_chunk_count, __ATOMIC_ACQUIRE);
                if (cur_sec_count >= SHM_MAX_SECONDARY_CHUNKS) {
                    NSLOG(wisp, ERROR, "[BBMQ] Maximum secondary shared memory chunks (%d) exceeded during flush!", SHM_MAX_SECONDARY_CHUNKS);
                    break;
                }
                active_idx = (int)cur_sec_count;
                char chunk_name[64];
                uint32_t seq = (uint32_t)__atomic_fetch_add(&bbmq_chunk_seq, 1, __ATOMIC_RELAXED);
                snprintf(chunk_name, sizeof(chunk_name), "/w_sec_%u_%u", (unsigned int)getpid(), (unsigned int)seq);

                sec_chunk = shm_mutation_chunk_create(chunk_name, true);
                if (!sec_chunk) {
                    NSLOG(wisp, ERROR, "[BBMQ] Failed to create secondary shared memory chunk %s", chunk_name);
                    break;
                }
                producer_sec_chunks[active_idx] = sec_chunk;
                strncpy(producer_sec_names[active_idx], chunk_name, sizeof(producer_sec_names[active_idx]) - 1);
                producer_sec_names[active_idx][sizeof(producer_sec_names[active_idx]) - 1] = '\0';

                shm_mutation_chunk_desc_t *desc = &mq->secondary_chunks[active_idx];
                desc->capacity = SHM_MUTATION_CHUNK_CAPACITY;
                __atomic_store_n(&desc->head, 0, __ATOMIC_RELEASE);
                __atomic_store_n(&desc->tail, 0, __ATOMIC_RELEASE);
                strncpy(desc->shm_name, chunk_name, sizeof(desc->shm_name) - 1);
                desc->shm_name[sizeof(desc->shm_name) - 1] = '\0';
                __atomic_thread_fence(__ATOMIC_RELEASE);
                __atomic_store_n(&mq->secondary_chunk_count, active_idx + 1, __ATOMIC_RELEASE);
            }

            shm_mutation_chunk_desc_t *desc = &mq->secondary_chunks[active_idx];
            uint32_t chead = __atomic_load_n(&desc->head, __ATOMIC_ACQUIRE);
            uint32_t ctail = __atomic_load_n(&desc->tail, __ATOMIC_ACQUIRE);
            while (processed < bbmq_size && (chead - ctail < desc->capacity)) {
                uint32_t src_idx = (bbmq_head + processed) % bbmq_capacity;
                uint32_t c_idx = chead % desc->capacity;
                sec_chunk->queue[c_idx] = bbmq_buffer[src_idx];
                chead++;
                processed++;
            }

            // Publish secondary_chunk_count and desc->head ONLY AFTER writing items into sec_chunk->queue
            __atomic_store_n(&desc->head, chead, __ATOMIC_RELEASE);
            uint32_t current_sec = __atomic_load_n(&mq->secondary_chunk_count, __ATOMIC_ACQUIRE);
            if ((uint32_t)(active_idx + 1) > current_sec) {
                __atomic_store_n(&mq->secondary_chunk_count, active_idx + 1, __ATOMIC_RELEASE);
            }
        }
    }

#ifdef _WIN32
    MemoryBarrier();
#else
    __sync_synchronize();
#endif

    __atomic_store_n(&mq->head, head, __ATOMIC_RELEASE);
    bbmq_head = 0;
    bbmq_tail = 0;
    bbmq_size = 0;
}

WispCompactNode* find_shm_node(shm_dom_t *shm, uint64_t id) {
    if (!shm) return NULL;

    if (wisp_is_js_process && shm == wisp_shm_dom) {
        extern uint32_t wisp_shm_capacity;
        if (wisp_shm_dom && wisp_shm_capacity < wisp_shm_dom->node_capacity) {
            uint32_t new_cap = wisp_shm_dom->node_capacity;
            wisp_shm_dom = shm_dom_remap(wisp_shm_dom, wisp_shm_capacity, new_cap);
            if (wisp_shm_dom) {
                wisp_shm_capacity = new_cap;
                shm = wisp_shm_dom;
            } else {
                wisp_shm_capacity = 0;
                shm = NULL;
            }
        }
    }

    if (!shm) return NULL;

    uint32_t idx = (uint32_t)id;
    if (idx > 0 && idx < shm->node_count) {
        return &shm_dom_get_nodes(shm)[idx];
    }
    return NULL;
}
