#include <quickjs.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

static bool is_ident_char(unsigned char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '-' || c == '_';
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

    /* Allocate buffer capable of holding max escapes (\\H+space per char) */
    size_t cap = len * 6 + 16;
    char *out = js_malloc(ctx, cap);
    if (!out) {
        JS_FreeCString(ctx, ident);
        return JS_ThrowOutOfMemory(ctx);
    }

    size_t out_idx = 0;

    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)ident[i];

        /* U+0000 NULL: Replace with replacement character U+FFFD */
        if (c == 0) {
            out[out_idx++] = '\xef';
            out[out_idx++] = '\xbf';
            out[out_idx++] = '\xbd';
            continue;
        }

        /* Control characters (0x01 to 0x1F or 0x7F) */
        if ((c >= 0x01 && c <= 0x1F) || c == 0x7F) {
            out_idx += snprintf(out + out_idx, cap - out_idx, "\\%x ", c);
            continue;
        }

        /* First character handling */
        if (i == 0) {
            if (c >= '0' && c <= '9') {
                out_idx += snprintf(out + out_idx, cap - out_idx, "\\%x ", c);
                continue;
            }
            if (c == '-' && len == 1) {
                out[out_idx++] = '\\';
                out[out_idx++] = '-';
                continue;
            }
        }

        /* Second character is digit preceded by '-' */
        if (i == 1 && (unsigned char)ident[0] == '-' && (c >= '0' && c <= '9')) {
            out_idx += snprintf(out + out_idx, cap - out_idx, "\\%x ", c);
            continue;
        }

        /* Non-ASCII characters (>= 0x80) or valid identifier characters */
        if (c >= 0x80 || is_ident_char(c)) {
            out[out_idx++] = c;
        } else {
            /* Any other character (e.g. #, ., :, spaces, symbols): escape with backslash */
            out[out_idx++] = '\\';
            out[out_idx++] = c;
        }
    }

    out[out_idx] = '\0';
    JS_FreeCString(ctx, ident);

    JSValue res = JS_NewStringLen(ctx, out, out_idx);
    js_free(ctx, out);
    return res;
}
