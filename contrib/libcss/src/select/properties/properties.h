/*
 * This file is part of LibCSS
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2009 John-Mark Bell <jmb@netsurf-browser.org>
 */

#ifndef css_select_properties_h_
#define css_select_properties_h_

#include <libcss/computed.h>
#include <libcss/errors.h>

#include "select/select.h"
#include "stylesheet.h"

#define PROPERTY_FUNCS(pname)                                                                                          \
    css_error css__cascade_##pname(uint32_t opv, css_style *style, css_select_state *state);                           \
    css_error css__set_##pname##_from_hint(const css_hint *hint, css_computed_style *style);                           \
    css_error css__initial_##pname(css_select_state *state);                                                           \
    css_error css__copy_##pname(const css_computed_style *from, css_computed_style *to);                               \
    css_error css__compose_##pname(                                                                                    \
        const css_computed_style *parent, const css_computed_style *child, css_computed_style *result);                \
    uint32_t destroy_##pname(void *bytecode)

/* Auto-generated cascade function declarations for all properties */
#include "cascade_declarations.inc"

#undef PROPERTY_FUNCS

#endif
