#include "ir/hlir.h"
#include "ir/hlir_opt.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define ASSERT(expected, actual) do { \
    int64_t e = (int64_t)(expected); \
    int64_t a = (int64_t)(actual); \
    if (e != a) { \
        fprintf(stderr, "%s:%d: ASSERT failed: %lld expected, but got %lld\n", __FILE__, __LINE__, (long long)e, (long long)a); \
        exit(1); \
    } \
} while (0)

#define ASSERT_PTR_EQ(expected, actual) do { \
    void *e = (void *)(expected); \
    void *a = (void *)(actual); \
    if (e != a) { \
        fprintf(stderr, "%s:%d: ASSERT_PTR_EQ failed: %p expected, but got %p\n", __FILE__, __LINE__, e, a); \
        exit(1); \
    } \
} while (0)

// Dummy implementations for linker
void chibicc_assert_fail(const char *expr, const char *file, int line, const char *func) {
    fprintf(stderr, "Assertion failed: %s, file %s, line %d, function %s\n", expr, file, line, func);
    exit(1);
}
bool file_exists(const char *path) { return false; }
StringArray include_paths;
char *base_file;
int opt_O;
bool opt_dump_ir;
bool opt_fpic;
bool opt_fcommon;
bool opt_ffunction_sections;
bool opt_fdata_sections;
bool opt_g;
StringArray tmpfiles;

// Helper to create a dummy HLIR function
static HLIRFunction *new_test_fn(char *name) {
    HLIRFunction *fn = calloc(1, sizeof(HLIRFunction));
    fn->name = _strdup(name);
    fn->val_cap = 16;
    fn->vals = calloc(fn->val_cap, sizeof(HLIRVal *));
    return fn;
}

// Helper for iconst
static HLIRVal *add_iconst(HLIRFunction *fn, int64_t val) {
    HLIRVal *dst = hlir_new_val(fn, ty_int);
    HLIRInsn *insn = hlir_new_insn(HLIR_ICONST);
    insn->dst = dst;
    insn->imm = val;
    hlir_append_insn(fn, insn);
    return dst;
}

// Helper for binary ops
static HLIRVal *add_binop(HLIRFunction *fn, HLIRKind kind, HLIRVal *src1, HLIRVal *src2) {
    HLIRVal *dst = hlir_new_val(fn, ty_int);
    HLIRInsn *insn = hlir_new_insn(kind);
    insn->dst = dst;
    insn->src1 = src1;
    insn->src2 = src2;
    hlir_append_insn(fn, insn);
    return dst;
}

// Helper for unary ops
static HLIRVal *add_unary(HLIRFunction *fn, HLIRKind kind, HLIRVal *src1) {
    HLIRVal *dst = hlir_new_val(fn, ty_int);
    HLIRInsn *insn = hlir_new_insn(kind);
    insn->dst = dst;
    insn->src1 = src1;
    hlir_append_insn(fn, insn);
    return dst;
}

// Helper for cast
static HLIRVal *add_cast(HLIRFunction *fn, HLIRVal *src1, Type *ty) {
    HLIRVal *dst = hlir_new_val(fn, ty);
    HLIRInsn *insn = hlir_new_insn(HLIR_CAST);
    insn->dst = dst;
    insn->src1 = src1;
    insn->ty = ty;
    hlir_append_insn(fn, insn);
    return dst;
}

static int count_insns(HLIRFunction *fn) {
    int cnt = 0;
    for (HLIRInsn *i = fn->head; i; i = i->next) cnt++;
    return cnt;
}

