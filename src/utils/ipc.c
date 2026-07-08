#include "wisp/utils/ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#endif

struct wisp_ipc_handle {
    int fd;
    bool is_server;
    char *name;
    bool non_blocking;
};

wisp_ipc_handle* wisp_ipc_create_server(const char *name) {
    wisp_ipc_handle *h = calloc(1, sizeof(*h));
    if (!h) return NULL;
    h->is_server = true;
    h->name = strdup(name);

#ifdef _WIN32
    static bool wsa_init = false;
    if (!wsa_init) {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
        wsa_init = true;
    }
    h->fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = 0; // OS picks port
    if (bind(h->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        free(h->name);
        free(h);
        return NULL;
    }

    struct sockaddr_in sin;
    int len = sizeof(sin);
    if (getsockname(h->fd, (struct sockaddr *)&sin, &len) == 0) {
        unsigned short port = ntohs(sin.sin_port);
        free(h->name);
        h->name = malloc(16);
        snprintf(h->name, 16, "%u", port);
    }

    listen(h->fd, 5);
#else
    h->fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, name, sizeof(addr.sun_path) - 1);
    unlink(name);
    if (bind(h->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        free(h->name);
        free(h);
        return NULL;
    }
    listen(h->fd, 5);
#endif
    return h;
}

wisp_ipc_handle* wisp_ipc_connect(const char *name) {
    wisp_ipc_handle *h = calloc(1, sizeof(*h));
    if (!h) return NULL;
#ifdef _WIN32
    h->fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = atoi(name); // Port passed as string
    if (connect(h->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        free(h);
        return NULL;
    }
#else
    h->fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, name, sizeof(addr.sun_path) - 1);
    if (connect(h->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        free(h);
        return NULL;
    }
#endif
    return h;
}

wisp_ipc_handle* wisp_ipc_accept(wisp_ipc_handle *server) {
    wisp_ipc_handle *h = calloc(1, sizeof(*h));
    if (!h) return NULL;
    h->fd = accept(server->fd, NULL, NULL);
    if (h->fd < 0) {
        free(h);
        return NULL;
    }
    return h;
}

void wisp_ipc_destroy(wisp_ipc_handle *handle) {
    if (!handle) return;
#ifdef _WIN32
    closesocket(handle->fd);
#else
    close(handle->fd);
    if (handle->is_server && handle->name) {
        unlink(handle->name);
    }
#endif
    if (handle->name) free(handle->name);
    free(handle);
}

static ssize_t write_all(int fd, const void *buf, size_t len) {
    size_t total = 0;
    const uint8_t *p = buf;
    while (total < len) {
        ssize_t n = write(fd, p + total, len - total);
        if (n <= 0) {
            if (n < 0 && (errno == EINTR || errno == EAGAIN)) continue;
            return -1;
        }
        total += n;
    }
    return total;
}

static ssize_t read_all(int fd, void *buf, size_t len) {
    size_t total = 0;
    uint8_t *p = buf;
    while (total < len) {
        ssize_t n = read(fd, p + total, len - total);
        if (n <= 0) {
            if (n < 0 && (errno == EINTR || errno == EAGAIN)) {
                if (total == 0) return -1; // EAGAIN on first read
                continue;
            }
            return total == 0 ? 0 : -1; // EOF or error
        }
        total += n;
    }
    return total;
}

nserror wisp_ipc_send(wisp_ipc_handle *handle, const wisp_ipc_msg *msg) {
    uint32_t header[2];
    header[0] = (uint32_t)msg->type;
    header[1] = msg->length;

    if (write_all(handle->fd, header, sizeof(header)) != sizeof(header)) return NSERROR_SAVE_FAILED;
    if (msg->length > 0) {
        if (write_all(handle->fd, msg->data, msg->length) != (ssize_t)msg->length) return NSERROR_SAVE_FAILED;
    }
    return NSERROR_OK;
}

nserror wisp_ipc_recv(wisp_ipc_handle *handle, wisp_ipc_msg *msg) {
    uint32_t header[2];
    ssize_t ret = read_all(handle->fd, header, sizeof(header));
    if (ret < 0) return NSERROR_NOT_FOUND; // EAGAIN
    if (ret == 0) return NSERROR_NOT_FOUND; // EOF
    if (ret != sizeof(header)) return NSERROR_INVALID;

    msg->type = (wisp_ipc_msg_type)header[0];
    msg->length = header[1];
    if (msg->length > 0) {
        msg->data = malloc(msg->length);
        if (!msg->data) return NSERROR_NOMEM;
        if (read_all(handle->fd, msg->data, msg->length) != (ssize_t)msg->length) {
            free(msg->data);
            return NSERROR_INVALID;
        }
    } else {
        msg->data = NULL;
    }
    return NSERROR_OK;
}

void wisp_ipc_msg_free(wisp_ipc_msg *msg) {
    if (msg->data) free(msg->data);
    msg->data = NULL;
}

void wisp_ipc_set_blocking(wisp_ipc_handle *handle, bool blocking) {
    handle->non_blocking = !blocking;
#ifdef _WIN32
    unsigned long mode = blocking ? 0 : 1;
    ioctlsocket(handle->fd, FIONBIO, &mode);
#else
    int flags = fcntl(handle->fd, F_GETFL, 0);
    if (blocking) flags &= ~O_NONBLOCK;
    else flags |= O_NONBLOCK;
    fcntl(handle->fd, F_SETFL, flags);
#endif
}

int wisp_ipc_spawn(const char *executable, const char *ipc_name) {
#ifdef _WIN32
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "%s %s", executable, ipc_name);
    if (!CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) return -1;
    return pi.dwProcessId;
#else
    pid_t pid = fork();
    if (pid == 0) {
        execl(executable, executable, ipc_name, NULL);
        exit(1);
    }
    return pid;
#endif
}
