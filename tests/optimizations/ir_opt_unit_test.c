#include "ir/ir.h"
#include "ir/opt.h"
#include "ir/regalloc.h"
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

#define ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "%s:%d: ASSERT_TRUE failed: %s\n", __FILE__, __LINE__, #expr); \
        exit(1); \
    } \
} while (0)

// Dummy implementations for linker
void chibicc_assert_fail(const char *expr, const char *file, int line, const char *func) {
    fprintf(stderr, "Assertion failed: %s, file %s, line %d, function %s\n", expr, file, line, func);
    exit(1);
}
bool file_exists(const char *path) { (void)path; return false; }
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

static IRFunction *new_test_ir_fn(char *name) {
    IRFunction *fn = calloc(1, sizeof(IRFunction));
    fn->name = _strdup(name);
    fn->vreg_capacity = 16;
    fn->vregs = calloc(fn->vreg_capacity, sizeof(IRVReg *));
    return fn;
}

static IRVReg *add_imm(IRFunction *fn, int64_t imm) {
    IRVReg *v = ir_new_vreg(fn, ty_long);
    IRInsn *insn = ir_new_insn(IR_IMM);
    insn->dst = v;
    insn->imm = imm;
    insn->ty = ty_long;
    v->def_insn = insn;
    ir_append_insn(fn, insn);
    return v;
}

static IRVReg *add_bin(IRFunction *fn, IRKind kind, IRVReg *s1, IRVReg *s2) {
    IRVReg *v = ir_new_vreg(fn, ty_long);
    IRInsn *insn = ir_new_insn(kind);
    insn->dst = v;
    insn->src1 = s1;
    insn->src2 = s2;
    insn->ty = ty_long;
    v->def_insn = insn;
    ir_append_insn(fn, insn);
    return v;
}

static void test_cfg_and_dom_tree(void) {
    printf("Testing CFG and Dominator Tree...\n");
    IRFunction *fn = new_test_ir_fn("test_cfg");

    // BB0: Entry block
    IRVReg *v1 = add_imm(fn, 10);
    IRInsn *br = ir_new_insn(IR_BR);
    br->src1 = v1;
    br->label_true = "L_then";
    br->label_false = "L_else";
    ir_append_insn(fn, br);

    // BB1: L_then
    IRInsn *l_then = ir_new_insn(IR_LABEL);
    l_then->label = "L_then";
    ir_append_insn(fn, l_then);
    add_imm(fn, 20);
    IRInsn *jmp_exit = ir_new_insn(IR_JMP);
    jmp_exit->label = "L_exit";
    ir_append_insn(fn, jmp_exit);

    // BB2: L_else
    IRInsn *l_else = ir_new_insn(IR_LABEL);
    l_else->label = "L_else";
    ir_append_insn(fn, l_else);
    add_imm(fn, 30);
    IRInsn *jmp_exit2 = ir_new_insn(IR_JMP);
    jmp_exit2->label = "L_exit";
    ir_append_insn(fn, jmp_exit2);

    // BB3: L_exit
    IRInsn *l_exit = ir_new_insn(IR_LABEL);
    l_exit->label = "L_exit";
    ir_append_insn(fn, l_exit);
    IRInsn *ret = ir_new_insn(IR_RET);
    ir_append_insn(fn, ret);

    IRCFG *cfg = ir_build_cfg(fn);
    ASSERT_TRUE(cfg != NULL);
    ASSERT_TRUE(cfg->num_blocks >= 4);
    ASSERT_TRUE(cfg->entry_block->reachable);

    IRDomTree *dt = ir_build_dom_tree(cfg);
    ASSERT_TRUE(dt != NULL);
    // Entry block dominates all reachable blocks
    for (int i = 0; i < cfg->num_blocks; i++) {
        if (cfg->blocks[i]->reachable) {
            ASSERT_TRUE(ir_dom_dominates(dt, cfg->entry_block, cfg->blocks[i]));
        }
    }

    ir_free_dom_tree(dt);
    ir_free_cfg(cfg);
}