// 1. Constant Folding Tests
static void test_const_folding_handcrafted(void) {
    // Binary arithmetic constant folding: 10 + 20 -> 30
    {
        HLIRFunction *fn = new_test_fn("test_fold_add");
        HLIRVal *v10 = add_iconst(fn, 10);
        HLIRVal *v20 = add_iconst(fn, 20);
        add_binop(fn, HLIR_ADD, v10, v20);
        
        ASSERT(HLIR_ADD, fn->tail->kind);
        hlir_opt_const_fold(fn);
        ASSERT(HLIR_ICONST, fn->tail->kind);
        ASSERT(30, fn->tail->imm);
    }

    // Binary multiplication & bitwise: 7 * 6 -> 42, 0xF0 & 0xAA -> 0xA0
    {
        HLIRFunction *fn = new_test_fn("test_fold_mul_and");
        HLIRVal *v7 = add_iconst(fn, 7);
        HLIRVal *v6 = add_iconst(fn, 6);
        add_binop(fn, HLIR_MUL, v7, v6);
        hlir_opt_const_fold(fn);
        ASSERT(HLIR_ICONST, fn->tail->kind);
        ASSERT(42, fn->tail->imm);

        HLIRVal *vf0 = add_iconst(fn, 0xF0);
        HLIRVal *vaa = add_iconst(fn, 0xAA);
        add_binop(fn, HLIR_BITAND, vf0, vaa);
        hlir_opt_const_fold(fn);
        ASSERT(HLIR_ICONST, fn->tail->kind);
        ASSERT(0xA0, fn->tail->imm);
    }

    // Unary constant folding: -(-50) -> -50 negated -> 50, ~0 -> -1, !0 -> 1
    {
        HLIRFunction *fn = new_test_fn("test_fold_unary");
        HLIRVal *v50 = add_iconst(fn, -50);
        add_unary(fn, HLIR_NEG, v50);
        hlir_opt_const_fold(fn);
        ASSERT(HLIR_ICONST, fn->tail->kind);
        ASSERT(50, fn->tail->imm);

        HLIRVal *v0 = add_iconst(fn, 0);
        add_unary(fn, HLIR_LOGNOT, v0);
        hlir_opt_const_fold(fn);
        ASSERT(HLIR_ICONST, fn->tail->kind);
        ASSERT(1, fn->tail->imm);
    }

    // Cast constant truncation: (char)300 -> 44
    {
        HLIRFunction *fn = new_test_fn("test_fold_cast_trunc");
        HLIRVal *v300 = add_iconst(fn, 300);
        add_cast(fn, v300, ty_char);
        hlir_opt_const_fold(fn);
        ASSERT(HLIR_ICONST, fn->tail->kind);
        ASSERT(44, fn->tail->imm);
    }

    // Canonicalization: const on src1 moved to src2 (5 + x -> x + 5, 5 < x -> x > 5)
    {
        HLIRFunction *fn = new_test_fn("test_fold_canonicalize");
        HLIRVal *v5 = add_iconst(fn, 5);
        HLIRVal *x = hlir_new_val(fn, ty_int);
        
        // 5 + x
        add_binop(fn, HLIR_ADD, v5, x);
        hlir_opt_const_fold(fn);
        ASSERT(HLIR_ADD, fn->tail->kind);
        ASSERT(x->id, fn->tail->src1->id);
        ASSERT(v5->id, fn->tail->src2->id);

        // 5 < x => x > 5
        add_binop(fn, HLIR_CMP_LT, v5, x);
        hlir_opt_const_fold(fn);
        ASSERT(HLIR_CMP_GT, fn->tail->kind);
        ASSERT(x->id, fn->tail->src1->id);
        ASSERT(v5->id, fn->tail->src2->id);
    }

    printf("test_const_folding_handcrafted passed\n");
}

