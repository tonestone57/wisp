/*
 * Copyright 2026 Wisp Contributors
 *
 * This file is part of Wisp, http://www.netsurf-browser.org/
 */

#include "crypto.h"
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <string.h>
#include <stdio.h>
#include <wisp/utils/log.h>

static JSValue js_crypto_randomUUID(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    uint8_t bytes[16];
    if (RAND_bytes(bytes, sizeof(bytes)) != 1) {
        return JS_ThrowInternalError(ctx, "Failed to generate cryptographically secure random bytes");
    }

    /* RFC 4122 Section 4.4: Set version 4 (0100) and variant 1 (10xx) */
    bytes[6] = (bytes[6] & 0x0f) | 0x40;
    bytes[8] = (bytes[8] & 0x3f) | 0x80;

    char uuid[37];
    snprintf(uuid, sizeof(uuid),
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             bytes[0], bytes[1], bytes[2], bytes[3],
             bytes[4], bytes[5], bytes[6], bytes[7],
             bytes[8], bytes[9], bytes[10], bytes[11],
             bytes[12], bytes[13], bytes[14], bytes[15]);

    return JS_NewStringLen(ctx, uuid, 36);
}

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

    /* W3C Web Cryptography API §10.1 QuotaExceededError (64 KiB) */
    if (byte_length > 65536) {
        JS_FreeValue(ctx, buffer);
        return JS_ThrowRangeError(ctx, "QuotaExceededError: byte length exceeds 65536 bytes");
    }
    if (offset + byte_length > buf_len) {
        JS_FreeValue(ctx, buffer);
        return JS_ThrowRangeError(ctx, "TypedArray offset out of bounds");
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

    const char *algo_str = NULL;
    JSValue algo_val = JS_UNDEFINED;

    /* Support both "SHA-256" and { name: "SHA-256" } per WebCrypto spec */
    if (JS_IsObject(argv[0])) {
        algo_val = JS_GetPropertyStr(ctx, argv[0], "name");
        algo_str = JS_ToCString(ctx, algo_val);
    } else {
        algo_str = JS_ToCString(ctx, argv[0]);
    }

    if (!algo_str) {
        JS_FreeValue(ctx, algo_val);
        return JS_EXCEPTION;
    }

    const EVP_MD *md = NULL;
    if (strcasecmp(algo_str, "SHA-256") == 0) {
        md = EVP_sha256();
    } else if (strcasecmp(algo_str, "SHA-1") == 0) {
        md = EVP_sha1();
    } else if (strcasecmp(algo_str, "SHA-512") == 0) {
        md = EVP_sha512();
    } else if (strcasecmp(algo_str, "SHA-384") == 0) {
        md = EVP_sha384();
    }

    JS_FreeCString(ctx, algo_str);
    JS_FreeValue(ctx, algo_val);

    if (!md) {
        return JS_ThrowTypeError(ctx, "Unsupported digest algorithm");
    }

    size_t data_len = 0;
    uint8_t *data_ptr = NULL;
    JSValue backing_buffer = JS_UNDEFINED;

    if (JS_IsArrayBuffer(argv[1])) {
        data_ptr = JS_GetArrayBuffer(ctx, &data_len, argv[1]);
    } else {
        size_t offset, byte_length, bytes_per_element;
        backing_buffer = JS_GetTypedArrayBuffer(ctx, argv[1], &offset, &byte_length, &bytes_per_element);
        if (JS_IsException(backing_buffer)) return backing_buffer;
        data_ptr = JS_GetArrayBuffer(ctx, &data_len, backing_buffer);
        if (data_ptr) {
            data_ptr += offset;
            data_len = byte_length;
        }
    }

    if (!data_ptr) {
        JS_FreeValue(ctx, backing_buffer);
        return JS_ThrowTypeError(ctx, "Expected ArrayBuffer or TypedArray");
    }

    uint8_t hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;

    if (EVP_Digest(data_ptr, data_len, hash, &hash_len, md, NULL) != 1) {
        JS_FreeValue(ctx, backing_buffer);
        return JS_ThrowInternalError(ctx, "Digest calculation failed");
    }
    JS_FreeValue(ctx, backing_buffer);

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
    JS_CFUNC_DEF("randomUUID", 0, js_crypto_randomUUID),
};

int qjs_init_crypto(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);

    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_crypto_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    /* Retrieve existing crypto object if defined, otherwise create a new one */
    JSValue crypto = JS_GetPropertyStr(ctx, global_obj, "crypto");
    if (JS_IsUndefined(crypto) || JS_IsNull(crypto)) {
        crypto = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, global_obj, "crypto", JS_DupValue(ctx, crypto));
    }

    JS_SetPropertyFunctionList(ctx, crypto, js_crypto_funcs, sizeof(js_crypto_funcs) / sizeof(js_crypto_funcs[0]));

    /* Retrieve existing subtle object or create new */
    JSValue subtle = JS_GetPropertyStr(ctx, crypto, "subtle");
    if (JS_IsUndefined(subtle) || JS_IsNull(subtle)) {
        subtle = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, crypto, "subtle", JS_DupValue(ctx, subtle));
    }

    JS_SetPropertyFunctionList(ctx, subtle, js_crypto_subtle_funcs, sizeof(js_crypto_subtle_funcs) / sizeof(js_crypto_subtle_funcs[0]));

    JS_FreeValue(ctx, subtle);
    JS_FreeValue(ctx, crypto);

    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_crypto_init", JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);
    return 0;
}
