/*
 * Copyright 2014 Chris Young <chris@unsatisfactorysoftware.co.uk>
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 * NetSurf international domain name handling implementation.
 */

#include <sys/types.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <wisp/ns_inttypes.h>
#include <inttypes.h>

#include <wisp/utils/errors.h>
#include <wisp/utils/log.h>
#include <wisp/utils/utf8.h>
#include <wisp/utils/utils.h>
#include "utils/idna.h"
#include "utils/idna_props.h"
#include "utils/punycode.h"


/**
 * Convert punycode status into nserror.
 *
 * \param status The punycode status to convert.
 * \return The corresponding nserror code for the status.
 */
static nserror punycode_status_to_nserror(enum punycode_status status)
{
    nserror ret = NSERROR_NOMEM;

    switch (status) {
    case punycode_success:
        ret = NSERROR_OK;
        break;

    case punycode_bad_input:
        NSLOG(wisp, INFO, "Bad input");
        ret = NSERROR_BAD_ENCODING;
        break;

    case punycode_big_output:
        NSLOG(wisp, INFO, "Output too big");
        ret = NSERROR_BAD_SIZE;
        break;

    case punycode_overflow:
        NSLOG(wisp, INFO, "Overflow");
        ret = NSERROR_NOSPACE;
        break;

    default:
        break;
    }
    return ret;
}


/**
 * Convert a host label in UCS-4 to an ACE version
 *
 * \param ucs4_label UCS-4 NFC string containing host label
 * \param len Length of host label (in characters/codepoints)
 * \param ace_label ASCII-compatible encoded version
 * \param out_len Length of ace_label
 * \return NSERROR_OK on success, appropriate error otherwise
 *
 * If return value != NSERROR_OK, output will be left untouched.
 */
static nserror idna__ucs4_to_ace(int32_t *ucs4_label, size_t len, char **ace_label, size_t *out_len)
{
    char punycode[65]; /* max length of host label + NULL */
    size_t output_length = 60; /* punycode length - 4 - 1 */
    nserror ret;

    punycode[0] = 'x';
    punycode[1] = 'n';
    punycode[2] = '-';
    punycode[3] = '-';

    ret = punycode_status_to_nserror(
        punycode_encode(len, (const punycode_uint *)ucs4_label, NULL, &output_length, punycode + 4));
    if (ret != NSERROR_OK) {
        return ret;
    }

    output_length += SLEN("xn--");
    punycode[output_length] = '\0';

    *ace_label = strdup(punycode);
    if (*ace_label == NULL) {
        return NSERROR_NOMEM;
    }
    *out_len = output_length;

    return NSERROR_OK;
}


/**
 * Convert a host label in ACE format to UCS-4
 *
 * \param ace_label ASCII string containing host label
 * \param ace_len Length of host label
 * \param ucs4_label Pointer to hold UCS4 decoded version
 * \param ucs4_len Pointer to hold length of ucs4_label
 * \return NSERROR_OK on success, appropriate error otherwise
 *
 * If return value != NSERROR_OK, output will be left untouched.
 */
static nserror idna__ace_to_ucs4(const char *ace_label, size_t ace_len, int32_t **ucs4_label, size_t *ucs4_len)
{
    int32_t *ucs4;
    nserror ret;
    size_t output_length = ace_len; /* never exceeds input length */

    if (ace_len < 4) {
        return NSERROR_BAD_PARAMETER;
    }

    /* The header should always have been checked before calling */
    assert((ace_label[0] == 'x') && (ace_label[1] == 'n') && (ace_label[2] == '-') && (ace_label[3] == '-'));

    if (output_length > SIZE_MAX / 4) {
        return NSERROR_BAD_SIZE;
    }

    ucs4 = malloc(output_length * 4);
    if (ucs4 == NULL) {
        return NSERROR_NOMEM;
    }

    ret = punycode_status_to_nserror(
        punycode_decode(ace_len - 4, ace_label + 4, &output_length, (punycode_uint *)ucs4, NULL));
    if (ret != NSERROR_OK) {
        free(ucs4);
        return ret;
    }

    ucs4[output_length] = '\0';

    *ucs4_label = ucs4;
    *ucs4_len = output_length;

    return NSERROR_OK;
}


