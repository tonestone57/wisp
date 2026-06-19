/*
 * This file is part of LibCSS
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 */

#include <stdlib.h>
#include <string.h>

#include "lex/lex.h"
#include "select/variables.h"

#define VAR_CTX_INITIAL_CAPACITY 4

parserutils_error css__tokens_clone(parserutils_vector *src, parserutils_vector **dst)
{
    parserutils_error perror;
    size_t len;
    perror = parserutils_vector_get_length(src, &len);
    if (perror != PARSERUTILS_OK) return perror;

    perror = parserutils_vector_create(sizeof(css_token), 8, dst);
    if (perror != PARSERUTILS_OK) return perror;

    for (uint32_t i = 0; i < (uint32_t)len; i++) {
        css_token *t = (css_token *)parserutils_vector_peek(src, i);
        css_token new_t = *t;
        if (new_t.idata != NULL) lwc_string_ref(new_t.idata);
        if (new_t.data.data != NULL) {
            new_t.data.data = malloc(new_t.data.len);
            memcpy(new_t.data.data, t->data.data, new_t.data.len);
        }
        parserutils_vector_append(*dst, &new_t);
    }
    return PARSERUTILS_OK;
}

void css__tokens_destroy(parserutils_vector *v)
{
    size_t len;
    if (v == NULL) return;
    parserutils_vector_get_length(v, &len);
    for (uint32_t i = 0; i < (uint32_t)len; i++) {
        css_token *t = (css_token *)parserutils_vector_peek(v, i);
        if (t->idata != NULL) lwc_string_unref(t->idata);
        free(t->data.data);
    }
    parserutils_vector_destroy(v);
}

css_error css__variables_ctx_create(css_var_context **out)
{
    css_var_context *ctx = calloc(1, sizeof(*ctx));
    if (ctx == NULL)
        return CSS_NOMEM;

    *out = ctx;
    return CSS_OK;
}

css_error css__variables_ctx_clone(const css_var_context *src, css_var_context **out)
{
    css_var_context *ctx;
    css_error error;

    error = css__variables_ctx_create(&ctx);
    if (error != CSS_OK)
        return error;

    if (src != NULL && src->count > 0) {
        ctx->entries = malloc(src->count * sizeof(css_var_entry));
        if (ctx->entries == NULL) {
            free(ctx);
            return CSS_NOMEM;
        }

        ctx->capacity = src->count;
        ctx->count = 0;

        for (uint32_t i = 0; i < src->count; i++) {
            parserutils_vector *new_tokens;
            if (css__tokens_clone(src->entries[i].tokens, &new_tokens) == PARSERUTILS_OK) {
                ctx->entries[ctx->count].name = lwc_string_ref(src->entries[i].name);
                ctx->entries[ctx->count].tokens = new_tokens;
                ctx->count++;
            }
        }
    }

    *out = ctx;
    return CSS_OK;
}

void css__variables_ctx_destroy(css_var_context *ctx)
{
    if (ctx == NULL)
        return;

    for (uint32_t i = 0; i < ctx->count; i++) {
        lwc_string_unref(ctx->entries[i].name);
        css__tokens_destroy(ctx->entries[i].tokens);
    }

    free(ctx->entries);
    free(ctx);
}

css_error css__variables_ctx_set(css_var_context *ctx,
    lwc_string *name, parserutils_vector *tokens)
{
    for (uint32_t i = 0; i < ctx->count; i++) {
        if (ctx->entries[i].name == name) {
            css__tokens_destroy(ctx->entries[i].tokens);
            css__tokens_clone(tokens, &ctx->entries[i].tokens);
            return CSS_OK;
        }
    }

    if (ctx->count >= ctx->capacity) {
        uint32_t new_cap = ctx->capacity == 0 ? VAR_CTX_INITIAL_CAPACITY : ctx->capacity * 2;
        css_var_entry *new_entries = realloc(ctx->entries, new_cap * sizeof(css_var_entry));
        if (new_entries == NULL) return CSS_NOMEM;
        ctx->entries = new_entries;
        ctx->capacity = new_cap;
    }

    ctx->entries[ctx->count].name = lwc_string_ref(name);
    css__tokens_clone(tokens, &ctx->entries[ctx->count].tokens);
    ctx->count++;

    return CSS_OK;
}

parserutils_vector *css__variables_ctx_get(const css_var_context *ctx,
    lwc_string *name)
{
    if (ctx == NULL) return NULL;
    for (uint32_t i = 0; i < ctx->count; i++) {
        if (ctx->entries[i].name == name) return ctx->entries[i].tokens;
    }
    return NULL;
}
