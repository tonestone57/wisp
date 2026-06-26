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

/**
 * \file
 * BeOS implementation of generic bitmaps.
 *
 * This implements the interface given by image/bitmap.h using BBitmap.
 */

#define __STDBOOL_H__ 1
#include <sys/param.h>
#include <Bitmap.h>
#include <BitmapStream.h>
#include <File.h>
#include <GraphicsDefs.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>
#include <View.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include "utils/log.h"
#include "wisp/bitmap.h"
#include "wisp/browser_window.h"
#include "wisp/content.h"
#include "wisp/content_type.h"
#include "wisp/plotters.h"
}

#include "beos/bitmap.h"
#include "beos/gui.h"
#include "beos/plotters.h"
#include "beos/scaffolding.h"


struct bitmap {
    BBitmap *primary;
    BBitmap *shadow; // in Wisp's preferred ARGB/XRGB order (actually ABGR/XBGR in some parts)
    BBitmap *pretile_x;
    BBitmap *pretile_y;
    BBitmap *pretile_xy;
    bool opaque;
};

#define MIN_PRETILE_WIDTH 256
#define MIN_PRETILE_HEIGHT 256

/**
 * Convert Wisp's XBGR (alpha in high byte, blue in next, then green, red in low)
 * to BeOS BGRA32 (B in low byte, then G, R, A in high).
 * Wisp uses 0xAABBGGRR where AA=0 is opaque and AA=255 is transparent.
 * BeOS B_RGBA32 is actually BGRA in memory on little-endian.
 */
static inline void nsbeos_xbgr_to_bgra(void *src, void *dst, int width, int height, size_t rowstride)
{
    struct xbgr {
        uint8 r, g, b, a;
    };
    struct bgra {
        uint8 b, g, r, a;
    };
    struct xbgr *from = (struct xbgr *)src;
    struct bgra *to = (struct bgra *)dst;

    int stride_pixels = rowstride >> 2;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            to[x].b = from[x].b;
            to[x].g = from[x].g;
            to[x].r = from[x].r;
            /* Invert alpha: Wisp 0 is opaque, 255 is transparent.
               BeOS/AGG 255 is opaque, 0 is transparent. */
            to[x].a = 255 - from[x].a;
        }
        from += stride_pixels;
        to += stride_pixels;
    }
}


/**
 * Create a bitmap.
 *
 * \param  width   width of image in pixels
 * \param  height  height of image in pixels
 * \param  bflags  flags for bitmap creation
 * \return an opaque struct bitmap, or NULL on memory exhaustion
 */
static void *bitmap_create(int width, int height, enum gui_bitmap_flags flags)
{
    struct bitmap *bmp = (struct bitmap *)malloc(sizeof(struct bitmap));
    if (bmp == NULL)
        return NULL;

    int32 Bflags = 0;
    if (flags & BITMAP_CLEAR)
        Bflags |= B_BITMAP_CLEAR_TO_WHITE;

    BRect frame(0, 0, width - 1, height - 1);
    bmp->primary = new BBitmap(frame, Bflags, B_RGBA32);
    bmp->shadow = new BBitmap(frame, Bflags, B_RGBA32);

    if (bmp->primary->InitCheck() != B_OK || bmp->shadow->InitCheck() != B_OK) {
        delete bmp->primary;
        delete bmp->shadow;
        free(bmp);
        return NULL;
    }

    bmp->pretile_x = bmp->pretile_y = bmp->pretile_xy = NULL;
    bmp->opaque = (flags & BITMAP_OPAQUE) != 0;

    return bmp;
}


/**
 * Sets whether a bitmap should be plotted opaque
 *
 * \param  vbitmap  a bitmap, as returned by bitmap_create()
 * \param  opaque   whether the bitmap should be plotted opaque
 */
static void bitmap_set_opaque(void *vbitmap, bool opaque)
{
    struct bitmap *bitmap = (struct bitmap *)vbitmap;
    assert(bitmap);
    bitmap->opaque = opaque;
}


/**
 * Gets whether a bitmap should be plotted opaque
 *
 * \param  vbitmap  a bitmap, as returned by bitmap_create()
 */
static bool bitmap_get_opaque(void *vbitmap)
{
    struct bitmap *bitmap = (struct bitmap *)vbitmap;
    assert(bitmap);
    return bitmap->opaque;
}


/**
 * Return a pointer to the pixel data in a bitmap.
 *
 * \param  vbitmap  a bitmap, as returned by bitmap_create()
 * \return pointer to the pixel buffer
 */

static unsigned char *bitmap_get_buffer(void *vbitmap)
{
    struct bitmap *bitmap = (struct bitmap *)vbitmap;
    assert(bitmap);
    /* Wisp core writes to the shadow buffer */
    return (unsigned char *)(bitmap->shadow->Bits());
}


