#include <check.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "wisp/utils/ipc.h"
#include "wisp/utils/nsurl.h"

#define main network_process_main
#include "../processes/network/main.c"
#undef main

START_TEST(test_default_mimetype)
{
    char *mt = default_mimetype("style.css");
    ck_assert_ptr_nonnull(mt);
    ck_assert_str_eq(mt, "text/css");
    free(mt);

    mt = default_mimetype(NULL);
    ck_assert_ptr_nonnull(mt);
    ck_assert_str_eq(mt, "text/plain");
    free(mt);
}
END_TEST

START_TEST(test_default_filetype)
{
    ck_assert_str_eq(default_filetype(NULL), "text/plain");
    ck_assert_str_eq(default_filetype("style.css"), "text/css");
    ck_assert_str_eq(default_filetype("page.html"), "text/html");
    ck_assert_str_eq(default_filetype("page.htm"), "text/html");
    ck_assert_str_eq(default_filetype("doc.xhtml"), "application/xhtml+xml");
    ck_assert_str_eq(default_filetype("script.js"), "application/javascript");
    ck_assert_str_eq(default_filetype("module.mjs"), "application/javascript");
    ck_assert_str_eq(default_filetype("common.cjs"), "application/javascript");
    ck_assert_str_eq(default_filetype("data.json"), "application/json");
    ck_assert_str_eq(default_filetype("bundle.js.map"), "application/json");
    ck_assert_str_eq(default_filetype("feed.xml"), "text/xml");
    ck_assert_str_eq(default_filetype("icon.svg"), "image/svg+xml");
    ck_assert_str_eq(default_filetype("image.png"), "image/png");
    ck_assert_str_eq(default_filetype("photo.jpg"), "image/jpeg");
    ck_assert_str_eq(default_filetype("photo.jpeg"), "image/jpeg");
    ck_assert_str_eq(default_filetype("anim.gif"), "image/gif");
    ck_assert_str_eq(default_filetype("pic.webp"), "image/webp");
    ck_assert_str_eq(default_filetype("pic.avif"), "image/avif");
    ck_assert_str_eq(default_filetype("bitmap.bmp"), "image/bmp");
    ck_assert_str_eq(default_filetype("favicon.ico"), "image/x-icon");
    ck_assert_str_eq(default_filetype("font.woff"), "font/woff");
    ck_assert_str_eq(default_filetype("font.woff2"), "font/woff2");
    ck_assert_str_eq(default_filetype("font.ttf"), "font/ttf");
    ck_assert_str_eq(default_filetype("font.otf"), "font/otf");
    ck_assert_str_eq(default_filetype("track.aac"), "audio/aac");
    ck_assert_str_eq(default_filetype("video.mp4"), "video/mp4");
    ck_assert_str_eq(default_filetype("video.m4v"), "video/mp4");
    ck_assert_str_eq(default_filetype("video.mov"), "video/quicktime");
    ck_assert_str_eq(default_filetype("video.webm"), "video/webm");
    ck_assert_str_eq(default_filetype("video.mkv"), "video/x-matroska");
    ck_assert_str_eq(default_filetype("video.avi"), "video/x-msvideo");
    ck_assert_str_eq(default_filetype("video.ogv"), "video/ogg");
    ck_assert_str_eq(default_filetype("audio.ogg"), "audio/ogg");
    ck_assert_str_eq(default_filetype("audio.mp3"), "audio/mpeg");
    ck_assert_str_eq(default_filetype("audio.m4a"), "audio/mp4");
    ck_assert_str_eq(default_filetype("audio.wav"), "audio/wav");
    ck_assert_str_eq(default_filetype("audio.opus"), "audio/opus");
    ck_assert_str_eq(default_filetype("audio.flac"), "audio/flac");
    ck_assert_str_eq(default_filetype("module.wasm"), "application/wasm");
    ck_assert_str_eq(default_filetype("document.pdf"), "application/pdf");
    ck_assert_str_eq(default_filetype("document.txt"), "text/plain");
    ck_assert_str_eq(default_filetype("unknown.xyz"), "text/plain");
    ck_assert_str_eq(default_filetype("/path/to/style.CSS?query=1"), "text/css");
    ck_assert_str_eq(default_filetype("/images/logo.PNG#fragment"), "image/png");
    ck_assert_str_eq(default_filetype("/assets/v1/font.woff2?v=2#font"), "font/woff2");
    ck_assert_str_eq(default_filetype("site.v2.html"), "text/html");
    ck_assert_str_eq(default_filetype("bundle.min.js"), "application/javascript");
    ck_assert_str_eq(default_filetype("no_extension"), "text/plain");
    ck_assert_str_eq(default_filetype("/path.with.dots/filename"), "text/plain");
    ck_assert_str_eq(default_filetype("page.html?a=1#fragment"), "text/html");
    ck_assert_str_eq(default_filetype("page.html#fragment?a=1"), "text/html");
    ck_assert_str_eq(default_filetype("folder.with.dot/"), "text/plain");
    ck_assert_str_eq(default_filetype("style.css?query=val#hash"), "text/css");
}
END_TEST

