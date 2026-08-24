#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <wisp/utils/errors.h>
#include <wisp/utils/nsurl.h>
#include <wisp/utils/corestrings.h>
#include <wisp/utils/file.h>
#include <wisp/content/content.h>
#include <wisp/content/content_protected.h>
#include "content/content_factory.h"
#include <wisp/content/hlcache.h>
#include <wisp/content/backing_store.h>
#include <wisp/misc.h>
#include <wisp/desktop/gui_internal.h>
#include <wisp/utils/nsoption.h>

static void (*scheduled_cb)(void *p) = NULL;
static void *scheduled_p = NULL;

/* Mock schedule function for guit */
static nserror mock_schedule(int t, void (*cb)(void *p), void *p)
{
    if (t >= 0) {
        scheduled_cb = cb;
        scheduled_p = p;
    } else {
        if (scheduled_cb == cb) {
            scheduled_cb = NULL;
            scheduled_p = NULL;
        }
    }
    return NSERROR_OK;
}

static nserror sync_check_callback(hlcache_handle *handle, const hlcache_event *event, void *pw)
{
    hlcache_handle **result_ptr = (hlcache_handle **)pw;
    (void)event;
    if (result_ptr != NULL) {
        /* Verify that *result_ptr is ALREADY set to handle when catchup callback executes */
        ck_assert_ptr_eq(*result_ptr, handle);
    }
    return NSERROR_OK;
}

static void pump_scheduled(void)
{
    int safety = 0;
    while (scheduled_cb != NULL && safety++ < 10) {
        void (*cb)(void *p) = scheduled_cb;
        void *p = scheduled_p;
        scheduled_cb = NULL;
        scheduled_p = NULL;
        cb(p);
    }
}

static struct gui_misc_table mock_misc = {
    .schedule = mock_schedule,
};

static struct wisp_table mock_gui_table = {
    .misc = &mock_misc,
};

extern struct wisp_table *guit;

static nserror dummy_llcache_callback(llcache_handle *handle, const llcache_event *event, void *pw)
{
    (void)handle; (void)event; (void)pw;
    return NSERROR_OK;
}

static nserror dummy_create(const struct content_handler *handler,
        lwc_string *imime_type, const struct http_parameter *params,
        struct llcache_handle *llcache, const char *fallback_charset,
        bool quirks, struct content **c)
{
    (void)imime_type; (void)params;
    (void)fallback_charset; (void)quirks;
    struct content *content = calloc(1, sizeof(struct content));
    if (!content) return NSERROR_NOMEM;
    content->handler = handler;
    lwc_intern_string("image/svg+xml", 13, &content->mime_type);
    content->llcache = llcache;
    llcache_handle_change_callback(content->llcache, dummy_llcache_callback, content);
    content->user_list = calloc(1, sizeof(struct content_user));
    content->status = CONTENT_STATUS_DONE;
    *c = content;
    return NSERROR_OK;
}

static content_type dummy_type(void)
{
    return CONTENT_IMAGE;
}

static const struct content_handler dummy_handler = {
    .create = dummy_create,
    .type = dummy_type,
};

static nserror dummy_callback(hlcache_handle *handle, const hlcache_event *event, void *pw)
{
    (void)handle; (void)event; (void)pw;
    return NSERROR_OK;
}

static nserror reentrant_callback(hlcache_handle *handle, const hlcache_event *event, void *pw)
{
    int *count = (int *)pw;
    if (count) (*count)++;
    if (event->type == CONTENT_MSG_LOADING || event->type == CONTENT_MSG_DONE) {
        /* Release handle reentrantly during callback */
        hlcache_handle_release(handle);
    }
    return NSERROR_OK;
}

START_TEST(test_hlcache_null_checks)
{
    guit = &mock_gui_table;

    ck_assert_ptr_null(hlcache_handle_get_url(NULL));
    ck_assert_ptr_null(hlcache_handle_get_content(NULL));
    ck_assert_int_eq(hlcache_handle_abort(NULL), NSERROR_OK);
    ck_assert_int_eq(hlcache_handle_release(NULL), NSERROR_OK);
    ck_assert_int_eq(hlcache_handle_replace_callback(NULL, dummy_callback, NULL), NSERROR_BAD_PARAMETER);
    ck_assert_int_eq(hlcache_handle_clone(NULL, NULL), NSERROR_BAD_PARAMETER);

    hlcache_handle *res = NULL;
    uint8_t buf[16] = "test";
    ck_assert_int_eq(hlcache_handle_retrieve_buffer(NULL, 4, "text/plain", dummy_callback, NULL, NULL, CONTENT_HTML, &res), NSERROR_BAD_PARAMETER);
    ck_assert_int_eq(hlcache_handle_retrieve_buffer(buf, 4, "text/plain", NULL, NULL, NULL, CONTENT_HTML, &res), NSERROR_BAD_PARAMETER);
    ck_assert_int_eq(hlcache_handle_retrieve_buffer(buf, 4, "text/plain", dummy_callback, NULL, NULL, CONTENT_HTML, NULL), NSERROR_BAD_PARAMETER);
}
END_TEST

