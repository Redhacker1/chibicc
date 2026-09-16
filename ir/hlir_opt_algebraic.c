#include "ir/hlir_opt.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// HLIR Algebraic Simplifications & Strength Reduction Pass
// ============================================================================
// Applies algebraic identities (identities, annihilators, idempotence,
// self-operands, double negations, strength reduction, reassociation,
// cancellation, and absorption).

typedef enum {
  OP_SIDE_NONE = 0,
  OP_SIDE_LEFT = 1,
  OP_SIDE_RIGHT = 2,
  OP_SIDE_BOTH = 3,
} OpSide;

typedef struct {
  HLIRKind kind;
  bool is_commutative;
  bool is_associative;
  bool has_identity;
  int64_t identity;
  OpSide identity_side;
  bool has_annihilator;
  int64_t annihilator;
  int64_t annihilator_res;
  OpSide annihilator_side;
  bool has_self_const;
  int64_t self_const_res;
  bool is_self_idempotent;
} HLIRAlgebraicProps;

static const HLIRAlgebraicProps *hlir_get_algebraic_props(HLIRKind kind) {
  static const HLIRAlgebraicProps props_table[] = {
    { HLIR_ADD,     true,  true,  true,  0,  OP_SIDE_BOTH,  false, 0,  0,  OP_SIDE_NONE,  false, 0, false },
    { HLIR_SUB,     false, false, true,  0,  OP_SIDE_RIGHT, false, 0,  0,  OP_SIDE_NONE,  true,  0, false },
    { HLIR_MUL,     true,  true,  true,  1,  OP_SIDE_BOTH,  true,  0,  0,  OP_SIDE_BOTH,  false, 0, false },
    { HLIR_DIV,     false, false, true,  1,  OP_SIDE_RIGHT, false, 0,  0,  OP_SIDE_NONE,  true,  1, false },
    { HLIR_MOD,     false, false, false, 0,  OP_SIDE_NONE,  true,  1,  0,  OP_SIDE_RIGHT, false, 0, false },
    { HLIR_BITAND,  true,  true,  true, -1,  OP_SIDE_BOTH,  true,  0,  0,  OP_SIDE_BOTH,  false, 0, true  },
    { HLIR_BITOR,   true,  true,  true,  0,  OP_SIDE_BOTH,  true, -1, -1,  OP_SIDE_BOTH,  false, 0, true  },
    { HLIR_BITXOR,  true,  true,  true,  0,  OP_SIDE_BOTH,  false, 0,  0,  OP_SIDE_NONE,  true,  0, false },
    { HLIR_SHL,     false, false, true,  0,  OP_SIDE_RIGHT, false, 0,  0,  OP_SIDE_NONE,  false, 0, false },
    { HLIR_SHR,     false, false, true,  0,  OP_SIDE_RIGHT, false, 0,  0,  OP_SIDE_NONE,  false, 0, false },
    { HLIR_CMP_EQ,  true,  false, false, 0,  OP_SIDE_NONE,  false, 0,  0,  OP_SIDE_NONE,  true,  1, false },
    { HLIR_CMP_NE,  true,  false, false, 0,  OP_SIDE_NONE,  false, 0,  0,  OP_SIDE_NONE,  true,  0, false },
    { HLIR_CMP_LE,  false, false, false, 0,  OP_SIDE_NONE,  false, 0,  0,  OP_SIDE_NONE,  true,  1, false },
    { HLIR_CMP_GE,  false, false, false, 0,  OP_SIDE_NONE,  false, 0,  0,  OP_SIDE_NONE,  true,  1, false },
    { HLIR_CMP_LT,  false, false, false, 0,  OP_SIDE_NONE,  false, 0,  0,  OP_SIDE_NONE,  true,  0, false },
    { HLIR_CMP_GT,  false, false, false, 0,  OP_SIDE_NONE,  false, 0,  0,  OP_SIDE_NONE,  true,  0, false },
  };
  for (size_t i = 0; i < sizeof(props_table) / sizeof(props_table[0]); i++) {
    if (props_table[i].kind == kind)
      return &props_table[i];
  }
  return NULL;
}

