#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/select.h>
#endif
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
#include <wisp/wisp.h>
#include <wisp/fetch.h>
#include <wisp/utils/stream_simd.h>

extern struct wisp_table *guit;

static wisp_ipc_handle *ipc_main;

struct network_fetch_info {
    uint32_t fetch_id;
    struct fetch *fetchh;
    bool finished;
    struct network_fetch_info *next;
    struct network_fetch_info *hash_next;
};

static struct network_fetch_info *active_fetches_list = NULL;

#define ACTIVE_FETCH_HASH_SIZE 64
static struct network_fetch_info *active_fetches_hash[ACTIVE_FETCH_HASH_SIZE];

static inline unsigned int hash_fetch_info(const struct network_fetch_info *info) {
    uintptr_t ptr = (uintptr_t)info;
    return (unsigned int)((ptr ^ (ptr >> 6)) % ACTIVE_FETCH_HASH_SIZE);
}

static void remove_active_fetch_hash(struct network_fetch_info *info) {
    if (!info) return;
    unsigned int idx = hash_fetch_info(info);
    struct network_fetch_info **curr = &active_fetches_hash[idx];
    while (*curr) {
        if (*curr == info) {
            *curr = info->hash_next;
            break;
        }
        curr = &(*curr)->hash_next;
    }
}

static void send_fetch_error(uint32_t fetch_id, const char *err_msg) {
    if (!err_msg) err_msg = "UnknownError";
    wisp_ipc_msg imsg;
    imsg.type = WISP_IPC_MSG_FETCH_ERROR;
    imsg.length = 4 + strlen(err_msg) + 1;
    uint8_t stack_buf[256];
    if (imsg.length <= sizeof(stack_buf)) {
        imsg.data = stack_buf;
    } else {
        imsg.data = malloc(imsg.length);
    }
    if (imsg.data) {
        memcpy(imsg.data, &fetch_id, 4);
        memcpy((char*)imsg.data + 4, err_msg, strlen(err_msg) + 1);
        wisp_ipc_send(ipc_main, &imsg);
        if (imsg.data != stack_buf) free(imsg.data);
    }
}

static bool is_active_fetch(struct network_fetch_info *info) {
    if (!info) return false;
    unsigned int idx = hash_fetch_info(info);
    struct network_fetch_info *curr = active_fetches_hash[idx];
    while (curr) {
        if (curr == info) {
            return true;
        }
        curr = curr->hash_next;
    }
    /* Fallback scan in case active_fetches_list was assigned directly */
    curr = active_fetches_list;
    while (curr) {
        if (curr == info) {
            return true;
        }
        curr = curr->next;
    }
    return false;
}

static void cleanup_finished_fetches(void) {
    struct network_fetch_info **curr = &active_fetches_list;
    while (*curr != NULL) {
        struct network_fetch_info *entry = *curr;
        if (entry->finished) {
            *curr = entry->next;
            remove_active_fetch_hash(entry);
            free(entry);
        } else {
            curr = &entry->next;
        }
    }
}

static void free_all_active_fetches(void) {
    struct network_fetch_info *curr = active_fetches_list;
    active_fetches_list = NULL;
    memset(active_fetches_hash, 0, sizeof(active_fetches_hash));
    while (curr != NULL) {
        struct network_fetch_info *next = curr->next;
        curr->finished = true;
        if (curr->fetchh) {
            fetch_abort(curr->fetchh);
            curr->fetchh = NULL;
        }
        free(curr);
        curr = next;
    }
}

