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

    /* Test Size-based Eviction Sorting */
    printf("Testing size-based eviction sorting...\n");
    wisp_recursive_rm("test_cache_eviction");
    mkdir("test_cache_eviction", 0755);
    /* Small cache limit so adding items triggers store_evict */
    struct llcache_store_parameters evict_params = {
        .path = "test_cache_eviction",
        .limit = 1000,
        .hysteresis = 200
    };

    ret = guit->llcache->initialise(&evict_params);
    assert(ret == NSERROR_OK);

    struct nsurl *url_e1, *url_e2, *url_e3;
    nsurl_create("http://evict1.com", &url_e1);
    nsurl_create("http://evict2.com", &url_e2);
    nsurl_create("http://evict3.com", &url_e3);

    /* Store small item first (400 bytes) */
    size_t elen1 = 400;
    uint8_t *edata1 = malloc(elen1);
    memset(edata1, '1', elen1);
    ret = guit->llcache->store(url_e1, BACKING_STORE_NONE, edata1, elen1);
    assert(ret == NSERROR_OK);

    /* Store larger item second (500 bytes). Both url_e1 and url_e2 have same use_count (1) and last_used time.
     * Total allocated = 900 <= limit 1000.
     */
    size_t elen2 = 500;
    uint8_t *edata2 = malloc(elen2);
    memset(edata2, '2', elen2);
    ret = guit->llcache->store(url_e2, BACKING_STORE_NONE, edata2, elen2);
    assert(ret == NSERROR_OK);

    /* Store third item (300 bytes). Total allocation would become 1200 > limit (1000).
     * store_evict() will sort entries: url_e2 (size 500) sorted before url_e1 (size 400).
     * url_e2 gets evicted first.
     */
    size_t elen3 = 300;
    uint8_t *edata3 = malloc(elen3);
    memset(edata3, '3', elen3);
    ret = guit->llcache->store(url_e3, BACKING_STORE_NONE, edata3, elen3);
    assert(ret == NSERROR_OK);

    /* Verify url_e2 (larger item) was evicted while url_e1 (smaller item) remains in cache */
    ret = guit->llcache->fetch(url_e2, BACKING_STORE_NONE, &fetched_data, &fetched_len);
    assert(ret == NSERROR_NOT_FOUND);

    ret = guit->llcache->fetch(url_e1, BACKING_STORE_NONE, &fetched_data, &fetched_len);
    assert(ret == NSERROR_OK);
    assert(fetched_len == elen1);
    guit->llcache->release(url_e1, BACKING_STORE_NONE);

    guit->llcache->finalise();

    nsurl_unref(url_e1);
    nsurl_unref(url_e2);
    nsurl_unref(url_e3);
    free(edata1);
    free(edata2);
    free(edata3);

    wisp_recursive_rm("test_cache_eviction");

    printf("Journal test passed!\n");
    return 0;
}
