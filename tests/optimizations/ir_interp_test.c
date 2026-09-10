#include "test.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

// ==============================================================================
// 1. LLIR Data Structures (Self-Contained for Test Interpreter)
// ==============================================================================

typedef enum {
  LLIR_NOP = 0,
  LLIR_IMM,
  LLIR_MOV,
  LLIR_CAST,
  LLIR_ADD,
  LLIR_SUB,
  LLIR_MUL,
  LLIR_DIV,
  LLIR_MOD,
  LLIR_AND,
  LLIR_OR,
  LLIR_XOR,
  LLIR_SHL,
  LLIR_SHR,
  LLIR_NEG,
  LLIR_NOT,
  LLIR_LOGNOT,
  LLIR_CMP_EQ,
  LLIR_CMP_NE,
  LLIR_CMP_LT,
  LLIR_CMP_LE,
  LLIR_CMP_GT,
  LLIR_CMP_GE,
  LLIR_LOAD,
  LLIR_STORE,
  LLIR_LABEL,
  LLIR_JMP,
  LLIR_BR_COND,
  LLIR_RET,
  LLIR_CALL,
  LLIR_PARAM,
} TestLLIRKind;

typedef struct TestVReg TestVReg;
typedef struct TestInsn TestInsn;
typedef struct TestFunction TestFunction;

struct TestVReg {
  int id;
  int64_t val;
};

struct TestInsn {
  TestLLIRKind kind;
  TestInsn *prev;
  TestInsn *next;
  int pos;

  TestVReg *dst;
  TestVReg *src1;
  TestVReg *src2;
  int64_t imm;
  char *label;
  char *label_true;
  char *label_false;
};

#define MAX_TEST_VREGS 64
#define MAX_TEST_INSNS 128

struct TestFunction {
  char *name;
  TestInsn *head;
  TestInsn *tail;
  int num_insns;
  int num_vregs;
  TestVReg *vregs[MAX_TEST_VREGS];
  TestVReg vreg_pool[MAX_TEST_VREGS];
  TestInsn insn_pool[MAX_TEST_INSNS];
  int insn_alloc_count;
};

// ==============================================================================
// 2. IR Builder Helpers
// ==============================================================================

static TestFunction global_test_fns[16];
static int global_fn_count = 0;

static TestFunction *new_test_fn(char *name) {
  TestFunction *fn = &global_test_fns[global_fn_count++];
  memset(fn, 0, sizeof(*fn));
  fn->name = name;
  return fn;
}

static TestVReg *new_test_vreg(TestFunction *fn) {
  int id = fn->num_vregs++;
  TestVReg *v = &fn->vreg_pool[id];
  v->id = id;
  fn->vregs[id] = v;
  return v;
}

static TestInsn *new_test_insn(TestFunction *fn, TestLLIRKind kind) {
  TestInsn *insn = &fn->insn_pool[fn->insn_alloc_count++];
  insn->kind = kind;
  return insn;
}

static void append_test_insn(TestFunction *fn, TestInsn *insn) {
  if (!fn->head) {
    fn->head = fn->tail = insn;
  } else {
    fn->tail->next = insn;
    insn->prev = fn->tail;
    fn->tail = insn;
  }
  insn->pos = fn->num_insns++;
}

static void remove_test_insn(TestFunction *fn, TestInsn *insn) {
  if (insn->prev) insn->prev->next = insn->next;
  if (insn->next) insn->next->prev = insn->prev;
  if (fn->head == insn) fn->head = insn->next;
  if (fn->tail == insn) fn->tail = insn->prev;
  fn->num_insns--;
}

static int count_insns_of_kind(TestFunction *fn, TestLLIRKind kind) {
  int c = 0;
  for (TestInsn *i = fn->head; i; i = i->next)
    if (i->kind == kind)
      c++;
  return c;
}

static int count_total_insns(TestFunction *fn) {
  int c = 0;
  for (TestInsn *i = fn->head; i; i = i->next)
    c++;
  return c;
}

