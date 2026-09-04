#include <wisp/fetch.h>
#include <wisp/utils/ipc.h>
#include <wisp/utils/log.h>
#include <wisp/utils/nsurl.h>
#include <wisp/utils/corestrings.h>
#include "content/fetchers.h"
#include <wisp/content/fetch.h>
#include <wisp/desktop/gui_internal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#ifndef _WIN32
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#endif

static wisp_ipc_handle *ipc_network = NULL;
static int wisp_network_pid = -1;
static char ipc_dir[256] = "";
static uint32_t next_fetch_id = 1;
static pthread_mutex_t ipc_send_mutex = PTHREAD_MUTEX_INITIALIZER;

static bool create_secure_ipc_path(char *ipc_path_buf, size_t path_len, char *dir_buf, size_t dir_len)
{
    const char *tmpdir = getenv("TMPDIR");
    if (!tmpdir || tmpdir[0] == '\0') {
        tmpdir = "/tmp";
    }
    char template[512];
    snprintf(template, sizeof(template), "%s/wisp-network-XXXXXX", tmpdir);
    char *dir_name = mkdtemp(template);
    if (!dir_name) {
        return false;
    }
    strncpy(dir_buf, dir_name, dir_len - 1);
    dir_buf[dir_len - 1] = '\0';

    snprintf(ipc_path_buf, path_len, "%s/ipc", dir_name);
    return true;
}

struct ipc_fetch_info {
    uint32_t id;
    struct fetch *fetchh;
    bool finished;
    struct ipc_fetch_info *next;
};

static struct ipc_fetch_info *active_fetches = NULL;
static pthread_mutex_t active_fetches_mutex = PTHREAD_MUTEX_INITIALIZER;

static bool fetch_ipc_initialise(lwc_string *scheme) {
    pthread_mutex_lock(&active_fetches_mutex);
    if (!ipc_network) {
        char ipc_name[256];
        if (!create_secure_ipc_path(ipc_name, sizeof(ipc_name), ipc_dir, sizeof(ipc_dir))) {
            pthread_mutex_unlock(&active_fetches_mutex);
            return false;
        }

        wisp_ipc_handle *server = wisp_ipc_create_server(ipc_name);
        if (!server) {
            rmdir(ipc_dir);
            ipc_dir[0] = '\0';
            pthread_mutex_unlock(&active_fetches_mutex);
            return false;
        }

        char exec_path[256];
        if (!wisp_ipc_find_executable("wisp-network", exec_path, sizeof(exec_path))) {
            wisp_ipc_destroy(server);
            unlink(ipc_name);
            rmdir(ipc_dir);
            ipc_dir[0] = '\0';
            pthread_mutex_unlock(&active_fetches_mutex);
            return false;
        }

        wisp_network_pid = wisp_ipc_spawn(exec_path, ipc_name);
        ipc_network = wisp_ipc_accept(server);
        wisp_ipc_destroy(server);
        if (ipc_network) {
            wisp_ipc_set_blocking(ipc_network, false);
        } else {
            unlink(ipc_name);
            rmdir(ipc_dir);
            ipc_dir[0] = '\0';
        }
    }
    bool initialized = (ipc_network != NULL);
    pthread_mutex_unlock(&active_fetches_mutex);
    return initialized;
}

