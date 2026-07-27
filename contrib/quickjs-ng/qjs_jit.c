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

void qjs_jit_compile(JSFunctionBytecode *b) {
    /* Step 1: Pre-alloc temporary buffers for compiler tracking */
    JITBuffer jb;
    jb.capacity = 256; /* Small starting capacity to force frequent resizes and exercise reallocation */
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

    /* Exception Exit Offset */
    int exception_exit_offset = -1;

    uint8_t *pc = b->byte_code_buf;
    uint8_t *pc_end = pc + b->byte_code_len;

    while (pc < pc_end) {
        int pc_offset = pc - b->byte_code_buf;
        bytecode_to_jit_offset[pc_offset] = jb.idx;

        int opcode = *pc++;

        /* Handle opcodes */
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
            /* mov rdi, r12 (ctx) */
            jit_emit8(&jb, 0x4c); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xe7);
            /* mov rsi, r13 (var_buf) */
            jit_emit8(&jb, 0x4c); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xee);
            /* mov rdx, rbx (sp) */
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xda);
            /* mov ecx, idx */
            jit_emit8(&jb, 0xb9); jit_emit32(&jb, idx);
            /* mov rax, js_jit_get_loc */
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0xb8);
            jit_emit_ptr(&jb, (void *)js_jit_get_loc);
            /* call rax */
            jit_emit8(&jb, 0xff); jit_emit8(&jb, 0xd0);
            /* mov rbx, rax */
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
            /* mov rdi, r12 (ctx) */
            jit_emit8(&jb, 0x4c); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xe7);
            /* mov rsi, r13 (var_buf) */
            jit_emit8(&jb, 0x4c); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xee);
            /* mov rdx, rbx (sp) */
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xda);
            /* mov ecx, idx */
            jit_emit8(&jb, 0xb9); jit_emit32(&jb, idx);
            /* mov rax, js_jit_put_loc */
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0xb8);
            jit_emit_ptr(&jb, (void *)js_jit_put_loc);
            /* call rax */
            jit_emit8(&jb, 0xff); jit_emit8(&jb, 0xd0);
            /* mov rbx, rax */
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
            /* mov rdi, r12 (ctx) */
            jit_emit8(&jb, 0x4c); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xe7);
            /* mov rsi, r13 (var_buf) */
            jit_emit8(&jb, 0x4c); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xee);
            /* mov rdx, rbx (sp) */
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xda);
            /* mov ecx, idx */
            jit_emit8(&jb, 0xb9); jit_emit32(&jb, idx);
            /* mov rax, js_jit_set_loc */
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0xb8);
            jit_emit_ptr(&jb, (void *)js_jit_set_loc);
            /* call rax */
            jit_emit8(&jb, 0xff); jit_emit8(&jb, 0xd0);
            /* mov rbx, rax */
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
            /* mov rdi, r12 (ctx) */
            jit_emit8(&jb, 0x4c); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xe7);
            /* mov rsi, r14 (cpool) */
            jit_emit8(&jb, 0x4c); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xf6);
            /* mov rdx, rbx (sp) */
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xda);
            /* mov ecx, idx */
            jit_emit8(&jb, 0xb9); jit_emit32(&jb, idx);
            /* mov rax, js_jit_push_const */
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0xb8);
            jit_emit_ptr(&jb, (void *)js_jit_push_const);
            /* call rax */
            jit_emit8(&jb, 0xff); jit_emit8(&jb, 0xd0);
            /* mov rbx, rax */
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xc3);
        }
        else if (opcode == OP_add || opcode == OP_sub || opcode == OP_mul || opcode == OP_lt || opcode == OP_neq) {
            void *func = NULL;
            if (opcode == OP_add) func = (void *)js_jit_add;
            else if (opcode == OP_sub) func = (void *)js_jit_sub;
            else if (opcode == OP_mul) func = (void *)js_jit_mul;
            else if (opcode == OP_lt) func = (void *)js_jit_lt;
            else func = (void *)js_jit_neq;

            /* mov rdi, r12 (ctx) */
            jit_emit8(&jb, 0x4c); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xe7);
            /* mov rsi, rbx (sp)
             * Fix: ModRM byte for mov rsi, rbx must be 0xde (0xdf is mov rdi, rbx)
             */
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xde);
            /* mov rax, func */
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0xb8);
            jit_emit_ptr(&jb, func);
            /* call rax */
            jit_emit8(&jb, 0xff); jit_emit8(&jb, 0xd0);
            /* test rax, rax */
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x85); jit_emit8(&jb, 0xc0);
            /* jz exit_exception */
            jit_emit8(&jb, 0x0f); jit_emit8(&jb, 0x84);
            relocs[reloc_count].jit_patch_offset = jb.idx;
            relocs[reloc_count].target_bytecode_pc = -999;
            reloc_count++;
            jit_emit32(&jb, 0);
            /* mov rbx, rax */
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xc3);
        }
        else if (opcode == OP_if_true || opcode == OP_if_false) {
            int target_bytecode_pc = pc_offset + pc_read32(pc);
            pc += 4;

            /* mov rdi, r12 (ctx) */
            jit_emit8(&jb, 0x4c); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0xe7);
            /* mov rsi, [rbp - 48] (sp_ref) */
            uint8_t mov_rsi[] = { 0x48, 0x8b, 0x75, 0xd0 };
            jit_emit_memcpy(&jb, mov_rsi, sizeof(mov_rsi));
            /* mov [rsi], rbx */
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0x1e);
            /* mov rax, js_jit_if_true */
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0xb8);
            jit_emit_ptr(&jb, (void *)js_jit_if_true);
            /* call rax */
            jit_emit8(&jb, 0xff); jit_emit8(&jb, 0xd0);
            /* cmp rax, -1 */
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x83); jit_emit8(&jb, 0xf8); jit_emit8(&jb, 0xff);
            /* je exit_exception */
            jit_emit8(&jb, 0x0f); jit_emit8(&jb, 0x84);
            relocs[reloc_count].jit_patch_offset = jb.idx;
            relocs[reloc_count].target_bytecode_pc = -999;
            reloc_count++;
            jit_emit32(&jb, 0);
            /* mov rsi, [rbp - 48] */
            jit_emit_memcpy(&jb, mov_rsi, sizeof(mov_rsi));
            /* mov rbx, [rsi] */
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x8b); jit_emit8(&jb, 0x1e);
            /* test rax, rax */
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x85); jit_emit8(&jb, 0xc0);
            /* jz/jnz target
             * OP_if_true (opcode 47) jumps if not zero (0x85)
             * OP_if_false (opcode 48) jumps if zero (0x84)
             */
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
            /* jmp displacement */
            jit_emit8(&jb, 0xe9);
            relocs[reloc_count].jit_patch_offset = jb.idx;
            relocs[reloc_count].target_bytecode_pc = target_bytecode_pc;
            reloc_count++;
            jit_emit32(&jb, 0);
        }
        else if (opcode == OP_return) {
            /* sub rbx, 16 */
            uint8_t sub_rbx[] = { 0x48, 0x83, 0xeb, 0x10 };
            jit_emit_memcpy(&jb, sub_rbx, sizeof(sub_rbx));
            /* mov rax, [rbx] */
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x8b); jit_emit8(&jb, 0x03);
            /* mov rdx, [rbx + 8] */
            uint8_t mov_rdx[] = { 0x48, 0x8b, 0x53, 0x08 };
            jit_emit_memcpy(&jb, mov_rdx, sizeof(mov_rdx));
            /* mov rsi, [rbp - 48] (sp_ref) */
            uint8_t mov_rsi[] = { 0x48, 0x8b, 0x75, 0xd0 };
            jit_emit_memcpy(&jb, mov_rsi, sizeof(mov_rsi));
            /* mov [rsi], rbx */
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0x1e);
            /* restore stack and return */
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
            /* mov rax, 0 */
            uint8_t mov_rax[] = { 0x48, 0xc7, 0xc0, 0x00, 0x00, 0x00, 0x00 };
            jit_emit_memcpy(&jb, mov_rax, sizeof(mov_rax));
            /* mov rdx, 3 (JS_TAG_UNDEFINED) */
            uint8_t mov_rdx[] = { 0x48, 0xc7, 0xc2, 0x03, 0x00, 0x00, 0x00 };
            jit_emit_memcpy(&jb, mov_rdx, sizeof(mov_rdx));
            /* mov rsi, [rbp - 48] (sp_ref) */
            uint8_t mov_rsi[] = { 0x48, 0x8b, 0x75, 0xd0 };
            jit_emit_memcpy(&jb, mov_rsi, sizeof(mov_rsi));
            /* mov [rsi], rbx */
            jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0x1e);
            /* restore stack and return */
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
            /* Unsupported opcode. Safe, dynamic fallback to Tier 0 interpreter! */
            free(jb.buf);
            free(bytecode_to_jit_offset);
            free(relocs);
            return;
        }

        /* If resize failure or other failure occurred, fallback */
        if (jb.failed) {
            free(jb.buf);
            free(bytecode_to_jit_offset);
            free(relocs);
            return;
        }
    }

    /* Exception exit path stencil */
    exception_exit_offset = jb.idx;
    /* mov rax, 0 */
    uint8_t mov_rax[] = { 0x48, 0xc7, 0xc0, 0x00, 0x00, 0x00, 0x00 };
    jit_emit_memcpy(&jb, mov_rax, sizeof(mov_rax));
    /* mov rdx, 6 (JS_TAG_EXCEPTION) */
    uint8_t mov_rdx[] = { 0x48, 0xc7, 0xc2, 0x06, 0x00, 0x00, 0x00 };
    jit_emit_memcpy(&jb, mov_rdx, sizeof(mov_rdx));
    /* mov rsi, [rbp - 48] (sp_ref) */
    uint8_t mov_rsi[] = { 0x48, 0x8b, 0x75, 0xd0 };
    jit_emit_memcpy(&jb, mov_rsi, sizeof(mov_rsi));
    /* mov [rsi], rbx */
    jit_emit8(&jb, 0x48); jit_emit8(&jb, 0x89); jit_emit8(&jb, 0x1e);
    /* restore stack and return */
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

    /* Relocation Pass: resolve jumps/branches and exception exits */
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

    /* Allocate Executable Memory Pages following W^X policies */
    void *jit_mem = mmap(NULL, jb.idx, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (jit_mem != MAP_FAILED) {
        memcpy(jit_mem, jb.buf, jb.idx);

        /* Safeguard 3: Toggle memory permissions and flush cache */
        mprotect(jit_mem, jb.idx, PROT_READ | PROT_EXEC);

#if defined(__GNUC__) || defined(__clang__)
        __builtin___clear_cache((char *)jit_mem, (char *)jit_mem + jb.idx);
#endif

        b->jit_code = jit_mem;
        b->jit_size = jb.idx;
    }

    /* Clean up compiler buffers */
    free(jb.buf);
    free(bytecode_to_jit_offset);
    free(relocs);
}

void qjs_jit_free(void *ptr, size_t size) {
    if (ptr) {
        munmap(ptr, size);
    }
}

#endif /* CONFIG_JIT */