// ==============================================================================
// 3. LLIR Interpreter Implementation
// ==============================================================================

#define INTERP_MEM_SIZE 4096

typedef struct {
  int64_t regs[1024];
  uint8_t memory[INTERP_MEM_SIZE];
  bool has_returned;
  int64_t return_val;
} InterpState;

static TestInsn *find_label(TestFunction *fn, const char *label) {
  if (!label) return NULL;
  for (TestInsn *i = fn->head; i; i = i->next) {
    if (i->kind == LLIR_LABEL && i->label && strcmp(i->label, label) == 0)
      return i;
  }
  return NULL;
}

static int64_t interp_run(TestFunction *fn, int64_t *args, int num_args) {
  InterpState state;
  memset(&state, 0, sizeof(state));

  // Initialize arguments into first N vregs
  for (int i = 0; i < num_args && i < 1024; i++) {
    state.regs[i] = args[i];
  }

  TestInsn *pc = fn->head;
  int step_limit = 100000; // infinite loop guard

  while (pc && step_limit-- > 0) {
    switch (pc->kind) {
    case LLIR_NOP:
    case LLIR_LABEL:
      pc = pc->next;
      break;

    case LLIR_IMM:
      state.regs[pc->dst->id] = pc->imm;
      pc = pc->next;
      break;

    case LLIR_MOV:
      state.regs[pc->dst->id] = state.regs[pc->src1->id];
      pc = pc->next;
      break;

    case LLIR_CAST:
      state.regs[pc->dst->id] = state.regs[pc->src1->id];
      pc = pc->next;
      break;

    case LLIR_ADD:
      state.regs[pc->dst->id] = state.regs[pc->src1->id] + state.regs[pc->src2->id];
      pc = pc->next;
      break;

    case LLIR_SUB:
      state.regs[pc->dst->id] = state.regs[pc->src1->id] - state.regs[pc->src2->id];
      pc = pc->next;
      break;

    case LLIR_MUL:
      state.regs[pc->dst->id] = state.regs[pc->src1->id] * state.regs[pc->src2->id];
      pc = pc->next;
      break;

    case LLIR_DIV:
      state.regs[pc->dst->id] = state.regs[pc->src1->id] / state.regs[pc->src2->id];
      pc = pc->next;
      break;

    case LLIR_MOD:
      state.regs[pc->dst->id] = state.regs[pc->src1->id] % state.regs[pc->src2->id];
      pc = pc->next;
      break;

    case LLIR_AND:
      state.regs[pc->dst->id] = state.regs[pc->src1->id] & state.regs[pc->src2->id];
      pc = pc->next;
      break;

    case LLIR_OR:
      state.regs[pc->dst->id] = state.regs[pc->src1->id] | state.regs[pc->src2->id];
      pc = pc->next;
      break;

    case LLIR_XOR:
      state.regs[pc->dst->id] = state.regs[pc->src1->id] ^ state.regs[pc->src2->id];
      pc = pc->next;
      break;

    case LLIR_SHL:
      state.regs[pc->dst->id] = state.regs[pc->src1->id] << state.regs[pc->src2->id];
      pc = pc->next;
      break;

    case LLIR_SHR:
      state.regs[pc->dst->id] = (uint64_t)state.regs[pc->src1->id] >> state.regs[pc->src2->id];
      pc = pc->next;
      break;

    case LLIR_NEG:
      state.regs[pc->dst->id] = -state.regs[pc->src1->id];
      pc = pc->next;
      break;

    case LLIR_NOT:
      state.regs[pc->dst->id] = ~state.regs[pc->src1->id];
      pc = pc->next;
      break;

    case LLIR_LOGNOT:
      state.regs[pc->dst->id] = !state.regs[pc->src1->id];
      pc = pc->next;
      break;

    case LLIR_CMP_EQ:
      state.regs[pc->dst->id] = (state.regs[pc->src1->id] == state.regs[pc->src2->id]);
      pc = pc->next;
      break;

    case LLIR_CMP_NE:
      state.regs[pc->dst->id] = (state.regs[pc->src1->id] != state.regs[pc->src2->id]);
      pc = pc->next;
      break;

    case LLIR_CMP_LT:
      state.regs[pc->dst->id] = (state.regs[pc->src1->id] < state.regs[pc->src2->id]);
      pc = pc->next;
      break;

    case LLIR_CMP_LE:
      state.regs[pc->dst->id] = (state.regs[pc->src1->id] <= state.regs[pc->src2->id]);
      pc = pc->next;
      break;

    case LLIR_CMP_GT:
      state.regs[pc->dst->id] = (state.regs[pc->src1->id] > state.regs[pc->src2->id]);
      pc = pc->next;
      break;

    case LLIR_CMP_GE:
      state.regs[pc->dst->id] = (state.regs[pc->src1->id] >= state.regs[pc->src2->id]);
      pc = pc->next;
      break;

    case LLIR_LOAD: {
      uint64_t addr = (uint64_t)state.regs[pc->src1->id];
      if (addr + 8 <= INTERP_MEM_SIZE) {
        int64_t val = 0;
        memcpy(&val, &state.memory[addr], 8);
        state.regs[pc->dst->id] = val;
      }
      pc = pc->next;
      break;
    }

    case LLIR_STORE: {
      uint64_t addr = (uint64_t)state.regs[pc->src1->id];
      if (addr + 8 <= INTERP_MEM_SIZE) {
        int64_t val = state.regs[pc->src2->id];
        memcpy(&state.memory[addr], &val, 8);
      }
      pc = pc->next;
      break;
    }

    case LLIR_JMP:
      pc = find_label(fn, pc->label);
      break;

    case LLIR_BR_COND:
      if (state.regs[pc->src1->id]) {
        pc = find_label(fn, pc->label_true);
      } else {
        pc = find_label(fn, pc->label_false);
      }
      break;

    case LLIR_RET:
      if (pc->src1) {
        state.return_val = state.regs[pc->src1->id];
      }
      state.has_returned = true;
      return state.return_val;

    default:
      pc = pc->next;
      break;
    }
  }

  return state.return_val;
}