/**
 * Find the width of a pixel row in bytes.
 *
 * \param  vbitmap  a bitmap, as returned by bitmap_create()
 * \return width of a pixel row in the bitmap
 */
static size_t bitmap_get_rowstride(void *vbitmap)
{
    struct bitmap *bitmap = (struct bitmap *)vbitmap;
    assert(bitmap);
    return (bitmap->shadow->BytesPerRow());
}


/**
 * Free pretiles of a bitmap.
 *
 * \param bitmap The bitmap to free the pretiles of.
 */
static void nsbeos_bitmap_free_pretiles(struct bitmap *bitmap)
{
#define FREE_TILE(XY)                                                                                                  \
    if (bitmap->pretile_##XY)                                                                                          \
        delete (bitmap->pretile_##XY);                                                                                 \
    bitmap->pretile_##XY = NULL
    FREE_TILE(x);
    FREE_TILE(y);
    FREE_TILE(xy);
#undef FREE_TILE
}


/**
 * Free a bitmap.
 *
 * \param  vbitmap  a bitmap, as returned by bitmap_create()
 */
static void bitmap_destroy(void *vbitmap)
{
    struct bitmap *bitmap = (struct bitmap *)vbitmap;
    assert(bitmap);
    nsbeos_bitmap_free_pretiles(bitmap);
    delete bitmap->primary;
    delete bitmap->shadow;
    free(bitmap);
}


/**
 * The bitmap image has changed, so flush any persistant cache.
 *
 * \param  vbitmap  a bitmap, as returned by bitmap_create()
 */
void bitmap_modified(void *vbitmap)
{
    struct bitmap *bitmap = (struct bitmap *)vbitmap;

    /* Convert the shadow (Wisp XBGR) into the primary bitmap (BeOS BGRA) */
    bitmap->primary->LockBits();
    bitmap->shadow->LockBits();

    nsbeos_xbgr_to_bgra(bitmap->shadow->Bits(), bitmap->primary->Bits(),
        (int)bitmap->primary->Bounds().Width() + 1,
        (int)bitmap->primary->Bounds().Height() + 1,
        bitmap->primary->BytesPerRow());

    bitmap->shadow->UnlockBits();
    bitmap->primary->UnlockBits();

    nsbeos_bitmap_free_pretiles(bitmap);
}


static int bitmap_get_width(void *vbitmap)
{
    struct bitmap *bitmap = (struct bitmap *)vbitmap;
    return (int)bitmap->primary->Bounds().Width() + 1;
}


static int bitmap_get_height(void *vbitmap)
{
    struct bitmap *bitmap = (struct bitmap *)vbitmap;
    return (int)bitmap->primary->Bounds().Height() + 1;
}


static BBitmap *nsbeos_bitmap_generate_pretile(BBitmap *primary, int repeat_x, int repeat_y)
{
    int width = (int)primary->Bounds().Width() + 1;
    int height = (int)primary->Bounds().Height() + 1;
    size_t primary_stride = primary->BytesPerRow();
    BRect frame(0, 0, width * repeat_x - 1, height * repeat_y - 1);
    BBitmap *result = new BBitmap(frame, 0, B_RGBA32);

    if (result->InitCheck() != B_OK) {
        delete result;
        return NULL;
    }

    char *target_buffer = (char *)result->Bits();
    int x, y, row;

    if (repeat_x == 1 && repeat_y == 1) {
        delete result;
        return new BBitmap(primary);
    }

    for (y = 0; y < repeat_y; ++y) {
        char *primary_buffer = (char *)primary->Bits();
        for (row = 0; row < height; ++row) {
            for (x = 0; x < repeat_x; ++x) {
                memcpy(target_buffer, primary_buffer, primary_stride);
                target_buffer += primary_stride;
            }
            primary_buffer += primary_stride;
        }
    }
    return result;
}


/**
 * The primary image associated with this bitmap object.
 *
 * \param  bitmap  a bitmap, as returned by bitmap_create()
 */
BBitmap *nsbeos_bitmap_get_primary(struct bitmap *bitmap)
{
    return bitmap->primary;
}


/**
 * The X-pretiled image associated with this bitmap object.
 *
 * \param  bitmap  a bitmap, as returned by bitmap_create()
 */
BBitmap *nsbeos_bitmap_get_pretile_x(struct bitmap *bitmap)
{
    if (!bitmap->pretile_x) {
        int width = (int)bitmap->primary->Bounds().Width() + 1;
        int xmult = (MIN_PRETILE_WIDTH + width - 1) / width;
        bitmap->pretile_x = nsbeos_bitmap_generate_pretile(bitmap->primary, xmult, 1);
    }
    return bitmap->pretile_x;
}


/**
 * The Y-pretiled image associated with this bitmap object.
 *
 * \param  bitmap  a bitmap, as returned by bitmap_create()
 */
BBitmap *nsbeos_bitmap_get_pretile_y(struct bitmap *bitmap)
{
    if (!bitmap->pretile_y) {
        int height = (int)bitmap->primary->Bounds().Height() + 1;
        int ymult = (MIN_PRETILE_HEIGHT + height - 1) / height;
        bitmap->pretile_y = nsbeos_bitmap_generate_pretile(bitmap->primary, 1, ymult);
    }
    return bitmap->pretile_y;
}


/**
 * The XY-pretiled image associated with this bitmap object.
 *
 * \param  bitmap  a bitmap, as returned by bitmap_create()
 */
BBitmap *nsbeos_bitmap_get_pretile_xy(struct bitmap *bitmap)
{
    if (!bitmap->pretile_xy) {
        int width = (int)bitmap->primary->Bounds().Width() + 1;
        int height = (int)bitmap->primary->Bounds().Height() + 1;
        int xmult = (MIN_PRETILE_WIDTH + width - 1) / width;
        int ymult = (MIN_PRETILE_HEIGHT + height - 1) / height;
        bitmap->pretile_xy = nsbeos_bitmap_generate_pretile(bitmap->primary, xmult, ymult);
    }
    return bitmap->pretile_xy;
}


/**
 * Create a thumbnail of a page.
 *
 * \param  bitmap   the bitmap to draw to
 * \param  content  content structure to thumbnail
 * \return true on success and bitmap updated else false
 */
static nserror bitmap_render(struct bitmap *bitmap, hlcache_handle *content)
{
    BBitmap *thumbnail;
    BBitmap *small;
    BBitmap *big;
    BView *oldView;
    BView *view;
    BView *thumbView;
    float width;
    float height;
    int big_width;
    int big_height;

    struct redraw_context ctx;
    ctx.interactive = false;
    ctx.background_images = true;
    ctx.plot = &nsbeos_plotters;

    assert(content);
    assert(bitmap);

    thumbnail = nsbeos_bitmap_get_primary(bitmap);
    width = thumbnail->Bounds().Width();
    height = thumbnail->Bounds().Height();

    big_width = MIN(content_get_width(content), 1024);
    big_height = (int)(((big_width * height) + (width / 2)) / width);

    BRect contentRect(0, 0, big_width - 1, big_height - 1);
    big = new BBitmap(contentRect, B_BITMAP_ACCEPTS_VIEWS, B_RGBA32);

    if (big->InitCheck() < B_OK) {
        delete big;
        return NSERROR_NOMEM;
    }

    small = new BBitmap(thumbnail->Bounds(), B_BITMAP_ACCEPTS_VIEWS, B_RGBA32);

    if (small->InitCheck() < B_OK) {
        delete small;
        delete big;
        return NSERROR_NOMEM;
    }

    oldView = nsbeos_current_gc();

    view = new BView(contentRect, "thumbnailer", B_FOLLOW_NONE, B_WILL_DRAW);
    big->AddChild(view);

    thumbView = new BView(small->Bounds(), "thumbnail", B_FOLLOW_NONE, B_WILL_DRAW);
    small->AddChild(thumbView);

    view->LockLooper();
    nsbeos_current_gc_set(view);
    content_scaled_redraw(content, big_width, big_height, &ctx);
    view->Sync();
    view->UnlockLooper();

    nsbeos_current_gc_set(oldView);

    thumbView->LockLooper();
    /* Draw scaled with high quality (bilinear filtering via AGG) */
    thumbView->DrawBitmap(big, big->Bounds(), small->Bounds(), B_FILTER_BITMAP_BILINEAR);
    thumbView->Sync();
    thumbView->UnlockLooper();

    small->LockBits();
    thumbnail->LockBits();
    memcpy(thumbnail->Bits(), small->Bits(), thumbnail->BitsLength());
    thumbnail->UnlockBits();
    small->UnlockBits();

    /* Primary is now ready. Update shadow if needed, though usually render
       output is only used for display. */

    nsbeos_bitmap_free_pretiles(bitmap);

    small->RemoveChild(thumbView);
    delete thumbView;
    delete small;
    big->RemoveChild(view);
    delete view;
    delete big;

    return NSERROR_OK;
}


static struct gui_bitmap_table bitmap_table = {
    /*.create =*/bitmap_create,
    /*.destroy =*/bitmap_destroy,
    /*.set_opaque =*/bitmap_set_opaque,
    /*.get_opaque =*/bitmap_get_opaque,
    /*.get_buffer =*/bitmap_get_buffer,
    /*.get_rowstride =*/bitmap_get_rowstride,
    /*.get_width =*/bitmap_get_width,
    /*.get_height =*/bitmap_get_height,
    /*.modified =*/bitmap_modified,
    /*.render =*/bitmap_render,
};

struct gui_bitmap_table *beos_bitmap_table = &bitmap_table;
