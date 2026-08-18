/*
 * Copyright 2016 Vincent Sanders <vince@netsurf-browser.org>
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 * Tests for utility functions.
 */

#include "utils/config.h"

#include <libwapcaplet/libwapcaplet.h>
#include <assert.h>
#include <check.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/corestrings.h"
#include "utils/utils.h"
#include <unistd.h>
#include "utils/string.h"

static void test_lwc_iterator(lwc_string *str, void *pw)
{
    unsigned *count = (unsigned *)pw;
    if (count != NULL) {
        (*count)++;
    }
    fprintf(stderr, "[lwc] [%3u] %.*s\n", str->refcnt, (int)lwc_string_length(str), lwc_string_data(str));
}

#define NELEMS(x) (sizeof(x) / sizeof((x)[0]))
#define SLEN(x) (sizeof((x)) - 1)

struct test_pairs {
    const unsigned long long int test;
    const char *res;
};

static const struct test_pairs human_friendly_bytesize_test_vec[] = {
    {0ULL, "0Bytes"},
    {0x2AULL, "42Bytes"},
    {0x400ULL, "1024Bytes"},
    {0x401ULL, "1.00KiBytes"},
    {0xA9AEULL, "42.42KiBytes"},
    {0x100000ULL, "1024.00KiBytes"},
    {0x100001ULL, "1.00MiBytes"},
    {0x2A6B852ULL, "42.42MiBytes"},
    {0x40000000ULL, "1024.00MiBytes"},
    {0x40000001ULL, "1.00GiBytes"},
    {0x80000000ULL, "2.00GiBytes"},
    {0xC0000000ULL, "3.00GiBytes"},
    {0x100000000ULL, "4.00GiBytes"},
    {0x10000000000ULL, "1024.00GiBytes"},
    {0x10000000001ULL, "1.00TiBytes"},
    {0x4000000000000ULL, "1024.00TiBytes"},
    {0x4000000000001ULL, "1.00PiBytes"},
    {0x1000000000000000ULL, "1024.00PiBytes"},
    {0x1000000000000100ULL, "1.00EiBytes"}, /* precision loss */
    {0xFFFFFFFFFFFFFFFFULL, "16.00EiBytes"},
};

/**
 * check each response one at a time
 */
START_TEST(human_friendly_bytesize_test)
{
    char *res_str;
    const struct test_pairs *tst = &human_friendly_bytesize_test_vec[_i];

    res_str = human_friendly_bytesize(tst->test);

    /* ensure result data is correct */
    ck_assert_str_eq(res_str, tst->res);
}
END_TEST

/**
 * check each response one after another
 */
START_TEST(human_friendly_bytesize_all_test)
{
    char *res_str;
    const struct test_pairs *tst;
    unsigned int idx;

    for (idx = 0; idx < NELEMS(human_friendly_bytesize_test_vec); idx++) {
        tst = &human_friendly_bytesize_test_vec[idx];

        res_str = human_friendly_bytesize(tst->test);

        /* ensure result data is correct */
        ck_assert_str_eq(res_str, tst->res);
    }
}
END_TEST

static TCase *human_friendly_bytesize_case_create(void)
{
    TCase *tc;
    tc = tcase_create("Human friendly bytesize");

    tcase_add_loop_test(tc, human_friendly_bytesize_test, 0, NELEMS(human_friendly_bytesize_test_vec));

    tcase_add_test(tc, human_friendly_bytesize_all_test);

    return tc;
}

struct test_strings {
    const char *test;
    const char *res;
};

static const struct test_strings squash_whitespace_test_vec[] = {
    {"", ""},
    {" ", " "},
    {"    ", " "},
    {" \n\r\t   ", " "},
    {" a ", " a "},
    {" a   b ", " a b "},
    {"   A string  with \t  \r \n  \t   lots\tof\nwhitespace\r    ", " A string with lots of whitespace "},
    {"  UTF-8 \t string \n with \r emojis 🚀 and \t accents \n éàè  ", " UTF-8 string with emojis 🚀 and accents éàè "},
};

