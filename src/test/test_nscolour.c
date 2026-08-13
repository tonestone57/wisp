#include <check.h>
#include <stdlib.h>

#include "wisp/types.h"
#include "wisp/utils/errors.h"
#include "wisp/utils/nsoption.h"
#include "utils/nscolour.h"
#include "desktop/system_colour.h"

START_TEST(nscolour_get_edge_cases_test)
{
    nserror res;
    colour bg, bg_hover, fg, fg_subtle, fg_faded, fg_good, fg_bad, border;

    res = nsoption_init(NULL, NULL, NULL);
    ck_assert_int_eq(res, NSERROR_OK);

    res = ns_system_colour_init();
    ck_assert_int_eq(res, NSERROR_OK);

    /* Test invalid background name */
    res = nscolour__get("InvalidBg", "CanvasText", 16, 16, &bg, &bg_hover, &fg, &fg_subtle, &fg_faded, &fg_good, &fg_bad, &border);
    ck_assert_int_eq(res, NSERROR_INVALID);

    /* Test invalid foreground name */
    res = nscolour__get("Canvas", "InvalidFg", 16, 16, &bg, &bg_hover, &fg, &fg_subtle, &fg_faded, &fg_good, &fg_bad, &border);
    ck_assert_int_eq(res, NSERROR_INVALID);

    /* Test light mode (dark fg, light bg) with no background adjustment (bg_num >= bg_den) */
    res = nscolour__get("Canvas", "CanvasText", 16, 16, &bg, &bg_hover, &fg, &fg_subtle, &fg_faded, &fg_good, &fg_bad, &border);
    ck_assert_int_eq(res, NSERROR_OK);

    /* Test dark mode (light fg, dark bg) with background adjustment (bg_num < bg_den) */
    res = nscolour__get("CanvasText", "Canvas", 15, 16, &bg, &bg_hover, &fg, &fg_subtle, &fg_faded, &fg_good, &fg_bad, &border);
    ck_assert_int_eq(res, NSERROR_OK);

    /* Test NULL outputs (should not crash) */
    res = nscolour__get("Canvas", "CanvasText", 16, 16, &bg, NULL, &fg, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(res, NSERROR_OK);

    ns_system_colour_finalize();
    nsoption_finalise(NULL, NULL);
}
END_TEST

START_TEST(nscolour_update_test)
{
    nserror res;

    res = nsoption_init(NULL, NULL, NULL);
    ck_assert_int_eq(res, NSERROR_OK);

    res = ns_system_colour_init();
    ck_assert_int_eq(res, NSERROR_OK);

    res = nscolour_update();
    ck_assert_int_eq(res, NSERROR_OK);

    ns_system_colour_finalize();
    nsoption_finalise(NULL, NULL);
}
END_TEST

START_TEST(nscolour_get_stylesheet_test)
{
    nserror res;
    const char *stylesheet = NULL;

    res = nsoption_init(NULL, NULL, NULL);
    ck_assert_int_eq(res, NSERROR_OK);

    res = ns_system_colour_init();
    ck_assert_int_eq(res, NSERROR_OK);

    res = nscolour_update();
    ck_assert_int_eq(res, NSERROR_OK);

    res = nscolour_get_stylesheet(&stylesheet);
    ck_assert_int_eq(res, NSERROR_OK);
    ck_assert_ptr_nonnull(stylesheet);

    ns_system_colour_finalize();
    nsoption_finalise(NULL, NULL);
}
END_TEST

static Suite *nscolour_suite_create(void)
{
    Suite *s;
    TCase *tc;

    s = suite_create("nscolour");
    tc = tcase_create("Core");

    tcase_add_test(tc, nscolour_update_test);
    tcase_add_test(tc, nscolour_get_stylesheet_test);
    tcase_add_test(tc, nscolour_get_edge_cases_test);

    suite_add_tcase(s, tc);

    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    SRunner *sr;

    sr = srunner_create(nscolour_suite_create());
    srunner_run_all(sr, CK_ENV);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
