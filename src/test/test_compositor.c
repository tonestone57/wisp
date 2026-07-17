#include <check.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <unistd.h>
#include <pthread.h>
#define ns_usleep(us) usleep(us)
#else
#include <windows.h>
#define ns_usleep(us) Sleep((us)/1000)
#endif

#include "wisp/desktop/compositor.h"
#include "wisp/utils/log.h"

/* Mock for nslog_log used by NSLOG macro */
void nslog_log(enum nslog_level level, const char *file, const char *func, int ln, const char *format, ...) {}

START_TEST(test_compositor_lifecycle)
{
    /* Test creating the compositor under different platform native APIs */
    wisp_compositor_t *comp_d3d = wisp_compositor_create(WISP_COMPOSITOR_API_D3D11, (void*)0x1111);
    ck_assert_ptr_nonnull(comp_d3d);
    ck_assert_int_eq(comp_d3d->api, WISP_COMPOSITOR_API_D3D11);
    ck_assert_ptr_eq(comp_d3d->native_window, (void*)0x1111);
    ck_assert_int_eq(comp_d3d->running, false);

    /* Test spawning and stopping the compositor thread */
    bool start_ok = wisp_compositor_start(comp_d3d);
    ck_assert_int_eq(start_ok, true);
    ck_assert_int_eq(comp_d3d->running, true);

    wisp_compositor_stop(comp_d3d);
    ck_assert_int_eq(comp_d3d->running, false);
    wisp_compositor_destroy(comp_d3d);

    /* Test Haiku specific BDirectWindow pathway */
    wisp_compositor_t *comp_haiku = wisp_compositor_create(WISP_COMPOSITOR_API_BDIRECTWINDOW, (void*)0x2222);
    ck_assert_ptr_nonnull(comp_haiku);
    ck_assert_ptr_null(comp_haiku->device_ctx.direct_window_info);

    wisp_compositor_destroy(comp_haiku);
}
END_TEST

START_TEST(test_compositor_textures)
{
    wisp_compositor_t *comp = wisp_compositor_create(WISP_COMPOSITOR_API_OPENGL_ES, (void*)0x3333);
    ck_assert_ptr_nonnull(comp);

    /* Initialize shared context (passes NULL as headless context) */
    wisp_compositor_initialize_egl_shared(comp, NULL);

    bool start_ok = wisp_compositor_start(comp);
    ck_assert_int_eq(start_ok, true);

    /* Allocate mock raw pixel data */
    size_t pixel_size = 512 * 512 * 4;
    unsigned char *mock_pixels = malloc(pixel_size);
    ck_assert_ptr_nonnull(mock_pixels);
    memset(mock_pixels, 0xAB, pixel_size);

    /* Create GPU-Shared texture representing a standard Redraw Tile (512x512) */
    wisp_texture_t *tex = wisp_compositor_get_tile_texture(comp, 100, 200, 512, mock_pixels);
    ck_assert_ptr_nonnull(tex);
    ck_assert_int_eq(tex->width, 512);
    ck_assert_int_eq(tex->height, 512);
    ck_assert_int_eq(tex->api, WISP_COMPOSITOR_API_OPENGL_ES);

    /* Verify backing store matches uploaded pixels (if raw_pixels is allocated in simulated mode) */
    if (tex->raw_pixels) {
        ck_assert_int_eq(((unsigned char*)tex->raw_pixels)[0], 0xAB);
        ck_assert_int_eq(((unsigned char*)tex->raw_pixels)[pixel_size - 1], 0xAB);
    }

    /* Retrieve from tile texture cache and verify it returns the identical cached texture! */
    wisp_texture_t *tex_cached = wisp_compositor_get_tile_texture(comp, 100, 200, 512, mock_pixels);
    ck_assert_ptr_eq(tex_cached, tex);

    /* Submit layered texture with custom affine matrix transform */
    float transform[6] = { 2.0f, 0.0f, 0.0f, 2.0f, 50.0f, 100.0f }; /* 2x Scale, Translate by (50, 100) */
    bool submit_ok = wisp_compositor_submit_texture(comp, tex, 10, 10, transform);
    ck_assert_int_eq(submit_ok, true);
    ck_assert_int_eq(comp->layer_count, 1);
    ck_assert_ptr_eq(comp->layers[0].texture, tex);
    ck_assert_int_eq(comp->layers[0].x, 10);
    ck_assert_int_eq(comp->layers[0].y, 10);
    ck_assert_float_eq(comp->layers[0].transform[0], 2.0f);
    ck_assert_float_eq(comp->layers[0].transform[4], 50.0f);

    /* Submit a second layer with default identity transform */
    wisp_texture_t *tex2 = wisp_compositor_get_tile_texture(comp, 300, 400, 256, mock_pixels);
    ck_assert_ptr_nonnull(tex2);
    bool submit_ok2 = wisp_compositor_submit_texture(comp, tex2, 100, 200, NULL);
    ck_assert_int_eq(submit_ok2, true);
    ck_assert_int_eq(comp->layer_count, 2);
    ck_assert_float_eq(comp->layers[1].transform[0], 1.0f); /* Default identity matrix scale */

    /* Trigger draw frame with scrolling offset */
    bool draw_ok = wisp_compositor_draw_frame(comp, 5.0f, 15.0f);
    ck_assert_int_eq(draw_ok, true);

    /* Wait a moment for the compositor thread to process the submitted frame */
    ns_usleep(100000);

    free(mock_pixels);

    wisp_compositor_stop(comp);
    wisp_compositor_destroy(comp);
}
END_TEST

Suite *compositor_suite(void)
{
    Suite *s = suite_create("WispCompositor");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_compositor_lifecycle);
    tcase_add_test(tc_core, test_compositor_textures);

    suite_add_tcase(s, tc_core);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = compositor_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