START_TEST(squash_whitespace_test)
{
    char *res_str;
    const struct test_strings *tst = &squash_whitespace_test_vec[_i];

    res_str = squash_whitespace(tst->test);
    ck_assert(res_str != NULL);

    /* ensure result data is correct */
    ck_assert_str_eq(res_str, tst->res);

    free(res_str);
}
END_TEST

START_TEST(squash_whitespace_api_test)
{
    char *res_str;

    res_str = squash_whitespace(NULL);
    ck_assert(res_str == NULL);

    free(res_str);
}
END_TEST

static TCase *squash_whitespace_case_create(void)
{
    TCase *tc;
    tc = tcase_create("Squash whitespace");

    tcase_add_test(tc, squash_whitespace_api_test);

    tcase_add_loop_test(tc, squash_whitespace_test, 0, NELEMS(squash_whitespace_test_vec));

    return tc;
}


START_TEST(corestrings_init_fini_test)
{
    nserror res;

    res = corestrings_init();
    ck_assert_int_eq(res, NSERROR_OK);

    corestrings_fini();
}
END_TEST

START_TEST(corestrings_double_init_test)
{
    nserror res;

    res = corestrings_init();
    ck_assert_int_eq(res, NSERROR_OK);

    res = corestrings_init();
    ck_assert_int_eq(res, NSERROR_OK);

    corestrings_fini();
}
END_TEST

START_TEST(corestrings_double_fini_test)
{
    nserror res;

    res = corestrings_init();
    ck_assert_int_eq(res, NSERROR_OK);

    corestrings_fini();

    corestrings_fini();
}
END_TEST


static TCase *corestrings_case_create(void)
{
    TCase *tc;
    tc = tcase_create("Corestrings");

    tcase_add_test(tc, corestrings_init_fini_test);
    tcase_add_test(tc, corestrings_double_init_test);
    tcase_add_test(tc, corestrings_double_fini_test);

    return tc;
}


struct test_strings_space2nbsp {
    const char *test;
    const char *res;
};

static const struct test_strings_space2nbsp cnv_space2nbsp_test_vec[] = {
    {"", ""},
    {"test", "test"},
    {" ", "\xC2\xA0"},
    {"\t", "\xC2\xA0"},
    {" a \tb", "\xC2\xA0""a""\xC2\xA0\xC2\xA0""b"},
    {" A string  with \t whitespace ", "\xC2\xA0""A""\xC2\xA0""string""\xC2\xA0\xC2\xA0""with""\xC2\xA0\xC2\xA0\xC2\xA0""whitespace""\xC2\xA0"}
};

START_TEST(string_utils_cnv_space2nbsp_test)
{
    char *res;
    const struct test_strings_space2nbsp *tst = &cnv_space2nbsp_test_vec[_i];

    res = cnv_space2nbsp(tst->test);
    ck_assert(res != NULL);
    ck_assert_str_eq(res, tst->res);

    free(res);
}
END_TEST

START_TEST(string_utils_cnv_space2nbsp_api_test)
{
    char *res;

    res = cnv_space2nbsp(NULL);
    ck_assert(res == NULL);
}
END_TEST

START_TEST(string_utils_strcasestr_test)
{

    char *res;
    const char *haystack =
        "A big old long haystack string that has a small Needle in the middle of it with a different case";

    res = strcasestr(haystack, "notfound");
    ck_assert(res == NULL);

    res = strcasestr(haystack, "needle");
    ck_assert(res != NULL);

    ck_assert_str_eq(res, haystack + 48);
}
END_TEST

START_TEST(string_utils_strchrnul_test)
{

    char *res;
    const char *haystack =
        "A big old long haystack string that has a small Needle in the middle of it with a different case";

    res = strchrnul(haystack, 'Z');
    ck_assert(res != NULL);
    ck_assert(*res == 0);

    res = strchrnul(haystack, 'N');
    ck_assert(res != NULL);

    ck_assert_str_eq(res, haystack + 48);
}
END_TEST


static TCase *string_utils_case_create(void)
{
    TCase *tc;
    tc = tcase_create("String utilities");

    tcase_add_loop_test(tc, string_utils_cnv_space2nbsp_test, 0, NELEMS(cnv_space2nbsp_test_vec));
    tcase_add_test(tc, string_utils_cnv_space2nbsp_api_test);
    tcase_add_test(tc, string_utils_strcasestr_test);
    tcase_add_test(tc, string_utils_strchrnul_test);

    return tc;
}