#ifdef WITH_UTF8PROC
#include <utf8proc.h>
#include "wisp/utils/utf8proc_wrapper.h"

/**
 * Find the IDNA property of a UCS-4 codepoint
 *
 * \param cp	Unicode codepoint
 * \return IDNA property
 */
static idna_property idna__cp_property(int32_t cp)
{
    const idna_table *t;

    t = idna_derived;
    while (t->p.property) {
        if ((cp >= t->start) && (cp <= t->end)) {
            return t->p.property;
        }
        t++;
    };

    return IDNA_P_DISALLOWED;
}


/**
 * Find the Joining_Type property of a UCS-4 codepoint
 *
 * \param cp	Unicode codepoint
 * \return JT property
 */
static idna_unicode_jt idna__jt_property(int32_t cp)
{
    const idna_table *t;

    t = idna_joiningtype;
    while (t->p.jt) {
        if ((cp >= t->start) && (cp <= t->end)) {
            return t->p.jt;
        }
        t++;
    };

    return IDNA_UNICODE_JT_U;
}


static bool is_greek(int32_t cp)
{
    return (cp >= 0x0370 && cp <= 0x03FF) || /* Greek and Coptic */
           (cp >= 0x1F00 && cp <= 0x1FFF);   /* Greek Extended */
}

static bool is_hebrew(int32_t cp)
{
    return (cp >= 0x0590 && cp <= 0x05FF);
}

static bool is_hiragana_katakana_han(int32_t cp)
{
    if (cp == 0x3005) return true; /* Ideographic Iteration Mark (々) */

    return (cp >= 0x3040 && cp <= 0x309F) ||   /* Hiragana */
           (cp >= 0x30A0 && cp <= 0x30FF) ||   /* Katakana */
           (cp >= 0x3400 && cp <= 0x4DBF) ||   /* CJK Unified Ideographs Extension A */
           (cp >= 0x4E00 && cp <= 0x9FFF) ||   /* CJK Unified Ideographs */
           (cp >= 0xF900 && cp <= 0xFAFF) ||   /* CJK Compatibility Ideographs */
           (cp >= 0x20000 && cp <= 0x2A6DF) || /* CJK Unified Ideographs Extension B */
           (cp >= 0x2A700 && cp <= 0x2B73F) || /* CJK Unified Ideographs Extension C */
           (cp >= 0x2B740 && cp <= 0x2B81F) || /* CJK Unified Ideographs Extension D */
           (cp >= 0x2B820 && cp <= 0x2CEAF) || /* CJK Unified Ideographs Extension E */
           (cp >= 0x2F800 && cp <= 0x2FA1F);   /* CJK Compatibility Ideographs Supplement */
}

/**
 * Check if a CONTEXTO codepoint has a rule defined,
 * and conforms to that rule.
 *
 * \param label UCS-4 string
 * \param index character in the string which is CONTEXTO
 * \param len The length of the label
 * \return true if conforming
 */
static bool idna__contexto_rule(int32_t *label, size_t index, size_t len)
{
    int32_t cp = label[index];
    size_t i;

    if (cp == 0x00b7) {
        if (index > 0 && index < (len - 1)) {
            if (label[index - 1] == 0x006c && label[index + 1] == 0x006c) {
                return true;
            }
        }
        return false;
    } else if (cp == 0x0375) {
        if (index < (len - 1)) {
            if (is_greek(label[index + 1])) {
                return true;
            }
        }
        return false;
    } else if (cp == 0x05f3 || cp == 0x05f4) {
        if (index > 0) {
            if (is_hebrew(label[index - 1])) {
                return true;
            }
        }
        return false;
    } else if (cp == 0x30fb) {
        for (i = 0; i < len; i++) {
            if (i == index) continue;
            if (is_hiragana_katakana_han(label[i])) {
                return true;
            }
        }
        return false;
    } else if (cp >= 0x0660 && cp <= 0x0669) {
        for (i = 0; i < len; i++) {
            if (label[i] >= 0x06f0 && label[i] <= 0x06f9) {
                return false;
            }
        }
        return true;
    } else if (cp >= 0x06f0 && cp <= 0x06f9) {
        for (i = 0; i < len; i++) {
            if (label[i] >= 0x0660 && label[i] <= 0x0669) {
                return false;
            }
        }
        return true;
    }

    return false;
}


