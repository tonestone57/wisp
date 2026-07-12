/*
 * Copyright 2008 François Revol <mmu_man@users.sourceforge.net>
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

/** \file
 * Font handling (BeOS implementation).
 */


#define __STDBOOL_H__ 1
#include <Font.h>
#include <String.h>
#include <View.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

extern "C" {
#include "utils/log.h"
#include "utils/nsoption.h"
#include "utils/nsurl.h"
#include "utils/utils.h"
#include "wisp/ns_inttypes.h"
#include "wisp/layout.h"
}

#include "beos/font.h"
#include "beos/gui.h"
#include "beos/plotters.h"

#define FONT_CACHE_SIZE 32

struct font_cache_entry {
    char family[B_FONT_FAMILY_LENGTH + 1];
    uint16 face;
    float size;
    BFont font;
    uint32 last_use;
};

static struct font_cache_entry font_cache[FONT_CACHE_SIZE];
static uint32 font_cache_count = 0;
static uint32 font_usage_timer = 0;

/**
 * Convert a font style to a BFont.
 *
 * \param font Beos font object.
 * \param fstyle style for this text
 */
void nsbeos_style_to_font(BFont &font, const struct plot_font_style *fstyle)
{
    float size;
    uint16 face = 0;
    const char *family;

    switch (fstyle->family) {
    case PLOT_FONT_FAMILY_SERIF:
        family = nsoption_charp(font_serif);
        break;
    case PLOT_FONT_FAMILY_MONOSPACE:
        family = nsoption_charp(font_mono);
        break;
    case PLOT_FONT_FAMILY_CURSIVE:
        family = nsoption_charp(font_cursive);
        break;
    case PLOT_FONT_FAMILY_FANTASY:
        family = nsoption_charp(font_fantasy);
        break;
    case PLOT_FONT_FAMILY_SANS_SERIF:
    default:
        family = nsoption_charp(font_sans);
        break;
    }

    if ((fstyle->flags & FONTF_ITALIC)) {
        face = B_ITALIC_FACE;
    } else if ((fstyle->flags & FONTF_OBLIQUE)) {
        face = B_ITALIC_FACE;
    }

    if (fstyle->weight >= 600) {
        if (fstyle->weight >= 800)
            face |= B_HEAVY_FACE;
        else
            face |= B_BOLD_FACE;
    } else if (fstyle->weight <= 300) {
        face |= B_LIGHT_FACE;
    }

    if (!face)
        face = B_REGULAR_FACE;

    size = fstyle->size / PLOT_STYLE_SCALE;

    /* Check cache */
    font_usage_timer++;
    for (uint32 i = 0; i < font_cache_count; i++) {
        if (font_cache[i].face == face &&
            font_cache[i].size == size &&
            strcmp(font_cache[i].family, family ? family : "") == 0) {
            font = font_cache[i].font;
            font_cache[i].last_use = font_usage_timer;
            return;
        }
    }

    /* Not in cache, create new */
    if (family) {
        font_family beos_family;
        strncpy(beos_family, family, B_FONT_FAMILY_LENGTH);
        beos_family[B_FONT_FAMILY_LENGTH] = '\0';
        font.SetFamilyAndFace(beos_family, face);
    } else {
        font = be_plain_font;
        font.SetFace(face);
    }
    font.SetSize(size);

    /* Add to cache */
    uint32 index;
    if (font_cache_count < FONT_CACHE_SIZE) {
        index = font_cache_count++;
    } else {
        /* LRU replacement */
        index = 0;
        uint32 oldest = font_cache[0].last_use;
        for (uint32 i = 1; i < FONT_CACHE_SIZE; i++) {
            if (font_cache[i].last_use < oldest) {
                oldest = font_cache[i].last_use;
                index = i;
            }
        }
    }

    strncpy(font_cache[index].family, family ? family : "", B_FONT_FAMILY_LENGTH);
    font_cache[index].family[B_FONT_FAMILY_LENGTH] = '\0';
    font_cache[index].face = face;
    font_cache[index].size = size;
    font_cache[index].font = font;
    font_cache[index].last_use = font_usage_timer;
}


/**
 * Measure the width of a string.
 *
 * \param  fstyle  style for this text
 * \param  string  UTF-8 string to measure
 * \param  length  length of string
 * \param  width   updated to width of string[0..length)
 * \return  true on success, false on error and error reported
 */
static nserror beos_font_width(const plot_font_style_t *fstyle, const char *string, size_t length, int *width)
{
    BFont font;

    if (length == 0) {
        *width = 0;
        return NSERROR_OK;
    }

    nsbeos_style_to_font(font, fstyle);
    *width = (int)font.StringWidth(string, length);

    /* Add letter-spacing: extra pixels between each character */
    if (fstyle->letter_spacing != 0) {
        BString str(string, length);
        int32 nchars = str.CountChars();
        if (nchars > 1) {
            *width += fstyle->letter_spacing * (nchars - 1);
        }
    }

    return NSERROR_OK;
}


static int utf8_char_len(const char *c)
{
    uint8 *p = (uint8 *)c;
    uint8 m = 0xE0;
    uint8 v = 0xC0;
    int i;

    if (!*p)
        return 0;
    if ((*p & 0x80) == 0)
        return 1;
    if ((*p & 0xC0) == 0x80)
        return 1; // actually one of the remaining bytes...
    for (i = 2; i < 5; i++) {
        if ((*p & m) == v)
            return i;
        v = (v >> 1) | 0x80;
        m = (m >> 1) | 0x80;
    }
    return i;
}


