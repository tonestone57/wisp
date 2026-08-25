#include "qjs_jit.h"

#ifdef CONFIG_JIT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

static inline void jit_write32(uint8_t *buf, int32_t val) {
    memcpy(buf, &val, sizeof(val));
}
static inline void jit_write_ptr(uint8_t *buf, void *ptr) {
    memcpy(buf, &ptr, sizeof(ptr));
}

typedef struct {
    uint8_t *buf;
    int idx;
    int capacity;
    bool failed;
} JITBuffer;

static inline void jit_ensure_capacity(JITBuffer *jb, int needed) {
    if (jb->failed) return;
    if (jb->idx + needed > jb->capacity) {
        int new_cap = jb->capacity;
        while (jb->idx + needed > new_cap) {
            new_cap *= 2;
        }
        uint8_t *new_buf = realloc(jb->buf, new_cap);
        if (!new_buf) {
            jb->failed = true;
            return;
        }
        jb->buf = new_buf;
        jb->capacity = new_cap;
    }
}

static inline void jit_emit8(JITBuffer *jb, uint8_t val) {
    jit_ensure_capacity(jb, 1);
    if (jb->failed) return;
    jb->buf[jb->idx++] = val;
}

static inline void jit_emit32(JITBuffer *jb, int32_t val) {
    jit_ensure_capacity(jb, sizeof(val));
    if (jb->failed) return;
    memcpy(jb->buf + jb->idx, &val, sizeof(val));
    jb->idx += sizeof(val);
}

static inline void jit_emit_ptr(JITBuffer *jb, void *ptr) {
    jit_ensure_capacity(jb, sizeof(ptr));
    if (jb->failed) return;
    memcpy(jb->buf + jb->idx, &ptr, sizeof(ptr));
    jb->idx += sizeof(ptr);
}

static inline void jit_emit_memcpy(JITBuffer *jb, const void *src, size_t size) {
    jit_ensure_capacity(jb, size);
    if (jb->failed) return;
    memcpy(jb->buf + jb->idx, src, size);
    jb->idx += size;
}

static inline int32_t pc_read32(const uint8_t *pc) {
    int32_t val;
    memcpy(&val, pc, sizeof(val));
    return val;
}
static inline uint16_t pc_read16(const uint8_t *pc) {
    uint16_t val;
    memcpy(&val, pc, sizeof(val));
    return val;
}

/* Compile-time static assertions verified */
_Static_assert(sizeof(JSValue) == 16, "JIT stencils require 16-byte JSValue alignment!");

/* C Runtime Helper Implementations */

JSValue *js_jit_get_loc(JSContext *ctx, JSValue *var_buf, JSValue *sp, int idx) {
    sp[0] = JS_DupValue(ctx, var_buf[idx]);
    return sp + 1;
}

JSValue *js_jit_put_loc(JSContext *ctx, JSValue *var_buf, JSValue *sp, int idx) {
    JSValue val = sp[-1];
    JSValue old_val = var_buf[idx];
    var_buf[idx] = val;
    JS_FreeValue(ctx, old_val);
    return sp - 1;
}

JSValue *js_jit_set_loc(JSContext *ctx, JSValue *var_buf, JSValue *sp, int idx) {
    JSValue val = sp[-1];
    JSValue old_val = var_buf[idx];
    var_buf[idx] = JS_DupValue(ctx, val);
    JS_FreeValue(ctx, old_val);
    return sp;
}

JSValue *js_jit_push_const(JSContext *ctx, JSValue *cpool, JSValue *sp, int idx) {
    sp[0] = JS_DupValue(ctx, cpool[idx]);
    return sp + 1;
}

JSValue *js_jit_add(JSContext *ctx, JSValue *sp) {
    JSValue op1 = sp[-2];
    JSValue op2 = sp[-1];
    if (likely(JS_VALUE_GET_TAG(op1) == JS_TAG_INT && JS_VALUE_GET_TAG(op2) == JS_TAG_INT)) {
        int64_t r = (int64_t)JS_VALUE_GET_INT(op1) + JS_VALUE_GET_INT(op2);
        if (unlikely(r < INT32_MIN || r > INT32_MAX)) {
            sp[-2] = __JS_NewFloat64((double)r);
        } else {
            sp[-2] = JS_NewInt32(ctx, (int32_t)r);
        }
        return sp - 1;
    } else if (JS_VALUE_GET_TAG(op1) == JS_TAG_FLOAT64 && JS_VALUE_GET_TAG(op2) == JS_TAG_FLOAT64) {
        sp[-2] = JS_NewFloat64(ctx, JS_VALUE_GET_FLOAT64(op1) + JS_VALUE_GET_FLOAT64(op2));
        return sp - 1;
    } else {
        if (js_add_slow(ctx, sp)) return NULL;
        return sp - 1;
    }
}

JSValue *js_jit_sub(JSContext *ctx, JSValue *sp) {
    JSValue op1 = sp[-2];
    JSValue op2 = sp[-1];
    if (likely(JS_VALUE_GET_TAG(op1) == JS_TAG_INT && JS_VALUE_GET_TAG(op2) == JS_TAG_INT)) {
        int64_t r = (int64_t)JS_VALUE_GET_INT(op1) - JS_VALUE_GET_INT(op2);
        if (unlikely(r < INT32_MIN || r > INT32_MAX)) {
            sp[-2] = __JS_NewFloat64((double)r);
        } else {
            sp[-2] = JS_NewInt32(ctx, (int32_t)r);
        }
        return sp - 1;
    } else if (JS_VALUE_GET_TAG(op1) == JS_TAG_FLOAT64 && JS_VALUE_GET_TAG(op2) == JS_TAG_FLOAT64) {
        sp[-2] = JS_NewFloat64(ctx, JS_VALUE_GET_FLOAT64(op1) - JS_VALUE_GET_FLOAT64(op2));
        return sp - 1;
    } else {
        if (js_binary_arith_slow(ctx, sp, OP_sub)) return NULL;
        return sp - 1;
    }
}

JSValue *js_jit_mul(JSContext *ctx, JSValue *sp) {
    JSValue op1 = sp[-2];
    JSValue op2 = sp[-1];
    if (likely(JS_VALUE_GET_TAG(op1) == JS_TAG_INT && JS_VALUE_GET_TAG(op2) == JS_TAG_INT)) {
        int32_t v1 = JS_VALUE_GET_INT(op1);
        int32_t v2 = JS_VALUE_GET_INT(op2);
        int64_t r = (int64_t)v1 * v2;
        if (unlikely(r < INT32_MIN || r > INT32_MAX)) {
            sp[-2] = __JS_NewFloat64((double)r);
            return sp - 1;
        }
        if (unlikely(r == 0 && (v1 | v2) < 0)) {
            sp[-2] = __JS_NewFloat64(-0.0);
            return sp - 1;
        }
        sp[-2] = JS_NewInt32(ctx, (int32_t)r);
        return sp - 1;
    } else if (JS_VALUE_GET_TAG(op1) == JS_TAG_FLOAT64 && JS_VALUE_GET_TAG(op2) == JS_TAG_FLOAT64) {
        sp[-2] = JS_NewFloat64(ctx, JS_VALUE_GET_FLOAT64(op1) * JS_VALUE_GET_FLOAT64(op2));
        return sp - 1;
    } else {
        if (js_binary_arith_slow(ctx, sp, OP_mul)) return NULL;
        return sp - 1;
    }
}