/**
 * Check if a CONTEXTJ codepoint has a rule defined,
 * and conforms to that rule.
 *
 * \param label UCS-4 string
 * \param index	character in the string which is CONTEXTJ
 * \param len The length of the label
 * \return true if conforming
 */
static bool idna__contextj_rule(int32_t *label, int index, size_t len)
{
    const utf8proc_property_t *unicode_props;
    idna_unicode_jt joining_type;
    int i;
    bool match;

    /* These CONTEXTJ rules are defined at
     * http://www.iana.org/assignments/idna-tables-5.2.0/idna-tables-5.2.0.xml
     */

    if (label[index] == 0x200c) {
        if (index == 0) {
            return false; /* No previous character */
        }
        unicode_props = utf8proc_get_property(label[index - 1]);
        if (unicode_props->combining_class == IDNA_UNICODE_CCC_VIRAMA) {
            return true;
        }

        match = false;
        for (i = 0; i < (index - 1); i++) {
            joining_type = idna__jt_property(label[i]);
            if (((joining_type == IDNA_UNICODE_JT_L) || (joining_type == IDNA_UNICODE_JT_D)) &&
                (idna__jt_property(label[i + 1]) == IDNA_UNICODE_JT_T)) {
                match = true;
                break;
            }
        }

        if (match == false) {
            return false;
        }

        if (idna__jt_property(label[index + 1]) != IDNA_UNICODE_JT_T) {
            return false;
        }

        for (i = (index + 1); i < (int)len; i++) {
            joining_type = idna__jt_property(label[i]);

            if ((joining_type == IDNA_UNICODE_JT_R) || (joining_type == IDNA_UNICODE_JT_D)) {
                return true;
            }
        }

        return false;

    } else if (label[index] == 0x200d) {
        if (index == 0) {
            return false; /* No previous character */
        }
        unicode_props = utf8proc_get_property(label[index - 1]);
        if (unicode_props->combining_class == IDNA_UNICODE_CCC_VIRAMA) {
            return true;
        }
        return false;
    }

    /* No rule defined */
    return false;
}


/**
 * Convert a UTF-8 string to UCS-4
 *
 * \param utf8_label	UTF-8 string containing host label
 * \param len	Length of host label (in bytes)
 * \param ucs4_label	Pointer to update with the output
 * \param ucs4_len	Pointer to update with the length
 * \return NSERROR_OK on success, appropriate error otherwise
 *
 * If return value != NSERROR_OK, output will be left untouched.
 */
static nserror idna__utf8_to_ucs4(const char *utf8_label, size_t len, int32_t **ucs4_label, size_t *ucs4_len)
{
    int32_t *nfc_label;
    ssize_t nfc_size;

    if (len > SIZE_MAX / 4) {
        return NSERROR_BAD_SIZE;
    }

    nfc_label = malloc(len * 4);
    if (nfc_label == NULL) {
        return NSERROR_NOMEM;
    }

    nfc_size = wisp_utf8proc_decompose(
        (const uint8_t *)utf8_label, len, nfc_label, len * 4, UTF8PROC_STABLE | UTF8PROC_COMPOSE);
    if (nfc_size < 0) {
        free(nfc_label);
        return NSERROR_NOMEM;
    }

    nfc_size = wisp_utf8proc_normalize_utf32(nfc_label, nfc_size, UTF8PROC_STABLE | UTF8PROC_COMPOSE);
    if (nfc_size < 0) {
        free(nfc_label);
        return NSERROR_NOMEM;
    }

    *ucs4_label = nfc_label;
    *ucs4_len = nfc_size;

    return NSERROR_OK;
}