static void test_liveness_analysis(void) {
    printf("Testing Liveness Analysis...\n");
    IRFunction *fn = new_test_ir_fn("test_liveness");

    IRVReg *v1 = add_imm(fn, 42);
    IRVReg *v2 = add_imm(fn, 58);
    IRVReg *v3 = add_bin(fn, IR_ADD, v1, v2);

    IRInsn *call = ir_new_insn(IR_CALL);
    call->label = "dummy_func";
    ir_append_insn(fn, call);

    add_bin(fn, IR_ADD, v3, v1); // v1 is used after call!
    ir_renumber_insns(fn);

    IRLiveness *liv = ir_compute_liveness(fn);
    ASSERT_TRUE(liv != NULL);
    ASSERT_TRUE(liv->is_live_across_calls[v1->id]);

    ir_free_liveness(liv);
}

static void test_pass_manager_and_registry(void) {
    printf("Testing Pass Manager and Registry...\n");
    ir_init_pass_registry();

    IRPass *cp = ir_find_pass("copy-prop");
    ASSERT_TRUE(cp != NULL);
    ASSERT_TRUE(!strcmp(cp->name, "copy-prop"));

    ASSERT_TRUE(ir_opt_set_pass_enabled("copy-prop", true));
    ASSERT_TRUE(ir_opt_is_pass_enabled("copy-prop"));

    IRPassManager *pm = ir_pass_manager_new();
    ASSERT_TRUE(pm != NULL);
    ir_pass_manager_add(pm, cp);
    ASSERT(1, pm->num_passes);

    ir_pass_manager_free(pm);
}

static void test_llir_peephole_and_algebraic(void) {
    printf("Testing LLIR Peephole & Algebraic Rewrites...\n");
    IRFunction *fn = new_test_ir_fn("test_peephole");

    // Symbolic variable (not an immediate)
    IRVReg *x = ir_new_vreg(fn, ty_long);
    IRInsn *param = ir_new_insn(IR_PARAM);
    param->dst = x;
    param->ty = ty_long;
    x->def_insn = param;
    ir_append_insn(fn, param);

    // x + 0 -> x (IR_MOV)
    IRVReg *zero = add_imm(fn, 0);
    IRVReg *res1 = add_bin(fn, IR_ADD, x, zero);

    // x * 8 -> x << 3 (IR_SHL)
    IRVReg *eight = add_imm(fn, 8);
    IRVReg *res2 = add_bin(fn, IR_MUL, x, eight);

    (void)res1;
    (void)res2;

    ASSERT_TRUE(ir_opt_peephole(fn));

    // Verify identity: x + 0 became MOV x
    for (IRInsn *i = fn->head; i; i = i->next) {
        if (i->dst == res1) {
            ASSERT(IR_MOV, i->kind);
            ASSERT_TRUE(i->src1 == x);
        }
    }

    // Verify strength reduction: x * 8 became x << 3
    for (IRInsn *i = fn->head; i; i = i->next) {
        if (i->dst == res2) {
            ASSERT(IR_SHL, i->kind);
            ASSERT(3, i->src2->def_insn->imm);
        }
    }
}

static void test_llir_dce(void) {
    printf("Testing LLIR Dead Code Elimination...\n");
    IRFunction *fn = new_test_ir_fn("test_dce");

    IRVReg *dead1 = add_imm(fn, 999);
    IRVReg *dead2 = add_imm(fn, 888);
    add_bin(fn, IR_ADD, dead1, dead2);

    IRVReg *live = add_imm(fn, 1);
    IRInsn *ret = ir_new_insn(IR_RET);
    ret->src1 = live;
    ir_append_insn(fn, ret);

    ASSERT_TRUE(ir_opt_dce(fn));

    // Dead instructions removed, only live and ret remain
    int count = 0;
    for (IRInsn *i = fn->head; i; i = i->next) count++;
    ASSERT(2, count);
}

