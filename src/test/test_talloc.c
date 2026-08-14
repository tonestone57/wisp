#include <check.h>
#include <stdlib.h>
#include <string.h>

/* Use correct path for internal header */
#include "utils/talloc.h"

static int destructor_called = 0;

static int success_destructor(void *ptr)
{
    destructor_called = 1;
    return 0;
}

static int fail_destructor(void *ptr)
{
    destructor_called = 1;
    return -1;
}

START_TEST(test_talloc_set_destructor_success)
{
    void *ctx = talloc_new(NULL);
    void *ptr = talloc_size(ctx, 10);

    destructor_called = 0;
    talloc_set_destructor(ptr, success_destructor);

    int ret = talloc_free(ptr);
    ck_assert_int_eq(ret, 0);
    ck_assert_int_eq(destructor_called, 1);

    talloc_free(ctx);
}
END_TEST

START_TEST(test_talloc_set_destructor_fail)
{
    void *ctx = talloc_new(NULL);
    void *ptr = talloc_size(ctx, 10);

    destructor_called = 0;
    talloc_set_destructor(ptr, fail_destructor);

    int ret = talloc_free(ptr);
    ck_assert_int_eq(ret, -1);
    ck_assert_int_eq(destructor_called, 1);

    /* In talloc, if a destructor returns -1, the free operation fails, and the memory remains allocated.
       We have to explicitly clear the destructor to allow the context to be freed successfully,
       otherwise talloc_free(ctx) will still see the failed child destructor and abort or leak. */
    talloc_set_destructor(ptr, NULL);
    talloc_free(ctx);
}
END_TEST


START_TEST(test_talloc_free_children_null)
{
    /* Should just return and not crash */
    talloc_free_children(NULL);
}
END_TEST

START_TEST(test_talloc_free_children_no_children)
{
    void *ctx = talloc_new(NULL);
    talloc_free_children(ctx);
    talloc_free(ctx);
}
END_TEST

START_TEST(test_talloc_free_children_success)
{
    void *parent = talloc_new(NULL);
    void *child1 = talloc_size(parent, 10);
    void *child2 = talloc_size(parent, 20);

    (void)child1;
    (void)child2;

    /* Test normal scenario where children can be freed */
    talloc_free_children(parent);

    talloc_free(parent);
}
END_TEST

START_TEST(test_talloc_free_children_destructor_fail_with_grandparent)
{
    void *grandparent = talloc_new(NULL);
    void *parent = talloc_new(grandparent);
    void *child = talloc_size(parent, 10);

    talloc_set_destructor(child, fail_destructor);

    talloc_free_children(parent);

    /* child should now be reparented to grandparent */
    ck_assert_ptr_eq(talloc_parent(child), grandparent);

    talloc_set_destructor(child, NULL);
    talloc_free(grandparent);
}
END_TEST

START_TEST(test_talloc_free_children_destructor_fail_no_parent)
{
    void *parent = talloc_new(NULL);
    void *child = talloc_size(parent, 10);

    talloc_set_destructor(child, fail_destructor);

    talloc_free_children(parent);

    /* child should now be reparented to null_context */
    ck_assert_ptr_eq(talloc_parent(child), NULL);

    talloc_set_destructor(child, NULL);
    talloc_free(child);
    talloc_free(parent);
}
END_TEST

START_TEST(test_talloc_free_children_destructor_fail_with_ref)
{
    void *ref_ctx = talloc_new(NULL);
    void *parent = talloc_new(NULL);
    void *child = talloc_size(parent, 10);

    /* create a reference */
    talloc_reference(ref_ctx, child);

    talloc_set_destructor(child, fail_destructor);

    talloc_free_children(parent);

    /* child should now be reparented to ref_ctx */
    ck_assert_ptr_eq(talloc_parent(child), ref_ctx);

    talloc_set_destructor(child, NULL);
    talloc_free(ref_ctx);
    talloc_free(parent);
}
END_TEST


START_TEST(test_talloc_free_null)
{
    int ret = talloc_free(NULL);
    ck_assert_int_eq(ret, -1);
}
END_TEST

START_TEST(test_talloc_free_ref_not_child)
{
    void *ctx1 = talloc_new(NULL);
    void *ctx2 = talloc_new(NULL);

    talloc_reference(ctx1, ctx2);

    /* talloc_free(ctx2) should fail (-1) because it has references and the reference is not from a child */
    int ret = talloc_free(ctx2);
    ck_assert_int_eq(ret, -1);

    /* Freeing ctx1 removes the reference, so we can free ctx2 afterwards */
    talloc_free(ctx1);
    talloc_free(ctx2);
}
END_TEST

START_TEST(test_talloc_free_ref_child)
{
    void *parent = talloc_new(NULL);
    void *child = talloc_new(parent);

    talloc_reference(child, parent);

    /* Freeing parent while referenced by child.
       Since child is a child of parent, talloc_free(parent) should succeed and free both. */
    int ret = talloc_free(parent);
    ck_assert_int_eq(ret, 0);
}
END_TEST

static void *global_parent_ctx = NULL;

static int child_loop_destructor(void *ptr)
{
    if (global_parent_ctx) {
        /* Should return 0 due to TALLOC_FLAG_LOOP stop */
        int ret = talloc_free(global_parent_ctx);
        ck_assert_int_eq(ret, 0);
    }
    return 0;
}

START_TEST(test_talloc_free_loop_child)
{
    global_parent_ctx = talloc_new(NULL);
    void *child = talloc_new(global_parent_ctx);
    talloc_set_destructor(child, child_loop_destructor);

    int ret = talloc_free(global_parent_ctx);
    ck_assert_int_eq(ret, 0);
    global_parent_ctx = NULL;
}
END_TEST

static int self_free_destructor(void *ptr)
{
    int ret = talloc_free(ptr);
    ck_assert_int_eq(ret, -1);
    return 0;
}

START_TEST(test_talloc_free_self_destructor)
{
    void *ctx = talloc_new(NULL);
    talloc_set_destructor(ctx, self_free_destructor);

    int ret = talloc_free(ctx);
    ck_assert_int_eq(ret, 0);
}
END_TEST

static TCase *talloc_case_create(void)
{
    TCase *tc;
    tc = tcase_create("Talloc");
    tcase_add_test(tc, test_talloc_set_destructor_success);
    tcase_add_test(tc, test_talloc_set_destructor_fail);
    tcase_add_test(tc, test_talloc_free_children_null);
    tcase_add_test(tc, test_talloc_free_children_no_children);
    tcase_add_test(tc, test_talloc_free_children_success);
    tcase_add_test(tc, test_talloc_free_children_destructor_fail_with_grandparent);
    tcase_add_test(tc, test_talloc_free_children_destructor_fail_no_parent);
    tcase_add_test(tc, test_talloc_free_children_destructor_fail_with_ref);

    tcase_add_test(tc, test_talloc_free_null);
    tcase_add_test(tc, test_talloc_free_ref_not_child);
    tcase_add_test(tc, test_talloc_free_ref_child);
    tcase_add_test(tc, test_talloc_free_loop_child);
    tcase_add_test(tc, test_talloc_free_self_destructor);
    return tc;

}

static Suite *talloc_suite_create(void)
{
    Suite *s;
    s = suite_create("Talloc Utils");
    suite_add_tcase(s, talloc_case_create());
    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    SRunner *sr;
    sr = srunner_create(talloc_suite_create());
    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