/**
 * Convert a UCS-4 string to UTF-8
 *
 * \param ucs4_label	UCS-4 string containing host label
 * \param ucs4_len	Length of host label (in bytes)
 * \param utf8_label	Pointer to update with the output
 * \param utf8_len	Pointer to update with the length
 * \return NSERROR_OK on success, appropriate error otherwise
 *
 * If return value != NSERROR_OK, output will be left untouched.
 */
static nserror idna__ucs4_to_utf8(const int32_t *ucs4_label, size_t ucs4_len, char **utf8_label, size_t *utf8_len)
{
    int32_t *nfc_label;
    ssize_t nfc_size = ucs4_len;

    if (ucs4_len > (SIZE_MAX - 1) / 4) {
        return NSERROR_BAD_SIZE;
    }

    nfc_label = malloc(1 + ucs4_len * 4);
    if (nfc_label == NULL) {
        return NSERROR_NOMEM;
    }
    memcpy(nfc_label, ucs4_label, ucs4_len * 4);

    nfc_size = wisp_utf8proc_reencode(nfc_label, ucs4_len, UTF8PROC_STABLE | UTF8PROC_COMPOSE);
    if (nfc_size < 0) {
        free(nfc_label);
        return NSERROR_NOMEM;
    }

    *utf8_label = (char *)nfc_label;
    ((char *)nfc_label)[nfc_size] = '\0';
    *utf8_len = nfc_size;

    return NSERROR_OK;
}


/**
 * Check if a host label is valid for IDNA2008
 *
 * \param label	Host label to check (UCS-4)
 * \param len	Length of host label (in characters/codepoints)
 * \return true if compliant, false otherwise
 */
