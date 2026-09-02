/*
 * Test for nscss_dump_computed_style in content/handlers/css/dump.c
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libcss/libcss.h>

#include "content/handlers/css/dump.h"

START_TEST(test_nscss_dump_computed_style_null)
{
    char buffer[256];
    FILE *stream = tmpfile();
    ck_assert_ptr_nonnull(stream);

    nscss_dump_computed_style(stream, NULL);

    rewind(stream);
    size_t n = fread(buffer, 1, sizeof(buffer) - 1, stream);
    buffer[n] = '\0';
    fclose(stream);

    ck_assert_str_eq(buffer, "{ }");
}
END_TEST

static Suite *css_dump_suite(void)
{
    Suite *s = suite_create("css_dump");
    TCase *tc = tcase_create("dump");

    tcase_add_test(tc, test_nscss_dump_computed_style_null);
    suite_add_tcase(s, tc);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = css_dump_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