JSValue *js_jit_lt(JSContext *ctx, JSValue *sp) {
    JSValue op1 = sp[-2];
    JSValue op2 = sp[-1];
    if (likely(JS_VALUE_GET_TAG(op1) == JS_TAG_INT && JS_VALUE_GET_TAG(op2) == JS_TAG_INT)) {
        sp[-2] = JS_NewBool(ctx, JS_VALUE_GET_INT(op1) < JS_VALUE_GET_INT(op2));
        return sp - 1;
    } else {
        if (js_relational_slow(ctx, sp, OP_lt)) return NULL;
        return sp - 1;
    }
}

JSValue *js_jit_neq(JSContext *ctx, JSValue *sp) {
    JSValue op1 = sp[-2];
    JSValue op2 = sp[-1];
    if (likely(JS_VALUE_GET_TAG(op1) == JS_TAG_INT && JS_VALUE_GET_TAG(op2) == JS_TAG_INT)) {
        sp[-2] = JS_NewBool(ctx, JS_VALUE_GET_INT(op1) != JS_VALUE_GET_INT(op2));
        return sp - 1;
    } else {
        if (js_eq_slow(ctx, sp, 1)) return NULL;
        return sp - 1;
    }
}

int js_jit_if_true(JSContext *ctx, JSValue **sp_ref) {
    JSValue *sp = *sp_ref;
    JSValue val = *--sp;
    *sp_ref = sp;
    int res = JS_ToBool(ctx, val);
    JS_FreeValue(ctx, val);
    return res;
}

/* JIT Compiler Core Implementation */

typedef struct {
    int jit_patch_offset;
    int target_bytecode_pc;
} JITReloc;

#if defined(__x86_64__)

