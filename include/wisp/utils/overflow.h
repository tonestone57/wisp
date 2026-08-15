#ifndef WISP_UTILS_OVERFLOW_H
#define WISP_UTILS_OVERFLOW_H

#include <stdint.h>
#include <limits.h>

static inline int safe_add_int(int a, int b) {
    int64_t res = (int64_t)a + (int64_t)b;
    if (res > INT_MAX) return INT_MAX;
    if (res < INT_MIN) return INT_MIN;
    return (int)res;
}

static inline int safe_sub_int(int a, int b) {
    int64_t res = (int64_t)a - (int64_t)b;
    if (res > INT_MAX) return INT_MAX;
    if (res < INT_MIN) return INT_MIN;
    return (int)res;
}

static inline int safe_scale_sub_int(int a, int b, double scale) {
    int64_t diff = (int64_t)a - (int64_t)b;
    double res = diff * scale;
    if (res > INT_MAX) return INT_MAX;
    if (res < INT_MIN) return INT_MIN;
    return (int)res;
}

#endif /* WISP_UTILS_OVERFLOW_H */
