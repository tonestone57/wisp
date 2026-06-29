#import <Cocoa/Cocoa.h>
#import <CoreText/CoreText.h>
#include <wisp/layout.h>
#include <wisp/plot_style.h>

static nserror macos_font_width(const plot_font_style_t *fstyle, const char *string, size_t length, int *width) {
    @autoreleasepool {
        NSString *nsStr = [[NSString alloc] initWithBytes:string length:length encoding:NSUTF8StringEncoding];
        if (!nsStr) {
            *width = 0;
            return NSERROR_OK;
        }

        NSFont *font = [NSFont systemFontOfSize:fstyle->size / 1000.0];
        NSDictionary *attrs = @{NSFontAttributeName: font};
        NSSize size = [nsStr sizeWithAttributes:attrs];
        *width = (int)size.width;
    }
    return NSERROR_OK;
}

static nserror macos_font_position(const plot_font_style_t *fstyle, const char *string, size_t length, int x, size_t *char_offset, int *actual_x) {
    @autoreleasepool {
        NSString *nsStr = [[NSString alloc] initWithBytes:string length:length encoding:NSUTF8StringEncoding];
        if (!nsStr) {
            *char_offset = 0;
            *actual_x = 0;
            return NSERROR_OK;
        }

        NSFont *font = [NSFont systemFontOfSize:fstyle->size / 1000.0];
        NSDictionary *attrs = @{NSFontAttributeName: font};

        size_t best_offset = 0;
        int best_x = 0;

        /* Simple linear search for position. In a full implementation,
         * we'd use Core Text's CTRunGetPositions or similar.
         */
        for (size_t i = 0; i <= length; i++) {
            NSString *sub = [[NSString alloc] initWithBytes:string length:i encoding:NSUTF8StringEncoding];
            if (!sub) continue;
            NSSize size = [sub sizeWithAttributes:attrs];
            if (size.width > x) {
                break;
            }
            best_offset = i;
            best_x = (int)size.width;
        }

        *char_offset = best_offset;
        *actual_x = best_x;
    }
    return NSERROR_OK;
}

static nserror macos_font_split(const plot_font_style_t *fstyle, const char *string, size_t length, int x, size_t *char_offset, int *actual_x) {
    /* For now, split is similar to position but usually used for word wrapping */
    return macos_font_position(fstyle, string, length, x, char_offset, actual_x);
}

static struct gui_layout_table layout_table = {
    .width = macos_font_width,
    .position = macos_font_position,
    .split = macos_font_split,
};

struct gui_layout_table *macos_layout_table = &layout_table;
