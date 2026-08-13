#include <check.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <wisp/utils/errors.h>
#include "utils/idna.h"

START_TEST(test_idna_encode_basic)
{
    char *ace_host = NULL;
    size_t ace_len = 0;
    nserror err = idna_encode("example.com", 11, &ace_host, &ace_len);
    if (err == NSERROR_NOT_IMPLEMENTED) return;
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(ace_host, "example.com");
    free(ace_host);
}
END_TEST

START_TEST(test_idna_encode_idn)
{
    char *ace_host = NULL;
    size_t ace_len = 0;
    /* "münchen.de" */
    const char *idn_host = "m\xc3\xbcnchen.de";
    nserror err = idna_encode(idn_host, strlen(idn_host), &ace_host, &ace_len);
    if (err == NSERROR_NOT_IMPLEMENTED) return;
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(ace_host, "xn--mnchen-3ya.de");
    free(ace_host);
}
END_TEST

START_TEST(test_idna_decode_basic)
{
    char *host = NULL;
    size_t host_len = 0;
    nserror err = idna_decode("example.com", 11, &host, &host_len);
    if (err == NSERROR_NOT_IMPLEMENTED) return;
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(host, "example.com");
    free(host);
}
END_TEST

START_TEST(test_idna_decode_idn)
{
    char *host = NULL;
    size_t host_len = 0;
    const char *ace_host = "xn--mnchen-3ya.de";
    nserror err = idna_decode(ace_host, strlen(ace_host), &host, &host_len);
    if (err == NSERROR_NOT_IMPLEMENTED) return;
    ck_assert_int_eq(err, NSERROR_OK);
    /* "münchen.de" */
    ck_assert_str_eq(host, "m\xc3\xbcnchen.de");
    free(host);
}
END_TEST

static Suite *idna_suite_create(void)
{
    Suite *s = suite_create("IDNA");
    TCase *tc = tcase_create("idna");
    tcase_add_test(tc, test_idna_encode_basic);
    tcase_add_test(tc, test_idna_encode_idn);
    tcase_add_test(tc, test_idna_decode_basic);
    tcase_add_test(tc, test_idna_decode_idn);
    suite_add_tcase(s, tc);
    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    SRunner *sr = srunner_create(idna_suite_create());
    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
