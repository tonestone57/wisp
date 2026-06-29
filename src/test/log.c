#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include "utils/log.h"

__attribute__((weak)) bool verbose_log = false;

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
