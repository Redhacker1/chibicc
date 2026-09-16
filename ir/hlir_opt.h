#ifndef CHIBICC_HLIR_OPT_H
#define CHIBICC_HLIR_OPT_H

#include "ir/hlir.h"
#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// High-Level Intermediate Representation (HLIR) Optimizer Interface
// ============================================================================

// ----------------------------------------------------------------------------
// HLIR Optimization Passes
// ----------------------------------------------------------------------------
bool hlir_opt_const_fold(HLIRFunction *fn);
bool hlir_opt_algebraic(HLIRFunction *fn);
bool hlir_opt_copy_prop(HLIRFunction *fn);
bool hlir_opt_local_cse(HLIRFunction *fn);
bool hlir_opt_load_store(HLIRFunction *fn);
bool hlir_opt_control_flow(HLIRFunction *fn);
bool hlir_opt_dce(HLIRFunction *fn);
bool hlir_opt_dead_code(HLIRFunction *fn);
bool hlir_opt_inlining(HLIRProg *prog);

// HLIR Fixed-Point Optimization Pipeline Driver
void hlir_optimize(HLIRProg *prog, int opt_level);

// ----------------------------------------------------------------------------
// Shared Instruction Predicates & Classification
// ----------------------------------------------------------------------------
bool hlir_is_pure(HLIRKind kind);
bool hlir_is_commutative(HLIRKind kind);
bool hlir_is_relational(HLIRKind kind);
HLIRKind hlir_swap_cmp(HLIRKind kind);
HLIRKind hlir_invert_cmp(HLIRKind kind);
bool hlir_is_bb_barrier(HLIRKind kind);
bool hlir_is_power_of_two(int64_t val, int *out_shift);

// ----------------------------------------------------------------------------
// Instruction Mutation Utilities
// ----------------------------------------------------------------------------
void hlir_set_iconst(HLIRInsn *insn, int64_t imm);
void hlir_set_cast(HLIRInsn *insn, HLIRVal *src);
void hlir_set_unary(HLIRInsn *insn, HLIRKind kind, HLIRVal *src);

typedef void (*HLIRUseCallback)(HLIRVal **use, void *data);
void hlir_for_each_use(HLIRInsn *insn, HLIRUseCallback cb, void *data);

// ----------------------------------------------------------------------------
// Block-Local Environment Tracking (Constants & Definitions)
// ----------------------------------------------------------------------------
typedef struct {
  int num_vals;
  int64_t *const_vals;
  bool *is_const;
  HLIRInsn **defs;
} HLIREnv;

void hlir_env_init(HLIREnv *env, int num_vals, bool track_defs);
void hlir_env_reset(HLIREnv *env);
void hlir_env_free(HLIREnv *env);
void hlir_env_set_const(HLIREnv *env, HLIRVal *val, int64_t imm);
void hlir_env_set_def(HLIREnv *env, HLIRVal *val, HLIRInsn *insn);
bool hlir_env_get_const(HLIREnv *env, HLIRVal *val, int64_t *out_imm);
HLIRInsn *hlir_env_get_def(HLIREnv *env, HLIRVal *val);

#endif // CHIBICC_HLIR_OPT_H
