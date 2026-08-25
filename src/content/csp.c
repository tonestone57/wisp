#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <wisp/utils/log.h>
#include <wisp/content/csp.h>
#include <wisp/utils/utf8proc_wrapper.h>

typedef struct csp_source {
    char *scheme;
    char *host;
    int port;
    bool is_self;
    bool is_none;
    bool is_unsafe_inline;
    bool is_unsafe_eval;
    char *nonce;
    struct csp_source *next;
} csp_source;

struct csp {
    csp_source *directives[CSP_DIRECTIVE_COUNT];
    nsurl *base_url;
    bool require_trusted_types_for_script;
    bool has_trusted_types_directive;
    char **allowed_policies;
    int allowed_policies_count;
};

static const char *directive_names[] = {
    "default-src",
    "script-src",
    "img-src",
    "style-src",
    "font-src",
    "object-src",
    "frame-src",
    "connect-src"
};

static void free_sources(csp_source *source) {
    while (source) {
        csp_source *next = source->next;
        free(source->scheme);
        free(source->host);
        free(source->nonce);
        free(source);
        source = next;
    }
}

void csp_destroy(struct csp *csp) {
    if (!csp) return;
    for (int i = 0; i < CSP_DIRECTIVE_COUNT; i++) {
        free_sources(csp->directives[i]);
    }
    if (csp->allowed_policies) {
        for (int i = 0; i < csp->allowed_policies_count; i++) {
            free(csp->allowed_policies[i]);
        }
        free(csp->allowed_policies);
    }
    if (csp->base_url) nsurl_unref(csp->base_url);
    free(csp);
}

static csp_source *parse_source(char *token) {
    csp_source *src = calloc(1, sizeof(csp_source));
    if (!src) return NULL;

    bool alloc_failed = false;

    if (strcasecmp(token, "'self'") == 0) {
        src->is_self = true;
    } else if (strcasecmp(token, "'none'") == 0) {
        src->is_none = true;
    } else if (strcasecmp(token, "'unsafe-inline'") == 0) {
        src->is_unsafe_inline = true;
    } else if (strcasecmp(token, "'unsafe-eval'") == 0) {
        src->is_unsafe_eval = true;
    } else if (strncasecmp(token, "'nonce-", 7) == 0) {
        size_t len = strlen(token);
        if (len >= 8 && token[len - 1] == '\'') {
            src->nonce = strndup(token + 7, len - 8);
        } else {
            src->nonce = strdup(token + 7);
        }
        if (!src->nonce) alloc_failed = true;
    } else if (strncasecmp(token, "nonce-", 6) == 0) {
        src->nonce = strdup(token + 6);
        if (!src->nonce) alloc_failed = true;
    } else if (strchr(token, ':')) {
        char *colon = strchr(token, ':');
        if (colon[1] != '\0' && colon[2] != '\0' && colon[1] == '/' && colon[2] == '/') {
            src->scheme = strndup(token, colon - token);
            if (!src->scheme) alloc_failed = true;
            char *host_start = colon + 3;
            char *port_start = strchr(host_start, ':');
            char *slash = strchr(host_start, '/');
            if (port_start && (!slash || port_start < slash)) {
                src->host = strndup(host_start, port_start - host_start);
                if (!src->host) alloc_failed = true;
                char *endptr;
                long port = strtol(port_start + 1, &endptr, 10);
                if (endptr != port_start + 1 && port >= 0 && port <= 65535) {
                    src->port = (int)port;
                }
            } else {
                if (slash) {
                    src->host = strndup(host_start, slash - host_start);
                } else {
                    src->host = strdup(host_start);
                }
                if (!src->host) alloc_failed = true;
            }
        } else {
            /* If colon is not followed by //, it might be host:port or scheme:
             * CSP sources like "https:" end with a colon.
             * A host:port source like "example.com:8080" has digits after colon.
             */
            if (colon[1] >= '0' && colon[1] <= '9') {
                src->host = strndup(token, colon - token);
                if (!src->host) alloc_failed = true;
                char *endptr;
                long port = strtol(colon + 1, &endptr, 10);
                if (endptr != colon + 1 && port >= 0 && port <= 65535) {
                    src->port = (int)port;
                }
            } else {
                src->scheme = strndup(token, colon - token);
                if (!src->scheme) alloc_failed = true;
            }
        }
    } else {
        char *slash = strchr(token, '/');
        if (slash) {
            src->host = strndup(token, slash - token);
        } else {
            src->host = strdup(token);
        }
        if (!src->host) alloc_failed = true;
    }

    if (alloc_failed) {
        free(src->scheme);
        free(src->host);
        free(src->nonce);
        free(src);
        return NULL;
    }

    return src;
}