/**
 * Find the position in a string where an x coordinate falls.
 *
 * \param  fstyle	style for this text
 * \param  string	UTF-8 string to measure
 * \param  length	length of string
 * \param  x		x coordinate to search for
 * \param  char_offset	updated to offset in string of actual_x, [0..length]
 * \param  actual_x	updated to x coordinate of character closest to x
 * \return  true on success, false on error and error reported
 */
static nserror beos_font_position(
    const plot_font_style_t *fstyle, const char *string, size_t length, int x, size_t *char_offset, int *actual_x)
{
    NSLOG(wisp, DEEPDEBUG, "(, '%s', %" PRIsizet ", %d, , )", string, length, x);

    int index;
    BFont font;

    nsbeos_style_to_font(font, fstyle);
    BString str(string, length);
    int32 len = str.CountChars();
    if (length == 0 || len <= 0) {
        *char_offset = 0;
        *actual_x = 0;
        return NSERROR_OK;
    }

    float escapements[len];
    float esc = 0.0;
    float current = 0.0;
    int i;

    index = 0;
    const char *safe_str = str.String();
    int32 safe_len = str.Length();
    font.GetEscapements(safe_str, len, escapements);
    // slow but it should work
    for (i = 0; index < safe_len && safe_str[index] && i < len; i++) {
        esc += escapements[i];
        current = font.Size() * esc + fstyle->letter_spacing * i;
        index += utf8_char_len(&safe_str[index]);
        // is current char already too far away?
        if (x < current)
            break;
    }
    *actual_x = (int)current;
    *char_offset = i; // index;

    return NSERROR_OK;
}


/**
 * Find where to split a string to make it fit a width.
 *
 * \param  fstyle       style for this text
 * \param  string       UTF-8 string to measure
 * \param  length       length of string, in bytes
 * \param  x            width available
 * \param  char_offset  updated to offset in string of actual_x, [1..length]
 * \param  actual_x     updated to x coordinate of character closest to x
 * \return  true on success, false on error and error reported
 *
 * On exit, char_offset indicates first character after split point.
 *
 * Note: char_offset of 0 should never be returned.
 *
 *   Returns:
 *     char_offset giving split point closest to x, where actual_x <= x
 *   else
 *     char_offset giving split point closest to x, where actual_x > x
 *
 * Returning char_offset == length means no split possible
 */
static nserror beos_font_split(
    const plot_font_style_t *fstyle, const char *string, size_t length, int x, size_t *char_offset, int *actual_x)
{
    NSLOG(wisp, DEEPDEBUG, "(, '%s', %" PRIsizet ", %d, , )", string, length, x);
    int index = 0;
    BFont font;

    nsbeos_style_to_font(font, fstyle);
    BString str(string, length);
    int32 len = str.CountChars();
    if (length == 0 || len <= 0) {
        *char_offset = 0;
        *actual_x = 0;
        return NSERROR_OK;
    }

    float escapements[len];
    float esc = 0.0;
    float current = 0.0;
    float last_x = 0.0;
    int i;
    int last_space = 0;

    const char *safe_str = str.String();
    int32 safe_len = str.Length();
    font.GetEscapements(safe_str, len, escapements);
    // very slow but it should work
    for (i = 0; index < safe_len && safe_str[index] && i < len; i++) {
        if (safe_str[index] == ' ') {
            last_x = current;
            last_space = index;
        }
        if (x < current && last_space != 0) {
            *actual_x = (int)last_x;
            *char_offset = last_space;
            return NSERROR_OK;
        }
        esc += escapements[i];
        current = font.Size() * esc + fstyle->letter_spacing * i;
        index += utf8_char_len(&safe_str[index]);
    }
    *actual_x = (int)current;
    *char_offset = index;

    return NSERROR_OK;
}


/**
 * Render a string.
 *
 * \param  fstyle  style for this text
 * \param  string  UTF-8 string to measure
 * \param  length  length of string
 * \param  x	   x coordinate
 * \param  y	   y coordinate
 * \return  true on success, false on error and error reported
 */

bool nsfont_paint(const plot_font_style_t *fstyle, const char *string, size_t length, int x, int y)
{
    BFont font;
    rgb_color oldbg;
    rgb_color background;
    rgb_color foreground;
    BView *view;

    if (length == 0)
        return true;

    nsbeos_style_to_font(font, fstyle);
    background = nsbeos_rgb_colour(fstyle->background);
    foreground = nsbeos_rgb_colour(fstyle->foreground);

    view = nsbeos_current_gc();
    if (view == NULL) {
        beos_warn_user("No GC", 0);
        return false;
    }

    oldbg = view->LowColor();
    drawing_mode oldmode = view->DrawingMode();
    view->SetLowColor(B_TRANSPARENT_32_BIT);

    view->SetFont(&font);
    view->SetHighColor(foreground);
    view->SetDrawingMode(B_OP_OVER);

    BString line(string, length);

    BPoint where(x, y + 1);
    view->DrawString(line.String(), where);

    view->SetDrawingMode(oldmode);
    if (memcmp(&oldbg, &background, sizeof(rgb_color)))
        view->SetLowColor(oldbg);

    return true;
}


static struct gui_layout_table layout_table = {
    /*.width = */ beos_font_width,
    /*.position = */ beos_font_position,
    /*.split = */ beos_font_split};

struct gui_layout_table *beos_layout_table = &layout_table;
