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
    } else if (i->src1 && is_const[i->src1->id] && i->src2 && !is_const[i->src2->id] && i->dst) {
      // Canonicalize constants to src2
      if (i->kind == LLIR_ADD || i->kind == LLIR_MUL ||
          i->kind == LLIR_AND || i->kind == LLIR_OR || i->kind == LLIR_XOR ||
          i->kind == LLIR_CMP_EQ || i->kind == LLIR_CMP_NE) {
        TestVReg *tmp = i->src1;
        i->src1 = i->src2;
        i->src2 = tmp;
        changed = true;
      } else if (i->kind == LLIR_CMP_LT) {
        i->kind = LLIR_CMP_GT;
        TestVReg *tmp = i->src1;
        i->src1 = i->src2;
        i->src2 = tmp;
        changed = true;
      } else if (i->kind == LLIR_CMP_LE) {
        i->kind = LLIR_CMP_GE;
        TestVReg *tmp = i->src1;
        i->src1 = i->src2;
        i->src2 = tmp;
        changed = true;
      } else if (i->kind == LLIR_CMP_GT) {
        i->kind = LLIR_CMP_LT;
        TestVReg *tmp = i->src1;
        i->src1 = i->src2;
        i->src2 = tmp;
        changed = true;
      } else if (i->kind == LLIR_CMP_GE) {
        i->kind = LLIR_CMP_LE;
        TestVReg *tmp = i->src1;
        i->src1 = i->src2;
        i->src2 = tmp;
        changed = true;
      }
    }
  }

  free(is_const);
  free(vals);
  return changed;
}

