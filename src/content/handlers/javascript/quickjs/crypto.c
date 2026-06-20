/*
 * Copyright 2026 Wisp Contributors
 *
 * This file is part of Wisp, http://www.netsurf-browser.org/
 */

#include "crypto.h"
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <string.h>
#include <wisp/utils/log.h>

static JSValue js_crypto_getRandomValues(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc < 1 || !JS_IsObject(argv[0])) {
        return JS_ThrowTypeError(ctx, "Expected TypedArray");
    }

    size_t offset, byte_length, bytes_per_element;
    JSValue buffer = JS_GetTypedArrayBuffer(ctx, argv[0], &offset, &byte_length, &bytes_per_element);
    if (JS_IsException(buffer)) {
        return buffer;
    }

    size_t buf_len;
    uint8_t *ptr = JS_GetArrayBuffer(ctx, &buf_len, buffer);
    if (!ptr) {
        JS_FreeValue(ctx, buffer);
        return JS_ThrowTypeError(ctx, "Failed to get ArrayBuffer pointer");
    }

    if (RAND_bytes(ptr + offset, byte_length) != 1) {
        JS_FreeValue(ctx, buffer);
        return JS_ThrowInternalError(ctx, "RAND_bytes failed");
    }

    JS_FreeValue(ctx, buffer);
    return JS_DupValue(ctx, argv[0]);
}

static JSValue js_crypto_subtle_digest(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected 2 arguments");
    }

    const char *algo_str = JS_ToCString(ctx, argv[0]);
    if (!algo_str) return JS_EXCEPTION;

    const EVP_MD *md = NULL;
    if (strcasecmp(algo_str, "SHA-256") == 0) {
        md = EVP_sha256();
    } else if (strcasecmp(algo_str, "SHA-1") == 0) {
        md = EVP_sha1();
    } else if (strcasecmp(algo_str, "SHA-512") == 0) {
        md = EVP_sha512();
    }

    JS_FreeCString(ctx, algo_str);

    if (!md) {
        return JS_ThrowTypeError(ctx, "Unsupported digest algorithm");
    }

    size_t data_len;
    uint8_t *data_ptr;
    bool is_typed_array = false;

    if (JS_IsArrayBuffer(argv[1])) {
        data_ptr = JS_GetArrayBuffer(ctx, &data_len, argv[1]);
    } else {
        size_t offset, byte_length, bytes_per_element;
        JSValue buffer = JS_GetTypedArrayBuffer(ctx, argv[1], &offset, &byte_length, &bytes_per_element);
        if (JS_IsException(buffer)) return buffer;
        data_ptr = JS_GetArrayBuffer(ctx, &data_len, buffer);
        data_ptr += offset;
        data_len = byte_length;
        JS_FreeValue(ctx, buffer);
        is_typed_array = true;
    }

    if (!data_ptr) {
        return JS_ThrowTypeError(ctx, "Expected ArrayBuffer or TypedArray");
    }

    uint8_t hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;

    if (EVP_Digest(data_ptr, data_len, hash, &hash_len, md, NULL) != 1) {
        return JS_ThrowInternalError(ctx, "Digest calculation failed");
    }

    JSValue resolving_funcs[2];
    JSValue promise = JS_NewPromiseCapability(ctx, resolving_funcs);
    if (JS_IsException(promise)) return promise;

    JSValue result = JS_NewArrayBufferCopy(ctx, hash, hash_len);
    if (JS_IsException(result)) {
        JS_FreeValue(ctx, resolving_funcs[0]);
        JS_FreeValue(ctx, resolving_funcs[1]);
        JS_FreeValue(ctx, promise);
        return result;
    }

    JS_Call(ctx, resolving_funcs[0], JS_UNDEFINED, 1, &result);
    JS_FreeValue(ctx, result);
    JS_FreeValue(ctx, resolving_funcs[0]);
    JS_FreeValue(ctx, resolving_funcs[1]);

    return promise;
}

static const JSCFunctionListEntry js_crypto_subtle_funcs[] = {
    JS_CFUNC_DEF("digest", 2, js_crypto_subtle_digest),
};

static const JSCFunctionListEntry js_crypto_funcs[] = {
    JS_CFUNC_DEF("getRandomValues", 1, js_crypto_getRandomValues),
};

int qjs_init_crypto(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue crypto = JS_NewObject(ctx);
    JSValue subtle = JS_NewObject(ctx);

    JS_SetPropertyFunctionList(ctx, crypto, js_crypto_funcs, sizeof(js_crypto_funcs) / sizeof(js_crypto_funcs[0]));
    JS_SetPropertyFunctionList(ctx, subtle, js_crypto_subtle_funcs, sizeof(js_crypto_subtle_funcs) / sizeof(js_crypto_subtle_funcs[0]));

    JS_SetPropertyStr(ctx, crypto, "subtle", subtle);
    JS_SetPropertyStr(ctx, global_obj, "crypto", crypto);

    JS_FreeValue(ctx, global_obj);
    return 0;
}
