#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <wisp/utils/ipc.h>
#include <wisp/utils/log.h>
#include <wisp/content/fetch.h>
#include "content/fetchers.h"
#include <wisp/utils/nsoption.h>
#include <wisp/utils/corestrings.h>
#include <wisp/utils/messages.h>
#include "content/fetchers/curl.h"
#include <libwapcaplet/libwapcaplet.h>
#include <wisp/utils/nsurl.h>
#include <wisp/desktop/gui_table.h>

struct wisp_table *guit;

static wisp_ipc_handle *ipc_main;

static void network_process_fetch_callback(const fetch_msg *msg, void *p) {
    wisp_ipc_msg imsg;
    uint32_t fetch_id = (uint32_t)(uintptr_t)p;

    switch (msg->type) {
        case FETCH_HEADER:
            imsg.type = WISP_IPC_MSG_FETCH_HEADER;
            imsg.length = 4 + msg->data.header_or_data.len;
            imsg.data = malloc(imsg.length);
            if (!imsg.data) return;
            memcpy(imsg.data, &fetch_id, 4);
            memcpy(imsg.data + 4, msg->data.header_or_data.buf, msg->data.header_or_data.len);
            wisp_ipc_send(ipc_main, &imsg);
            free(imsg.data);
            break;
        case FETCH_DATA:
            imsg.type = WISP_IPC_MSG_FETCH_DATA;
            imsg.length = 4 + msg->data.header_or_data.len;
            imsg.data = malloc(imsg.length);
            if (!imsg.data) return;
            memcpy(imsg.data, &fetch_id, 4);
            memcpy(imsg.data + 4, msg->data.header_or_data.buf, msg->data.header_or_data.len);
            wisp_ipc_send(ipc_main, &imsg);
            free(imsg.data);
            break;
        case FETCH_FINISHED:
            imsg.type = WISP_IPC_MSG_FETCH_FINISHED;
            imsg.length = 4;
            imsg.data = malloc(4);
            if (!imsg.data) return;
            memcpy(imsg.data, &fetch_id, 4);
            wisp_ipc_send(ipc_main, &imsg);
            free(imsg.data);
            break;
        case FETCH_ERROR:
            imsg.type = WISP_IPC_MSG_FETCH_ERROR;
            imsg.length = 4 + strlen(msg->data.error) + 1;
            imsg.data = malloc(imsg.length);
            if (!imsg.data) return;
            memcpy(imsg.data, &fetch_id, 4);
            memcpy((char*)imsg.data + 4, msg->data.error, imsg.length - 4);
            wisp_ipc_send(ipc_main, &imsg);
            free(imsg.data);
            break;
        default:
            break;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) return 1;
    const char *ipc_name = argv[1];

    ipc_main = wisp_ipc_connect(ipc_name);
    if (!ipc_main) return 1;
    wisp_ipc_set_blocking(ipc_main, false);

    corestrings_init();
    nsoption_init(NULL, NULL, NULL);
    fetcher_init();
    fetch_curl_register();

    while (1) {
        struct timeval tv = {0, 10000}; // 10ms

        wisp_ipc_msg msg;
        nserror err = wisp_ipc_recv(ipc_main, &msg);

        if (err == NSERROR_OK) {
            if (msg.type == WISP_IPC_MSG_FETCH_REQUEST) {
                uint32_t fetch_id;
                uint32_t url_len;
                if (msg.length >= 8) {
                    memcpy(&fetch_id, msg.data, 4);
                    memcpy(&url_len, msg.data + 4, 4);
                    if (msg.length >= 8 + url_len + 2) {
                        char *url_str = malloc(url_len + 1);
                        if (url_str) {
                            memcpy(url_str, msg.data + 8, url_len);
                            url_str[url_len] = '\0';
                            nsurl *url;
                            nsurl_create(url_str, &url);
                            bool only_2xx = (msg.data[8 + url_len] != 0);
                            bool downgrade_tls = (msg.data[8 + url_len + 1] != 0);
                            struct fetch *f_out;
                            fetch_start(url, NULL, network_process_fetch_callback, (void*)(uintptr_t)fetch_id,
                                        only_2xx, NULL, true, downgrade_tls, NULL, &f_out);
                            nsurl_unref(url);
                            free(url_str);
                        }
                    }
                }
            } else if (msg.type == WISP_IPC_MSG_DNS_PREFETCH_REQUEST) {
                uint32_t url_len;
                if (msg.length >= 4) {
                    memcpy(&url_len, msg.data, 4);
                    if (msg.length >= 4 + url_len) {
                        char *url_str = malloc(url_len + 1);
                        if (url_str) {
                            memcpy(url_str, msg.data + 4, url_len);
                            url_str[url_len] = '\0';
                            nsurl *url;
                            if (nsurl_create(url_str, &url) == NSERROR_OK) {
                                lwc_string *host_lwc = nsurl_get_component(url, NSURL_HOST);
                                if (host_lwc) {
                                    fetch_curl_dns_prefetch(lwc_string_data(host_lwc));
                                    lwc_string_unref(host_lwc);
                                }
                                nsurl_unref(url);
                            }
                            free(url_str);
                        }
                    }
                }
            } else if (msg.type == WISP_IPC_MSG_PRECONNECT_REQUEST) {
                uint32_t url_len;
                if (msg.length >= 4) {
                    memcpy(&url_len, msg.data, 4);
                    if (msg.length >= 4 + url_len) {
                        char *url_str = malloc(url_len + 1);
                        if (url_str) {
                            memcpy(url_str, msg.data + 4, url_len);
                            url_str[url_len] = '\0';
                            nsurl *url;
                            if (nsurl_create(url_str, &url) == NSERROR_OK) {
                                fetch_curl_preconnect(url_str);
                                nsurl_unref(url);
                            }
                            free(url_str);
                        }
                    }
                }
            }
            wisp_ipc_msg_free(&msg);
        } else if (err != NSERROR_NOT_FOUND) {
            break;
        }

        fetch_poll_all();
        select(0, NULL, NULL, NULL, &tv);
    }

    wisp_ipc_destroy(ipc_main);
    return 0;
}