START_TEST(test_is_active_fetch)
{
    struct network_fetch_info f1 = { .fetch_id = 1, .finished = false, .next = NULL, .hash_next = NULL };
    struct network_fetch_info f2 = { .fetch_id = 2, .finished = false, .next = NULL, .hash_next = NULL };
    struct network_fetch_info f3 = { .fetch_id = 3, .finished = false, .next = NULL, .hash_next = NULL };

    active_fetches_list = NULL;
    memset(active_fetches_hash, 0, sizeof(active_fetches_hash));
    ck_assert_int_eq(is_active_fetch(&f1), false);

    /* Test hash table lookup */
    unsigned int idx1 = hash_fetch_info(&f1);
    f1.hash_next = active_fetches_hash[idx1];
    active_fetches_hash[idx1] = &f1;

    ck_assert_int_eq(is_active_fetch(&f1), true);

    /* Test fallback list scan lookup */
    active_fetches_list = &f2;
    f2.next = &f3;

    ck_assert_int_eq(is_active_fetch(&f2), true);
    ck_assert_int_eq(is_active_fetch(&f3), true);

    struct network_fetch_info unlisted = { .fetch_id = 99, .finished = false, .next = NULL, .hash_next = NULL };
    ck_assert_int_eq(is_active_fetch(&unlisted), false);

    active_fetches_list = NULL;
    memset(active_fetches_hash, 0, sizeof(active_fetches_hash));
}
END_TEST

START_TEST(test_cleanup_finished_fetches)
{
    active_fetches_list = NULL;
    cleanup_finished_fetches();
    ck_assert_ptr_null(active_fetches_list);

    struct network_fetch_info *n1 = malloc(sizeof(*n1));
    struct network_fetch_info *n2 = malloc(sizeof(*n2));
    struct network_fetch_info *n3 = malloc(sizeof(*n3));
    struct network_fetch_info *n4 = malloc(sizeof(*n4));

    ck_assert_ptr_nonnull(n1);
    ck_assert_ptr_nonnull(n2);
    ck_assert_ptr_nonnull(n3);
    ck_assert_ptr_nonnull(n4);

    n1->fetch_id = 1; n1->finished = true;
    n2->fetch_id = 2; n2->finished = false;
    n3->fetch_id = 3; n3->finished = true;
    n4->fetch_id = 4; n4->finished = false;

    /* Link list: n1 (finished) -> n2 (active) -> n3 (finished) -> n4 (active) -> NULL */
    n1->next = n2;
    n2->next = n3;
    n3->next = n4;
    n4->next = NULL;
    active_fetches_list = n1;

    cleanup_finished_fetches();

    /* Head (n1) and middle (n3) should be freed and removed */
    ck_assert_ptr_eq(active_fetches_list, n2);
    ck_assert_ptr_eq(n2->next, n4);
    ck_assert_ptr_null(n4->next);

    /* Mark tail n4 as finished and cleanup */
    n4->finished = true;
    cleanup_finished_fetches();

    ck_assert_ptr_eq(active_fetches_list, n2);
    ck_assert_ptr_null(n2->next);

    /* Clean up remaining active node */
    free(n2);
    active_fetches_list = NULL;

    /* Additional edge case: All nodes in list are finished */
    struct network_fetch_info *a1 = malloc(sizeof(*a1));
    struct network_fetch_info *a2 = malloc(sizeof(*a2));
    ck_assert_ptr_nonnull(a1);
    ck_assert_ptr_nonnull(a2);

    a1->fetch_id = 10; a1->finished = true; a1->next = a2;
    a2->fetch_id = 20; a2->finished = true; a2->next = NULL;
    active_fetches_list = a1;

    cleanup_finished_fetches();
    ck_assert_ptr_null(active_fetches_list);
}
END_TEST

