#ifndef WISP_QUICKJS_BASE64_HELPER_H
#define WISP_QUICKJS_BASE64_HELPER_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include <nsutils/base64.h>

static inline bool is_latin1_utf8(const char *str) {
    if (!str) return true;
    const uint8_t *p = (const uint8_t *)str;
    while (*p) {
        if (*p <= 0x7F) {
            p++;
        } else if ((*p & 0xE0) == 0xC0) {
            /* 2-byte sequence */
            uint8_t b1 = *p++;
            if (*p == '\0') return false; /* Malformed UTF-8 */
            p++;
            if (b1 >= 0xC4) {
                return false; /* Code point >= 256 */
            }
        } else {
            /* 3-byte, 4-byte, or malformed sequence */
            return false;
        }
    }
    return true;
}

static inline uint8_t *utf8_to_latin1_alloc(const char *str, size_t *out_len) {
    if (!str) {
        *out_len = 0;
        return NULL;
    }
    size_t len = strlen(str);
    uint8_t *buf = malloc(len + 1);
    if (!buf) return NULL;

    const uint8_t *p = (const uint8_t *)str;
    size_t idx = 0;
    while (*p) {
        if (*p <= 0x7F) {
            buf[idx++] = *p++;
        } else if ((*p & 0xE0) == 0xC0) {
            uint8_t b1 = *p++;
            uint8_t b2 = *p++;
            uint8_t cp = ((b1 & 0x1F) << 6) | (b2 & 0x3F);
            buf[idx++] = cp;
        } else {
            free(buf);
            return NULL;
        }
    }
    buf[idx] = '\0';
    *out_len = idx;
    return buf;
}

static inline char *latin1_to_utf8_alloc(const uint8_t *buf, size_t len) {
    if (!buf) return NULL;
    size_t alloc_size = 0;
    for (size_t i = 0; i < len; i++) {
        if (buf[i] <= 0x7F) alloc_size += 1;
        else alloc_size += 2;
    }
    char *out = malloc(alloc_size + 1);
    if (!out) return NULL;

    size_t idx = 0;
    for (size_t i = 0; i < len; i++) {
        uint8_t cp = buf[i];
        if (cp <= 0x7F) {
            out[idx++] = cp;
        } else {
            out[idx++] = 0xC0 | (cp >> 6);
            out[idx++] = 0x80 | (cp & 0x3F);
        }
    }
    out[idx] = '\0';
    return out;
}

static inline char *strip_whitespace_alloc(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char *out = malloc(len + 1);
    if (!out) return NULL;
    size_t idx = 0;
    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f') {
            continue;
        }
        out[idx++] = c;
    }
    out[idx] = '\0';
    return out;
}

static inline bool is_valid_base64(const char *str) {
    if (!str) return true;
    size_t len = strlen(str);
    size_t padding = 0;
    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        if ((c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '+' || c == '/') {
            if (padding > 0) return false;
        } else if (c == '=') {
            padding++;
            if (padding > 2) return false;
        } else {
            return false;
        }
    }
    if (len % 4 == 1) return false;
    return true;
}

static inline JSValue throw_dom_exception(JSContext *ctx, const char *name, const char *msg) {
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue dom_exception_ctor = JS_GetPropertyStr(ctx, global, "DOMException");
    JS_FreeValue(ctx, global);
    if (JS_IsFunction(ctx, dom_exception_ctor)) {
        JSValue args[2];
        args[0] = JS_NewString(ctx, msg);
        args[1] = JS_NewString(ctx, name);
        JSValue exc = JS_CallConstructor(ctx, dom_exception_ctor, 2, args);
        JS_FreeValue(ctx, args[0]);
        JS_FreeValue(ctx, args[1]);
        JS_FreeValue(ctx, dom_exception_ctor);
        if (!JS_IsException(exc)) {
            return JS_Throw(ctx, exc);
        }
    }
    JS_FreeValue(ctx, dom_exception_ctor);
    /* Fallback to TypeError */
    return JS_ThrowTypeError(ctx, "%s: %s", name, msg);
}

static inline JSValue common_atob(JSContext *ctx, const char *input) {
    if (!input) {
        return JS_ThrowTypeError(ctx, "Invalid argument for atob");
    }
    char *stripped = strip_whitespace_alloc(input);
    if (!stripped) {
        return JS_ThrowOutOfMemory(ctx);
    }
    if (!is_valid_base64(stripped)) {
        free(stripped);
        return throw_dom_exception(ctx, "InvalidCharacterError", "The string to be decoded is not correctly encoded.");
    }
    size_t stripped_len = strlen(stripped);
    if (stripped_len == 0) {
        free(stripped);
        return JS_NewString(ctx, "");
    }

    uint8_t *decoded = NULL;
    size_t decoded_len = 0;
    nsuerror err = nsu_base64_decode_alloc((const uint8_t *)stripped, stripped_len, &decoded, &decoded_len);
    free(stripped);

    if (err != NSUERROR_OK) {
        return throw_dom_exception(ctx, "InvalidCharacterError", "The string to be decoded is not correctly encoded.");
    }

    char *utf8_str = latin1_to_utf8_alloc(decoded, decoded_len);
    free(decoded);
    if (!utf8_str) {
        return JS_ThrowOutOfMemory(ctx);
    }

    JSValue res = JS_NewString(ctx, utf8_str);
    free(utf8_str);
    return res;
}

static inline JSValue common_btoa(JSContext *ctx, const char *input) {
    if (!input) {
        return JS_ThrowTypeError(ctx, "Invalid argument for btoa");
    }
    if (!is_latin1_utf8(input)) {
        return throw_dom_exception(ctx, "InvalidCharacterError", "The string to be encoded contains characters outside of the Latin1 range.");
    }

    size_t binary_len = 0;
    uint8_t *binary_data = utf8_to_latin1_alloc(input, &binary_len);
    if (!binary_data) {
        return JS_ThrowOutOfMemory(ctx);
    }

    if (binary_len == 0) {
        free(binary_data);
        return JS_NewString(ctx, "");
    }

    uint8_t *encoded = NULL;
    size_t encoded_len = 0;
    nsuerror err = nsu_base64_encode_alloc(binary_data, binary_len, &encoded, &encoded_len);
    free(binary_data);

    if (err != NSUERROR_OK) {
        return JS_ThrowOutOfMemory(ctx);
    }

    JSValue res = JS_NewStringLen(ctx, (const char *)encoded, encoded_len);
    free(encoded);
    return res;
}

#endif /* WISP_QUICKJS_BASE64_HELPER_H */