static bool idna__is_valid(int32_t *label, size_t len)
{
    const utf8proc_property_t *unicode_props;
    idna_property idna_prop;
    size_t i = 0;
    bool is_rtl_label = false;
    bool has_rtl = false;
    bool has_en = false;
    bool has_an = false;
    ssize_t last_idx;

    /* 1. Check that the string is NFC.
     * This check is skipped as the conversion to Unicode
     * does normalisation as part of the conversion.
     */

    /* 2. Check characters 3 and 4 are not '--'. */
    if ((len >= 4) && (label[2] == 0x002d) && (label[3] == 0x002d)) {
        NSLOG(wisp, INFO, "Check failed: characters 2 and 3 are '--'");
        return false;
    }

    /* 3. Check the first character is not a combining mark */
    unicode_props = utf8proc_get_property(label[0]);

    if ((unicode_props->category == UTF8PROC_CATEGORY_MN) || (unicode_props->category == UTF8PROC_CATEGORY_MC) ||
        (unicode_props->category == UTF8PROC_CATEGORY_ME)) {
        NSLOG(wisp, INFO, "Check failed: character 0 is a combining mark");
        return false;
    }

    /* Bidi setup: Check if we have RTL characters and determine label type */
    for (i = 0; i < len; i++) {
        utf8proc_bidi_class_t bc = utf8proc_get_property(label[i])->bidi_class;
        if (bc == UTF8PROC_BIDI_CLASS_R || bc == UTF8PROC_BIDI_CLASS_AL || bc == UTF8PROC_BIDI_CLASS_AN) {
            has_rtl = true;
            break;
        }
    }

    if (has_rtl) {
        utf8proc_bidi_class_t first_bc = utf8proc_get_property(label[0])->bidi_class; /* label[0] */
        if (first_bc == UTF8PROC_BIDI_CLASS_R || first_bc == UTF8PROC_BIDI_CLASS_AL) {
            is_rtl_label = true;
        } else if (first_bc == UTF8PROC_BIDI_CLASS_L) {
            is_rtl_label = false;
        } else {
            NSLOG(wisp, INFO, "Bidi check failed: First character must be L, R, or AL");
            return false;
        }
    }

    for (i = 0; i < len; i++) {
        idna_prop = idna__cp_property(label[i]);

        /* 4. Check characters not DISALLOWED by RFC5892 */
        if (idna_prop == IDNA_P_DISALLOWED) {
            NSLOG(
                wisp, INFO, "Check failed: character %" PRIsizet " (%08x) is DISALLOWED", i, (unsigned int)label[i]);
            return false;
        }

        /* 5. Check CONTEXTJ characters conform to defined rules */
        if (idna_prop == IDNA_P_CONTEXTJ) {
            if (idna__contextj_rule(label, i, len) == false) {
                NSLOG(wisp, INFO, "Check failed: character %" PRIsizet " (%08x) does not conform to CONTEXTJ rule",
                    i, (unsigned int)label[i]);
                return false;
            }
        }

        /* 6. Check CONTEXTO characters conform to defined rules */
        if (idna_prop == IDNA_P_CONTEXTO) {
            if (idna__contexto_rule(label, i, len) == false) {
                NSLOG(wisp, INFO, "Check failed: character %" PRIsizet " (%08x) does not conform to CONTEXTO rule", i,
                    (unsigned int)label[i]);
                return false;
            }
        }

        /* 7. Check characters are not UNASSIGNED */
        if (idna_prop == IDNA_P_UNASSIGNED) {
            NSLOG(
                wisp, INFO, "Check failed: character %" PRIsizet " (%08x) is UNASSIGNED", i, (unsigned int)label[i]);
            return false;
        }

        /* 8. check Bidi compliance */
        if (has_rtl) {
            utf8proc_bidi_class_t bc = utf8proc_get_property(label[i])->bidi_class;
            if (is_rtl_label) {
                /* Rule 2 */
                if (bc != UTF8PROC_BIDI_CLASS_R && bc != UTF8PROC_BIDI_CLASS_AL &&
                    bc != UTF8PROC_BIDI_CLASS_AN && bc != UTF8PROC_BIDI_CLASS_EN &&
                    bc != UTF8PROC_BIDI_CLASS_ES && bc != UTF8PROC_BIDI_CLASS_CS &&
                    bc != UTF8PROC_BIDI_CLASS_ET && bc != UTF8PROC_BIDI_CLASS_ON &&
                    bc != UTF8PROC_BIDI_CLASS_BN && bc != UTF8PROC_BIDI_CLASS_NSM) {
                    NSLOG(wisp, INFO, "Bidi check failed: Invalid character class %d in RTL label at %zu", bc, i);
                    return false;
                }
                if (bc == UTF8PROC_BIDI_CLASS_EN) has_en = true;
                if (bc == UTF8PROC_BIDI_CLASS_AN) has_an = true;
            } else {
                /* Rule 5 */
                if (bc != UTF8PROC_BIDI_CLASS_L && bc != UTF8PROC_BIDI_CLASS_EN &&
                    bc != UTF8PROC_BIDI_CLASS_ES && bc != UTF8PROC_BIDI_CLASS_CS &&
                    bc != UTF8PROC_BIDI_CLASS_ET && bc != UTF8PROC_BIDI_CLASS_ON &&
                    bc != UTF8PROC_BIDI_CLASS_BN && bc != UTF8PROC_BIDI_CLASS_NSM) {
                    NSLOG(wisp, INFO, "Bidi check failed: Invalid character class %d in LTR label at %zu", bc, i);
                    return false;
                }
            }
        }
    }

    if (has_rtl) {
        /* Rule 4 */
        if (is_rtl_label && has_en && has_an) {
            NSLOG(wisp, INFO, "Bidi check failed: RTL label cannot have both EN and AN");
            return false;
        }

        /* Rule 3 and 6 */
        last_idx = len - 1;
        while (last_idx >= 0) {
            utf8proc_bidi_class_t bc = utf8proc_get_property(label[last_idx])->bidi_class;
            if (bc != UTF8PROC_BIDI_CLASS_NSM) {
                break;
            }
            last_idx--;
        }

        if (last_idx >= 0) {
            utf8proc_bidi_class_t last_bc = utf8proc_get_property(label[last_idx])->bidi_class;
            if (is_rtl_label) {
                if (last_bc != UTF8PROC_BIDI_CLASS_R && last_bc != UTF8PROC_BIDI_CLASS_AL &&
                    last_bc != UTF8PROC_BIDI_CLASS_EN && last_bc != UTF8PROC_BIDI_CLASS_AN) {
                    NSLOG(wisp, INFO, "Bidi check failed: RTL label must end with R, AL, EN, or AN");
                    return false;
                }
            } else {
                if (last_bc != UTF8PROC_BIDI_CLASS_L && last_bc != UTF8PROC_BIDI_CLASS_EN) {
                    NSLOG(wisp, INFO, "Bidi check failed: LTR label must end with L or EN");
                    return false;
                }
            }
        }
    }

    return true;
}


