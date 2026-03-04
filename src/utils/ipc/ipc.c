
#include "wisp/ipc/ipc.h"
#include <stdlib.h>
#include <stdio.h>

struct ipc_connection {
    int fd;
};

nserror ipc_init(void) {
    return NSERROR_OK;
}

nserror ipc_connect(const char *name, struct ipc_connection **conn) {
    *conn = calloc(1, sizeof(struct ipc_connection));
    return NSERROR_OK;
}

nserror ipc_listen(const char *name, struct ipc_connection **conn) {
    *conn = calloc(1, sizeof(struct ipc_connection));
    return NSERROR_OK;
}

nserror ipc_send(struct ipc_connection *conn, enum ipc_message_type type, const void *data, size_t len) {
    return NSERROR_OK;
}

nserror ipc_receive(struct ipc_connection *conn, enum ipc_message_type *type, void **data, size_t *len) {
    return NSERROR_OK;
}

void ipc_close(struct ipc_connection *conn) {
    free(conn);
}
