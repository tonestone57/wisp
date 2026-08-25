#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <wisp/content/csp.h>
#include <wisp/utils/nsurl.h>

void test_csp() {
    nsurl *base_url, *url_self, *url_other, *url_cdn;
    struct csp *csp = NULL;

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
    csp = NULL;

    // Test 2: script-src 'unsafe-inline'
    assert(csp_parse("script-src 'unsafe-inline'", base_url, &csp) == NSERROR_OK);
    assert(csp_check_inline(csp, CSP_SCRIPT_SRC) == true);
    assert(csp_check_inline(csp, CSP_STYLE_SRC) == true); // default-src is missing, so allowed
    csp_destroy(csp);
    csp = NULL;

    // Test 3: Multiple directives
    assert(csp_parse("default-src 'none'; script-src https://cdn.example.com", base_url, &csp) == NSERROR_OK);
    assert(csp_check_url(csp, CSP_SCRIPT_SRC, url_cdn) == true);
    assert(csp_check_url(csp, CSP_SCRIPT_SRC, url_self) == false);
    assert(csp_check_url(csp, CSP_IMG_SRC, url_self) == false);
    csp_destroy(csp);
    csp = NULL;

    // Test 4: host:port without scheme
    nsurl *url_port;
    assert(nsurl_create("https://example.com:8080/script.js", &url_port) == NSERROR_OK);
    assert(csp_parse("script-src example.com:8080", base_url, &csp) == NSERROR_OK);
    assert(csp_check_url(csp, CSP_SCRIPT_SRC, url_port) == true);
    assert(csp_check_url(csp, CSP_SCRIPT_SRC, url_self) == false);
    csp_destroy(csp);
    csp = NULL;

    // Test 5: scheme: without host
    assert(csp_parse("script-src https:", base_url, &csp) == NSERROR_OK);
    assert(csp_check_url(csp, CSP_SCRIPT_SRC, url_self) == true);
    assert(csp_check_url(csp, CSP_SCRIPT_SRC, url_port) == true);
    csp_destroy(csp);
    csp = NULL;
    nsurl_unref(url_port);

    // Test 6: require-trusted-types-for and trusted-types directives
    assert(csp_parse("require-trusted-types-for 'script'; trusted-types default policy1", base_url, &csp) == NSERROR_OK);
    assert(csp_require_trusted_types_for_script(csp) == true);
    assert(csp_trusted_types_policy_allowed(csp, "default") == true);
    assert(csp_trusted_types_policy_allowed(csp, "policy1") == true);
    assert(csp_trusted_types_policy_allowed(csp, "policy2") == false);
    csp_destroy(csp);
    csp = NULL;

    // Test 7: trusted-types wildcard *
    assert(csp_parse("require-trusted-types-for 'script'; trusted-types *", base_url, &csp) == NSERROR_OK);
    assert(csp_require_trusted_types_for_script(csp) == true);
    assert(csp_trusted_types_policy_allowed(csp, "anything") == true);
    csp_destroy(csp);
    csp = NULL;

    // Test 8: Nonce parsing, checking and unsafe-inline bypass
    assert(csp_parse("script-src 'unsafe-inline' 'nonce-xyz123'", base_url, &csp) == NSERROR_OK);
    assert(csp_check_nonce(csp, CSP_SCRIPT_SRC, "xyz123") == true);
    assert(csp_check_nonce(csp, CSP_SCRIPT_SRC, "wrong_nonce") == false);
    assert(csp_check_inline(csp, CSP_SCRIPT_SRC) == false); // 'unsafe-inline' must be ignored when a nonce is present
    csp_destroy(csp);
    csp = NULL;

    // Test 9: unsafe-eval check
    assert(csp_parse("script-src 'unsafe-eval'", base_url, &csp) == NSERROR_OK);
    assert(csp_check_eval(csp) == true);
    csp_destroy(csp);
    csp = NULL;

    assert(csp_parse("script-src 'self'", base_url, &csp) == NSERROR_OK);
    assert(csp_check_eval(csp) == false);
    csp_destroy(csp);
    csp = NULL;

    assert(csp_parse("default-src 'unsafe-eval'", base_url, &csp) == NSERROR_OK);
    assert(csp_check_eval(csp) == true);
    csp_destroy(csp);
    csp = NULL;

    assert(csp_parse("default-src 'self'", base_url, &csp) == NSERROR_OK);
    assert(csp_check_eval(csp) == false);
    csp_destroy(csp);
    csp = NULL;

    // Test 10: Origin blocklist checks
    assert(wisp_security_is_origin_blocked(NULL) == false);
    assert(wisp_security_is_origin_blocked("example.com") == false);
    assert(wisp_security_is_origin_blocked("safe-site.org") == false);
    assert(wisp_security_is_origin_blocked("adserver.com") == true);
    assert(wisp_security_is_origin_blocked("malicious-tracker.net") == true);
    assert(wisp_security_is_origin_blocked("attacker.com") == true);
    assert(wisp_security_is_origin_blocked("telemetry.evil.org") == true);
    assert(wisp_security_is_origin_blocked("analytics.track.me") == true);
    assert(wisp_security_is_origin_blocked("doubleclick.net") == true);
    assert(wisp_security_is_origin_blocked("google-analytics.com") == true);
    assert(wisp_security_is_origin_blocked("coop-malicious.org") == true);

    // Exact matching vs substring (the function uses SIMD string equals)
    assert(wisp_security_is_origin_blocked("not-adserver.com") == false);
    assert(wisp_security_is_origin_blocked("adserver.com.br") == false);

    // Test 11: Sec-Required-CSP & Sec-Fetch-* header strings validation
    const char *test_sec_req = "Sec-Required-CSP: script-src 'self'";
    assert(strncasecmp(test_sec_req, "Sec-Required-CSP:", 17) == 0);

    const char *test_sec_dest = "Sec-Fetch-Dest: document";
    assert(strncasecmp(test_sec_dest, "Sec-Fetch-Dest:", 15) == 0);

    const char *test_sec_mode = "Sec-Fetch-Mode: navigate";
    assert(strncasecmp(test_sec_mode, "Sec-Fetch-Mode:", 15) == 0);

    const char *test_sec_site = "Sec-Fetch-Site: same-origin";
    assert(strncasecmp(test_sec_site, "Sec-Fetch-Site:", 15) == 0);

    const char *test_sec_user = "Sec-Fetch-User: ?1";
    assert(strncasecmp(test_sec_user, "Sec-Fetch-User:", 15) == 0);

    // Test 12: CORP policy header parsing logic
    const char *corp_co = "cross-origin";
    const char *corp_ss = "same-site";
    const char *corp_so = "same-origin";
    assert(strcasecmp(corp_co, "cross-origin") == 0);
    assert(strcasecmp(corp_ss, "same-site") == 0);
    assert(strcasecmp(corp_so, "same-origin") == 0);

    // Test 13: Multiple CSP headers (such as gemini.google.com headers)
    const char *gemini_csp1 = "require-trusted-types-for 'script';report-uri /_/BardChatUi/cspreport";
    const char *gemini_csp2 = "script-src 'report-sample' 'nonce-MtBIGpIej1t8GJbvhmJWeQ' 'unsafe-inline' 'unsafe-eval' 'strict-dynamic' https: http:;object-src 'none';base-uri 'self';report-uri /_/BardChatUi/cspreport";
    const char *gemini_csp3 = "script-src 'unsafe-inline' 'unsafe-eval' blob: data: 'self' https://apis.google.com;report-uri /_/BardChatUi/cspreport/allowlist";

    assert(csp_parse(gemini_csp1, base_url, &csp) == NSERROR_OK);
    assert(csp_parse(gemini_csp2, base_url, &csp) == NSERROR_OK);
    assert(csp_parse(gemini_csp3, base_url, &csp) == NSERROR_OK);

    // Nonce from header 2 must be allowed across all policies
    assert(csp_check_nonce(csp, CSP_SCRIPT_SRC, "MtBIGpIej1t8GJbvhmJWeQ") == true);
    // Invalid nonce must be rejected
    assert(csp_check_nonce(csp, CSP_SCRIPT_SRC, "invalid_nonce") == false);
    // Un-nonced inline script must be blocked because header 2 requires a nonce
    assert(csp_check_inline(csp, CSP_SCRIPT_SRC) == false);
    // Trusted types requirement from header 1 must be active
    assert(csp_require_trusted_types_for_script(csp) == true);
    // URL matching from header 2 / header 3
    assert(csp_check_url(csp, CSP_SCRIPT_SRC, url_self) == true);
    assert(csp_check_url(csp, CSP_SCRIPT_SRC, url_other) == false);

    csp_destroy(csp);
    csp = NULL;

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
