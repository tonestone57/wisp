#include <quickjs.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

static bool is_ident_char(uint32_t cp) {
    return (cp >= 'a' && cp <= 'z') ||
           (cp >= 'A' && cp <= 'Z') ||
           (cp >= '0' && cp <= '9') ||
           cp == '-' || cp == '_';
}

static uint32_t utf8_decode_char(const unsigned char *p, size_t remaining, size_t *len_out) {
    if (remaining == 0) {
        *len_out = 0;
        return 0;
    }
    unsigned char c = p[0];
    if (c < 0x80) {
        *len_out = 1;
        return c;
    } else if ((c & 0xe0) == 0xc0 && remaining >= 2) {
        *len_out = 2;
        return ((uint32_t)(c & 0x1f) << 6) | (p[1] & 0x3f);
    } else if ((c & 0xf0) == 0xe0 && remaining >= 3) {
        *len_out = 3;
        return ((uint32_t)(c & 0x0f) << 12) | ((uint32_t)(p[1] & 0x3f) << 6) | (p[2] & 0x3f);
    } else if ((c & 0xf8) == 0xf0 && remaining >= 4) {
        *len_out = 4;
        return ((uint32_t)(c & 0x07) << 18) | ((uint32_t)(p[1] & 0x3f) << 12) | ((uint32_t)(p[2] & 0x3f) << 6) | (p[3] & 0x3f);
    }
    *len_out = 1;
    return c;
}

JSValue js_css_escape(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc < 1) {
        return JS_NewString(ctx, "");
    }

    size_t len = 0;
    const char *ident = JS_ToCStringLen(ctx, &len, argv[0]);
    if (!ident) return JS_EXCEPTION;

    if (len == 0) {
        JS_FreeCString(ctx, ident);
        return JS_NewString(ctx, "");
    }

    /* First pass: count total code points and store first code point */
    size_t total_cp = 0;
    uint32_t first_cp = 0;
    {
        size_t pos = 0;
        while (pos < len) {
            size_t clen = 0;
            uint32_t cp = utf8_decode_char((const unsigned char *)ident + pos, len - pos, &clen);
            if (clen == 0) break;
            if (total_cp == 0) first_cp = cp;
            total_cp++;
            pos += clen;
        }
    }

    /* Allocate buffer capable of holding max escapes (\\H+space per char) */
    size_t cap = len * 6 + 16;
    char *out = js_malloc(ctx, cap);
    if (!out) {
        JS_FreeCString(ctx, ident);
        return JS_ThrowOutOfMemory(ctx);
    }

    size_t out_idx = 0;
    size_t cp_index = 0;
    size_t pos = 0;

    while (pos < len) {
        size_t clen = 0;
        uint32_t cp = utf8_decode_char((const unsigned char *)ident + pos, len - pos, &clen);
        if (clen == 0) break;

        /* U+0000 NULL: Replace with replacement character U+FFFD */
        if (cp == 0) {
            if (out_idx + 3 < cap) {
                out[out_idx++] = '\xef';
                out[out_idx++] = '\xbf';
                out[out_idx++] = '\xbd';
            }
            pos += clen;
            cp_index++;
            continue;
        }

        /* Control characters (0x01 to 0x1F or 0x7F) */
        if ((cp >= 0x01 && cp <= 0x1F) || cp == 0x7F) {
            size_t avail = cap > out_idx ? cap - out_idx : 0;
            int written = snprintf(out + out_idx, avail, "\\%" PRIx32 " ", cp);
            if (written > 0) {
                out_idx += written;
                if (out_idx >= cap) out_idx = cap - 1;
            }
            pos += clen;
            cp_index++;
            continue;
        }

        /* First character handling */
        if (cp_index == 0) {
            if (cp >= '0' && cp <= '9') {
                size_t avail = cap > out_idx ? cap - out_idx : 0;
                int written = snprintf(out + out_idx, avail, "\\%" PRIx32 " ", cp);
                if (written > 0) {
                    out_idx += written;
                    if (out_idx >= cap) out_idx = cap - 1;
                }
                pos += clen;
                cp_index++;
                continue;
            }
            if (cp == '-' && total_cp == 1) {
                if (out_idx + 2 < cap) {
                    out[out_idx++] = '\\';
                    out[out_idx++] = '-';
                }
                pos += clen;
                cp_index++;
                continue;
            }
        }

        /* Second character is digit preceded by '-' */
        if (cp_index == 1 && first_cp == '-' && (cp >= '0' && cp <= '9')) {
            size_t avail = cap > out_idx ? cap - out_idx : 0;
            int written = snprintf(out + out_idx, avail, "\\%" PRIx32 " ", cp);
            if (written > 0) {
                out_idx += written;
                if (out_idx >= cap) out_idx = cap - 1;
            }
            pos += clen;
            cp_index++;
            continue;
        }

        /* Non-ASCII characters (>= 0x80) or valid identifier characters */
        if (cp >= 0x80 || is_ident_char(cp)) {
            for (size_t b = 0; b < clen && out_idx < cap; b++) {
                out[out_idx++] = ident[pos + b];
            }
        } else {
            /* Any other character (e.g. #, ., :, spaces, symbols): escape with backslash */
            if (out_idx + 1 < cap) {
                out[out_idx++] = '\\';
            }
            for (size_t b = 0; b < clen && out_idx < cap; b++) {
                out[out_idx++] = ident[pos + b];
            }
        }

        pos += clen;
        cp_index++;
    }

    out[out_idx] = '\0';
    JS_FreeCString(ctx, ident);

    JSValue res = JS_NewStringLen(ctx, out, out_idx);
    js_free(ctx, out);
    return res;
}
