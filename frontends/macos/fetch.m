#import <Foundation/Foundation.h>
#include <wisp/fetch.h>
#include <wisp/utils/errors.h>
#include <wisp/utils/log.h>
#include <string.h>

static const char *macos_fetch_filetype(const char *unix_path)
{
    const char *ext = strrchr(unix_path, '.');
    if (ext == NULL) return "text/html";
    ext++;

    if (strcasecmp(ext, "css") == 0) return "text/css";
    if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0) return "image/jpeg";
    if (strcasecmp(ext, "png") == 0) return "image/png";
    if (strcasecmp(ext, "gif") == 0) return "image/gif";
    if (strcasecmp(ext, "svg") == 0) return "image/svg+xml";
    if (strcasecmp(ext, "webp") == 0) return "image/webp";

    return "text/html";
}

static NSMutableDictionary<NSString *, NSData *> *resourceCache = nil;
static NSObject *cacheLock = nil;

static nserror macos_get_resource_data(const char *path, const uint8_t **data_out, size_t *data_len_out)
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        cacheLock = [[NSObject alloc] init];
    });

    @autoreleasepool {
        NSString *nsPath = [NSString stringWithUTF8String:path];
        NSData *cachedData = nil;

        @synchronized(cacheLock) {
            if (!resourceCache) {
                resourceCache = [[NSMutableDictionary alloc] init];
            }
            cachedData = resourceCache[nsPath];
        }

        if (cachedData) {
            *data_out = [cachedData bytes];
            *data_len_out = [cachedData length];
            return NSERROR_OK;
        }

        NSString *resourcePath = [[NSBundle mainBundle] pathForResource:[nsPath stringByDeletingPathExtension]
                                                              ofType:[nsPath pathExtension]];

        if (!resourcePath) return NSERROR_NOT_FOUND;

        NSData *data = [NSData dataWithContentsOfFile:resourcePath];
        if (!data) return NSERROR_NOT_FOUND;

        @synchronized(cacheLock) {
            resourceCache[nsPath] = data;
        }

        *data_out = [data bytes];
        *data_len_out = [data length];
    }

    return NSERROR_OK;
}

void macos_fetch_cleanup(void) {
    @synchronized(cacheLock) {
        [resourceCache removeAllObjects];
        resourceCache = nil;
    }
}

static struct gui_fetch_table fetch_table = {
    .filetype = macos_fetch_filetype,
    .get_resource_data = macos_get_resource_data,
};

struct gui_fetch_table *macos_fetch_table = &fetch_table;
