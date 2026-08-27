#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "wisp/utils/ipc.h"
#include "wisp/utils/errors.h"

#ifdef _WIN32
#include <winsock2.h>
#endif

START_TEST(test_ipc_server_create_destroy)
{
    const char *sock_name = "test_ipc_server_create_destroy_sock";
    wisp_ipc_handle *server = wisp_ipc_create_server(sock_name);
    ck_assert_ptr_nonnull(server);
    wisp_ipc_destroy(server);
}
END_TEST

START_TEST(test_ipc_connect_accept)
{
    const char *sock_name = "test_ipc_connect_accept_sock";
    wisp_ipc_handle *server = wisp_ipc_create_server(sock_name);
    ck_assert_ptr_nonnull(server);

    wisp_ipc_handle *client = wisp_ipc_connect(sock_name);
    ck_assert_ptr_nonnull(client);

    wisp_ipc_handle *accepted = wisp_ipc_accept(server);
    ck_assert_ptr_nonnull(accepted);

    wisp_ipc_destroy(accepted);
    wisp_ipc_destroy(client);
    wisp_ipc_destroy(server);
}
END_TEST

START_TEST(test_ipc_send_recv_basic)
{
    const char *sock_name = "test_ipc_send_recv_basic_sock";
    wisp_ipc_handle *server = wisp_ipc_create_server(sock_name);
    ck_assert_ptr_nonnull(server);

    wisp_ipc_handle *client = wisp_ipc_connect(sock_name);
    ck_assert_ptr_nonnull(client);

    wisp_ipc_handle *accepted = wisp_ipc_accept(server);
    ck_assert_ptr_nonnull(accepted);

    wisp_ipc_msg msg_send;
    msg_send.type = WISP_IPC_MSG_FETCH_REQUEST;
    const char *payload = "Hello IPC!";
    msg_send.length = strlen(payload) + 1;
    msg_send.data = (uint8_t *)payload;

    nserror err = wisp_ipc_send(client, &msg_send);
    ck_assert_int_eq(err, NSERROR_OK);

    wisp_ipc_msg msg_recv;
    err = wisp_ipc_recv(accepted, &msg_recv);
    ck_assert_int_eq(err, NSERROR_OK);

    ck_assert_int_eq(msg_recv.type, WISP_IPC_MSG_FETCH_REQUEST);
    ck_assert_int_eq(msg_recv.length, msg_send.length);
    ck_assert_str_eq((const char *)msg_recv.data, payload);

    wisp_ipc_msg_free(&msg_recv);

    wisp_ipc_destroy(accepted);
    wisp_ipc_destroy(client);
    wisp_ipc_destroy(server);
}
END_TEST

START_TEST(test_ipc_find_executable_not_found)
{
    char out_path[1024];
    bool found = wisp_ipc_find_executable("this_executable_does_not_exist_12345", out_path, sizeof(out_path));
    ck_assert_int_eq(found, false);
}
END_TEST

START_TEST(test_ipc_safe_path_truncation)
{
    /* Test creating server and connecting with a long socket name */
    char long_sock_name[256];
    memset(long_sock_name, 'a', sizeof(long_sock_name) - 1);
    long_sock_name[sizeof(long_sock_name) - 1] = '\0';

    wisp_ipc_handle *server = wisp_ipc_create_server(long_sock_name);
    if (server) {
        wisp_ipc_handle *client = wisp_ipc_connect(long_sock_name);
        if (client) {
            wisp_ipc_destroy(client);
        }
        wisp_ipc_destroy(server);
    }
}
END_TEST

START_TEST(test_ipc_spawn_security_validation)
{
    /* Test NULL and empty inputs */
    ck_assert_int_eq(wisp_ipc_spawn(NULL, "valid_name"), -1);
    ck_assert_int_eq(wisp_ipc_spawn("/bin/ls", NULL), -1);
    ck_assert_int_eq(wisp_ipc_spawn("", "valid_name"), -1);
    ck_assert_int_eq(wisp_ipc_spawn("/bin/ls", ""), -1);

    /* Test control characters and double quotes */
    ck_assert_int_eq(wisp_ipc_spawn("/bin/ls\n", "valid_name"), -1);
    ck_assert_int_eq(wisp_ipc_spawn("/bin/ls", "valid\nname"), -1);
    ck_assert_int_eq(wisp_ipc_spawn("/bin/ls\"", "valid_name"), -1);
    ck_assert_int_eq(wisp_ipc_spawn("/bin/ls", "valid\"name"), -1);

    /* Test non-existent executable path */
    ck_assert_int_eq(wisp_ipc_spawn("/nonexistent_path_12345/binary", "valid_name"), -1);

    /* Test non-regular file path (e.g. directory or character device) */
    ck_assert_int_eq(wisp_ipc_spawn("/tmp", "valid_name"), -1);
#ifndef _WIN32
    ck_assert_int_eq(wisp_ipc_spawn("/dev/null", "valid_name"), -1);
#endif
}
END_TEST

static TCase *ipc_case_create(void)
{
    TCase *tc;
    tc = tcase_create("IPC");
    tcase_add_test(tc, test_ipc_server_create_destroy);
    tcase_add_test(tc, test_ipc_connect_accept);
    tcase_add_test(tc, test_ipc_send_recv_basic);
    tcase_add_test(tc, test_ipc_find_executable_not_found);
    tcase_add_test(tc, test_ipc_safe_path_truncation);
    tcase_add_test(tc, test_ipc_spawn_security_validation);
    return tc;
}

static Suite *ipc_suite_create(void)
{
    Suite *s;
    s = suite_create("IPC Utils");
    suite_add_tcase(s, ipc_case_create());
    return s;
}

int main(int argc, char **argv)
{
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    int number_failed;
    SRunner *sr;
    sr = srunner_create(ipc_suite_create());
    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

#ifdef _WIN32
    WSACleanup();
#endif

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
