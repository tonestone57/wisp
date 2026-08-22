/*
 * Copyright 2026 Wisp Contributors
 *
 * This file is part of Wisp, http://www.netsurf-browser.org/
 */

#include "crypto.h"
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <wisp/utils/log.h>

static JSValue make_resolved_promise(JSContext *ctx, JSValue val)
{
    JSValue resolving_funcs[2];
    JSValue promise = JS_NewPromiseCapability(ctx, resolving_funcs);
    if (JS_IsException(promise)) {
        JS_FreeValue(ctx, val);
        return promise;
    }
    JS_Call(ctx, resolving_funcs[0], JS_UNDEFINED, 1, &val);
    JS_FreeValue(ctx, val);
    JS_FreeValue(ctx, resolving_funcs[0]);
    JS_FreeValue(ctx, resolving_funcs[1]);
    return promise;
}

static JSValue make_rejected_promise(JSContext *ctx, const char *msg)
{
    JSValue resolving_funcs[2];
    JSValue promise = JS_NewPromiseCapability(ctx, resolving_funcs);
    if (JS_IsException(promise)) return promise;
    JSValue err = JS_NewError(ctx);
    JS_SetPropertyStr(ctx, err, "message", JS_NewString(ctx, msg));
    JS_Call(ctx, resolving_funcs[1], JS_UNDEFINED, 1, &err);
    JS_FreeValue(ctx, err);
    JS_FreeValue(ctx, resolving_funcs[0]);
    JS_FreeValue(ctx, resolving_funcs[1]);
    return promise;
}

static uint8_t *get_bytes_from_jsval(JSContext *ctx, JSValueConst val, size_t *out_len, JSValue *out_buf_to_free)
{
    *out_buf_to_free = JS_UNDEFINED;
    if (JS_IsArrayBuffer(val)) {
        return JS_GetArrayBuffer(ctx, out_len, val);
    }
    size_t offset = 0, byte_length = 0, bytes_per_element = 0;
    JSValue buffer = JS_GetTypedArrayBuffer(ctx, val, &offset, &byte_length, &bytes_per_element);
    if (JS_IsException(buffer) || JS_IsUndefined(buffer) || JS_IsNull(buffer)) {
        return NULL;
    }
    size_t buf_len = 0;
    uint8_t *ptr = JS_GetArrayBuffer(ctx, &buf_len, buffer);
    if (!ptr || offset + byte_length > buf_len) {
        JS_FreeValue(ctx, buffer);
        return NULL;
    }
    *out_buf_to_free = buffer;
    *out_len = byte_length;
    return ptr + offset;
}

static bool get_algo_name(JSContext *ctx, JSValueConst algo_val, char *buf, size_t buf_size)
{
    if (buf_size == 0) return false;
    buf[0] = '\0';

    if (JS_IsString(algo_val)) {
        const char *str = JS_ToCString(ctx, algo_val);
        if (!str) return false;
        snprintf(buf, buf_size, "%s", str);
        JS_FreeCString(ctx, str);
        return true;
    } else if (JS_IsObject(algo_val)) {
        JSValue name_val = JS_GetPropertyStr(ctx, algo_val, "name");
        if (JS_IsString(name_val)) {
            const char *str = JS_ToCString(ctx, name_val);
            if (str) {
                snprintf(buf, buf_size, "%s", str);
                JS_FreeCString(ctx, str);
                JS_FreeValue(ctx, name_val);
                return true;
            }
        }
        JS_FreeValue(ctx, name_val);
    }
    return false;
}

