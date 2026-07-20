#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define write_socket(fd, buf, len) send((SOCKET)(fd), (const char *)(buf), (int)(len), 0)
#define close_socket(fd) closesocket((SOCKET)(fd))
#else
#include <unistd.h>
#include <sys/socket.h>
#define write_socket(fd, buf, len) write((fd), (buf), (len))
#define close_socket(fd) close(fd)
#endif

#include "wisp/utils/ipc.h"
#include "wisp/utils/errors.h"

struct wisp_ipc_handle {
    intptr_t fd;
    bool is_server;
    char *name;
    bool non_blocking;
};

int main(int argc, char **argv) {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    printf("Running IPC Partial Read Test...\n");

    // 1. Create Server
    const char *sock_name = "test_ipc_partial_read_sock";
    wisp_ipc_handle *server = wisp_ipc_create_server(sock_name);
    assert(server != NULL);

    // 2. Connect Client
    wisp_ipc_handle *client = wisp_ipc_connect(server->name);
    assert(client != NULL);

    // 3. Accept Connection
    wisp_ipc_handle *accepted = wisp_ipc_accept(server);
    assert(accepted != NULL);

    // Test case A: Send nothing and check if recv returns NSERROR_NOT_FOUND (EAGAIN / WOULDBLOCK)
    wisp_ipc_set_blocking(accepted, false);
    wisp_ipc_msg msg;
    nserror err = wisp_ipc_recv(accepted, &msg);
    assert(err == NSERROR_NOT_FOUND);
    printf("EAGAIN handling: PASS\n");

    // Test case B: Write partial header (e.g. 4 bytes of 8) and close connection
    uint32_t partial_header = 12345; // 4 bytes
    ssize_t written = write_socket(client->fd, &partial_header, 4);
    assert(written == 4);

    // Close the client to cause EOF during the read on accepted
    close_socket(client->fd);
    client->fd = -1;

    // Recv should now hit EOF with total > 0, returning -2, which maps to NSERROR_INVALID
    err = wisp_ipc_recv(accepted, &msg);
    assert(err == NSERROR_INVALID);
    printf("Partial header read failure handling: PASS\n");

    // Clean up
    wisp_ipc_destroy(accepted);
    wisp_ipc_destroy(client);
    wisp_ipc_destroy(server);

#ifdef _WIN32
    WSACleanup();
#endif

    printf("IPC Partial Read Test: ALL PASSED\n");
    return 0;
}