nserror csp_parse(const char *header_value, nsurl *base_url, struct csp **csp_out) {
    if (!csp_out) return NSERROR_BAD_PARAMETER;

    struct csp *csp = *csp_out;
    bool new_csp = false;
    if (!csp) {
        csp = calloc(1, sizeof(struct csp));
        if (!csp) return NSERROR_NOMEM;
        csp->base_url = nsurl_ref(base_url);
        new_csp = true;
    }

    char *copy = strdup(header_value);
    if (!copy) {
        if (new_csp) {
            csp_destroy(csp);
        }
        return NSERROR_NOMEM;
    }

    char *saveptr1, *saveptr2;
    char *directive_str = strtok_r(copy, ";", &saveptr1);
    while (directive_str) {
        while (*directive_str == ' ') directive_str++;
        char *token = strtok_r(directive_str, " ", &saveptr2);
        if (token) {
            csp_directive dir = CSP_DIRECTIVE_COUNT;
            for (int i = 0; i < CSP_DIRECTIVE_COUNT; i++) {
                if (strcasecmp(token, directive_names[i]) == 0) {
                    dir = (csp_directive)i;
                    break;
                }
            }

            if (dir != CSP_DIRECTIVE_COUNT) {
                token = strtok_r(NULL, " ", &saveptr2);
                while (token) {
                    csp_source *src = parse_source(token);
                    if (src) {
                        src->next = csp->directives[dir];
                        csp->directives[dir] = src;
                    }
                    token = strtok_r(NULL, " ", &saveptr2);
                }
            } else {
                if (strcasecmp(token, "require-trusted-types-for") == 0) {
                    token = strtok_r(NULL, " ", &saveptr2);
                    while (token) {
                        if (strcasecmp(token, "'script'") == 0) {
                            csp->require_trusted_types_for_script = true;
                        }
                        token = strtok_r(NULL, " ", &saveptr2);
                    }
                } else if (strcasecmp(token, "trusted-types") == 0) {
                    csp->has_trusted_types_directive = true;
                    token = strtok_r(NULL, " ", &saveptr2);
                    while (token) {
                        char **new_policies = realloc(csp->allowed_policies, (csp->allowed_policies_count + 1) * sizeof(char *));
                        if (new_policies) {
                            csp->allowed_policies = new_policies;
                            csp->allowed_policies[csp->allowed_policies_count] = strdup(token);
                            if (csp->allowed_policies[csp->allowed_policies_count]) {
                                csp->allowed_policies_count++;
                            }
                        }
                        token = strtok_r(NULL, " ", &saveptr2);
                    }
                }
            }
        }
        directive_str = strtok_r(NULL, ";", &saveptr1);
    }

    free(copy);
    *csp_out = csp;
    return NSERROR_OK;
}

static bool match_source(csp_source *src, nsurl *base_url, nsurl *url) {
    if (src->is_none) return false;
    if (src->is_self) {
        return nsurl_compare(base_url, url, NSURL_SCHEME | NSURL_HOST | NSURL_PORT);
    }
    if (src->is_unsafe_inline) return false; // Not handled here

    if (src->scheme) {
        lwc_string *url_scheme = nsurl_get_component(url, NSURL_SCHEME);
        if (!url_scheme) return false;
        bool match = (strcasecmp(src->scheme, lwc_string_data(url_scheme)) == 0);
        lwc_string_unref(url_scheme);
        if (!match) return false;
    }

    if (src->host) {
        lwc_string *url_host_lwc = nsurl_get_component(url, NSURL_HOST);
        if (!url_host_lwc) return false;
        const char *url_host = lwc_string_data(url_host_lwc);
        bool match = false;
        if (strncmp(src->host, "*.", 2) == 0) {
            const char *suffix = src->host + 1; /* ".apple.com" */
            size_t suffix_len = strlen(suffix);
            size_t host_len = strlen(url_host);
            if (host_len >= suffix_len) {
                if (strcasecmp(url_host + (host_len - suffix_len), suffix) == 0) {
                    match = true;
                }
            }
            if (!match && strcasecmp(src->host + 2, url_host) == 0) {
                match = true;
            }
        } else {
            match = (strcasecmp(src->host, url_host) == 0);
        }
        lwc_string_unref(url_host_lwc);
        if (!match) return false;
    }

    if (src->port != 0) {
        lwc_string *url_port_str = nsurl_get_component(url, NSURL_PORT);
        int url_port = 0;
        if (url_port_str) {
            char *endptr;
            long port = strtol(lwc_string_data(url_port_str), &endptr, 10);
            if (endptr != lwc_string_data(url_port_str)) {
                url_port = (int)port;
            }
            lwc_string_unref(url_port_str);
        } else {
            lwc_string *url_scheme = nsurl_get_component(url, NSURL_SCHEME);
            if (url_scheme) {
                if (strcasecmp(lwc_string_data(url_scheme), "http") == 0) url_port = 80;
                else if (strcasecmp(lwc_string_data(url_scheme), "https") == 0) url_port = 443;
                lwc_string_unref(url_scheme);
            }
        }
        if (src->port != url_port) return false;
    }

    return true;
}