START_TEST(test_hlcache_init_and_buffer_retrieval)
{
    guit = &mock_gui_table;
    guit->llcache = filesystem_llcache_table;
    guit->file = default_file_table;

    content_factory_register_handler("image/svg+xml", &dummy_handler);

    struct hlcache_parameters params = {
        .bg_clean_time = 10000,
        .llcache = {
            .limit = 1024 * 1024,
        },
    };

    nserror error = hlcache_initialise(&params);
    ck_assert_int_eq(error, NSERROR_OK);

    uint8_t data[32] = "<svg><rect/></svg>";
    hlcache_handle *handle1 = NULL;
    error = hlcache_handle_retrieve_buffer(data, strlen((char *)data), "image/svg+xml", dummy_callback, NULL, NULL, CONTENT_IMAGE, &handle1);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(handle1);

    pump_scheduled();

    nsurl *url1 = hlcache_handle_get_url(handle1);
    ck_assert_ptr_nonnull(url1);

    /* Retrieve same buffer again to test deduplication / cache hit */
    hlcache_handle *handle2 = NULL;
    error = hlcache_handle_retrieve_buffer(data, strlen((char *)data), "image/svg+xml", dummy_callback, NULL, NULL, CONTENT_IMAGE, &handle2);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(handle2);

    /* Test handle clone */
    hlcache_handle *cloned = NULL;
    error = hlcache_handle_clone(handle1, &cloned);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(cloned);

    /* Release handles */
    ck_assert_int_eq(hlcache_handle_release(cloned), NSERROR_OK);
    ck_assert_int_eq(hlcache_handle_release(handle2), NSERROR_OK);
    ck_assert_int_eq(hlcache_handle_release(handle1), NSERROR_OK);

    hlcache_stop();
    hlcache_finalise();
}
END_TEST

START_TEST(test_hlcache_reentrancy)
{
    guit = &mock_gui_table;
    guit->llcache = filesystem_llcache_table;
    guit->file = default_file_table;

    content_factory_register_handler("image/svg+xml", &dummy_handler);

    struct hlcache_parameters params = {
        .bg_clean_time = 10000,
        .llcache = {
            .limit = 1024 * 1024,
        },
    };

    nserror error = hlcache_initialise(&params);
    ck_assert_int_eq(error, NSERROR_OK);

    uint8_t data[32] = "<svg><rect/></svg>";
    hlcache_handle *handle1 = NULL;
    error = hlcache_handle_retrieve_buffer(data, strlen((char *)data), "image/svg+xml", dummy_callback, NULL, NULL, CONTENT_IMAGE, &handle1);
    ck_assert_int_eq(error, NSERROR_OK);

    pump_scheduled();

    /* Second retrieval with reentrant callback that releases handle inside callback */
    int cb_count = 0;
    hlcache_handle *handle2 = NULL;
    error = hlcache_handle_retrieve_buffer(data, strlen((char *)data), "image/svg+xml", reentrant_callback, &cb_count, NULL, CONTENT_IMAGE, &handle2);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_int_ge(cb_count, 1);
    ck_assert_ptr_null(handle2);

    ck_assert_int_eq(hlcache_handle_release(handle1), NSERROR_OK);

    hlcache_stop();
    hlcache_finalise();
}
END_TEST