START_TEST(test_free_all_active_fetches)
{
    active_fetches_list = NULL;
    free_all_active_fetches();
    ck_assert_ptr_null(active_fetches_list);

    struct network_fetch_info *f1 = malloc(sizeof(*f1));
    struct network_fetch_info *f2 = malloc(sizeof(*f2));
    ck_assert_ptr_nonnull(f1);
    ck_assert_ptr_nonnull(f2);

    f1->fetch_id = 100;
    f1->fetchh = NULL;
    f1->finished = false;
    f1->next = f2;

    f2->fetch_id = 200;
    f2->fetchh = NULL;
    f2->finished = true;
    f2->next = NULL;

    active_fetches_list = f1;

    free_all_active_fetches();
    ck_assert_ptr_null(active_fetches_list);
}
END_TEST

static wisp_ipc_handle *test_ipc_server = NULL;
static wisp_ipc_handle *test_ipc_accepted = NULL;

static void setup_ipc(void)
{
    const char *sock_name = "test_net_main_cb_sock";
    test_ipc_server = wisp_ipc_create_server(sock_name);
    ck_assert_ptr_nonnull(test_ipc_server);

    ipc_main = wisp_ipc_connect(sock_name);
    ck_assert_ptr_nonnull(ipc_main);

    test_ipc_accepted = wisp_ipc_accept(test_ipc_server);
    ck_assert_ptr_nonnull(test_ipc_accepted);

    wisp_ipc_set_blocking(test_ipc_accepted, false);
}

static void teardown_ipc(void)
{
    if (ipc_main) {
        wisp_ipc_destroy(ipc_main);
        ipc_main = NULL;
    }
    if (test_ipc_accepted) {
        wisp_ipc_destroy(test_ipc_accepted);
        test_ipc_accepted = NULL;
    }
    if (test_ipc_server) {
        wisp_ipc_destroy(test_ipc_server);
        test_ipc_server = NULL;
    }
}

START_TEST(test_send_fetch_error)
{
    setup_ipc();

    /* Test standard error message */
    send_fetch_error(123, "NetworkFailed");

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_FETCH_ERROR);
    ck_assert_int_eq(recv_msg.length, 4 + strlen("NetworkFailed") + 1);

    uint32_t recv_fid;
    memcpy(&recv_fid, recv_msg.data, 4);
    ck_assert_int_eq(recv_fid, 123);
    ck_assert_str_eq((const char *)recv_msg.data + 4, "NetworkFailed");

    wisp_ipc_msg_free(&recv_msg);

    /* Test NULL error message fallback to "UnknownError" */
    send_fetch_error(124, NULL);

    err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_FETCH_ERROR);
    ck_assert_int_eq(recv_msg.length, 4 + strlen("UnknownError") + 1);

    memcpy(&recv_fid, recv_msg.data, 4);
    ck_assert_int_eq(recv_fid, 124);
    ck_assert_str_eq((const char *)recv_msg.data + 4, "UnknownError");

    wisp_ipc_msg_free(&recv_msg);
    teardown_ipc();
}
END_TEST

START_TEST(test_send_fetch_error_handling)
{
    setup_ipc();

    send_fetch_error(500, "Blocked");

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_FETCH_ERROR);

    uint32_t recv_fid;
    memcpy(&recv_fid, recv_msg.data, 4);
    ck_assert_int_eq(recv_fid, 500);
    ck_assert_str_eq((const char *)recv_msg.data + 4, "Blocked");

    wisp_ipc_msg_free(&recv_msg);
    teardown_ipc();
}
END_TEST

static void process_one_ipc_msg(void) {
    wisp_ipc_msg msg;
    nserror err = wisp_ipc_recv(ipc_main, &msg);
    if (err == NSERROR_OK) {
        network_process_ipc_msg(&msg);
        wisp_ipc_msg_free(&msg);
    }
}

