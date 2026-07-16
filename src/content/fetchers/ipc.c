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

static wisp_ipc_handle *ipc_network = NULL;
static uint32_t next_fetch_id = 1;
static pthread_mutex_t ipc_send_mutex = PTHREAD_MUTEX_INITIALIZER;

struct ipc_fetch_info {
    uint32_t id;
    struct fetch *fetchh;
    bool finished;
    struct ipc_fetch_info *next;
};

static struct ipc_fetch_info *active_fetches = NULL;

static bool fetch_ipc_initialise(lwc_string *scheme) {
    if (!ipc_network) {
        char ipc_name[128];
        const char *tmpdir = getenv("TMPDIR");
        if (!tmpdir) tmpdir = "/tmp";
        snprintf(ipc_name, sizeof(ipc_name), "%s/wisp-network-ipc-%d-%u", tmpdir, getpid(), (unsigned int)time(NULL));

        wisp_ipc_handle *server = wisp_ipc_create_server(ipc_name);
        if (!server) return false;

        char exec_path[256];
        if (!wisp_ipc_find_executable("wisp-network", exec_path, sizeof(exec_path))) {
            wisp_ipc_destroy(server);
            return false;
        }

        wisp_ipc_spawn(exec_path, ipc_name);
        ipc_network = wisp_ipc_accept(server);
        wisp_ipc_destroy(server);
        if (ipc_network) {
            wisp_ipc_set_blocking(ipc_network, false);
        }
    }
    return ipc_network != NULL;
}

static void* fetch_ipc_setup(struct fetch *parent_fetch, nsurl *url, bool only_2xx, bool downgrade_tls,
                             const struct fetch_postdata *postdata, const char **headers) {
    if (!ipc_network && !fetch_ipc_initialise(NULL)) return NULL;

    struct ipc_fetch_info *f = calloc(1, sizeof(*f));
    if (!f) return NULL;
    f->id = next_fetch_id++;
    f->fetchh = parent_fetch;
    f->next = active_fetches;
    active_fetches = f;

    /* Forward request to network process */
    wisp_ipc_msg msg;
    msg.type = WISP_IPC_MSG_FETCH_REQUEST;
    const char *url_access = nsurl_access(url);
    uint32_t url_len = strlen(url_access);
    msg.length = 4 + 4 + url_len + 1 + 1;
    msg.data = malloc(msg.length);
    if (msg.data) {
        memcpy(msg.data, &f->id, 4);
        memcpy(msg.data + 4, &url_len, 4);
        memcpy(msg.data + 8, url_access, url_len);
        msg.data[8 + url_len] = only_2xx ? 1 : 0;
        msg.data[8 + url_len + 1] = downgrade_tls ? 1 : 0;
        pthread_mutex_lock(&ipc_send_mutex);
        nserror send_err = wisp_ipc_send(ipc_network, &msg);
        pthread_mutex_unlock(&ipc_send_mutex);
        if (send_err != NSERROR_OK) {
            NSLOG(wisp, ERROR, "wisp_ipc_send failed with error %d", send_err);
        } else {
            NSLOG(wisp, DEBUG, "wisp_ipc_send succeeded for fetch_id %u", f->id);
        }
        free(msg.data);
    }

    return f;
}

static bool fetch_ipc_start(void *vf) {
    struct ipc_fetch_info *f = vf;
    /* In this IPC model, setup already started it or we can have a separate START msg */
    return true;
}

static void fetch_ipc_abort(void *vf) {
    struct ipc_fetch_info *f = vf;
    if (f->finished) return;
    f->finished = true;

    /* Send an ABORT message to network process */
    wisp_ipc_msg msg;
    msg.type = WISP_IPC_MSG_FETCH_ABORT;
    msg.length = 4;
    msg.data = malloc(4);
    if (msg.data) {
        memcpy(msg.data, &f->id, 4);
        pthread_mutex_lock(&ipc_send_mutex);
        wisp_ipc_send(ipc_network, &msg);
        pthread_mutex_unlock(&ipc_send_mutex);
        free(msg.data);
    }

    fetch_remove_from_queues(f->fetchh);
    fetch_free(f->fetchh);
}

static void fetch_ipc_free(void *vf) {
    struct ipc_fetch_info *f = vf;
    struct ipc_fetch_info **prev = &active_fetches;
    struct ipc_fetch_info *curr = active_fetches;
    while (curr) {
        if (curr == f) {
            *prev = curr->next;
            free(curr);
            return;
        }
        prev = &curr->next;
        curr = curr->next;
    }
}

static bool is_active_fetch(struct ipc_fetch_info *f) {
    struct ipc_fetch_info *curr = active_fetches;
    while (curr) {
        if (curr == f) return true;
        curr = curr->next;
    }
    return false;
}

