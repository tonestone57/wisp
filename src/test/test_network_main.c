#include <check.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "wisp/utils/ipc.h"
#include "wisp/utils/nsurl.h"

#define main network_process_main
#include "../processes/network/main.c"
#undef main

START_TEST(test_default_filetype)
{
    ck_assert_str_eq(default_filetype("style.css"), "text/css");
    ck_assert_str_eq(default_filetype("page.html"), "text/html");
    ck_assert_str_eq(default_filetype("image.png"), "image/png");
    ck_assert_str_eq(default_filetype("photo.jpg"), "image/jpeg");
    ck_assert_str_eq(default_filetype("photo.jpeg"), "image/jpeg");
    ck_assert_str_eq(default_filetype("document.txt"), "text/plain");
    ck_assert_str_eq(default_filetype("unknown.xyz"), "text/plain");
}
END_TEST

START_TEST(test_is_active_fetch)
{
    struct network_fetch_info f1 = { .fetch_id = 1, .finished = false, .next = NULL };
    struct network_fetch_info f2 = { .fetch_id = 2, .finished = false, .next = NULL };
    struct network_fetch_info f3 = { .fetch_id = 3, .finished = false, .next = NULL };

    active_fetches_list = NULL;
    ck_assert_int_eq(is_active_fetch(&f1), false);

    active_fetches_list = &f1;
    f1.next = &f2;
    f2.next = &f3;

    ck_assert_int_eq(is_active_fetch(&f1), true);
    ck_assert_int_eq(is_active_fetch(&f2), true);
    ck_assert_int_eq(is_active_fetch(&f3), true);

    struct network_fetch_info unlisted = { .fetch_id = 99, .finished = false, .next = NULL };
    ck_assert_int_eq(is_active_fetch(&unlisted), false);

    active_fetches_list = NULL;
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

static Suite *network_main_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("NetworkMain");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_default_filetype);
    tcase_add_test(tc_core, test_is_active_fetch);
    tcase_add_test(tc_core, test_cleanup_finished_fetches);
    tcase_add_test(tc_core, test_fetch_callback_header);
    tcase_add_test(tc_core, test_fetch_callback_data);
    tcase_add_test(tc_core, test_fetch_callback_finished);
    tcase_add_test(tc_core, test_fetch_callback_redirect);
    tcase_add_test(tc_core, test_fetch_callback_error);
    tcase_add_test(tc_core, test_fetch_callback_errors_special);
    tcase_add_test(tc_core, test_fetch_callback_guards);

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
