#include <stdint.h>
#include <stdlib.h>
#include "utils/css_utils.h"
#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "select/propget.h"
#include "select/propset.h"
#include "select/properties/helpers.h"
#include "select/properties/properties.h"

css_error css__cascade_counter_set(uint32_t opv, css_style *style, css_select_state *state)
{
	return css__cascade_counter_increment_reset(opv, style, state, set_counter_set);
}

css_error css__set_counter_set_from_hint(const css_hint *hint, css_computed_style *style)
{
	css_computed_counter *item;
	css_error error;

	error = set_counter_set(style, hint->status, hint->data.counter);

	if (hint->status == CSS_COUNTER_SET_NAMED && hint->data.counter != NULL) {
		for (item = hint->data.counter; item->name != NULL; item++) {
			lwc_string_unref(item->name);
		}
	}

	if (error != CSS_OK && hint->data.counter != NULL)
		free(hint->data.counter);

	return error;
}

css_error css__initial_counter_set(css_select_state *state)
{
	return set_counter_set(state->computed, CSS_COUNTER_SET_NONE, NULL);
}

css_error css__copy_counter_set(const css_computed_style *from, css_computed_style *to)
{
	css_error error;
	css_computed_counter *copy = NULL;
	const css_computed_counter *counter_set = NULL;
	uint8_t type = get_counter_set(from, &counter_set);

	if (from == to) {
		return CSS_OK;
	}

	error = css__copy_computed_counter_array(false, counter_set, &copy);
	if (error != CSS_OK) {
		return CSS_NOMEM;
	}

	error = set_counter_set(to, type, copy);
	if (error != CSS_OK) {
		free(copy);
	}

	return error;
}

css_error css__compose_counter_set(
	const css_computed_style *parent, const css_computed_style *child, css_computed_style *result)
{
	const css_computed_counter *counter_set = NULL;
	uint8_t type = get_counter_set(child, &counter_set);

	return css__copy_counter_set(type == CSS_COUNTER_SET_INHERIT ? parent : child, result);
}