static void qjs_jit_compile_x86_64(JSFunctionBytecode *b) {
    JITBuffer jb;
    jb.capacity = 256;
    jb.buf = malloc(jb.capacity);
    jb.idx = 0;
    jb.failed = false;
    if (!jb.buf) return;

    int *bytecode_to_jit_offset = malloc(b->byte_code_len * sizeof(int));
    if (!bytecode_to_jit_offset) {
        free(jb.buf);
        return;
    }
    memset(bytecode_to_jit_offset, -1, b->byte_code_len * sizeof(int));

    JITReloc *relocs = malloc(b->byte_code_len * sizeof(JITReloc));
    int reloc_count = 0;

    /* Write Function Prologue
     * Preserves rbx, r12, r13, r14, r15 (callee-saved) across any C runtime helper calls.
     * Aligns stack on 16-byte boundary perfectly under the System V AMD64 ABI (sub rsp, 8).
     */
    uint8_t prologue[] = {
        0x55,                               /* push rbp */
        0x48, 0x89, 0xe5,                   /* mov rbp, rsp */
        0x53,                               /* push rbx */
        0x41, 0x54,                         /* push r12 */
        0x41, 0x55,                         /* push r13 */
        0x41, 0x56,                         /* push r14 */
        0x41, 0x57,                         /* push r15 */
        0x48, 0x83, 0xec, 0x08,             /* sub rsp, 8 (aligns stack pointer RSP to 16 bytes) */
        0x48, 0x89, 0x75, 0xd0,             /* mov [rbp - 48], rsi (save sp_ref) */
        0x48, 0x8b, 0x1e,                   /* mov rbx, [rsi] (load working stack pointer sp into rbx) */
        0x49, 0x89, 0xfc,                   /* mov r12, rdi (save ctx into r12) */
        0x49, 0x89, 0xd5,                   /* mov r13, rdx (save var_buf into r13) */
        0x49, 0x89, 0xce,                   /* mov r14, rcx (save cpool into r14) */
        0x4d, 0x89, 0xc7                    /* mov r15, r8 (save b into r15) */
    };
    jit_emit_memcpy(&jb, prologue, sizeof(prologue));

    int exception_exit_offset = -1;

    uint8_t *pc = b->byte_code_buf;
    uint8_t *pc_end = pc + b->byte_code_len;

    while (pc < pc_end) {
        int pc_offset = pc - b->byte_code_buf;
        bytecode_to_jit_offset[pc_offset] = jb.idx;

        int opcode = *pc++;

        if (opcode == OP_push_i32) {
            int32_t val = pc_read32(pc);
            pc += 4;
            /* mov dword ptr [rbx], val */
            jit_emit8(&jb, 0xc7);
            jit_emit8(&jb, 0x03);
            jit_emit32(&jb, val);
            /* mov qword ptr [rbx + 8], JS_TAG_INT (0) */
            uint8_t mov_tag[] = { 0x48, 0xc7, 0x43, 0x08, 0x00, 0x00, 0x00, 0x00 };
            jit_emit_memcpy(&jb, mov_tag, sizeof(mov_tag));
            /* add rbx, 16 */
            uint8_t add_rbx[] = { 0x48, 0x83, 0xc3, 0x10 };
            jit_emit_memcpy(&jb, add_rbx, sizeof(add_rbx));
        }
        else if (opcode == OP_get_loc || opcode == OP_get_loc8 || (opcode >= OP_get_loc0 && opcode <= OP_get_loc3)) {
            int idx = 0;
            if (opcode == OP_get_loc) {
                idx = pc_read16(pc);
                pc += 2;
            } else if (opcode == OP_get_loc8) {
                idx = *pc++;
            } else {
                idx = opcode - OP_get_loc0;
            }
            jit_emit8(&jb, 0x4c); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xe7);
            jit_emit8(&jb, 0x4c); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xee);
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xda);
            jit_emit8(&jb, 0xb9); jit_emit32(&jb, idx);
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0xb8);
            jit_emit_ptr(&jb, (void *)js_jit_get_loc);
            jit_emit8(&jb, 0xff); jit_emit8(&jb, 0xd0);
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xc3);
        }
        else if (opcode == OP_put_loc || opcode == OP_put_loc8 || (opcode >= OP_put_loc0 && opcode <= OP_put_loc3)) {
            int idx = 0;
            if (opcode == OP_put_loc) {
                idx = pc_read16(pc);
                pc += 2;
            } else if (opcode == OP_put_loc8) {
                idx = *pc++;
            } else {
                idx = opcode - OP_put_loc0;
            }
            jit_emit8(&jb, 0x4c); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xe7);
            jit_emit8(&jb, 0x4c); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xee);
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xda);
            jit_emit8(&jb, 0xb9); jit_emit32(&jb, idx);
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0xb8);
            jit_emit_ptr(&jb, (void *)js_jit_put_loc);
            jit_emit8(&jb, 0xff); jit_emit8(&jb, 0xd0);
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xc3);
        }
        else if (opcode == OP_set_loc || opcode == OP_set_loc8 || (opcode >= OP_set_loc0 && opcode <= OP_set_loc3)) {
            int idx = 0;
            if (opcode == OP_set_loc) {
                idx = pc_read16(pc);
                pc += 2;
            } else if (opcode == OP_set_loc8) {
                idx = *pc++;
            } else {
                idx = opcode - OP_set_loc0;
            }
            jit_emit8(&jb, 0x4c); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xe7);
            jit_emit8(&jb, 0x4c); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xee);
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xda);
            jit_emit8(&jb, 0xb9); jit_emit32(&jb, idx);
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0xb8);
            jit_emit_ptr(&jb, (void *)js_jit_set_loc);
            jit_emit8(&jb, 0xff); jit_emit8(&jb, 0xd0);
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xc3);
        }
        else if (opcode == OP_push_const8 || opcode == OP_push_const) {
            int idx = 0;
            if (opcode == OP_push_const8) {
                idx = *pc++;
            } else {
                idx = pc_read16(pc);
                pc += 2;
            }
            jit_emit8(&jb, 0x4c); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xe7);
            jit_emit8(&jb, 0x4c); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xf6);
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xda);
            jit_emit8(&jb, 0xb9); jit_emit32(&jb, idx);
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0xb8);
            jit_emit_ptr(&jb, (void *)js_jit_push_const);
            jit_emit8(&jb, 0xff); jit_emit8(&jb, 0xd0);
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xc3);
        }
        else if (opcode == OP_add || opcode == OP_sub || opcode == OP_mul || opcode == OP_lt || opcode == OP_neq) {
            void *func = NULL;
            if (opcode == OP_add) func = (void *)js_jit_add;
            else if (opcode == OP_sub) func = (void *)js_jit_sub;
            else if (opcode == OP_mul) func = (void *)js_jit_mul;
            else if (opcode == OP_lt) func = (void *)js_jit_lt;
            else func = (void *)js_jit_neq;

            jit_emit8(&jb, 0x4c); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xe7);
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xde);
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0xb8);
            jit_emit_ptr(&jb, func);
            jit_emit8(&jb, 0xff); jit_emit8(&jb, 0xd0);
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x85); jit_emit8(&jb, 0xc0);
            jit_emit8(&jb, 0x0f); jit_emit8(&jb, 0x84);
            relocs[reloc_count].jit_patch_offset = jb.idx;
            relocs[reloc_count].target_bytecode_pc = -999;
            reloc_count++;
            jit_emit32(&jb, 0);
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xc3);
        }
        else if (opcode == OP_if_true || opcode == OP_if_false) {
            int target_bytecode_pc = pc_offset + pc_read32(pc);
            pc += 4;

            jit_emit8(&jb, 0x4c); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xe7);
            uint8_t mov_rsi[] = { 0x48, 0x8b, 0x75, 0xd0 };
            jit_emit_memcpy(&jb, mov_rsi, sizeof(mov_rsi));
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0x1e);
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0xb8);
            jit_emit_ptr(&jb, (void *)js_jit_if_true);
            jit_emit8(&jb, 0xff); jit_emit8(&jb, 0xd0);
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x83); jit_emit8(&jb, 0xf8); jit_emit8(&jb, 0xff);
            jit_emit8(&jb, 0x0f); jit_emit8(&jb, 0x84);
            relocs[reloc_count].jit_patch_offset = jb.idx;
            relocs[reloc_count].target_bytecode_pc = -999;
            reloc_count++;
            jit_emit32(&jb, 0);
            jit_emit_memcpy(&jb, mov_rsi, sizeof(mov_rsi));
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x8b); jit_emit8(&jb, 0x1e);
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x85); jit_emit8(&jb, 0xc0);
            jit_emit8(&jb, 0x0f);
            jit_emit8(&jb, (opcode == OP_if_true) ? 0x85 : 0x84);
            relocs[reloc_count].jit_patch_offset = jb.idx;
            relocs[reloc_count].target_bytecode_pc = target_bytecode_pc;
            reloc_count++;
            jit_emit32(&jb, 0);
        }
        else if (opcode == OP_goto) {
            int target_bytecode_pc = pc_offset + pc_read32(pc);
            pc += 4;
            jit_emit8(&jb, 0xe9);
            relocs[reloc_count].jit_patch_offset = jb.idx;
            relocs[reloc_count].target_bytecode_pc = target_bytecode_pc;
            reloc_count++;
            jit_emit32(&jb, 0);
        }
        else if (opcode == OP_return) {
            uint8_t sub_rbx[] = { 0x48, 0x83, 0xeb, 0x10 };
            jit_emit_memcpy(&jb, sub_rbx, sizeof(sub_rbx));
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x8b); jit_emit8(&jb, 0x03);
            uint8_t mov_rdx[] = { 0x48, 0x8b, 0x53, 0x08 };
            jit_emit_memcpy(&jb, mov_rdx, sizeof(mov_rdx));
            uint8_t mov_rsi[] = { 0x48, 0x8b, 0x75, 0xd0 };
            jit_emit_memcpy(&jb, mov_rsi, sizeof(mov_rsi));
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0x1e);
            uint8_t ret_seq[] = {
                0x48, 0x83, 0xc4, 0x08,             /* add rsp, 8 */
                0x41, 0x5f,                         /* pop r15 */
                0x41, 0x5e,                         /* pop r14 */
                0x41, 0x5d,                         /* pop r13 */
                0x41, 0x5c,                         /* pop r12 */
                0x5b,                               /* pop rbx */
                0x5d,                               /* pop rbp */
                0xc3                                /* ret */
            };
            jit_emit_memcpy(&jb, ret_seq, sizeof(ret_seq));
        }
        else if (opcode == OP_return_undef) {
            uint8_t mov_rax[] = { 0x48, 0xc7, 0xc0, 0x00, 0x00, 0x00, 0x00 };
            jit_emit_memcpy(&jb, mov_rax, sizeof(mov_rax));
            uint8_t mov_rdx[] = { 0x48, 0xc7, 0xc2, 0x03, 0x00, 0x00, 0x00 };
            jit_emit_memcpy(&jb, mov_rdx, sizeof(mov_rdx));
            uint8_t mov_rsi[] = { 0x48, 0x8b, 0x75, 0xd0 };
            jit_emit_memcpy(&jb, mov_rsi, sizeof(mov_rsi));
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0x1e);
            uint8_t ret_seq[] = {
                0x48, 0x83, 0xc4, 0x08,             /* add rsp, 8 */
                0x41, 0x5f,                         /* pop r15 */
                0x41, 0x5e,                         /* pop r14 */
                0x41, 0x5d,                         /* pop r13 */
                0x41, 0x5c,                         /* pop r12 */
                0x5b,                               /* pop rbx */
                0x5d,                               /* pop rbp */
                0xc3                                /* ret */
            };
            jit_emit_memcpy(&jb, ret_seq, sizeof(ret_seq));
        }
        else {
            free(jb.buf);
            free(bytecode_to_jit_offset);
            free(relocs);
            return;
        }

        if (jb.failed) {
            free(jb.buf);
            free(bytecode_to_jit_offset);
            free(relocs);
            return;
        }
    }

    exception_exit_offset = jb.idx;
    uint8_t mov_rax[] = { 0x48, 0xc7, 0xc0, 0x00, 0x00, 0x00, 0x00 };
    jit_emit_memcpy(&jb, mov_rax, sizeof(mov_rax));
    uint8_t mov_rdx[] = { 0x48, 0xc7, 0xc2, 0x06, 0x00, 0x00, 0x00 };
    jit_emit_memcpy(&jb, mov_rdx, sizeof(mov_rdx));
    uint8_t mov_rsi[] = { 0x48, 0x8b, 0x75, 0xd0 };
    jit_emit_memcpy(&jb, mov_rsi, sizeof(mov_rsi));
    jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0x1e);
    uint8_t ret_seq[] = {
        0x48, 0x83, 0xc4, 0x08,             /* add rsp, 8 */
        0x41, 0x5f,                         /* pop r15 */
        0x41, 0x5e,                         /* pop r14 */
        0x41, 0x5d,                         /* pop r13 */
        0x41, 0x5c,                         /* pop r12 */
        0x5b,                               /* pop rbx */
        0x5d,                               /* pop rbp */
        0xc3                                /* ret */
    };
    jit_emit_memcpy(&jb, ret_seq, sizeof(ret_seq));

    if (jb.failed) {
        free(jb.buf);
        free(bytecode_to_jit_offset);
        free(relocs);
        return;
    }

    for (int i = 0; i < reloc_count; i++) {
        int patch_offset = relocs[i].jit_patch_offset;
        int target_pc = relocs[i].target_bytecode_pc;
        int target_jit_pos = -1;

        if (target_pc == -999) {
            target_jit_pos = exception_exit_offset;
        } else {
            target_jit_pos = bytecode_to_jit_offset[target_pc];
        }

        if (target_jit_pos != -1) {
            int rel_offset = target_jit_pos - (patch_offset + 4);
            jit_write32(jb.buf + patch_offset, rel_offset);
        }
    }

    void *jit_mem = mmap(NULL, jb.idx, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (jit_mem != MAP_FAILED) {
        memcpy(jit_mem, jb.buf, jb.idx);
        mprotect(jit_mem, jb.idx, PROT_READ | PROT_EXEC);
#if defined(__GNUC__) || defined(__clang__)
        __builtin___clear_cache((char *)jit_mem, (char *)jit_mem + jb.idx);
#endif
        b->jit_code = jit_mem;
        b->jit_size = jb.idx;
    }

    free(jb.buf);
    free(bytecode_to_jit_offset);
    free(relocs);
}

