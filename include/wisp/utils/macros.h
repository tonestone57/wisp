/*
 * Common macros for Wisp
 */

#ifndef WISP_UTILS_MACROS_H
#define WISP_UTILS_MACROS_H

#include <stddef.h>

#ifndef NOF_ELEMENTS
#define NOF_ELEMENTS(array) (sizeof(array) / sizeof(*(array)))
#endif

#ifndef N_ELEMENTS
#define N_ELEMENTS(array) NOF_ELEMENTS(array)
#endif

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

#if defined(__GNUC__) && (__GNUC__ < 3)
#define FLEX_ARRAY_LEN_DECL 0
#else
#define FLEX_ARRAY_LEN_DECL
#endif

/**
 * Calculate length of constant C string.
 *
 * \param  x a constant C string.
 * \return The length of C string without its terminator.
 */
#define SLEN(x) (sizeof((x)) - 1)

/**
 * switch fall through
 */
#if defined __cplusplus && defined __has_cpp_attribute
#if __has_cpp_attribute(fallthrough) && __cplusplus >= __has_cpp_attribute(fallthrough)
#define fallthrough [[fallthrough]]
#elif __has_cpp_attribute(gnu::fallthrough) && __STDC_VERSION__ >= __has_cpp_attribute(gnu::fallthrough)
#define fallthrough [[gnu::fallthrough]]
#elif __has_cpp_attribute(clang::fallthrough) && __STDC_VERSION__ >= __has_cpp_attribute(clang::fallthrough)
#define fallthrough [[clang::fallthrough]]
#endif
#elif defined __STDC_VERSION__ && defined __has_c_attribute
#if __has_c_attribute(fallthrough) && __STDC_VERSION__ >= __has_c_attribute(fallthrough)
#define fallthrough [[fallthrough]]
#endif
#endif
#if !defined fallthrough && defined __has_attribute
#if __has_attribute(__fallthrough__)
#define fallthrough __attribute__((__fallthrough__))
#endif
#endif
#if !defined fallthrough
/*  early gcc and clang have no implicit fallthrough warning */
#define fallthrough                                                                                                    \
    do {                                                                                                               \
    } while (0)
#endif

#endif
