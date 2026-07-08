#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include "utils/log.h"
#include "desktop/gui_internal.h"

extern bool verbose_log;
extern struct wisp_table *guit;

nserror nslog_init(nslog_ensure_t *ensure, int *pargc, char **argv)
{
    return NSERROR_OK;
}

void nslog_log(enum nslog_level level, const char *file, const char *func, int ln, const char *format, ...)
{
    va_list ap;
    if (verbose_log) {
        fprintf(stderr, "%s:%i %s: ", file, ln, func);
        va_start(ap, format);
        vfprintf(stderr, format, ap);
        va_end(ap);
        fputc('\n', stderr);
    }
}