static const EVP_MD *get_hash_md(const char *hash_name)
{
    if (!hash_name) return NULL;
    if (strcasecmp(hash_name, "SHA-256") == 0) return EVP_sha256();
    if (strcasecmp(hash_name, "SHA-384") == 0) return EVP_sha384();
    if (strcasecmp(hash_name, "SHA-512") == 0) return EVP_sha512();
    if (strcasecmp(hash_name, "SHA-1") == 0) return EVP_sha1();
    return NULL;
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

    if (offset + byte_length > buf_len) {
        JS_FreeValue(ctx, buffer);
        return JS_ThrowRangeError(ctx, "TypedArray offset out of bounds");
    }
    if (byte_length > 65536) {
        JS_FreeValue(ctx, buffer);
        return JS_ThrowRangeError(ctx, "QuotaExceededError: byteLength exceeds 65536 bytes");
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
        return make_rejected_promise(ctx, "Expected 2 arguments");
    }

    char algo_name[64];
    if (!get_algo_name(ctx, argv[0], algo_name, sizeof(algo_name))) {
        return make_rejected_promise(ctx, "Invalid algorithm parameter");
    }

    const EVP_MD *md = get_hash_md(algo_name);
    if (!md) {
        return make_rejected_promise(ctx, "Unsupported digest algorithm");
    }

    size_t data_len = 0;
    JSValue free_buf = JS_UNDEFINED;
    uint8_t *data_ptr = get_bytes_from_jsval(ctx, argv[1], &data_len, &free_buf);

    if (!data_ptr) {
        JS_FreeValue(ctx, free_buf);
        return make_rejected_promise(ctx, "Expected ArrayBuffer or TypedArray");
    }

    uint8_t hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;

    if (EVP_Digest(data_ptr, data_len, hash, &hash_len, md, NULL) != 1) {
        JS_FreeValue(ctx, free_buf);
        return make_rejected_promise(ctx, "Digest calculation failed");
    }

    JS_FreeValue(ctx, free_buf);
    JSValue result = JS_NewArrayBufferCopy(ctx, hash, hash_len);
    return make_resolved_promise(ctx, result);
}

static JSValue js_crypto_subtle_generateKey(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc < 3) {
        return make_rejected_promise(ctx, "Expected 3 arguments (algorithm, extractable, keyUsages)");
    }

    char algo_name[64] = "";
    get_algo_name(ctx, argv[0], algo_name, sizeof(algo_name));

    bool extractable = JS_ToBool(ctx, argv[1]);

    size_t key_bits = 256;
    if (JS_IsObject(argv[0])) {
        JSValue len_val = JS_GetPropertyStr(ctx, argv[0], "length");
        if (JS_IsNumber(len_val)) {
            uint32_t len_num = 0;
            JS_ToUint32(ctx, &len_num, len_val);
            if (len_num > 0) key_bits = len_num;
        }
        JS_FreeValue(ctx, len_val);
    }

    size_t key_bytes_len = key_bits / 8;
    if (key_bytes_len == 0) key_bytes_len = 32;

    uint8_t *key_buf = malloc(key_bytes_len);
    if (!key_buf) {
        return make_rejected_promise(ctx, "Out of memory");
    }

    if (RAND_bytes(key_buf, key_bytes_len) != 1) {
        free(key_buf);
        return make_rejected_promise(ctx, "Key generation failed");
    }

    JSValue key_obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, key_obj, "type", JS_NewString(ctx, "secret"));
    JS_SetPropertyStr(ctx, key_obj, "extractable", JS_NewBool(ctx, extractable));
    JS_SetPropertyStr(ctx, key_obj, "algorithm", JS_DupValue(ctx, argv[0]));
    JS_SetPropertyStr(ctx, key_obj, "usages", JS_DupValue(ctx, argv[2]));

    JSValue raw_buf = JS_NewArrayBufferCopy(ctx, key_buf, key_bytes_len);
    free(key_buf);
    JS_SetPropertyStr(ctx, key_obj, "__raw_key", raw_buf);

    return make_resolved_promise(ctx, key_obj);
}

static JSValue js_crypto_subtle_importKey(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc < 5) {
        return make_rejected_promise(ctx, "Expected 5 arguments (format, keyData, algorithm, extractable, keyUsages)");
    }

    const char *format = JS_ToCString(ctx, argv[0]);
    if (!format) return make_rejected_promise(ctx, "Invalid format");

    bool extractable = JS_ToBool(ctx, argv[3]);

    size_t key_len = 0;
    JSValue free_buf = JS_UNDEFINED;
    uint8_t *key_ptr = get_bytes_from_jsval(ctx, argv[1], &key_len, &free_buf);

    if (!key_ptr) {
        JS_FreeCString(ctx, format);
        JS_FreeValue(ctx, free_buf);
        return make_rejected_promise(ctx, "Invalid keyData");
    }

    JSValue key_obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, key_obj, "type", JS_NewString(ctx, "secret"));
    JS_SetPropertyStr(ctx, key_obj, "extractable", JS_NewBool(ctx, extractable));
    JS_SetPropertyStr(ctx, key_obj, "algorithm", JS_DupValue(ctx, argv[2]));
    JS_SetPropertyStr(ctx, key_obj, "usages", JS_DupValue(ctx, argv[4]));

    JSValue raw_buf = JS_NewArrayBufferCopy(ctx, key_ptr, key_len);
    JS_FreeValue(ctx, free_buf);
    JS_FreeCString(ctx, format);

    JS_SetPropertyStr(ctx, key_obj, "__raw_key", raw_buf);

    return make_resolved_promise(ctx, key_obj);
}

