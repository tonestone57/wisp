/*
 * Math macros for Wisp
 */

#ifndef WISP_UTILS_MATH_H
#define WISP_UTILS_MATH_H

#ifndef ABS
#define ABS(x) (((x) > 0) ? (x) : (-(x)))
#endif

#ifdef __MINT__ /* avoid using GCCs builtin min/max functions */
#undef min
#undef max
#endif

#ifndef __cplusplus
#ifndef min
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef max
#define max(x, y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef clamp
#define clamp(x, low, high) (min(max((x), (low)), (high)))
#endif
#endif

#if defined(__HAIKU__) || defined(__BEOS__)
#include <stdlib.h>
#define strtof(s, p) ((float)(strtod((s), (p))))
#endif

#if !defined(ceilf) && defined(__MINT__)
#define ceilf(x) (float)ceil((double)x)
#endif

#endif