static void test_base_index_reg_validation(void) {
    printf("Testing base_reg & index_reg IR validation...\n");
    IRFunction *fn = new_test_ir_fn("test_base_index_validate");

    IRVReg *base = add_imm(fn, 100);
    IRVReg *idx = add_imm(fn, 8);
    IRVReg *dst = ir_new_vreg(fn, ty_long);

    IRInsn *load = ir_new_insn(IR_LOAD);
    load->dst = dst;
    load->base_reg = base;
    load->index_reg = idx;
    ir_append_insn(fn, load);

    char *err = NULL;
    ASSERT_TRUE(ir_verify_function(fn, &err));

    // Test out of bounds base_reg
    IRVReg invalid_vreg;
    memset(&invalid_vreg, 0, sizeof(invalid_vreg));
    invalid_vreg.id = 9999;
    load->base_reg = &invalid_vreg;
    ASSERT_TRUE(!ir_verify_function(fn, &err));
    load->base_reg = base;

    // Test out of bounds index_reg
    load->index_reg = &invalid_vreg;
    ASSERT_TRUE(!ir_verify_function(fn, &err));
    load->index_reg = idx;
    ASSERT_TRUE(ir_verify_function(fn, &err));
}

static void test_base_index_reg_liveness(void) {
    printf("Testing base_reg & index_reg liveness analysis...\n");
    IRFunction *fn = new_test_ir_fn("test_base_index_liveness");

    IRVReg *base = add_imm(fn, 100);
    IRVReg *idx = add_imm(fn, 8);
    IRVReg *val = add_imm(fn, 42);

    IRInsn *call = ir_new_insn(IR_CALL);
    call->label = "dummy_func";
    ir_append_insn(fn, call);

    IRInsn *store = ir_new_insn(IR_STORE);
    store->src1 = val;
    store->base_reg = base;
    store->index_reg = idx;
    ir_append_insn(fn, store);

    IRInsn *ret = ir_new_insn(IR_RET);
    ir_append_insn(fn, ret);

    ir_renumber_insns(fn);

    IRLiveness *liv = ir_compute_liveness(fn);
    ASSERT_TRUE(liv != NULL);
    // Both base and index must be live across the call because their use is after the call
    ASSERT_TRUE(liv->is_live_across_calls[base->id]);
    ASSERT_TRUE(liv->is_live_across_calls[idx->id]);
    ASSERT_TRUE(liv->is_live_across_calls[val->id]);

    ASSERT(store->pos, liv->last_use_pos[base->id]);
    ASSERT(store->pos, liv->last_use_pos[idx->id]);
    ASSERT(store->pos, liv->last_use_pos[val->id]);

    ir_free_liveness(liv);
}

static void test_base_index_reg_dce(void) {
    printf("Testing base_reg & index_reg Dead Code Elimination...\n");
    IRFunction *fn = new_test_ir_fn("test_base_index_dce");

    IRVReg *dead = add_imm(fn, 999);
    IRVReg *base = add_imm(fn, 100);
    IRVReg *idx = add_imm(fn, 8);
    IRVReg *val = add_imm(fn, 42);

    IRInsn *store = ir_new_insn(IR_STORE);
    store->src1 = val;
    store->base_reg = base;
    store->index_reg = idx;
    ir_append_insn(fn, store);

    IRInsn *ret = ir_new_insn(IR_RET);
    ir_append_insn(fn, ret);

    (void)dead;

    ASSERT_TRUE(ir_opt_dce(fn));

    // Ensure base and index defs were NOT eliminated, but dead was
    bool has_dead = false;
    bool has_base = false;
    bool has_idx = false;
    bool has_val = false;

    for (IRInsn *i = fn->head; i; i = i->next) {
        if (i->dst == dead) has_dead = true;
        if (i->dst == base) has_base = true;
        if (i->dst == idx) has_idx = true;
        if (i->dst == val) has_val = true;
    }

    ASSERT_TRUE(!has_dead);
    ASSERT_TRUE(has_base);
    ASSERT_TRUE(has_idx);
    ASSERT_TRUE(has_val);
}