static JSValue js_crypto_subtle_exportKey(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc < 2) {
        return make_rejected_promise(ctx, "Expected 2 arguments (format, key)");
    }

    JSValue key_obj = argv[1];
    if (!JS_IsObject(key_obj)) {
        return make_rejected_promise(ctx, "Invalid key object");
    }

    JSValue raw_key = JS_GetPropertyStr(ctx, key_obj, "__raw_key");
    if (JS_IsUndefined(raw_key) || JS_IsNull(raw_key)) {
        return make_rejected_promise(ctx, "Key material not found");
    }

    return make_resolved_promise(ctx, raw_key);
}

static JSValue js_crypto_subtle_encrypt(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc < 3) {
        return make_rejected_promise(ctx, "Expected 3 arguments (algorithm, key, data)");
    }

    char algo_name[64] = "";
    get_algo_name(ctx, argv[0], algo_name, sizeof(algo_name));

    JSValue key_obj = argv[1];
    JSValue raw_key_val = JS_GetPropertyStr(ctx, key_obj, "__raw_key");
    size_t key_len = 0;
    uint8_t *key_ptr = JS_GetArrayBuffer(ctx, &key_len, raw_key_val);

    size_t data_len = 0;
    JSValue free_buf = JS_UNDEFINED;
    uint8_t *data_ptr = get_bytes_from_jsval(ctx, argv[2], &data_len, &free_buf);

    if (!key_ptr || !data_ptr) {
        JS_FreeValue(ctx, raw_key_val);
        JS_FreeValue(ctx, free_buf);
        return make_rejected_promise(ctx, "Invalid key or data");
    }

    /* Extract IV if available */
    uint8_t *iv_ptr = NULL;
    size_t iv_len = 0;
    JSValue free_iv_buf = JS_UNDEFINED;
    if (JS_IsObject(argv[0])) {
        JSValue iv_val = JS_GetPropertyStr(ctx, argv[0], "iv");
        if (!JS_IsUndefined(iv_val) && !JS_IsNull(iv_val)) {
            iv_ptr = get_bytes_from_jsval(ctx, iv_val, &iv_len, &free_iv_buf);
        }
        JS_FreeValue(ctx, iv_val);
    }

    const EVP_CIPHER *cipher = NULL;
    bool is_gcm = false;
    if (strcasecmp(algo_name, "AES-GCM") == 0) {
        is_gcm = true;
        cipher = (key_len <= 16) ? EVP_aes_128_gcm() : EVP_aes_256_gcm();
    } else {
        /* Default to AES-CBC */
        cipher = (key_len <= 16) ? EVP_aes_128_cbc() : EVP_aes_256_cbc();
    }

    EVP_CIPHER_CTX *cctx = EVP_CIPHER_CTX_new();
    if (!cctx) {
        JS_FreeValue(ctx, raw_key_val);
        JS_FreeValue(ctx, free_buf);
        JS_FreeValue(ctx, free_iv_buf);
        return make_rejected_promise(ctx, "EVP_CIPHER_CTX_new failed");
    }

    uint8_t dummy_iv[16] = {0};
    if (!iv_ptr) {
        iv_ptr = dummy_iv;
        iv_len = 16;
    }

    if (EVP_EncryptInit_ex(cctx, cipher, NULL, NULL, NULL) != 1) {
        EVP_CIPHER_CTX_free(cctx);
        JS_FreeValue(ctx, raw_key_val);
        JS_FreeValue(ctx, free_buf);
        JS_FreeValue(ctx, free_iv_buf);
        return make_rejected_promise(ctx, "EncryptInit failed");
    }

    if (is_gcm && iv_len > 0) {
        EVP_CIPHER_CTX_ctrl(cctx, EVP_CTRL_GCM_SET_IVLEN, (int)iv_len, NULL);
    }

    if (EVP_EncryptInit_ex(cctx, NULL, NULL, key_ptr, iv_ptr) != 1) {
        EVP_CIPHER_CTX_free(cctx);
        JS_FreeValue(ctx, raw_key_val);
        JS_FreeValue(ctx, free_buf);
        JS_FreeValue(ctx, free_iv_buf);
        return make_rejected_promise(ctx, "EncryptInit key/iv failed");
    }

    size_t out_alloc = data_len + 32;
    uint8_t *out_buf = malloc(out_alloc);
    int out_len1 = 0, out_len2 = 0;

    if (EVP_EncryptUpdate(cctx, out_buf, &out_len1, data_ptr, (int)data_len) != 1) {
        free(out_buf);
        EVP_CIPHER_CTX_free(cctx);
        JS_FreeValue(ctx, raw_key_val);
        JS_FreeValue(ctx, free_buf);
        JS_FreeValue(ctx, free_iv_buf);
        return make_rejected_promise(ctx, "EncryptUpdate failed");
    }

    if (EVP_EncryptFinal_ex(cctx, out_buf + out_len1, &out_len2) != 1) {
        free(out_buf);
        EVP_CIPHER_CTX_free(cctx);
        JS_FreeValue(ctx, raw_key_val);
        JS_FreeValue(ctx, free_buf);
        JS_FreeValue(ctx, free_iv_buf);
        return make_rejected_promise(ctx, "EncryptFinal failed");
    }

    size_t total_len = out_len1 + out_len2;

    if (is_gcm) {
        uint8_t tag[16];
        EVP_CIPHER_CTX_ctrl(cctx, EVP_CTRL_GCM_GET_TAG, 16, tag);
        out_buf = realloc(out_buf, total_len + 16);
        memcpy(out_buf + total_len, tag, 16);
        total_len += 16;
    }

    EVP_CIPHER_CTX_free(cctx);
    JS_FreeValue(ctx, raw_key_val);
    JS_FreeValue(ctx, free_buf);
    JS_FreeValue(ctx, free_iv_buf);

    JSValue res = JS_NewArrayBufferCopy(ctx, out_buf, total_len);
    free(out_buf);

    return make_resolved_promise(ctx, res);
}

