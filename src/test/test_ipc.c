#include <check.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wisp/utils/ipc.h>
#include <wisp/utils/errors.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#endif

static const char *test_sock_name = "test_ipc_socket";

static void setup(void) {
#ifndef _WIN32
    unlink(test_sock_name);
#endif
}

START_TEST(test_ipc_create_server)
{
    wisp_ipc_handle *server = wisp_ipc_create_server(test_sock_name);
    ck_assert_ptr_ne(server, NULL);
    wisp_ipc_destroy(server);
}
END_TEST

START_TEST(test_ipc_connect_accept)
{
    wisp_ipc_handle *server = wisp_ipc_create_server(test_sock_name);
    ck_assert_ptr_ne(server, NULL);

    wisp_ipc_handle *client = wisp_ipc_connect(test_sock_name);
    ck_assert_ptr_ne(client, NULL);

    wisp_ipc_handle *accepted = wisp_ipc_accept(server);
    ck_assert_ptr_ne(accepted, NULL);

    wisp_ipc_destroy(accepted);
    wisp_ipc_destroy(client);
    wisp_ipc_destroy(server);
}
END_TEST

START_TEST(test_ipc_send_recv)
{
    wisp_ipc_handle *server = wisp_ipc_create_server(test_sock_name);
    ck_assert_ptr_ne(server, NULL);

    wisp_ipc_handle *client = wisp_ipc_connect(test_sock_name);
    ck_assert_ptr_ne(client, NULL);

    wisp_ipc_handle *accepted = wisp_ipc_accept(server);
    ck_assert_ptr_ne(accepted, NULL);

    /* Send message from client to accepted server connection */
    wisp_ipc_msg send_msg;
    send_msg.type = WISP_IPC_MSG_FETCH_REQUEST;
    send_msg.length = 5;
    send_msg.data = (uint8_t *)"hello";

    nserror err = wisp_ipc_send(client, &send_msg);
    ck_assert_int_eq(err, NSERROR_OK);

    /* Receive message on server side */
    wisp_ipc_msg recv_msg;
    err = wisp_ipc_recv(accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);

    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_FETCH_REQUEST);
    ck_assert_int_eq(recv_msg.length, 5);
    ck_assert_mem_eq(recv_msg.data, "hello", 5);

    wisp_ipc_msg_free(&recv_msg);
    wisp_ipc_destroy(accepted);
    wisp_ipc_destroy(client);
    wisp_ipc_destroy(server);
}
END_TEST

START_TEST(test_ipc_send_large_message)
{
    wisp_ipc_handle *server = wisp_ipc_create_server(test_sock_name);
    ck_assert_ptr_ne(server, NULL);

    wisp_ipc_handle *client = wisp_ipc_connect(test_sock_name);
    ck_assert_ptr_ne(client, NULL);

    wisp_ipc_handle *accepted = wisp_ipc_accept(server);
    ck_assert_ptr_ne(accepted, NULL);

    /* Send 16KB message to completely avoid deadlocking the local socket buffer in single-threaded test */
    uint32_t len = 16 * 1024;
    uint8_t *large_data = malloc(len);
    ck_assert_ptr_ne(large_data, NULL);
    memset(large_data, 0xAB, len);

    wisp_ipc_msg send_msg;
    send_msg.type = WISP_IPC_MSG_FETCH_DATA;
    send_msg.length = len;
    send_msg.data = large_data;

    nserror err = wisp_ipc_send(client, &send_msg);
    ck_assert_int_eq(err, NSERROR_OK);

    /* Receive message on server side */
    wisp_ipc_msg recv_msg;
    err = wisp_ipc_recv(accepted, &recv_msg);
    ck_assert_int_eq(err, NSERROR_OK);

    ck_assert_int_eq(recv_msg.type, WISP_IPC_MSG_FETCH_DATA);
    ck_assert_int_eq(recv_msg.length, len);
    ck_assert_mem_eq(recv_msg.data, large_data, len);

    free(large_data);
    wisp_ipc_msg_free(&recv_msg);
    wisp_ipc_destroy(accepted);
    wisp_ipc_destroy(client);
    wisp_ipc_destroy(server);
}
END_TEST

static Suite *ipc_suite(void)
{
    Suite *s = suite_create("ipc");
    TCase *tc_core = tcase_create("Core");

    tcase_add_checked_fixture(tc_core, setup, NULL);
    tcase_add_test(tc_core, test_ipc_create_server);
    tcase_add_test(tc_core, test_ipc_connect_accept);
    tcase_add_test(tc_core, test_ipc_send_recv);
    tcase_add_test(tc_core, test_ipc_send_large_message);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    int number_failed;
    Suite *s = ipc_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

#ifdef _WIN32
    WSACleanup();
#endif

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
