/*
 * Copyright 2026 Jules
 *
 * This file is part of NetSurf / Wisp.
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#include <check.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dom/dom.h>

#include <wisp/content/content_protected.h>
#include <wisp/content/hlcache.h>
#include <wisp/desktop/options.h>
#include <wisp/utils/corestrings.h>
#include <wisp/utils/log.h>
#include <wisp/utils/messages.h>
#include <wisp/utils/nsurl.h>
#include <wisp/utils/utils.h>

#include "content/handlers/html/imagemap.h"
#include "content/handlers/html/private.h"
#include "utils/hashmap.h"

static html_content *create_dummy_html_content(void)
{
    html_content *c = calloc(1, sizeof(html_content));
    ck_assert_ptr_nonnull(c);
    nsurl_create("http://example.com/", &c->base_url);
    ck_assert_ptr_nonnull(c->base_url);
    return c;
}

static void destroy_dummy_html_content(html_content *c)
{
    if (c == NULL) return;
    imagemap_destroy(c);
    if (c->base_url != NULL) {
        nsurl_unref(c->base_url);
    }
    if (c->document != NULL) {
        dom_node_unref(c->document);
    }
    free(c);
}

START_TEST(test_imagemap_destroy_null)
{
    html_content *c = create_dummy_html_content();
    imagemap_destroy(c);
    ck_assert_ptr_null(c->imagemaps);
    destroy_dummy_html_content(c);
}
END_TEST

START_TEST(test_imagemap_dom_extract_and_lookup)
{
    dom_exception exc;
    dom_document *doc = NULL;
    dom_element *map = NULL, *area_rect = NULL, *area_circle = NULL, *area_poly = NULL, *area_default = NULL;
    dom_string *str_map = NULL, *str_area = NULL, *str_name = NULL, *str_map_name = NULL;
    dom_string *str_href = NULL, *str_shape = NULL, *str_coords = NULL;
    dom_string *val_rect = NULL, *val_coords_rect = NULL, *val_href_rect = NULL;
    dom_string *val_circle = NULL, *val_coords_circle = NULL, *val_href_circle = NULL;
    dom_string *val_poly = NULL, *val_coords_poly = NULL, *val_href_poly = NULL;
    dom_string *val_default = NULL, *val_href_default = NULL;
    dom_string *str_html = NULL;
    dom_element *html_elem = NULL;

    html_content *c = create_dummy_html_content();

    exc = dom_implementation_create_document(DOM_IMPLEMENTATION_XML, NULL, NULL, NULL, &doc, NULL);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    ck_assert_ptr_nonnull(doc);
    c->document = doc;

    exc = dom_string_create((const uint8_t *)"html", 4, &str_html);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_document_create_element(doc, str_html, &html_elem);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_node_append_child(doc, html_elem, NULL);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    dom_string_unref(str_html);

    exc = dom_string_create((const uint8_t *)"map", 3, &str_map);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_document_create_element(doc, str_map, &map);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_node_append_child(html_elem, map, NULL);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    dom_string_unref(str_map);

    exc = dom_string_create((const uint8_t *)"name", 4, &str_name);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create((const uint8_t *)"testmap", 7, &str_map_name);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_element_set_attribute(map, str_name, str_map_name);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    dom_string_unref(str_name);
    dom_string_unref(str_map_name);

    exc = dom_string_create((const uint8_t *)"area", 4, &str_area);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create((const uint8_t *)"href", 4, &str_href);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create((const uint8_t *)"shape", 5, &str_shape);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create((const uint8_t *)"coords", 6, &str_coords);
    ck_assert_int_eq(exc, DOM_NO_ERR);

    /* 1. Rect area: href="rect.html", coords="10,10,50,50" */
    exc = dom_document_create_element(doc, str_area, &area_rect);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create((const uint8_t *)"rect.html", 9, &val_href_rect);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create((const uint8_t *)"rect", 4, &val_rect);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create((const uint8_t *)"10,10,50,50", 11, &val_coords_rect);
    ck_assert_int_eq(exc, DOM_NO_ERR);

    dom_element_set_attribute(area_rect, str_href, val_href_rect);
    dom_element_set_attribute(area_rect, str_shape, val_rect);
    dom_element_set_attribute(area_rect, str_coords, val_coords_rect);
    dom_node_append_child(map, area_rect, NULL);

    /* 2. Circle area: href="circle.html", coords="100,100,20" */
    exc = dom_document_create_element(doc, str_area, &area_circle);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create((const uint8_t *)"circle.html", 11, &val_href_circle);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create((const uint8_t *)"circle", 6, &val_circle);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create((const uint8_t *)"100,100,20", 10, &val_coords_circle);
    ck_assert_int_eq(exc, DOM_NO_ERR);

    dom_element_set_attribute(area_circle, str_href, val_href_circle);
    dom_element_set_attribute(area_circle, str_shape, val_circle);
    dom_element_set_attribute(area_circle, str_coords, val_coords_circle);
    dom_node_append_child(map, area_circle, NULL);

    /* 3. Poly area: href="poly.html", coords="200,200,250,200,225,250" */
    exc = dom_document_create_element(doc, str_area, &area_poly);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create((const uint8_t *)"poly.html", 9, &val_href_poly);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create((const uint8_t *)"poly", 4, &val_poly);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create((const uint8_t *)"200,200,250,200,225,250", 23, &val_coords_poly);
    ck_assert_int_eq(exc, DOM_NO_ERR);

    dom_element_set_attribute(area_poly, str_href, val_href_poly);
    dom_element_set_attribute(area_poly, str_shape, val_poly);
    dom_element_set_attribute(area_poly, str_coords, val_coords_poly);
    dom_node_append_child(map, area_poly, NULL);

    /* 4. Default area: href="default.html", shape="default" */
    exc = dom_document_create_element(doc, str_area, &area_default);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create((const uint8_t *)"default.html", 12, &val_href_default);
    ck_assert_int_eq(exc, DOM_NO_ERR);
    exc = dom_string_create((const uint8_t *)"default", 7, &val_default);
    ck_assert_int_eq(exc, DOM_NO_ERR);

    dom_element_set_attribute(area_default, str_href, val_href_default);
    dom_element_set_attribute(area_default, str_shape, val_default);
    dom_node_append_child(map, area_default, NULL);

    dom_string_unref(str_area);
    dom_string_unref(str_href);
    dom_string_unref(str_shape);
    dom_string_unref(str_coords);
    dom_string_unref(val_href_rect);
    dom_string_unref(val_rect);
    dom_string_unref(val_coords_rect);
    dom_string_unref(val_href_circle);
    dom_string_unref(val_circle);
    dom_string_unref(val_coords_circle);
    dom_string_unref(val_href_poly);
    dom_string_unref(val_poly);
    dom_string_unref(val_coords_poly);
    dom_string_unref(val_href_default);
    dom_string_unref(val_default);

    dom_node_unref(area_rect);
    dom_node_unref(area_circle);
    dom_node_unref(area_poly);
    dom_node_unref(area_default);
    dom_node_unref(map);
    dom_node_unref(html_elem);

    /* Extract map from document */
    nserror err = imagemap_extract(c);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(c->imagemaps);

    /* Test Case-Insensitive Map Lookup & Rect Hit */
    const char *target = NULL;
    nsurl *url = imagemap_get(c, "TESTMAP", 0, 0, 20, 20, &target);
    ck_assert_ptr_nonnull(url);
    ck_assert_str_eq(nsurl_access(url), "http://example.com/rect.html");

    /* Test Circle Hit */
    url = imagemap_get(c, "testmap", 0, 0, 105, 105, &target);
    ck_assert_ptr_nonnull(url);
    ck_assert_str_eq(nsurl_access(url), "http://example.com/circle.html");

    /* Test Poly Hit */
    url = imagemap_get(c, "TestMap", 0, 0, 225, 210, &target);
    ck_assert_ptr_nonnull(url);
    ck_assert_str_eq(nsurl_access(url), "http://example.com/poly.html");

    /* Test Default Hit (Missed rect, circle, poly -> falls through to default) */
    url = imagemap_get(c, "testmap", 0, 0, 1, 1, &target);
    ck_assert_ptr_nonnull(url);
    ck_assert_str_eq(nsurl_access(url), "http://example.com/default.html");

    /* Test Non-existent map */
    url = imagemap_get(c, "nonexistent", 0, 0, 20, 20, &target);
    ck_assert_ptr_null(url);

    /* Dump imagemap */
    imagemap_dump(c);

    destroy_dummy_html_content(c);
}
END_TEST

static Suite *imagemap_suite(void)
{
    Suite *s = suite_create("imagemap");
    TCase *tc = tcase_create("core");

    tcase_add_test(tc, test_imagemap_destroy_null);
    tcase_add_test(tc, test_imagemap_dom_extract_and_lookup);

    suite_add_tcase(s, tc);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = imagemap_suite();
    SRunner *sr = srunner_create(s);

    corestrings_init();
    nsoption_init(NULL, NULL, NULL);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    nsoption_finalise(NULL, NULL);
    corestrings_fini();

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