/**
 * api tests
 */
START_TEST(string_utils_snstrjoin_api_test)
{
    nserror res;
    char outstr[32];
    char *resstr = &outstr[0];
    size_t resstrlen = 32;

    /* bad count parameters */
    res = snstrjoin(&resstr, &resstrlen, ',', 0, "1");
    ck_assert_int_eq(res, NSERROR_BAD_PARAMETER);

    res = snstrjoin(&resstr, &resstrlen, ',', 17, "1");
    ck_assert_int_eq(res, NSERROR_BAD_PARAMETER);

    /* if there is a buffer must set length */
    res = snstrjoin(&resstr, NULL, ',', 4, "1", "2", "3", "4");
    ck_assert_int_eq(res, NSERROR_BAD_PARAMETER);

    /* null argument value is bad parameter */
    res = snstrjoin(&resstr, &resstrlen, ',', 4, "1", NULL, "3", "4");
    ck_assert_int_eq(res, NSERROR_BAD_PARAMETER);

    /* attempt to use an undersize buffer */
    resstrlen = 1;
    res = snstrjoin(&resstr, &resstrlen, ',', 4, "1", "2", "3", "4");
    ck_assert_int_eq(res, NSERROR_NOSPACE);
}
END_TEST


/**
 * good four parameter join
 */
START_TEST(string_utils_snstrjoin_four_test)
{
    nserror res;
    char *resstr = NULL;
    size_t resstrlen;

    res = snstrjoin(&resstr, &resstrlen, ',', 4, "1", "2", "3", "4");
    ck_assert_int_eq(res, NSERROR_OK);
    ck_assert(resstr != NULL);
    ck_assert_int_eq(resstrlen, 8);
    ck_assert_str_eq(resstr, "1,2,3,4");
    free(resstr);
}
END_TEST


/**
 * good three parameter join with no length
 */
START_TEST(string_utils_snstrjoin_three_test)
{
    nserror res;
    char *resstr = NULL;

    res = snstrjoin(&resstr, NULL, ',', 3, "1", "2,", "3");
    ck_assert_int_eq(res, NSERROR_OK);
    ck_assert(resstr != NULL);
    ck_assert_str_eq(resstr, "1,2,3");
    free(resstr);
}
END_TEST

/**
 * good two parameter join into pre allocated buffer
 */
START_TEST(string_utils_snstrjoin_two_test)
{
    nserror res;
    char outstr[32];
    char *resstr = &outstr[0];
    size_t resstrlen = 32;

    res = snstrjoin(&resstr, &resstrlen, ',', 2, "1", "2");
    ck_assert_int_eq(res, NSERROR_OK);
    ck_assert(resstr != NULL);
    ck_assert_int_eq(resstrlen, 4);
    ck_assert_str_eq(resstr, "1,2");
}
END_TEST


static TCase *snstrjoin_case_create(void)
{
    TCase *tc;
    tc = tcase_create("snstrjoin utilities");

    tcase_add_test(tc, string_utils_snstrjoin_api_test);
    tcase_add_test(tc, string_utils_snstrjoin_four_test);
    tcase_add_test(tc, string_utils_snstrjoin_three_test);
    tcase_add_test(tc, string_utils_snstrjoin_three_test);
    tcase_add_test(tc, string_utils_snstrjoin_two_test);

    return tc;
}

/* nsfmt_float tests */
START_TEST(nsfmt_float_test)
{
    char buf[64];
    int len;

    len = nsfmt_float(buf, sizeof(buf), "%.2f", 3.14159);
    ck_assert_int_gt(len, 0);
    ck_assert_str_eq(buf, "3.14");

    len = nsfmt_float(buf, sizeof(buf), "%.1f", -12.34);
    ck_assert_int_gt(len, 0);
    ck_assert_str_eq(buf, "-12.3");

    len = nsfmt_float(buf, sizeof(buf), "%.0f", 0.0);
    ck_assert_int_gt(len, 0);
    ck_assert_str_eq(buf, "0");
}
END_TEST