static HLIRVal *hlir_make_iconst(HLIRFunction *fn, HLIREnv *env, HLIRInsn *before, int64_t val, Type *ty) {
  HLIRVal *v = hlir_new_val(fn, ty ? ty : ty_long);
  HLIRInsn *insn = hlir_new_insn(HLIR_ICONST);
  insn->dst = v;
  insn->imm = val;
  insn->ty = ty ? ty : ty_long;
  hlir_insert_before(fn, before, insn);
  if (env) {
    hlir_env_set_def(env, v, insn);
    hlir_env_set_const(env, v, val);
  }
  return v;
}

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

// Rule: Double unary operations and relational inversions
static bool hlir_rule_simplify_unary(HLIREnv *env, HLIRInsn *insn) {
  if (!insn->dst || !insn->src1 || insn->src2)
    return false;

  HLIRInsn *def = hlir_env_get_def(env, insn->src1);
  if (!def)
    return false;

  if ((insn->kind == HLIR_NEG && def->kind == HLIR_NEG) ||
      (insn->kind == HLIR_BITNOT && def->kind == HLIR_BITNOT)) {
    hlir_set_cast(insn, def->src1);
    return true;
  }
  if (insn->kind == HLIR_LOGNOT && hlir_is_relational(def->kind)) {
    insn->kind = hlir_invert_cmp(def->kind);
    insn->src1 = def->src1;
    insn->src2 = def->src2;
    return true;
  }
  return false;
}

// Rule: Identity element simplification (e.g. x + 0 -> x, x * 1 -> x, x & -1 -> x)
static bool hlir_rule_identity(const HLIRAlgebraicProps *props, HLIREnv *env, HLIRInsn *insn) {
  if (!props || !props->has_identity)
    return false;

  int64_t c2, c1;
  if ((props->identity_side & OP_SIDE_RIGHT) && hlir_env_get_const(env, insn->src2, &c2) && c2 == props->identity) {
    hlir_set_cast(insn, insn->src1);
    return true;
  }
  if ((props->identity_side & OP_SIDE_LEFT) && hlir_env_get_const(env, insn->src1, &c1) && c1 == props->identity) {
    hlir_set_cast(insn, insn->src2);
    return true;
  }
  return false;
}

// Rule: Annihilator element simplification (e.g. x * 0 -> 0, x & 0 -> 0, x | -1 -> -1)
static bool hlir_rule_annihilator(const HLIRAlgebraicProps *props, HLIREnv *env, HLIRInsn *insn) {
  if (!props || !props->has_annihilator)
    return false;

  int64_t c1, c2;
  if (((props->annihilator_side & OP_SIDE_RIGHT) && hlir_env_get_const(env, insn->src2, &c2) && c2 == props->annihilator) ||
      ((props->annihilator_side & OP_SIDE_LEFT) && hlir_env_get_const(env, insn->src1, &c1) && c1 == props->annihilator)) {
    hlir_set_iconst(insn, props->annihilator_res);
    hlir_env_set_const(env, insn->dst, props->annihilator_res);
    return true;
  }
  return false;
}

// Rule: Identical operand simplification (e.g. x - x -> 0, x == x -> 1, x & x -> x, x + x -> x << 1)
static bool hlir_rule_self_operands(HLIRFunction *fn, const HLIRAlgebraicProps *props, HLIREnv *env, HLIRInsn *insn) {
  if (insn->src1 != insn->src2)
    return false;

  bool is_float = (insn->ty && is_flonum(insn->ty)) || (insn->src1->ty && is_flonum(insn->src1->ty));
  if (is_float)
    return false;

  if (props && props->has_self_const) {
    hlir_set_iconst(insn, props->self_const_res);
    hlir_env_set_const(env, insn->dst, props->self_const_res);
    return true;
  }
  if (props && props->is_self_idempotent) {
    hlir_set_cast(insn, insn->src1);
    return true;
  }
  if (insn->kind == HLIR_ADD) {
    HLIRVal *shift_val = hlir_make_iconst(fn, env, insn, 1, insn->src1->ty);
    insn->kind = HLIR_SHL;
    insn->src2 = shift_val;
    return true;
  }
  return false;
}

