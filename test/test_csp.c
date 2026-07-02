#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "include/wisp/content/csp.h"
#include <wisp/utils/nsurl.h>

void test_csp() {
    nsurl *base_url, *url_self, *url_other, *url_cdn;
    struct csp *csp;

    assert(nsurl_create("https://example.com/page.html", &base_url) == NSERROR_OK);
    assert(nsurl_create("https://example.com/script.js", &url_self) == NSERROR_OK);
    assert(nsurl_create("https://malicious.com/evil.js", &url_other) == NSERROR_OK);
    assert(nsurl_create("https://cdn.example.com/lib.js", &url_cdn) == NSERROR_OK);

    // Test 1: default-src 'self'
    assert(csp_parse("default-src 'self'", base_url, &csp) == NSERROR_OK);
    assert(csp_check_url(csp, CSP_SCRIPT_SRC, url_self) == true);
    assert(csp_check_url(csp, CSP_SCRIPT_SRC, url_other) == false);
    assert(csp_check_inline(csp, CSP_SCRIPT_SRC) == false);
    csp_destroy(csp);

    // Test 2: script-src 'unsafe-inline'
    assert(csp_parse("script-src 'unsafe-inline'", base_url, &csp) == NSERROR_OK);
    assert(csp_check_inline(csp, CSP_SCRIPT_SRC) == true);
    assert(csp_check_inline(csp, CSP_STYLE_SRC) == true); // default-src is missing, so allowed
    csp_destroy(csp);

    // Test 3: Multiple directives
    assert(csp_parse("default-src 'none'; script-src https://cdn.example.com", base_url, &csp) == NSERROR_OK);
    assert(csp_check_url(csp, CSP_SCRIPT_SRC, url_cdn) == true);
    assert(csp_check_url(csp, CSP_SCRIPT_SRC, url_self) == false);
    assert(csp_check_url(csp, CSP_IMG_SRC, url_self) == false);
    csp_destroy(csp);

    nsurl_unref(base_url);
    nsurl_unref(url_self);
    nsurl_unref(url_other);
    nsurl_unref(url_cdn);

    printf("CSP tests passed!\n");
}

int main() {
    test_csp();
    return 0;
}
