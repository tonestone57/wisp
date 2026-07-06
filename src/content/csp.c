#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <wisp/utils/log.h>
#include <wisp/content/csp.h>

typedef struct csp_source {
    char *scheme;
    char *host;
    int port;
    bool is_self;
    bool is_none;
    bool is_unsafe_inline;
    struct csp_source *next;
} csp_source;

struct csp {
    csp_source *directives[CSP_DIRECTIVE_COUNT];
    nsurl *base_url;
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
        free(source);
        source = next;
    }
}

void csp_destroy(struct csp *csp) {
    if (!csp) return;
    for (int i = 0; i < CSP_DIRECTIVE_COUNT; i++) {
        free_sources(csp->directives[i]);
    }
    if (csp->base_url) nsurl_unref(csp->base_url);
    free(csp);
}

static csp_source *parse_source(char *token) {
    csp_source *src = calloc(1, sizeof(csp_source));
    if (!src) return NULL;

    if (strcasecmp(token, "'self'") == 0) {
        src->is_self = true;
    } else if (strcasecmp(token, "'none'") == 0) {
        src->is_none = true;
    } else if (strcasecmp(token, "'unsafe-inline'") == 0) {
        src->is_unsafe_inline = true;
    } else if (strchr(token, ':')) {
        char *colon = strchr(token, ':');
        if (colon[1] == '/' && colon[2] == '/') {
            src->scheme = strndup(token, colon - token);
            char *host_start = colon + 3;
            char *port_start = strchr(host_start, ':');
            char *slash = strchr(host_start, '/');
            if (port_start && (!slash || port_start < slash)) {
                src->host = strndup(host_start, port_start - host_start);
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
            }
        } else {
            /* If colon is not followed by //, it might be host:port or scheme:
             * CSP sources like "https:" end with a colon.
             * A host:port source like "example.com:8080" has digits after colon.
             */
            if (colon[1] >= '0' && colon[1] <= '9') {
                src->host = strndup(token, colon - token);
                char *endptr;
                long port = strtol(colon + 1, &endptr, 10);
                if (endptr != colon + 1 && port >= 0 && port <= 65535) {
                    src->port = (int)port;
                }
            } else {
                src->scheme = strndup(token, colon - token);
            }
        }
    } else {
        char *slash = strchr(token, '/');
        if (slash) {
            src->host = strndup(token, slash - token);
        } else {
            src->host = strdup(token);
        }
    }
    return src;
}

nserror csp_parse(const char *header_value, nsurl *base_url, struct csp **csp_out) {
    struct csp *csp = calloc(1, sizeof(struct csp));
    if (!csp) return NSERROR_NOMEM;

    csp->base_url = nsurl_ref(base_url);

    char *copy = strdup(header_value);
    if (!copy) {
        csp_destroy(csp);
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
        lwc_string *url_host = nsurl_get_component(url, NSURL_HOST);
        if (!url_host) return false;
        bool match = (strcasecmp(src->host, lwc_string_data(url_host)) == 0);
        lwc_string_unref(url_host);
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
        }
        if (src->port != url_port) return false;
    }

    return true;
}

bool csp_check_url(struct csp *csp, csp_directive directive, nsurl *url) {
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

    while (src) {
        if (src->is_unsafe_inline) return true;
        if (src->is_none) return false;
        src = src->next;
    }

    return false;
}