START_TEST(test_fetch_request_invalid_url_and_bounds)
{
    setup_ipc();

    /* 1. Short message (< 10 bytes but >= 4 bytes, e.g. 6 bytes): sends InvalidURL for fetch_id */
    uint32_t fetch_id = 88;
    wisp_ipc_msg msg;
    msg.type = WISP_IPC_MSG_FETCH_REQUEST;
    msg.length = 6;
    msg.data = malloc(msg.length);
    ck_assert_ptr_nonnull(msg.data);
    memcpy(msg.data, &fetch_id, 4);

    nserror send_res = wisp_ipc_send(test_ipc_accepted, &msg);
    ck_assert_int_eq(send_res, NSERROR_OK);
    free(msg.data);

    /* Process the message via actual IPC handling routine */
    process_one_ipc_msg();

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_FETCH_ERROR);
    uint32_t recv_fid;
    memcpy(&recv_fid, recv_msg.data, 4);
    ck_assert_int_eq(recv_fid, 88);
    ck_assert_str_eq((const char *)recv_msg.data + 4, "InvalidURL");
    wisp_ipc_msg_free(&recv_msg);

    /* 2. Excess url_len (> msg.length - 10): sends InvalidURL */
    fetch_id = 89;
    uint32_t url_len = 50; /* claims 50 bytes, but msg.length is only 15 */
    msg.length = 15;
    msg.data = malloc(msg.length);
    ck_assert_ptr_nonnull(msg.data);
    memcpy(msg.data, &fetch_id, 4);
    memcpy(msg.data + 4, &url_len, 4);

    send_res = wisp_ipc_send(test_ipc_accepted, &msg);
    ck_assert_int_eq(send_res, NSERROR_OK);
    free(msg.data);

    process_one_ipc_msg();

    err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_FETCH_ERROR);
    memcpy(&recv_fid, recv_msg.data, 4);
    ck_assert_int_eq(recv_fid, 89);
    ck_assert_str_eq((const char *)recv_msg.data + 4, "InvalidURL");
    wisp_ipc_msg_free(&recv_msg);

    teardown_ipc();
}
END_TEST

START_TEST(test_fetch_callback_http_codes)
{
    setup_ipc();

    struct network_fetch_info info = {
        .fetch_id = 77,
        .fetchh = NULL,
        .finished = false,
        .next = NULL
    };
    active_fetches_list = &info;

    /* FETCH_HEADER with NULL fetchh (returns http code 0) */
    fetch_msg msg;
    msg.type = FETCH_HEADER;
    const char *hdr = "HTTP/1.1 200 OK\r\n";
    msg.data.header_or_data.buf = (const uint8_t *)hdr;
    msg.data.header_or_data.len = strlen(hdr);

    network_process_fetch_callback(&msg, &info);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_FETCH_HEADER);

    uint32_t recv_fid, recv_code;
    memcpy(&recv_fid, recv_msg.data, 4);
    memcpy(&recv_code, recv_msg.data + 4, 4);
    ck_assert_int_eq(recv_fid, 77);
    ck_assert_int_eq(recv_code, 0);

    wisp_ipc_msg_free(&recv_msg);

    /* FETCH_NOTMODIFIED with NULL fetchh (returns http code 304 fallback) */
    info.finished = false;
    msg.type = FETCH_NOTMODIFIED;

    network_process_fetch_callback(&msg, &info);

    err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_FETCH_FINISHED);

    memcpy(&recv_fid, recv_msg.data, 4);
    memcpy(&recv_code, recv_msg.data + 4, 4);
    ck_assert_int_eq(recv_fid, 77);
    ck_assert_int_eq(recv_code, 304);

    wisp_ipc_msg_free(&recv_msg);
    active_fetches_list = NULL;
    teardown_ipc();
}
END_TEST

START_TEST(test_fetch_callback_header)
{
    setup_ipc();

    struct network_fetch_info info = {
        .fetch_id = 42,
        .fetchh = NULL,
        .finished = false,
        .next = NULL
    };
    active_fetches_list = &info;

    fetch_msg msg;
    msg.type = FETCH_HEADER;
    const char *hdr = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    msg.data.header_or_data.buf = (const uint8_t *)hdr;
    msg.data.header_or_data.len = strlen(hdr);

    network_process_fetch_callback(&msg, &info);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_FETCH_HEADER);
    ck_assert_int_eq(recv_msg.length, 8 + strlen(hdr));

    uint32_t recv_fid, recv_code;
    memcpy(&recv_fid, recv_msg.data, 4);
    memcpy(&recv_code, recv_msg.data + 4, 4);
    ck_assert_int_eq(recv_fid, 42);
    ck_assert_int_eq(recv_code, 0);
    ck_assert_mem_eq(recv_msg.data + 8, hdr, strlen(hdr));

    ck_assert_int_eq(info.finished, false);

    wisp_ipc_msg_free(&recv_msg);
    active_fetches_list = NULL;
    teardown_ipc();
}
END_TEST

