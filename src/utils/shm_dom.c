#include "wisp/utils/shm_dom.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "wisp/utils/log.h"

int peak_nodes_used = 0;
static bool shm_dom_metrics_registered = false;

static void shm_dom_log_final_peak(void) {
    NSLOG(wisp, INFO, "=========================================");
    NSLOG(wisp, INFO, "[SHM_DOM] BROWSER EXIT METRICS");
    NSLOG(wisp, INFO, "[SHM_DOM] Peak shared-memory nodes used: %d / %d",
          peak_nodes_used, SHM_DOM_MAX_NODES);
    NSLOG(wisp, INFO, "=========================================");
}

#ifdef _WIN32
#include <windows.h>

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
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

shm_dom_t* shm_dom_create(const char *name, bool is_server) {
    if (is_server && !shm_dom_metrics_registered) {
        atexit(shm_dom_log_final_peak);
        shm_dom_metrics_registered = true;
    }
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

        hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, sizeof(shm_dom_t), name);
    } else {
        hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name);
    }

    if (!hMap) return NULL;

    shm_dom_t *shm = (shm_dom_t *)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(shm_dom_t));
    if (shm) {
        if (is_server) {
            memset(shm, 0, sizeof(shm_dom_t));
        }
        register_shm_handle(shm, hMap);
    } else {
        CloseHandle(hMap);
    }
    return shm;