// Rule: Negation & Sign Inversion rules
static bool hlir_rule_negations(HLIREnv *env, HLIRInsn *insn) {
  int64_t c1, c2;
  // Constant sign inversion: x * -1 -> -x, -1 * x -> -x
  if (insn->kind == HLIR_MUL) {
    if (hlir_env_get_const(env, insn->src2, &c2) && c2 == -1) {
      hlir_set_unary(insn, HLIR_NEG, insn->src1);
      return true;
    }
    if (hlir_env_get_const(env, insn->src1, &c1) && c1 == -1) {
      hlir_set_unary(insn, HLIR_NEG, insn->src2);
      return true;
    }
  }

  // Bitwise not via XOR with -1: x ^ -1 -> ~x, -1 ^ x -> ~x
  if (insn->kind == HLIR_BITXOR) {
    if (hlir_env_get_const(env, insn->src2, &c2) && c2 == -1) {
      hlir_set_unary(insn, HLIR_BITNOT, insn->src1);
      return true;
    }
    if (hlir_env_get_const(env, insn->src1, &c1) && c1 == -1) {
      hlir_set_unary(insn, HLIR_BITNOT, insn->src2);
      return true;
    }
  }

  // Negation from 0 - x -> -x
  if (insn->kind == HLIR_SUB && hlir_env_get_const(env, insn->src1, &c1) && c1 == 0) {
    hlir_set_unary(insn, HLIR_NEG, insn->src2);
    return true;
  }

  // Negation propagation: x + (-y) -> x - y, (-x) + y -> y - x
  if (insn->kind == HLIR_ADD) {
    HLIRInsn *def2 = hlir_env_get_def(env, insn->src2);
    if (def2 && def2->kind == HLIR_NEG && def2->src1) {
      insn->kind = HLIR_SUB;
      insn->src2 = def2->src1;
      return true;
    }
    HLIRInsn *def1 = hlir_env_get_def(env, insn->src1);
    if (def1 && def1->kind == HLIR_NEG && def1->src1) {
      insn->kind = HLIR_SUB;
      insn->src1 = insn->src2;
      insn->src2 = def1->src1;
      return true;
    }
  }

  // x - (-y) -> x + y
  if (insn->kind == HLIR_SUB) {
    HLIRInsn *def2 = hlir_env_get_def(env, insn->src2);
    if (def2 && def2->kind == HLIR_NEG && def2->src1) {
      insn->kind = HLIR_ADD;
      insn->src2 = def2->src1;
      return true;
    }
  }

  return false;
}

// Rule: Strength reduction (power-of-two multiplication, division, modulo)
static bool hlir_rule_strength_reduction(HLIRFunction *fn, HLIREnv *env, HLIRInsn *insn) {
  int64_t val;
  int shift;
  bool is_float = (insn->ty && is_flonum(insn->ty));

  // x * 2^k -> x << k
  if (insn->kind == HLIR_MUL && !is_float) {
    if (hlir_env_get_const(env, insn->src2, &val) && hlir_is_power_of_two(val, &shift)) {
      HLIRVal *shift_val = hlir_make_iconst(fn, env, insn, shift, insn->src2->ty);
      insn->kind = HLIR_SHL;
      insn->src2 = shift_val;
      return true;
    }
    if (hlir_env_get_const(env, insn->src1, &val) && hlir_is_power_of_two(val, &shift)) {
      HLIRVal *shift_val = hlir_make_iconst(fn, env, insn, shift, insn->src1->ty);
      insn->kind = HLIR_SHL;
      insn->src1 = insn->src2;
      insn->src2 = shift_val;
      return true;
    }
  }

  // unsigned x / 2^k -> x >> k
  if (insn->kind == HLIR_DIV && insn->ty && insn->ty->is_unsigned) {
    if (hlir_env_get_const(env, insn->src2, &val) && hlir_is_power_of_two(val, &shift)) {
      HLIRVal *shift_val = hlir_make_iconst(fn, env, insn, shift, insn->src2->ty);
      insn->kind = HLIR_SHR;
      insn->src2 = shift_val;
      return true;
    }
  }

  // unsigned x % 2^k -> x & (2^k - 1)
  if (insn->kind == HLIR_MOD && insn->ty && insn->ty->is_unsigned) {
    if (hlir_env_get_const(env, insn->src2, &val) && hlir_is_power_of_two(val, NULL)) {
      HLIRVal *mask_val = hlir_make_iconst(fn, env, insn, val - 1, insn->src2->ty);
      insn->kind = HLIR_BITAND;
      insn->src2 = mask_val;
      return true;
    }
  }

  return false;
}

