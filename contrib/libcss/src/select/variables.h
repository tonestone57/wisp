/*
 * This file is part of LibCSS
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 */

#ifndef css_select_variables_h_
#define css_select_variables_h_

#include <libwapcaplet/libwapcaplet.h>
#include <libcss/errors.h>
#include <parserutils/utils/vector.h>

/**
 * A single custom property binding: name → value.
 * name is a ref-counted lwc_string.
 * tokens is a parserutils_vector of css_tokens.
 */
typedef struct css_var_entry {
    lwc_string *name;   /* e.g. "--primary" */
    parserutils_vector *tokens; /* pre-tokenized value */
} css_var_entry;

/**
 * Per-element variable context: flat array of name→value pairs.
 * Typical size is 0-20 entries; linear scan with lwc pointer
 * comparison is faster than a hash map at these sizes.
 */
typedef struct css_var_cache_entry {
    uint32_t src_hash;
    parserutils_vector *resolved_tokens;
} css_var_cache_entry;

typedef struct css_var_context {
    css_var_entry *entries;
    uint32_t count;
    uint32_t capacity;

    /* Style-context hashing & caching */
    uint32_t hash;
    css_var_cache_entry *cache;
    uint32_t cache_count;
    uint32_t cache_capacity;
} css_var_context;

uint32_t css__tokens_vector_hash(parserutils_vector *v);
parserutils_vector *css__variables_ctx_get_resolved(const css_var_context *ctx, uint32_t src_hash);
css_error css__variables_ctx_set_resolved(css_var_context *ctx, uint32_t src_hash, parserutils_vector *resolved);
void css__var_cache_clear(css_var_context *ctx);

/**
 * Create an empty variable context.
 */
css_error css__variables_ctx_create(css_var_context **out);

/**
 * Clone a variable context (full deep copy with lwc_string_ref).
 * If src is NULL, creates an empty context.
 */
css_error css__variables_ctx_clone(const css_var_context *src, css_var_context **out);

/**
 * Destroy a variable context and unref all strings.
 */
void css__variables_ctx_destroy(css_var_context *ctx);

/**
 * Set a variable in the context. If name already exists, its value
 * is replaced. Both name and tokens are ref'd/copied by this function.
 */
css_error css__variables_ctx_set(css_var_context *ctx,
    lwc_string *name, parserutils_vector *tokens);

/**
 * Look up a variable by name.
 * Returns the tokens vector (not ref'd — caller must NOT destroy),
 * or NULL if not found.
 */
parserutils_vector *css__variables_ctx_get(const css_var_context *ctx,
    lwc_string *name);

parserutils_error css__tokens_clone(parserutils_vector *src, parserutils_vector **dst);
void css__tokens_destroy(parserutils_vector *v);

#endif
