#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "wisp/types.h"
#include "wisp/utils/errors.h"
#include "wisp/utils/corestrings.h"
#include "utils/http/strict-transport-security.h"

START_TEST(test_sts_parse_valid)
{
    nserror err;
    http_strict_transport_security *sts = NULL;

    err = corestrings_init();
    ck_assert_int_eq(err, NSERROR_OK);

    err = http_parse_strict_transport_security("max-age=3600", &sts);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(sts);
    ck_assert_int_eq(http_strict_transport_security_max_age(sts), 3600);
    ck_assert_int_eq(http_strict_transport_security_include_subdomains(sts), false);
    http_strict_transport_security_destroy(sts);
    sts = NULL;

    err = http_parse_strict_transport_security("max-age=31536000; includeSubDomains", &sts);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(sts);
    ck_assert_int_eq(http_strict_transport_security_max_age(sts), 31536000);
    ck_assert_int_eq(http_strict_transport_security_include_subdomains(sts), true);
    http_strict_transport_security_destroy(sts);
    sts = NULL;

    err = http_parse_strict_transport_security("max-age=\"3600\" ; includeSubDomains", &sts);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(sts);
    ck_assert_int_eq(http_strict_transport_security_max_age(sts), 3600);
    ck_assert_int_eq(http_strict_transport_security_include_subdomains(sts), true);
    http_strict_transport_security_destroy(sts);
    sts = NULL;

    corestrings_fini();
}
END_TEST

START_TEST(test_sts_parse_invalid)
{
    nserror err;
    http_strict_transport_security *sts = NULL;

    err = corestrings_init();
    ck_assert_int_eq(err, NSERROR_OK);

    err = http_parse_strict_transport_security("max-age=3600; max-age=3600", &sts);
    ck_assert_int_eq(err, NSERROR_NOT_FOUND);
    if (sts != NULL) {
        http_strict_transport_security_destroy(sts);
        sts = NULL;
    }

    err = http_parse_strict_transport_security("includeSubDomains", &sts);
    ck_assert_int_eq(err, NSERROR_NOT_FOUND);
    if (sts != NULL) {
        http_strict_transport_security_destroy(sts);
        sts = NULL;
    }

    err = http_parse_strict_transport_security("max-age=invalid", &sts);
    ck_assert_int_eq(err, NSERROR_NOT_FOUND);
    if (sts != NULL) {
        http_strict_transport_security_destroy(sts);
        sts = NULL;
    }

    err = http_parse_strict_transport_security("max-age=3600; includeSubDomains=true", &sts);
    ck_assert_int_eq(err, NSERROR_NOT_FOUND);
    if (sts != NULL) {
        http_strict_transport_security_destroy(sts);
        sts = NULL;
    }

    corestrings_fini();
}
END_TEST

static Suite *sts_suite_create(void)
{
    Suite *s;
    TCase *tc;

    s = suite_create("strict-transport-security");
    tc = tcase_create("Core");

    tcase_add_test(tc, test_sts_parse_valid);
    tcase_add_test(tc, test_sts_parse_invalid);

    suite_add_tcase(s, tc);

    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    SRunner *sr;

    sr = srunner_create(sts_suite_create());
    srunner_run_all(sr, CK_ENV);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