static TCase *nsfmt_float_case_create(void)
{
    TCase *tc;
    tc = tcase_create("nsfmt_float");

    tcase_add_test(tc, nsfmt_float_test);

    return tc;
}

/* ns_strto tests */
START_TEST(ns_strtoint_test)
{
    int res = 0;
    nserror err;

    /* Parameter validation */
    err = ns_strtoint(NULL, 10, &res);
    ck_assert_int_eq(err, NSERROR_BAD_PARAMETER);

    err = ns_strtoint("123", 10, NULL);
    ck_assert_int_eq(err, NSERROR_BAD_PARAMETER);

    /* Valid values */
    err = ns_strtoint("123", 10, &res);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(res, 123);

    err = ns_strtoint("-456", 10, &res);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(res, -456);

    err = ns_strtoint("0x1A", 16, &res);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(res, 26);

    /* Trailing whitespace */
    err = ns_strtoint("  789 \t\n", 10, &res);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(res, 789);

    /* Invalid input */
    err = ns_strtoint("123abc", 10, &res);
    ck_assert_int_eq(err, NSERROR_INVALID);

    err = ns_strtoint("", 10, &res);
    ck_assert_int_eq(err, NSERROR_INVALID);

    err = ns_strtoint("  ", 10, &res);
    ck_assert_int_eq(err, NSERROR_INVALID);

    /* Overflow / Underflow */
    err = ns_strtoint("99999999999999999999999", 10, &res);
    ck_assert_int_eq(err, NSERROR_INVALID);

    err = ns_strtoint("-99999999999999999999999", 10, &res);
    ck_assert_int_eq(err, NSERROR_INVALID);
}
END_TEST

START_TEST(ns_strtouint_test)
{
    unsigned int res = 0;
    nserror err;

    /* Parameter validation */
    err = ns_strtouint(NULL, 10, &res);
    ck_assert_int_eq(err, NSERROR_BAD_PARAMETER);

    err = ns_strtouint("123", 10, NULL);
    ck_assert_int_eq(err, NSERROR_BAD_PARAMETER);

    /* Valid values */
    err = ns_strtouint("123", 10, &res);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(res, 123);

    err = ns_strtouint("0x1A", 16, &res);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(res, 26);

    /* Trailing whitespace */
    err = ns_strtouint(" 456 \t", 10, &res);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(res, 456);

    /* Invalid input */
    err = ns_strtouint("123abc", 10, &res);
    ck_assert_int_eq(err, NSERROR_INVALID);

    err = ns_strtouint("", 10, &res);
    ck_assert_int_eq(err, NSERROR_INVALID);

    /* Overflow */
    err = ns_strtouint("99999999999999999999999", 10, &res);
    ck_assert_int_eq(err, NSERROR_INVALID);
}
END_TEST

START_TEST(ns_strtoll_test)
{
    long long res = 0;
    nserror err;

    /* Parameter validation */
    err = ns_strtoll(NULL, 10, &res);
    ck_assert_int_eq(err, NSERROR_BAD_PARAMETER);

    err = ns_strtoll("123", 10, NULL);
    ck_assert_int_eq(err, NSERROR_BAD_PARAMETER);

    /* Valid values */
    err = ns_strtoll("1234567890123", 10, &res);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(res, 1234567890123LL);

    err = ns_strtoll("-1234567890123", 10, &res);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(res, -1234567890123LL);

    /* Trailing whitespace */
    err = ns_strtoll(" 9876543210 \n", 10, &res);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(res, 9876543210LL);

    /* Invalid input */
    err = ns_strtoll("123abc", 10, &res);
    ck_assert_int_eq(err, NSERROR_INVALID);

    err = ns_strtoll("", 10, &res);
    ck_assert_int_eq(err, NSERROR_INVALID);

    /* Overflow / Underflow */
    err = ns_strtoll("99999999999999999999999999999999", 10, &res);
    ck_assert_int_eq(err, NSERROR_INVALID);

    err = ns_strtoll("-99999999999999999999999999999999", 10, &res);
    ck_assert_int_eq(err, NSERROR_INVALID);
}
END_TEST

