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

    /* Test Open Failure Invalidation */
    printf("Testing open failure invalidation...\n");
    struct llcache_store_parameters no_journal_params = {
        .path = "test_cache_no_journal",
        .limit = 1024 * 1024,
        .hysteresis = 128 * 1024
    };
    wisp_recursive_rm("test_cache_no_journal");
    mkdir("test_cache_no_journal", 0755);
    /* Make journal a directory so journal_fd fails to open and store_write_file / store_read_file is used */
    mkdir("test_cache_no_journal/journal", 0755);

    ret = guit->llcache->initialise(&no_journal_params);
    assert(ret == NSERROR_OK);

    struct nsurl *url3;
    nsurl_create("http://test3.com", &url3);
    /* len3 > 64KB so it uses separate file storage rather than small block */
    size_t len3 = 70000;
    uint8_t *data3 = malloc(len3);
    memset(data3, 'C', len3);

    ret = guit->llcache->store(url3, BACKING_STORE_NONE, data3, len3);
    assert(ret == NSERROR_OK);

    guit->llcache->finalise();

    /* Delete the file backing store file manually while preserving entries index */
    /* Store file path for URL: test_cache_no_journal/d/... */
    /* Remove data files directory to force store_open failure */
    wisp_recursive_rm("test_cache_no_journal/d");

    ret = guit->llcache->initialise(&no_journal_params);
    assert(ret == NSERROR_OK);

    /* Fetch should fail due to missing file and invalidate the entry */
    ret = guit->llcache->fetch(url3, BACKING_STORE_NONE, &fetched_data, &fetched_len);
    assert(ret == NSERROR_NOT_FOUND);

    /* Second fetch should return NSERROR_NOT_FOUND because entry was invalidated */
    ret = guit->llcache->fetch(url3, BACKING_STORE_NONE, &fetched_data, &fetched_len);
    assert(ret == NSERROR_NOT_FOUND);

    guit->llcache->finalise();
    nsurl_unref(url3);
    free(data3);

    /* Test Finalisation with Outstanding (Unreleased) Allocations */
    printf("Testing finalisation with outstanding allocations...\n");
    wisp_recursive_rm("test_cache_unreleased");
    mkdir("test_cache_unreleased", 0755);
    struct llcache_store_parameters unreleased_params = {
        .path = "test_cache_unreleased",
        .limit = 1024 * 1024,
        .hysteresis = 128 * 1024
    };

    ret = guit->llcache->initialise(&unreleased_params);
    assert(ret == NSERROR_OK);

    struct nsurl *url_u1, *url_u2;
    nsurl_create("http://unreleased1.com", &url_u1);
    nsurl_create("http://unreleased2.com", &url_u2);

    size_t ulen1 = 500;
    size_t ulen2 = 25000;
    uint8_t *udata1 = malloc(ulen1);
    uint8_t *udata2 = malloc(ulen2);
    memset(udata1, 'X', ulen1);
    memset(udata2, 'Y', ulen2);

    ret = guit->llcache->store(url_u1, BACKING_STORE_NONE, udata1, ulen1);
    assert(ret == NSERROR_OK);
    ret = guit->llcache->store(url_u2, BACKING_STORE_NONE, udata2, ulen2);
    assert(ret == NSERROR_OK);

    /* Fetch assets into memory without calling release() before finalise() */
    ret = guit->llcache->fetch(url_u1, BACKING_STORE_NONE, &fetched_data, &fetched_len);
    assert(ret == NSERROR_OK);
    assert(fetched_len == ulen1);

    ret = guit->llcache->fetch(url_u2, BACKING_STORE_NONE, &fetched_data, &fetched_len);
    assert(ret == NSERROR_OK);
    assert(fetched_len == ulen2);

    /* Finalise backing store with active/outstanding allocations to ensure clean teardown */
    guit->llcache->finalise();

    nsurl_unref(url_u1);
    nsurl_unref(url_u2);
    free(udata1);
    free(udata2);

    wisp_recursive_rm("test_cache");
    wisp_recursive_rm("test_cache_no_journal");
    wisp_recursive_rm("test_cache_unreleased");

    /* Test Eviction Order Strategy */
    printf("Testing size-aware eviction order strategy...\n");
    wisp_recursive_rm("test_cache_evict");
    mkdir("test_cache_evict", 0755);

    /* Limit = 100KB, Hysteresis = 40KB */
    struct llcache_store_parameters evict_params = {
        .path = "test_cache_evict",
        .limit = 100 * 1024,
        .hysteresis = 40 * 1024
    };

    ret = guit->llcache->initialise(&evict_params);
    assert(ret == NSERROR_OK);

    struct nsurl *url_small, *url_large, *url_trigger;
    nsurl_create("http://evict-small.com", &url_small);
    nsurl_create("http://evict-large.com", &url_large);
    nsurl_create("http://evict-trigger.com", &url_trigger);

    size_t len_small = 10 * 1024;  /* 10KB */
    size_t len_large = 80 * 1024;  /* 80KB */
    size_t len_trigger = 20 * 1024;/* 20KB -> Push total above 100KB limit */

    uint8_t *data_small = malloc(len_small);
    uint8_t *data_large = malloc(len_large);
    uint8_t *data_trigger = malloc(len_trigger);
    memset(data_small, 'S', len_small);
    memset(data_large, 'L', len_large);
    memset(data_trigger, 'T', len_trigger);

    /* Store small (10KB) first, then large (80KB). Total = 90KB <= limit (100KB). */
    ret = guit->llcache->store(url_small, BACKING_STORE_NONE, data_small, len_small);
    assert(ret == NSERROR_OK);
    ret = guit->llcache->store(url_large, BACKING_STORE_NONE, data_large, len_large);
    assert(ret == NSERROR_OK);

    /*
     * Both url_small and url_large have use_count = 1 and no active RAM allocations.
     * url_small was stored first (older timestamp).
     * Under the new size-aware eviction policy, url_large (80KB) is prioritized for eviction
     * over url_small (10KB) because larger items free more space.
     *
     * Storing url_trigger (20KB) pushes total_alloc to 110KB > limit (100KB), triggering eviction.
     */
    ret = guit->llcache->store(url_trigger, BACKING_STORE_NONE, data_trigger, len_trigger);
    assert(ret == NSERROR_OK);

    /* Verify that url_large was evicted while url_small remains intact in the backing store */
    ret = guit->llcache->fetch(url_large, BACKING_STORE_NONE, &fetched_data, &fetched_len);
    assert(ret == NSERROR_NOT_FOUND);

    ret = guit->llcache->fetch(url_small, BACKING_STORE_NONE, &fetched_data, &fetched_len);
    assert(ret == NSERROR_OK);
    assert(fetched_len == len_small);
    guit->llcache->release(url_small, BACKING_STORE_NONE);

    guit->llcache->finalise();

    nsurl_unref(url_small);
    nsurl_unref(url_large);
    nsurl_unref(url_trigger);
    free(data_small);
    free(data_large);
    free(data_trigger);

    wisp_recursive_rm("test_cache_evict");

    printf("Journal test passed!\n");
    return 0;
}