START_TEST(test_hlcache_abort_and_replace_callback)
{
    guit = &mock_gui_table;
    guit->llcache = filesystem_llcache_table;
    guit->file = default_file_table;

    content_factory_register_handler("image/svg+xml", &dummy_handler);

    struct hlcache_parameters params = {
        .bg_clean_time = 10000,
        .llcache = {
            .limit = 1024 * 1024,
        },
    };

    nserror error = hlcache_initialise(&params);
    ck_assert_int_eq(error, NSERROR_OK);

    uint8_t data[32] = "<svg><circle/></svg>";
    hlcache_handle *handle = NULL;
    error = hlcache_handle_retrieve_buffer(data, strlen((char *)data), "image/svg+xml", dummy_callback, NULL, NULL, CONTENT_IMAGE, &handle);
    ck_assert_int_eq(error, NSERROR_OK);

    pump_scheduled();

    /* Test replacing callback */
    error = hlcache_handle_replace_callback(handle, dummy_callback, (void *)0x1234);
    ck_assert_int_eq(error, NSERROR_OK);

    /* Test aborting handle */
    error = hlcache_handle_abort(handle);
    ck_assert_int_eq(error, NSERROR_OK);

    ck_assert_int_eq(hlcache_handle_release(handle), NSERROR_OK);

    hlcache_stop();
    hlcache_finalise();
}
END_TEST

START_TEST(test_hlcache_sync_result_populated_during_catchup)
{
    guit = &mock_gui_table;
    guit->llcache = filesystem_llcache_table;
    guit->file = default_file_table;

    content_factory_register_handler("image/svg+xml", &dummy_handler);

    struct hlcache_parameters params = {
        .bg_clean_time = 10000,
        .llcache = {
            .limit = 1024 * 1024,
        },
    };

    nserror error = hlcache_initialise(&params);
    ck_assert_int_eq(error, NSERROR_OK);

    uint8_t data[32] = "<svg><path/></svg>";
    hlcache_handle *handle1 = NULL;
    error = hlcache_handle_retrieve_buffer(data, strlen((char *)data), "image/svg+xml", dummy_callback, NULL, NULL, CONTENT_IMAGE, &handle1);
    ck_assert_int_eq(error, NSERROR_OK);

    pump_scheduled();

    /* Second retrieval triggers cache HIT with synchronous catchup callbacks */
    hlcache_handle *handle2 = NULL;
    error = hlcache_handle_retrieve_buffer(data, strlen((char *)data), "image/svg+xml", sync_check_callback, &handle2, NULL, CONTENT_IMAGE, &handle2);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(handle2);

    ck_assert_int_eq(hlcache_handle_release(handle2), NSERROR_OK);
    ck_assert_int_eq(hlcache_handle_release(handle1), NSERROR_OK);

    hlcache_stop();
    hlcache_finalise();
}
END_TEST

START_TEST(test_hlcache_finalise_with_pending_retrieval_ctx)
{
    guit = &mock_gui_table;
    guit->llcache = filesystem_llcache_table;
    guit->file = default_file_table;

    content_factory_register_handler("image/svg+xml", &dummy_handler);

    struct hlcache_parameters params = {
        .bg_clean_time = 10000,
        .llcache = {
            .limit = 1024 * 1024,
        },
    };

    nserror error = hlcache_initialise(&params);
    ck_assert_int_eq(error, NSERROR_OK);

    nsurl *url = NULL;
    error = nsurl_create("http://example.com/test.css", &url);
    ck_assert_int_eq(error, NSERROR_OK);

    hlcache_handle *handle = NULL;
    error = hlcache_handle_retrieve(url, 0, NULL, NULL, dummy_callback, NULL, NULL, CONTENT_CSS, &handle);
    ck_assert_int_eq(error, NSERROR_NEED_DATA);
    ck_assert_ptr_nonnull(handle);

    /* Finalise hlcache while handle is still in pending retrieval context ring */
    hlcache_stop();
    hlcache_finalise();

    /* Re-releasing handle after hlcache_finalise disarmed it should safely succeed */
    ck_assert_int_eq(hlcache_handle_release(handle), NSERROR_OK);

    nsurl_unref(url);
}
END_TEST

static Suite *hlcache_suite(void)
{
    Suite *s = suite_create("hlcache");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_hlcache_null_checks);
    tcase_add_test(tc_core, test_hlcache_init_and_buffer_retrieval);
    tcase_add_test(tc_core, test_hlcache_reentrancy);
    tcase_add_test(tc_core, test_hlcache_abort_and_replace_callback);
    tcase_add_test(tc_core, test_hlcache_sync_result_populated_during_catchup);
    tcase_add_test(tc_core, test_hlcache_finalise_with_pending_retrieval_ctx);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;

    if (nsoption_init(NULL, NULL, NULL) != NSERROR_OK) {
        return EXIT_FAILURE;
    }

    if (corestrings_init() != NSERROR_OK) {
        return EXIT_FAILURE;
    }

    Suite *s = hlcache_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    corestrings_fini();
    nsoption_finalise(NULL, NULL);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
