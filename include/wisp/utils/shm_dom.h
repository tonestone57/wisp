#ifndef WISP_UTILS_SHM_DOM_H
#define WISP_UTILS_SHM_DOM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define SHM_DOM_MAX_NODES 1024
#define SHM_MUTATION_QUEUE_SIZE 1024

#define SHM_STRING_HASH_SIZE 65536
#define SHM_STRING_HEAP_SIZE (2 * 1024 * 1024)

typedef uint32_t WispNodeID;
#define WISP_NODE_NULL 0

typedef enum {
    WISP_NODE_ELEMENT = 1,
    WISP_NODE_TEXT = 3,
    WISP_NODE_DOCUMENT = 9
} WispNodeType;

typedef enum {
    SHM_MUTATION_SET_ATTRIBUTE = 1,
    SHM_MUTATION_REMOVE_ATTRIBUTE = 2,
    SHM_MUTATION_APPEND_CHILD = 3,
    SHM_MUTATION_REMOVE_CHILD = 4,
    SHM_MUTATION_INSERT_BEFORE = 5,
    SHM_MUTATION_REPLACE_CHILD = 6,
    SHM_MUTATION_SET_NODE_VALUE = 7,
    SHM_MUTATION_SET_TEXT_CONTENT = 8,
    SHM_MUTATION_SET_INNER_HTML = 9,
} shm_mutation_type_t;

typedef uint32_t WispStringRef;
#define WISP_SSO_FLAG (1U << 31)

typedef struct {
    uint32_t type;                /* shm_mutation_type_t */
    uint64_t target_id;           /* ID of the target node */
    uint64_t param1_id;           /* e.g., child node ID to append/remove */
    uint64_t param2_id;           /* e.g., child node ID for insertBefore/replace */
    WispStringRef name;           /* attribute name */
    WispStringRef value;          /* attribute value or text value */
} shm_mutation_t;

typedef struct {
    volatile uint32_t head;       /* Written by JS process */
    volatile uint32_t tail;       /* Read/drained by main UI thread */
    shm_mutation_t queue[SHM_MUTATION_QUEUE_SIZE];
} shm_mutation_queue_t;

// Split topology node - exactly 32 bytes
typedef struct {
    uint32_t parent_id;
    uint32_t first_child_id;
    uint32_t next_sibling_id;
    uint32_t prev_sibling_id;
    uint16_t node_type;
    uint16_t tag_atom;
    uint32_t class_hash;
    uint32_t layout_index; // Index into flat LayoutCache array (0 if unrendered)
    uint32_t reserved;     // Explicit padding to ensure exactly 32 bytes
} WispCompactNode;

_Static_assert(sizeof(WispCompactNode) == 32, "WispCompactNode must be exactly 32 bytes");

typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    uint32_t seq_version;
    uint16_t layout_dirty;
    uint16_t flags;
    uint32_t reserved[10]; // Padding to make it exactly 64 bytes
} __attribute__((aligned(64))) WispShmLayoutCache;

_Static_assert(sizeof(WispShmLayoutCache) == 64, "WispShmLayoutCache must be exactly 64 bytes");

typedef struct {
    WispStringRef name;
    WispStringRef value;
    WispStringRef tag_name;
    struct {
        WispStringRef name;
        WispStringRef value;
    } attrs[16];
    uint32_t attr_count;
} WispNodeStrings;

typedef struct {
    volatile uint32_t lock;           /* Process-shared Read-Write Spinlock */
    char shm_name[64];                /* Name of the shared memory object */
    uint32_t node_capacity;           /* Current mapped capacity of nodes */
    uint32_t node_count;
    uint32_t layout_cache_count;
    bool layout_dirty;
    bool is_server;
    uint32_t string_hash_table[SHM_STRING_HASH_SIZE];
    char string_heap[SHM_STRING_HEAP_SIZE];
    uint32_t string_heap_top;
    shm_mutation_queue_t mutation_queue;
    /* In memory, this struct is followed by arrays:
     * WispCompactNode nodes[node_capacity]
     * WispShmLayoutCache layout_cache[node_capacity]
     * WispNodeStrings node_strings[node_capacity]
     * uint64_t dom_ptrs[node_capacity]
     */
} __attribute__((aligned(64))) shm_dom_t;

