#include "ir/hlir_opt.h"
#include "ir/opt.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// High-Level Intermediate Representation (HLIR) Optimization Driver
// ============================================================================
// This file provides shared predicate helpers and coordinates the high-level
// optimization pipeline.
//
// Individual modular HLIR passes:
//   - ir/hlir_opt_fold.c         : Constant folding & operand canonicalization
//   - ir/hlir_opt_algebraic.c    : Algebraic identities & strength reduction
//   - ir/hlir_opt_copy_prop.c    : Copy propagation across basic blocks
//   - ir/hlir_opt_cse.c          : Local common subexpression elimination
//   - ir/hlir_opt_load_store.c   : Store-to-load forwarding & dead stores
//   - ir/hlir_opt_control_flow.c : Branch simplification & jump threading
//   - ir/hlir_opt_dce.c          : Dead code & unused value elimination
//   - ir/hlir_opt_inlining.c     : Function inlining
// ============================================================================

// ----------------------------------------------------------------------------
// 1. Instruction Classification & Predicates
// ----------------------------------------------------------------------------

void hlir_for_each_use(HLIRInsn *insn, HLIRUseCallback cb, void *data) {
  if (insn->src1) cb(&insn->src1, data);
  if (insn->src2) cb(&insn->src2, data);
  if (insn->src3) cb(&insn->src3, data);
  for (int i = 0; i < insn->num_args; i++) {
    if (insn->args[i])
      cb(&insn->args[i], data);
  }
}

bool hlir_is_pure(HLIRKind kind) {
  switch (kind) {
  case HLIR_ICONST:
  case HLIR_FCONST:
  case HLIR_SCONST:
  case HLIR_ADDR_VAR:
  case HLIR_CAST:
  case HLIR_ADD:
  case HLIR_SUB:
  case HLIR_MUL:
  case HLIR_DIV:
  case HLIR_MOD:
  case HLIR_BITAND:
  case HLIR_BITOR:
  case HLIR_BITXOR:
  case HLIR_SHL:
  case HLIR_SHR:
  case HLIR_NEG:
  case HLIR_BITNOT:
  case HLIR_LOGNOT:
  case HLIR_CMP_EQ:
  case HLIR_CMP_NE:
  case HLIR_CMP_LT:
  case HLIR_CMP_LE:
  case HLIR_CMP_GT:
  case HLIR_CMP_GE:
    return true;
  default:
    return false;
  }
}

bool hlir_is_commutative(HLIRKind kind) {
  switch (kind) {
  case HLIR_ADD:
  case HLIR_MUL:
  case HLIR_BITAND:
  case HLIR_BITOR:
  case HLIR_BITXOR:
  case HLIR_CMP_EQ:
  case HLIR_CMP_NE:
    return true;
  default:
    return false;
  }
}

bool hlir_is_relational(HLIRKind kind) {
  switch (kind) {
  case HLIR_CMP_EQ:
  case HLIR_CMP_NE:
  case HLIR_CMP_LT:
  case HLIR_CMP_LE:
  case HLIR_CMP_GT:
  case HLIR_CMP_GE:
    return true;
  default:
    return false;
  }
}

HLIRKind hlir_swap_cmp(HLIRKind kind) {
  switch (kind) {
  case HLIR_CMP_EQ: return HLIR_CMP_EQ;
  case HLIR_CMP_NE: return HLIR_CMP_NE;
  case HLIR_CMP_LT: return HLIR_CMP_GT;
  case HLIR_CMP_LE: return HLIR_CMP_GE;
  case HLIR_CMP_GT: return HLIR_CMP_LT;
  case HLIR_CMP_GE: return HLIR_CMP_LE;
  default: return kind;
  }
}

HLIRKind hlir_invert_cmp(HLIRKind kind) {
  switch (kind) {
  case HLIR_CMP_EQ: return HLIR_CMP_NE;
  case HLIR_CMP_NE: return HLIR_CMP_EQ;
  case HLIR_CMP_LT: return HLIR_CMP_GE;
  case HLIR_CMP_LE: return HLIR_CMP_GT;
  case HLIR_CMP_GT: return HLIR_CMP_LE;
  case HLIR_CMP_GE: return HLIR_CMP_LT;
  default: return HLIR_NOP;
  }
}

bool hlir_is_bb_barrier(HLIRKind kind) {
  switch (kind) {
  case HLIR_LABEL:
  case HLIR_JMP:
  case HLIR_JMP_IF_ZERO:
  case HLIR_JMP_IF_NZ:
  case HLIR_CALL:
  case HLIR_RET:
  case HLIR_ASM:
  case HLIR_CAS:
  case HLIR_EXCH:
    return true;
  default:
    return false;
  }
}