static nserror fetch_ipc_setup(struct fetch *parent_fetch, nsurl *url, bool only_2xx, bool downgrade_tls,
                              const struct fetch_postdata *postdata, const char **headers, void **handle_out) {
    pthread_mutex_lock(&active_fetches_mutex);
    bool check_init = (ipc_network == NULL);
    pthread_mutex_unlock(&active_fetches_mutex);

    if (check_init && !fetch_ipc_initialise(NULL)) return NSERROR_INIT_FAILED;

    struct ipc_fetch_info *f = calloc(1, sizeof(*f));
    if (!f) return NSERROR_NOMEM;
    f->id = next_fetch_id++;
    f->fetchh = parent_fetch;
    pthread_mutex_lock(&active_fetches_mutex);
    f->next = active_fetches;
    active_fetches = f;
    pthread_mutex_unlock(&active_fetches_mutex);

    /* Forward request to network process */
    uint16_t num_headers = 0;
    size_t headers_bytes = 0;
    if (headers != NULL) {
        while (headers[num_headers] != NULL) {
            headers_bytes += 4 + strlen(headers[num_headers]);
            num_headers++;
        }
    }

    wisp_ipc_msg msg;
    msg.type = WISP_IPC_MSG_FETCH_REQUEST;
    const char *url_access = nsurl_access(url);
    uint32_t url_len = strlen(url_access);
    msg.length = 4 + 4 + url_len + 1 + 1 + 2 + headers_bytes;
    msg.data = malloc(msg.length);
    if (!msg.data) {
        pthread_mutex_lock(&active_fetches_mutex);
        if (active_fetches == f) {
            active_fetches = f->next;
        } else {
            struct ipc_fetch_info *curr = active_fetches;
            while (curr && curr->next != f) curr = curr->next;
            if (curr) curr->next = f->next;
        }
        pthread_mutex_unlock(&active_fetches_mutex);
        free(f);
        return NSERROR_NOMEM;
    }

    uint8_t *p = msg.data;
    memcpy(p, &f->id, 4); p += 4;
    memcpy(p, &url_len, 4); p += 4;
    memcpy(p, url_access, url_len); p += url_len;
    *p++ = only_2xx ? 1 : 0;
    *p++ = downgrade_tls ? 1 : 0;
    memcpy(p, &num_headers, 2); p += 2;
    if (num_headers > 0) {
        for (uint16_t i = 0; i < num_headers; i++) {
            uint32_t hlen = strlen(headers[i]);
            memcpy(p, &hlen, 4); p += 4;
            memcpy(p, headers[i], hlen); p += hlen;
        }
    }
    pthread_mutex_lock(&ipc_send_mutex);
    nserror send_err = wisp_ipc_send(ipc_network, &msg);
    pthread_mutex_unlock(&ipc_send_mutex);
    free(msg.data);

    if (send_err != NSERROR_OK) {
        NSLOG(wisp, ERROR, "wisp_ipc_send failed with error %d", send_err);
        pthread_mutex_lock(&active_fetches_mutex);
        if (active_fetches == f) {
            active_fetches = f->next;
        } else {
            struct ipc_fetch_info *curr = active_fetches;
            while (curr && curr->next != f) curr = curr->next;
            if (curr) curr->next = f->next;
        }
        pthread_mutex_unlock(&active_fetches_mutex);
        free(f);
        return send_err;
    }

    NSLOG(wisp, DEBUG, "wisp_ipc_send succeeded for fetch_id %u", f->id);
    *handle_out = f;
    return NSERROR_OK;
}

static bool fetch_ipc_start(void *vf) {
    struct ipc_fetch_info *f = vf;
    /* In this IPC model, setup already started it or we can have a separate START msg */
    return true;
}

static void fetch_ipc_abort(void *vf) {
    if (!vf) return;
    struct ipc_fetch_info *f = vf;
    pthread_mutex_lock(&active_fetches_mutex);
    struct ipc_fetch_info *curr = active_fetches;
    bool valid = false;
    while (curr) {
        if (curr == f) {
            valid = true;
            break;
        }
        curr = curr->next;
    }
    if (!valid) {
        pthread_mutex_unlock(&active_fetches_mutex);
        return;
    }

    if (f->finished) {
        pthread_mutex_unlock(&active_fetches_mutex);
        return;
    }
    f->finished = true;
    pthread_mutex_unlock(&active_fetches_mutex);

    /* Send an ABORT message to network process */
    wisp_ipc_msg msg;
    msg.type = WISP_IPC_MSG_FETCH_ABORT;
    msg.length = 4;
    msg.data = malloc(4);
    if (msg.data) {
        memcpy(msg.data, &f->id, 4);
        pthread_mutex_lock(&ipc_send_mutex);
        if (ipc_network) {
            wisp_ipc_send(ipc_network, &msg);
        }
        pthread_mutex_unlock(&ipc_send_mutex);
        free(msg.data);
    }

    fetch_remove_from_queues(f->fetchh);
    fetch_free(f->fetchh);
}

static void fetch_ipc_free(void *vf) {
    struct ipc_fetch_info *f = vf;
    pthread_mutex_lock(&active_fetches_mutex);
    struct ipc_fetch_info **prev = &active_fetches;
    struct ipc_fetch_info *curr = active_fetches;
    while (curr) {
        if (curr == f) {
            *prev = curr->next;
            free(curr);
            pthread_mutex_unlock(&active_fetches_mutex);
            return;
        }
        prev = &curr->next;
        curr = curr->next;
    }
    pthread_mutex_unlock(&active_fetches_mutex);
}