/**
 * Verify an ACE label is valid
 *
 * \param label	Host label to check
 * \param len	Length of label
 * \return true if valid, false otherwise
 */
static bool idna__verify(const char *label, size_t len)
{
    nserror error;
    int32_t *ucs4;
    char *ace;
    ssize_t ucs4_len;
    size_t u_ucs4_len, ace_len;

    /* Convert our ACE label back to UCS-4 */
    error = idna__ace_to_ucs4(label, len, &ucs4, &u_ucs4_len);
    if (error != NSERROR_OK) {
        return false;
    }

    /* Perform NFC normalisation */
    ucs4_len = wisp_utf8proc_normalize_utf32(ucs4, u_ucs4_len, UTF8PROC_STABLE | UTF8PROC_COMPOSE);
    if (ucs4_len < 0) {
        free(ucs4);
        return false;
    }

    /* Convert the UCS-4 label back to ACE */
    error = idna__ucs4_to_ace(ucs4, (size_t)ucs4_len, &ace, &ace_len);
    free(ucs4);
    if (error != NSERROR_OK) {
        return false;
    }

    /* Check if it matches the input */
    if ((len == ace_len) && (strncmp(label, ace, len) == 0)) {
        free(ace);
        return true;
    }

    NSLOG(wisp, INFO, "Re-encoded ACE label %s does not match input", ace);
    free(ace);

    return false;
}


#else /* WITH_UTF8PROC */


/**
 * Convert a UTF-8 string to UCS-4
 *
 * \param utf8_label	UTF-8 string containing host label
 * \param len	Length of host label (in bytes)
 * \param ucs4_label	Pointer to update with the output
 * \param ucs4_len	Pointer to update with the length
 * \return NSERROR_OK on success, appropriate error otherwise
 *
 * If return value != NSERROR_OK, output will be left untouched.
 */
static nserror idna__utf8_to_ucs4(const char *utf8_label, size_t len, int32_t **ucs4_label, size_t *ucs4_len)
{
    return NSERROR_NOT_IMPLEMENTED;
}


/**
 * Convert a UCS-4 string to UTF-8
 *
 * \param ucs4_label	UCS-4 string containing host label
 * \param ucs4_len	Length of host label (in bytes)
 * \param utf8_label	Pointer to update with the output
 * \param utf8_len	Pointer to update with the length
 * \return NSERROR_OK on success, appropriate error otherwise
 *
 * If return value != NSERROR_OK, output will be left untouched.
 */
static nserror idna__ucs4_to_utf8(const int32_t *ucs4_label, size_t ucs4_len, char **utf8_label, size_t *utf8_len)
{
    return NSERROR_NOT_IMPLEMENTED;
}


/**
 * Check if a host label is valid for IDNA2008
 *
 * \param label	Host label to check (UCS-4)
 * \param len	Length of host label (in characters/codepoints)
 * \return true if compliant, false otherwise
 */
static bool idna__is_valid(int32_t *label, size_t len)
{
    return true;
}


/**
 * Verify an ACE label is valid
 *
 * \param label	Host label to check
 * \param len	Length of label
 * \return true if valid, false otherwise
 */
static bool idna__verify(const char *label, size_t len)
{
    return true;
}


#endif /* WITH_UTF8PROC */