bool csp_check_url(struct csp *csp, csp_directive directive, nsurl *url) {
    if (url) {
        lwc_string *host_lwc = nsurl_get_component(url, NSURL_HOST);
        if (host_lwc) {
            if (wisp_security_is_origin_blocked(lwc_string_data(host_lwc))) {
                NSLOG(wisp, INFO, "SECURITY BLOCKED URL: %s (domain is blocklisted)", nsurl_access(url));
                lwc_string_unref(host_lwc);
                return false;
            }
            lwc_string_unref(host_lwc);
        }
    }

    if (!csp) return true;

    csp_source *src = csp->directives[directive];
    if (!src && directive != CSP_DEFAULT_SRC) {
        src = csp->directives[CSP_DEFAULT_SRC];
    }
    if (!src) return true;

    while (src) {
        if (match_source(src, csp->base_url, url)) return true;
        src = src->next;
    }

    NSLOG(wisp, INFO, "CSP BLOCKED URL: %s for directive %s", nsurl_access(url), directive_names[directive]);
    return false;
}

bool csp_check_inline(struct csp *csp, csp_directive directive) {
    if (!csp) return true;

    csp_source *src = csp->directives[directive];
    if (!src && directive != CSP_DEFAULT_SRC) {
        src = csp->directives[CSP_DEFAULT_SRC];
    }
    if (!src) return true;

    /* Check if a nonce is present in this source chain.
     * If a nonce is defined, 'unsafe-inline' is ignored/ignored-fallback per CSP spec.
     */
    csp_source *curr = src;
    while (curr) {
        if (curr->nonce != NULL) {
            return false;
        }
        curr = curr->next;
    }

    curr = src;
    while (curr) {
        if (curr->is_unsafe_inline) return true;
        if (curr->is_none) return false;
        curr = curr->next;
    }

    return false;
}

bool csp_check_nonce(struct csp *csp, csp_directive directive, const char *nonce) {
    if (!csp) return true;
    if (!nonce) return false;

    csp_source *src = csp->directives[directive];
    if (!src && directive != CSP_DEFAULT_SRC) {
        src = csp->directives[CSP_DEFAULT_SRC];
    }
    if (!src) return true;

    while (src) {
        if (src->nonce && wisp_simd_streq(src->nonce, nonce)) {
            return true;
        }
        src = src->next;
    }

    return false;
}

bool csp_check_eval(struct csp *csp) {
    if (!csp) return true;

    csp_source *src = csp->directives[CSP_SCRIPT_SRC];
    if (!src) {
        src = csp->directives[CSP_DEFAULT_SRC];
    }
    if (!src) return true;

    csp_source *curr = src;
    while (curr) {
        if (curr->is_unsafe_eval) return true;
        if (curr->is_none) return false;
        curr = curr->next;
    }

    return false;
}

bool csp_require_trusted_types_for_script(const struct csp *csp) {
    if (!csp) return false;
    return csp->require_trusted_types_for_script;
}

bool csp_trusted_types_policy_allowed(const struct csp *csp, const char *policy_name) {
    if (!csp) return true;
    if (!csp->has_trusted_types_directive) return true;
    for (int i = 0; i < csp->allowed_policies_count; i++) {
        if (wisp_simd_streq(csp->allowed_policies[i], "*") ||
            wisp_simd_streq(csp->allowed_policies[i], policy_name)) {
            return true;
        }
    }
    return false;
}

static const char *blocked_origins[] = {
    "adserver.com",
    "malicious-tracker.net",
    "attacker.com",
    "telemetry.evil.org",
    "analytics.track.me",
    "doubleclick.net",
    "google-analytics.com",
    "coop-malicious.org"
};

bool wisp_security_is_origin_blocked(const char *origin) {
    if (!origin) return false;
    for (size_t i = 0; i < sizeof(blocked_origins) / sizeof(blocked_origins[0]); i++) {
        if (wisp_simd_streq(blocked_origins[i], origin)) {
            return true;
        }
    }
    return false;
}
