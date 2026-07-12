#ifndef WISP_UTILS_IPC_H
#define WISP_UTILS_IPC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "wisp/utils/errors.h"

typedef struct wisp_ipc_handle wisp_ipc_handle;

typedef enum {
    WISP_IPC_MSG_INVALID = 0,
    /* Network messages */
    WISP_IPC_MSG_FETCH_REQUEST = 1,
    WISP_IPC_MSG_FETCH_HEADER = 2,
    WISP_IPC_MSG_FETCH_DATA = 3,
    WISP_IPC_MSG_FETCH_FINISHED = 4,
    WISP_IPC_MSG_FETCH_ERROR = 5,
    WISP_IPC_MSG_FETCH_REDIRECT = 6,
    WISP_IPC_MSG_DNS_PREFETCH_REQUEST = 7,
    WISP_IPC_MSG_PRECONNECT_REQUEST = 8,

    /* JS messages */
    WISP_IPC_MSG_JS_EXEC = 100,
    WISP_IPC_MSG_JS_EVENT = 101,
    WISP_IPC_MSG_DOM_REQUEST = 102,
    WISP_IPC_MSG_DOM_RESPONSE = 103,
} wisp_ipc_msg_type;

typedef struct {
    wisp_ipc_msg_type type;
    uint32_t length;
    uint8_t *data;
} wisp_ipc_msg;

/* IPC lifecycle */
wisp_ipc_handle* wisp_ipc_create_server(const char *name);
wisp_ipc_handle* wisp_ipc_connect(const char *name);
wisp_ipc_handle* wisp_ipc_accept(wisp_ipc_handle *server);
void wisp_ipc_destroy(wisp_ipc_handle *handle);

/* Communication */
nserror wisp_ipc_send(wisp_ipc_handle *handle, const wisp_ipc_msg *msg);
nserror wisp_ipc_recv(wisp_ipc_handle *handle, wisp_ipc_msg *msg);
void wisp_ipc_msg_free(wisp_ipc_msg *msg);

/* Control */
void wisp_ipc_set_blocking(wisp_ipc_handle *handle, bool blocking);

/* Process spawning */
int wisp_ipc_spawn(const char *executable, const char *ipc_name);

/* Helper to locate a child process executable */
bool wisp_ipc_find_executable(const char *name, char *out_path, size_t out_len);

#endif