static bool is_active_fetch_id(uint32_t id) {
    pthread_mutex_lock(&active_fetches_mutex);
    struct ipc_fetch_info *curr = active_fetches;
    while (curr) {
        if (curr->id == id) {
            pthread_mutex_unlock(&active_fetches_mutex);
            return true;
        }
        curr = curr->next;
    }
    pthread_mutex_unlock(&active_fetches_mutex);
    return false;
}

static void fetch_ipc_poll(lwc_string *scheme) {
    pthread_mutex_lock(&active_fetches_mutex);
    wisp_ipc_handle *handle = ipc_network;
    pthread_mutex_unlock(&active_fetches_mutex);

    if (!handle) return;

    wisp_ipc_msg msg;
    nserror err;
    while ((err = wisp_ipc_recv(handle, &msg)) == NSERROR_OK) {
        NSLOG(wisp, DEBUG, "fetch_ipc_poll: Received message of type %d, length %d", msg.type, msg.length);
        if (msg.length < 4) {
            wisp_ipc_msg_free(&msg);
            continue;
        }

        uint32_t fetch_id;
        memcpy(&fetch_id, msg.data, 4);

        /* Read f safely under active_fetches_mutex */
        pthread_mutex_lock(&active_fetches_mutex);
        struct ipc_fetch_info *f = active_fetches;
        while (f && f->id != fetch_id) f = f->next;

        struct fetch *fetchh = f ? f->fetchh : NULL;
        bool finished = f ? f->finished : true;
        pthread_mutex_unlock(&active_fetches_mutex);

        if (fetchh && !finished) {
            fetch_msg fmsg;
            switch (msg.type) {
                case WISP_IPC_MSG_FETCH_HEADER:
                    fmsg.type = FETCH_HEADER;
                    if (msg.length >= 8) {
                        uint32_t http_code;
                        memcpy(&http_code, msg.data + 4, 4);
                        if (http_code > 0) {
                            fetch_set_http_code(fetchh, (long)http_code);
                        }
                        fmsg.data.header_or_data.buf = msg.data + 8;
                        fmsg.data.header_or_data.len = msg.length - 8;
                    }
                    fetch_send_callback(&fmsg, fetchh);
                    break;
                case WISP_IPC_MSG_FETCH_DATA:
                    fmsg.type = FETCH_DATA;
                    fmsg.data.header_or_data.buf = msg.data + 4;
                    fmsg.data.header_or_data.len = msg.length - 4;
                    fetch_send_callback(&fmsg, fetchh);
                    break;
                case WISP_IPC_MSG_FETCH_FINISHED:
                    fmsg.type = FETCH_FINISHED;
                    if (msg.length >= 8) {
                        uint32_t http_code;
                        memcpy(&http_code, msg.data + 4, 4);
                        fetch_set_http_code(fetchh, (long)http_code);
                    }
                    fetch_send_callback(&fmsg, fetchh);
                    pthread_mutex_lock(&active_fetches_mutex);
                    struct ipc_fetch_info *f_post = active_fetches;
                    while (f_post && f_post->id != fetch_id) f_post = f_post->next;
                    if (f_post && !f_post->finished) {
                        f_post->finished = true;
                        struct fetch *fh = f_post->fetchh;
                        pthread_mutex_unlock(&active_fetches_mutex);
                        fetch_remove_from_queues(fh);
                        fetch_free(fh);
                    } else {
                        pthread_mutex_unlock(&active_fetches_mutex);
                    }
                    break;
                case WISP_IPC_MSG_FETCH_REDIRECT:
                    fmsg.type = FETCH_REDIRECT;
                    if (msg.length >= 8) {
                        uint32_t http_code;
                        memcpy(&http_code, msg.data + 4, 4);
                        fetch_set_http_code(fetchh, (long)http_code);
                    }
                    if (msg.length > 8) {
                        char *redir = strndup((char*)msg.data + 8, msg.length - 8);
                        nsurl *redirect_url = NULL;
                        nserror err = nsurl_create(redir ? redir : "", &redirect_url);
                        if (err == NSERROR_OK) {
                            fmsg.data.redirect = redirect_url;
                            fetch_send_callback(&fmsg, fetchh);
                            nsurl_unref(redirect_url);
                        } else {
                            fmsg.type = FETCH_ERROR;
                            fmsg.data.error = "Failed to parse redirect URL";
                            fetch_send_callback(&fmsg, fetchh);
                        }
                        free(redir);
                        break;
                    } else {
                        nsurl *redirect_url = NULL;
                        nserror err = nsurl_create("", &redirect_url);
                        if (err == NSERROR_OK) {
                            fmsg.data.redirect = redirect_url;
                            fetch_send_callback(&fmsg, fetchh);
                            nsurl_unref(redirect_url);
                        } else {
                            fmsg.type = FETCH_ERROR;
                            fmsg.data.error = "Failed to parse redirect URL";
                            fetch_send_callback(&fmsg, fetchh);
                        }
                    }
                    if (is_active_fetch_id(fetch_id)) {
                        pthread_mutex_lock(&active_fetches_mutex);
                        struct ipc_fetch_info *f_post = active_fetches;
                        while (f_post && f_post->id != fetch_id) f_post = f_post->next;
                        if (f_post) {
                            f_post->finished = true;
                            struct fetch *fh = f_post->fetchh;
                            pthread_mutex_unlock(&active_fetches_mutex);
                            fetch_remove_from_queues(fh);
                            fetch_free(fh);
                        } else {
                            pthread_mutex_unlock(&active_fetches_mutex);
                        }
                    }
                    break;
                case WISP_IPC_MSG_FETCH_ERROR:
                    fmsg.type = FETCH_ERROR;
                    if (msg.length > 4) {
                        /* Ensure received error string is safely null-terminated */
                        char *err_str = strndup((char*)msg.data + 4, msg.length - 4);
                        fmsg.data.error = err_str ? err_str : "UnknownError";
                        fetch_send_callback(&fmsg, fetchh);
                        free(err_str);
                    } else {
                        fmsg.data.error = "UnknownError";
                        fetch_send_callback(&fmsg, fetchh);
                    }
                    if (is_active_fetch_id(fetch_id)) {
                        pthread_mutex_lock(&active_fetches_mutex);
                        struct ipc_fetch_info *f_post = active_fetches;
                        while (f_post && f_post->id != fetch_id) f_post = f_post->next;
                        if (f_post) {
                            f_post->finished = true;
                            struct fetch *fh = f_post->fetchh;
                            pthread_mutex_unlock(&active_fetches_mutex);
                            fetch_remove_from_queues(fh);
                            fetch_free(fh);
                        } else {
                            pthread_mutex_unlock(&active_fetches_mutex);
                        }
                    }
                    break;
                default:
                    break;
            }
        }
        wisp_ipc_msg_free(&msg);
    }
    if (err != NSERROR_NOT_FOUND && err != NSERROR_SHUTDOWN) {
        NSLOG(wisp, ERROR, "fetch_ipc_poll: recv returned error %d", err);
    }
}