#elif defined(__aarch64__)

static void qjs_jit_compile_arm64(JSFunctionBytecode *b) {
    JITBuffer jb;
    jb.capacity = 256;
    jb.buf = malloc(jb.capacity);
    jb.idx = 0;
    jb.failed = false;
    if (!jb.buf) return;

    int *bytecode_to_jit_offset = malloc(b->byte_code_len * sizeof(int));
    if (!bytecode_to_jit_offset) {
        free(jb.buf);
        return;
    }
    memset(bytecode_to_jit_offset, -1, b->byte_code_len * sizeof(int));

    JITReloc *relocs = malloc(b->byte_code_len * sizeof(JITReloc));
    int reloc_count = 0;

    /* ARM64 Function Prologue (AAPCS64 Standard Register Allocation)
     * Register map:
     *   x0: JSContext *ctx -> x20
     *   x1: JSValue **sp_ref -> x23 (sp_ref saved on stack [sp, #48])
     *   x2: JSValue *var_buf -> x21
     *   x3: JSValue *cpool -> x22
     *   x4: JSFunctionBytecode *b -> x24
     *   working stack pointer sp -> x19 (loaded from [sp_ref])
     *
     * Stack Frame Layout (64 bytes aligned to 16 bytes):
     *   [sp, #0]:  x29, x30 (FP, LR)
     *   [sp, #16]: x19, x20
     *   [sp, #32]: x21, x22
     *   [sp, #48]: x23, x24
     */
    uint8_t prologue[] = {
        0xff, 0x03, 0x01, 0xd1,             /* sub sp, sp, #64 */
        0xfd, 0x7b, 0x00, 0xa9,             /* stp x29, x30, [sp, #0] */
        0xfd, 0x03, 0x00, 0x91,             /* mov x29, sp */
        0xf3, 0x53, 0x01, 0xa9,             /* stp x19, x20, [sp, #16] */
        0xf5, 0x5b, 0x02, 0xa9,             /* stp x21, x22, [sp, #32] */
        0xf7, 0x63, 0x03, 0xa9,             /* stp x23, x24, [sp, #48] */
        0xf4, 0x03, 0x00, 0xaa,             /* mov x20, x0 (save ctx) */
        0xf7, 0x03, 0x01, 0xaa,             /* mov x23, x1 (save sp_ref) */
        0xf5, 0x03, 0x02, 0xaa,             /* mov x21, x2 (save var_buf) */
        0xf6, 0x03, 0x03, 0xaa,             /* mov x22, x3 (save cpool) */
        0xf8, 0x03, 0x04, 0xaa,             /* mov x24, x4 (save b) */
        0xf3, 0x02, 0x40, 0xf9              /* ldr x19, [x23] (load working sp into x19) */
    };
    jit_emit_memcpy(&jb, prologue, sizeof(prologue));

    int exception_exit_offset = -1;

    uint8_t *pc = b->byte_code_buf;
    uint8_t *pc_end = pc + b->byte_code_len;

    while (pc < pc_end) {
        int pc_offset = pc - b->byte_code_buf;
        bytecode_to_jit_offset[pc_offset] = jb.idx;

        int opcode = *pc++;

        if (opcode == OP_push_i32) {
            int32_t val = pc_read32(pc);
            pc += 4;
            /* movz w0, val_low16 */
            uint16_t vlow = (uint16_t)(val & 0xffff);
            uint32_t inst_movz = 0x52800000 | (((uint32_t)vlow) << 5);
            jit_emit32(&jb, inst_movz);
            /* movk w0, val_high16, lsl #16 */
            uint16_t vhigh = (uint16_t)((val >> 16) & 0xffff);
            uint32_t inst_movk = 0x72a00000 | (((uint32_t)vhigh) << 5);
            jit_emit32(&jb, inst_movk);
            /* str w0, [x19] */
            jit_emit32(&jb, 0xb9000260);
            /* str xzr, [x19, #8] */
            jit_emit32(&jb, 0xf900067f);
            /* add x19, x19, #16 */
            jit_emit32(&jb, 0x91004273);
        }
        else if (opcode == OP_get_loc || opcode == OP_get_loc8 || (opcode >= OP_get_loc0 && opcode <= OP_get_loc3)) {
            int idx = 0;
            if (opcode == OP_get_loc) {
                idx = pc_read16(pc);
                pc += 2;
            } else if (opcode == OP_get_loc8) {
                idx = *pc++;
            } else {
                idx = opcode - OP_get_loc0;
            }
            /* mov x0, x20 (ctx) */
            jit_emit32(&jb, 0xaa1403e0);
            /* mov x1, x21 (var_buf) */
            jit_emit32(&jb, 0xaa1503e1);
            /* mov x2, x19 (sp) */
            jit_emit32(&jb, 0xaa1303e2);
            /* movz w3, idx */
            uint32_t inst_movz = 0x52800003 | (((uint32_t)(idx & 0xffff)) << 5);
            jit_emit32(&jb, inst_movz);
            /* ldr x4, .+8 */
            jit_emit32(&jb, 0x58000044);
            /* b .+12 */
            jit_emit32(&jb, 0x14000003);
            /* .quad helper_ptr */
            jit_emit_ptr(&jb, (void *)js_jit_get_loc);
            /* blr x4 */
            jit_emit32(&jb, 0xd63f0080);
            /* mov x19, x0 */
            jit_emit32(&jb, 0xaa0003f3);
        }
        else if (opcode == OP_put_loc || opcode == OP_put_loc8 || (opcode >= OP_put_loc0 && opcode <= OP_put_loc3)) {
            int idx = 0;
            if (opcode == OP_put_loc) {
                idx = pc_read16(pc);
                pc += 2;
            } else if (opcode == OP_put_loc8) {
                idx = *pc++;
            } else {
                idx = opcode - OP_put_loc0;
            }
            jit_emit32(&jb, 0xaa1403e0);
            jit_emit32(&jb, 0xaa1503e1);
            jit_emit32(&jb, 0xaa1303e2);
            uint32_t inst_movz = 0x52800003 | (((uint32_t)(idx & 0xffff)) << 5);
            jit_emit32(&jb, inst_movz);
            jit_emit32(&jb, 0x58000044);
            jit_emit32(&jb, 0x14000003);
            jit_emit_ptr(&jb, (void *)js_jit_put_loc);
            jit_emit32(&jb, 0xd63f0080);
            jit_emit32(&jb, 0xaa0003f3);
        }
        else if (opcode == OP_set_loc || opcode == OP_set_loc8 || (opcode >= OP_set_loc0 && opcode <= OP_set_loc3)) {
            int idx = 0;
            if (opcode == OP_set_loc) {
                idx = pc_read16(pc);
                pc += 2;
            } else if (opcode == OP_set_loc8) {
                idx = *pc++;
            } else {
                idx = opcode - OP_set_loc0;
            }
            jit_emit32(&jb, 0xaa1403e0);
            jit_emit32(&jb, 0xaa1503e1);
            jit_emit32(&jb, 0xaa1303e2);
            uint32_t inst_movz = 0x52800003 | (((uint32_t)(idx & 0xffff)) << 5);
            jit_emit32(&jb, inst_movz);
            jit_emit32(&jb, 0x58000044);
            jit_emit32(&jb, 0x14000003);
            jit_emit_ptr(&jb, (void *)js_jit_set_loc);
            jit_emit32(&jb, 0xd63f0080);
            jit_emit32(&jb, 0xaa0003f3);
        }
        else if (opcode == OP_push_const8 || opcode == OP_push_const) {
            int idx = 0;
            if (opcode == OP_push_const8) {
                idx = *pc++;
            } else {
                idx = pc_read16(pc);
                pc += 2;
            }
            /* mov x0, x20 (ctx) */
            jit_emit32(&jb, 0xaa1403e0);
            /* mov x1, x22 (cpool) */
            jit_emit32(&jb, 0xaa1603e1);
            /* mov x2, x19 (sp) */
            jit_emit32(&jb, 0xaa1303e2);
            /* movz w3, idx */
            uint32_t inst_movz = 0x52800003 | (((uint32_t)(idx & 0xffff)) << 5);
            jit_emit32(&jb, inst_movz);
            jit_emit32(&jb, 0x58000044);
            jit_emit32(&jb, 0x14000003);
            jit_emit_ptr(&jb, (void *)js_jit_push_const);
            jit_emit32(&jb, 0xd63f0080);
            jit_emit32(&jb, 0xaa0003f3);
        }
        else if (opcode == OP_add || opcode == OP_sub || opcode == OP_mul || opcode == OP_lt || opcode == OP_neq) {
            void *func = NULL;
            if (opcode == OP_add) func = (void *)js_jit_add;
            else if (opcode == OP_sub) func = (void *)js_jit_sub;
            else if (opcode == OP_mul) func = (void *)js_jit_mul;
            else if (opcode == OP_lt) func = (void *)js_jit_lt;
            else func = (void *)js_jit_neq;

            /* mov x0, x20 (ctx) */
            jit_emit32(&jb, 0xaa1403e0);
            /* mov x1, x19 (sp) */
            jit_emit32(&jb, 0xaa1303e1);
            jit_emit32(&jb, 0x58000044);
            jit_emit32(&jb, 0x14000003);
            jit_emit_ptr(&jb, func);
            jit_emit32(&jb, 0xd63f0080);
            /* cbz x0, exit_exception */
            relocs[reloc_count].jit_patch_offset = jb.idx;
            relocs[reloc_count].target_bytecode_pc = -999;
            reloc_count++;
            jit_emit32(&jb, 0xb4000000);
            /* mov x19, x0 */
            jit_emit32(&jb, 0xaa0003f3);
        }
        else if (opcode == OP_if_true || opcode == OP_if_false) {
            int target_bytecode_pc = pc_offset + pc_read32(pc);
            pc += 4;

            /* mov x0, x20 (ctx) */
            jit_emit32(&jb, 0xaa1403e0);
            /* ldr x1, [sp, #48] (sp_ref pointer saved in prologue) */
            jit_emit32(&jb, 0xf9401b81);
            /* str x19, [x1] (*sp_ref = x19) */
            jit_emit32(&jb, 0xf9000033);
            /* call js_jit_if_true */
            jit_emit32(&jb, 0x58000042);
            jit_emit32(&jb, 0x14000003);
            jit_emit_ptr(&jb, (void *)js_jit_if_true);
            jit_emit32(&jb, 0xd63f0040);
            /* cmn x0, #1 (check if res == -1) */
            jit_emit32(&jb, 0xb100041f);
            /* b.eq exit_exception */
            relocs[reloc_count].jit_patch_offset = jb.idx;
            relocs[reloc_count].target_bytecode_pc = -999;
            reloc_count++;
            jit_emit32(&jb, 0x54000000);
            /* reload working sp: ldr x1, [sp, #48]; ldr x19, [x1] */
            jit_emit32(&jb, 0xf9401b81);
            jit_emit32(&jb, 0xf9400033);
            /* cmp x0, xzr */
            jit_emit32(&jb, 0xeb1f001f);
            /* branch to target */
            relocs[reloc_count].jit_patch_offset = jb.idx;
            relocs[reloc_count].target_bytecode_pc = target_bytecode_pc;
            reloc_count++;
            jit_emit32(&jb, (opcode == OP_if_true) ? 0x54000001 : 0x54000000);
        }
        else if (opcode == OP_goto) {
            int target_bytecode_pc = pc_offset + pc_read32(pc);
            pc += 4;
            relocs[reloc_count].jit_patch_offset = jb.idx;
            relocs[reloc_count].target_bytecode_pc = target_bytecode_pc;
            reloc_count++;
            jit_emit32(&jb, 0x14000000);
        }
        else if (opcode == OP_return) {
            /* sub x19, x19, #16 */
            jit_emit32(&jb, 0xd1004273);
            /* ldr x0, [x19] */
            jit_emit32(&jb, 0xf9400260);
            /* ldr x1, [x19, #8] */
            jit_emit32(&jb, 0xf9400661);
            /* ldr x2, [sp, #48] (sp_ref) */
            jit_emit32(&jb, 0xf9401b82);
            /* str x19, [x2] (*sp_ref = x19) */
            jit_emit32(&jb, 0xf9000053);
            /* Epilogue sequence */
            uint8_t ret_seq[] = {
                0xf7, 0x63, 0x43, 0xa9,             /* ldp x23, x24, [sp, #48] */
                0xf5, 0x5b, 0x42, 0xa9,             /* ldp x21, x22, [sp, #32] */
                0xf3, 0x53, 0x41, 0xa9,             /* ldp x19, x20, [sp, #16] */
                0xfd, 0x7b, 0x40, 0xa9,             /* ldp x29, x30, [sp, #0] */
                0xff, 0x03, 0x01, 0x91,             /* add sp, sp, #64 */
                0xc0, 0x03, 0x5f, 0xd6              /* ret */
            };
            jit_emit_memcpy(&jb, ret_seq, sizeof(ret_seq));
        }
        else if (opcode == OP_return_undef) {
            /* mov x0, xzr */
            jit_emit32(&jb, 0xaa1f03e0);
            /* mov x1, #3 (JS_TAG_UNDEFINED) */
            jit_emit32(&jb, 0xd2800061);
            /* ldr x2, [sp, #48] */
            jit_emit32(&jb, 0xf9401b82);
            /* str x19, [x2] */
            jit_emit32(&jb, 0xf9000053);
            uint8_t ret_seq[] = {
                0xf7, 0x63, 0x43, 0xa9,
                0xf5, 0x5b, 0x42, 0xa9,
                0xf3, 0x53, 0x41, 0xa9,
                0xfd, 0x7b, 0x40, 0xa9,
                0xff, 0x03, 0x01, 0x91,
                0xc0, 0x03, 0x5f, 0xd6
            };
            jit_emit_memcpy(&jb, ret_seq, sizeof(ret_seq));
        }
        else {
            free(jb.buf);
            free(bytecode_to_jit_offset);
            free(relocs);
            return;
        }

        if (jb.failed) {
            free(jb.buf);
            free(bytecode_to_jit_offset);
            free(relocs);
            return;
        }
    }

    exception_exit_offset = jb.idx;
    /* mov x0, xzr */
    jit_emit32(&jb, 0xaa1f03e0);
    /* mov x1, #6 (JS_TAG_EXCEPTION) */
    jit_emit32(&jb, 0xd28000c1);
    /* ldr x2, [sp, #48] */
    jit_emit32(&jb, 0xf9401b82);
    /* str x19, [x2] */
    jit_emit32(&jb, 0xf9000053);
    uint8_t ret_seq[] = {
        0xf7, 0x63, 0x43, 0xa9,
        0xf5, 0x5b, 0x42, 0xa9,
        0xf3, 0x53, 0x41, 0xa9,
        0xfd, 0x7b, 0x40, 0xa9,
        0xff, 0x03, 0x01, 0x91,
        0xc0, 0x03, 0x5f, 0xd6
    };
    jit_emit_memcpy(&jb, ret_seq, sizeof(ret_seq));

    if (jb.failed) {
        free(jb.buf);
        free(bytecode_to_jit_offset);
        free(relocs);
        return;
    }

    for (int i = 0; i < reloc_count; i++) {
        int patch_offset = relocs[i].jit_patch_offset;
        int target_pc = relocs[i].target_bytecode_pc;
        int target_jit_pos = -1;

        if (target_pc == -999) {
            target_jit_pos = exception_exit_offset;
        } else {
            target_jit_pos = bytecode_to_jit_offset[target_pc];
        }

        if (target_jit_pos != -1) {
            int rel_offset = target_jit_pos - patch_offset;
            int32_t imm = rel_offset / 4;
            uint32_t *inst_ptr = (uint32_t *)(jb.buf + patch_offset);
            uint32_t inst = *inst_ptr;
            if ((inst & 0xfc000000) == 0x14000000) {
                /* B instruction: 26-bit immediate */
                inst = (inst & 0xfc000000) | (((uint32_t)imm) & 0x03ffffff);
            } else if ((inst & 0xff000000) == 0x54000000) {
                /* B.cond instruction: 19-bit immediate at bit 5 */
                inst = (inst & 0xff00001f) | ((((uint32_t)imm) & 0x7ffff) << 5);
            } else if ((inst & 0x7f000000) == 0x34000000) {
                /* CBZ/CBNZ instruction: 19-bit immediate at bit 5 */
                inst = (inst & 0xff00001f) | ((((uint32_t)imm) & 0x7ffff) << 5);
            }
            *inst_ptr = inst;
        }
    }

    void *jit_mem = mmap(NULL, jb.idx, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (jit_mem != MAP_FAILED) {
        memcpy(jit_mem, jb.buf, jb.idx);
        mprotect(jit_mem, jb.idx, PROT_READ | PROT_EXEC);
#if defined(__GNUC__) || defined(__clang__)
        __builtin___clear_cache((char *)jit_mem, (char *)jit_mem + jb.idx);
#endif
        b->jit_code = jit_mem;
        b->jit_size = jb.idx;
    }

    free(jb.buf);
    free(bytecode_to_jit_offset);
    free(relocs);
}

#elif defined(__riscv) && __riscv_xlen == 64

static void qjs_jit_compile_rv64(JSFunctionBytecode *b) {
    JITBuffer jb;
    jb.capacity = 256;
    jb.buf = malloc(jb.capacity);
    jb.idx = 0;
    jb.failed = false;
    if (!jb.buf) return;

    int *bytecode_to_jit_offset = malloc(b->byte_code_len * sizeof(int));
    if (!bytecode_to_jit_offset) {
        free(jb.buf);
        return;
    }
    memset(bytecode_to_jit_offset, -1, b->byte_code_len * sizeof(int));

    JITReloc *relocs = malloc(b->byte_code_len * sizeof(JITReloc));
    int reloc_count = 0;

    /* RISC-V 64 Function Prologue (Standard RISC-V ABI Register Allocation)
     * Register map:
     *   a0: JSContext *ctx -> s2 (x18)
     *   a1: JSValue **sp_ref -> s3 (x19)
     *   a2: JSValue *var_buf -> s4 (x20)
     *   a3: JSValue *cpool -> s5 (x21)
     *   a4: JSFunctionBytecode *b -> s6 (x22)
     *   working stack pointer sp -> s1 (x9)
     *
     * Stack Frame Layout (64 bytes aligned to 16 bytes):
     *   56(sp): ra
     *   48(sp): s0
     *   40(sp): s1
     *   32(sp): s2
     *   24(sp): s3
     *   16(sp): s4
     *    8(sp): s5
     *    0(sp): s6
     */
    uint8_t prologue[] = {
        0x13, 0x01, 0x01, 0xfc,             /* addi sp, sp, -64 */
        0x23, 0x3c, 0x11, 0x02,             /* sd ra, 56(sp) */
        0x23, 0x38, 0x81, 0x02,             /* sd s0, 48(sp) */
        0x23, 0x34, 0x91, 0x02,             /* sd s1, 40(sp) */
        0x23, 0x30, 0x21, 0x03,             /* sd s2, 32(sp) */
        0x23, 0x3c, 0x31, 0x01,             /* sd s3, 24(sp) */
        0x23, 0x38, 0x41, 0x01,             /* sd s4, 16(sp) */
        0x23, 0x34, 0x51, 0x01,             /* sd s5, 8(sp) */
        0x23, 0x30, 0x61, 0x01,             /* sd s6, 0(sp) */
        0x13, 0x04, 0x01, 0x04,             /* addi s0, sp, 64 */
        0x13, 0x09, 0x05, 0x00,             /* mv s2, a0 */
        0x93, 0x89, 0x05, 0x00,             /* mv s3, a1 */
        0x83, 0xb4, 0x05, 0x00,             /* ld s1, 0(a1) (load working sp into s1) */
        0x13, 0x8a, 0x06, 0x00,             /* mv s4, a2 */
        0x93, 0x0a, 0x07, 0x00,             /* mv s5, a3 */
        0x13, 0x0b, 0x08, 0x00              /* mv s6, a4 */
    };
    jit_emit_memcpy(&jb, prologue, sizeof(prologue));

    int exception_exit_offset = -1;

    uint8_t *pc = b->byte_code_buf;
    uint8_t *pc_end = pc + b->byte_code_len;

    while (pc < pc_end) {
        int pc_offset = pc - b->byte_code_buf;
        bytecode_to_jit_offset[pc_offset] = jb.idx;

        int opcode = *pc++;

        if (opcode == OP_push_i32) {
            int32_t val = pc_read32(pc);
            pc += 4;
            /* lui a0, val_hi20 */
            int32_t hi20 = (val + 0x800) >> 12;
            int32_t lo12 = val - (hi20 << 12);
            uint32_t inst_lui = 0x00000537 | (((uint32_t)hi20 & 0xfffff) << 12);
            jit_emit32(&jb, inst_lui);
            /* addiw a0, a0, val_lo12 */
            uint32_t inst_addiw = 0x0005051b | (((uint32_t)lo12 & 0xfff) << 20);
            jit_emit32(&jb, inst_addiw);
            /* sw a0, 0(s1) */
            jit_emit32(&jb, 0x00a4a023);
            /* sd zero, 8(s1) */
            jit_emit32(&jb, 0x0004b423);
            /* addi s1, s1, 16 */
            jit_emit32(&jb, 0x01048413);
        }
        else if (opcode == OP_get_loc || opcode == OP_get_loc8 || (opcode >= OP_get_loc0 && opcode <= OP_get_loc3)) {
            int idx = 0;
            if (opcode == OP_get_loc) {
                idx = pc_read16(pc);
                pc += 2;
            } else if (opcode == OP_get_loc8) {
                idx = *pc++;
            } else {
                idx = opcode - OP_get_loc0;
            }
            /* mv a0, s2 (ctx) */
            jit_emit32(&jb, 0x00090513);
            /* mv a1, s4 (var_buf) */
            jit_emit32(&jb, 0x000a0593);
            /* mv a2, s1 (sp) */
            jit_emit32(&jb, 0x00048613);
            /* li a3, idx */
            int32_t hi20 = (idx + 0x800) >> 12;
            int32_t lo12 = idx - (hi20 << 12);
            jit_emit32(&jb, 0x000006b7 | (((uint32_t)hi20 & 0xfffff) << 12));
            jit_emit32(&jb, 0x00068693 | (((uint32_t)lo12 & 0xfff) << 20));
            /* auipc t0, 0; ld t0, 16(t0); jalr ra, t0, 0; jal zero, 12; .quad ptr */
            jit_emit32(&jb, 0x00000297);
            jit_emit32(&jb, 0x0102b283);
            jit_emit32(&jb, 0x000280e7);
            jit_emit32(&jb, 0x00c0006f);
            jit_emit_ptr(&jb, (void *)js_jit_get_loc);
            /* mv s1, a0 */
            jit_emit32(&jb, 0x00050493);
        }
        else if (opcode == OP_put_loc || opcode == OP_put_loc8 || (opcode >= OP_put_loc0 && opcode <= OP_put_loc3)) {
            int idx = 0;
            if (opcode == OP_put_loc) {
                idx = pc_read16(pc);
                pc += 2;
            } else if (opcode == OP_put_loc8) {
                idx = *pc++;
            } else {
                idx = opcode - OP_put_loc0;
            }
            jit_emit32(&jb, 0x00090513);
            jit_emit32(&jb, 0x000a0593);
            jit_emit32(&jb, 0x00048613);
            int32_t hi20 = (idx + 0x800) >> 12;
            int32_t lo12 = idx - (hi20 << 12);
            jit_emit32(&jb, 0x000006b7 | (((uint32_t)hi20 & 0xfffff) << 12));
            jit_emit32(&jb, 0x00068693 | (((uint32_t)lo12 & 0xfff) << 20));
            jit_emit32(&jb, 0x00000297);
            jit_emit32(&jb, 0x0102b283);
            jit_emit32(&jb, 0x000280e7);
            jit_emit32(&jb, 0x00c0006f);
            jit_emit_ptr(&jb, (void *)js_jit_put_loc);
            jit_emit32(&jb, 0x00050493);
        }
        else if (opcode == OP_set_loc || opcode == OP_set_loc8 || (opcode >= OP_set_loc0 && opcode <= OP_set_loc3)) {
            int idx = 0;
            if (opcode == OP_set_loc) {
                idx = pc_read16(pc);
                pc += 2;
            } else if (opcode == OP_set_loc8) {
                idx = *pc++;
            } else {
                idx = opcode - OP_set_loc0;
            }
            jit_emit32(&jb, 0x00090513);
            jit_emit32(&jb, 0x000a0593);
            jit_emit32(&jb, 0x00048613);
            int32_t hi20 = (idx + 0x800) >> 12;
            int32_t lo12 = idx - (hi20 << 12);
            jit_emit32(&jb, 0x000006b7 | (((uint32_t)hi20 & 0xfffff) << 12));
            jit_emit32(&jb, 0x00068693 | (((uint32_t)lo12 & 0xfff) << 20));
            jit_emit32(&jb, 0x00000297);
            jit_emit32(&jb, 0x0102b283);
            jit_emit32(&jb, 0x000280e7);
            jit_emit32(&jb, 0x00c0006f);
            jit_emit_ptr(&jb, (void *)js_jit_set_loc);
            jit_emit32(&jb, 0x00050493);
        }
        else if (opcode == OP_push_const8 || opcode == OP_push_const) {
            int idx = 0;
            if (opcode == OP_push_const8) {
                idx = *pc++;
            } else {
                idx = pc_read16(pc);
                pc += 2;
            }
            /* mv a0, s2 (ctx) */
            jit_emit32(&jb, 0x00090513);
            /* mv a1, s5 (cpool) */
            jit_emit32(&jb, 0x000a8593);
            /* mv a2, s1 (sp) */
            jit_emit32(&jb, 0x00048613);
            /* li a3, idx */
            int32_t hi20 = (idx + 0x800) >> 12;
            int32_t lo12 = idx - (hi20 << 12);
            jit_emit32(&jb, 0x000006b7 | (((uint32_t)hi20 & 0xfffff) << 12));
            jit_emit32(&jb, 0x00068693 | (((uint32_t)lo12 & 0xfff) << 20));
            jit_emit32(&jb, 0x00000297);
            jit_emit32(&jb, 0x0102b283);
            jit_emit32(&jb, 0x000280e7);
            jit_emit32(&jb, 0x00c0006f);
            jit_emit_ptr(&jb, (void *)js_jit_push_const);
            jit_emit32(&jb, 0x00050493);
        }
        else if (opcode == OP_add || opcode == OP_sub || opcode == OP_mul || opcode == OP_lt || opcode == OP_neq) {
            void *func = NULL;
            if (opcode == OP_add) func = (void *)js_jit_add;
            else if (opcode == OP_sub) func = (void *)js_jit_sub;
            else if (opcode == OP_mul) func = (void *)js_jit_mul;
            else if (opcode == OP_lt) func = (void *)js_jit_lt;
            else func = (void *)js_jit_neq;

            /* mv a0, s2 (ctx) */
            jit_emit32(&jb, 0x00090513);
            /* mv a1, s1 (sp) */
            jit_emit32(&jb, 0x00048593);
            jit_emit32(&jb, 0x00000297);
            jit_emit32(&jb, 0x0102b283);
            jit_emit32(&jb, 0x000280e7);
            jit_emit32(&jb, 0x00c0006f);
            jit_emit_ptr(&jb, func);
            /* beqz a0, exit_exception */
            relocs[reloc_count].jit_patch_offset = jb.idx;
            relocs[reloc_count].target_bytecode_pc = -999;
            reloc_count++;
            jit_emit32(&jb, 0x00050063);
            /* mv s1, a0 */
            jit_emit32(&jb, 0x00050493);
        }
        else if (opcode == OP_if_true || opcode == OP_if_false) {
            int target_bytecode_pc = pc_offset + pc_read32(pc);
            pc += 4;

            /* mv a0, s2 (ctx) */
            jit_emit32(&jb, 0x00090513);
            /* mv a1, s3 (sp_ref pointer saved in prologue) */
            jit_emit32(&jb, 0x00098593);
            /* sd s1, 0(a1) (*sp_ref = s1) */
            jit_emit32(&jb, 0x0095b023);
            /* call js_jit_if_true */
            jit_emit32(&jb, 0x00000297);
            jit_emit32(&jb, 0x0102b283);
            jit_emit32(&jb, 0x000280e7);
            jit_emit32(&jb, 0x00c0006f);
            jit_emit_ptr(&jb, (void *)js_jit_if_true);
            /* addi t0, zero, -1 */
            jit_emit32(&jb, 0xfff00293);
            /* beq a0, t0, exit_exception */
            relocs[reloc_count].jit_patch_offset = jb.idx;
            relocs[reloc_count].target_bytecode_pc = -999;
            reloc_count++;
            jit_emit32(&jb, 0x00550063);
            /* reload working sp: ld s1, 0(s3) */
            jit_emit32(&jb, 0x0009b483);
            /* branch to target */
            relocs[reloc_count].jit_patch_offset = jb.idx;
            relocs[reloc_count].target_bytecode_pc = target_bytecode_pc;
            reloc_count++;
            jit_emit32(&jb, (opcode == OP_if_true) ? 0x00051063 : 0x00050063);
        }
        else if (opcode == OP_goto) {
            int target_bytecode_pc = pc_offset + pc_read32(pc);
            pc += 4;
            relocs[reloc_count].jit_patch_offset = jb.idx;
            relocs[reloc_count].target_bytecode_pc = target_bytecode_pc;
            reloc_count++;
            jit_emit32(&jb, 0x0000006f);
        }
        else if (opcode == OP_return) {
            /* addi s1, s1, -16 */
            jit_emit32(&jb, 0xff048413);
            /* ld a0, 0(s1) */
            jit_emit32(&jb, 0x0004b503);
            /* ld a1, 8(s1) */
            jit_emit32(&jb, 0x0084b583);
            /* sd s1, 0(s3) (*sp_ref = s1) */
            jit_emit32(&jb, 0x0099b023);
            /* Epilogue sequence */
            uint8_t ret_seq[] = {
                0x83, 0x3b, 0x01, 0x00,             /* ld s6, 0(sp) */
                0x83, 0x3a, 0x81, 0x00,             /* ld s5, 8(sp) */
                0x83, 0x3a, 0x01, 0x01,             /* ld s4, 16(sp) */
                0x83, 0x39, 0x81, 0x01,             /* ld s3, 24(sp) */
                0x83, 0x39, 0x01, 0x02,             /* ld s2, 32(sp) */
                0x83, 0x34, 0x81, 0x02,             /* ld s1, 40(sp) */
                0x83, 0x30, 0x81, 0x03,             /* ld s0, 48(sp) */
                0x83, 0x30, 0x01, 0x04,             /* ld ra, 56(sp) */
                0x13, 0x01, 0x01, 0x04,             /* addi sp, sp, 64 */
                0x67, 0x80, 0x00, 0x00              /* ret */
            };
            jit_emit_memcpy(&jb, ret_seq, sizeof(ret_seq));
        }
        else if (opcode == OP_return_undef) {
            /* mv a0, zero */
            jit_emit32(&jb, 0x00000513);
            /* addi a1, zero, 3 (JS_TAG_UNDEFINED) */
            jit_emit32(&jb, 0x00300593);
            /* sd s1, 0(s3) (*sp_ref = s1) */
            jit_emit32(&jb, 0x0099b023);
            uint8_t ret_seq[] = {
                0x83, 0x3b, 0x01, 0x00,
                0x83, 0x3a, 0x81, 0x00,
                0x83, 0x3a, 0x01, 0x01,
                0x83, 0x39, 0x81, 0x01,
                0x83, 0x39, 0x01, 0x02,
                0x83, 0x34, 0x81, 0x02,
                0x83, 0x30, 0x81, 0x03,
                0x83, 0x30, 0x01, 0x04,
                0x13, 0x01, 0x01, 0x04,
                0x67, 0x80, 0x00, 0x00
            };
            jit_emit_memcpy(&jb, ret_seq, sizeof(ret_seq));
        }
        else {
            free(jb.buf);
            free(bytecode_to_jit_offset);
            free(relocs);
            return;
        }

        if (jb.failed) {
            free(jb.buf);
            free(bytecode_to_jit_offset);
            free(relocs);
            return;
        }
    }

    exception_exit_offset = jb.idx;
    /* mv a0, zero */
    jit_emit32(&jb, 0x00000513);
    /* addi a1, zero, 6 (JS_TAG_EXCEPTION) */
    jit_emit32(&jb, 0x00600593);
    /* sd s1, 0(s3) */
    jit_emit32(&jb, 0x0099b023);
    uint8_t ret_seq[] = {
        0x83, 0x3b, 0x01, 0x00,
        0x83, 0x3a, 0x81, 0x00,
        0x83, 0x3a, 0x01, 0x01,
        0x83, 0x39, 0x81, 0x01,
        0x83, 0x39, 0x01, 0x02,
        0x83, 0x34, 0x81, 0x02,
        0x83, 0x30, 0x81, 0x03,
        0x83, 0x30, 0x01, 0x04,
        0x13, 0x01, 0x01, 0x04,
        0x67, 0x80, 0x00, 0x00
    };
    jit_emit_memcpy(&jb, ret_seq, sizeof(ret_seq));

    if (jb.failed) {
        free(jb.buf);
        free(bytecode_to_jit_offset);
        free(relocs);
        return;
    }

    for (int i = 0; i < reloc_count; i++) {
        int patch_offset = relocs[i].jit_patch_offset;
        int target_pc = relocs[i].target_bytecode_pc;
        int target_jit_pos = -1;

        if (target_pc == -999) {
            target_jit_pos = exception_exit_offset;
        } else {
            target_jit_pos = bytecode_to_jit_offset[target_pc];
        }

        if (target_jit_pos != -1) {
            int rel_offset = target_jit_pos - patch_offset;
            uint32_t *inst_ptr = (uint32_t *)(jb.buf + patch_offset);
            uint32_t inst = *inst_ptr;
            if ((inst & 0x7f) == 0x6f) {
                /* JAL instruction encoding: 20-bit byte offset */
                int32_t imm = rel_offset;
                uint32_t imm20 = ((uint32_t)imm >> 20) & 1;
                uint32_t imm10_1 = ((uint32_t)imm >> 1) & 0x3ff;
                uint32_t imm11 = ((uint32_t)imm >> 11) & 1;
                uint32_t imm19_12 = ((uint32_t)imm >> 12) & 0xff;
                inst = (inst & 0x0000007f) | (imm20 << 31) | (imm10_1 << 21) | (imm11 << 20) | (imm19_12 << 12);
            } else if ((inst & 0x7f) == 0x63) {
                /* B-type branch instruction encoding (BEQ/BNE): 12-bit byte offset */
                int32_t imm = rel_offset;
                uint32_t imm12 = ((uint32_t)imm >> 12) & 1;
                uint32_t imm10_5 = ((uint32_t)imm >> 5) & 0x3f;
                uint32_t imm4_1 = ((uint32_t)imm >> 1) & 0xf;
                uint32_t imm11 = ((uint32_t)imm >> 11) & 1;
                inst = (inst & 0x01fff07f) | (imm12 << 31) | (imm10_5 << 25) | (imm4_1 << 8) | (imm11 << 7);
            }
            *inst_ptr = inst;
        }
    }

    void *jit_mem = mmap(NULL, jb.idx, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (jit_mem != MAP_FAILED) {
        memcpy(jit_mem, jb.buf, jb.idx);
        mprotect(jit_mem, jb.idx, PROT_READ | PROT_EXEC);
#if defined(__GNUC__) || defined(__clang__)
        __builtin___clear_cache((char *)jit_mem, (char *)jit_mem + jb.idx);
#endif
        b->jit_code = jit_mem;
        b->jit_size = jb.idx;
    }

    free(jb.buf);
    free(bytecode_to_jit_offset);
    free(relocs);
}

#endif

void qjs_jit_compile(JSFunctionBytecode *b) {
#if defined(__x86_64__)
    qjs_jit_compile_x86_64(b);
#elif defined(__aarch64__)
    qjs_jit_compile_arm64(b);
#elif defined(__riscv) && __riscv_xlen == 64
    qjs_jit_compile_rv64(b);
#else
    (void)b;
#endif
}

void qjs_jit_free(void *ptr, size_t size) {
    if (ptr) {
        munmap(ptr, size);
    }
}

#endif /* CONFIG_JIT */