// Rule: Generalized Constant Re-association
static bool hlir_rule_reassociation(HLIRFunction *fn, const HLIRAlgebraicProps *props, HLIREnv *env, HLIRInsn *insn) {
  int64_t val;
  if (!hlir_env_get_const(env, insn->src2, &val))
    return false;

  HLIRInsn *def1 = hlir_env_get_def(env, insn->src1);
  if (!def1 || !def1->src2)
    return false;

  int64_t c1;
  if (!hlir_env_get_const(env, def1->src2, &c1))
    return false;

  // 1. Same associative operator: (x OP c1) OP c2 -> x OP (c1 OP c2)
  if (props && props->is_associative && def1->kind == insn->kind) {
    int64_t res;
    if (hlir_fold_binary(insn->kind, c1, val, &res)) {
      insn->src1 = def1->src1;
      insn->src2 = hlir_make_iconst(fn, env, insn, res, insn->src2->ty);
      return true;
    }
  }

  // 2. Chained shift operators: (x << c1) << c2 -> x << (c1 + c2)
  if ((insn->kind == HLIR_SHL || insn->kind == HLIR_SHR) && def1->kind == insn->kind) {
    int64_t new_shift = c1 + val;
    if (new_shift < 64) {
      insn->src1 = def1->src1;
      insn->src2 = hlir_make_iconst(fn, env, insn, new_shift, insn->src2->ty);
      return true;
    }
  }

  // 3. Mixed Additive operations (ADD and SUB)
  if (insn->kind == HLIR_ADD) {
    if (def1->kind == HLIR_ADD) {
      insn->src1 = def1->src1;
      insn->src2 = hlir_make_iconst(fn, env, insn, c1 + val, insn->src2->ty);
      return true;
    }
    if (def1->kind == HLIR_SUB) {
      insn->src1 = def1->src1;
      insn->src2 = hlir_make_iconst(fn, env, insn, val - c1, insn->src2->ty);
      return true;
    }
  } else if (insn->kind == HLIR_SUB) {
    if (def1->kind == HLIR_ADD) {
      insn->kind = HLIR_ADD;
      insn->src1 = def1->src1;
      insn->src2 = hlir_make_iconst(fn, env, insn, c1 - val, insn->src2->ty);
      return true;
    }
    if (def1->kind == HLIR_SUB) {
      insn->src1 = def1->src1;
      insn->src2 = hlir_make_iconst(fn, env, insn, c1 + val, insn->src2->ty);
      return true;
    }
  }

  return false;
}

// Rule: Generalized Cancellation
static bool hlir_rule_cancellation(HLIREnv *env, HLIRInsn *insn) {
  // Subtraction cancels addition: (x + y) - y -> x, (x + y) - x -> y
  if (insn->kind == HLIR_SUB) {
    HLIRInsn *def1 = hlir_env_get_def(env, insn->src1);
    if (def1 && def1->kind == HLIR_ADD) {
      if (def1->src2 == insn->src2) {
        hlir_set_cast(insn, def1->src1);
        return true;
      }
      if (def1->src1 == insn->src2) {
        hlir_set_cast(insn, def1->src2);
        return true;
      }
    }
  }

  // Addition cancels subtraction: (x - y) + y -> x, y + (x - y) -> x
  if (insn->kind == HLIR_ADD) {
    HLIRInsn *def1 = hlir_env_get_def(env, insn->src1);
    if (def1 && def1->kind == HLIR_SUB && def1->src2 == insn->src2) {
      hlir_set_cast(insn, def1->src1);
      return true;
    }
    HLIRInsn *def2 = hlir_env_get_def(env, insn->src2);
    if (def2 && def2->kind == HLIR_SUB && def2->src2 == insn->src1) {
      hlir_set_cast(insn, def2->src1);
      return true;
    }
  }

  // XOR cancellation: (x ^ y) ^ y -> x, (x ^ y) ^ x -> y, y ^ (x ^ y) -> x, x ^ (x ^ y) -> y
  if (insn->kind == HLIR_BITXOR) {
    HLIRInsn *def1 = hlir_env_get_def(env, insn->src1);
    if (def1 && def1->kind == HLIR_BITXOR) {
      if (def1->src2 == insn->src2) {
        hlir_set_cast(insn, def1->src1);
        return true;
      }
      if (def1->src1 == insn->src2) {
        hlir_set_cast(insn, def1->src2);
        return true;
      }
    }
    HLIRInsn *def2 = hlir_env_get_def(env, insn->src2);
    if (def2 && def2->kind == HLIR_BITXOR) {
      if (def2->src2 == insn->src1) {
        hlir_set_cast(insn, def2->src1);
        return true;
      }
      if (def2->src1 == insn->src1) {
        hlir_set_cast(insn, def2->src2);
        return true;
      }
    }
  }

  return false;
}

