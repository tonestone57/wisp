/*
 * This file is part of LibCSS
 * Licensed under the MIT License,
 *		  http://www.opensource.org/licenses/mit-license.php
 * Copyright 2026 NeoSurf Contributors
 */

#include "utils/css_utils.h"
#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "select/propget.h"
#include "select/propset.h"

#include "select/properties/helpers.h"
#include "select/properties/properties.h"

css_error css__cascade_tab_size(uint32_t opv, css_style *style, css_select_state *state)
{
	uint16_t value = CSS_TAB_SIZE_INHERIT;
	css_fixed val = 0;

	if (hasFlagValue(opv) == false) {
		switch (getValue(opv)) {
		case TAB_SIZE_SET:
			value = CSS_TAB_SIZE_SET;
			val = *((css_fixed *)style->bytecode);
			advance_bytecode(style, sizeof(val));
			break;
		default:
			assert(0 && "Invalid value");
			break;
		}
	}

	if (css__outranks_existing(getOpcode(opv), isImportant(opv), state, getFlagValue(opv))) {
		return set_tab_size(state->computed, value, FIXTOINT(val));
	}

	return CSS_OK;
}

css_error css__set_tab_size_from_hint(const css_hint *hint, css_computed_style *style)
{
	return set_tab_size(style, hint->status, hint->data.integer);
}

css_error css__initial_tab_size(css_select_state *state)
{
	return set_tab_size(state->computed, CSS_TAB_SIZE_SET, 8);
}

css_error css__copy_tab_size(const css_computed_style *from, css_computed_style *to)
{
	int32_t itval;
	uint8_t type = get_tab_size(from, &itval);

	if (from == to) {
		return CSS_OK;
	}

	return set_tab_size(to, type, itval);
}

css_error
css__compose_tab_size(const css_computed_style *parent, const css_computed_style *child, css_computed_style *result)
{
	int32_t itval;
	uint8_t type = get_tab_size(child, &itval);

	return css__copy_tab_size(type == CSS_TAB_SIZE_INHERIT ? parent : child, result);
}
