#include <check.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "wisp/content/handlers/html/private.h"

START_TEST(test_dirty_grid_spatial_mapping)
{
    struct html_content content;
    memset(&content, 0, sizeof(content));

    /* 1. Initialize dirty grid lifecycle */
    content.dirty_grid = hashset_create(4);
    ck_assert(content.dirty_grid != NULL);

    /* 2. Test Single Tile Inner Coordinates (0..255 -> Tile 0,0) */
    struct rect r1 = { .x0 = 10, .y0 = 10, .x1 = 100, .y1 = 100 };
    html_mark_grid_dirty(&content, &r1);

    /* Tile key calculation: tx=0, ty=0 -> (0 << 32) | 0 = 0 */
    uint64_t tile_key_0_0 = 0;

    /* Check if tile 0,0 is inserted */
    uint64_t stored_val = tile_key_0_0 + 1;
    bool found = false;
    for (unsigned int i = 0; i < content.dirty_grid->capacity; i++) {
        if (content.dirty_grid->keys[i] == stored_val) {
            found = true;
            break;
        }
    }
    ck_assert(found);
    ck_assert_int_eq(content.dirty_grid->count, 1);

    /* 3. Test Boundary Straddling (crosses x=256 and y=256)
     * Rect from (250, 250) to (270, 270) must span tiles (0,0), (1,0), (0,1), (1,1) */
    struct rect r2 = { .x0 = 250, .y0 = 250, .x1 = 270, .y1 = 270 };
    html_mark_grid_dirty(&content, &r2);

    uint64_t tile_key_1_0 = ((uint64_t)1 << 32) | (uint32_t)0;
    uint64_t tile_key_0_1 = ((uint64_t)0 << 32) | (uint32_t)1;
    uint64_t tile_key_1_1 = ((uint64_t)1 << 32) | (uint32_t)1;

    uint64_t expected_keys[4] = { tile_key_0_0, tile_key_1_0, tile_key_0_1, tile_key_1_1 };
    for (int k = 0; k < 4; k++) {
        uint64_t val = expected_keys[k] + 1;
        found = false;
        for (unsigned int i = 0; i < content.dirty_grid->capacity; i++) {
            if (content.dirty_grid->keys[i] == val) {
                found = true;
                break;
            }
        }
        ck_assert(found);
    }
    ck_assert_int_eq(content.dirty_grid->count, 4);

    /* 4. Test Idempotency (marking same rect again shouldn't increase count) */
    html_mark_grid_dirty(&content, &r2);
    ck_assert_int_eq(content.dirty_grid->count, 4);

    /* 5. Test High-Density / Heavy Rehashing Trigger
     * Mark a large 2048x2048 rect (64 tiles) to force hashset open-address expansion */
    struct rect r3 = { .x0 = 0, .y0 = 0, .x1 = 2048, .y1 = 2048 };
    html_mark_grid_dirty(&content, &r3);
    ck_assert_int_eq(content.dirty_grid->count, 64);

    /* 6. Test Zero Width / Zero Height (should not modify count) */
    struct rect r_zero_w = { .x0 = 100, .y0 = 100, .x1 = 100, .y1 = 200 };
    html_mark_grid_dirty(&content, &r_zero_w);
    ck_assert_int_eq(content.dirty_grid->count, 64);

    struct rect r_zero_h = { .x0 = 100, .y0 = 100, .x1 = 200, .y1 = 100 };
    html_mark_grid_dirty(&content, &r_zero_h);
    ck_assert_int_eq(content.dirty_grid->count, 64);

    /* 7. Test Negative Coordinates clamping to 0 */
    struct rect r_neg = { .x0 = -100, .y0 = -100, .x1 = 50, .y1 = 50 };
    html_mark_grid_dirty(&content, &r_neg);
    /* Should map to (0,0) which is already in the set, so count remains 64 */
    ck_assert_int_eq(content.dirty_grid->count, 64);

    /* 8. Test Clear & Lifecycle Reset */
    hashset_clear(content.dirty_grid);
    ck_assert_int_eq(content.dirty_grid->count, 0);

    hashset_destroy(content.dirty_grid);
    content.dirty_grid = NULL;
}
END_TEST

Suite *dirty_grid_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("DirtyGrid");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_dirty_grid_spatial_mapping);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = dirty_grid_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