static JSValue js_crypto_subtle_decrypt(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc < 3) {
        return make_rejected_promise(ctx, "Expected 3 arguments (algorithm, key, data)");
    }

    char algo_name[64] = "";
    get_algo_name(ctx, argv[0], algo_name, sizeof(algo_name));

    JSValue key_obj = argv[1];
    JSValue raw_key_val = JS_GetPropertyStr(ctx, key_obj, "__raw_key");
    size_t key_len = 0;
    uint8_t *key_ptr = JS_GetArrayBuffer(ctx, &key_len, raw_key_val);

    size_t data_len = 0;
    JSValue free_buf = JS_UNDEFINED;
    uint8_t *data_ptr = get_bytes_from_jsval(ctx, argv[2], &data_len, &free_buf);

    if (!key_ptr || !data_ptr) {
        JS_FreeValue(ctx, raw_key_val);
        JS_FreeValue(ctx, free_buf);
        return make_rejected_promise(ctx, "Invalid key or data");
    }

    uint8_t *iv_ptr = NULL;
    size_t iv_len = 0;
    JSValue free_iv_buf = JS_UNDEFINED;
    if (JS_IsObject(argv[0])) {
        JSValue iv_val = JS_GetPropertyStr(ctx, argv[0], "iv");
        if (!JS_IsUndefined(iv_val) && !JS_IsNull(iv_val)) {
            iv_ptr = get_bytes_from_jsval(ctx, iv_val, &iv_len, &free_iv_buf);
        }
        JS_FreeValue(ctx, iv_val);
    }

    const EVP_CIPHER *cipher = NULL;
    bool is_gcm = false;
    if (strcasecmp(algo_name, "AES-GCM") == 0) {
        is_gcm = true;
        cipher = (key_len <= 16) ? EVP_aes_128_gcm() : EVP_aes_256_gcm();
    } else {
        cipher = (key_len <= 16) ? EVP_aes_128_cbc() : EVP_aes_256_cbc();
    }

    EVP_CIPHER_CTX *cctx = EVP_CIPHER_CTX_new();
    if (!cctx) {
        JS_FreeValue(ctx, raw_key_val);
        JS_FreeValue(ctx, free_buf);
        JS_FreeValue(ctx, free_iv_buf);
        return make_rejected_promise(ctx, "EVP_CIPHER_CTX_new failed");
    }

    uint8_t dummy_iv[16] = {0};
    if (!iv_ptr) {
        iv_ptr = dummy_iv;
        iv_len = 16;
    }

    if (EVP_DecryptInit_ex(cctx, cipher, NULL, NULL, NULL) != 1) {
        EVP_CIPHER_CTX_free(cctx);
        JS_FreeValue(ctx, raw_key_val);
        JS_FreeValue(ctx, free_buf);
        JS_FreeValue(ctx, free_iv_buf);
        return make_rejected_promise(ctx, "DecryptInit failed");
    }

    if (is_gcm && iv_len > 0) {
        EVP_CIPHER_CTX_ctrl(cctx, EVP_CTRL_GCM_SET_IVLEN, (int)iv_len, NULL);
    }

    if (EVP_DecryptInit_ex(cctx, NULL, NULL, key_ptr, iv_ptr) != 1) {
        EVP_CIPHER_CTX_free(cctx);
        JS_FreeValue(ctx, raw_key_val);
        JS_FreeValue(ctx, free_buf);
        JS_FreeValue(ctx, free_iv_buf);
        return make_rejected_promise(ctx, "DecryptInit key/iv failed");
    }

    size_t cipher_bytes_len = data_len;
    uint8_t tag[16];
    if (is_gcm && data_len >= 16) {
        cipher_bytes_len = data_len - 16;
        memcpy(tag, data_ptr + cipher_bytes_len, 16);
        EVP_CIPHER_CTX_ctrl(cctx, EVP_CTRL_GCM_SET_TAG, 16, tag);
    }

    uint8_t *out_buf = malloc(cipher_bytes_len + 32);
    int out_len1 = 0, out_len2 = 0;

    if (EVP_DecryptUpdate(cctx, out_buf, &out_len1, data_ptr, (int)cipher_bytes_len) != 1) {
        free(out_buf);
        EVP_CIPHER_CTX_free(cctx);
        JS_FreeValue(ctx, raw_key_val);
        JS_FreeValue(ctx, free_buf);
        JS_FreeValue(ctx, free_iv_buf);
        return make_rejected_promise(ctx, "DecryptUpdate failed");
    }

    if (EVP_DecryptFinal_ex(cctx, out_buf + out_len1, &out_len2) != 1) {
        free(out_buf);
        EVP_CIPHER_CTX_free(cctx);
        JS_FreeValue(ctx, raw_key_val);
        JS_FreeValue(ctx, free_buf);
        JS_FreeValue(ctx, free_iv_buf);
        return make_rejected_promise(ctx, "DecryptFinal failed / Tag verification failed");
    }

    EVP_CIPHER_CTX_free(cctx);
    JS_FreeValue(ctx, raw_key_val);
    JS_FreeValue(ctx, free_buf);
    JS_FreeValue(ctx, free_iv_buf);

    JSValue res = JS_NewArrayBufferCopy(ctx, out_buf, out_len1 + out_len2);
    free(out_buf);

    return make_resolved_promise(ctx, res);
}