static void fetch_ipc_poll(lwc_string *scheme) {
    if (!ipc_network) return;

    wisp_ipc_msg msg;
    nserror err;
    while ((err = wisp_ipc_recv(ipc_network, &msg)) == NSERROR_OK) {
        NSLOG(wisp, DEBUG, "fetch_ipc_poll: Received message of type %d, length %d", msg.type, msg.length);
        if (msg.length < 4) {
            wisp_ipc_msg_free(&msg);
            continue;
        }

        uint32_t fetch_id;
        memcpy(&fetch_id, msg.data, 4);

        struct ipc_fetch_info *f = active_fetches;
        while (f && f->id != fetch_id) f = f->next;

        if (f && !f->finished) {
            fetch_msg fmsg;
            switch (msg.type) {
                case WISP_IPC_MSG_FETCH_HEADER:
                    fmsg.type = FETCH_HEADER;
                    fmsg.data.header_or_data.buf = msg.data + 4;
                    fmsg.data.header_or_data.len = msg.length - 4;
                    fetch_send_callback(&fmsg, f->fetchh);
                    break;
                case WISP_IPC_MSG_FETCH_DATA:
                    fmsg.type = FETCH_DATA;
                    fmsg.data.header_or_data.buf = msg.data + 4;
                    fmsg.data.header_or_data.len = msg.length - 4;
                    fetch_send_callback(&fmsg, f->fetchh);
                    break;
                case WISP_IPC_MSG_FETCH_FINISHED:
                    fmsg.type = FETCH_FINISHED;
                    if (msg.length >= 8) {
                        uint32_t http_code;
                        memcpy(&http_code, msg.data + 4, 4);
                        fetch_set_http_code(f->fetchh, (long)http_code);
                    }
                    fetch_send_callback(&fmsg, f->fetchh);
                    if (is_active_fetch(f)) {
                        f->finished = true;
                        fetch_remove_from_queues(f->fetchh);
                        fetch_free(f->fetchh);
                    }
                    break;
                case WISP_IPC_MSG_FETCH_REDIRECT:
                    fmsg.type = FETCH_REDIRECT;
                    if (msg.length >= 8) {
                        uint32_t http_code;
                        memcpy(&http_code, msg.data + 4, 4);
                        fetch_set_http_code(f->fetchh, (long)http_code);
                    }
                    if (msg.length > 8) {
                        msg.data[msg.length - 1] = '\0';
                        fmsg.data.redirect = (char*)msg.data + 8;
                    } else {
                        fmsg.data.redirect = "";
                    }
                    fetch_send_callback(&fmsg, f->fetchh);
                    if (is_active_fetch(f)) {
                        f->finished = true;
                        fetch_remove_from_queues(f->fetchh);
                        fetch_free(f->fetchh);
                    }
                    break;
                case WISP_IPC_MSG_FETCH_ERROR:
                    fmsg.type = FETCH_ERROR;
                    if (msg.length > 4) {
                        /* Ensure received error string is safely null-terminated */
                        msg.data[msg.length - 1] = '\0';
                        fmsg.data.error = (char*)msg.data + 4;
                    } else {
                        fmsg.data.error = "UnknownError";
                    }
                    fetch_send_callback(&fmsg, f->fetchh);
                    if (is_active_fetch(f)) {
                        f->finished = true;
                        fetch_remove_from_queues(f->fetchh);
                        fetch_free(f->fetchh);
                    }
                    break;
                default:
                    break;
            }
        }
        wisp_ipc_msg_free(&msg);
    }
    if (err != NSERROR_NOT_FOUND) {
        NSLOG(wisp, ERROR, "fetch_ipc_poll: recv returned error %d", err);
    }
}

static bool fetch_ipc_can_fetch(const nsurl *url) {
    return nsurl_has_component(url, NSURL_HOST);
}

static void fetch_ipc_finalise(lwc_string *scheme) {
    if (ipc_network) {
        wisp_ipc_destroy(ipc_network);
        ipc_network = NULL;
    }
    struct ipc_fetch_info *curr = active_fetches;
    while (curr) {
        struct ipc_fetch_info *next = curr->next;
        free(curr);
        curr = next;
    }
    active_fetches = NULL;
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
        .finalise = fetch_ipc_finalise
    };
    fetcher_add(lwc_string_ref(corestring_lwc_http), &fetcher_ops);
    fetcher_add(lwc_string_ref(corestring_lwc_https), &fetcher_ops);
    return NSERROR_OK;
}

void fetch_ipc_early_request(nsurl *url, bool preconnect) {
    if (!url) return;
    if (!ipc_network && !fetch_ipc_initialise(NULL)) return;

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
        wisp_ipc_send(ipc_network, &msg);
        pthread_mutex_unlock(&ipc_send_mutex);
        free(msg.data);
    }
}