static inline WispCompactNode* shm_dom_get_nodes(shm_dom_t *shm) {
    if (!shm) return NULL;
    return (WispCompactNode*)((char*)shm + sizeof(shm_dom_t));
}

static inline WispShmLayoutCache* shm_dom_get_layout_cache(shm_dom_t *shm) {
    if (!shm) return NULL;
    return (WispShmLayoutCache*)((char*)shm + sizeof(shm_dom_t) + shm->node_capacity * sizeof(WispCompactNode));
}

static inline WispNodeStrings* shm_dom_get_node_strings(shm_dom_t *shm) {
    if (!shm) return NULL;
    return (WispNodeStrings*)((char*)shm + sizeof(shm_dom_t) + shm->node_capacity * (sizeof(WispCompactNode) + sizeof(WispShmLayoutCache)));
}

static inline uint64_t* shm_dom_get_dom_ptrs(shm_dom_t *shm) {
    if (!shm) return NULL;
    return (uint64_t*)((char*)shm + sizeof(shm_dom_t) + shm->node_capacity * (sizeof(WispCompactNode) + sizeof(WispShmLayoutCache) + sizeof(WispNodeStrings)));
}

/* Thread-local variables for SSO decoding */
extern __thread char wisp_sso_decode_bufs[16][4];
extern __thread uint32_t wisp_sso_decode_idx;

/* Helper Inline Functions for SSO & String references */
static inline WispStringRef wisp_make_string_ref(const char *str, size_t len, uint32_t heap_offset) {
    if (len <= 3) {
        WispStringRef sso = WISP_SSO_FLAG | ((uint8_t)len << 24);
        for (size_t i = 0; i < len; i++) {
            sso |= ((uint8_t)str[i]) << (i * 8);
        }
        return sso;
    }
    return heap_offset & ~WISP_SSO_FLAG;
}

static inline const char* wisp_string_ref_data(const shm_dom_t *shm, WispStringRef ref) {
    if (ref == 0) return "";
    if (ref & WISP_SSO_FLAG) {
        uint32_t idx = wisp_sso_decode_idx;
        wisp_sso_decode_idx = (wisp_sso_decode_idx + 1) & 15;
        char *buf = wisp_sso_decode_bufs[idx];
        uint32_t len = (ref >> 24) & 0x7F;
        if (len > 3) len = 3;
        for (uint32_t i = 0; i < len; i++) {
            buf[i] = (char)((ref >> (i * 8)) & 0xFF);
        }
        buf[len] = '\0';
        return buf;
    }
    if (shm) {
        uint32_t offset = ref & ~WISP_SSO_FLAG;
        if (offset < SHM_STRING_HEAP_SIZE) {
            return &shm->string_heap[offset];
        }
    }
    return "";
}

static inline bool wisp_string_ref_eq(const shm_dom_t *shm, WispStringRef ref, const char *str) {
    if (!str) return false;
    const char *data = wisp_string_ref_data(shm, ref);
    return strcmp(data, str) == 0;
}

static inline bool wisp_string_ref_caseeq(const shm_dom_t *shm, WispStringRef ref, const char *str) {
    if (!str) return false;
    const char *data = wisp_string_ref_data(shm, ref);
    return strcasecmp(data, str) == 0;
}

/* API */
shm_dom_t* shm_dom_create(const char *name, uint32_t capacity, bool is_server);
void shm_dom_destroy(shm_dom_t *shm, const char *name, bool is_server);
void shm_mutation_enqueue(shm_dom_t *shm, uint32_t type, uint64_t target_id, uint64_t param1_id, uint64_t param2_id, const char *name, const char *value);
void bbmq_flush(void);
bool bbmq_has_pending_for_node(uint64_t target_id);
WispCompactNode* find_shm_node(shm_dom_t *shm, uint64_t id);

WispStringRef wisp_shm_alloc_string(shm_dom_t *shm, const char *str);

void shm_dom_lock_read(shm_dom_t *shm);
void shm_dom_unlock_read(shm_dom_t *shm);
void shm_dom_lock_write(shm_dom_t *shm);
void shm_dom_unlock_write(shm_dom_t *shm);
shm_dom_t* shm_dom_remap(shm_dom_t *old_shm, uint32_t new_capacity);
size_t shm_dom_size(uint32_t capacity);

#endif /* WISP_UTILS_SHM_DOM_H */
