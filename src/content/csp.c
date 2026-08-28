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
    bool is_strict_dynamic;
    char *nonce;
    struct csp_source *next;
} csp_source;

typedef struct csp_policy {
    csp_source *directives[CSP_DIRECTIVE_COUNT];
    bool require_trusted_types_for_script;
    bool has_trusted_types_directive;
    char **allowed_policies;
    int allowed_policies_count;
    struct csp_policy *next;
} csp_policy;

struct csp {
    csp_policy *policies;
    nsurl *base_url;
};

static const char *directive_names[] = {
    "default-src",
    "script-src",
    "script-src-elem",
    "script-src-attr",
    "img-src",
    "style-src",
    "style-src-elem",
    "style-src-attr",
    "font-src",
    "object-src",
    "frame-src",
    "connect-src",
    "media-src",
    "worker-src",
    "frame-ancestors"
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

static void free_policy(csp_policy *policy) {
    if (!policy) return;
    for (int i = 0; i < CSP_DIRECTIVE_COUNT; i++) {
        free_sources(policy->directives[i]);
    }
    if (policy->allowed_policies) {
        for (int i = 0; i < policy->allowed_policies_count; i++) {
            free(policy->allowed_policies[i]);
        }
        free(policy->allowed_policies);
    }
    free(policy);
}

void csp_destroy(struct csp *csp) {
    if (!csp) return;
    csp_policy *pol = csp->policies;
    while (pol) {
        csp_policy *next = pol->next;
        free_policy(pol);
        pol = next;
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
    } else if (strcasecmp(token, "'strict-dynamic'") == 0) {
        src->is_strict_dynamic = true;
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
    } else if (token[0] == '\'') {
        /* Unrecognized single-quoted keywords (e.g. 'report-sample', 'unsafe-hashes')
         * are CSP keywords, not host expressions. */
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
    if (!header_value || !csp_out) return NSERROR_BAD_PARAMETER;

    struct csp *csp = *csp_out;
    if (!csp) {
        csp = calloc(1, sizeof(struct csp));
        if (!csp) return NSERROR_NOMEM;
        csp->base_url = nsurl_ref(base_url);
        *csp_out = csp;
    }

    char *copy = strdup(header_value);
    if (!copy) {
        if (*csp_out == csp && !csp->policies) {
            csp_destroy(csp);
            *csp_out = NULL;
        }
        return NSERROR_NOMEM;
    }

    char *saveptr_pol;
    char *policy_str = strtok_r(copy, ",", &saveptr_pol);
    while (policy_str) {
        csp_policy *policy = calloc(1, sizeof(csp_policy));
        if (!policy) {
            free(copy);
            return NSERROR_NOMEM;
        }

        char *saveptr1, *saveptr2;
        char *directive_str = strtok_r(policy_str, ";", &saveptr1);
        while (directive_str) {
            while (*directive_str == ' ' || *directive_str == '\t' || *directive_str == '\r' || *directive_str == '\n') directive_str++;
            char *token = strtok_r(directive_str, " \t\r\n", &saveptr2);
            if (token) {
                csp_directive dir = CSP_DIRECTIVE_COUNT;
                for (int i = 0; i < CSP_DIRECTIVE_COUNT; i++) {
                    if (strcasecmp(token, directive_names[i]) == 0) {
                        dir = (csp_directive)i;
                        break;
                    }
                }

                if (dir != CSP_DIRECTIVE_COUNT) {
                    token = strtok_r(NULL, " \t\r\n", &saveptr2);
                    while (token) {
                        csp_source *src = parse_source(token);
                        if (src) {
                            src->next = policy->directives[dir];
                            policy->directives[dir] = src;
                        }
                        token = strtok_r(NULL, " \t\r\n", &saveptr2);
                    }
                } else {
                    if (strcasecmp(token, "require-trusted-types-for") == 0) {
                        token = strtok_r(NULL, " \t\r\n", &saveptr2);
                        while (token) {
                            if (strcasecmp(token, "'script'") == 0) {
                                policy->require_trusted_types_for_script = true;
                            }
                            token = strtok_r(NULL, " \t\r\n", &saveptr2);
                        }
                    } else if (strcasecmp(token, "trusted-types") == 0) {
                        policy->has_trusted_types_directive = true;
                        token = strtok_r(NULL, " \t\r\n", &saveptr2);
                        while (token) {
                            char **new_policies = realloc(policy->allowed_policies, (policy->allowed_policies_count + 1) * sizeof(char *));
                            if (new_policies) {
                                policy->allowed_policies = new_policies;
                                policy->allowed_policies[policy->allowed_policies_count] = strdup(token);
                                if (policy->allowed_policies[policy->allowed_policies_count]) {
                                    policy->allowed_policies_count++;
                                }
                            }
                            token = strtok_r(NULL, " \t\r\n", &saveptr2);
                        }
                    }
                }
            }
            directive_str = strtok_r(NULL, ";", &saveptr1);
        }

        bool empty = true;
        for (int i = 0; i < CSP_DIRECTIVE_COUNT; i++) {
            if (policy->directives[i] != NULL) {
                empty = false;
                break;
            }
        }
        if (policy->require_trusted_types_for_script || policy->has_trusted_types_directive) {
            empty = false;
        }

        if (!empty) {
            if (!csp->policies) {
                csp->policies = policy;
            } else {
                csp_policy *last = csp->policies;
                while (last->next) {
                    last = last->next;
                }
                last->next = policy;
            }
        } else {
            free_policy(policy);
        }

        policy_str = strtok_r(NULL, ",", &saveptr_pol);
    }

    free(copy);
    return NSERROR_OK;
}

static csp_source *get_directive_sources(const csp_policy *policy, csp_directive directive) {
    csp_source *src = policy->directives[directive];
    if (src) return src;

    if (directive == CSP_SCRIPT_SRC_ELEM || directive == CSP_SCRIPT_SRC_ATTR) {
        src = policy->directives[CSP_SCRIPT_SRC];
        if (src) return src;
    } else if (directive == CSP_STYLE_SRC_ELEM || directive == CSP_STYLE_SRC_ATTR) {
        src = policy->directives[CSP_STYLE_SRC];
        if (src) return src;
    } else if (directive == CSP_WORKER_SRC) {
        src = policy->directives[CSP_SCRIPT_SRC];
        if (src) return src;
    } else if (directive == CSP_SCRIPT_SRC) {
        src = policy->directives[CSP_SCRIPT_SRC_ELEM];
        if (src) return src;
    } else if (directive == CSP_STYLE_SRC) {
        src = policy->directives[CSP_STYLE_SRC_ELEM];
        if (src) return src;
    }

    if (directive != CSP_DEFAULT_SRC) {
        src = policy->directives[CSP_DEFAULT_SRC];
    }
    return src;
}

static bool match_source(csp_source *src, nsurl *base_url, nsurl *url) {
    if (src->is_none) return false;
    if (src->is_unsafe_inline || src->is_unsafe_eval) return false;
    if (src->is_strict_dynamic) return false;
    if (src->nonce != NULL) return false;
    if (src->is_self) {
        return nsurl_compare(base_url, url, NSURL_SCHEME | NSURL_HOST | NSURL_PORT);
    }

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
        if (strcmp(src->host, "*") == 0) {
            match = true;
        } else if (strncmp(src->host, "*.", 2) == 0) {
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
        lwc_string *scheme_lwc = nsurl_get_component(url, NSURL_SCHEME);
        if (scheme_lwc) {
            const char *s = lwc_string_data(scheme_lwc);
            if (strcasecmp(s, "x-ns-css") == 0 ||
                strcasecmp(s, "wisp-inline") == 0 ||
                strcasecmp(s, "resource") == 0 ||
                strcasecmp(s, "about") == 0) {
                lwc_string_unref(scheme_lwc);
                return true;
            }
            lwc_string_unref(scheme_lwc);
        }

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

    for (csp_policy *pol = csp->policies; pol != NULL; pol = pol->next) {
        csp_source *src = get_directive_sources(pol, directive);
        if (!src) {
            continue;
        }

        bool has_strict_dynamic = false;
        if (directive == CSP_SCRIPT_SRC || directive == CSP_SCRIPT_SRC_ELEM) {
            for (csp_source *s = src; s != NULL; s = s->next) {
                if (s->is_strict_dynamic) {
                    has_strict_dynamic = true;
                    break;
                }
            }
        }

        bool matched = false;
        if (has_strict_dynamic) {
            /* Under 'strict-dynamic', host-based allowlists, 'self', and schemes are suppressed.
             * Script loads must be authorized via valid element nonce (checked in csp_check_nonce)
             * or dynamic script creation context. Nonce presence in policy permits URL check. */
            for (csp_source *s = src; s != NULL; s = s->next) {
                if (s->nonce != NULL) {
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                NSLOG(wisp, INFO, "CSP STRICT-DYNAMIC BLOCKED UN-NONCED SCRIPT URL: %s", nsurl_access(url));
                return false;
            }
        } else {
            for (csp_source *s = src; s != NULL; s = s->next) {
                if (match_source(s, csp->base_url, url)) {
                    matched = true;
                    break;
                }
            }
        }

        if (!matched) {
            NSLOG(wisp, INFO, "CSP BLOCKED URL: %s for directive %s", nsurl_access(url), directive_names[directive]);
            return false;
        }
    }

    return true;
}

bool csp_check_inline(struct csp *csp, csp_directive directive) {
    if (!csp) return true;

    for (csp_policy *pol = csp->policies; pol != NULL; pol = pol->next) {
        csp_source *src = get_directive_sources(pol, directive);
        if (!src) {
            continue;
        }

        bool has_nonce = false;
        for (csp_source *curr = src; curr != NULL; curr = curr->next) {
            if (curr->nonce != NULL) {
                has_nonce = true;
                break;
            }
        }

        if (has_nonce) {
            return false;
        }

        bool allows_inline = false;
        for (csp_source *curr = src; curr != NULL; curr = curr->next) {
            if (curr->is_unsafe_inline) {
                allows_inline = true;
                break;
            }
            if (curr->is_none) {
                allows_inline = false;
                break;
            }
        }

        if (!allows_inline) {
            return false;
        }
    }

    return true;
}

bool csp_check_nonce(struct csp *csp, csp_directive directive, const char *nonce) {
    if (!csp) return true;
    if (!nonce) return false;

    for (csp_policy *pol = csp->policies; pol != NULL; pol = pol->next) {
        csp_source *src = get_directive_sources(pol, directive);
        if (!src) {
            continue;
        }

        bool matched = false;
        bool has_nonce_in_policy = false;
        for (csp_source *s = src; s != NULL; s = s->next) {
            if (s->nonce) {
                has_nonce_in_policy = true;
                if (wisp_simd_streq(s->nonce, nonce)) {
                    matched = true;
                    break;
                }
            }
        }

        if (matched) {
            continue;
        }

        if (has_nonce_in_policy) {
            return false;
        }

        bool allows_inline = false;
        for (csp_source *s = src; s != NULL; s = s->next) {
            if (s->is_unsafe_inline) {
                allows_inline = true;
                break;
            }
        }

        if (!allows_inline) {
            return false;
        }
    }

    return true;
}

bool csp_check_eval(struct csp *csp) {
    if (!csp) return true;

    for (csp_policy *pol = csp->policies; pol != NULL; pol = pol->next) {
        csp_source *src = get_directive_sources(pol, CSP_SCRIPT_SRC);
        if (!src) {
            continue;
        }

        bool allowed = false;
        for (csp_source *curr = src; curr != NULL; curr = curr->next) {
            if (curr->is_unsafe_eval) {
                allowed = true;
                break;
            }
            if (curr->is_none) {
                allowed = false;
                break;
            }
        }

        if (!allowed) {
            return false;
        }
    }

    return true;
}

bool csp_require_trusted_types_for_script(const struct csp *csp) {
    if (!csp) return false;

    for (csp_policy *pol = csp->policies; pol != NULL; pol = pol->next) {
        if (pol->require_trusted_types_for_script) {
            return true;
        }
    }

    return false;
}

bool csp_trusted_types_policy_allowed(const struct csp *csp, const char *policy_name) {
    if (!csp) return true;

    for (csp_policy *pol = csp->policies; pol != NULL; pol = pol->next) {
        if (!pol->has_trusted_types_directive) {
            continue;
        }

        bool allowed = false;
        for (int i = 0; i < pol->allowed_policies_count; i++) {
            if (wisp_simd_streq(pol->allowed_policies[i], "*") ||
                wisp_simd_streq(pol->allowed_policies[i], policy_name)) {
                allowed = true;
                break;
            }
        }

        if (!allowed) {
            return false;
        }
    }

    return true;
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

bool csp_check_frame_ancestor(struct csp *csp, nsurl *ancestor_url) {
    if (!csp || !ancestor_url) return true;
    for (csp_policy *pol = csp->policies; pol != NULL; pol = pol->next) {
        csp_source *src = pol->directives[CSP_FRAME_ANCESTORS];
        if (!src) continue;
        bool matched = false;
        for (csp_source *s = src; s != NULL; s = s->next) {
            if (match_source(s, csp->base_url, ancestor_url)) {
                matched = true;
                break;
            }
        }
        if (!matched) {
            NSLOG(wisp, INFO, "CSP FRAME-ANCESTORS BLOCKED EMBEDDING FROM %s", nsurl_access(ancestor_url));
            return false;
        }
    }
    return true;
}

bool wisp_security_is_origin_blocked(const char *origin) {
    if (!origin) return false;
    for (size_t i = 0; i < sizeof(blocked_origins) / sizeof(blocked_origins[0]); i++) {
        if (wisp_simd_streq(blocked_origins[i], origin)) {
            return true;
        }
    }
    return false;
}
