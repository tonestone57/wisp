/*
 * This file is part of libdom.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2026 Wisp
 */

#ifndef dom_core_mutation_observer_h_
#define dom_core_mutation_observer_h_

#include <dom/core/exceptions.h>

struct dom_node;
struct dom_string;
struct dom_document;

typedef enum {
    DOM_MUTATION_NOTIFICATION_CHILD_LIST,
    DOM_MUTATION_NOTIFICATION_ATTRIBUTES,
    DOM_MUTATION_NOTIFICATION_CHARACTER_DATA
} dom_mutation_notification_type;

struct dom_mutation_notification {
    dom_mutation_notification_type type;
    struct dom_node *target;

    /* For CHILD_LIST */
    struct dom_node *added_node;   /* May be NULL */
    struct dom_node *removed_node; /* May be NULL */
    struct dom_node *previous_sibling;
    struct dom_node *next_sibling;

    /* For ATTRIBUTES */
    struct dom_string *attr_name;
    struct dom_string *attr_namespace;

    /* For ATTRIBUTES and CHARACTER_DATA */
    struct dom_string *old_value;
    struct dom_string *new_value;
};

typedef void (*dom_mutation_callback)(const struct dom_mutation_notification *notification, void *pw);

dom_exception dom_document_add_mutation_callback(struct dom_document *doc,
        dom_mutation_callback callback, void *pw);

dom_exception dom_document_remove_mutation_callback(struct dom_document *doc,
        dom_mutation_callback callback, void *pw);

#endif
