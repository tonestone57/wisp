/*
 * Copyright 2026 Wisp Project
 * Test for inline vertical alignment calculation in layout.c
 */

#include <check.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "wisp/content/handlers/html/box.h"
#include "wisp/css.h"
#include "wisp/types.h"

typedef struct html_content html_content;

struct html_content {
	struct css_unit_ctx unit_len_ctx;
};

#include "content/handlers/html/layout_internal.h"

START_TEST(test_outer_height_calculation)
{
	struct html_content content = {0};

	struct box *text_box = calloc(1, sizeof(struct box));
	text_box->type = BOX_TEXT;
	text_box->height = 20;
	ck_assert_int_eq(layout_box_get_outer_height(text_box, &content), 20);

	struct box *ib_box = calloc(1, sizeof(struct box));
	ib_box->type = BOX_INLINE_BLOCK;
	ib_box->height = 50;
	ib_box->margin[TOP] = 5;
	ib_box->margin[BOTTOM] = 5;
	ib_box->border[TOP].width = 2;
	ib_box->border[BOTTOM].width = 2;
	ib_box->padding[TOP] = 10;
	ib_box->padding[BOTTOM] = 10;
	ck_assert_int_eq(layout_box_get_outer_height(ib_box, &content), 84);

	free(text_box);
	free(ib_box);
}
END_TEST

START_TEST(test_baseline_calculation)
{
	struct box *text_box = calloc(1, sizeof(struct box));
	text_box->type = BOX_TEXT;
	text_box->height = 20;
	ck_assert_int_eq(layout_box_get_baseline(text_box), 16);

	struct box *ib_box = calloc(1, sizeof(struct box));
	ib_box->type = BOX_INLINE_BLOCK;
	ib_box->margin[TOP] = 5;
	ib_box->border[TOP].width = 2;
	ib_box->padding[TOP] = 10;
	ib_box->children = text_box;
	text_box->y = 0;

	/* Baseline = margin-top (5) + border-top (2) + padding-top (10) + text baseline (16) = 33 */
	ck_assert_int_eq(layout_box_get_baseline(ib_box), 33);

	free(text_box);
	free(ib_box);
}
END_TEST

START_TEST(test_replaced_inline_baseline)
{
	struct box *replaced_box = calloc(1, sizeof(struct box));
	replaced_box->type = BOX_INLINE;
	replaced_box->flags |= REPLACE_DIM; /* Mark as replaced element */
	replaced_box->height = 40;
	replaced_box->margin[TOP] = 5;
	replaced_box->margin[BOTTOM] = 5;
	replaced_box->border[TOP].width = 2;
	replaced_box->border[BOTTOM].width = 2;
	replaced_box->padding[TOP] = 10;
	replaced_box->padding[BOTTOM] = 10;

	ck_assert(lh__box_is_replace(replaced_box));
	/* Replaced inline baseline is bottom margin edge: 5 + 2 + 10 + 40 + 10 + 2 + 5 = 74 */
	ck_assert_int_eq(layout_box_get_baseline(replaced_box), 74);

	free(replaced_box);
}
END_TEST

START_TEST(test_vertical_align_deltas)
{
	/* Test alignment offsets between text box and inline-block */
	struct box *text = calloc(1, sizeof(struct box));
	text->type = BOX_TEXT;
	text->height = 20;

	struct box *ib = calloc(1, sizeof(struct box));
	ib->type = BOX_INLINE_BLOCK;
	ib->height = 40;
	struct box *ib_child = calloc(1, sizeof(struct box));
	ib_child->type = BOX_TEXT;
	ib_child->height = 20;
	ib->children = ib_child;

	int text_base = layout_box_get_baseline(text); /* 16 */
	int ib_base = layout_box_get_baseline(ib);     /* 16 */

	int max_baseline = text_base > ib_base ? text_base : ib_base; /* 16 */

	int delta_text = max_baseline - text_base; /* 0 */
	int delta_ib = max_baseline - ib_base;     /* 0 */

	ck_assert_int_eq(delta_text, 0);
	ck_assert_int_eq(delta_ib, 0);

	free(ib_child);
	free(ib);
	free(text);
}
END_TEST

Suite *layout_vertical_align_suite(void)
{
	Suite *s = suite_create("layout_vertical_align");
	TCase *tc = tcase_create("core");
	tcase_add_test(tc, test_outer_height_calculation);
	tcase_add_test(tc, test_baseline_calculation);
	tcase_add_test(tc, test_replaced_inline_baseline);
	tcase_add_test(tc, test_vertical_align_deltas);
	suite_add_tcase(s, tc);
	return s;
}

int main(void)
{
	Suite *s = layout_vertical_align_suite();
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr, CK_ENV);
	int failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