static bool opt_cfg_simplify(TestFunction *fn) {
  bool changed = false;
  bool *is_const = calloc(fn->num_vregs, sizeof(bool));
  int64_t *vals = calloc(fn->num_vregs, sizeof(int64_t));

  for (TestInsn *i = fn->head; i; i = i->next) {
    if (i->kind == LLIR_IMM && i->dst) {
      is_const[i->dst->id] = true;
      vals[i->dst->id] = i->imm;
    } else if (i->kind == LLIR_BR_COND && i->src1 && is_const[i->src1->id]) {
      int64_t c = vals[i->src1->id];
      if (c != 0) {
        if (i->label_true) {
          i->kind = LLIR_JMP;
          i->label = i->label_true;
          i->label_true = i->label_false = NULL;
          i->src1 = NULL;
          changed = true;
        } else {
          TestInsn *del = i;
          i = i->prev ? i->prev : fn->head;
          remove_test_insn(fn, del);
          changed = true;
          if (!i) break;
        }
      } else {
        if (i->label_false) {
          i->kind = LLIR_JMP;
          i->label = i->label_false;
          i->label_true = i->label_false = NULL;
          i->src1 = NULL;
          changed = true;
        } else {
          TestInsn *del = i;
          i = i->prev ? i->prev : fn->head;
          remove_test_insn(fn, del);
          changed = true;
          if (!i) break;
        }
      }
    }
  }

  // Eliminate unreachable instructions following unconditional jumps / returns
  for (TestInsn *i = fn->head; i; i = i->next) {
    if (i->kind == LLIR_JMP || i->kind == LLIR_RET) {
      while (i->next && i->next->kind != LLIR_LABEL) {
        TestInsn *del = i->next;
        remove_test_insn(fn, del);
        changed = true;
      }
    }
  }

  // Eliminate dead labels and unreferenced blocks preceded by unconditional jumps
  bool *label_targeted = calloc(MAX_TEST_INSNS, sizeof(bool));
  for (TestInsn *i = fn->head; i; i = i->next) {
    if (i->kind == LLIR_JMP && i->label) {
      TestInsn *target = find_label(fn, i->label);
      if (target && target->pos < MAX_TEST_INSNS) label_targeted[target->pos] = true;
    } else if (i->kind == LLIR_BR_COND) {
      if (i->label_true) {
        TestInsn *target = find_label(fn, i->label_true);
        if (target && target->pos < MAX_TEST_INSNS) label_targeted[target->pos] = true;
      }
      if (i->label_false) {
        TestInsn *target = find_label(fn, i->label_false);
        if (target && target->pos < MAX_TEST_INSNS) label_targeted[target->pos] = true;
      }
    }
  }

  for (TestInsn *i = fn->head; i;) {
    TestInsn *next = i->next;
    if (i->kind == LLIR_LABEL && i->pos < MAX_TEST_INSNS && !label_targeted[i->pos]) {
      if (i->prev && (i->prev->kind == LLIR_JMP || i->prev->kind == LLIR_RET)) {
        TestInsn *cur = i;
        while (cur && (cur == i || cur->kind != LLIR_LABEL)) {
          TestInsn *to_del = cur;
          cur = cur->next;
          remove_test_insn(fn, to_del);
          changed = true;
        }
        next = cur;
      }
    }
    i = next;
  }
  free(label_targeted);

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

    // 4. Division by power of 2 -> shift right
    if (i->kind == LLIR_DIV && i->src2) {
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
              imm_i->next = i;
              imm_i->prev = i->prev;
              if (i->prev) i->prev->next = imm_i;
              else fn->head = imm_i;
              i->prev = imm_i;
              fn->num_insns++;

              i->kind = LLIR_SHR;
              i->src2 = shift_v;
              changed = true;
            }
          }
          break;
        }
      }
    }

    // 5. Modulo by power of 2 -> bitwise and (val - 1)
    if (i->kind == LLIR_MOD && i->src2) {
      for (TestInsn *def = fn->head; def != i; def = def->next) {
        if (def->kind == LLIR_IMM && def->dst == i->src2) {
          int64_t val = def->imm;
          if (val > 0 && (val & (val - 1)) == 0) {
            TestVReg *mask_v = new_test_vreg(fn);
            TestInsn *imm_i = new_test_insn(fn, LLIR_IMM);
            imm_i->dst = mask_v;
            imm_i->imm = val - 1;
            imm_i->next = i;
            imm_i->prev = i->prev;
            if (i->prev) i->prev->next = imm_i;
            else fn->head = imm_i;
            i->prev = imm_i;
            fn->num_insns++;

            i->kind = LLIR_AND;
            i->src2 = mask_v;
            changed = true;
          }
          break;
        }
      }
    }

    // 6. Algebraic identities with 0 and 1
    if (i->kind == LLIR_ADD && i->src1 && i->src2) {
      for (TestInsn *def = fn->head; def != i; def = def->next) {
        if (def->kind == LLIR_IMM && def->dst == i->src2 && def->imm == 0) {
          i->kind = LLIR_MOV;
          i->src2 = NULL;
          changed = true;
          break;
        } else if (def->kind == LLIR_IMM && def->dst == i->src1 && def->imm == 0) {
          i->kind = LLIR_MOV;
          i->src1 = i->src2;
          i->src2 = NULL;
          changed = true;
          break;
        }
      }
    } else if (i->kind == LLIR_SUB && i->src1 && i->src2) {
      if (i->src1 == i->src2) {
        i->kind = LLIR_IMM;
        i->imm = 0;
        i->src1 = i->src2 = NULL;
        changed = true;
      } else {
        for (TestInsn *def = fn->head; def != i; def = def->next) {
          if (def->kind == LLIR_IMM && def->dst == i->src2 && def->imm == 0) {
            i->kind = LLIR_MOV;
            i->src2 = NULL;
            changed = true;
            break;
          } else if (def->kind == LLIR_IMM && def->dst == i->src1 && def->imm == 0) {
            i->kind = LLIR_NEG;
            i->src1 = i->src2;
            i->src2 = NULL;
            changed = true;
            break;
          }
        }
      }
    } else if (i->kind == LLIR_MUL && i->src1 && i->src2) {
      for (TestInsn *def = fn->head; def != i; def = def->next) {
        if (def->kind == LLIR_IMM && (def->dst == i->src1 || def->dst == i->src2) && def->imm == 0) {
          i->kind = LLIR_IMM;
          i->imm = 0;
          i->src1 = i->src2 = NULL;
          changed = true;
          break;
        } else if (def->kind == LLIR_IMM && def->dst == i->src2 && def->imm == 1) {
          i->kind = LLIR_MOV;
          i->src2 = NULL;
          changed = true;
          break;
        } else if (def->kind == LLIR_IMM && def->dst == i->src1 && def->imm == 1) {
          i->kind = LLIR_MOV;
          i->src1 = i->src2;
          i->src2 = NULL;
          changed = true;
          break;
        } else if (def->kind == LLIR_IMM && def->dst == i->src2 && def->imm == -1) {
          i->kind = LLIR_NEG;
          i->src2 = NULL;
          changed = true;
          break;
        } else if (def->kind == LLIR_IMM && def->dst == i->src1 && def->imm == -1) {
          i->kind = LLIR_NEG;
          i->src1 = i->src2;
          i->src2 = NULL;
          changed = true;
          break;
        }
      }
    } else if (i->kind == LLIR_DIV && i->src1 && i->src2) {
      if (i->src1 == i->src2) {
        i->kind = LLIR_IMM;
        i->imm = 1;
        i->src1 = i->src2 = NULL;
        changed = true;
      } else {
        for (TestInsn *def = fn->head; def != i; def = def->next) {
          if (def->kind == LLIR_IMM && def->dst == i->src2 && def->imm == 1) {
            i->kind = LLIR_MOV;
            i->src2 = NULL;
            changed = true;
            break;
          }
        }
      }
    } else if (i->kind == LLIR_MOD && i->src1 && i->src2) {
      for (TestInsn *def = fn->head; def != i; def = def->next) {
        if (def->kind == LLIR_IMM && def->dst == i->src2 && def->imm == 1) {
          i->kind = LLIR_IMM;
          i->imm = 0;
          i->src1 = i->src2 = NULL;
          changed = true;
          break;
        }
      }
    } else if (i->kind == LLIR_AND && i->src1 && i->src2) {
      if (i->src1 == i->src2) {
        i->kind = LLIR_MOV;
        i->src2 = NULL;
        changed = true;
      } else {
        for (TestInsn *def = fn->head; def != i; def = def->next) {
          if (def->kind == LLIR_IMM && (def->dst == i->src1 || def->dst == i->src2) && def->imm == 0) {
            i->kind = LLIR_IMM;
            i->imm = 0;
            i->src1 = i->src2 = NULL;
            changed = true;
            break;
          } else if (def->kind == LLIR_IMM && def->dst == i->src2 && def->imm == -1) {
            i->kind = LLIR_MOV;
            i->src2 = NULL;
            changed = true;
            break;
          } else if (def->kind == LLIR_IMM && def->dst == i->src1 && def->imm == -1) {
            i->kind = LLIR_MOV;
            i->src1 = i->src2;
            i->src2 = NULL;
            changed = true;
            break;
          }
        }
      }
    } else if (i->kind == LLIR_OR && i->src1 && i->src2) {
      if (i->src1 == i->src2) {
        i->kind = LLIR_MOV;
        i->src2 = NULL;
        changed = true;
      } else {
        for (TestInsn *def = fn->head; def != i; def = def->next) {
          if (def->kind == LLIR_IMM && def->dst == i->src2 && def->imm == 0) {
            i->kind = LLIR_MOV;
            i->src2 = NULL;
            changed = true;
            break;
          } else if (def->kind == LLIR_IMM && def->dst == i->src1 && def->imm == 0) {
            i->kind = LLIR_MOV;
            i->src1 = i->src2;
            i->src2 = NULL;
            changed = true;
            break;
          } else if (def->kind == LLIR_IMM && (def->dst == i->src1 || def->dst == i->src2) && def->imm == -1) {
            i->kind = LLIR_IMM;
            i->imm = -1;
            i->src1 = i->src2 = NULL;
            changed = true;
            break;
          }
        }
      }
    } else if (i->kind == LLIR_XOR && i->src1 && i->src2) {
      if (i->src1 == i->src2) {
        i->kind = LLIR_IMM;
        i->imm = 0;
        i->src1 = i->src2 = NULL;
        changed = true;
      } else {
        for (TestInsn *def = fn->head; def != i; def = def->next) {
          if (def->kind == LLIR_IMM && def->dst == i->src2 && def->imm == 0) {
            i->kind = LLIR_MOV;
            i->src2 = NULL;
            changed = true;
            break;
          } else if (def->kind == LLIR_IMM && def->dst == i->src1 && def->imm == 0) {
            i->kind = LLIR_MOV;
            i->src1 = i->src2;
            i->src2 = NULL;
            changed = true;
            break;
          } else if (def->kind == LLIR_IMM && def->dst == i->src2 && def->imm == -1) {
            i->kind = LLIR_NOT;
            i->src2 = NULL;
            changed = true;
            break;
          } else if (def->kind == LLIR_IMM && def->dst == i->src1 && def->imm == -1) {
            i->kind = LLIR_NOT;
            i->src1 = i->src2;
            i->src2 = NULL;
            changed = true;
            break;
          }
        }
      }
    }
  }

  return changed;
}