// ==============================================================================
// 4. Exact Optimizer Passes over Test LLIR
// ==============================================================================

static bool opt_const_fold(TestFunction *fn) {
  bool changed = false;
  bool *is_const = calloc(fn->num_vregs, sizeof(bool));
  int64_t *vals = calloc(fn->num_vregs, sizeof(int64_t));

  for (TestInsn *i = fn->head; i; i = i->next) {
    if (i->kind == LLIR_IMM && i->dst) {
      is_const[i->dst->id] = true;
      vals[i->dst->id] = i->imm;
    } else if (i->src1 && is_const[i->src1->id] && i->src2 && is_const[i->src2->id] && i->dst) {
      int64_t c1 = vals[i->src1->id];
      int64_t c2 = vals[i->src2->id];
      int64_t res = 0;
      bool folded = true;

      switch (i->kind) {
      case LLIR_ADD: res = c1 + c2; break;
      case LLIR_SUB: res = c1 - c2; break;
      case LLIR_MUL: res = c1 * c2; break;
      case LLIR_DIV: if (c2 != 0) res = c1 / c2; else folded = false; break;
      case LLIR_AND: res = c1 & c2; break;
      case LLIR_OR:  res = c1 | c2; break;
      case LLIR_XOR: res = c1 ^ c2; break;
      case LLIR_SHL: res = c1 << c2; break;
      case LLIR_SHR: res = (uint64_t)c1 >> c2; break;
      case LLIR_CMP_EQ: res = (c1 == c2); break;
      case LLIR_CMP_NE: res = (c1 != c2); break;
      case LLIR_CMP_LT: res = (c1 < c2); break;
      case LLIR_CMP_LE: res = (c1 <= c2); break;
      case LLIR_CMP_GT: res = (c1 > c2); break;
      case LLIR_CMP_GE: res = (c1 >= c2); break;
      default: folded = false; break;
      }

      if (folded) {
        i->kind = LLIR_IMM;
        i->imm = res;
        i->src1 = i->src2 = NULL;
        is_const[i->dst->id] = true;
        vals[i->dst->id] = res;
        changed = true;
      }
    } else if (i->src1 && is_const[i->src1->id] && !i->src2 && i->dst) {
      int64_t c = vals[i->src1->id];
      int64_t res = 0;
      bool folded = true;

      switch (i->kind) {
      case LLIR_NEG: res = -c; break;
      case LLIR_NOT: res = ~c; break;
      case LLIR_LOGNOT: res = !c; break;
      default: folded = false; break;
      }

      if (folded) {
        i->kind = LLIR_IMM;
        i->imm = res;
        i->src1 = NULL;
        is_const[i->dst->id] = true;
        vals[i->dst->id] = res;
        changed = true;
      }
    }
  }

  free(is_const);
  free(vals);
  return changed;
}