#else
    int fd = -1;
    if (is_server) {
        fd = shm_open(name, O_CREAT | O_RDWR, 0666);
        if (fd >= 0) {
            if (ftruncate(fd, sizeof(shm_dom_t)) != 0) {
                close(fd);
                shm_unlink(name);
                return NULL;
            }
        }
    } else {
        fd = shm_open(name, O_RDWR, 0666);
    }

    if (fd < 0) return NULL;

    shm_dom_t *shm = (shm_dom_t *)mmap(NULL, sizeof(shm_dom_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if (shm == MAP_FAILED) {
        if (is_server) {
            shm_unlink(name);
        }
        return NULL;
    }

    if (is_server) {
        memset(shm, 0, sizeof(shm_dom_t));
    }
    return shm;
#endif
}

void shm_dom_destroy(shm_dom_t *shm, const char *name, bool is_server) {
    if (!shm) return;
#ifdef _WIN32
    UnmapViewOfFile(shm);
    HANDLE h = find_and_unregister_shm_handle(shm);
    if (h) CloseHandle(h);
#else
    munmap(shm, sizeof(shm_dom_t));
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

static void bbmq_cleanup(void) {
    if (bbmq_buffer) {
        free(bbmq_buffer);
        bbmq_buffer = NULL;
    }
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

void shm_mutation_enqueue(shm_dom_t *shm, uint32_t type, uint64_t target_id, uint64_t param1_id, uint64_t param2_id, const char *name, const char *value) {
    if (!shm) return;

    shm->layout_dirty = true;
    WispCompactNode *sn = find_shm_node(shm, target_id);
    if (sn && sn->layout_index != 0) {
        shm->layout_cache[sn->layout_index].layout_dirty = 1;
    }

    if (sn) {
        WispNodeStrings *sns = &shm->node_strings[target_id];
        if (type == SHM_MUTATION_SET_ATTRIBUTE && name) {
            bool found = false;
            for (uint32_t i = 0; i < sns->attr_count; i++) {
                if (strcasecmp(sns->attrs[i].name, name) == 0) {
                    if (value) {
                        strncpy(sns->attrs[i].value, value, SHM_DOM_STRING_MAX - 1);
                        sns->attrs[i].value[SHM_DOM_STRING_MAX - 1] = '\0';
                    } else {
                        sns->attrs[i].value[0] = '\0';
                    }
                    found = true;
                    break;
                }
            }
            if (!found && sns->attr_count < 16) {
                uint32_t idx = sns->attr_count++;
                strncpy(sns->attrs[idx].name, name, SHM_DOM_STRING_MAX - 1);
                sns->attrs[idx].name[SHM_DOM_STRING_MAX - 1] = '\0';
                if (value) {
                    strncpy(sns->attrs[idx].value, value, SHM_DOM_STRING_MAX - 1);
                    sns->attrs[idx].value[SHM_DOM_STRING_MAX - 1] = '\0';
                } else {
                    sns->attrs[idx].value[0] = '\0';
                }
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
            for (uint32_t i = 0; i < sns->attr_count; i++) {
                if (strcasecmp(sns->attrs[i].name, name) == 0) {
                    sns->attrs[i] = sns->attrs[--sns->attr_count];
                    break;
                }
            }
        } else if (type == SHM_MUTATION_SET_NODE_VALUE || type == SHM_MUTATION_SET_TEXT_CONTENT) {
            if (value) {
                strncpy(sns->value, value, SHM_DOM_STRING_MAX - 1);
                sns->value[SHM_DOM_STRING_MAX - 1] = '\0';
            } else {
                sns->value[0] = '\0';
            }
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
        if (name) {
            strncpy(m->name, name, SHM_DOM_STRING_MAX - 1);
            m->name[SHM_DOM_STRING_MAX - 1] = '\0';
        } else {
            m->name[0] = '\0';
        }
        if (value) {
            strncpy(m->value, value, SHM_DOM_STRING_MAX - 1);
            m->value[SHM_DOM_STRING_MAX - 1] = '\0';
        } else {
            m->value[0] = '\0';
        }
        bbmq_tail = (bbmq_tail + 1) % bbmq_capacity;
        bbmq_size++;
        return;
    }

    shm_mutation_queue_t *mq = &shm->mutation_queue;
    uint32_t head = mq->head;
    uint32_t tail = mq->tail;
    if (head - tail >= SHM_MUTATION_QUEUE_SIZE) {
        return;
    }
    uint32_t idx = head % SHM_MUTATION_QUEUE_SIZE;
    shm_mutation_t *m = &mq->queue[idx];
    m->type = type;
    m->target_id = target_id;
    m->param1_id = param1_id;
    m->param2_id = param2_id;
    if (name) {
        strncpy(m->name, name, SHM_DOM_STRING_MAX - 1);
        m->name[SHM_DOM_STRING_MAX - 1] = '\0';
    } else {
        m->name[0] = '\0';
    }
    if (value) {
        strncpy(m->value, value, SHM_DOM_STRING_MAX - 1);
        m->value[SHM_DOM_STRING_MAX - 1] = '\0';
    } else {
        m->value[0] = '\0';
    }

#ifdef _WIN32
    MemoryBarrier();
#else
    __sync_synchronize();
#endif
    mq->head = head + 1;
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

void bbmq_flush(void) {
    if (!wisp_shm_dom) return;
    if (bbmq_size == 0) return;

    shm_mutation_queue_t *mq = &wisp_shm_dom->mutation_queue;
    uint32_t head = mq->head;
    uint32_t tail = mq->tail;

    for (uint32_t i = 0; i < bbmq_size; i++) {
        if (head - tail >= SHM_MUTATION_QUEUE_SIZE) {
            NSLOG(wisp, WARNING, "[BBMQ] Shared mutation queue is full during flush!");
            break;
        }
        uint32_t src_idx = (bbmq_head + i) % bbmq_capacity;
        uint32_t idx = head % SHM_MUTATION_QUEUE_SIZE;
        mq->queue[idx] = bbmq_buffer[src_idx];
        head++;
    }

#ifdef _WIN32
    MemoryBarrier();
#else
    __sync_synchronize();
#endif

    mq->head = head;
    bbmq_head = 0;
    bbmq_tail = 0;
    bbmq_size = 0;
}

WispCompactNode* find_shm_node(shm_dom_t *shm, uint64_t id) {
    if (!shm) return NULL;
    uint32_t idx = (uint32_t)id;
    if (idx > 0 && idx < shm->node_count) {
        return &shm->nodes[idx];
    }
    return NULL;
}
