
#ifndef WISP_IPC_H_
#define WISP_IPC_H_

#include "wisp/utils/errors.h"

/**
 * IPC message types
 */
enum ipc_message_type {
    IPC_MSG_NAVIGATE,
    IPC_MSG_PLOT,
    IPC_MSG_MOUSE_EVENT,
    IPC_MSG_KEY_EVENT
};

/**
 * IPC connection handle
 */
struct ipc_connection;

nserror ipc_init(void);
nserror ipc_connect(const char *name, struct ipc_connection **conn);
nserror ipc_listen(const char *name, struct ipc_connection **conn);
nserror ipc_send(struct ipc_connection *conn, enum ipc_message_type type, const void *data, size_t len);
nserror ipc_receive(struct ipc_connection *conn, enum ipc_message_type *type, void **data, size_t *len);
void ipc_close(struct ipc_connection *conn);

#endif
