#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

#define unlikely(x) (x)

int to_digit(int c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'A' && c <= 'Z')
        return c - 'A' + 10;
    else if (c >= 'a' && c <= 'z')
        return c - 'a' + 10;
    else
        return 36;
}

int main() {
    const char *p_start = "1.5e-";
    const char *p = p_start + 3; // pointing to 'e'
    int flags = 0;
    int radix = 10;
    int radix_bits = 0;
    int sep = 256;
    int expn = 0;
    bool expn_overflow = false;
    bool is_bin_exp = false;

    // Simulating the condition
    if (!(flags & 1) && // JS_ATOD_INT_ONLY
        ((radix == 10 && (*p == 'e' || *p == 'E')) ||
            (radix != 10 && (*p == '@' || (radix_bits >= 1 && radix_bits <= 4 && (*p == 'p' || *p == 'P'))))) &&
        p > p_start) {

        const char *p_exp = p;
        bool exp_is_neg;
        int c;
        is_bin_exp = (*p == 'p' || *p == 'P');
        p++;
        exp_is_neg = false;
        if (*p == '+') {
            p++;
        } else if (*p == '-') {
            exp_is_neg = true;
            p++;
        }

        printf("Before parsing exponent digits: p points to '%c' (0x%x)\n", *p ? *p : 'N', *p);

        c = to_digit(*p);
        if (c >= 10) {
            printf("No digits after exponent indicator! Reverting to p_exp.\n");
            p = p_exp;
        } else {
            expn = c;
            p++;
            for (;;) {
                if (*p == sep && to_digit(p[1]) < 10)
                    p++;
                c = to_digit(*p);
                if (c >= 10)
                    break;
                if (!expn_overflow) {
                    if (unlikely(expn > ((INT32_MAX - 2 - 9) / 10))) {
                        expn_overflow = true;
                    } else {
                        expn = expn * 10 + c;
                    }
                }
                p++;
            }
            if (exp_is_neg)
                expn = -expn;
        }
    }

    printf("Final pointer points to '%c' (0x%x)\n", *p ? *p : 'N', *p);

    return 0;
}