/**
 * Find the length of a host label
 *
 * \param host	String containing a host or FQDN
 * \param max_length	Length of host string to search (in bytes)
 * \return Distance to next separator character or end of string
 */
static size_t idna__host_label_length(const char *host, size_t max_length)
{
    const char *p = host;
    size_t length = 0;

    while (length < max_length) {
        if ((*p == '.') || (*p == ':') || (*p == '\0')) {
            break;
        }
        length++;
        p++;
    }

    return length;
}


/**
 * Check if a host label is LDH
 *
 * \param label	Host label to check
 * \param len	Length of host label
 * \return true if LDH compliant, false otherwise
 */
static bool idna__is_ldh(const char *label, size_t len)
{
    const char *p = label;
    size_t i = 0;
    if (len == 0)
        return false;

    /* Check for leading or trailing hyphens */
    if ((p[0] == '-') || (p[len - 1] == '-'))
        return false;

    /* Check for non-alphanumeric, non-hyphen characters */
    for (i = 0; i < len; p++) {
        i++;
        if (*p == '-')
            continue;
        if ((*p >= '0') && (*p <= '9'))
            continue;
        if ((*p >= 'a') && (*p <= 'z'))
            continue;
        if ((*p >= 'A') && (*p <= 'Z'))
            continue;

        return false;
    }

    return true;
}


/**
 * Check if a host label appears to be ACE
 *
 * \param label	Host label to check
 * \param len	Length of host label
 * \return true if ACE compliant, false otherwise
 */
static bool idna__is_ace(const char *label, size_t len)
{
    /* Check it is a valid DNS string */
    if (idna__is_ldh(label, len) == false) {
        return false;
    }

    /* Check the ACE prefix is present */
    if ((label[0] == 'x') && (label[1] == 'n') && (label[2] == '-') && (label[3] == '-')) {
        return true;
    }

    return false;
}

/* This is the maximum length of a full DNS name */
#define FQDN_MAX 256

/* A "no action" action for FQDN_APPEND */
#define NO_ACTION (void)0

#define FQDN_APPEND(s, len, action)                                                                                    \
    do {                                                                                                               \
        if ((FQDN_MAX - fqdn_len) <= len) {                                                                            \
            /* Not enough room to append this element */                                                               \
            action;                                                                                                    \
            error = NSERROR_BAD_URL;                                                                                   \
            goto cleanup;                                                                                              \
        } else {                                                                                                       \
            memcpy(fqdn_p, s, len);                                                                                    \
            fqdn_p += len;                                                                                             \
            fqdn_len += len;                                                                                           \
            action;                                                                                                    \
        }                                                                                                              \
    } while (0)