START_TEST(test_fetch_callback_notmodified)
{
    setup_ipc();

    struct network_fetch_info info = {
        .fetch_id = 444,
        .fetchh = NULL,
        .finished = false,
        .next = NULL
    };
    active_fetches_list = &info;

    fetch_msg msg;
    msg.type = FETCH_NOTMODIFIED;

    network_process_fetch_callback(&msg, &info);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_FETCH_FINISHED);
    ck_assert_int_eq(recv_msg.length, 8);

    uint32_t recv_fid, recv_code;
    memcpy(&recv_fid, recv_msg.data, 4);
    memcpy(&recv_code, recv_msg.data + 4, 4);
    ck_assert_int_eq(recv_fid, 444);
    ck_assert_int_eq(recv_code, 304);

    ck_assert_int_eq(info.finished, true);
    ck_assert_ptr_null(info.fetchh);

    wisp_ipc_msg_free(&recv_msg);
    active_fetches_list = NULL;
    teardown_ipc();
}
END_TEST

START_TEST(test_fetch_callback_data)
{
    setup_ipc();

    struct network_fetch_info info = {
        .fetch_id = 43,
        .fetchh = NULL,
        .finished = false,
        .next = NULL
    };
    active_fetches_list = &info;

    fetch_msg msg;
    msg.type = FETCH_DATA;
    const char *data = "Hello World Data!";
    msg.data.header_or_data.buf = (const uint8_t *)data;
    msg.data.header_or_data.len = strlen(data);

    network_process_fetch_callback(&msg, &info);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_FETCH_DATA);
    ck_assert_int_eq(recv_msg.length, 4 + strlen(data));

    uint32_t recv_fid;
    memcpy(&recv_fid, recv_msg.data, 4);
    ck_assert_int_eq(recv_fid, 43);
    ck_assert_mem_eq(recv_msg.data + 4, data, strlen(data));

    ck_assert_int_eq(info.finished, false);

    wisp_ipc_msg_free(&recv_msg);
    active_fetches_list = NULL;
    teardown_ipc();
}
END_TEST

START_TEST(test_fetch_callback_finished)
{
    setup_ipc();

    struct network_fetch_info info = {
        .fetch_id = 44,
        .fetchh = NULL,
        .finished = false,
        .next = NULL
    };
    active_fetches_list = &info;

    fetch_msg msg;
    msg.type = FETCH_FINISHED;

    network_process_fetch_callback(&msg, &info);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_FETCH_FINISHED);
    ck_assert_int_eq(recv_msg.length, 8);

    uint32_t recv_fid, recv_code;
    memcpy(&recv_fid, recv_msg.data, 4);
    memcpy(&recv_code, recv_msg.data + 4, 4);
    ck_assert_int_eq(recv_fid, 44);
    ck_assert_int_eq(recv_code, 0);

    ck_assert_int_eq(info.finished, true);
    ck_assert_ptr_null(info.fetchh);

    wisp_ipc_msg_free(&recv_msg);
    active_fetches_list = NULL;
    teardown_ipc();
}
END_TEST

START_TEST(test_fetch_callback_redirect)
{
    setup_ipc();

    struct network_fetch_info info = {
        .fetch_id = 45,
        .fetchh = NULL,
        .finished = false,
        .next = NULL
    };
    active_fetches_list = &info;

    nsurl *target_url = NULL;
    ck_assert_int_eq(nsurl_create("http://example.com/redirected", &target_url), NSERROR_OK);

    fetch_msg msg;
    msg.type = FETCH_REDIRECT;
    msg.data.redirect = target_url;

    network_process_fetch_callback(&msg, &info);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_FETCH_REDIRECT);

    uint32_t recv_fid, recv_code;
    memcpy(&recv_fid, recv_msg.data, 4);
    memcpy(&recv_code, recv_msg.data + 4, 4);
    ck_assert_int_eq(recv_fid, 45);
    ck_assert_int_eq(recv_code, 302);
    ck_assert_str_eq((const char *)recv_msg.data + 8, "http://example.com/redirected");

    ck_assert_int_eq(info.finished, true);
    ck_assert_ptr_null(info.fetchh);

    nsurl_unref(target_url);
    wisp_ipc_msg_free(&recv_msg);

    /* Test null redirect target fallback */
    info.finished = false;
    msg.data.redirect = NULL;

    network_process_fetch_callback(&msg, &info);

    err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_FETCH_REDIRECT);
    ck_assert_str_eq((const char *)recv_msg.data + 8, "");
    ck_assert_int_eq(info.finished, true);

    wisp_ipc_msg_free(&recv_msg);
    active_fetches_list = NULL;
    teardown_ipc();
}
END_TEST