static bool opt_dce(TestFunction *fn) {
  bool overall_changed = false;
  bool changed = true;
  bool *used = calloc(fn->num_vregs, sizeof(bool));

  while (changed) {
    changed = false;
    memset(used, 0, fn->num_vregs * sizeof(bool));

    for (TestInsn *i = fn->head; i; i = i->next) {
      if (i->src1) used[i->src1->id] = true;
      if (i->src2) used[i->src2->id] = true;
    }

    for (TestInsn *i = fn->head; i;) {
      TestInsn *next = i->next;
      // Do not eliminate instructions with memory side effects or control flow
      if (i->dst && !used[i->dst->id] && i->kind != LLIR_STORE && i->kind != LLIR_CALL && i->kind != LLIR_RET && i->kind != LLIR_JMP && i->kind != LLIR_BR_COND) {
        remove_test_insn(fn, i);
        changed = true;
        overall_changed = true;
      }
      i = next;
    }
  }

  free(used);
  return overall_changed;
}

static bool opt_copy_prop(TestFunction *fn) {
  bool changed = false;
  TestVReg **aliases = calloc(fn->num_vregs, sizeof(TestVReg *));

  for (TestInsn *i = fn->head; i; i = i->next) {
    if (i->kind == LLIR_LABEL || i->kind == LLIR_BR_COND || i->kind == LLIR_JMP) {
      memset(aliases, 0, fn->num_vregs * sizeof(TestVReg *));
      continue;
    }
    if (i->src1 && aliases[i->src1->id]) {
      i->src1 = aliases[i->src1->id];
      changed = true;
    }
    if (i->src2 && aliases[i->src2->id]) {
      i->src2 = aliases[i->src2->id];
      changed = true;
    }
    if (i->kind == LLIR_MOV && i->dst && i->src1) {
      TestVReg *root = i->src1;
      while (aliases[root->id])
        root = aliases[root->id];
      if (root != i->dst)
        aliases[i->dst->id] = root;
    }
  }

  free(aliases);
  return changed;
}

