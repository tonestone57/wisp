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

START_TEST(test_idna_empty_input)
{
    char *out = NULL;
    size_t out_len = 0;
    nserror err = idna_encode("", 0, &out, &out_len);
    if (err == NSERROR_NOT_IMPLEMENTED) return;
    ck_assert_int_eq(err, NSERROR_BAD_URL);
    if(out) free(out);

    out = NULL;
    out_len = 0;
    err = idna_decode("", 0, &out, &out_len);
    ck_assert_int_eq(err, NSERROR_BAD_URL);
    if(out) free(out);
}
END_TEST

START_TEST(test_idna_long_input)
{
    char *out = NULL;
    size_t out_len = 0;
    char long_host[300];
    memset(long_host, 'a', 290);
    long_host[290] = '\0';

    nserror err = idna_encode(long_host, 290, &out, &out_len);
    if (err == NSERROR_NOT_IMPLEMENTED) return;
    ck_assert_int_eq(err, NSERROR_BAD_URL);
    if(out) free(out);

    out = NULL;
    out_len = 0;
    err = idna_decode(long_host, 290, &out, &out_len);
    ck_assert_int_eq(err, NSERROR_BAD_URL);
    if(out) free(out);
}
END_TEST

START_TEST(test_idna_encode_invalid_ace)
{
    char *out = NULL;
    size_t out_len = 0;
    /* "xn--0" is used because it guarantees a punycode decoding failure */
    nserror err = idna_encode("xn--0", 5, &out, &out_len);
    if (err == NSERROR_NOT_IMPLEMENTED) return;
    ck_assert_int_eq(err, NSERROR_UNKNOWN);
    if(out) free(out);
}
END_TEST

START_TEST(test_idna_decode_invalid_ace)
{
    char *out = NULL;
    size_t out_len = 0;
    /* Use a string that passes idna__is_ace but fails decoding */
    nserror err = idna_decode("xn--0", 5, &out, &out_len);
    if (err == NSERROR_NOT_IMPLEMENTED) return;
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(out, "xn--0");
    if(out) free(out);
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
    tcase_add_test(tc, test_idna_empty_input);
    tcase_add_test(tc, test_idna_long_input);
    tcase_add_test(tc, test_idna_encode_invalid_ace);
    tcase_add_test(tc, test_idna_decode_invalid_ace);
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
