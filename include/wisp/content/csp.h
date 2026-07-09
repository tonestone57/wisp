/*
 * Content Security Policy (CSP) implementation
 */

#ifndef WISP_CONTENT_CSP_H_
#define WISP_CONTENT_CSP_H_

#include <stdbool.h>
#include <wisp/utils/nsurl.h>
#include <wisp/utils/errors.h>

/**
 * CSP Directives
 */
typedef enum {
    CSP_DEFAULT_SRC,
    CSP_SCRIPT_SRC,
    CSP_IMG_SRC,
    CSP_STYLE_SRC,
    CSP_FONT_SRC,
    CSP_OBJECT_SRC,
    CSP_FRAME_SRC,
    CSP_CONNECT_SRC,
    CSP_DIRECTIVE_COUNT
} csp_directive;

struct csp;

/**
 * Parse a CSP header value.
 *
 * \param header_value The CSP header value string.
 * \param base_url The base URL of the document for 'self' resolution.
 * \param csp_out Pointer to receive the new CSP object.
 * \return NSERROR_OK on success, appropriate error otherwise.
 */
nserror csp_parse(const char *header_value, nsurl *base_url, struct csp **csp_out);

/**
 * Check if a URL is allowed by the CSP for a specific directive.
 *
 * \param csp The CSP object.
 * \param directive The directive to check against.
 * \param url The URL to check.
 * \return true if allowed, false if blocked.
 */
bool csp_check_url(struct csp *csp, csp_directive directive, nsurl *url);

/**
 * Check if inline script or style is allowed.
 *
 * \param csp The CSP object.
 * \param directive The directive (CSP_SCRIPT_SRC or CSP_STYLE_SRC).
 * \return true if allowed, false if blocked.
 */
bool csp_check_inline(struct csp *csp, csp_directive directive);

/**
 * Destroy a CSP object and free its resources.
 */
void csp_destroy(struct csp *csp);

/**
 * Check if Trusted Types are required for script-based sinks.
 */
bool csp_require_trusted_types_for_script(const struct csp *csp);

/**
 * Check if a specific Trusted Type policy name is allowed.
 */
bool csp_trusted_types_policy_allowed(const struct csp *csp, const char *policy_name);

#endif