// 2. Algebraic Simplification Tests
static void test_algebraic_handcrafted(void) {
    // x + 0 -> x, 0 + x -> x
    {
        HLIRFunction *fn = new_test_fn("test_x_plus_0");
        HLIRVal *x = hlir_new_val(fn, ty_int);
        HLIRVal *v0 = add_iconst(fn, 0);
        add_binop(fn, HLIR_ADD, x, v0);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_CAST, fn->tail->kind);
        ASSERT(x->id, fn->tail->src1->id);

        add_binop(fn, HLIR_ADD, v0, x);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_CAST, fn->tail->kind);
        ASSERT(x->id, fn->tail->src1->id);
    }

    // x - 0 -> x, 0 - x -> -x, x - x -> 0
    {
        HLIRFunction *fn = new_test_fn("test_x_sub_rules");
        HLIRVal *x = hlir_new_val(fn, ty_int);
        HLIRVal *v0 = add_iconst(fn, 0);
        
        add_binop(fn, HLIR_SUB, x, v0);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_CAST, fn->tail->kind);
        ASSERT(x->id, fn->tail->src1->id);

        add_binop(fn, HLIR_SUB, v0, x);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_NEG, fn->tail->kind);
        ASSERT(x->id, fn->tail->src1->id);

        add_binop(fn, HLIR_SUB, x, x);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_ICONST, fn->tail->kind);
        ASSERT(0, fn->tail->imm);
    }

    // x * 1 -> x, x * 0 -> 0, x * -1 -> -x, x * 8 -> x << 3
    {
        HLIRFunction *fn = new_test_fn("test_x_mul_rules");
        HLIRVal *x = hlir_new_val(fn, ty_int);
        HLIRVal *v1 = add_iconst(fn, 1);
        HLIRVal *v0 = add_iconst(fn, 0);
        HLIRVal *vm1 = add_iconst(fn, -1);
        HLIRVal *v8 = add_iconst(fn, 8);

        add_binop(fn, HLIR_MUL, x, v1);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_CAST, fn->tail->kind);
        ASSERT(x->id, fn->tail->src1->id);

        add_binop(fn, HLIR_MUL, x, v0);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_ICONST, fn->tail->kind);
        ASSERT(0, fn->tail->imm);

        add_binop(fn, HLIR_MUL, x, vm1);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_NEG, fn->tail->kind);
        ASSERT(x->id, fn->tail->src1->id);

        add_binop(fn, HLIR_MUL, x, v8);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_SHL, fn->tail->kind);
        ASSERT(x->id, fn->tail->src1->id);
    }

    // Division / Modulo: x / 1 -> x, x / x -> 1, unsigned x / 16 -> x >> 4, unsigned x % 16 -> x & 15
    {
        HLIRFunction *fn = new_test_fn("test_div_mod_rules");
        HLIRVal *ux = hlir_new_val(fn, ty_uint);
        HLIRVal *v1 = add_iconst(fn, 1);
        HLIRVal *v16 = add_iconst(fn, 16);

        add_binop(fn, HLIR_DIV, ux, v1);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_CAST, fn->tail->kind);
        ASSERT(ux->id, fn->tail->src1->id);

        add_binop(fn, HLIR_DIV, ux, ux);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_ICONST, fn->tail->kind);
        ASSERT(1, fn->tail->imm);

        HLIRVal *div_res = add_binop(fn, HLIR_DIV, ux, v16);
        div_res->ty = ty_uint;
        fn->tail->ty = ty_uint;
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_SHR, fn->tail->kind);

        HLIRVal *mod_res = add_binop(fn, HLIR_MOD, ux, v16);
        mod_res->ty = ty_uint;
        fn->tail->ty = ty_uint;
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_BITAND, fn->tail->kind);
    }

    // Bitwise identities: x ^ x -> 0, x ^ 0 -> x, x ^ -1 -> ~x, x & x -> x, x & 0 -> 0, x | x -> x, x | -1 -> -1
    {
        HLIRFunction *fn = new_test_fn("test_bitwise_rules");
        HLIRVal *x = hlir_new_val(fn, ty_int);
        HLIRVal *v0 = add_iconst(fn, 0);
        HLIRVal *vm1 = add_iconst(fn, -1);

        add_binop(fn, HLIR_BITXOR, x, x);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_ICONST, fn->tail->kind);
        ASSERT(0, fn->tail->imm);

        add_binop(fn, HLIR_BITXOR, x, vm1);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_BITNOT, fn->tail->kind);

        add_binop(fn, HLIR_BITAND, x, v0);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_ICONST, fn->tail->kind);
        ASSERT(0, fn->tail->imm);

        add_binop(fn, HLIR_BITOR, x, vm1);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_ICONST, fn->tail->kind);
        ASSERT(-1, fn->tail->imm);
    }

    // Involutions: -(-x) -> x, ~(~x) -> x, !(a < b) -> a >= b
    {
        HLIRFunction *fn = new_test_fn("test_involutions");
        HLIRVal *x = hlir_new_val(fn, ty_int);
        HLIRVal *nx = add_unary(fn, HLIR_NEG, x);
        add_unary(fn, HLIR_NEG, nx);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_CAST, fn->tail->kind);
        ASSERT(x->id, fn->tail->src1->id);

        HLIRVal *bx = add_unary(fn, HLIR_BITNOT, x);
        add_unary(fn, HLIR_BITNOT, bx);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_CAST, fn->tail->kind);
        ASSERT(x->id, fn->tail->src1->id);

        HLIRVal *y = hlir_new_val(fn, ty_int);
        HLIRVal *cmp = add_binop(fn, HLIR_CMP_LT, x, y);
        add_unary(fn, HLIR_LOGNOT, cmp);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_CMP_GE, fn->tail->kind);
        ASSERT(x->id, fn->tail->src1->id);
        ASSERT(y->id, fn->tail->src2->id);
    }

    // Comparison reflexivity: x == x -> 1, x != x -> 0, x <= x -> 1, x < x -> 0
    {
        HLIRFunction *fn = new_test_fn("test_reflexive_cmp");
        HLIRVal *x = hlir_new_val(fn, ty_int);

        add_binop(fn, HLIR_CMP_EQ, x, x);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_ICONST, fn->tail->kind);
        ASSERT(1, fn->tail->imm);

        add_binop(fn, HLIR_CMP_NE, x, x);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_ICONST, fn->tail->kind);
        ASSERT(0, fn->tail->imm);

        add_binop(fn, HLIR_CMP_LE, x, x);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_ICONST, fn->tail->kind);
        ASSERT(1, fn->tail->imm);

        add_binop(fn, HLIR_CMP_LT, x, x);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_ICONST, fn->tail->kind);
        ASSERT(0, fn->tail->imm);
    }

    // Re-association: (x + 10) + 20 -> x + 30
    {
        HLIRFunction *fn = new_test_fn("test_reassoc_add");
        HLIRVal *x = hlir_new_val(fn, ty_int);
        HLIRVal *v10 = add_iconst(fn, 10);
        HLIRVal *v20 = add_iconst(fn, 20);
        HLIRVal *add1 = add_binop(fn, HLIR_ADD, x, v10);
        add_binop(fn, HLIR_ADD, add1, v20);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_ADD, fn->tail->kind);
        ASSERT(x->id, fn->tail->src1->id);
    }

    // Cancellation: (x ^ y) ^ y -> x
    {
        HLIRFunction *fn = new_test_fn("test_xor_cancel");
        HLIRVal *x = hlir_new_val(fn, ty_int);
        HLIRVal *y = hlir_new_val(fn, ty_int);
        HLIRVal *xor1 = add_binop(fn, HLIR_BITXOR, x, y);
        add_binop(fn, HLIR_BITXOR, xor1, y);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_CAST, fn->tail->kind);
        ASSERT(x->id, fn->tail->src1->id);
    }

    // Absorption: (x & y) | x -> x
    {
        HLIRFunction *fn = new_test_fn("test_and_or_absorb");
        HLIRVal *x = hlir_new_val(fn, ty_int);
        HLIRVal *y = hlir_new_val(fn, ty_int);
        HLIRVal *and1 = add_binop(fn, HLIR_BITAND, x, y);
        add_binop(fn, HLIR_BITOR, and1, x);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_CAST, fn->tail->kind);
        ASSERT(x->id, fn->tail->src1->id);
    }

    // Generalized Absorption (commutative): x | (y & x) -> x
    {
        HLIRFunction *fn = new_test_fn("test_and_or_absorb_comm");
        HLIRVal *x = hlir_new_val(fn, ty_int);
        HLIRVal *y = hlir_new_val(fn, ty_int);
        HLIRVal *and1 = add_binop(fn, HLIR_BITAND, y, x);
        add_binop(fn, HLIR_BITOR, x, and1);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_CAST, fn->tail->kind);
        ASSERT(x->id, fn->tail->src1->id);
    }

    // Generalized Cancellation: (x - y) + y -> x and y + (x - y) -> x
    {
        HLIRFunction *fn = new_test_fn("test_sub_add_cancel");
        HLIRVal *x = hlir_new_val(fn, ty_int);
        HLIRVal *y = hlir_new_val(fn, ty_int);
        HLIRVal *sub1 = add_binop(fn, HLIR_SUB, x, y);
        add_binop(fn, HLIR_ADD, sub1, y);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_CAST, fn->tail->kind);
        ASSERT(x->id, fn->tail->src1->id);

        HLIRVal *sub2 = add_binop(fn, HLIR_SUB, x, y);
        add_binop(fn, HLIR_ADD, y, sub2);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_CAST, fn->tail->kind);
        ASSERT(x->id, fn->tail->src1->id);
    }

    // Generalized Negation: (-x) + y -> y - x
    {
        HLIRFunction *fn = new_test_fn("test_neg_add_rules");
        HLIRVal *x = hlir_new_val(fn, ty_int);
        HLIRVal *y = hlir_new_val(fn, ty_int);
        HLIRVal *nx = add_unary(fn, HLIR_NEG, x);
        add_binop(fn, HLIR_ADD, nx, y);
        hlir_opt_algebraic(fn);
        ASSERT(HLIR_SUB, fn->tail->kind);
        ASSERT(y->id, fn->tail->src1->id);
        ASSERT(x->id, fn->tail->src2->id);
    }

    printf("test_algebraic_handcrafted passed\n");
}