START_TEST(ns_strtoull_test)
{
    unsigned long long res = 0;
    nserror err;

    /* Parameter validation */
    err = ns_strtoull(NULL, 10, &res);
    ck_assert_int_eq(err, NSERROR_BAD_PARAMETER);

    err = ns_strtoull("123", 10, NULL);
    ck_assert_int_eq(err, NSERROR_BAD_PARAMETER);

    /* Valid values */
    err = ns_strtoull("1234567890123", 10, &res);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(res, 1234567890123ULL);

    /* Trailing whitespace */
    err = ns_strtoull(" 1234567890123 \t", 10, &res);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(res, 1234567890123ULL);

    /* Invalid input */
    err = ns_strtoull("123abc", 10, &res);
    ck_assert_int_eq(err, NSERROR_INVALID);

    err = ns_strtoull("", 10, &res);
    ck_assert_int_eq(err, NSERROR_INVALID);

    /* Overflow */
    err = ns_strtoull("99999999999999999999999999999999", 10, &res);
    ck_assert_int_eq(err, NSERROR_INVALID);
}
END_TEST

static TCase *ns_strto_case_create(void)
{
    TCase *tc;
    tc = tcase_create("ns_strto");

    tcase_add_test(tc, ns_strtoint_test);
    tcase_add_test(tc, ns_strtouint_test);
    tcase_add_test(tc, ns_strtoll_test);
    tcase_add_test(tc, ns_strtoull_test);

    return tc;
}

/* stable_sort tests */
struct sort_item {
    int key;
    int val;
};

static int sort_cmp(const void *a, const void *b)
{
    const struct sort_item *ia = a;
    const struct sort_item *ib = b;
    return ia->key - ib->key;
}

START_TEST(stable_sort_test)
{
    struct sort_item arr[] = {
        {3, 1},
        {2, 1},
        {3, 2},
        {1, 1},
        {2, 2},
        {3, 3}
    };

    stable_sort(arr, 6, sizeof(struct sort_item), sort_cmp);

    ck_assert_int_eq(arr[0].key, 1); ck_assert_int_eq(arr[0].val, 1);
    ck_assert_int_eq(arr[1].key, 2); ck_assert_int_eq(arr[1].val, 1);
    ck_assert_int_eq(arr[2].key, 2); ck_assert_int_eq(arr[2].val, 2);
    ck_assert_int_eq(arr[3].key, 3); ck_assert_int_eq(arr[3].val, 1);
    ck_assert_int_eq(arr[4].key, 3); ck_assert_int_eq(arr[4].val, 2);
    ck_assert_int_eq(arr[5].key, 3); ck_assert_int_eq(arr[5].val, 3);
}
END_TEST

START_TEST(stable_sort_edge_cases_test)
{
    struct sort_item single[] = { {5, 1} };
    struct sort_item sorted[] = { {1, 1}, {2, 1}, {3, 1} };
    struct sort_item reversed[] = { {3, 1}, {2, 1}, {1, 1} };
    struct sort_item equal[] = { {2, 1}, {2, 2}, {2, 3} };

    /* 0 and 1 element array handled safely without crashing */
    stable_sort(NULL, 0, sizeof(struct sort_item), sort_cmp);
    stable_sort(single, 1, sizeof(struct sort_item), sort_cmp);
    ck_assert_int_eq(single[0].key, 5);

    /* Already sorted */
    stable_sort(sorted, 3, sizeof(struct sort_item), sort_cmp);
    ck_assert_int_eq(sorted[0].key, 1);
    ck_assert_int_eq(sorted[1].key, 2);
    ck_assert_int_eq(sorted[2].key, 3);

    /* Reversed array */
    stable_sort(reversed, 3, sizeof(struct sort_item), sort_cmp);
    ck_assert_int_eq(reversed[0].key, 1);
    ck_assert_int_eq(reversed[1].key, 2);
    ck_assert_int_eq(reversed[2].key, 3);

    /* Equal keys preserve relative order */
    stable_sort(equal, 3, sizeof(struct sort_item), sort_cmp);
    ck_assert_int_eq(equal[0].val, 1);
    ck_assert_int_eq(equal[1].val, 2);
    ck_assert_int_eq(equal[2].val, 3);
}
END_TEST

