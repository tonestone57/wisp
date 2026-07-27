#include "wisp/utils/shm_dom.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "wisp/utils/log.h"
#include <unistd.h>

int peak_nodes_used = 0;
static bool shm_dom_metrics_registered = false;

static void shm_dom_log_final_peak(void) {
    NSLOG(wisp, INFO, "=========================================");
    NSLOG(wisp, INFO, "[SHM_DOM] BROWSER EXIT METRICS");
    NSLOG(wisp, INFO, "[SHM_DOM] Peak shared-memory nodes used: %d",
          peak_nodes_used);
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

void shm_dom_lock_read(shm_dom_t *shm) {
    if (!shm) return;
    while (1) {
        uint32_t val = shm->lock;
        if (val != 0xFFFFFFFF) {
            if (__atomic_compare_exchange_n(&shm->lock, &val, val + 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
                break;
            }
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
    __atomic_sub_fetch(&shm->lock, 1, __ATOMIC_RELEASE);
}

void shm_dom_lock_write(shm_dom_t *shm) {
    if (!shm) return;
    while (1) {
        uint32_t expected = 0;
        if (__atomic_compare_exchange_n(&shm->lock, &expected, 0xFFFFFFFF, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
            break;
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
    __atomic_store_n(&shm->lock, 0, __ATOMIC_RELEASE);
}

size_t shm_dom_size(uint32_t capacity) {
    return sizeof(shm_dom_t) + capacity * (sizeof(WispCompactNode) + sizeof(WispShmLayoutCache) + sizeof(WispNodeStrings) + sizeof(uint64_t));
}

shm_dom_t* shm_dom_remap(shm_dom_t *old_shm, uint32_t new_capacity) {
    if (!old_shm) return NULL;
    char name[64];
    strncpy(name, old_shm->shm_name, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
    uint32_t old_cap = old_shm->node_capacity;
    bool is_server = old_shm->is_server;

    size_t old_size = shm_dom_size(old_cap);
    size_t new_size = shm_dom_size(new_capacity);

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

    if (!hMap) return NULL;

    shm_dom_t *new_shm = (shm_dom_t *)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, new_size);
    if (new_shm) {
        register_shm_handle(new_shm, hMap);
    } else {
        CloseHandle(hMap);
    }
    return new_shm;
#else
    int fd = shm_open(name, O_RDWR, 0666);
    if (fd < 0) {
        NSLOG(wisp, ERROR, "[SHM_DOM] shm_dom_remap: failed to shm_open %s", name);
        return old_shm;
    }

    if (is_server) {
        if (ftruncate(fd, new_size) != 0) {
            NSLOG(wisp, ERROR, "[SHM_DOM] shm_dom_remap: ftruncate failed");
            close(fd);
            return old_shm;
        }
    }

    munmap(old_shm, old_size);

    shm_dom_t *new_shm = (shm_dom_t *)mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if (new_shm == MAP_FAILED) {
        NSLOG(wisp, ERROR, "[SHM_DOM] shm_dom_remap: mmap failed");
        return NULL;
    }
    return new_shm;
#endif
}

shm_dom_t* shm_dom_create(const char *name, bool is_server) {
    if (is_server && !shm_dom_metrics_registered) {
        atexit(shm_dom_log_final_peak);
        shm_dom_metrics_registered = true;
    }
#ifdef _WIN32
    HANDLE hMap = NULL;
    size_t size = shm_dom_size(SHM_DOM_MAX_NODES);
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
            shm->node_capacity = SHM_DOM_MAX_NODES;
            shm->is_server = true;
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
            if (cap == 0) cap = SHM_DOM_MAX_NODES;
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
    size_t size = shm_dom_size(SHM_DOM_MAX_NODES);
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
        shm->node_capacity = SHM_DOM_MAX_NODES;
        shm->is_server = true;
        strncpy(shm->shm_name, name, sizeof(shm->shm_name) - 1);
        shm->shm_name[sizeof(shm->shm_name) - 1] = '\0';
    } else {
        shm_dom_t *temp = (shm_dom_t *)mmap(NULL, sizeof(shm_dom_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (temp == MAP_FAILED) {
            close(fd);
            return NULL;
        }
        uint32_t cap = temp->node_capacity;
        if (cap == 0) cap = SHM_DOM_MAX_NODES;
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

    if (wisp_is_js_process) {
        shm_dom_lock_read(wisp_shm_dom);
        extern uint32_t wisp_shm_capacity;
        if (wisp_shm_dom && wisp_shm_capacity < wisp_shm_dom->node_capacity) {
            uint32_t new_cap = wisp_shm_dom->node_capacity;
            wisp_shm_dom = shm_dom_remap(wisp_shm_dom, new_cap);
            wisp_shm_capacity = new_cap;
            shm = wisp_shm_dom;
        }
    } else {
        shm_dom_lock_read(shm);
    }

    shm->layout_dirty = true;
    WispCompactNode *sn = find_shm_node(shm, target_id);
    if (sn && sn->layout_index != 0) {
        shm_dom_get_layout_cache(shm)[sn->layout_index].layout_dirty = 1;
    }

    if (sn) {
        WispNodeStrings *sns = &shm_dom_get_node_strings(shm)[target_id];
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
        shm_dom_unlock_read(wisp_shm_dom);
        return;
    }

    shm_mutation_queue_t *mq = &shm->mutation_queue;
    uint32_t head = mq->head;
    uint32_t tail = mq->tail;
    if (head - tail >= SHM_MUTATION_QUEUE_SIZE) {
        shm_dom_unlock_read(shm);
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
    shm_dom_unlock_read(shm);
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

    if (wisp_is_js_process && shm == wisp_shm_dom) {
        extern uint32_t wisp_shm_capacity;
        if (wisp_shm_dom && wisp_shm_capacity < wisp_shm_dom->node_capacity) {
            shm_dom_lock_read(wisp_shm_dom);
            uint32_t new_cap = wisp_shm_dom->node_capacity;
            wisp_shm_dom = shm_dom_remap(wisp_shm_dom, new_cap);
            wisp_shm_capacity = new_cap;
            shm = wisp_shm_dom;
            shm_dom_unlock_read(wisp_shm_dom);
        }
    }

    uint32_t idx = (uint32_t)id;
    if (idx > 0 && idx < shm->node_count) {
        return &shm_dom_get_nodes(shm)[idx];
    }
    return NULL;
}