static JSValue js_crypto_subtle_sign(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc < 3) {
        return make_rejected_promise(ctx, "Expected 3 arguments (algorithm, key, data)");
    }

    JSValue key_obj = argv[1];
    JSValue raw_key_val = JS_GetPropertyStr(ctx, key_obj, "__raw_key");
    size_t key_len = 0;
    uint8_t *key_ptr = JS_GetArrayBuffer(ctx, &key_len, raw_key_val);

    size_t data_len = 0;
    JSValue free_buf = JS_UNDEFINED;
    uint8_t *data_ptr = get_bytes_from_jsval(ctx, argv[2], &data_len, &free_buf);

    if (!key_ptr || !data_ptr) {
        JS_FreeValue(ctx, raw_key_val);
        JS_FreeValue(ctx, free_buf);
        return make_rejected_promise(ctx, "Invalid key or data");
    }

    char hash_name[64] = "SHA-256";
    JSValue algo_prop = JS_GetPropertyStr(ctx, key_obj, "algorithm");
    if (JS_IsObject(algo_prop)) {
        JSValue hash_prop = JS_GetPropertyStr(ctx, algo_prop, "hash");
        get_algo_name(ctx, hash_prop, hash_name, sizeof(hash_name));
        JS_FreeValue(ctx, hash_prop);
    }
    JS_FreeValue(ctx, algo_prop);

    const EVP_MD *md = get_hash_md(hash_name);
    if (!md) md = EVP_sha256();

    unsigned char mac[EVP_MAX_MD_SIZE];
    unsigned int mac_len = 0;

    if (!HMAC(md, key_ptr, (int)key_len, data_ptr, data_len, mac, &mac_len)) {
        JS_FreeValue(ctx, raw_key_val);
        JS_FreeValue(ctx, free_buf);
        return make_rejected_promise(ctx, "HMAC signing failed");
    }

    JS_FreeValue(ctx, raw_key_val);
    JS_FreeValue(ctx, free_buf);

    JSValue res = JS_NewArrayBufferCopy(ctx, mac, mac_len);
    return make_resolved_promise(ctx, res);
}

