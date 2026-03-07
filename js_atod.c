double js_atod(const char *str, const char **pnext, int radix, int flags, JSATODTempMem *tmp_mem)
{
    uint64_t *mptr = tmp_mem->mem;
    const char *p, *p_start;
    limb_t cur_limb, radix_base, extra_digits;
    int is_neg, digit_count, limb_digit_count, digits_per_limb, sep, radix1, radix_shift;
    int radix_bits, expn, e, max_digits, expn_offset, dot_pos, sig_pos, pos;
    mpb_t *tmp0;
    double dval;
    bool is_bin_exp, is_zero, expn_overflow;
    uint64_t m, a;

    tmp0 = dtoa_malloc(&mptr, sizeof(mpb_t) + sizeof(limb_t) * DBIGNUM_LEN_MAX);
    assert((mptr - tmp_mem->mem) <= sizeof(JSATODTempMem) / sizeof(mptr[0]));
    /* optional separator between digits */
    sep = (flags & JS_ATOD_ACCEPT_UNDERSCORES) ? '_' : 256;

    p = str;
    is_neg = 0;
    if (p[0] == '+') {
        p++;
        p_start = p;
    } else if (p[0] == '-') {
        is_neg = 1;
        p++;
        p_start = p;
    } else {
        p_start = p;
    }

    if (p[0] == '0') {
        if ((p[1] == 'x' || p[1] == 'X') && (radix == 0 || radix == 16)) {
            p += 2;
            radix = 16;
        } else if ((p[1] == 'o' || p[1] == 'O') && radix == 0 && (flags & JS_ATOD_ACCEPT_BIN_OCT)) {
            p += 2;
            radix = 8;
        } else if ((p[1] == 'b' || p[1] == 'B') && radix == 0 && (flags & JS_ATOD_ACCEPT_BIN_OCT)) {
            p += 2;
            radix = 2;
        } else if ((p[1] >= '0' && p[1] <= '9') && radix == 0 && (flags & JS_ATOD_ACCEPT_LEGACY_OCTAL)) {
            int i;
            sep = 256;
            for (i = 1; (p[i] >= '0' && p[i] <= '7'); i++)
                continue;
            if (p[i] == '8' || p[i] == '9')
                goto no_prefix;
            p += 1;
            radix = 8;
        } else {
            goto no_prefix;
        }
        /* there must be a digit after the prefix */
        if (to_digit((uint8_t)*p) >= radix)
            goto fail;
    no_prefix:;
    } else {
        if (!(flags & JS_ATOD_INT_ONLY) && js__strstart(p, "Infinity", &p))
            goto overflow;
    }
    if (radix == 0)
        radix = 10;

    cur_limb = 0;
    expn_offset = 0;
    digit_count = 0;
    limb_digit_count = 0;
    max_digits = atod_max_digits_table[radix - 2];
    digits_per_limb = digits_per_limb_table[radix - 2];
    radix_base = radix_base_table[radix - 2];
    radix_shift = ctz32(radix);
    radix1 = radix >> radix_shift;
    if (radix1 == 1) {
        /* radix = 2^radix_bits */
        radix_bits = radix_shift;
    } else {
        radix_bits = 0;
    }
    tmp0->len = 1;
    tmp0->tab[0] = 0;
    extra_digits = 0;
    pos = 0;
    dot_pos = -1;
    /* skip leading zeros */
    for (;;) {
        if (*p == '.' && (p > p_start || to_digit(p[1]) < radix) && !(flags & JS_ATOD_INT_ONLY)) {
            if (*p == sep)
                goto fail;
            if (dot_pos >= 0)
                break;
            dot_pos = pos;
            p++;
        }
        if (*p == sep && p > p_start && p[1] == '0')
            p++;
        if (*p != '0')
            break;
        p++;
        pos++;
    }

    sig_pos = pos;
    for (;;) {
        limb_t c;
        if (*p == '.' && (p > p_start || to_digit(p[1]) < radix) && !(flags & JS_ATOD_INT_ONLY)) {
            if (*p == sep)
                goto fail;
            if (dot_pos >= 0)
                break;
            dot_pos = pos;
            p++;
        }
        if (*p == sep && p > p_start && to_digit(p[1]) < radix)
            p++;
        c = to_digit(*p);
        if (c >= radix)
            break;
        p++;
        pos++;
        if (digit_count < max_digits) {
            if (radix_bits != 0) {
                cur_limb = (cur_limb << radix_bits) | c;
            } else {
                cur_limb = cur_limb * radix + c;
            }
            limb_digit_count++;
            if (limb_digit_count == digits_per_limb) {
                mpb_mul1_base(tmp0, radix_base, cur_limb);
                cur_limb = 0;
                limb_digit_count = 0;
            }
            digit_count++;
        } else {
            extra_digits |= c;
        }
    }
    if (limb_digit_count != 0) {
        mpb_mul1_base(tmp0, pow_ui(radix, limb_digit_count), cur_limb);
    }
    if (digit_count == 0) {
        is_zero = true;
        expn_offset = 0;
    } else {
        is_zero = false;
        if (dot_pos < 0)
            dot_pos = pos;
        expn_offset = sig_pos + digit_count - dot_pos;
    }

    /* Use the extra digits for rounding if the base is a power of
       two. Otherwise they are just truncated. */
    if (radix_bits != 0 && extra_digits != 0) {
        tmp0->tab[0] |= 1;
    }

    /* parse the exponent, if any */
    expn = 0;
    expn_overflow = false;
    is_bin_exp = false;
    if (!(flags & JS_ATOD_INT_ONLY) &&
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
        c = to_digit(*p);
        if (c >= 10) {
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
            /* if zero result, the exponent can be arbitrarily large */
            if (!is_zero && expn_overflow) {
                if (exp_is_neg)
                    a = 0;
                else
                    a = (uint64_t)0x7ff << 52; /* infinity */
                goto done;
            }
        }
    }

    if (p == p_start)
        goto fail;

    if (is_zero) {
        a = 0;
    } else {
        int expn1;
        if (radix_bits != 0) {
            if (!is_bin_exp)
                expn *= radix_bits;
            expn -= expn_offset * radix_bits;
            expn1 = expn + digit_count * radix_bits;
            if (expn1 >= 1024 + radix_bits)
                goto overflow;
            else if (expn1 <= -1075)
                goto underflow;
            m = round_to_d(&e, tmp0, -expn, JS_RNDN);
        } else {
            expn -= expn_offset;
            expn1 = expn + digit_count;
            if (expn1 >= max_exponent[radix - 2] + 1)
                goto overflow;
            else if (expn1 <= min_exponent[radix - 2])
                goto underflow;
            m = mul_pow_round_to_d(&e, tmp0, radix1, radix_shift, expn, JS_RNDN);
        }
        if (m == 0) {
        underflow:
            a = 0;
        } else if (e > 1024) {
        overflow:
            /* overflow */
            a = (uint64_t)0x7ff << 52;
        } else if (e < -1073) {
            /* underflow */
            /* XXX: check rounding */
            a = 0;
        } else if (e < -1021) {
            /* subnormal */
            a = m >> (-e - 1021);
        } else {
            a = ((uint64_t)(e + 1022) << 52) | (m & (((uint64_t)1 << 52) - 1));
        }
    }
done:
    a |= (uint64_t)is_neg << 63;
    dval = uint64_as_float64(a);
done1:
    if (pnext)
        *pnext = p;
    dtoa_free(tmp0);
    return dval;
fail:
    dval = NAN;
    goto done1;
}
