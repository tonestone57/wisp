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

#define BBMQ_MAX_MUTATIONS 2048

extern bool wisp_is_js_process;
extern shm_dom_t *wisp_shm_dom;

static shm_mutation_t bbmq_queue[BBMQ_MAX_MUTATIONS];
static volatile uint32_t bbmq_count = 0;

void shm_mutation_enqueue(shm_dom_t *shm, uint32_t type, uint64_t target_id, uint64_t param1_id, uint64_t param2_id, const char *name, const char *value) {
    if (!shm) return;
    if (wisp_is_js_process) {
        if (bbmq_count >= BBMQ_MAX_MUTATIONS) {
            NSLOG(wisp, WARNING, "[BBMQ] Local mutation buffer is full, discarding mutation!");
            return;
        }
        shm_mutation_t *m = &bbmq_queue[bbmq_count];
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
        bbmq_count++;
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

void bbmq_flush(void) {
    if (!wisp_shm_dom) return;
    if (bbmq_count == 0) return;

    shm_mutation_queue_t *mq = &wisp_shm_dom->mutation_queue;
    uint32_t head = mq->head;
    uint32_t tail = mq->tail;

    for (uint32_t i = 0; i < bbmq_count; i++) {
        if (head - tail >= SHM_MUTATION_QUEUE_SIZE) {
            NSLOG(wisp, WARNING, "[BBMQ] Shared mutation queue is full during flush!");
            break;
        }
        uint32_t idx = head % SHM_MUTATION_QUEUE_SIZE;
        mq->queue[idx] = bbmq_queue[i];
        head++;
    }

#ifdef _WIN32
    MemoryBarrier();
#else
    __sync_synchronize();
#endif

    mq->head = head;
    bbmq_count = 0;
}

shm_dom_node_t* find_shm_node(shm_dom_t *shm, uint64_t id) {
    if (!shm) return NULL;
    for (uint32_t i = 0; i < shm->node_count; i++) {
        if (shm->nodes[i].id == id) {
            return &shm->nodes[i];
        }
    }
    return NULL;
}
