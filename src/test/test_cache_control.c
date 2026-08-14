#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "wisp/types.h"
#include "wisp/utils/errors.h"
#include "wisp/utils/corestrings.h"
#include "utils/http/cache-control.h"
#include <limits.h>

START_TEST(test_cache_control_parse)
{
    nserror err;
    http_cache_control *cc = NULL;

    // Setup corestrings which is needed for max-age etc lwc_strings
    err = corestrings_init();
    ck_assert_int_eq(err, NSERROR_OK);

    err = http_parse_cache_control("max-age=3600", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);

    ck_assert_int_eq(http_cache_control_has_max_age(cc), true);
    ck_assert_int_eq(http_cache_control_max_age(cc), 3600);
    ck_assert_int_eq(http_cache_control_no_cache(cc), false);
    ck_assert_int_eq(http_cache_control_no_store(cc), false);

    http_cache_control_destroy(cc);
    cc = NULL;

    err = http_parse_cache_control("no-cache, no-store", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);

    ck_assert_int_eq(http_cache_control_has_max_age(cc), false);
    ck_assert_int_eq(http_cache_control_no_cache(cc), true);
    ck_assert_int_eq(http_cache_control_no_store(cc), true);

    http_cache_control_destroy(cc);
    cc = NULL;

    err = http_parse_cache_control("no-cache, max-age=123", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);

    ck_assert_int_eq(http_cache_control_has_max_age(cc), true);
    ck_assert_int_eq(http_cache_control_max_age(cc), 123);
    ck_assert_int_eq(http_cache_control_no_cache(cc), true);
    ck_assert_int_eq(http_cache_control_no_store(cc), false);

    http_cache_control_destroy(cc);
    cc = NULL;

    err = http_parse_cache_control("max-age=invalid", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);

    ck_assert_int_eq(http_cache_control_has_max_age(cc), false);
    ck_assert_int_eq(http_cache_control_no_cache(cc), false);
    ck_assert_int_eq(http_cache_control_no_store(cc), false);

    http_cache_control_destroy(cc);
    cc = NULL;

    err = http_parse_cache_control("no-cache, no-cache", &cc);
    ck_assert_int_eq(err, NSERROR_NOT_FOUND);

    err = http_parse_cache_control("public, max-age=3600", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);
    ck_assert_int_eq(http_cache_control_has_max_age(cc), true);
    ck_assert_int_eq(http_cache_control_max_age(cc), 3600);
    ck_assert_int_eq(http_cache_control_no_cache(cc), false);
    ck_assert_int_eq(http_cache_control_no_store(cc), false);
    http_cache_control_destroy(cc);
    cc = NULL;

    err = http_parse_cache_control("private=\"foo\", no-cache", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);
    ck_assert_int_eq(http_cache_control_has_max_age(cc), false);
    ck_assert_int_eq(http_cache_control_no_cache(cc), true);
    ck_assert_int_eq(http_cache_control_no_store(cc), false);
    http_cache_control_destroy(cc);
    cc = NULL;

    err = http_parse_cache_control("max-age=", &cc);
    ck_assert_int_eq(err, NSERROR_NOT_FOUND);

    err = http_parse_cache_control("max-age=4294967296", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);
    ck_assert_int_eq(http_cache_control_has_max_age(cc), true);
    ck_assert_int_eq(http_cache_control_max_age(cc), UINT_MAX);
    http_cache_control_destroy(cc);
    cc = NULL;

    err = http_parse_cache_control("  max-age  =  3600  ,  no-cache  ", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);
    ck_assert_int_eq(http_cache_control_has_max_age(cc), true);
    ck_assert_int_eq(http_cache_control_max_age(cc), 3600);
    ck_assert_int_eq(http_cache_control_no_cache(cc), true);
    ck_assert_int_eq(http_cache_control_no_store(cc), false);
    http_cache_control_destroy(cc);
    cc = NULL;

    corestrings_fini();
}
END_TEST

static Suite *cache_control_suite_create(void)
{
    Suite *s;
    TCase *tc;

    s = suite_create("cache-control");
    tc = tcase_create("Core");

    tcase_add_test(tc, test_cache_control_parse);

    suite_add_tcase(s, tc);

    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    SRunner *sr;

    sr = srunner_create(cache_control_suite_create());
    srunner_run_all(sr, CK_ENV);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