START_TEST(test_fetch_callback_unknown_type)
{
    setup_ipc();

    struct network_fetch_info info = {
        .fetch_id = 99,
        .fetchh = NULL,
        .finished = false,
        .next = NULL
    };
    active_fetches_list = &info;

    fetch_msg msg;
    msg.type = (fetch_msg_type)9999; /* Unknown type */

    network_process_fetch_callback(&msg, &info);

    /* Should not send any IPC message for unknown message type */
    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_ne(err, NSERROR_OK);

    ck_assert_int_eq(info.finished, false);

    active_fetches_list = NULL;
    teardown_ipc();
}
END_TEST

START_TEST(test_fetch_callback_error)
{
    setup_ipc();

    struct network_fetch_info info = {
        .fetch_id = 46,
        .fetchh = NULL,
        .finished = false,
        .next = NULL
    };
    active_fetches_list = &info;

    fetch_msg msg;
    msg.type = FETCH_ERROR;
    msg.data.error = "CustomNetworkError";

    network_process_fetch_callback(&msg, &info);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_FETCH_ERROR);

    uint32_t recv_fid;
    memcpy(&recv_fid, recv_msg.data, 4);
    ck_assert_int_eq(recv_fid, 46);
    ck_assert_str_eq((const char *)recv_msg.data + 4, "CustomNetworkError");

    ck_assert_int_eq(info.finished, true);

    wisp_ipc_msg_free(&recv_msg);

    /* Test null error fallback to "UnknownError" */
    info.finished = false;
    msg.data.error = NULL;

    network_process_fetch_callback(&msg, &info);

    err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq((const char *)recv_msg.data + 4, "UnknownError");

    ck_assert_int_eq(info.finished, true);

    wisp_ipc_msg_free(&recv_msg);
    active_fetches_list = NULL;
    teardown_ipc();
}
END_TEST

START_TEST(test_fetch_callback_errors_special)
{
    setup_ipc();

    struct network_fetch_info info = {
        .fetch_id = 47,
        .fetchh = NULL,
        .finished = false,
        .next = NULL
    };
    active_fetches_list = &info;

    fetch_msg_type error_types[] = { FETCH_TIMEDOUT, FETCH_SSL_ERR, FETCH_CERT_ERR };
    const char *expected_msgs[] = { "Timeout", "SSLError", "CertError" };

    for (int i = 0; i < 3; i++) {
        info.finished = false;
        fetch_msg msg;
        msg.type = error_types[i];

        network_process_fetch_callback(&msg, &info);

        wisp_ipc_msg recv_msg;
        nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
        ck_assert_int_eq(err, NSERROR_OK);
        ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_FETCH_ERROR);

        uint32_t recv_fid;
        memcpy(&recv_fid, recv_msg.data, 4);
        ck_assert_int_eq(recv_fid, 47);
        ck_assert_str_eq((const char *)recv_msg.data + 4, expected_msgs[i]);

        ck_assert_int_eq(info.finished, true);
        wisp_ipc_msg_free(&recv_msg);
    }

    active_fetches_list = NULL;
    teardown_ipc();
}
END_TEST

START_TEST(test_fetch_callback_guards)
{
    setup_ipc();

    struct network_fetch_info info = {
        .fetch_id = 48,
        .fetchh = NULL,
        .finished = false,
        .next = NULL
    };

    fetch_msg msg;
    msg.type = FETCH_DATA;
    msg.data.header_or_data.buf = (const uint8_t *)"data";
    msg.data.header_or_data.len = 4;

    /* Guard 1: info is not in active_fetches_list */
    active_fetches_list = NULL;
    network_process_fetch_callback(&msg, &info);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_ne(err, NSERROR_OK);

    /* Guard 2: info is in active_fetches_list, but info.finished is true */
    active_fetches_list = &info;
    info.finished = true;

    network_process_fetch_callback(&msg, &info);

    err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_ne(err, NSERROR_OK);

    active_fetches_list = NULL;
    teardown_ipc();
}
END_TEST