// Rule: Generalized Absorption
static bool hlir_rule_absorption(HLIREnv *env, HLIRInsn *insn) {
  if (insn->kind != HLIR_BITAND && insn->kind != HLIR_BITOR)
    return false;

  HLIRKind inner_kind = (insn->kind == HLIR_BITAND) ? HLIR_BITOR : HLIR_BITAND;

  // (x | y) & x -> x, (x | y) & y -> y, (x & y) | x -> x, (x & y) | y -> y
  HLIRInsn *def1 = hlir_env_get_def(env, insn->src1);
  if (def1 && def1->kind == inner_kind) {
    if (def1->src1 == insn->src2 || def1->src2 == insn->src2) {
      hlir_set_cast(insn, insn->src2);
      return true;
    }
  }

  // x & (x | y) -> x, y & (x | y) -> y, x | (x & y) -> x, y | (x & y) -> y
  HLIRInsn *def2 = hlir_env_get_def(env, insn->src2);
  if (def2 && def2->kind == inner_kind) {
    if (def2->src1 == insn->src1 || def2->src2 == insn->src1) {
      hlir_set_cast(insn, insn->src1);
      return true;
    }
  }

  return false;
}

bool hlir_opt_algebraic(HLIRFunction *fn) {
  if (!fn || fn->num_vals == 0)
    return false;

  bool changed = false;
  HLIREnv env;
  hlir_env_init(&env, fn->num_vals, true);

  for (HLIRInsn *insn = fn->head; insn; insn = insn->next) {
    if (hlir_is_bb_barrier(insn->kind)) {
      hlir_env_reset(&env);
      continue;
    }

    if (insn->dst)
      hlir_env_set_def(&env, insn->dst, insn);

    if (insn->kind == HLIR_ICONST && insn->dst) {
      hlir_env_set_const(&env, insn->dst, insn->imm);
      continue;
    }

    // 1. Unary simplifications & involutions
    if (hlir_rule_simplify_unary(&env, insn)) {
      changed = true;
      continue;
    }

    // Binary operations require dst, src1, and src2
    if (!insn->dst || !insn->src1 || !insn->src2)
      continue;

    const HLIRAlgebraicProps *props = hlir_get_algebraic_props(insn->kind);

    // 2. Identity element rule (e.g. x + 0 -> x, x * 1 -> x, x & -1 -> x, x ^ 0 -> x)
    if (hlir_rule_identity(props, &env, insn)) {
      changed = true;
      continue;
    }

    // 3. Annihilator element rule (e.g. x * 0 -> 0, x & 0 -> 0, x | -1 -> -1, x % 1 -> 0)
    if (hlir_rule_annihilator(props, &env, insn)) {
      changed = true;
      continue;
    }

    // 4. Self-operands rule (e.g. x - x -> 0, x == x -> 1, x & x -> x, x + x -> x << 1)
    if (hlir_rule_self_operands(fn, props, &env, insn)) {
      changed = true;
      continue;
    }

    // 5. Negation & sign inversion rules (e.g. x * -1 -> -x, x + (-y) -> x - y, x - (-y) -> x + y)
    if (hlir_rule_negations(&env, insn)) {
      changed = true;
      continue;
    }

    // 6. Strength reduction (e.g. x * 2^k -> x << k, unsigned x / 2^k -> x >> k, unsigned x % 2^k -> x & (2^k-1))
    if (hlir_rule_strength_reduction(fn, &env, insn)) {
      changed = true;
      continue;
    }

    // 7. Generalized constant re-association (e.g. (x OP c1) OP c2 -> x OP (c1 OP c2), mixed add/sub)
    if (hlir_rule_reassociation(fn, props, &env, insn)) {
      changed = true;
      continue;
    }

    // 8. Generalized cancellation (e.g. (x + y) - y -> x, (x - y) + y -> x, (x ^ y) ^ y -> x)
    if (hlir_rule_cancellation(&env, insn)) {
      changed = true;
      continue;
    }

    // 9. Generalized absorption (e.g. (x | y) & x -> x, (x & y) | x -> x)
    if (hlir_rule_absorption(&env, insn)) {
      changed = true;
      continue;
    }
  }

  hlir_env_free(&env);
  return changed;
}
