#include "wisp/utils/config.h"

#ifdef WISP_WINDOWS_USE_D2D

#include <windows.h>
#include <dwrite.h>
#include <vector>
#include <string>
#include <map>
#include <cctype>

extern "C" {
#include <libwapcaplet/libwapcaplet.h>
#include "wisp/layout.h"
#include "wisp/plot_style.h"
#include "wisp/utils/errors.h"
#include "wisp/utils/log.h"
#include "wisp/utils/utf8.h"
#include "windows/font.h"
#include "windows/window.h"
}

struct font_cache_key {
    std::wstring family;
    int weight;
    int style;
    float size;

    bool operator<(const font_cache_key& other) const {
        if (family != other.family) return family < other.family;
        if (weight != other.weight) return weight < other.weight;
        if (style != other.style) return style < other.style;
        return size < other.size;
    }
};

static std::map<font_cache_key, IDWriteTextFormat*> format_cache;

static IDWriteTextFormat* get_text_format(IDWriteFactory* factory, const plot_font_style_t* style) {
    std::wstring family = L"Segoe UI";

    if (style->families && style->families[0]) {
        const char *fam = lwc_string_data(style->families[0]);
        int wlen = MultiByteToWideChar(CP_UTF8, 0, fam, -1, NULL, 0);
        std::vector<WCHAR> wfamily(wlen);
        MultiByteToWideChar(CP_UTF8, 0, fam, -1, wfamily.data(), wlen);
        family = wfamily.data();
    } else {
        switch (style->family) {
            case PLOT_FONT_FAMILY_SERIF: family = L"Times New Roman"; break;
            case PLOT_FONT_FAMILY_MONOSPACE: family = L"Consolas"; break;
            case PLOT_FONT_FAMILY_CURSIVE: family = L"Segoe Script"; break;
            case PLOT_FONT_FAMILY_FANTASY: family = L"Impact"; break;
            default: family = L"Segoe UI"; break;
        }
    }

    font_cache_key key = { family, style->weight, (int)style->flags, plot_style_fixed_to_float(style->size) };
    auto it = format_cache.find(key);
    if (it != format_cache.end()) {
        return it->second;
    }

    IDWriteTextFormat* format = NULL;
    if (SUCCEEDED(factory->CreateTextFormat(
        family.c_str(), NULL,
        (DWRITE_FONT_WEIGHT)style->weight,
        (style->flags & FONTF_ITALIC) ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        plot_style_fixed_to_float(style->size),
        L"en-us", &format))) {
        format_cache[key] = format;
    }
    return format;
}

extern "C" IDWriteTextFormat* win32_dwrite_get_format(const plot_font_style_t* style) {
    if (!g_dwrite_factory) return NULL;
    return get_text_format(g_dwrite_factory, style);
}

static nserror win32_dwrite_width(const plot_font_style_t *style, const char *utf8str, size_t utf8len, int *width) {
    if (utf8len == 0) { *width = 0; return NSERROR_OK; }
    extern IDWriteFactory* g_dwrite_factory;
    if (!g_dwrite_factory) return NSERROR_INVALID;

    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8str, (int)utf8len, NULL, 0);
    std::vector<WCHAR> wstr(wlen + 1);
    MultiByteToWideChar(CP_UTF8, 0, utf8str, (int)utf8len, wstr.data(), wlen);

    IDWriteTextFormat* format = get_text_format(g_dwrite_factory, style);
    if (!format) return NSERROR_INVALID;

    IDWriteTextLayout* layout;
    if (SUCCEEDED(g_dwrite_factory->CreateTextLayout(wstr.data(), wlen, format, 10000.0f, 1000.0f, &layout))) {
        DWRITE_TEXT_METRICS metrics;
        layout->GetMetrics(&metrics);
        *width = (int)metrics.width;
        layout->Release();
    }
    return NSERROR_OK;
}

static nserror win32_dwrite_position(const plot_font_style_t *style, const char *utf8str, size_t utf8len, int x, size_t *char_offset, int *actual_x) {
    extern IDWriteFactory* g_dwrite_factory;
    if (!g_dwrite_factory || (x < 1)) {
        *char_offset = 0;
        *actual_x = 0;
        return NSERROR_OK;
    }

    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8str, (int)utf8len, NULL, 0);
    std::vector<WCHAR> wstr(wlen + 1);
    MultiByteToWideChar(CP_UTF8, 0, utf8str, (int)utf8len, wstr.data(), wlen);

    IDWriteTextFormat* format = get_text_format(g_dwrite_factory, style);
    if (!format) return NSERROR_INVALID;

    IDWriteTextLayout* layout;
    if (SUCCEEDED(g_dwrite_factory->CreateTextLayout(wstr.data(), wlen, format, 10000.0f, 1000.0f, &layout))) {
        BOOL is_trailing_hit;
        BOOL is_inside;
        DWRITE_HIT_TEST_METRICS htm;
        layout->HitTestPoint((float)x, 0, &is_trailing_hit, &is_inside, &htm);

        uint32_t pos = htm.textPosition;
        if (is_trailing_hit) pos++;

        *char_offset = utf8_bounded_byte_length(utf8str, utf8len, pos);
        *actual_x = (int)htm.left + (is_trailing_hit ? (int)htm.width : 0);
        layout->Release();
    }
    return NSERROR_OK;
}

static nserror win32_dwrite_split(const plot_font_style_t *style, const char *string, size_t length, int x, size_t *offset, int *actual_x) {
    nserror res = win32_dwrite_position(style, string, length, x, offset, actual_x);
    if (res != NSERROR_OK || *offset == length || *offset == 0) return res;

    size_t c_off = *offset;

    /* Look for the last space before or at the hit position */
    while ((*offset > 0) && !isspace((unsigned char)string[*offset])) {
        (*offset)--;
    }

    /* If no space found before the hit, search forward to the next space or end of string */
    if (*offset == 0 && !isspace((unsigned char)string[0])) {
        *offset = c_off;
        while ((*offset < length) && !isspace((unsigned char)string[*offset])) {
            (*offset)++;
        }
    }

    return win32_dwrite_width(style, string, *offset, actual_x);
}

extern "C" IDWriteFactory* g_dwrite_factory = NULL;

extern "C" void win32_dwrite_init(void) {
    if (!g_dwrite_factory) {
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown **)&g_dwrite_factory);
    }
}

extern "C" void win32_dwrite_fini(void) {
    for (auto const& [key, format] : format_cache) {
        format->Release();
    }
    format_cache.clear();
    if (g_dwrite_factory) {
        g_dwrite_factory->Release();
        g_dwrite_factory = NULL;
    }
}

static struct gui_layout_table layout_table_dwrite = {
    .width = win32_dwrite_width,
    .position = win32_dwrite_position,
    .split = win32_dwrite_split,
    .load_font_data = NULL,
    .free_font_data = NULL,
    .init = NULL,
    .finalise = NULL,
};

extern "C" struct gui_layout_table *win_layout_table_dwrite = &layout_table_dwrite;

#endif