static int fetch_ipc_fdset(lwc_string *scheme, fd_set *read_fd_set, fd_set *write_fd_set, fd_set *except_fd_set) {
    pthread_mutex_lock(&active_fetches_mutex);
    wisp_ipc_handle *handle = ipc_network;
    pthread_mutex_unlock(&active_fetches_mutex);

    if (!handle) return -1;

    int fd = wisp_ipc_get_fd(handle);
    if (fd < 0) return -1;

    FD_SET(fd, read_fd_set);
    return fd;
}

static bool fetch_ipc_can_fetch(const nsurl *url) {
    return nsurl_has_component(url, NSURL_HOST);
}

static void fetch_ipc_finalise(lwc_string *scheme) {
    if (ipc_network) {
        wisp_ipc_destroy(ipc_network);
        ipc_network = NULL;
    }

    if (ipc_dir[0] != '\0') {
        char ipc_path[512];
        snprintf(ipc_path, sizeof(ipc_path), "%s/ipc", ipc_dir);
        unlink(ipc_path);
        rmdir(ipc_dir);
        ipc_dir[0] = '\0';
    }

    if (wisp_network_pid > 0) {
        bool exited = false;
#ifdef _WIN32
        HANDLE hProcess = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, wisp_network_pid);
        if (hProcess) {
            /* Wait up to 200ms in an event-driven manner */
            DWORD res = WaitForSingleObject(hProcess, 200);
            CloseHandle(hProcess);
            if (res == WAIT_OBJECT_0) {
                exited = true;
            }
        } else {
            exited = true;
        }

        /* If the child process didn't exit cleanly on its own, terminate it forcefully */
        if (!exited) {
            NSLOG(wisp, WARNING, "wisp-network (PID %d) did not terminate in 200ms, sending forceful termination", wisp_network_pid);
            HANDLE hProcessKill = OpenProcess(PROCESS_TERMINATE, FALSE, wisp_network_pid);
            if (hProcessKill) {
                TerminateProcess(hProcessKill, 0);
                CloseHandle(hProcessKill);
            }
        }
#else
        kill(wisp_network_pid, SIGTERM);
        /* Wait up to 200ms for the network process to exit cleanly using adaptive sub-millisecond backoff */
        useconds_t sleep_us = 100;
        useconds_t elapsed_us = 0;
        const useconds_t max_wait_us = 200000; /* 200ms */
        while (elapsed_us < max_wait_us) {
            int status;
            pid_t res = waitpid(wisp_network_pid, &status, WNOHANG);
            if (res == wisp_network_pid || (res == -1 && errno == ECHILD)) {
                exited = true;
                break;
            }
            usleep(sleep_us);
            elapsed_us += sleep_us;
            if (sleep_us < 2000) {
                sleep_us *= 2;
            }
        }

        /* If the child process didn't exit cleanly on its own, terminate it forcefully */
        if (!exited) {
            NSLOG(wisp, WARNING, "wisp-network (PID %d) did not terminate in 200ms, sending forceful termination", wisp_network_pid);
            kill(wisp_network_pid, SIGTERM);
            /* Give it 100ms more to handle SIGTERM, then send SIGKILL if needed */
            useconds_t kill_sleep_us = 100;
            useconds_t kill_elapsed_us = 0;
            const useconds_t kill_max_wait_us = 100000; /* 100ms */
            while (kill_elapsed_us < kill_max_wait_us) {
                int status;
                pid_t res = waitpid(wisp_network_pid, &status, WNOHANG);
                if (res == wisp_network_pid || (res == -1 && errno == ECHILD)) {
                    exited = true;
                    break;
                }
                usleep(kill_sleep_us);
                kill_elapsed_us += kill_sleep_us;
                if (kill_sleep_us < 2000) {
                    kill_sleep_us *= 2;
                }
            }
            if (!exited) {
                kill(wisp_network_pid, SIGKILL);
                waitpid(wisp_network_pid, NULL, 0);
            }
        }
#endif
        wisp_network_pid = -1;
    }

    pthread_mutex_lock(&active_fetches_mutex);
    struct ipc_fetch_info *curr = active_fetches;
    while (curr) {
        struct ipc_fetch_info *next = curr->next;
        free(curr);
        curr = next;
    }
    active_fetches = NULL;
    pthread_mutex_unlock(&active_fetches_mutex);
}