/* exported interface documented in idna.h */
nserror idna_encode(const char *host, size_t len, char **ace_host, size_t *ace_len)
{
    nserror error = NSERROR_OK;
    int32_t *ucs4_host;
    size_t label_len, output_len, ucs4_len, fqdn_len = 0;
    char fqdn[FQDN_MAX];
    char *output, *fqdn_p = fqdn;

    label_len = idna__host_label_length(host, len);
    if (label_len == 0) {
        error = NSERROR_BAD_URL;
        goto cleanup;
    }

    while (label_len != 0) {
        if (idna__is_ldh(host, label_len) == false) {
            /* This string is IDN or invalid */

            /* Convert to Unicode */
            error = idna__utf8_to_ucs4(host, label_len, &ucs4_host, &ucs4_len);
            if (error != NSERROR_OK) {
                goto cleanup;
            }

            /* Check this is valid for conversion */
            if (idna__is_valid(ucs4_host, ucs4_len) == false) {
                free(ucs4_host);
                error = NSERROR_BAD_URL;
                goto cleanup;
            }

            /* Convert to ACE */
            error = idna__ucs4_to_ace(ucs4_host, ucs4_len, &output, &output_len);
            free(ucs4_host);
            if (error != NSERROR_OK) {
                goto cleanup;
            }
            FQDN_APPEND(output, output_len, free(output));
        } else {
            /* This is already a DNS-valid ASCII string */
            if (idna__is_ace(host, label_len) == true) {
                /* We must pass the exact substring length, as host is not null-terminated at the label boundary */
                char temp_label[FQDN_MAX];
                if (label_len >= FQDN_MAX) {
                    error = NSERROR_BAD_URL;
                    goto cleanup;
                }
                memcpy(temp_label, host, label_len);
                temp_label[label_len] = '\0';

                /* Attempt ACE to UCS-4 conversion to verify valid Punycode, mimicking what the user told us */
                int32_t *ucs4_buf;
                size_t ucs4_len;
                nserror err = idna__ace_to_ucs4(temp_label, label_len, &ucs4_buf, &ucs4_len);
                if (err != NSERROR_OK) {
                    error = NSERROR_UNKNOWN;
                    goto cleanup;
                }
                free(ucs4_buf);

                if (idna__verify(temp_label, label_len) == false) {
                    NSLOG(wisp, INFO, "Cannot verify ACE label %s", temp_label);
                    error = NSERROR_UNKNOWN;
                    goto cleanup;
                }
            }
            FQDN_APPEND(host, label_len, NO_ACTION);
        }

        FQDN_APPEND(".", 1, NO_ACTION);

        host += label_len;
        if ((*host == '\0') || (*host == ':')) {
            break;
        }
        host++;
        len = len - label_len - 1;

        label_len = idna__host_label_length(host, len);
    }

    if (fqdn_len > 0) {
        fqdn_p--;
        *fqdn_p = '\0';
    }

    if (error == NSERROR_OK) {
        *ace_host = strdup(fqdn);
        if (*ace_host == NULL) return NSERROR_NOMEM;
        *ace_len = fqdn_len > 0 ? fqdn_len - 1 : 0; /* last character is NULL */
    }

cleanup:
    return error;
}


/* exported interface documented in idna.h */
nserror idna_decode(const char *ace_host, size_t ace_len, char **host, size_t *host_len)
{
    nserror error = NSERROR_OK;
    int32_t *ucs4_host;
    size_t label_len, output_len, ucs4_len, fqdn_len = 0;
    char fqdn[FQDN_MAX];
    char *output, *fqdn_p = fqdn;

    label_len = idna__host_label_length(ace_host, ace_len);
    if (label_len == 0) {
        error = NSERROR_BAD_URL;
        goto cleanup;
    }

    while (label_len != 0) {
        if (idna__is_ace(ace_host, label_len) == true) {
            /* This string is DNS-valid and (probably) encoded */

            /* Decode to Unicode */
            error = idna__ace_to_ucs4(ace_host, label_len, &ucs4_host, &ucs4_len);
            if (error != NSERROR_OK) {
                /* Gracefully handle decoding failure for invalid ACE */
                FQDN_APPEND(ace_host, label_len, NO_ACTION);
                error = NSERROR_OK;
            } else {
                /* Convert to UTF-8 */
                error = idna__ucs4_to_utf8(ucs4_host, ucs4_len, &output, &output_len);
                free(ucs4_host);
                if (error != NSERROR_OK) {
                    goto cleanup;
                }
                FQDN_APPEND(output, output_len, free(output));
            }
        } else {
            /* Not ACE */
            FQDN_APPEND(ace_host, label_len, NO_ACTION);
        }

        FQDN_APPEND(".", 1, NO_ACTION);

        ace_host += label_len;
        if ((*ace_host == '\0') || (*ace_host == ':')) {
            break;
        }
        ace_host++;
        ace_len = ace_len - label_len - 1;

        label_len = idna__host_label_length(ace_host, ace_len);
    }

    if (fqdn_len > 0) {
        fqdn_p--;
        *fqdn_p = '\0';
    }

    if (error == NSERROR_OK) {
        *host = strdup(fqdn);
        if (*host == NULL) return NSERROR_NOMEM;
        *host_len = fqdn_len > 0 ? fqdn_len - 1 : 0; /* last character is NULL */
    }

cleanup:
    return error;
}