static TCase *stable_sort_case_create(void)
{
    TCase *tc;
    tc = tcase_create("stable_sort");

    tcase_add_test(tc, stable_sort_test);
    tcase_add_test(tc, stable_sort_edge_cases_test);

    return tc;
}

/* Macro tests */
START_TEST(utils_macros_test)
{
    int arr[5] = {10, 20, 30, 40, 50};
    const char str[] = "hello";

    ck_assert_int_eq(NOF_ELEMENTS(arr), 5);
    ck_assert_int_eq(N_ELEMENTS(arr), 5);

    ck_assert_int_eq(ABS(5), 5);
    ck_assert_int_eq(ABS(-5), 5);
    ck_assert_int_eq(ABS(0), 0);

    ck_assert_int_eq(min(10, 20), 10);
    ck_assert_int_eq(min(20, 10), 10);

    ck_assert_int_eq(max(10, 20), 20);
    ck_assert_int_eq(max(20, 10), 20);

    ck_assert_int_eq(clamp(15, 10, 20), 15);
    ck_assert_int_eq(clamp(5, 10, 20), 10);
    ck_assert_int_eq(clamp(25, 10, 20), 20);

    ck_assert_int_eq(SLEN("test"), 4);
    ck_assert_int_eq(SLEN(str), 5);
}
END_TEST

static TCase *utils_macros_case_create(void)
{
    TCase *tc;
    tc = tcase_create("utils_macros");

    tcase_add_test(tc, utils_macros_test);

    return tc;
}

/* is_dir tests */
START_TEST(is_dir_test)
{
    const char *tmp_dir = getenv("TMPDIR");
    if (!tmp_dir) {
#ifdef _WIN32
        tmp_dir = getenv("TEMP");
        if (!tmp_dir) tmp_dir = ".";
#else
        tmp_dir = "/tmp";
#endif
    }

    char dir_template[256];
    snprintf(dir_template, sizeof(dir_template), "%s/ns_test_dir_XXXXXX", tmp_dir);

    char file_template[256];
    snprintf(file_template, sizeof(file_template), "%s/ns_test_file_XXXXXX", tmp_dir);

    char nonexistent_path[256];
    snprintf(nonexistent_path, sizeof(nonexistent_path), "%s/nonexistent_dir_ns_test_12345/foo", tmp_dir);


    char *dname = mkdtemp(dir_template);
    ck_assert(dname != NULL);

    int fd = mkstemp(file_template);
    ck_assert_int_ne(fd, -1);
    close(fd);

    ck_assert(is_dir(dname));
    ck_assert(!is_dir(file_template));
    ck_assert(!is_dir(nonexistent_path));

    rmdir(dname);
    unlink(file_template);
}
END_TEST

static TCase *is_dir_case_create(void)
{
    TCase *tc;
    tc = tcase_create("is_dir");

    tcase_add_test(tc, is_dir_test);

    return tc;
}
/*
 * Utility test suite creation
 */
static Suite *utils_suite_create(void)
{
    Suite *s;
    s = suite_create("Utility API");

    suite_add_tcase(s, human_friendly_bytesize_case_create());
    suite_add_tcase(s, squash_whitespace_case_create());
    suite_add_tcase(s, corestrings_case_create());
    suite_add_tcase(s, snstrjoin_case_create());
    suite_add_tcase(s, string_utils_case_create());
    suite_add_tcase(s, nsfmt_float_case_create());
    suite_add_tcase(s, ns_strto_case_create());
    suite_add_tcase(s, stable_sort_case_create());
    suite_add_tcase(s, is_dir_case_create());
    suite_add_tcase(s, utils_macros_case_create());

    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    SRunner *sr;

    sr = srunner_create(utils_suite_create());

    srunner_run_all(sr, CK_ENV);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    fprintf(stderr, "[lwc] Remaining lwc strings:\n");
    unsigned lwc_count = 0;
    lwc_iterate_strings(test_lwc_iterator, &lwc_count);
    fprintf(stderr, "[lwc] Remaining lwc strings count: %u\n", lwc_count);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
