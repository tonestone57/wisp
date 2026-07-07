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

static wisp_ipc_handle *ipc_network = NULL;
static uint32_t next_fetch_id = 1;

struct ipc_fetch_info {
    uint32_t id;
    fetch_callback callback;
    void *callback_pw;
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
        /* In a real app we'd use something like procfs to find our own path,
           but here we assume it's in the same dir as the main executable. */
        snprintf(exec_path, sizeof(exec_path), "./wisp-network");

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
        wisp_ipc_send(ipc_network, &msg);
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
    f->finished = true;
    /* We should send an ABORT message to network process too */
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

static void fetch_ipc_poll(lwc_string *scheme) {
    if (!ipc_network) return;

    wisp_ipc_msg msg;
    while (wisp_ipc_recv(ipc_network, &msg) == NSERROR_OK) {
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
                    f->callback(&fmsg, f->callback_pw);
                    break;
                case WISP_IPC_MSG_FETCH_DATA:
                    fmsg.type = FETCH_DATA;
                    fmsg.data.header_or_data.buf = msg.data + 4;
                    fmsg.data.header_or_data.len = msg.length - 4;
                    f->callback(&fmsg, f->callback_pw);
                    break;
                case WISP_IPC_MSG_FETCH_FINISHED:
                    fmsg.type = FETCH_FINISHED;
                    f->callback(&fmsg, f->callback_pw);
                    f->finished = true;
                    break;
                case WISP_IPC_MSG_FETCH_ERROR:
                    fmsg.type = FETCH_ERROR;
                    fmsg.data.error = (char*)msg.data + 4;
                    f->callback(&fmsg, f->callback_pw);
                    f->finished = true;
                    break;
                default:
                    break;
            }
        }
        wisp_ipc_msg_free(&msg);
    }
}

static bool fetch_ipc_can_fetch(const nsurl *url) {
    return nsurl_has_component(url, NSURL_HOST);
}

nserror fetch_ipc_register(void) {
    static const struct fetcher_operation_table fetcher_ops = {
        .initialise = fetch_ipc_initialise,
        .acceptable = fetch_ipc_can_fetch,
        .setup = fetch_ipc_setup,
        .start = fetch_ipc_start,
        .abort = fetch_ipc_abort,
        .free = fetch_ipc_free,
        .poll = fetch_ipc_poll,
        .finalise = NULL
    };
    fetcher_add(lwc_string_ref(corestring_lwc_http), &fetcher_ops);
    fetcher_add(lwc_string_ref(corestring_lwc_https), &fetcher_ops);
    return NSERROR_OK;
}
