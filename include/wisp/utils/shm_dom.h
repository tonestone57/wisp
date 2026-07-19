#ifndef WISP_UTILS_SHM_DOM_H
#define WISP_UTILS_SHM_DOM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SHM_DOM_MAX_NODES 8192
#define SHM_DOM_STRING_MAX 128
#define SHM_MUTATION_QUEUE_SIZE 1024

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

typedef struct {
    uint64_t id;                  /* cast of (uintptr_t)dom_node* */
    uint32_t type;                /* dom_node_type */
    uint64_t parent_id;
    uint64_t first_child_id;
    uint64_t last_child_id;
    uint64_t next_sibling_id;
    uint64_t previous_sibling_id;
    char name[SHM_DOM_STRING_MAX];
    char value[SHM_DOM_STRING_MAX];
    char tag_name[SHM_DOM_STRING_MAX];
    struct {
        char name[SHM_DOM_STRING_MAX];
        char value[SHM_DOM_STRING_MAX];
    } attrs[16];
    uint32_t attr_count;
} shm_dom_node_t;

typedef struct {
    shm_dom_node_t nodes[SHM_DOM_MAX_NODES];
    uint32_t node_count;
    shm_mutation_queue_t mutation_queue;
} shm_dom_t;

/* API */
shm_dom_t* shm_dom_create(const char *name, bool is_server);
void shm_dom_destroy(shm_dom_t *shm, const char *name, bool is_server);
void shm_mutation_enqueue(shm_dom_t *shm, uint32_t type, uint64_t target_id, uint64_t param1_id, uint64_t param2_id, const char *name, const char *value);
shm_dom_node_t* find_shm_node(shm_dom_t *shm, uint64_t id);

#endif /* WISP_UTILS_SHM_DOM_H */
