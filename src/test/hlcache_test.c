#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <wisp/utils/errors.h>
#include <wisp/utils/nsurl.h>
#include <wisp/utils/corestrings.h>
#include <wisp/utils/file.h>
#include <wisp/content/content.h>
#include <wisp/content/fetch.h>
#include <wisp/content/content_protected.h>
#include "content/content_factory.h"
#include "content/fetchers.h"
#include <wisp/content/hlcache.h>
#include <wisp/content/backing_store.h>
#include <wisp/misc.h>
#include <wisp/desktop/gui_internal.h>
#include <wisp/fetch.h>
#include <wisp/utils/nsoption.h>
#include <wisp/content/handlers/html/html.h>

struct mock_schedule_item {
    void (*cb)(void *p);
    void *p;
    int t;
};

static struct mock_schedule_item scheduled_queue[128];
static int scheduled_count = 0;

/* Mock schedule function for guit */
static nserror mock_schedule(int t, void (*cb)(void *p), void *p)
{
    if (t >= 0) {
        if (scheduled_count < 128) {
            scheduled_queue[scheduled_count].cb = cb;
            scheduled_queue[scheduled_count].p = p;
            scheduled_queue[scheduled_count].t = t;
            scheduled_count++;
        }
    } else {
        for (int i = 0; i < scheduled_count; i++) {
            if (scheduled_queue[i].cb == cb && (p == NULL || scheduled_queue[i].p == p)) {
                for (int j = i; j < scheduled_count - 1; j++) {
                    scheduled_queue[j] = scheduled_queue[j + 1];
                }
                scheduled_count--;
                i--;
            }
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
    int count = scheduled_count;
    int safety = 0;
    while (scheduled_count > 0 && count > 0 && safety++ < 100) {
        struct mock_schedule_item item = scheduled_queue[0];
        for (int i = 0; i < scheduled_count - 1; i++) {
            scheduled_queue[i] = scheduled_queue[i + 1];
        }
        scheduled_count--;
        count--;
        if (item.cb != NULL) {
            item.cb(item.p);
        }
    }
}

static nserror mock_get_resource_data(const char *path, const uint8_t **data, size_t *len)
{
    static const uint8_t dummy_css[] = "body { margin: 0; }";
    *data = dummy_css;
    *len = sizeof(dummy_css) - 1;
    return NSERROR_OK;
}

static struct gui_fetch_table mock_fetch_table = {
    .get_resource_data = mock_get_resource_data,
};

static struct gui_misc_table mock_misc = {
    .schedule = mock_schedule,
};

static struct wisp_table mock_gui_table = {
    .misc = &mock_misc,
    .fetch = &mock_fetch_table,
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

static int mock_reformat_count = 0;
static int mock_reformat_w = 0;
static int mock_reformat_h = 0;

static void mock_reformat_cb(struct content *c, int width, int height)
{
    (void)c;
    mock_reformat_count++;
    mock_reformat_w = width;
    mock_reformat_h = height;
}

static nserror reformat_dummy_create(const struct content_handler *handler,
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
    content->available_width = -1;
    content->available_height = -1;
    *c = content;
    return NSERROR_OK;
}

static const struct content_handler reformat_handler = {
    .create = reformat_dummy_create,
    .type = dummy_type,
    .reformat = mock_reformat_cb,
};

static int user_reformat_msg_count = 0;
static bool user_reformat_bg_val = false;

static void mock_content_user_cb(struct content *c, content_msg msg, const union content_msg_data *data, void *pw)
{
    (void)c; (void)pw;
    if (msg == CONTENT_MSG_REFORMAT) {
        user_reformat_msg_count++;
        if (data != NULL) {
            user_reformat_bg_val = data->background;
        }
    }
}

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
    hlcache_retrieve_options opts = {
        .accepted_types = CONTENT_HTML
    };
    ck_assert_int_eq(hlcache_handle_retrieve_buffer(NULL, 4, "text/plain", &opts, dummy_callback, NULL, &res), NSERROR_BAD_PARAMETER);
    ck_assert_int_eq(hlcache_handle_retrieve_buffer(buf, 4, "text/plain", &opts, NULL, NULL, &res), NSERROR_BAD_PARAMETER);
    ck_assert_int_eq(hlcache_handle_retrieve_buffer(buf, 4, "text/plain", &opts, dummy_callback, NULL, NULL), NSERROR_BAD_PARAMETER);
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
    hlcache_retrieve_options opts = {
        .accepted_types = CONTENT_IMAGE
    };
    error = hlcache_handle_retrieve_buffer(data, strlen((char *)data), "image/svg+xml", &opts, dummy_callback, NULL, &handle1);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(handle1);

    pump_scheduled();

    nsurl *url1 = hlcache_handle_get_url(handle1);
    ck_assert_ptr_nonnull(url1);

    /* Retrieve same buffer again to test deduplication / cache hit */
    hlcache_handle *handle2 = NULL;
    error = hlcache_handle_retrieve_buffer(data, strlen((char *)data), "image/svg+xml", &opts, dummy_callback, NULL, &handle2);
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

START_TEST(test_html_content_clone)
{
    guit = &mock_gui_table;
    guit->llcache = filesystem_llcache_table;
    guit->file = default_file_table;

    fetch_use_ipc = false;
    ck_assert_int_eq(fetcher_init(), NSERROR_OK);
    ck_assert_int_eq(html_init(), NSERROR_OK);

    struct hlcache_parameters params = {
        .bg_clean_time = 10000,
        .llcache = {
            .limit = 1024 * 1024,
        },
    };

    nserror error = hlcache_initialise(&params);
    ck_assert_int_eq(error, NSERROR_OK);

    uint8_t html_data[] = "<html><head><title>HTML Clone Test</title></head><body><p>Test</p></body></html>";
    hlcache_handle *html_handle = NULL;
    hlcache_retrieve_options opts = {
        .accepted_types = CONTENT_HTML
    };
    error = hlcache_handle_retrieve_buffer(html_data, strlen((char *)html_data), "text/html", &opts, dummy_callback, NULL, &html_handle);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(html_handle);

    pump_scheduled();

    struct content *c_orig = hlcache_handle_get_content(html_handle);
    ck_assert_ptr_nonnull(c_orig);

    struct content *c_cloned = content_clone(c_orig);
    ck_assert_ptr_nonnull(c_cloned);
    ck_assert_int_eq(c_cloned->handler->type(), CONTENT_HTML);

    content_destroy(c_cloned);
    ck_assert_int_eq(hlcache_handle_release(html_handle), NSERROR_OK);

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
    hlcache_retrieve_options opts = {
        .accepted_types = CONTENT_IMAGE
    };
    error = hlcache_handle_retrieve_buffer(data, strlen((char *)data), "image/svg+xml", &opts, dummy_callback, NULL, &handle1);
    ck_assert_int_eq(error, NSERROR_OK);

    pump_scheduled();

    /* Second retrieval with reentrant callback that releases handle inside callback */
    int cb_count = 0;
    hlcache_handle *handle2 = NULL;
    error = hlcache_handle_retrieve_buffer(data, strlen((char *)data), "image/svg+xml", &opts, reentrant_callback, &cb_count, &handle2);
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
    hlcache_retrieve_options opts = {
        .accepted_types = CONTENT_IMAGE
    };
    error = hlcache_handle_retrieve_buffer(data, strlen((char *)data), "image/svg+xml", &opts, dummy_callback, NULL, &handle);
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
    hlcache_retrieve_options opts = {
        .accepted_types = CONTENT_IMAGE
    };
    error = hlcache_handle_retrieve_buffer(data, strlen((char *)data), "image/svg+xml", &opts, dummy_callback, NULL, &handle1);
    ck_assert_int_eq(error, NSERROR_OK);

    pump_scheduled();

    /* Second retrieval triggers cache HIT with synchronous catchup callbacks */
    hlcache_handle *handle2 = NULL;
    error = hlcache_handle_retrieve_buffer(data, strlen((char *)data), "image/svg+xml", &opts, sync_check_callback, &handle2, &handle2);
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

    uint8_t data[32] = "<svg><ellipse/></svg>";
    hlcache_handle *handle = NULL;
    hlcache_retrieve_options opts = {
        .accepted_types = CONTENT_IMAGE
    };
    error = hlcache_handle_retrieve_buffer(data, strlen((char *)data), "image/svg+xml", &opts, dummy_callback, NULL, &handle);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(handle);

    /* Finalise hlcache while handle is still active in retrieval context ring */
    hlcache_stop();
    hlcache_finalise();

    /* Re-releasing handle after hlcache_finalise disarmed it should safely succeed */
    ck_assert_int_eq(hlcache_handle_release(handle), NSERROR_OK);
}
END_TEST

START_TEST(test_content_reformat)
{
    guit = &mock_gui_table;
    guit->llcache = filesystem_llcache_table;
    guit->file = default_file_table;

    /* Check NULL handle capability */
    ck_assert(!content_can_reformat(NULL));

    /* Register handler with NO reformat callback */
    content_factory_register_handler("image/svg+xml", &dummy_handler);

    struct hlcache_parameters params = {
        .bg_clean_time = 10000,
        .llcache = {
            .limit = 1024 * 1024,
        },
    };

    nserror error = hlcache_initialise(&params);
    ck_assert_int_eq(error, NSERROR_OK);

    uint8_t data1[32] = "<svg><polygon/></svg>";
    hlcache_handle *handle_no_reformat = NULL;
    hlcache_retrieve_options opts = {
        .accepted_types = CONTENT_IMAGE
    };
    error = hlcache_handle_retrieve_buffer(data1, strlen((char *)data1), "image/svg+xml", &opts, dummy_callback, NULL, &handle_no_reformat);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(handle_no_reformat);
    pump_scheduled();

    /* Handler without reformat function returns false for content_can_reformat */
    ck_assert(!content_can_reformat(handle_no_reformat));

    /* Release first handle */
    ck_assert_int_eq(hlcache_handle_release(handle_no_reformat), NSERROR_OK);
    hlcache_stop();
    hlcache_finalise();

    /* Register handler WITH reformat callback */
    mock_reformat_count = 0;
    mock_reformat_w = 0;
    mock_reformat_h = 0;
    user_reformat_msg_count = 0;
    user_reformat_bg_val = false;

    content_factory_register_handler("image/svg+xml", &reformat_handler);

    error = hlcache_initialise(&params);
    ck_assert_int_eq(error, NSERROR_OK);

    uint8_t data2[32] = "<svg><polyline/></svg>";
    hlcache_handle *handle = NULL;
    error = hlcache_handle_retrieve_buffer(data2, strlen((char *)data2), "image/svg+xml", &opts, dummy_callback, NULL, &handle);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(handle);
    pump_scheduled();

    /* Handler with reformat function returns true for content_can_reformat */
    ck_assert(content_can_reformat(handle));

    struct content *c = hlcache_handle_get_content(handle);
    ck_assert_ptr_nonnull(c);

    /* Add content user to verify CONTENT_MSG_REFORMAT broadcast */
    ck_assert(content_add_user(c, mock_content_user_cb, NULL));

    /* First reformat: background=true, 800x600 */
    content_reformat(handle, true, 800, 600);
    ck_assert_int_eq(mock_reformat_count, 1);
    ck_assert_int_eq(mock_reformat_w, 800);
    ck_assert_int_eq(mock_reformat_h, 600);
    ck_assert_int_eq(user_reformat_msg_count, 1);
    ck_assert(user_reformat_bg_val == true);
    ck_assert_int_eq(content_get_available_width(handle), 800);

    /* Second reformat with identical dimensions: should return early (no reformat callback or broadcast) */
    content_reformat(handle, false, 800, 600);
    ck_assert_int_eq(mock_reformat_count, 1);
    ck_assert_int_eq(user_reformat_msg_count, 1);

    /* Third reformat with new dimensions: background=false, 1024x768 */
    content_reformat(handle, false, 1024, 768);
    ck_assert_int_eq(mock_reformat_count, 2);
    ck_assert_int_eq(mock_reformat_w, 1024);
    ck_assert_int_eq(mock_reformat_h, 768);
    ck_assert_int_eq(user_reformat_msg_count, 2);
    ck_assert(user_reformat_bg_val == false);
    ck_assert_int_eq(content_get_available_width(handle), 1024);

    /* Cleanup */
    content_remove_user(c, mock_content_user_cb, NULL);
    ck_assert_int_eq(hlcache_handle_release(handle), NSERROR_OK);

    hlcache_stop();
    hlcache_finalise();
}
END_TEST

START_TEST(test_hlcache_clean_stale_and_fresh)
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

    uint8_t data[32] = "<svg><circle id='c'/></svg>";
    hlcache_handle *handle = NULL;
    hlcache_retrieve_options opts = {
        .accepted_types = CONTENT_IMAGE
    };
    error = hlcache_handle_retrieve_buffer(data, strlen((char *)data), "image/svg+xml", &opts, dummy_callback, NULL, &handle);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(handle);

    pump_scheduled();

    struct content *c = hlcache_handle_get_content(handle);
    ck_assert_ptr_nonnull(c);

    /* Release handle so content user count becomes 0 */
    ck_assert_int_eq(hlcache_handle_release(handle), NSERROR_OK);

    /* Trigger background cleanup (scheduled_cb = hlcache_clean, force_clean = false) */
    pump_scheduled();

    /* Buffer-retrieved synthetic object is uncached and considered stale/not fresh in background clean,
     * so it gets cleaned up. Re-retrieving should create a new handle.
     */
    hlcache_handle *handle2 = NULL;
    error = hlcache_handle_retrieve_buffer(data, strlen((char *)data), "image/svg+xml", &opts, dummy_callback, NULL, &handle2);
    ck_assert_int_eq(error, NSERROR_OK);
    ck_assert_ptr_nonnull(handle2);

    ck_assert_int_eq(hlcache_handle_release(handle2), NSERROR_OK);

    hlcache_stop();
    hlcache_finalise();
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
    tcase_add_test(tc_core, test_content_reformat);
    tcase_add_test(tc_core, test_hlcache_clean_stale_and_fresh);
    tcase_add_test(tc_core, test_html_content_clone);
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
