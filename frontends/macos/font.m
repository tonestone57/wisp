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
        NSAttributedString *attrStr = [[NSAttributedString alloc] initWithString:nsStr attributes:attrs];
        CTLineRef line = CTLineCreateWithAttributedString((__bridge CFAttributedStringRef)attrStr);

        CFIndex index = CTLineGetStringIndexForPosition(line, CGPointMake(x, 0));
        CGFloat secondaryOffset;
        CGFloat offset = CTLineGetOffsetForStringIndex(line, index, &secondaryOffset);

        /* NetSurf expects byte offset, index is UTF-16 code unit offset */
        NSRange range = NSMakeRange(0, index);
        NSString *sub = [nsStr substringWithRange:range];
        *char_offset = [sub lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
        *actual_x = (int)offset;

        CFRelease(line);
    }
    return NSERROR_OK;
}

static nserror macos_font_split(const plot_font_style_t *fstyle, const char *string, size_t length, int x, size_t *char_offset, int *actual_x) {
    @autoreleasepool {
        NSString *nsStr = [[NSString alloc] initWithBytes:string length:length encoding:NSUTF8StringEncoding];
        if (!nsStr) {
            *char_offset = 0;
            *actual_x = 0;
            return NSERROR_OK;
        }

        NSFont *font = [NSFont systemFontOfSize:fstyle->size / 1000.0];
        NSDictionary *attrs = @{NSFontAttributeName: font};
        NSAttributedString *attrStr = [[NSAttributedString alloc] initWithString:nsStr attributes:attrs];
        CTTypesetterRef typesetter = CTTypesetterCreateWithAttributedString((__bridge CFAttributedStringRef)attrStr);

        CFIndex count = CTTypesetterSuggestLineBreak(typesetter, 0, x);
        CTLineRef line = CTLineCreateWithAttributedString((__bridge CFAttributedStringRef)[attrStr attributedSubstringFromRange:NSMakeRange(0, count)]);

        NSRange range = NSMakeRange(0, count);
        NSString *sub = [nsStr substringWithRange:range];
        *char_offset = [sub lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
        *actual_x = (int)CTLineGetTypographicBounds(line, NULL, NULL, NULL);

        CFRelease(line);
        CFRelease(typesetter);
    }
    return NSERROR_OK;
}

static struct gui_layout_table layout_table = {
    .width = macos_font_width,
    .position = macos_font_position,
    .split = macos_font_split,
    .load_font_data = NULL,
    .free_font_data = NULL,
    .init = NULL,
    .finalise = NULL,
};

struct gui_layout_table *macos_layout_table = &layout_table;