static bool opt_peephole(TestFunction *fn) {
  bool changed = false;

  for (TestInsn *i = fn->head; i; i = i->next) {
    // 1. Redundant self-move: dst = mov dst
    if (i->kind == LLIR_MOV && i->dst && i->src1 && i->dst == i->src1) {
      TestInsn *del = i;
      i = i->prev ? i->prev : fn->head;
      remove_test_insn(fn, del);
      changed = true;
      if (!i) break;
      continue;
    }

    // 2. Self cancellation: sub x, x -> 0, xor x, x -> 0
    if ((i->kind == LLIR_SUB || i->kind == LLIR_XOR) && i->dst && i->src1 && i->src2 && i->src1 == i->src2) {
      i->kind = LLIR_IMM;
      i->imm = 0;
      i->src1 = i->src2 = NULL;
      changed = true;
      continue;
    }

    // 3. Multiplication by power of 2 -> shift
    if (i->kind == LLIR_MUL && i->src2) {
      for (TestInsn *def = fn->head; def != i; def = def->next) {
        if (def->kind == LLIR_IMM && def->dst == i->src2) {
          int64_t val = def->imm;
          if (val > 0 && (val & (val - 1)) == 0) {
            int shift = 0;
            while ((1LL << shift) < val) shift++;
            if ((1LL << shift) == val) {
              TestVReg *shift_v = new_test_vreg(fn);
              TestInsn *imm_i = new_test_insn(fn, LLIR_IMM);
              imm_i->dst = shift_v;
              imm_i->imm = shift;
              // insert before
              imm_i->next = i;
              imm_i->prev = i->prev;
              if (i->prev) i->prev->next = imm_i;
              else fn->head = imm_i;
              i->prev = imm_i;
              fn->num_insns++;

              i->kind = LLIR_SHL;
              i->src2 = shift_v;
              changed = true;
            }
          }
          break;
        }
      }
    }
  }

  return changed;
}

static void run_all_optimizations(TestFunction *fn) {
  for (int iter = 0; iter < 5; iter++) {
    bool changed = false;
    changed |= opt_const_fold(fn);
    changed |= opt_copy_prop(fn);
    changed |= opt_peephole(fn);
    changed |= opt_dce(fn);
    if (!changed)
      break;
  }
}

// ==============================================================================
// 5. Test Suite: Verification of Transformations and Interpreter Evaluation
// ==============================================================================

// Test 1: Constant folding reduces arithmetic into imm and evaluates identically
static void test_eval_constant_folding(void) {
  TestFunction *fn = new_test_fn("test_cf");
  TestVReg *v0 = new_test_vreg(fn); // imm 10
  TestVReg *v1 = new_test_vreg(fn); // imm 20
  TestVReg *v2 = new_test_vreg(fn); // v0 + v1 = 30
  TestVReg *v3 = new_test_vreg(fn); // imm 2
  TestVReg *v4 = new_test_vreg(fn); // v2 * v3 = 60

  TestInsn *i0 = new_test_insn(fn, LLIR_IMM); i0->dst = v0; i0->imm = 10; append_test_insn(fn, i0);
  TestInsn *i1 = new_test_insn(fn, LLIR_IMM); i1->dst = v1; i1->imm = 20; append_test_insn(fn, i1);
  TestInsn *i2 = new_test_insn(fn, LLIR_ADD); i2->dst = v2; i2->src1 = v0; i2->src2 = v1; append_test_insn(fn, i2);
  TestInsn *i3 = new_test_insn(fn, LLIR_IMM); i3->dst = v3; i3->imm = 2; append_test_insn(fn, i3);
  TestInsn *i4 = new_test_insn(fn, LLIR_MUL); i4->dst = v4; i4->src1 = v2; i4->src2 = v3; append_test_insn(fn, i4);
  TestInsn *ret = new_test_insn(fn, LLIR_RET); ret->src1 = v4; append_test_insn(fn, ret);

  // 1. Evaluate unoptimized
  int64_t unopt_res = interp_run(fn, NULL, 0);
  ASSERT(60, unopt_res);

  int add_count_before = count_insns_of_kind(fn, LLIR_ADD);
  int mul_count_before = count_insns_of_kind(fn, LLIR_MUL);
  ASSERT(1, add_count_before);
  ASSERT(1, mul_count_before);

  // 2. Run Constant Folding & DCE
  opt_const_fold(fn);
  opt_dce(fn);

  // Verify bytecode was actually transformed (ADD and MUL eliminated)
  int add_count_after = count_insns_of_kind(fn, LLIR_ADD);
  int mul_count_after = count_insns_of_kind(fn, LLIR_MUL);
  ASSERT(0, add_count_after);
  ASSERT(0, mul_count_after);

  // 3. Evaluate optimized
  int64_t opt_res = interp_run(fn, NULL, 0);
  ASSERT(60, opt_res);
}