START_TEST(test_fetch_abort_handling)
{
    struct network_fetch_info info1 = {
        .fetch_id = 101,
        .fetchh = NULL,
        .finished = false,
        .next = NULL
    };
    struct network_fetch_info info2 = {
        .fetch_id = 102,
        .fetchh = NULL,
        .finished = false,
        .next = &info1
    };

    active_fetches_list = &info2;

    /* Simulate FETCH_ABORT message processing for fetch_id 101 */
    uint32_t target_id = 101;
    struct network_fetch_info *curr = active_fetches_list;
    while (curr && curr->fetch_id != target_id) {
        curr = curr->next;
    }
    ck_assert_ptr_nonnull(curr);
    ck_assert_ptr_eq(curr, &info1);
    if (curr && !curr->finished) {
        if (curr->fetchh) {
            fetch_abort(curr->fetchh);
            curr->fetchh = NULL;
        }
        curr->finished = true;
    }

    ck_assert_int_eq(info1.finished, true);
    ck_assert_int_eq(info2.finished, false);

    active_fetches_list = NULL;
}
END_TEST

START_TEST(test_dns_prefetch_request)
{
    setup_ipc();

    const char *url_str = "http://example.com/test";
    uint32_t url_len = strlen(url_str);
    wisp_ipc_msg msg;
    msg.type = WISP_IPC_MSG_DNS_PREFETCH_REQUEST;
    msg.length = 4 + url_len;
    msg.data = malloc(msg.length);
    ck_assert_ptr_nonnull(msg.data);
    memcpy(msg.data, &url_len, 4);
    memcpy(msg.data + 4, url_str, url_len);

    nserror send_res = wisp_ipc_send(test_ipc_accepted, &msg);
    ck_assert_int_eq(send_res, NSERROR_OK);
    free(msg.data);

    process_one_ipc_msg();

    teardown_ipc();
}
END_TEST

START_TEST(test_preconnect_request)
{
    setup_ipc();

    const char *url_str = "https://example.com/api";
    uint32_t url_len = strlen(url_str);
    wisp_ipc_msg msg;
    msg.type = WISP_IPC_MSG_PRECONNECT_REQUEST;
    msg.length = 4 + url_len;
    msg.data = malloc(msg.length);
    ck_assert_ptr_nonnull(msg.data);
    memcpy(msg.data, &url_len, 4);
    memcpy(msg.data + 4, url_str, url_len);

    nserror send_res = wisp_ipc_send(test_ipc_accepted, &msg);
    ck_assert_int_eq(send_res, NSERROR_OK);
    free(msg.data);

    process_one_ipc_msg();

    teardown_ipc();
}
END_TEST

START_TEST(test_fetch_callback_null_buffers)
{
    setup_ipc();

    struct network_fetch_info info = {
        .fetch_id = 333,
        .fetchh = NULL,
        .finished = false,
        .next = NULL
    };
    active_fetches_list = &info;

    /* FETCH_HEADER with NULL buf and len 0 */
    fetch_msg msg;
    msg.type = FETCH_HEADER;
    msg.data.header_or_data.buf = NULL;
    msg.data.header_or_data.len = 0;

    network_process_fetch_callback(&msg, &info);

    wisp_ipc_msg recv_msg;
    nserror err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_FETCH_HEADER);
    ck_assert_int_eq(recv_msg.length, 8);
    wisp_ipc_msg_free(&recv_msg);

    /* FETCH_DATA with NULL buf and len 0 */
    msg.type = FETCH_DATA;
    msg.data.header_or_data.buf = NULL;
    msg.data.header_or_data.len = 0;

    network_process_fetch_callback(&msg, &info);

    err = wisp_ipc_recv(test_ipc_accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_FETCH_DATA);
    ck_assert_int_eq(recv_msg.length, 4);
    wisp_ipc_msg_free(&recv_msg);

    active_fetches_list = NULL;
    teardown_ipc();
}
END_TEST