static void network_process_fetch_callback(const fetch_msg *msg, void *p) {
    wisp_ipc_msg imsg;
    struct network_fetch_info *info = p;

    if (!msg || !is_active_fetch(info) || info->finished) {
        return;
    }

    uint32_t fetch_id = info->fetch_id;
    uint8_t stack_buf[512];

    switch (msg->type) {
        case FETCH_HEADER: {
            if (msg->data.header_or_data.buf && msg->data.header_or_data.len > 0) {
                const char *hdr_str = (const char *)msg->data.header_or_data.buf;
                size_t hdr_len = msg->data.header_or_data.len;
                if (!wisp_simd_validate_http_header_block(hdr_str, hdr_len)) {
                    NSLOG(wisp, WARNING, "WISP-NETWORK: Invalid header received for fetch_id %u, skipping", fetch_id);
                    break;
                }
                /* Note: libcurl (CURLOPT_HTTP_TRANSFER_DECODING = 1L) automatically decodes chunked
                 * transfer encoding in write callbacks before delivering FETCH_DATA.
                 * Secondary stream unchunking is bypassed to prevent double-decoding raw body bytes. */
            }
            imsg.type = WISP_IPC_MSG_FETCH_HEADER;
            imsg.length = 8 + msg->data.header_or_data.len;
            if (imsg.length <= sizeof(stack_buf)) {
                imsg.data = stack_buf;
            } else {
                imsg.data = malloc(imsg.length);
            }
            if (!imsg.data) return;
            memcpy(imsg.data, &fetch_id, 4);
            uint32_t header_http_code = info->fetchh ? (uint32_t)fetch_http_code(info->fetchh) : 0;
            memcpy(imsg.data + 4, &header_http_code, 4);
            if (msg->data.header_or_data.buf && msg->data.header_or_data.len > 0) {
                memcpy(imsg.data + 8, msg->data.header_or_data.buf, msg->data.header_or_data.len);
            }
            wisp_ipc_send(ipc_main, &imsg);
            if (imsg.data != stack_buf) free(imsg.data);
            break;
        }
        case FETCH_NOTMODIFIED: {
            imsg.type = WISP_IPC_MSG_FETCH_FINISHED;
            imsg.length = 8;
            imsg.data = stack_buf;
            memcpy(imsg.data, &fetch_id, 4);
            uint32_t notmod_http_code = info->fetchh ? (uint32_t)fetch_http_code(info->fetchh) : 304;
            if (notmod_http_code == 0) notmod_http_code = 304;
            memcpy(imsg.data + 4, &notmod_http_code, 4);
            wisp_ipc_send(ipc_main, &imsg);
            break;
        }
        case FETCH_DATA: {
            const uint8_t *payload_data = msg->data.header_or_data.buf;
            size_t payload_len = msg->data.header_or_data.len;
            uint8_t *out_data = (uint8_t *)payload_data;
            size_t out_len = payload_len;

            imsg.type = WISP_IPC_MSG_FETCH_DATA;
            imsg.length = 4 + out_len;
            if (imsg.length <= sizeof(stack_buf)) {
                imsg.data = stack_buf;
            } else {
                imsg.data = malloc(imsg.length);
            }
            if (imsg.data) {
                memcpy(imsg.data, &fetch_id, 4);
                if (out_data && out_len > 0) {
                    memcpy(imsg.data + 4, out_data, out_len);
                }
                wisp_ipc_send(ipc_main, &imsg);
                if (imsg.data != stack_buf) free(imsg.data);
            }
            break;
        }
        case FETCH_FINISHED: {
            imsg.type = WISP_IPC_MSG_FETCH_FINISHED;
            imsg.length = 8;
            imsg.data = stack_buf;
            memcpy(imsg.data, &fetch_id, 4);
            uint32_t http_code = info->fetchh ? (uint32_t)fetch_http_code(info->fetchh) : 0;
            memcpy(imsg.data + 4, &http_code, 4);
            wisp_ipc_send(ipc_main, &imsg);
            break;
        }
        case FETCH_REDIRECT: {
            imsg.type = WISP_IPC_MSG_FETCH_REDIRECT;
            const char *redir_target = msg->data.redirect ? nsurl_access(msg->data.redirect) : "";
            imsg.length = 4 + 4 + strlen(redir_target) + 1;
            if (imsg.length <= sizeof(stack_buf)) {
                imsg.data = stack_buf;
            } else {
                imsg.data = malloc(imsg.length);
            }
            if (!imsg.data) return;
            memcpy(imsg.data, &fetch_id, 4);
            uint32_t redirect_http_code = info->fetchh ? (uint32_t)fetch_http_code(info->fetchh) : 302;
            memcpy(imsg.data + 4, &redirect_http_code, 4);
            memcpy((char*)imsg.data + 8, redir_target, strlen(redir_target) + 1);
            wisp_ipc_send(ipc_main, &imsg);
            if (imsg.data != stack_buf) free(imsg.data);
            break;
        }
        case FETCH_ERROR: {
            imsg.type = WISP_IPC_MSG_FETCH_ERROR;
            const char *err_str = msg->data.error ? msg->data.error : "UnknownError";
            imsg.length = 4 + strlen(err_str) + 1;
            if (imsg.length <= sizeof(stack_buf)) {
                imsg.data = stack_buf;
            } else {
                imsg.data = malloc(imsg.length);
            }
            if (!imsg.data) return;
            memcpy(imsg.data, &fetch_id, 4);
            memcpy((char*)imsg.data + 4, err_str, strlen(err_str) + 1);
            wisp_ipc_send(ipc_main, &imsg);
            if (imsg.data != stack_buf) free(imsg.data);
            break;
        }
        case FETCH_TIMEDOUT:
        case FETCH_CERT_ERR:
        case FETCH_SSL_ERR: {
            imsg.type = WISP_IPC_MSG_FETCH_ERROR;
            const char *err_msg = (msg->type == FETCH_TIMEDOUT) ? "Timeout" :
                                  (msg->type == FETCH_SSL_ERR) ? "SSLError" : "CertError";
            imsg.length = 4 + strlen(err_msg) + 1;
            if (imsg.length <= sizeof(stack_buf)) {
                imsg.data = stack_buf;
            } else {
                imsg.data = malloc(imsg.length);
            }
            if (!imsg.data) return;
            memcpy(imsg.data, &fetch_id, 4);
            memcpy((char*)imsg.data + 4, err_msg, strlen(err_msg) + 1);
            wisp_ipc_send(ipc_main, &imsg);
            if (imsg.data != stack_buf) free(imsg.data);
            break;
        }
        default:
            break;
    }

    /* Memory safety note: FETCH_SSL_ERR is the upper bound of terminal fetch message types */
    if (msg->type >= FETCH_FINISHED && msg->type <= FETCH_SSL_ERR) {
        info->finished = true;
        info->fetchh = NULL;
    }
}