// 3. Copy Propagation Tests
static void test_copy_prop_handcrafted(void) {
    HLIRFunction *fn = new_test_fn("test_copy_prop");
    
    // v1 = 100
    // v2 = (cast)v1
    // v3 = (cast)v2
    // v4 = v3 + 5
    HLIRVal *v1 = add_iconst(fn, 100);
    HLIRVal *v2 = add_cast(fn, v1, ty_int);
    HLIRVal *v3 = add_cast(fn, v2, ty_int);
    add_binop(fn, HLIR_ADD, v3, hlir_new_val(fn, ty_int));
    
    hlir_opt_copy_prop(fn);
    
    // v4 = v3 + dummy should become v4 = v1 + dummy
    ASSERT(HLIR_ADD, fn->tail->kind);
    ASSERT(v1->id, fn->tail->src1->id);
    
    printf("test_copy_prop_handcrafted passed\n");
}

// 4. Local CSE Tests
static void test_local_cse_handcrafted(void) {
    // Direct CSE: t1 = a * b, t2 = a * b -> t2 becomes cast(t1)
    {
        HLIRFunction *fn = new_test_fn("test_cse_direct");
        HLIRVal *a = hlir_new_val(fn, ty_int);
        HLIRVal *b = hlir_new_val(fn, ty_int);
        
        HLIRVal *t1 = add_binop(fn, HLIR_MUL, a, b);
        HLIRVal *t2 = add_binop(fn, HLIR_MUL, a, b);
        (void)t2;
        
        hlir_opt_local_cse(fn);
        ASSERT(HLIR_CAST, fn->tail->kind);
        ASSERT(t1->id, fn->tail->src1->id);
    }

    // Commutative CSE: t1 = a + b, t2 = b + a -> t2 becomes cast(t1)
    {
        HLIRFunction *fn = new_test_fn("test_cse_commutative");
        HLIRVal *a = hlir_new_val(fn, ty_int);
        HLIRVal *b = hlir_new_val(fn, ty_int);
        
        HLIRVal *t1 = add_binop(fn, HLIR_ADD, a, b);
        HLIRVal *t2 = add_binop(fn, HLIR_ADD, b, a);
        (void)t2;
        
        hlir_opt_local_cse(fn);
        ASSERT(HLIR_CAST, fn->tail->kind);
        ASSERT(t1->id, fn->tail->src1->id);
    }

    // Unary CSE: t1 = -a, t2 = -a -> t2 becomes cast(t1)
    {
        HLIRFunction *fn = new_test_fn("test_cse_unary");
        HLIRVal *a = hlir_new_val(fn, ty_int);
        
        HLIRVal *t1 = add_unary(fn, HLIR_NEG, a);
        HLIRVal *t2 = add_unary(fn, HLIR_NEG, a);
        (void)t2;
        
        hlir_opt_local_cse(fn);
        ASSERT(HLIR_CAST, fn->tail->kind);
        ASSERT(t1->id, fn->tail->src1->id);
    }

    printf("test_local_cse_handcrafted passed\n");
}