// Test 2: Dead Code Elimination removes unused computation
static void test_eval_dead_code_elimination(void) {
  TestFunction *fn = new_test_fn("test_dce");
  TestVReg *v_live = new_test_vreg(fn);
  TestVReg *v_dead1 = new_test_vreg(fn);
  TestVReg *v_dead2 = new_test_vreg(fn);

  TestInsn *i_live = new_test_insn(fn, LLIR_IMM); i_live->dst = v_live; i_live->imm = 42; append_test_insn(fn, i_live);
  TestInsn *i_dead1 = new_test_insn(fn, LLIR_IMM); i_dead1->dst = v_dead1; i_dead1->imm = 999; append_test_insn(fn, i_dead1);
  TestInsn *i_dead2 = new_test_insn(fn, LLIR_ADD); i_dead2->dst = v_dead2; i_dead2->src1 = v_dead1; i_dead2->src2 = v_dead1; append_test_insn(fn, i_dead2);
  TestInsn *ret = new_test_insn(fn, LLIR_RET); ret->src1 = v_live; append_test_insn(fn, ret);

  int total_before = count_total_insns(fn);
  ASSERT(4, total_before);

  int64_t unopt_res = interp_run(fn, NULL, 0);
  ASSERT(42, unopt_res);

  // Optimize: DCE should eliminate i_dead1 and i_dead2
  opt_dce(fn);

  int total_after = count_total_insns(fn);
  ASSERT(2, total_after); // Only i_live and ret remain

  int64_t opt_res = interp_run(fn, NULL, 0);
  ASSERT(42, opt_res);
}

// Test 3: Strength Reduction converts MUL 16 to SHL 4
static void test_eval_strength_reduction(void) {
  TestFunction *fn = new_test_fn("test_sr");
  TestVReg *v_arg = new_test_vreg(fn); // v0 (arg)
  TestVReg *v_pow2 = new_test_vreg(fn); // imm 16
  TestVReg *v_res = new_test_vreg(fn); // v0 * 16

  TestInsn *i_imm = new_test_insn(fn, LLIR_IMM); i_imm->dst = v_pow2; i_imm->imm = 16; append_test_insn(fn, i_imm);
  TestInsn *i_mul = new_test_insn(fn, LLIR_MUL); i_mul->dst = v_res; i_mul->src1 = v_arg; i_mul->src2 = v_pow2; append_test_insn(fn, i_mul);
  TestInsn *ret = new_test_insn(fn, LLIR_RET); ret->src1 = v_res; append_test_insn(fn, ret);

  int64_t arg_val = 5;
  int64_t unopt_res = interp_run(fn, &arg_val, 1);
  ASSERT(80, unopt_res);

  opt_peephole(fn);
  opt_dce(fn);

  // Verify MUL is replaced by SHL
  ASSERT(0, count_insns_of_kind(fn, LLIR_MUL));
  ASSERT(1, count_insns_of_kind(fn, LLIR_SHL));

  int64_t opt_res = interp_run(fn, &arg_val, 1);
  ASSERT(80, opt_res);
}

// Test 4: Self Cancellation (x - x -> 0)
static void test_eval_self_cancellation(void) {
  TestFunction *fn = new_test_fn("test_cancel");
  TestVReg *v_arg = new_test_vreg(fn);
  TestVReg *v_res = new_test_vreg(fn);

  TestInsn *i_sub = new_test_insn(fn, LLIR_SUB); i_sub->dst = v_res; i_sub->src1 = v_arg; i_sub->src2 = v_arg; append_test_insn(fn, i_sub);
  TestInsn *ret = new_test_insn(fn, LLIR_RET); ret->src1 = v_res; append_test_insn(fn, ret);

  int64_t arg_val = 12345;
  int64_t unopt_res = interp_run(fn, &arg_val, 1);
  ASSERT(0, unopt_res);

  opt_peephole(fn);

  ASSERT(0, count_insns_of_kind(fn, LLIR_SUB));
  ASSERT(1, count_insns_of_kind(fn, LLIR_IMM));

  int64_t opt_res = interp_run(fn, &arg_val, 1);
  ASSERT(0, opt_res);
}