bool hlir_is_power_of_two(int64_t val, int *out_shift) {
  if (val <= 0 || (val & (val - 1)) != 0)
    return false;
  int shift = 0;
  while ((1LL << shift) < val)
    shift++;
  if ((1LL << shift) == val) {
    if (out_shift) *out_shift = shift;
    return true;
  }
  return false;
}

// ----------------------------------------------------------------------------
// 2. Instruction Mutation Utilities
// ----------------------------------------------------------------------------

void hlir_set_iconst(HLIRInsn *insn, int64_t imm) {
  insn->kind = HLIR_ICONST;
  insn->imm = imm;
  insn->src1 = NULL;
  insn->src2 = NULL;
  insn->src3 = NULL;
}

void hlir_set_cast(HLIRInsn *insn, HLIRVal *src) {
  insn->kind = HLIR_CAST;
  insn->src1 = src;
  insn->src2 = NULL;
  insn->src3 = NULL;
}

void hlir_set_unary(HLIRInsn *insn, HLIRKind kind, HLIRVal *src) {
  insn->kind = kind;
  insn->src1 = src;
  insn->src2 = NULL;
  insn->src3 = NULL;
}

// ----------------------------------------------------------------------------
// 3. Block-Local Environment Tracking
// ----------------------------------------------------------------------------

void hlir_env_init(HLIREnv *env, int num_vals, bool track_defs) {
  env->num_vals = num_vals;
  env->const_vals = calloc(num_vals ? num_vals : 1, sizeof(int64_t));
  env->is_const = calloc(num_vals ? num_vals : 1, sizeof(bool));
  env->defs = track_defs ? calloc(num_vals ? num_vals : 1, sizeof(HLIRInsn *)) : NULL;
}

void hlir_env_reset(HLIREnv *env) {
  if (env->num_vals > 0) {
    memset(env->is_const, 0, env->num_vals * sizeof(bool));
    if (env->defs)
      memset(env->defs, 0, env->num_vals * sizeof(HLIRInsn *));
  }
}

void hlir_env_free(HLIREnv *env) {
  free(env->const_vals);
  free(env->is_const);
  if (env->defs) free(env->defs);
}

void hlir_env_set_const(HLIREnv *env, HLIRVal *val, int64_t imm) {
  if (val && val->id < env->num_vals) {
    env->is_const[val->id] = true;
    env->const_vals[val->id] = imm;
  }
}

void hlir_env_set_def(HLIREnv *env, HLIRVal *val, HLIRInsn *insn) {
  if (env->defs && val && val->id < env->num_vals) {
    env->defs[val->id] = insn;
  }
}

bool hlir_env_get_const(HLIREnv *env, HLIRVal *val, int64_t *out_imm) {
  if (val && val->id < env->num_vals && env->is_const[val->id]) {
    if (out_imm) *out_imm = env->const_vals[val->id];
    return true;
  }
  return false;
}

HLIRInsn *hlir_env_get_def(HLIREnv *env, HLIRVal *val) {
  if (env->defs && val && val->id < env->num_vals)
    return env->defs[val->id];
  return NULL;
}

// ----------------------------------------------------------------------------
// 4. Fixed-Point HLIR Pipeline Driver
// ----------------------------------------------------------------------------

void hlir_optimize(HLIRProg *prog, int opt_level) {
  if (!prog || opt_level <= 0)
    return;

  ir_init_pass_registry();

  if (opt_level >= 2 && pass_hlir_inlining.enabled) {
    hlir_opt_inlining(prog);
  }

  int max_iter = (opt_max_passes > 0) ? opt_max_passes : 2;

  for (int i = 0; i < prog->num_fns; i++) {
    HLIRFunction *fn = prog->fns[i];
    if (!fn) continue;

    for (int iter = 0; iter < max_iter; iter++) {
      bool changed = false;
      if (pass_hlir_const_fold.enabled)
        changed |= hlir_opt_const_fold(fn);
      if (pass_hlir_algebraic.enabled)
        changed |= hlir_opt_algebraic(fn);
      if (pass_hlir_copy_prop.enabled)
        changed |= hlir_opt_copy_prop(fn);
      if (pass_hlir_local_cse.enabled)
        changed |= hlir_opt_local_cse(fn);
      if (pass_hlir_load_store.enabled)
        changed |= hlir_opt_load_store(fn);
      if (pass_hlir_control_flow.enabled)
        changed |= hlir_opt_control_flow(fn);
      if (pass_hlir_dead_code.enabled) {
        changed |= hlir_opt_dead_code(fn);
        changed |= hlir_opt_dce(fn);
      }

      if (!changed)
        break;
    }
  }
}
