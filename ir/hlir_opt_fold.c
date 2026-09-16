#include "ir/hlir_opt.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// HLIR Constant Folding & Operand Canonicalization Pass
// ============================================================================
// Evaluates constant expressions at compile time and normalizes binary operations
// by commuting constants to the right-hand operand (src2).

static bool hlir_fold_binary(HLIRKind kind, int64_t c1, int64_t c2, int64_t *out_res) {
  switch (kind) {
  case HLIR_ADD: *out_res = c1 + c2; return true;
  case HLIR_SUB: *out_res = c1 - c2; return true;
  case HLIR_MUL: *out_res = c1 * c2; return true;
  case HLIR_DIV: if (c2 != 0) { *out_res = c1 / c2; return true; } return false;
  case HLIR_MOD: if (c2 != 0) { *out_res = c1 % c2; return true; } return false;
  case HLIR_BITAND: *out_res = c1 & c2; return true;
  case HLIR_BITOR:  *out_res = c1 | c2; return true;
  case HLIR_BITXOR: *out_res = c1 ^ c2; return true;
  case HLIR_SHL: if (c2 >= 0 && c2 < 64) { *out_res = c1 << c2; return true; } return false;
  case HLIR_SHR: if (c2 >= 0 && c2 < 64) { *out_res = c1 >> c2; return true; } return false;
  case HLIR_CMP_EQ: *out_res = (c1 == c2); return true;
  case HLIR_CMP_NE: *out_res = (c1 != c2); return true;
  case HLIR_CMP_LT: *out_res = (c1 < c2); return true;
  case HLIR_CMP_LE: *out_res = (c1 <= c2); return true;
  case HLIR_CMP_GT: *out_res = (c1 > c2); return true;
  case HLIR_CMP_GE: *out_res = (c1 >= c2); return true;
  default: return false;
  }
}

static bool hlir_fold_unary(HLIRKind kind, Type *ty, int64_t c, int64_t *out_res) {
  switch (kind) {
  case HLIR_NEG: *out_res = -c; return true;
  case HLIR_BITNOT: *out_res = ~c; return true;
  case HLIR_LOGNOT: *out_res = !c; return true;
  case HLIR_CAST: {
    if (ty && is_flonum(ty))
      return false;
    int sz = ty ? ty->size : 8;
    bool is_unsigned = ty ? ty->is_unsigned : false;
    int64_t res = c;
    if (sz == 1) res = is_unsigned ? (uint8_t)c : (int8_t)c;
    else if (sz == 2) res = is_unsigned ? (uint16_t)c : (int16_t)c;
    else if (sz == 4) res = is_unsigned ? (uint32_t)c : (int32_t)c;
    *out_res = res;
    return true;
  }
  default:
    return false;
  }
}

bool hlir_opt_const_fold(HLIRFunction *fn) {
  if (!fn || fn->num_vals == 0)
    return false;

  bool changed = false;
  HLIREnv env;
  hlir_env_init(&env, fn->num_vals, false);

  for (HLIRInsn *insn = fn->head; insn; insn = insn->next) {
    if (hlir_is_bb_barrier(insn->kind)) {
      hlir_env_reset(&env);
      continue;
    }

    if (insn->kind == HLIR_ICONST && insn->dst) {
      hlir_env_set_const(&env, insn->dst, insn->imm);
      continue;
    }

    int64_t c1, c2, res;
    // Fold binary constant operations
    if (insn->src1 && insn->src2 && insn->dst &&
        hlir_env_get_const(&env, insn->src1, &c1) &&
        hlir_env_get_const(&env, insn->src2, &c2) &&
        hlir_fold_binary(insn->kind, c1, c2, &res)) {
      hlir_set_iconst(insn, res);
      hlir_env_set_const(&env, insn->dst, res);
      changed = true;
      continue;
    }

    // Fold unary constant operations
    if (insn->src1 && !insn->src2 && insn->dst &&
        hlir_env_get_const(&env, insn->src1, &c1) &&
        hlir_fold_unary(insn->kind, insn->ty, c1, &res)) {
      hlir_set_iconst(insn, res);
      hlir_env_set_const(&env, insn->dst, res);
      changed = true;
      continue;
    }

    // Canonicalize: place constant in src2 for commutative and relational operations
    bool s1_is_const = hlir_env_get_const(&env, insn->src1, NULL);
    bool s2_is_const = hlir_env_get_const(&env, insn->src2, NULL);
    if (s1_is_const && insn->src2 && !s2_is_const) {
      if (hlir_is_commutative(insn->kind)) {
        HLIRVal *tmp = insn->src1;
        insn->src1 = insn->src2;
        insn->src2 = tmp;
        changed = true;
      } else if (hlir_is_relational(insn->kind)) {
        insn->kind = hlir_swap_cmp(insn->kind);
        HLIRVal *tmp = insn->src1;
        insn->src1 = insn->src2;
        insn->src2 = tmp;
        changed = true;
      }
    }
  }

  hlir_env_free(&env);
  return changed;
}