START_TEST(test_fetch_ipc_abort_msg)
{
    setup_ipc();

    struct network_fetch_info info = {
        .fetch_id = 555,
        .fetchh = NULL,
        .finished = false,
        .next = NULL
    };
    active_fetches_list = &info;

    wisp_ipc_msg msg;
    msg.type = WISP_IPC_MSG_FETCH_ABORT;
    msg.length = 4;
    msg.data = malloc(4);
    ck_assert_ptr_nonnull(msg.data);
    uint32_t fid = 555;
    memcpy(msg.data, &fid, 4);

    nserror send_res = wisp_ipc_send(test_ipc_accepted, &msg);
    ck_assert_int_eq(send_res, NSERROR_OK);
    free(msg.data);

    process_one_ipc_msg();

    ck_assert_int_eq(info.finished, true);
    ck_assert_ptr_null(info.fetchh);

    active_fetches_list = NULL;
    teardown_ipc();
}
END_TEST

START_TEST(test_network_process_ipc_msg_null)
{
    network_process_ipc_msg(NULL);
    ck_assert_int_eq(1, 1);
}
END_TEST

START_TEST(test_ipc_msg_fetch_request_hash_insertion)
{
    setup_ipc();
    corestrings_init();
    nsoption_init(NULL, NULL, NULL);
    fetcher_init();

    active_fetches_list = NULL;
    memset(active_fetches_hash, 0, sizeof(active_fetches_hash));

    const char *url_str = "http://example.com/test";
    uint32_t fetch_id = 999;
    uint32_t url_len = strlen(url_str);
    wisp_ipc_msg msg;
    msg.type = WISP_IPC_MSG_FETCH_REQUEST;
    msg.length = 8 + url_len + 2;
    msg.data = calloc(1, msg.length);
    ck_assert_ptr_nonnull(msg.data);
    memcpy(msg.data, &fetch_id, 4);
    memcpy(msg.data + 4, &url_len, 4);
    memcpy(msg.data + 8, url_str, url_len);

    network_process_ipc_msg(&msg);
    free(msg.data);

    ck_assert_ptr_nonnull(active_fetches_list);
    ck_assert_int_eq(active_fetches_list->fetch_id, 999);
    ck_assert_int_eq(is_active_fetch(active_fetches_list), true);

    /* Verify it is present in active_fetches_hash */
    unsigned int h_idx = hash_fetch_info(active_fetches_list);
    ck_assert_ptr_nonnull(active_fetches_hash[h_idx]);
    ck_assert_ptr_eq(active_fetches_hash[h_idx], active_fetches_list);

    free_all_active_fetches();
    fetcher_quit();
    corestrings_fini();
    teardown_ipc();
}
END_TEST

static Suite *network_main_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("NetworkMain");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_default_mimetype);
    tcase_add_test(tc_core, test_default_filetype);
    tcase_add_test(tc_core, test_send_fetch_error);
    tcase_add_test(tc_core, test_fetch_callback_http_codes);
    tcase_add_test(tc_core, test_send_fetch_error_handling);
    tcase_add_test(tc_core, test_fetch_request_invalid_url_and_bounds);
    tcase_add_test(tc_core, test_is_active_fetch);
    tcase_add_test(tc_core, test_cleanup_finished_fetches);
    tcase_add_test(tc_core, test_free_all_active_fetches);
    tcase_add_test(tc_core, test_fetch_callback_header);
    tcase_add_test(tc_core, test_fetch_callback_data);
    tcase_add_test(tc_core, test_fetch_callback_finished);
    tcase_add_test(tc_core, test_fetch_callback_notmodified);
    tcase_add_test(tc_core, test_fetch_callback_redirect);
    tcase_add_test(tc_core, test_fetch_callback_error);
    tcase_add_test(tc_core, test_fetch_callback_errors_special);
    tcase_add_test(tc_core, test_fetch_callback_guards);
    tcase_add_test(tc_core, test_fetch_callback_unknown_type);
    tcase_add_test(tc_core, test_fetch_abort_handling);
    tcase_add_test(tc_core, test_dns_prefetch_request);
    tcase_add_test(tc_core, test_preconnect_request);
    tcase_add_test(tc_core, test_fetch_callback_null_buffers);
    tcase_add_test(tc_core, test_fetch_ipc_abort_msg);
    tcase_add_test(tc_core, test_network_process_ipc_msg_null);
    tcase_add_test(tc_core, test_ipc_msg_fetch_request_hash_insertion);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = network_main_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
