/*
 * Benchmark test for table layout height calculation optimization
 */

#include <check.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "wisp/content/handlers/html/box.h"
#include "wisp/css.h"
#include "wisp/types.h"
#include "wisp/content/handlers/html/private.h"
#include "layout_margin_collapse_style.h"
#include "content/handlers/html/layout_internal.h"

/* Helper to create a dummy box */
static struct box *create_dummy_box(box_type type, css_computed_style *style)
{
	struct box *b = calloc(1, sizeof(struct box));
	b->type = type;
	b->width = 100;
	b->height = 20;
	b->style = style;
	return b;
}

static struct box *build_benchmark_table(int num_row_groups, int rows_per_group, int cols, css_computed_style *table_style, css_computed_style *row_style)
{
	struct box *table = create_dummy_box(BOX_TABLE, table_style);
	table->columns = cols;
	table->col = calloc(cols, sizeof(struct column));
	for (int i = 0; i < cols; i++) {
		table->col[i].type = COLUMN_WIDTH_FIXED;
		table->col[i].width = 100;
		table->col[i].min = 100;
		table->col[i].max = 100;
	}

	struct box *prev_rg = NULL;

	for (int g = 0; g < num_row_groups; g++) {
		struct box *rg = create_dummy_box(BOX_TABLE_ROW_GROUP, row_style);
		rg->parent = table;
		if (prev_rg) {
			prev_rg->next = rg;
		} else {
			table->children = rg;
		}
		prev_rg = rg;

		struct box *prev_row = NULL;
		for (int r = 0; r < rows_per_group; r++) {
			struct box *row = create_dummy_box(BOX_TABLE_ROW, row_style);
			row->parent = rg;
			if (prev_row) {
				prev_row->next = row;
			} else {
				rg->children = row;
			}
			prev_row = row;

			struct box *prev_cell = NULL;
			for (int c = 0; c < cols; c++) {
				struct box *cell = create_dummy_box(BOX_TABLE_CELL, row_style);
				cell->parent = row;
				cell->start_column = c;
				cell->columns = 1;
				cell->rows = 1;
				if (prev_cell) {
					prev_cell->next = cell;
				} else {
					row->children = cell;
				}
				prev_cell = cell;
			}
		}
	}

	return table;
}

static void free_benchmark_table(struct box *table)
{
	if (!table) return;
	struct box *rg = table->children;
	while (rg) {
		struct box *next_rg = rg->next;
		struct box *row = rg->children;
		while (row) {
			struct box *next_row = row->next;
			struct box *cell = row->children;
			while (cell) {
				struct box *next_cell = cell->next;
				free(cell);
				cell = next_cell;
			}
			free(row);
			row = next_row;
		}
		free(rg);
		rg = next_rg;
	}
	if (table->col) free(table->col);
	free(table);
}

START_TEST(benchmark_table_height_distribution)
{
	html_content content = {0};
	content.unit_len_ctx.viewport_width = INTTOFIX(1024);
	content.unit_len_ctx.viewport_height = INTTOFIX(768);

	css_computed_style *table_style = create_block_style();
	css_computed_style *row_style = create_block_style();

	/* Set specified height on table to 200,000px so extra_height > 0 */
	style_set_height_px(table_style, 200000);

	/* 50 row groups, 100 rows per group = 5,000 rows, 4 columns */
	int num_rgs = 50;
	int rows_per_rg = 100;
	int cols = 4;

	struct box *table = build_benchmark_table(num_rgs, rows_per_rg, cols, table_style, row_style);

	/* We measure layout_table execution time over iterations */
	int iterations = 500;
	clock_t start = clock();

	for (int i = 0; i < iterations; i++) {
		/* Reset flags so layout_table actually runs */
		table->flags |= DIRTY_LAYOUT;
		layout_table(table, 800, &content);
	}

	clock_t end = clock();
	double elapsed_ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
	printf("\n[BENCHMARK] Table layout with extra height distribution (5000 rows, %d iterations): %.2f ms (%.4f ms/iter)\n",
	       iterations, elapsed_ms, elapsed_ms / iterations);

	free_benchmark_table(table);
	destroy_mock_style(table_style);
	destroy_mock_style(row_style);
}
END_TEST

Suite *table_benchmark_suite(void)
{
	Suite *s = suite_create("table_benchmark");
	TCase *tc = tcase_create("benchmark");
	tcase_set_timeout(tc, 120.0);
	tcase_add_test(tc, benchmark_table_height_distribution);
	suite_add_tcase(s, tc);
	return s;
}

int main(void)
{
	Suite *s = table_benchmark_suite();
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr, CK_ENV);
	int failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