nserror fetch_ipc_register(void) {
    char exec_path[256];
    if (!wisp_ipc_find_executable("wisp-network", exec_path, sizeof(exec_path))) {
        NSLOG(wisp, WARNING, "wisp-network executable not found, skipping IPC fetcher registration");
        return NSERROR_NOT_FOUND;
    }
    static const struct fetcher_operation_table fetcher_ops = {
        .initialise = fetch_ipc_initialise,
        .acceptable = fetch_ipc_can_fetch,
        .setup = fetch_ipc_setup,
        .start = fetch_ipc_start,
        .abort = fetch_ipc_abort,
        .free = fetch_ipc_free,
        .poll = fetch_ipc_poll,
        .fdset = fetch_ipc_fdset,
        .finalise = fetch_ipc_finalise
    };
    fetcher_add(lwc_string_ref(corestring_lwc_http), &fetcher_ops);
    fetcher_add(lwc_string_ref(corestring_lwc_https), &fetcher_ops);
    return NSERROR_OK;
}

void fetch_ipc_early_request(nsurl *url, bool preconnect) {
    if (!url) return;

    pthread_mutex_lock(&active_fetches_mutex);
    bool check_init = (ipc_network == NULL);
    pthread_mutex_unlock(&active_fetches_mutex);

    if (check_init && !fetch_ipc_initialise(NULL)) return;

    wisp_ipc_msg msg;
    msg.type = preconnect ? WISP_IPC_MSG_PRECONNECT_REQUEST : WISP_IPC_MSG_DNS_PREFETCH_REQUEST;
    const char *url_access = nsurl_access(url);
    if (!url_access) return;
    uint32_t url_len = strlen(url_access);
    msg.length = 4 + url_len;
    msg.data = malloc(msg.length);
    if (msg.data) {
        memcpy(msg.data, &url_len, 4);
        memcpy(msg.data + 4, url_access, url_len);
        pthread_mutex_lock(&ipc_send_mutex);
        if (ipc_network) {
            wisp_ipc_send(ipc_network, &msg);
        }
        pthread_mutex_unlock(&ipc_send_mutex);
        free(msg.data);
    }
}
