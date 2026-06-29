#import "gui.h"
#include <wisp/bitmap.h>
#include <wisp/utils/log.h>

static void *macos_bitmap_create(int width, int height, unsigned int state) {
    struct gui_bitmap *bitmap = calloc(1, sizeof(struct gui_bitmap));
    if (!bitmap) return NULL;

    bitmap->rep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                          pixelsWide:width
                                                          pixelsHigh:height
                                                       bitsPerSample:8
                                                     samplesPerPixel:4
                                                            hasAlpha:YES
                                                            isPlanar:NO
                                                      colorSpaceName:NSDeviceRGBColorSpace
                                                         bytesPerRow:width * 4
                                                        bitsPerPixel:32];
    return bitmap;
}

static void macos_bitmap_destroy(void *bitmap) {
    struct gui_bitmap *bm = bitmap;
    bm->rep = nil;
    free(bm);
}

static unsigned char *macos_bitmap_get_buffer(void *bitmap) {
    struct gui_bitmap *bm = bitmap;
    return [bm->rep bitmapData];
}

static size_t macos_bitmap_get_rowstride(void *bitmap) {
    struct gui_bitmap *bm = bitmap;
    return [bm->rep bytesPerRow];
}

static int macos_bitmap_get_width(void *bitmap) {
    struct gui_bitmap *bm = bitmap;
    return (int)[bm->rep pixelsWide];
}

static int macos_bitmap_get_height(void *bitmap) {
    struct gui_bitmap *bm = bitmap;
    return (int)[bm->rep pixelsHigh];
}

static bool macos_bitmap_get_opaque(void *bitmap) {
    struct gui_bitmap *bm = bitmap;
    return bm->opaque;
}

static void macos_bitmap_set_opaque(void *bitmap, bool opaque) {
    struct gui_bitmap *bm = bitmap;
    bm->opaque = opaque;
}

static struct gui_bitmap_table bitmap_table = {
    .create = macos_bitmap_create,
    .destroy = macos_bitmap_destroy,
    .get_buffer = macos_bitmap_get_buffer,
    .get_rowstride = macos_bitmap_get_rowstride,
    .get_width = macos_bitmap_get_width,
    .get_height = macos_bitmap_get_height,
    .get_opaque = macos_bitmap_get_opaque,
    .set_opaque = macos_bitmap_set_opaque,
};

struct gui_bitmap_table *macos_bitmap_table = &bitmap_table;