// Test 5: Copy Propagation eliminates intermediate copies
static void test_eval_copy_propagation(void) {
  TestFunction *fn = new_test_fn("test_cp");
  TestVReg *v_arg = new_test_vreg(fn);
  TestVReg *v_copy1 = new_test_vreg(fn);
  TestVReg *v_copy2 = new_test_vreg(fn);
  TestVReg *v_five = new_test_vreg(fn);
  TestVReg *v_res = new_test_vreg(fn);

  TestInsn *i_c1 = new_test_insn(fn, LLIR_MOV); i_c1->dst = v_copy1; i_c1->src1 = v_arg; append_test_insn(fn, i_c1);
  TestInsn *i_c2 = new_test_insn(fn, LLIR_MOV); i_c2->dst = v_copy2; i_c2->src1 = v_copy1; append_test_insn(fn, i_c2);
  TestInsn *i_imm = new_test_insn(fn, LLIR_IMM); i_imm->dst = v_five; i_imm->imm = 5; append_test_insn(fn, i_imm);
  TestInsn *i_add = new_test_insn(fn, LLIR_ADD); i_add->dst = v_res; i_add->src1 = v_copy2; i_add->src2 = v_five; append_test_insn(fn, i_add);
  TestInsn *ret = new_test_insn(fn, LLIR_RET); ret->src1 = v_res; append_test_insn(fn, ret);

  int64_t arg_val = 20;
  int64_t unopt_res = interp_run(fn, &arg_val, 1);
  ASSERT(25, unopt_res);

  opt_copy_prop(fn);
  opt_dce(fn);

  // Copies should be dead and removed
  ASSERT(0, count_insns_of_kind(fn, LLIR_MOV));

  int64_t opt_res = interp_run(fn, &arg_val, 1);
  ASSERT(25, opt_res);
}

// Test 6: Memory Load / Store round-trip in interpreter
static void test_eval_memory_operations(void) {
  TestFunction *fn = new_test_fn("test_mem");
  TestVReg *v_addr = new_test_vreg(fn);
  TestVReg *v_val = new_test_vreg(fn);
  TestVReg *v_read = new_test_vreg(fn);

  TestInsn *i_addr = new_test_insn(fn, LLIR_IMM); i_addr->dst = v_addr; i_addr->imm = 64; append_test_insn(fn, i_addr);
  TestInsn *i_val = new_test_insn(fn, LLIR_IMM); i_val->dst = v_val; i_val->imm = 12345678; append_test_insn(fn, i_val);
  TestInsn *i_st = new_test_insn(fn, LLIR_STORE); i_st->src1 = v_addr; i_st->src2 = v_val; append_test_insn(fn, i_st);
  TestInsn *i_ld = new_test_insn(fn, LLIR_LOAD); i_ld->dst = v_read; i_ld->src1 = v_addr; append_test_insn(fn, i_ld);
  TestInsn *ret = new_test_insn(fn, LLIR_RET); ret->src1 = v_read; append_test_insn(fn, ret);

  int64_t res = interp_run(fn, NULL, 0);
  ASSERT(12345678, res);
}

int main(void) {
  test_eval_constant_folding();
  test_eval_dead_code_elimination();
  test_eval_strength_reduction();
  test_eval_self_cancellation();
  test_eval_copy_propagation();
  test_eval_memory_operations();

  printf("OK\n");
  return 0;
}