static void run_all_optimizations(TestFunction *fn) {
  for (int iter = 0; iter < 7; iter++) {
    bool changed = false;
    changed |= opt_const_fold(fn);
    changed |= opt_cfg_simplify(fn);
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

// Test 7: Constant Operand Canonicalization (src1 constant moved to src2, comparison inverted)
static void test_eval_constant_canonicalization(void) {
  TestFunction *fn = new_test_fn("test_canon");
  TestVReg *v_arg = new_test_vreg(fn);
  TestVReg *v_const1 = new_test_vreg(fn);
  TestVReg *v_const2 = new_test_vreg(fn);
  TestVReg *v_add_res = new_test_vreg(fn);
  TestVReg *v_cmp_res = new_test_vreg(fn);
  TestVReg *v_final = new_test_vreg(fn);

  // 10 + x (IMM in src1)
  TestInsn *i_c1 = new_test_insn(fn, LLIR_IMM); i_c1->dst = v_const1; i_c1->imm = 10; append_test_insn(fn, i_c1);
  TestInsn *i_add = new_test_insn(fn, LLIR_ADD); i_add->dst = v_add_res; i_add->src1 = v_const1; i_add->src2 = v_arg; append_test_insn(fn, i_add);

  // 15 < x (IMM in src1, should invert to x > 15)
  TestInsn *i_c2 = new_test_insn(fn, LLIR_IMM); i_c2->dst = v_const2; i_c2->imm = 15; append_test_insn(fn, i_c2);
  TestInsn *i_cmp = new_test_insn(fn, LLIR_CMP_LT); i_cmp->dst = v_cmp_res; i_cmp->src1 = v_const2; i_cmp->src2 = v_arg; append_test_insn(fn, i_cmp);

  TestInsn *i_res = new_test_insn(fn, LLIR_ADD); i_res->dst = v_final; i_res->src1 = v_add_res; i_res->src2 = v_cmp_res; append_test_insn(fn, i_res);
  TestInsn *ret = new_test_insn(fn, LLIR_RET); ret->src1 = v_final; append_test_insn(fn, ret);

  int64_t arg = 20; // 10 + 20 = 30; 15 < 20 = 1; sum = 31
  int64_t unopt_res = interp_run(fn, &arg, 1);
  ASSERT(31, unopt_res);

  // Run constant fold canonicalization
  opt_const_fold(fn);

  // Verify ADD had operands swapped (src1 becomes v_arg, src2 becomes v_const1)
  ASSERT(v_arg->id, i_add->src1->id);
  ASSERT(v_const1->id, i_add->src2->id);

  // Verify CMP_LT became CMP_GT and operands swapped
  ASSERT(LLIR_CMP_GT, i_cmp->kind);
  ASSERT(v_arg->id, i_cmp->src1->id);
  ASSERT(v_const2->id, i_cmp->src2->id);

  int64_t opt_res = interp_run(fn, &arg, 1);
  ASSERT(31, opt_res);
}

// Test 8: Unsigned Division and Modulo Strength Reduction (/ 8 -> >> 3, % 8 -> & 7)
static void test_eval_unsigned_div_mod_strength_reduction(void) {
  TestFunction *fn = new_test_fn("test_div_mod_sr");
  TestVReg *v_arg = new_test_vreg(fn);
  TestVReg *v_eight1 = new_test_vreg(fn);
  TestVReg *v_eight2 = new_test_vreg(fn);
  TestVReg *v_div_res = new_test_vreg(fn);
  TestVReg *v_mod_res = new_test_vreg(fn);
  TestVReg *v_final = new_test_vreg(fn);

  TestInsn *i_e1 = new_test_insn(fn, LLIR_IMM); i_e1->dst = v_eight1; i_e1->imm = 8; append_test_insn(fn, i_e1);
  TestInsn *i_div = new_test_insn(fn, LLIR_DIV); i_div->dst = v_div_res; i_div->src1 = v_arg; i_div->src2 = v_eight1; append_test_insn(fn, i_div);

  TestInsn *i_e2 = new_test_insn(fn, LLIR_IMM); i_e2->dst = v_eight2; i_e2->imm = 8; append_test_insn(fn, i_e2);
  TestInsn *i_mod = new_test_insn(fn, LLIR_MOD); i_mod->dst = v_mod_res; i_mod->src1 = v_arg; i_mod->src2 = v_eight2; append_test_insn(fn, i_mod);

  TestInsn *i_res = new_test_insn(fn, LLIR_ADD); i_res->dst = v_final; i_res->src1 = v_div_res; i_res->src2 = v_mod_res; append_test_insn(fn, i_res);
  TestInsn *ret = new_test_insn(fn, LLIR_RET); ret->src1 = v_final; append_test_insn(fn, ret);

  int64_t arg = 75; // 75 / 8 = 9, 75 % 8 = 3, sum = 12
  int64_t unopt_res = interp_run(fn, &arg, 1);
  ASSERT(12, unopt_res);

  // Run peephole and DCE
  opt_peephole(fn);
  opt_dce(fn);

  // Verify DIV was converted to SHR and MOD to AND
  ASSERT(0, count_insns_of_kind(fn, LLIR_DIV));
  ASSERT(0, count_insns_of_kind(fn, LLIR_MOD));
  ASSERT(1, count_insns_of_kind(fn, LLIR_SHR));
  ASSERT(1, count_insns_of_kind(fn, LLIR_AND));

  int64_t opt_res = interp_run(fn, &arg, 1);
  ASSERT(12, opt_res);
}

// Test 9: Constant Conditional Branch Folding & Dead Basic Block Elimination
static void test_eval_constant_branch_folding(void) {
  TestFunction *fn = new_test_fn("test_const_br");
  TestVReg *v_cond = new_test_vreg(fn);
  TestVReg *v_live_res = new_test_vreg(fn);
  TestVReg *v_dead_res = new_test_vreg(fn);
  TestVReg *v_phi = new_test_vreg(fn);

  TestInsn *i_cond = new_test_insn(fn, LLIR_IMM); i_cond->dst = v_cond; i_cond->imm = 1; append_test_insn(fn, i_cond);
  TestInsn *i_br = new_test_insn(fn, LLIR_BR_COND); i_br->src1 = v_cond; i_br->label_true = "taken_bb"; i_br->label_false = "dead_bb"; append_test_insn(fn, i_br);

  // Dead block
  TestInsn *l_dead = new_test_insn(fn, LLIR_LABEL); l_dead->label = "dead_bb"; append_test_insn(fn, l_dead);
  TestInsn *i_dead = new_test_insn(fn, LLIR_IMM); i_dead->dst = v_dead_res; i_dead->imm = 999; append_test_insn(fn, i_dead);
  TestInsn *i_jmp_dead = new_test_insn(fn, LLIR_JMP); i_jmp_dead->label = "join_bb"; append_test_insn(fn, i_jmp_dead);

  // Live block
  TestInsn *l_taken = new_test_insn(fn, LLIR_LABEL); l_taken->label = "taken_bb"; append_test_insn(fn, l_taken);
  TestInsn *i_live = new_test_insn(fn, LLIR_IMM); i_live->dst = v_live_res; i_live->imm = 42; append_test_insn(fn, i_live);
  TestInsn *i_mov = new_test_insn(fn, LLIR_MOV); i_mov->dst = v_phi; i_mov->src1 = v_live_res; append_test_insn(fn, i_mov);

  // Join block
  TestInsn *l_join = new_test_insn(fn, LLIR_LABEL); l_join->label = "join_bb"; append_test_insn(fn, l_join);
  TestInsn *ret = new_test_insn(fn, LLIR_RET); ret->src1 = v_phi; append_test_insn(fn, ret);

  int64_t unopt_res = interp_run(fn, NULL, 0);
  ASSERT(42, unopt_res);

  // Run CFG simplify & DCE
  opt_cfg_simplify(fn);
  opt_dce(fn);

  // Verify conditional branch is converted to unconditional JMP
  ASSERT(0, count_insns_of_kind(fn, LLIR_BR_COND));
  ASSERT(1, count_insns_of_kind(fn, LLIR_JMP));

  int64_t opt_res = interp_run(fn, NULL, 0);
  ASSERT(42, opt_res);
}

// Test 10: Algebraic Identities IR Verification (x + 0, x * 0, x * 1, x / 1, x & 0, x & x, x | 0, x | x, x ^ 0, x ^ x)
static void test_eval_algebraic_identities_ir(void) {
  TestFunction *fn = new_test_fn("test_algebraic_ir");
  TestVReg *v_x = new_test_vreg(fn);
  TestVReg *v_zero = new_test_vreg(fn);
  TestVReg *v_one = new_test_vreg(fn);
  TestVReg *v_r1 = new_test_vreg(fn);
  TestVReg *v_r2 = new_test_vreg(fn);
  TestVReg *v_r3 = new_test_vreg(fn);
  TestVReg *v_r4 = new_test_vreg(fn);
  TestVReg *v_r5 = new_test_vreg(fn);
  TestVReg *v_r6 = new_test_vreg(fn);
  TestVReg *v_sum = new_test_vreg(fn);

  TestInsn *i_z = new_test_insn(fn, LLIR_IMM); i_z->dst = v_zero; i_z->imm = 0; append_test_insn(fn, i_z);
  TestInsn *i_o = new_test_insn(fn, LLIR_IMM); i_o->dst = v_one; i_o->imm = 1; append_test_insn(fn, i_o);

  TestInsn *i_add0 = new_test_insn(fn, LLIR_ADD); i_add0->dst = v_r1; i_add0->src1 = v_x; i_add0->src2 = v_zero; append_test_insn(fn, i_add0); // x
  TestInsn *i_mul0 = new_test_insn(fn, LLIR_MUL); i_mul0->dst = v_r2; i_mul0->src1 = v_x; i_mul0->src2 = v_zero; append_test_insn(fn, i_mul0); // 0
  TestInsn *i_mul1 = new_test_insn(fn, LLIR_MUL); i_mul1->dst = v_r3; i_mul1->src1 = v_x; i_mul1->src2 = v_one; append_test_insn(fn, i_mul1); // x
  TestInsn *i_andx = new_test_insn(fn, LLIR_AND); i_andx->dst = v_r4; i_andx->src1 = v_x; i_andx->src2 = v_x; append_test_insn(fn, i_andx); // x
  TestInsn *i_or0  = new_test_insn(fn, LLIR_OR);  i_or0->dst  = v_r5; i_or0->src1  = v_x; i_or0->src2  = v_zero; append_test_insn(fn, i_or0); // x
  TestInsn *i_xorx = new_test_insn(fn, LLIR_XOR); i_xorx->dst = v_r6; i_xorx->src1 = v_x; i_xorx->src2 = v_x; append_test_insn(fn, i_xorx); // 0

  // Total: r1(x) + r2(0) + r3(x) + r4(x) + r5(x) + r6(0) = 4*x
  TestInsn *i_acc1 = new_test_insn(fn, LLIR_ADD); i_acc1->dst = v_sum; i_acc1->src1 = v_r1; i_acc1->src2 = v_r3; append_test_insn(fn, i_acc1);
  TestInsn *ret = new_test_insn(fn, LLIR_RET); ret->src1 = v_sum; append_test_insn(fn, ret);

  int64_t arg = 7; // 7 + 7 = 14
  int64_t unopt_res = interp_run(fn, &arg, 1);
  ASSERT(14, unopt_res);

  // Run peephole, copy prop, DCE
  opt_peephole(fn);
  opt_copy_prop(fn);
  opt_dce(fn);

  // Verify MUL, AND, OR, XOR operations were eliminated
  ASSERT(0, count_insns_of_kind(fn, LLIR_MUL));
  ASSERT(0, count_insns_of_kind(fn, LLIR_AND));
  ASSERT(0, count_insns_of_kind(fn, LLIR_OR));
  ASSERT(0, count_insns_of_kind(fn, LLIR_XOR));

  int64_t opt_res = interp_run(fn, &arg, 1);
  ASSERT(14, opt_res);
}

// Test 11: Multi-Pass Fixed-Point Loop Convergence
static void test_eval_multipass_convergence(void) {
  TestFunction *fn = new_test_fn("test_fixed_point");
  TestVReg *v_x = new_test_vreg(fn);
  TestVReg *v_y = new_test_vreg(fn);
  TestVReg *v_c10 = new_test_vreg(fn);
  TestVReg *v_c20 = new_test_vreg(fn);
  TestVReg *v_c30 = new_test_vreg(fn);
  TestVReg *v_cancel = new_test_vreg(fn);
  TestVReg *v_zero_mul = new_test_vreg(fn);
  TestVReg *v_add_c = new_test_vreg(fn);
  TestVReg *v_stage1 = new_test_vreg(fn);
  TestVReg *v_final = new_test_vreg(fn);

  // Expression: (x - x) + (y * 0) + (10 + 20)
  TestInsn *i_sub = new_test_insn(fn, LLIR_SUB); i_sub->dst = v_cancel; i_sub->src1 = v_x; i_sub->src2 = v_x; append_test_insn(fn, i_sub);
  TestInsn *i_z = new_test_insn(fn, LLIR_IMM); i_z->dst = v_c30; i_z->imm = 0; append_test_insn(fn, i_z);
  TestInsn *i_mul = new_test_insn(fn, LLIR_MUL); i_mul->dst = v_zero_mul; i_mul->src1 = v_y; i_mul->src2 = v_c30; append_test_insn(fn, i_mul);

  TestInsn *i_10 = new_test_insn(fn, LLIR_IMM); i_10->dst = v_c10; i_10->imm = 10; append_test_insn(fn, i_10);
  TestInsn *i_20 = new_test_insn(fn, LLIR_IMM); i_20->dst = v_c20; i_20->imm = 20; append_test_insn(fn, i_20);
  TestInsn *i_add1 = new_test_insn(fn, LLIR_ADD); i_add1->dst = v_add_c; i_add1->src1 = v_c10; i_add1->src2 = v_c20; append_test_insn(fn, i_add1);

  TestInsn *i_add2 = new_test_insn(fn, LLIR_ADD); i_add2->dst = v_stage1; i_add2->src1 = v_cancel; i_add2->src2 = v_zero_mul; append_test_insn(fn, i_add2);
  TestInsn *i_add3 = new_test_insn(fn, LLIR_ADD); i_add3->dst = v_final; i_add3->src1 = v_stage1; i_add3->src2 = v_add_c; append_test_insn(fn, i_add3);
  TestInsn *ret = new_test_insn(fn, LLIR_RET); ret->src1 = v_final; append_test_insn(fn, ret);

  int64_t args[2] = { 100, 200 };
  int64_t unopt_res = interp_run(fn, args, 2);
  ASSERT(30, unopt_res);

  // Run full multi-pass pipeline until fixed-point convergence
  run_all_optimizations(fn);

  // Entire expression should collapse down to an IMM 30 and RET
  ASSERT(0, count_insns_of_kind(fn, LLIR_ADD));
  ASSERT(0, count_insns_of_kind(fn, LLIR_SUB));
  ASSERT(0, count_insns_of_kind(fn, LLIR_MUL));
  ASSERT(1, count_insns_of_kind(fn, LLIR_IMM));
  ASSERT(1, count_insns_of_kind(fn, LLIR_RET));

  int64_t opt_res = interp_run(fn, args, 2);
  ASSERT(30, opt_res);
}

// Test 12: Negation, NOT, and -1 / 0 Identities in IR (x * -1, x ^ -1, x & -1, 0 - x, x % 1)
static void test_eval_neg_not_identities(void) {
  TestFunction *fn = new_test_fn("test_neg_not_ir");
  TestVReg *v_x = new_test_vreg(fn);
  TestVReg *v_minus1 = new_test_vreg(fn);
  TestVReg *v_one = new_test_vreg(fn);
  TestVReg *v_zero = new_test_vreg(fn);
  TestVReg *v_neg = new_test_vreg(fn);
  TestVReg *v_not = new_test_vreg(fn);
  TestVReg *v_and = new_test_vreg(fn);
  TestVReg *v_sub0 = new_test_vreg(fn);
  TestVReg *v_mod1 = new_test_vreg(fn);
  TestVReg *v_sum1 = new_test_vreg(fn);
  TestVReg *v_sum2 = new_test_vreg(fn);
  TestVReg *v_final = new_test_vreg(fn);

  TestInsn *i_m1 = new_test_insn(fn, LLIR_IMM); i_m1->dst = v_minus1; i_m1->imm = -1; append_test_insn(fn, i_m1);
  TestInsn *i_1  = new_test_insn(fn, LLIR_IMM); i_1->dst  = v_one;    i_1->imm  = 1;  append_test_insn(fn, i_1);
  TestInsn *i_0  = new_test_insn(fn, LLIR_IMM); i_0->dst  = v_zero;   i_0->imm  = 0;  append_test_insn(fn, i_0);

  // x * -1 -> -x
  TestInsn *i_mul_neg = new_test_insn(fn, LLIR_MUL); i_mul_neg->dst = v_neg; i_mul_neg->src1 = v_x; i_mul_neg->src2 = v_minus1; append_test_insn(fn, i_mul_neg);
  // x ^ -1 -> ~x
  TestInsn *i_xor_not = new_test_insn(fn, LLIR_XOR); i_xor_not->dst = v_not; i_xor_not->src1 = v_x; i_xor_not->src2 = v_minus1; append_test_insn(fn, i_xor_not);
  // x & -1 -> x
  TestInsn *i_and_m1  = new_test_insn(fn, LLIR_AND); i_and_m1->dst  = v_and; i_and_m1->src1  = v_x; i_and_m1->src2  = v_minus1; append_test_insn(fn, i_and_m1);
  // 0 - x -> -x
  TestInsn *i_0_sub   = new_test_insn(fn, LLIR_SUB); i_0_sub->dst   = v_sub0; i_0_sub->src1   = v_zero; i_0_sub->src2 = v_x; append_test_insn(fn, i_0_sub);
  // x % 1 -> 0
  TestInsn *i_m1_mod  = new_test_insn(fn, LLIR_MOD); i_m1_mod->dst  = v_mod1; i_m1_mod->src1  = v_x; i_m1_mod->src2  = v_one; append_test_insn(fn, i_m1_mod);

  // Combine results
  TestInsn *i_s1 = new_test_insn(fn, LLIR_ADD); i_s1->dst = v_sum1; i_s1->src1 = v_neg; i_s1->src2 = v_not; append_test_insn(fn, i_s1);
  TestInsn *i_s2 = new_test_insn(fn, LLIR_ADD); i_s2->dst = v_sum2; i_s2->src1 = v_and; i_s2->src2 = v_sub0; append_test_insn(fn, i_s2);
  TestInsn *i_f  = new_test_insn(fn, LLIR_ADD); i_f->dst  = v_final; i_f->src1  = v_sum1; i_f->src2 = v_sum2; append_test_insn(fn, i_f);
  TestInsn *ret  = new_test_insn(fn, LLIR_RET); ret->src1 = v_final; append_test_insn(fn, ret);

  int64_t arg = 42; // (-42) + (~42) + (42) + (-42) = -42 + (-43) + 42 - 42 = -85
  int64_t unopt_res = interp_run(fn, &arg, 1);
  ASSERT(-85, unopt_res);

  // Optimize IR
  opt_peephole(fn);
  opt_copy_prop(fn);
  opt_dce(fn);

  // Verify MUL and MOD were eliminated and converted to NEG, NOT, MOV, IMM
  ASSERT(0, count_insns_of_kind(fn, LLIR_MUL));
  ASSERT(0, count_insns_of_kind(fn, LLIR_MOD));
  ASSERT(2, count_insns_of_kind(fn, LLIR_NEG));
  ASSERT(1, count_insns_of_kind(fn, LLIR_NOT));

  int64_t opt_res = interp_run(fn, &arg, 1);
  ASSERT(-85, opt_res);
}

int main(void) {
  test_eval_constant_folding();
  test_eval_dead_code_elimination();
  test_eval_strength_reduction();
  test_eval_self_cancellation();
  test_eval_copy_propagation();
  test_eval_memory_operations();
  test_eval_constant_canonicalization();
  test_eval_unsigned_div_mod_strength_reduction();
  test_eval_constant_branch_folding();
  test_eval_algebraic_identities_ir();
  test_eval_multipass_convergence();
  test_eval_neg_not_identities();

  printf("OK\n");
  return 0;
}
