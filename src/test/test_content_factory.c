#include <check.h>
#include <stdlib.h>
#include <wisp/utils/errors.h>
#include <wisp/content/content.h>
#include <wisp/content/content_protected.h>
#include "content/content_factory.h"
#include "utils/http/parameter.h"

// Forward declaration of internal function to make it available for testing
extern void content_factory_fini(void);

// Dummy content handler methods
static nserror dummy_create(const struct content_handler *handler,
        lwc_string *imime_type, const struct http_parameter *params,
        struct llcache_handle *llcache, const char *fallback_charset,
        bool quirks, struct content **c)
{
    (void)handler; (void)imime_type; (void)params; (void)llcache;
    (void)fallback_charset; (void)quirks;
    *c = malloc(sizeof(struct content));
    if (!*c) return NSERROR_NOMEM;
    return NSERROR_OK;
}

static content_type dummy_type(void)
{
    return CONTENT_HTML;
}

static content_type dummy_type2(void)
{
    return CONTENT_CSS;
}


// Dummy content handler objects
static const struct content_handler dummy_handler1 = {
    .create = dummy_create,
    .type = dummy_type,
};

static const struct content_handler dummy_handler2 = {
    .create = dummy_create,
    .type = dummy_type2,
};

static nserror dummy_clone_success(const struct content *old, struct content **newc)
{
    (void)old;
    *newc = malloc(sizeof(struct content));
    if (!*newc) return NSERROR_NOMEM;
    return NSERROR_OK;
}

static nserror dummy_clone_failure(const struct content *old, struct content **newc)
{
    (void)old;
    *newc = NULL;
    return NSERROR_CLONE_FAILED;
}

static const struct content_handler dummy_clone_handler = {
    .create = dummy_create,
    .clone = dummy_clone_success,
    .type = dummy_type,
};

static const struct content_handler dummy_clone_fail_handler = {
    .create = dummy_create,
    .clone = dummy_clone_failure,
    .type = dummy_type,
};


START_TEST(test_content_factory_register)
{
    nserror error;
    lwc_string *mime1, *mime2;
    content_type t1, t2;

    lwc_intern_string("text/html", 9, &mime1);
    lwc_intern_string("text/css", 8, &mime2);

    // Register first handler
    error = content_factory_register_handler("text/html", &dummy_handler1);
    ck_assert_int_eq(error, NSERROR_OK);

    // Verify it was registered
    t1 = content_factory_type_from_mime_type(mime1);
    ck_assert_int_eq(t1, CONTENT_HTML);

    // Register second handler for same mime type (should overwrite)
    error = content_factory_register_handler("text/html", &dummy_handler2);
    ck_assert_int_eq(error, NSERROR_OK);

    // Verify it was overwritten
    t1 = content_factory_type_from_mime_type(mime1);
    ck_assert_int_eq(t1, CONTENT_CSS);

    // Register for different mime type
    error = content_factory_register_handler("text/css", &dummy_handler1);
    ck_assert_int_eq(error, NSERROR_OK);

    // Verify
    t2 = content_factory_type_from_mime_type(mime2);
    ck_assert_int_eq(t2, CONTENT_HTML);

    lwc_string_unref(mime1);
    lwc_string_unref(mime2);

    content_factory_fini();
}
END_TEST

START_TEST(test_content_factory_register_null)
{
    nserror error;

    // Test with an empty handler to ensure no crashes in content_factory_fini
    static const struct content_handler empty_handler = {0};

    error = content_factory_register_handler("application/json", &empty_handler);
    ck_assert_int_eq(error, NSERROR_OK);

    // Verify it handles looking up a non-existent MIME type
    lwc_string *mime_unknown;
    lwc_intern_string("text/unknown", 12, &mime_unknown);
    content_type unknown = content_factory_type_from_mime_type(mime_unknown);
    ck_assert_int_eq(unknown, CONTENT_NONE);
    lwc_string_unref(mime_unknown);

    content_factory_fini();
}
END_TEST

START_TEST(test_content_clone)
{
    struct content old = {0};

    /* Test successful clone */
    old.handler = &dummy_clone_handler;
    struct content *cloned = content_clone(&old);
    ck_assert_ptr_nonnull(cloned);
    free(cloned);

    /* Test failed clone */
    old.handler = &dummy_clone_fail_handler;
    struct content *failed_clone = content_clone(&old);
    ck_assert_ptr_null(failed_clone);
}
END_TEST

static Suite *content_factory_suite(void)
{
    Suite *s = suite_create("content_factory");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_content_factory_register);
    tcase_add_test(tc_core, test_content_factory_register_null);

    TCase *tc_clone = tcase_create("Clone");
    tcase_add_test(tc_clone, test_content_clone);
    suite_add_tcase(s, tc_clone);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = content_factory_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
