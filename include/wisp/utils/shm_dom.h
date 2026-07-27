#ifndef WISP_UTILS_SHM_DOM_H
#define WISP_UTILS_SHM_DOM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SHM_DOM_MAX_NODES 8192
#define SHM_DOM_STRING_MAX 128
#define SHM_MUTATION_QUEUE_SIZE 1024

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
} shm_mutation_type_t;

typedef struct {
    uint32_t type;                /* shm_mutation_type_t */
    uint64_t target_id;           /* ID of the target node */
    uint64_t param1_id;           /* e.g., child node ID to append/remove */
    uint64_t param2_id;           /* e.g., child node ID for insertBefore/replace */
    char name[SHM_DOM_STRING_MAX];  /* attribute name */
    char value[SHM_DOM_STRING_MAX]; /* attribute value or text value */
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
    char name[SHM_DOM_STRING_MAX];
    char value[SHM_DOM_STRING_MAX];
    char tag_name[SHM_DOM_STRING_MAX];
    struct {
        char name[SHM_DOM_STRING_MAX];
        char value[SHM_DOM_STRING_MAX];
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
    shm_mutation_queue_t mutation_queue;
    /* In memory, this struct is followed by arrays:
     * WispCompactNode nodes[node_capacity]
     * WispShmLayoutCache layout_cache[node_capacity]
     * WispNodeStrings node_strings[node_capacity]
     * uint64_t dom_ptrs[node_capacity]
     */
} shm_dom_t;

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

/* API */
shm_dom_t* shm_dom_create(const char *name, bool is_server);
void shm_dom_destroy(shm_dom_t *shm, const char *name, bool is_server);
void shm_mutation_enqueue(shm_dom_t *shm, uint32_t type, uint64_t target_id, uint64_t param1_id, uint64_t param2_id, const char *name, const char *value);
void bbmq_flush(void);
bool bbmq_has_pending_for_node(uint64_t target_id);
WispCompactNode* find_shm_node(shm_dom_t *shm, uint64_t id);

void shm_dom_lock_read(shm_dom_t *shm);
void shm_dom_unlock_read(shm_dom_t *shm);
void shm_dom_lock_write(shm_dom_t *shm);
void shm_dom_unlock_write(shm_dom_t *shm);
shm_dom_t* shm_dom_remap(shm_dom_t *old_shm, uint32_t new_capacity);
size_t shm_dom_size(uint32_t capacity);

#endif /* WISP_UTILS_SHM_DOM_H */