static JSValue js_crypto_subtle_verify(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc < 4) {
        return make_rejected_promise(ctx, "Expected 4 arguments (algorithm, key, signature, data)");
    }

    JSValue key_obj = argv[1];
    JSValue raw_key_val = JS_GetPropertyStr(ctx, key_obj, "__raw_key");
    size_t key_len = 0;
    uint8_t *key_ptr = JS_GetArrayBuffer(ctx, &key_len, raw_key_val);

    size_t sig_len = 0;
    JSValue free_sig_buf = JS_UNDEFINED;
    uint8_t *sig_ptr = get_bytes_from_jsval(ctx, argv[2], &sig_len, &free_sig_buf);

    size_t data_len = 0;
    JSValue free_data_buf = JS_UNDEFINED;
    uint8_t *data_ptr = get_bytes_from_jsval(ctx, argv[3], &data_len, &free_data_buf);

    if (!key_ptr || !sig_ptr || !data_ptr) {
        JS_FreeValue(ctx, raw_key_val);
        JS_FreeValue(ctx, free_sig_buf);
        JS_FreeValue(ctx, free_data_buf);
        return make_rejected_promise(ctx, "Invalid parameters");
    }

    char hash_name[64] = "SHA-256";
    JSValue algo_prop = JS_GetPropertyStr(ctx, key_obj, "algorithm");
    if (JS_IsObject(algo_prop)) {
        JSValue hash_prop = JS_GetPropertyStr(ctx, algo_prop, "hash");
        get_algo_name(ctx, hash_prop, hash_name, sizeof(hash_name));
        JS_FreeValue(ctx, hash_prop);
    }
    JS_FreeValue(ctx, algo_prop);

    const EVP_MD *md = get_hash_md(hash_name);
    if (!md) md = EVP_sha256();

    unsigned char mac[EVP_MAX_MD_SIZE];
    unsigned int mac_len = 0;

    if (!HMAC(md, key_ptr, (int)key_len, data_ptr, data_len, mac, &mac_len)) {
        JS_FreeValue(ctx, raw_key_val);
        JS_FreeValue(ctx, free_sig_buf);
        JS_FreeValue(ctx, free_data_buf);
        return make_rejected_promise(ctx, "HMAC verification failed");
    }

    bool match = (sig_len == mac_len) && (memcmp(sig_ptr, mac, mac_len) == 0);

    JS_FreeValue(ctx, raw_key_val);
    JS_FreeValue(ctx, free_sig_buf);
    JS_FreeValue(ctx, free_data_buf);

    return make_resolved_promise(ctx, JS_NewBool(ctx, match));
}

static const JSCFunctionListEntry js_crypto_subtle_funcs[] = {
    JS_CFUNC_DEF("digest", 2, js_crypto_subtle_digest),
    JS_CFUNC_DEF("encrypt", 3, js_crypto_subtle_encrypt),
    JS_CFUNC_DEF("decrypt", 3, js_crypto_subtle_decrypt),
    JS_CFUNC_DEF("sign", 3, js_crypto_subtle_sign),
    JS_CFUNC_DEF("verify", 4, js_crypto_subtle_verify),
    JS_CFUNC_DEF("generateKey", 3, js_crypto_subtle_generateKey),
    JS_CFUNC_DEF("importKey", 5, js_crypto_subtle_importKey),
    JS_CFUNC_DEF("exportKey", 2, js_crypto_subtle_exportKey),
};

static const JSCFunctionListEntry js_crypto_funcs[] = {
    JS_CFUNC_DEF("getRandomValues", 1, js_crypto_getRandomValues),
};

int qjs_init_crypto(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);

    /* Check if already initialized */
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_crypto_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    JSValue crypto = JS_NewObject(ctx);
    JSValue subtle = JS_NewObject(ctx);

    JS_SetPropertyFunctionList(ctx, crypto, js_crypto_funcs, sizeof(js_crypto_funcs) / sizeof(js_crypto_funcs[0]));
    JS_SetPropertyFunctionList(ctx, subtle, js_crypto_subtle_funcs, sizeof(js_crypto_subtle_funcs) / sizeof(js_crypto_subtle_funcs[0]));

    JS_SetPropertyStr(ctx, crypto, "subtle", subtle);
    JS_SetPropertyStr(ctx, global_obj, "crypto", crypto);

    /* Mark as initialized */
    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_crypto_init", JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);
    return 0;
}
