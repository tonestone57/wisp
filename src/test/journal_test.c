#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <wisp/utils/nsurl.h>
#include <wisp/utils/log.h>
#include <wisp/content/backing_store.h>
#include "utils/corestrings.h"
#include <wisp/misc.h>
#include <wisp/desktop/gui_internal.h>
#include <wisp/utils/file.h>
extern struct wisp_table *guit;

/* Mock schedule functions */
nserror schedule(int t, void (*callback)(void *p), void *p) { return NSERROR_OK; }

struct gui_misc_table misc_table = {
    .schedule = schedule,
};

struct wisp_table guit_test = {
    .llcache = NULL, /* initialized in main */
    .misc = &misc_table,
    .file = NULL,    /* initialized in main */
};
int main(int argc, char **argv)
{
    guit = &guit_test;
    nserror ret;
    struct nsurl *url1, *url2;
    uint8_t *data1, *data2;
    size_t len1 = 100, len2 = 20000; /* len2 > 16KB to trigger mmap */
    uint8_t *fetched_data;
    size_t fetched_len;

    ret = corestrings_init();
    assert(ret == NSERROR_OK);

    guit->llcache = filesystem_llcache_table;
    guit->file = default_file_table;

    struct llcache_store_parameters params = {
        .path = "test_cache",
        .limit = 1024 * 1024,
        .hysteresis = 128 * 1024
    };

    wisp_recursive_rm("test_cache");
    mkdir("test_cache", 0755);

    ret = guit->llcache->initialise(&params);
    assert(ret == NSERROR_OK);

    nsurl_create("http://test1.com", &url1);
    nsurl_create("http://test2.com", &url2);

    data1 = malloc(len1);
    memset(data1, 'A', len1);
    data2 = malloc(len2);
    memset(data2, 'B', len2);

    /* Store assets */
    printf("Storing assets...\n");
    ret = guit->llcache->store(url1, BACKING_STORE_NONE, data1, len1);
    assert(ret == NSERROR_OK);
    ret = guit->llcache->store(url2, BACKING_STORE_NONE, data2, len2);
    assert(ret == NSERROR_OK);

    /* Fetch assets */
    printf("Fetching assets...\n");
    ret = guit->llcache->fetch(url1, BACKING_STORE_NONE, &fetched_data, &fetched_len);
    assert(ret == NSERROR_OK);
    assert(fetched_len == len1);
    if (memcmp(fetched_data, data1, len1) != 0) {
        printf("Fetch 1 failed: expected 'A's, got something else\n");
        for (int i=0; i<10; i++) printf("%02x ", fetched_data[i]);
        printf("\n");
        return 1;
    }
    guit->llcache->release(url1, BACKING_STORE_NONE);

    ret = guit->llcache->fetch(url2, BACKING_STORE_NONE, &fetched_data, &fetched_len);
    assert(ret == NSERROR_OK);
    assert(fetched_len == len2);
    assert(memcmp(fetched_data, data2, len2) == 0);
    guit->llcache->release(url2, BACKING_STORE_NONE);

    guit->llcache->finalise();

    /* Test Recovery */
    printf("Testing recovery...\n");
    ret = guit->llcache->initialise(&params);
    assert(ret == NSERROR_OK);

    ret = guit->llcache->fetch(url1, BACKING_STORE_NONE, &fetched_data, &fetched_len);
    assert(ret == NSERROR_OK);
    assert(fetched_len == len1);
    if (memcmp(fetched_data, data1, len1) != 0) {
        printf("Recovery Fetch 1 failed\n");
        return 1;
    }
    guit->llcache->release(url1, BACKING_STORE_NONE);

    ret = guit->llcache->fetch(url2, BACKING_STORE_NONE, &fetched_data, &fetched_len);
    assert(ret == NSERROR_OK);
    assert(fetched_len == len2);
    if (memcmp(fetched_data, data2, len2) != 0) {
        printf("Recovery Fetch 2 failed\n");
        return 1;
    }
    guit->llcache->release(url2, BACKING_STORE_NONE);

    guit->llcache->finalise();

    nsurl_unref(url1);
    nsurl_unref(url2);
    free(data1);
    free(data2);

    printf("Journal test passed!\n");
    return 0;
}
