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

uint32_t css__tokens_vector_hash(parserutils_vector *v)
{
    if (v == NULL) return 0;
    size_t len;
    if (parserutils_vector_get_length(v, &len) != PARSERUTILS_OK) return 0;
    uint32_t hash = 2166136261U;
    for (size_t i = 0; i < len; i++) {
        css_token *t = (css_token *)parserutils_vector_peek(v, i);
        hash ^= (uint8_t)t->type;
        hash *= 16777619U;
        if (t->idata != NULL) {
            const char *idata_data = lwc_string_data(t->idata);
            size_t idata_len = lwc_string_length(t->idata);
            for (size_t k = 0; k < idata_len; k++) {
                hash ^= (uint8_t)idata_data[k];
                hash *= 16777619U;
            }
        }
        if (t->data.data != NULL) {
            for (size_t k = 0; k < t->data.len; k++) {
                hash ^= t->data.data[k];
                hash *= 16777619U;
            }
        }
    }
    return hash;
}

static uint32_t css__variables_ctx_compute_hash(const css_var_context *ctx)
{
    if (ctx == NULL || ctx->count == 0)
        return 0;

    uint32_t hash = 2166136261U;

    for (uint32_t i = 0; i < ctx->count; i++) {
        const char *name_data = lwc_string_data(ctx->entries[i].name);
        size_t name_len = lwc_string_length(ctx->entries[i].name);
        for (size_t j = 0; j < name_len; j++) {
            hash ^= (uint8_t)name_data[j];
            hash *= 16777619U;
        }

        size_t len;
        if (parserutils_vector_get_length(ctx->entries[i].tokens, &len) == PARSERUTILS_OK) {
            for (size_t j = 0; j < len; j++) {
                css_token *t = (css_token *)parserutils_vector_peek(ctx->entries[i].tokens, j);
                hash ^= (uint8_t)t->type;
                hash *= 16777619U;
                if (t->idata != NULL) {
                    const char *idata_data = lwc_string_data(t->idata);
                    size_t idata_len = lwc_string_length(t->idata);
                    for (size_t k = 0; k < idata_len; k++) {
                        hash ^= (uint8_t)idata_data[k];
                        hash *= 16777619U;
                    }
                }
                if (t->data.data != NULL) {
                    for (size_t k = 0; k < t->data.len; k++) {
                        hash ^= t->data.data[k];
                        hash *= 16777619U;
                    }
                }
            }
        }
    }

    return hash;
}

parserutils_vector *css__variables_ctx_get_resolved(const css_var_context *ctx, uint32_t src_hash)
{
    if (ctx == NULL || ctx->cache == NULL)
        return NULL;

    for (uint32_t i = 0; i < ctx->cache_count; i++) {
        if (ctx->cache[i].src_hash == src_hash) {
            return ctx->cache[i].resolved_tokens;
        }
    }
    return NULL;
}

css_error css__variables_ctx_set_resolved(css_var_context *ctx, uint32_t src_hash, parserutils_vector *resolved)
{
    if (ctx == NULL)
        return CSS_BADPARM;

    for (uint32_t i = 0; i < ctx->cache_count; i++) {
        if (ctx->cache[i].src_hash == src_hash) {
            css__tokens_destroy(ctx->cache[i].resolved_tokens);
            css__tokens_clone(resolved, &ctx->cache[i].resolved_tokens);
            return CSS_OK;
        }
    }

    if (ctx->cache_count >= ctx->cache_capacity) {
        uint32_t new_cap = ctx->cache_capacity == 0 ? 4 : ctx->cache_capacity * 2;
        css_var_cache_entry *new_cache = realloc(ctx->cache, new_cap * sizeof(css_var_cache_entry));
        if (new_cache == NULL)
            return CSS_NOMEM;
        ctx->cache = new_cache;
        ctx->cache_capacity = new_cap;
    }

    ctx->cache[ctx->cache_count].src_hash = src_hash;
    css__tokens_clone(resolved, &ctx->cache[ctx->cache_count].resolved_tokens);
    ctx->cache_count++;

    return CSS_OK;
}

void css__var_cache_clear(css_var_context *ctx)
{
    if (ctx == NULL || ctx->cache == NULL)
        return;

    for (uint32_t i = 0; i < ctx->cache_count; i++) {
        css__tokens_destroy(ctx->cache[i].resolved_tokens);
    }
    free(ctx->cache);
    ctx->cache = NULL;
    ctx->cache_count = 0;
    ctx->cache_capacity = 0;
}

css_error css__variables_ctx_create(css_var_context **out)
{
    css_var_context *ctx = calloc(1, sizeof(*ctx));
    if (ctx == NULL)
        return CSS_NOMEM;

    ctx->hash = 0;
    ctx->cache = NULL;
    ctx->cache_count = 0;
    ctx->cache_capacity = 0;

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

    if (src != NULL) {
        ctx->hash = src->hash;

        if (src->count > 0) {
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

        if (src->cache_count > 0) {
            ctx->cache = malloc(src->cache_count * sizeof(css_var_cache_entry));
            if (ctx->cache != NULL) {
                ctx->cache_capacity = src->cache_count;
                ctx->cache_count = 0;
                for (uint32_t i = 0; i < src->cache_count; i++) {
                    parserutils_vector *new_tokens;
                    if (css__tokens_clone(src->cache[i].resolved_tokens, &new_tokens) == PARSERUTILS_OK) {
                        ctx->cache[ctx->cache_count].src_hash = src->cache[i].src_hash;
                        ctx->cache[ctx->cache_count].resolved_tokens = new_tokens;
                        ctx->cache_count++;
                    }
                }
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

    css__var_cache_clear(ctx);

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
    css__var_cache_clear(ctx);

    for (uint32_t i = 0; i < ctx->count; i++) {
        if (ctx->entries[i].name == name) {
            css__tokens_destroy(ctx->entries[i].tokens);
            css__tokens_clone(tokens, &ctx->entries[i].tokens);
            ctx->hash = css__variables_ctx_compute_hash(ctx);
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

    ctx->hash = css__variables_ctx_compute_hash(ctx);

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