static void test_base_index_reg_copy_prop(void) {
    printf("Testing base_reg & index_reg copy propagation...\n");
    IRFunction *fn = new_test_ir_fn("test_base_index_copy_prop");

    IRVReg *orig_base = add_imm(fn, 100);
    IRVReg *orig_idx = add_imm(fn, 8);

    IRVReg *alias_base = ir_new_vreg(fn, ty_long);
    IRInsn *mov_base = ir_new_insn(IR_MOV);
    mov_base->dst = alias_base;
    mov_base->src1 = orig_base;
    alias_base->def_insn = mov_base;
    ir_append_insn(fn, mov_base);

    IRVReg *alias_idx = ir_new_vreg(fn, ty_long);
    IRInsn *mov_idx = ir_new_insn(IR_MOV);
    mov_idx->dst = alias_idx;
    mov_idx->src1 = orig_idx;
    alias_idx->def_insn = mov_idx;
    ir_append_insn(fn, mov_idx);

    IRVReg *dst = ir_new_vreg(fn, ty_long);
    IRInsn *load = ir_new_insn(IR_LOAD);
    load->dst = dst;
    load->base_reg = alias_base;
    load->index_reg = alias_idx;
    ir_append_insn(fn, load);

    IRInsn *ret = ir_new_insn(IR_RET);
    ret->src1 = dst;
    ir_append_insn(fn, ret);

    ASSERT_TRUE(ir_opt_copy_prop(fn));

    ASSERT_TRUE(load->base_reg == orig_base);
    ASSERT_TRUE(load->index_reg == orig_idx);
}

static void test_base_index_regalloc(void) {
    printf("Testing base_reg & index_reg register allocation...\n");
    IRFunction *fn = new_test_ir_fn("test_base_index_regalloc");

    static const int gp_regs[] = { 10, 11, 12, 13 };
    static const int scratch_gp_regs[] = { 14, 15 };
    RegAllocPool pool = {
        .num_gp_regs = 4,
        .gp_regs = gp_regs,
        .num_scratch_gp_regs = 2,
        .scratch_gp_regs = scratch_gp_regs,
        .num_fp_regs = 0,
        .fp_regs = NULL,
        .spill_base_offset = -16,
        .spill_align = 8,
    };

    IRVReg *base = add_imm(fn, 100);
    IRVReg *idx = add_imm(fn, 8);
    IRVReg *val = add_imm(fn, 42);

    IRInsn *store = ir_new_insn(IR_STORE);
    store->src1 = val;
    store->base_reg = base;
    store->index_reg = idx;
    ir_append_insn(fn, store);

    IRInsn *ret = ir_new_insn(IR_RET);
    ir_append_insn(fn, ret);

    regalloc_function(fn, &pool);

    // Verify intervals were extended to the store instruction
    ASSERT(store->pos, base->last_use_pos);
    ASSERT(store->pos, idx->last_use_pos);
    ASSERT(store->pos, val->last_use_pos);

    // base, idx, val are all simultaneously live at the store instruction, so if assigned physical regs, they must all be distinct
    if (base->phys_reg != -1 && idx->phys_reg != -1) {
        ASSERT_TRUE(base->phys_reg != idx->phys_reg);
    }
    if (base->phys_reg != -1 && val->phys_reg != -1) {
        ASSERT_TRUE(base->phys_reg != val->phys_reg);
    }
    if (idx->phys_reg != -1 && val->phys_reg != -1) {
        ASSERT_TRUE(idx->phys_reg != val->phys_reg);
    }
}

int main(void) {
    printf("========================================\n");
    printf("   LLIR Optimization & Analysis Tests   \n");
    printf("========================================\n");

    test_cfg_and_dom_tree();
    test_liveness_analysis();
    test_pass_manager_and_registry();
    test_llir_peephole_and_algebraic();
    test_llir_dce();
    test_base_index_reg_validation();
    test_base_index_reg_liveness();
    test_base_index_reg_dce();
    test_base_index_reg_copy_prop();
    test_base_index_regalloc();

    printf("\nAll LLIR unit tests passed successfully!\n");
    return 0;
}