static void network_process_ipc_msg(const wisp_ipc_msg *msg) {
    if (!msg) return;

    if (msg->type == WISP_IPC_MSG_FETCH_REQUEST) {
        uint32_t fetch_id = 0;
        uint32_t url_len = 0;
        if (msg->length >= 4) {
            memcpy(&fetch_id, msg->data, 4);
        }
        if (msg->length >= 8) {
            memcpy(&url_len, msg->data + 4, 4);
        }
        if (msg->length < 10 || url_len > msg->length - 10) {
            if (msg->length >= 4) {
                send_fetch_error(fetch_id, "InvalidURL");
            }
        } else {
            char *url_str = malloc(url_len + 1);
            if (!url_str) {
                send_fetch_error(fetch_id, "NoMem");
            } else {
                memcpy(url_str, msg->data + 8, url_len);
                url_str[url_len] = '\0';
                nsurl *url = NULL;
                if (nsurl_create(url_str, &url) == NSERROR_OK && url != NULL) {
                    bool only_2xx = (msg->data[8 + url_len] != 0);
                    bool downgrade_tls = (msg->data[8 + url_len + 1] != 0);

                    /* Parse custom/conditional request headers if present */
                    char **hdr_list = NULL;
                    uint16_t num_headers = 0;
                    if (msg->length >= 8 + url_len + 2 + 2) {
                        memcpy(&num_headers, msg->data + 8 + url_len + 2, 2);
                        if (num_headers > 0) {
                            hdr_list = calloc(num_headers + 1, sizeof(char *));
                            size_t hdr_offset = 8 + url_len + 4;
                            for (uint16_t i = 0; i < num_headers; i++) {
                                if (hdr_offset + 4 > msg->length) break;
                                uint32_t hlen = 0;
                                memcpy(&hlen, msg->data + hdr_offset, 4);
                                hdr_offset += 4;
                                if (hdr_offset + hlen > msg->length) break;
                                if (hdr_list != NULL) {
                                    hdr_list[i] = malloc(hlen + 1);
                                    if (hdr_list[i] != NULL) {
                                        memcpy(hdr_list[i], msg->data + hdr_offset, hlen);
                                        hdr_list[i][hlen] = '\0';
                                    }
                                }
                                hdr_offset += hlen;
                            }
                        }
                    }

                    struct network_fetch_info *info = malloc(sizeof(*info));
                    if (info) {
                        info->fetch_id = fetch_id;
                        info->fetchh = NULL;
                        info->finished = false;
                        info->next = active_fetches_list;
                        info->hash_next = NULL;
                        active_fetches_list = info;
                        unsigned int h_idx = hash_fetch_info(info);
                        info->hash_next = active_fetches_hash[h_idx];
                        active_fetches_hash[h_idx] = info;

                        struct fetch *f_out = NULL;
                        if (fetch_start(url, NULL, network_process_fetch_callback, info,
                                        only_2xx, NULL, true, downgrade_tls, (const char **)hdr_list, &f_out) == NSERROR_OK) {
                            info->fetchh = f_out;
                        } else {
                            /* Unlink info from active_fetches_list before error sending/freeing to prevent use-after-free */
                            struct network_fetch_info **pcurr = &active_fetches_list;
                            while (*pcurr) {
                                if (*pcurr == info) {
                                    *pcurr = info->next;
                                    break;
                                }
                                pcurr = &(*pcurr)->next;
                            }
                            remove_active_fetch_hash(info);
                            send_fetch_error(fetch_id, "Blocked");
                            free(info);
                        }
                    } else {
                        send_fetch_error(fetch_id, "NoMem");
                    }

                    if (hdr_list != NULL) {
                        for (uint16_t i = 0; i < num_headers; i++) {
                            if (hdr_list[i] != NULL) free(hdr_list[i]);
                        }
                        free(hdr_list);
                    }
                    nsurl_unref(url);
                } else {
                    /* Immediately report error to avoid hanging the browser fetcher */
                    send_fetch_error(fetch_id, "InvalidURL");
                }
                free(url_str);
            }
        }
    } else if (msg->type == WISP_IPC_MSG_DNS_PREFETCH_REQUEST) {
        uint32_t url_len;
        if (msg->length >= 4) {
            memcpy(&url_len, msg->data, 4);
            /* Use overflow-safe checks to validate bounds */
            if (url_len <= msg->length - 4) {
                char *url_str = malloc(url_len + 1);
                if (url_str) {
                    memcpy(url_str, msg->data + 4, url_len);
                    url_str[url_len] = '\0';
                    nsurl *url = NULL;
                    if (nsurl_create(url_str, &url) == NSERROR_OK && url != NULL) {
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
    } else if (msg->type == WISP_IPC_MSG_PRECONNECT_REQUEST) {
        uint32_t url_len;
        if (msg->length >= 4) {
            memcpy(&url_len, msg->data, 4);
            /* Use overflow-safe checks to validate bounds */
            if (url_len <= msg->length - 4) {
                char *url_str = malloc(url_len + 1);
                if (url_str) {
                    memcpy(url_str, msg->data + 4, url_len);
                    url_str[url_len] = '\0';
                    nsurl *url = NULL;
                    if (nsurl_create(url_str, &url) == NSERROR_OK && url != NULL) {
                        fetch_curl_preconnect(url_str);
                        nsurl_unref(url);
                    }
                    free(url_str);
                }
            }
        }
    } else if (msg->type == WISP_IPC_MSG_FETCH_ABORT) {
        uint32_t fetch_id;
        if (msg->length >= 4) {
            memcpy(&fetch_id, msg->data, 4);
            struct network_fetch_info *info = active_fetches_list;
            while (info && info->fetch_id != fetch_id) {
                info = info->next;
            }
            if (info && !info->finished) {
                info->finished = true;
                if (info->fetchh) {
                    struct fetch *f = info->fetchh;
                    info->fetchh = NULL;
                    fetch_abort(f);
                }
            }
        }
    }
}

#include <ctype.h>

static const char *default_filetype(const char *unix_path) {
    if (!unix_path) return "text/plain";

    /* Find base path length without query (?) or fragment (#) */
    size_t path_len = strcspn(unix_path, "?#");

    /* Extract dot extension within path_len */
    const char *ext = NULL;
    for (size_t i = path_len; i > 0; i--) {
        char c = unix_path[i - 1];
        if (c == '.') {
            ext = &unix_path[i - 1];
            break;
        }
        if (c == '/' || c == '\\') {
            break;
        }
    }

    if (!ext) return "text/plain";

    size_t ext_len = path_len - (size_t)(ext - unix_path);
    char ext_buf[16];
    if (ext_len == 0 || ext_len >= sizeof(ext_buf)) return "text/plain";

    for (size_t i = 0; i < ext_len; i++) {
        ext_buf[i] = (char)tolower((unsigned char)ext[i]);
    }
    ext_buf[ext_len] = '\0';

    if (strcmp(ext_buf, ".html") == 0 || strcmp(ext_buf, ".htm") == 0) return "text/html";
    if (strcmp(ext_buf, ".xhtml") == 0) return "application/xhtml+xml";
    if (strcmp(ext_buf, ".css") == 0) return "text/css";
    if (strcmp(ext_buf, ".js") == 0 || strcmp(ext_buf, ".mjs") == 0 || strcmp(ext_buf, ".cjs") == 0) return "application/javascript";
    if (strcmp(ext_buf, ".json") == 0 || strcmp(ext_buf, ".map") == 0) return "application/json";
    if (strcmp(ext_buf, ".xml") == 0) return "text/xml";
    if (strcmp(ext_buf, ".svg") == 0) return "image/svg+xml";
    if (strcmp(ext_buf, ".png") == 0) return "image/png";
    if (strcmp(ext_buf, ".jpg") == 0 || strcmp(ext_buf, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext_buf, ".gif") == 0) return "image/gif";
    if (strcmp(ext_buf, ".webp") == 0) return "image/webp";
    if (strcmp(ext_buf, ".avif") == 0) return "image/avif";
    if (strcmp(ext_buf, ".bmp") == 0) return "image/bmp";
    if (strcmp(ext_buf, ".ico") == 0) return "image/x-icon";
    if (strcmp(ext_buf, ".woff") == 0) return "font/woff";
    if (strcmp(ext_buf, ".woff2") == 0) return "font/woff2";
    if (strcmp(ext_buf, ".ttf") == 0) return "font/ttf";
    if (strcmp(ext_buf, ".otf") == 0) return "font/otf";
    if (strcmp(ext_buf, ".wasm") == 0) return "application/wasm";
    if (strcmp(ext_buf, ".pdf") == 0) return "application/pdf";
    if (strcmp(ext_buf, ".mp4") == 0 || strcmp(ext_buf, ".m4v") == 0) return "video/mp4";
    if (strcmp(ext_buf, ".mov") == 0) return "video/quicktime";
    if (strcmp(ext_buf, ".webm") == 0) return "video/webm";
    if (strcmp(ext_buf, ".mkv") == 0) return "video/x-matroska";
    if (strcmp(ext_buf, ".avi") == 0) return "video/x-msvideo";
    if (strcmp(ext_buf, ".ogv") == 0) return "video/ogg";
    if (strcmp(ext_buf, ".mp3") == 0) return "audio/mpeg";
    if (strcmp(ext_buf, ".m4a") == 0) return "audio/mp4";
    if (strcmp(ext_buf, ".wav") == 0) return "audio/wav";
    if (strcmp(ext_buf, ".ogg") == 0) return "audio/ogg";
    if (strcmp(ext_buf, ".opus") == 0) return "audio/opus";
    if (strcmp(ext_buf, ".flac") == 0) return "audio/flac";
    if (strcmp(ext_buf, ".aac") == 0) return "audio/aac";
    if (strcmp(ext_buf, ".txt") == 0) return "text/plain";

    return "text/plain";
}

static char *default_mimetype(const char *path) {
    const char *type = default_filetype(path);
    return type ? strdup(type) : NULL;
}

static struct gui_fetch_table network_fetch_table = {
    .filetype = default_filetype,
    .mimetype = default_mimetype,
};

int main(int argc, char **argv) {
    if (argc < 2) return 1;
    const char *ipc_name = argv[1];

    ipc_main = wisp_ipc_connect(ipc_name);
    if (!ipc_main) return 1;
    wisp_ipc_set_blocking(ipc_main, false);

    extern struct gui_file_table *default_file_table;
    static struct wisp_table network_table;
    network_table.file = default_file_table;
    network_table.fetch = &network_fetch_table;
    wisp_register(&network_table);

    corestrings_init();
    nsoption_init(NULL, NULL, NULL);
    fetch_use_ipc = false;
    fetcher_init();

    NSLOG(wisp, INFO, "WISP-NETWORK: Process started, connecting...");
    while (1) {
        wisp_ipc_msg msg;
        nserror err;
        bool had_work = false;

        while ((err = wisp_ipc_recv(ipc_main, &msg)) == NSERROR_OK) {
            had_work = true;
            NSLOG(wisp, DEBUG, "WISP-NETWORK: Received message of type %d, length %d", msg.type, msg.length);
            network_process_ipc_msg(&msg);
            wisp_ipc_msg_free(&msg);
        }

        if (err != NSERROR_NOT_FOUND) {
            if (err != NSERROR_SHUTDOWN) {
                NSLOG(wisp, ERROR, "WISP-NETWORK: recv returned %d", err);
            }
            break;
        }

        fetch_poll_all();
        cleanup_finished_fetches();

        if (active_fetches_list != NULL) {
            had_work = true;
        }

        if (!had_work) {
#ifdef _WIN32
            Sleep(5);
#else
            usleep(5000);
#endif
        }
    }

    free_all_active_fetches();
    fetcher_quit();
    corestrings_fini();

    if (ipc_main) {
        wisp_ipc_handle *to_destroy = ipc_main;
        ipc_main = NULL;
        wisp_ipc_destroy(to_destroy);
    }
    return 0;
}