// 5. Load-Store Optimization Tests
static void test_load_store_handcrafted(void) {
    // Redundant load-after-store: store_var x, v1; v2 = load_var x -> v2 = cast(v1)
    {
        HLIRFunction *fn = new_test_fn("test_load_after_store");
        Obj var = { .name = "x", .is_local = true };
        HLIRVal *v1 = add_iconst(fn, 42);
        
        HLIRInsn *st = hlir_new_insn(HLIR_STORE_VAR);
        st->var = &var;
        st->src1 = v1;
        hlir_append_insn(fn, st);
        
        HLIRVal *v2 = hlir_new_val(fn, ty_int);
        HLIRInsn *ld = hlir_new_insn(HLIR_LOAD_VAR);
        ld->dst = v2;
        ld->var = &var;
        hlir_append_insn(fn, ld);
        
        hlir_opt_load_store(fn);
        ASSERT(HLIR_CAST, fn->tail->kind);
        ASSERT(v1->id, fn->tail->src1->id);
    }

    // Dead store elimination: store_var x, v1; store_var x, v2 -> first store removed
    {
        HLIRFunction *fn = new_test_fn("test_dead_store");
        Obj var = { .name = "x", .is_local = true };
        HLIRVal *v1 = add_iconst(fn, 10);
        HLIRVal *v2 = add_iconst(fn, 20);
        
        HLIRInsn *st1 = hlir_new_insn(HLIR_STORE_VAR);
        st1->var = &var;
        st1->src1 = v1;
        hlir_append_insn(fn, st1);
        
        HLIRInsn *st2 = hlir_new_insn(HLIR_STORE_VAR);
        st2->var = &var;
        st2->src1 = v2;
        hlir_append_insn(fn, st2);
        
        int before = count_insns(fn);
        hlir_opt_load_store(fn);
        int after = count_insns(fn);
        ASSERT(before - 1, after);
    }

    // Load-to-Load forwarding: v1 = load_var x; v2 = load_var x -> v2 = cast(v1)
    {
        HLIRFunction *fn = new_test_fn("test_load_to_load");
        Obj var = { .name = "x", .is_local = true };
        
        HLIRVal *v1 = hlir_new_val(fn, ty_int);
        HLIRInsn *ld1 = hlir_new_insn(HLIR_LOAD_VAR);
        ld1->dst = v1;
        ld1->var = &var;
        hlir_append_insn(fn, ld1);
        
        HLIRVal *v2 = hlir_new_val(fn, ty_int);
        HLIRInsn *ld2 = hlir_new_insn(HLIR_LOAD_VAR);
        ld2->dst = v2;
        ld2->var = &var;
        hlir_append_insn(fn, ld2);
        
        hlir_opt_load_store(fn);
        ASSERT(HLIR_CAST, fn->tail->kind);
        ASSERT(v1->id, fn->tail->src1->id);
    }

    printf("test_load_store_handcrafted passed\n");
}

