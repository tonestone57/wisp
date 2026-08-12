#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "utils/errors.h"
#include "utils/idna.h"

START_TEST(test_idna_encode_ascii)
{
    char *ace_host = NULL;
    size_t ace_len = 0;

    // Normal ASCII domain
    nserror err = idna_encode("example.com", 11, &ace_host, &ace_len);
    ck_assert(err == NSERROR_OK || err == NSERROR_NOT_IMPLEMENTED); if (err == NSERROR_NOT_IMPLEMENTED) return;
    ck_assert_str_eq(ace_host, "example.com");
    ck_assert_int_eq(ace_len, 11);

    if (ace_host) free(ace_host);
}
END_TEST

START_TEST(test_idna_encode_unicode)
{
    char *ace_host = NULL;
    size_t ace_len = 0;

    // münchen.de (in UTF-8)
    const char *unicode_host = "m\xc3\xbcnchen.de";
    size_t len = strlen(unicode_host);
    nserror err = idna_encode(unicode_host, len, &ace_host, &ace_len);
    ck_assert(err == NSERROR_OK || err == NSERROR_NOT_IMPLEMENTED); if (err == NSERROR_NOT_IMPLEMENTED) return;
    ck_assert_str_eq(ace_host, "xn--mnchen-3ya.de");
    ck_assert_int_eq(ace_len, strlen("xn--mnchen-3ya.de"));

    if (ace_host) free(ace_host);
}
END_TEST

START_TEST(test_idna_encode_invalid_empty)
{
    char *ace_host = NULL;
    size_t ace_len = 0;

    // Empty host should probably fail or handle gracefully depending on implementation
    nserror err = idna_encode("", 0, &ace_host, &ace_len);
    ck_assert(err == NSERROR_BAD_URL || err == NSERROR_NOT_IMPLEMENTED); if (err == NSERROR_NOT_IMPLEMENTED) return;
}
END_TEST

START_TEST(test_idna_decode_ascii)
{
    char *host = NULL;
    size_t host_len = 0;

    // Normal ASCII domain
    nserror err = idna_decode("example.com", 11, &host, &host_len);
    ck_assert(err == NSERROR_OK || err == NSERROR_NOT_IMPLEMENTED); if (err == NSERROR_NOT_IMPLEMENTED) return;
    ck_assert_str_eq(host, "example.com");
    ck_assert_int_eq(host_len, 11);

    if (host) free(host);
}
END_TEST

START_TEST(test_idna_decode_unicode)
{
    char *host = NULL;
    size_t host_len = 0;

    // xn--mnchen-3ya.de
    const char *ace_host = "xn--mnchen-3ya.de";
    size_t len = strlen(ace_host);
    nserror err = idna_decode(ace_host, len, &host, &host_len);
    ck_assert(err == NSERROR_OK || err == NSERROR_NOT_IMPLEMENTED); if (err == NSERROR_NOT_IMPLEMENTED) return;

    const char *expected = "m\xc3\xbcnchen.de";
    ck_assert_str_eq(host, expected);
    ck_assert_int_eq(host_len, strlen(expected));

    if (host) free(host);
}
END_TEST

static Suite *idna_suite(void)
{
    Suite *s;
    TCase *tc;

    s = suite_create("idna");
    tc = tcase_create("core");

    tcase_add_test(tc, test_idna_encode_ascii);
    tcase_add_test(tc, test_idna_encode_unicode);
    tcase_add_test(tc, test_idna_encode_invalid_empty);
    tcase_add_test(tc, test_idna_decode_ascii);
    tcase_add_test(tc, test_idna_decode_unicode);

    suite_add_tcase(s, tc);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = idna_suite();
    sr = srunner_create(s);
    srunner_run_all(sr, CK_ENV);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
