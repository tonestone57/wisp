#include <check.h>
#include <wisp/utils/messages.h>
#include <wisp/utils/errors.h>
#include <wisp/ssl_certs.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <zlib.h>

START_TEST(test_messages_add_key_value)
{
    nserror err;
    err = messages_add_key_value("TestKey1", "TestValue1");
    ck_assert_int_eq(err, NSERROR_OK);

    const char *val = messages_get("TestKey1");
    ck_assert_str_eq(val, "TestValue1");
}
END_TEST

START_TEST(test_messages_add_from_inline)
{
    nserror err;
    const char *data = "InlineKey:InlineValue\n";
    err = messages_add_from_inline((const uint8_t *)data, strlen(data));
    ck_assert_int_eq(err, NSERROR_OK);

    const char *val = messages_get("InlineKey");
    ck_assert_str_eq(val, "InlineValue");
}
END_TEST

START_TEST(test_messages_get_errorcode)
{
    messages_add_key_value("OK", "No error");
    const char *val = messages_get_errorcode(NSERROR_OK);
    ck_assert_str_eq(val, "No error");

    messages_add_key_value("NoMemory", "Memory exhaustion");
    val = messages_get_errorcode(NSERROR_NOMEM);
    ck_assert_str_eq(val, "Memory exhaustion");

    messages_add_key_value("Unknown", "Unknown error");
    val = messages_get_errorcode(NSERROR_UNKNOWN);
    ck_assert_str_eq(val, "Unknown error");
}
END_TEST

START_TEST(test_messages_get_sslcode)
{
    messages_add_key_value("SSLCertErrOk", "Nothing wrong");
    const char *val = messages_get_sslcode(SSL_CERT_ERR_OK);
    ck_assert_str_eq(val, "Nothing wrong");

    messages_add_key_value("SSLCertErrUnknown", "Unknown error");
    val = messages_get_sslcode(SSL_CERT_ERR_UNKNOWN);
    ck_assert_str_eq(val, "Unknown error");
}
END_TEST

START_TEST(test_messages_get_buff)
{
    nserror err;
    err = messages_add_key_value("TestFormat", "Test %s %d");
    ck_assert_int_eq(err, NSERROR_OK);

    char *buff = messages_get_buff("TestFormat", "foo", 42);
    ck_assert_ptr_nonnull(buff);
    ck_assert_str_eq(buff, "Test foo 42");
    free(buff);
}
END_TEST

START_TEST(test_messages_destroy)
{
    messages_add_key_value("TestKeyToDestroy", "Value");
    messages_destroy();

    const char *val = messages_get("TestKeyToDestroy");
    ck_assert_str_eq(val, "TestKeyToDestroy");
}
END_TEST

Suite *messages_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("messages");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_messages_add_key_value);
    tcase_add_test(tc_core, test_messages_add_from_inline);
    tcase_add_test(tc_core, test_messages_get_errorcode);
    tcase_add_test(tc_core, test_messages_get_sslcode);
    tcase_add_test(tc_core, test_messages_get_buff);
    tcase_add_test(tc_core, test_messages_destroy);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = messages_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