// 6. Dead Code & Dead Value Elimination Tests
static void test_dce_handcrafted(void) {
    HLIRFunction *fn = new_test_fn("test_dce");
    
    // v1 = 10
    // v2 = 20 (unused)
    // RET v1
    HLIRVal *v1 = add_iconst(fn, 10);
    HLIRVal *v2 = add_iconst(fn, 20);
    (void)v2;
    
    HLIRInsn *ret = hlir_new_insn(HLIR_RET);
    ret->src1 = v1;
    hlir_append_insn(fn, ret);
    
    int before = count_insns(fn);
    hlir_opt_dce(fn);
    int after = count_insns(fn);
    
    ASSERT(before - 1, after); // v2 iconst should be eliminated
    
    printf("test_dce_handcrafted passed\n");
}

static void test_dead_code_handcrafted(void) {
    HLIRFunction *fn = new_test_fn("test_unreachable");
    
    HLIRVal *v1 = add_iconst(fn, 10);
    HLIRInsn *ret = hlir_new_insn(HLIR_RET);
    ret->src1 = v1;
    hlir_append_insn(fn, ret);
    
    // Unreachable instructions after RET
    add_iconst(fn, 999);
    add_iconst(fn, 888);
    
    HLIRInsn *lbl = hlir_new_insn(HLIR_LABEL);
    lbl->label = "L_NEXT";
    hlir_append_insn(fn, lbl);
    
    hlir_opt_dead_code(fn);
    
    // After RET, immediate next instruction must be the label
    ASSERT_PTR_EQ(lbl, ret->next);
    
    printf("test_dead_code_handcrafted passed\n");
}

