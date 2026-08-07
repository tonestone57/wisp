#ifndef QJS_JIT_H
#define QJS_JIT_H

#include "quickjs.h"

#if defined(__x86_64__) && (defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__Haiku__))
#if defined(__SANITIZE_ADDRESS__) || defined(ADDRESS_SANITIZER) || defined(__SANITIZE_THREAD__) || defined(THREAD_SANITIZER)
/* Disable Copy-Patch JIT compilation under ASan/LSan/TSan to avoid frame pointer unwinding/stack walking crashes */
#ifndef CONFIG_JIT
#define CONFIG_JIT 0
#endif
#else
#ifndef CONFIG_JIT
#define CONFIG_JIT 1
#endif
#endif
#endif

#ifdef CONFIG_JIT

#include <stddef.h>
#include <stdint.h>

/* Forward declare JSFunctionBytecode */
struct JSFunctionBytecode;
typedef struct JSFunctionBytecode JSFunctionBytecode;

/* Safeguard 2: Compile-time assertion of JSValue memory representation layout */
_Static_assert(sizeof(JSValue) == 16, "JIT stencils require 16-byte JSValue alignment!");

/* JIT Compiler API */
void qjs_jit_compile(JSFunctionBytecode *b);
void qjs_jit_free(void *ptr, size_t size);

/* C Runtime Helper Declarations */
JSValue *js_jit_get_loc(JSContext *ctx, JSValue *var_buf, JSValue *sp, int idx);
JSValue *js_jit_put_loc(JSContext *ctx, JSValue *var_buf, JSValue *sp, int idx);
JSValue *js_jit_set_loc(JSContext *ctx, JSValue *var_buf, JSValue *sp, int idx);
JSValue *js_jit_push_const(JSContext *ctx, JSValue *cpool, JSValue *sp, int idx);
JSValue *js_jit_add(JSContext *ctx, JSValue *sp);
JSValue *js_jit_sub(JSContext *ctx, JSValue *sp);
JSValue *js_jit_mul(JSContext *ctx, JSValue *sp);
JSValue *js_jit_lt(JSContext *ctx, JSValue *sp);
JSValue *js_jit_neq(JSContext *ctx, JSValue *sp);
int js_jit_if_true(JSContext *ctx, JSValue **sp_ref);

#endif /* CONFIG_JIT */

#endif /* QJS_JIT_H */
