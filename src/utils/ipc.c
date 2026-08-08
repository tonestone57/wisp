#include "wisp/utils/ipc.h"
#include "wisp/utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define socket_errno WSAGetLastError()
#define SOCKET_EAGAIN WSAEWOULDBLOCK
#define SOCKET_EINTR WSAEINTR
#define send_socket(fd, buf, len) send((SOCKET)(fd), (const char *)(buf), (int)(len), 0)
#define recv_socket(fd, buf, len) recv((SOCKET)(fd), (char *)(buf), (int)(len), 0)
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#define socket_errno errno
#define SOCKET_EAGAIN EAGAIN
#define SOCKET_EINTR EINTR
#define send_socket(fd, buf, len) write((fd), (buf), (len))
#define recv_socket(fd, buf, len) read((fd), (buf), (len))
#endif

#include <stdint.h>

struct wisp_ipc_handle {
    intptr_t fd;
    bool is_server;
    char *name;
    bool non_blocking;
};

static void wisp_ipc_set_cloexec(int fd) {
#ifdef _WIN32
    SetHandleInformation((HANDLE)fd, HANDLE_FLAG_INHERIT, 0);
#else
    int flags = fcntl(fd, F_GETFD, 0);
    if (flags != -1) {
        fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
    }
#endif
}

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
    h->fd = (intptr_t)socket(AF_INET, SOCK_STREAM, 0);
    wisp_ipc_set_cloexec(h->fd);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = 0; // OS picks port
    if (bind((SOCKET)h->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        closesocket((SOCKET)h->fd);
        free(h->name);
        free(h);
        return NULL;
    }

    struct sockaddr_in sin;
    int len = sizeof(sin);
    if (getsockname((SOCKET)h->fd, (struct sockaddr *)&sin, &len) == 0) {
        unsigned short port = ntohs(sin.sin_port);
        free(h->name);
        h->name = malloc(16);
        snprintf(h->name, 16, "%u", port);
    }

    listen((SOCKET)h->fd, 5);
#else
    h->fd = socket(AF_UNIX, SOCK_STREAM, 0);
    wisp_ipc_set_cloexec(h->fd);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, name, sizeof(addr.sun_path) - 1);
    unlink(name);
    if (bind(h->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(h->fd);
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
    h->fd = (intptr_t)socket(AF_INET, SOCK_STREAM, 0);
    wisp_ipc_set_cloexec(h->fd);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    int port;
    if (ns_strtoint(name, 10, &port) != NSERROR_OK) {
        closesocket((SOCKET)h->fd);
        free(h);
        return NULL;
    }
    addr.sin_port = htons(port); // Port passed as string
    if (connect((SOCKET)h->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        closesocket((SOCKET)h->fd);
        free(h);
        return NULL;
    }
#else
    h->fd = socket(AF_UNIX, SOCK_STREAM, 0);
    wisp_ipc_set_cloexec(h->fd);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, name, sizeof(addr.sun_path) - 1);
    if (connect(h->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(h->fd);
        free(h);
        return NULL;
    }
#endif
    return h;
}

static bool wait_socket(intptr_t fd, bool for_write, int timeout_ms);

wisp_ipc_handle* wisp_ipc_accept(wisp_ipc_handle *server) {
    if (!server) return NULL;

    /* Wait up to 2 seconds for a client connection to prevent infinite hang if the child process crashes on startup */
    if (!wait_socket(server->fd, false, 2000)) {
        return NULL;
    }

    wisp_ipc_handle *h = calloc(1, sizeof(*h));
    if (!h) return NULL;
#ifdef _WIN32
    h->fd = (intptr_t)accept((SOCKET)server->fd, NULL, NULL);
    if ((SOCKET)h->fd == INVALID_SOCKET) {
        free(h);
        return NULL;
    }
#else
    h->fd = accept(server->fd, NULL, NULL);
    if (h->fd < 0) {
        free(h);
        return NULL;
    }
#endif
    wisp_ipc_set_cloexec(h->fd);
    return h;
}

void wisp_ipc_destroy(wisp_ipc_handle *handle) {
    if (!handle) return;
#ifdef _WIN32
    closesocket((SOCKET)handle->fd);
#else
    close(handle->fd);
    if (handle->is_server && handle->name) {
        unlink(handle->name);
    }
#endif
    if (handle->is_server && handle->name) {
        free(handle->name);
    }
    free(handle);
}


static bool wait_socket(intptr_t fd, bool for_write, int timeout_ms) {
    fd_set fds;
    FD_ZERO(&fds);
#ifdef _WIN32
    FD_SET((SOCKET)fd, &fds);
#else
    FD_SET((int)fd, &fds);
#endif
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    int ret;
    int nfds = 0;
#ifndef _WIN32
    nfds = (int)(fd + 1);
#endif
    if (for_write) {
        ret = select(nfds, NULL, &fds, NULL, timeout_ms >= 0 ? &tv : NULL);
    } else {
        ret = select(nfds, &fds, NULL, NULL, timeout_ms >= 0 ? &tv : NULL);
    }
    return ret > 0;
}

static ssize_t write_all(intptr_t fd, const void *buf, size_t len) {
    size_t total = 0;
    const uint8_t *p = buf;
    while (total < len) {
        ssize_t n = send_socket(fd, p + total, len - total);
        if (n <= 0) {
            if (n < 0) {
                int err = socket_errno;
                if (err == SOCKET_EINTR) {
                    continue;
                }
                if (err == SOCKET_EAGAIN) {
                    if (wait_socket(fd, true, 5000)) { // wait up to 5 seconds
                        continue;
                    }
                }
            }
            return -1;
        }
        total += n;
    }
    return total;
}

static ssize_t read_all(intptr_t fd, void *buf, size_t len, bool allow_eagain) {
    size_t total = 0;
    uint8_t *p = buf;
    while (total < len) {
        ssize_t n = recv_socket(fd, p + total, len - total);
        if (n <= 0) {
            if (n < 0) {
                int err = socket_errno;
                if (err == SOCKET_EINTR) {
                    continue;
                }
                if (err == SOCKET_EAGAIN) {
                    if (allow_eagain && total == 0) {
                        return -1; // EAGAIN on first read
                    }
                    /* Committed to a message, wait for remainder */
                    if (wait_socket(fd, false, 5000)) { // wait up to 5 seconds
                        continue;
                    }
                }
            }
            return total == 0 ? 0 : -2; // EOF or error (-2 for partial read failure)
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
    ssize_t ret = read_all(handle->fd, header, sizeof(header), true);
    if (ret < 0) {
        if (ret == -2) return NSERROR_INVALID;
        return NSERROR_NOT_FOUND; // EAGAIN
    }
    if (ret == 0) return NSERROR_SHUTDOWN; // EOF
    if (ret != sizeof(header)) return NSERROR_INVALID;

    msg->type = (wisp_ipc_msg_type)header[0];
    msg->length = header[1];
    if (msg->length > 0) {
        msg->data = malloc(msg->length);
        if (!msg->data) return NSERROR_NOMEM;
        if (read_all(handle->fd, msg->data, msg->length, false) != (ssize_t)msg->length) {
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
    ioctlsocket((SOCKET)handle->fd, FIONBIO, &mode);
#else
    int flags = fcntl((int)handle->fd, F_GETFL, 0);
    if (blocking) flags &= ~O_NONBLOCK;
    else flags |= O_NONBLOCK;
    fcntl((int)handle->fd, F_SETFL, flags);
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
        execlp(executable, executable, ipc_name, NULL);
        _exit(1);
    }
    return pid;
#endif
}

bool wisp_ipc_find_executable(const char *name, char *out_path, size_t out_len) {
#ifdef _WIN32
    char self_path[MAX_PATH];
    if (GetModuleFileNameA(NULL, self_path, sizeof(self_path)) > 0) {
        char *last_backslash = strrchr(self_path, '\\');
        if (last_backslash) {
            *last_backslash = '\0';
            // Try same directory (e.g. for installed)
            snprintf(out_path, out_len, "%s\\%s.exe", self_path, name);
            if (access(out_path, 0) == 0) {
                return true;
            }
            // Try ..\src\ (for build directory)
            snprintf(out_path, out_len, "%s\\..\\src\\%s.exe", self_path, name);
            if (access(out_path, 0) == 0) {
                return true;
            }
            // Try ..\..\src\ (for frontend build directories like build\frontends\qt)
            snprintf(out_path, out_len, "%s\\..\\..\\src\\%s.exe", self_path, name);
            if (access(out_path, 0) == 0) {
                return true;
            }
        }
    }
    snprintf(out_path, out_len, ".\\%s.exe", name);
    if (access(out_path, 0) == 0) {
        return true;
    }
    strncpy(out_path, name, out_len - 1);
    out_path[out_len - 1] = '\0';
    return true;
#else
    // 1. Try to read /proc/self/exe (Linux)
    char self_path[512];
    ssize_t len = readlink("/proc/self/exe", self_path, sizeof(self_path) - 1);
    if (len != -1) {
        self_path[len] = '\0';
        char *last_slash = strrchr(self_path, '/');
        if (last_slash) {
            *last_slash = '\0';
            // Try same directory (e.g. for installed)
            snprintf(out_path, out_len, "%s/%s", self_path, name);
            if (access(out_path, X_OK) == 0) {
                return true;
            }
            // Try ../src/ (for build directory)
            snprintf(out_path, out_len, "%s/../src/%s", self_path, name);
            if (access(out_path, X_OK) == 0) {
                return true;
            }
            // Try ../../src/ (for frontend build directories like build/frontends/gtk)
            snprintf(out_path, out_len, "%s/../../src/%s", self_path, name);
            if (access(out_path, X_OK) == 0) {
                return true;
            }
        }
    }

    // 2. Try the current directory
    snprintf(out_path, out_len, "./%s", name);
    if (access(out_path, X_OK) == 0) {
        return true;
    }
    snprintf(out_path, out_len, "./build/src/%s", name);
    if (access(out_path, X_OK) == 0) {
        return true;
    }

    // 3. Try standard installation paths
    snprintf(out_path, out_len, "/usr/local/bin/%s", name);
    if (access(out_path, X_OK) == 0) {
        return true;
    }
    snprintf(out_path, out_len, "/usr/bin/%s", name);
    if (access(out_path, X_OK) == 0) {
        return true;
    }

    // 4. Try searching PATH
    const char *path_env = getenv("PATH");
    if (path_env) {
        char *path_copy = strdup(path_env);
        if (path_copy) {
            char *dir = strtok(path_copy, ":");
            while (dir) {
                snprintf(out_path, out_len, "%s/%s", dir, name);
                if (access(out_path, X_OK) == 0) {
                    free(path_copy);
                    return true;
                }
                dir = strtok(NULL, ":");
            }
            free(path_copy);
        }
    }

    return false;
#endif
}