// 7. CFG Optimization Tests
static void test_cfg_handcrafted(void) {
    // JMP to immediately succeeding label removed
    {
        HLIRFunction *fn = new_test_fn("test_jmp_label");
        char *l1 = "L1";
        
        HLIRInsn *jmp = hlir_new_insn(HLIR_JMP);
        jmp->label = l1;
        hlir_append_insn(fn, jmp);
        
        HLIRInsn *label = hlir_new_insn(HLIR_LABEL);
        label->label = l1;
        hlir_append_insn(fn, label);
        
        int before = count_insns(fn);
        hlir_opt_control_flow(fn);
        int after = count_insns(fn);
        
        ASSERT(before - 1, after);
        ASSERT(HLIR_LABEL, fn->head->kind);
    }
    
    // JMP_IF_ZERO(0) -> JMP
    {
        HLIRFunction *fn = new_test_fn("test_jmp_if_zero_0");
        HLIRVal *v0 = add_iconst(fn, 0);
        HLIRInsn *br = hlir_new_insn(HLIR_JMP_IF_ZERO);
        br->src1 = v0;
        br->label = "L_TARGET";
        hlir_append_insn(fn, br);
        
        hlir_opt_control_flow(fn);
        ASSERT(HLIR_JMP, fn->tail->kind);
        ASSERT_PTR_EQ(NULL, fn->tail->src1);
    }

    // JMP_IF_ZERO(non-zero) -> branch removed
    {
        HLIRFunction *fn = new_test_fn("test_jmp_if_zero_1");
        HLIRVal *v1 = add_iconst(fn, 1);
        HLIRInsn *br = hlir_new_insn(HLIR_JMP_IF_ZERO);
        br->src1 = v1;
        br->label = "L_TARGET";
        hlir_append_insn(fn, br);
        
        int before = count_insns(fn);
        hlir_opt_control_flow(fn);
        int after = count_insns(fn);
        ASSERT(before - 1, after);
    }

    // JMP_IF_ZERO (x == 0) -> JMP_IF_NZ x
    {
        HLIRFunction *fn = new_test_fn("test_cond_branch_fold");
        HLIRVal *x = hlir_new_val(fn, ty_int);
        HLIRVal *v0 = add_iconst(fn, 0);
        HLIRVal *cmp = add_binop(fn, HLIR_CMP_EQ, x, v0);
        
        HLIRInsn *br = hlir_new_insn(HLIR_JMP_IF_ZERO);
        br->src1 = cmp;
        br->label = "L_TARGET";
        hlir_append_insn(fn, br);
        
        hlir_opt_control_flow(fn);
        ASSERT(HLIR_JMP_IF_NZ, fn->tail->kind);
        ASSERT(x->id, fn->tail->src1->id);
    }

    // JMP_IF_ZERO (!x) -> JMP_IF_NZ x
    {
        HLIRFunction *fn = new_test_fn("test_lognot_branch_fold");
        HLIRVal *x = hlir_new_val(fn, ty_int);
        HLIRVal *not_x = add_unary(fn, HLIR_LOGNOT, x);
        
        HLIRInsn *br = hlir_new_insn(HLIR_JMP_IF_ZERO);
        br->src1 = not_x;
        br->label = "L_TARGET";
        hlir_append_insn(fn, br);
        
        hlir_opt_control_flow(fn);
        ASSERT(HLIR_JMP_IF_NZ, fn->tail->kind);
        ASSERT(x->id, fn->tail->src1->id);
    }

    // Jump threading: JMP L1; ... L1: JMP L2 -> JMP L2
    {
        HLIRFunction *fn = new_test_fn("test_jump_threading");
        HLIRInsn *jmp1 = hlir_new_insn(HLIR_JMP);
        jmp1->label = "L1";
        hlir_append_insn(fn, jmp1);
        
        HLIRInsn *lbl_other = hlir_new_insn(HLIR_LABEL);
        lbl_other->label = "L_OTHER";
        hlir_append_insn(fn, lbl_other);
        
        HLIRInsn *lbl1 = hlir_new_insn(HLIR_LABEL);
        lbl1->label = "L1";
        hlir_append_insn(fn, lbl1);
        
        HLIRInsn *jmp2 = hlir_new_insn(HLIR_JMP);
        jmp2->label = "L2";
        hlir_append_insn(fn, jmp2);
        
        hlir_opt_control_flow(fn);
        ASSERT(0, strcmp("L2", jmp1->label));
    }

    printf("test_cfg_handcrafted passed\n");
}

int main(void) {
    test_const_folding_handcrafted();
    test_algebraic_handcrafted();
    test_copy_prop_handcrafted();
    test_local_cse_handcrafted();
    test_load_store_handcrafted();
    test_dce_handcrafted();
    test_dead_code_handcrafted();
    test_cfg_handcrafted();
    return 0;
}
