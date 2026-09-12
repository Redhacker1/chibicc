#include "ir/hlir_opt.h"
#include "ir/opt.h"
#include <stdlib.h>
#include <string.h>

// ==============================================================================
// 1. Instruction Classification Predicates & Properties
// ==============================================================================

typedef void (*HLIRUseCallback)(HLIRVal **use, void *data);

// Invokes callback for each read operand in the instruction.
static void hlir_for_each_use(HLIRInsn *insn, HLIRUseCallback cb, void *data) {
  if (insn->src1) cb(&insn->src1, data);
  if (insn->src2) cb(&insn->src2, data);
  if (insn->src3) cb(&insn->src3, data);
  for (int i = 0; i < insn->num_args; i++) {
    if (insn->args[i])
      cb(&insn->args[i], data);
  }
}

// Pure instructions have no side-effects, do not access memory, and do not branch.
static bool hlir_is_pure(HLIRKind kind) {
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

// Commutative binary operators where src1 and src2 can be safely swapped.
static bool hlir_is_commutative(HLIRKind kind) {
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

// Relational comparisons.
static bool hlir_is_relational(HLIRKind kind) {
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

// Swapped comparison operator when reversing operands: (A op B) <=> (B swap(op) A).
static HLIRKind hlir_swap_cmp(HLIRKind kind) {
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

// Inverted comparison operator for logical negation: !(A op B) <=> (A invert(op) B).
static HLIRKind hlir_invert_cmp(HLIRKind kind) {
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

// Basic-block boundaries and control flow barriers that invalidate straight-line state.
static bool hlir_is_bb_barrier(HLIRKind kind) {
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

// Check if value is a positive power of two and extract shift exponent.
static bool hlir_is_power_of_two(int64_t val, int *out_shift) {
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

// ==============================================================================
// 2. Instruction Modification Helpers
// ==============================================================================

static void hlir_set_iconst(HLIRInsn *insn, int64_t imm) {
  insn->kind = HLIR_ICONST;
  insn->imm = imm;
  insn->src1 = NULL;
  insn->src2 = NULL;
  insn->src3 = NULL;
}

static void hlir_set_cast(HLIRInsn *insn, HLIRVal *src) {
  insn->kind = HLIR_CAST;
  insn->src1 = src;
  insn->src2 = NULL;
  insn->src3 = NULL;
}

static void hlir_set_unary(HLIRInsn *insn, HLIRKind kind, HLIRVal *src) {
  insn->kind = kind;
  insn->src1 = src;
  insn->src2 = NULL;
  insn->src3 = NULL;
}

// ==============================================================================
// 3. Block-Local Environment Tracking (Constants & Definitions)
// ==============================================================================

typedef struct {
  int num_vals;
  int64_t *const_vals;
  bool *is_const;
  HLIRInsn **defs;
} HLIREnv;

static void hlir_env_init(HLIREnv *env, int num_vals, bool track_defs) {
  env->num_vals = num_vals;
  env->const_vals = calloc(num_vals ? num_vals : 1, sizeof(int64_t));
  env->is_const = calloc(num_vals ? num_vals : 1, sizeof(bool));
  env->defs = track_defs ? calloc(num_vals ? num_vals : 1, sizeof(HLIRInsn *)) : NULL;
}

static void hlir_env_reset(HLIREnv *env) {
  if (env->num_vals > 0) {
    memset(env->is_const, 0, env->num_vals * sizeof(bool));
    if (env->defs)
      memset(env->defs, 0, env->num_vals * sizeof(HLIRInsn *));
  }
}

static void hlir_env_free(HLIREnv *env) {
  free(env->const_vals);
  free(env->is_const);
  if (env->defs) free(env->defs);
}

static void hlir_env_set_const(HLIREnv *env, HLIRVal *val, int64_t imm) {
  if (val && val->id < env->num_vals) {
    env->is_const[val->id] = true;
    env->const_vals[val->id] = imm;
  }
}

static void hlir_env_set_def(HLIREnv *env, HLIRVal *val, HLIRInsn *insn) {
  if (env->defs && val && val->id < env->num_vals) {
    env->defs[val->id] = insn;
  }
}

static bool hlir_env_get_const(HLIREnv *env, HLIRVal *val, int64_t *out_imm) {
  if (val && val->id < env->num_vals && env->is_const[val->id]) {
    if (out_imm) *out_imm = env->const_vals[val->id];
    return true;
  }
  return false;
}

static bool hlir_env_is_const(HLIREnv *env, HLIRVal *val) {
  return val && val->id < env->num_vals && env->is_const[val->id];
}

static bool hlir_env_is_const_val(HLIREnv *env, HLIRVal *val, int64_t imm) {
  return val && val->id < env->num_vals && env->is_const[val->id] && env->const_vals[val->id] == imm;
}

static HLIRInsn *hlir_env_get_def(HLIREnv *env, HLIRVal *val) {
  if (env->defs && val && val->id < env->num_vals)
    return env->defs[val->id];
  return NULL;
}

// ==============================================================================
// 4. Constant Folding Evaluation Helpers
// ==============================================================================

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

// ==============================================================================
// 5. HLIR Optimization Passes
// ==============================================================================

// Pass 1: Constant Folding & Operand Canonicalization
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
    if (hlir_env_is_const(&env, insn->src1) && insn->src2 && !hlir_env_is_const(&env, insn->src2)) {
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

// ==============================================================================
// 5. Algebraic Properties & Generalized Simplification Rules
// ==============================================================================

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

  if ((props->identity_side & OP_SIDE_RIGHT) && hlir_env_is_const_val(env, insn->src2, props->identity)) {
    hlir_set_cast(insn, insn->src1);
    return true;
  }
  if ((props->identity_side & OP_SIDE_LEFT) && hlir_env_is_const_val(env, insn->src1, props->identity)) {
    hlir_set_cast(insn, insn->src2);
    return true;
  }
  return false;
}

// Rule: Annihilator element simplification (e.g. x * 0 -> 0, x & 0 -> 0, x | -1 -> -1)
static bool hlir_rule_annihilator(const HLIRAlgebraicProps *props, HLIREnv *env, HLIRInsn *insn) {
  if (!props || !props->has_annihilator)
    return false;

  if (((props->annihilator_side & OP_SIDE_RIGHT) && hlir_env_is_const_val(env, insn->src2, props->annihilator)) ||
      ((props->annihilator_side & OP_SIDE_LEFT) && hlir_env_is_const_val(env, insn->src1, props->annihilator))) {
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
  // Constant sign inversion: x * -1 -> -x, -1 * x -> -x
  if (insn->kind == HLIR_MUL) {
    if (hlir_env_is_const_val(env, insn->src2, -1)) {
      hlir_set_unary(insn, HLIR_NEG, insn->src1);
      return true;
    }
    if (hlir_env_is_const_val(env, insn->src1, -1)) {
      hlir_set_unary(insn, HLIR_NEG, insn->src2);
      return true;
    }
  }

  // Bitwise not via XOR with -1: x ^ -1 -> ~x, -1 ^ x -> ~x
  if (insn->kind == HLIR_BITXOR) {
    if (hlir_env_is_const_val(env, insn->src2, -1)) {
      hlir_set_unary(insn, HLIR_BITNOT, insn->src1);
      return true;
    }
    if (hlir_env_is_const_val(env, insn->src1, -1)) {
      hlir_set_unary(insn, HLIR_BITNOT, insn->src2);
      return true;
    }
  }

  // Negation from 0 - x -> -x
  if (insn->kind == HLIR_SUB && hlir_env_is_const_val(env, insn->src1, 0)) {
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
      // (x + c1) + c2 = x + (c1 + c2)
      insn->src1 = def1->src1;
      insn->src2 = hlir_make_iconst(fn, env, insn, c1 + val, insn->src2->ty);
      return true;
    }
    if (def1->kind == HLIR_SUB) {
      // (x - c1) + c2 = x + (val - c1)
      insn->src1 = def1->src1;
      insn->src2 = hlir_make_iconst(fn, env, insn, val - c1, insn->src2->ty);
      return true;
    }
  } else if (insn->kind == HLIR_SUB) {
    if (def1->kind == HLIR_ADD) {
      // (x + c1) - c2 = x + (c1 - val)
      insn->kind = HLIR_ADD;
      insn->src1 = def1->src1;
      insn->src2 = hlir_make_iconst(fn, env, insn, c1 - val, insn->src2->ty);
      return true;
    }
    if (def1->kind == HLIR_SUB) {
      // (x - c1) - c2 = x - (c1 + val)
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

// Pass 2: Algebraic Identities & Strength Reduction
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

// Pass 3: Copy Propagation
static HLIRVal *hlir_resolve_alias(HLIRVal **aliases, HLIRVal *val, int num_vals) {
  if (!val) return NULL;
  HLIRVal *root = val;
  while (root && root->id < num_vals && aliases[root->id])
    root = aliases[root->id];
  return root;
}

static void hlir_copy_prop_apply(HLIRVal **use, void *data) {
  HLIRVal **aliases = (HLIRVal **)data;
  HLIRVal *v = *use;
  if (v && aliases[v->id])
    *use = aliases[v->id];
}

bool hlir_opt_copy_prop(HLIRFunction *fn) {
  if (!fn || fn->num_vals == 0)
    return false;

  bool changed = false;
  HLIRVal **aliases = calloc(fn->num_vals, sizeof(HLIRVal *));

  for (HLIRInsn *insn = fn->head; insn; insn = insn->next) {
    if (hlir_is_bb_barrier(insn->kind)) {
      memset(aliases, 0, fn->num_vals * sizeof(HLIRVal *));
      continue;
    }

    HLIRVal *s1 = insn->src1;
    HLIRVal *s2 = insn->src2;
    HLIRVal *s3 = insn->src3;
    hlir_for_each_use(insn, hlir_copy_prop_apply, aliases);
    if (insn->src1 != s1 || insn->src2 != s2 || insn->src3 != s3)
      changed = true;

    if (insn->dst && insn->dst->id < fn->num_vals) {
      aliases[insn->dst->id] = NULL;
      for (int i = 0; i < fn->num_vals; i++) {
        if (aliases[i] == insn->dst)
          aliases[i] = NULL;
      }
    }

    if (insn->kind == HLIR_CAST && insn->dst && insn->src1) {
      Type *t1 = insn->dst->ty;
      Type *t2 = insn->src1->ty;
      if (t1 && t2 && t1->size == t2->size &&
          is_flonum(t1) == is_flonum(t2) &&
          t1->is_unsigned == t2->is_unsigned) {
        HLIRVal *root = hlir_resolve_alias(aliases, insn->src1, fn->num_vals);
        if (root && root != insn->dst && insn->dst->id < fn->num_vals)
          aliases[insn->dst->id] = root;
      }
    }
  }

  free(aliases);
  return changed;
}

// Pass 4: Local Common Subexpression Elimination (CSE)
static bool hlir_insn_match_cse(HLIRInsn *a, HLIRInsn *b) {
  if (a->kind != b->kind)
    return false;

  // Constant / symbol kinds
  if (a->kind == HLIR_ICONST) return a->imm == b->imm;
  if (a->kind == HLIR_FCONST) return a->fimm == b->fimm;
  if (a->kind == HLIR_SCONST) return (a->label && b->label && !strcmp(a->label, b->label)) || (a->var == b->var);
  if (a->kind == HLIR_ADDR_VAR) return a->var == b->var;

  // Binary operations (commutative vs non-commutative)
  if (a->src1 && a->src2 && b->src1 && b->src2) {
    if (a->src1 == b->src1 && a->src2 == b->src2)
      return true;
    if (hlir_is_commutative(a->kind) && a->src1 == b->src2 && a->src2 == b->src1)
      return true;
    return false;
  }

  // Unary operations
  if (a->src1 && !a->src2 && b->src1 && !b->src2)
    return a->src1 == b->src1;

  return false;
}

bool hlir_opt_local_cse(HLIRFunction *fn) {
  if (!fn || !fn->head)
    return false;

  bool changed = false;

  for (HLIRInsn *insn = fn->head; insn; insn = insn->next) {
    if (!insn->dst || !hlir_is_pure(insn->kind) || insn->kind == HLIR_CAST)
      continue;

    for (HLIRInsn *sub = insn->next; sub; sub = sub->next) {
      if (hlir_is_bb_barrier(sub->kind))
        break;

      if (sub->dst && hlir_insn_match_cse(insn, sub)) {
        hlir_set_cast(sub, insn->dst);
        changed = true;
      }
    }
  }

  return changed;
}

// Pass 5: Local Memory Tracking (Store Forwarding & Dead Store Elimination)
typedef struct {
  Obj *var;
  HLIRVal *avail_val;
  HLIRInsn *last_store;
} HLIRLocalVarEntry;

typedef struct {
  HLIRLocalVarEntry *entries;
  int count;
  int cap;
} HLIRLocalMemEnv;

static void hlir_local_mem_reset(HLIRLocalMemEnv *env) {
  env->count = 0;
}

static HLIRLocalVarEntry *hlir_local_mem_find(HLIRLocalMemEnv *env, Obj *var) {
  for (int i = 0; i < env->count; i++) {
    if (env->entries[i].var == var)
      return &env->entries[i];
  }
  return NULL;
}

static HLIRLocalVarEntry *hlir_local_mem_get_or_add(HLIRLocalMemEnv *env, Obj *var) {
  HLIRLocalVarEntry *e = hlir_local_mem_find(env, var);
  if (e) return e;
  if (env->count >= env->cap) {
    env->cap = env->cap ? env->cap * 2 : 16;
    env->entries = realloc(env->entries, sizeof(HLIRLocalVarEntry) * env->cap);
  }
  e = &env->entries[env->count++];
  e->var = var;
  e->avail_val = NULL;
  e->last_store = NULL;
  return e;
}

static void hlir_local_mem_remove(HLIRLocalMemEnv *env, Obj *var) {
  for (int i = 0; i < env->count; i++) {
    if (env->entries[i].var == var) {
      env->entries[i] = env->entries[--env->count];
      return;
    }
  }
}

bool hlir_opt_load_store(HLIRFunction *fn) {
  if (!fn || !fn->head)
    return false;

  bool changed = false;
  HLIRLocalMemEnv env = {0};

  for (HLIRInsn *insn = fn->head; insn; insn = insn->next) {
    if (hlir_is_bb_barrier(insn->kind) || insn->kind == HLIR_STORE_PTR ||
        insn->kind == HLIR_MEMCPY || insn->kind == HLIR_MEMZERO) {
      hlir_local_mem_reset(&env);
      continue;
    }

    if (insn->kind == HLIR_ADDR_VAR && insn->var) {
      hlir_local_mem_remove(&env, insn->var);
      continue;
    }

    if (insn->kind == HLIR_STORE_VAR && insn->var && insn->src1) {
      Obj *var = insn->var;
      HLIRLocalVarEntry *entry = hlir_local_mem_find(&env, var);
      if (entry && entry->last_store && var->is_local) {
        // Dead Store Elimination: previous store was never read
        HLIRInsn *prev_store = entry->last_store;
        hlir_remove_insn(fn, prev_store);
        changed = true;
      }
      entry = hlir_local_mem_get_or_add(&env, var);
      entry->avail_val = insn->src1;
      entry->last_store = var->is_local ? insn : NULL;
      continue;
    }

    if (insn->kind == HLIR_LOAD_VAR && insn->var && insn->dst) {
      Obj *var = insn->var;
      HLIRLocalVarEntry *entry = hlir_local_mem_find(&env, var);
      if (entry && entry->avail_val) {
        // Store-to-Load and Load-to-Load forwarding
        hlir_set_cast(insn, entry->avail_val);
        entry->last_store = NULL;
        changed = true;
      } else {
        entry = hlir_local_mem_get_or_add(&env, var);
        entry->avail_val = insn->dst;
        entry->last_store = NULL;
      }
      continue;
    }
  }

  free(env.entries);
  return changed;
}

// Pass 6: Dead Branch Elimination & Control Flow Simplification
static char *hlir_find_forwarded_label(HLIRFunction *fn, const char *label) {
  if (!label) return NULL;
  for (HLIRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind == HLIR_LABEL && insn->label && !strcmp(insn->label, label)) {
      HLIRInsn *nxt = insn->next;
      while (nxt && nxt->kind == HLIR_NOP)
        nxt = nxt->next;
      if (nxt && nxt->kind == HLIR_JMP && nxt->label && strcmp(nxt->label, label) != 0)
        return nxt->label;
      break;
    }
  }
  return NULL;
}

bool hlir_opt_control_flow(HLIRFunction *fn) {
  if (!fn || !fn->head)
    return false;

  bool changed = false;
  HLIREnv env;
  hlir_env_init(&env, fn->num_vals, true);

  for (HLIRInsn *insn = fn->head; insn; insn = insn->next) {
    // 1. Eliminate jump to immediate next label
    if (insn->kind == HLIR_JMP && insn->label) {
      HLIRInsn *nxt = insn->next;
      while (nxt && nxt->kind == HLIR_NOP)
        nxt = nxt->next;
      if (nxt && nxt->kind == HLIR_LABEL && nxt->label && !strcmp(insn->label, nxt->label)) {
        HLIRInsn *del = insn;
        insn = insn->prev ? insn->prev : fn->head;
        hlir_remove_insn(fn, del);
        changed = true;
        hlir_env_reset(&env);
        if (!insn) break;
        continue;
      }
    }

    // 2. Jump threading (target label forwarding)
    if ((insn->kind == HLIR_JMP || insn->kind == HLIR_JMP_IF_ZERO || insn->kind == HLIR_JMP_IF_NZ) && insn->label) {
      char *forwarded = hlir_find_forwarded_label(fn, insn->label);
      if (forwarded) {
        insn->label = forwarded;
        changed = true;
      }
    }

    if (insn->kind == HLIR_LABEL || insn->kind == HLIR_JMP || insn->kind == HLIR_CALL) {
      hlir_env_reset(&env);
      continue;
    }

    if (insn->dst)
      hlir_env_set_def(&env, insn->dst, insn);

    if (insn->kind == HLIR_ICONST && insn->dst) {
      hlir_env_set_const(&env, insn->dst, insn->imm);
      continue;
    }

    // 3. Branch on constant values
    int64_t c;
    if ((insn->kind == HLIR_JMP_IF_ZERO || insn->kind == HLIR_JMP_IF_NZ) &&
        hlir_env_get_const(&env, insn->src1, &c)) {
      bool taken = (insn->kind == HLIR_JMP_IF_ZERO) ? (c == 0) : (c != 0);
      if (taken) {
        insn->kind = HLIR_JMP;
        insn->src1 = NULL;
        changed = true;
      } else {
        HLIRInsn *del = insn;
        insn = insn->prev ? insn->prev : fn->head;
        hlir_remove_insn(fn, del);
        changed = true;
        if (!insn) break;
        continue;
      }
    }

    // 4. Conditional branch simplification on logical/relational inversions
    if ((insn->kind == HLIR_JMP_IF_ZERO || insn->kind == HLIR_JMP_IF_NZ) && insn->src1) {
      HLIRInsn *def = hlir_env_get_def(&env, insn->src1);
      if (def) {
        if (def->kind == HLIR_LOGNOT && def->src1) {
          // JMP_IF_ZERO (!x) -> JMP_IF_NZ (x); JMP_IF_NZ (!x) -> JMP_IF_ZERO (x)
          insn->kind = (insn->kind == HLIR_JMP_IF_ZERO) ? HLIR_JMP_IF_NZ : HLIR_JMP_IF_ZERO;
          insn->src1 = def->src1;
          changed = true;
        } else if (def->src1 && def->src2) {
          bool right_zero = hlir_env_is_const_val(&env, def->src2, 0);
          bool left_zero = hlir_env_is_const_val(&env, def->src1, 0);
          if (def->kind == HLIR_CMP_EQ && (right_zero || left_zero)) {
            insn->kind = (insn->kind == HLIR_JMP_IF_ZERO) ? HLIR_JMP_IF_NZ : HLIR_JMP_IF_ZERO;
            insn->src1 = right_zero ? def->src1 : def->src2;
            changed = true;
          } else if (def->kind == HLIR_CMP_NE && (right_zero || left_zero)) {
            insn->src1 = right_zero ? def->src1 : def->src2;
            changed = true;
          }
        }
      }
    }
  }

  hlir_env_free(&env);
  return changed;
}

// Pass 7: Dead Value Elimination (DCE)
static void hlir_dce_mark_use(HLIRVal **use, void *data) {
  bool *used = (bool *)data;
  if (*use)
    used[(*use)->id] = true;
}

bool hlir_opt_dce(HLIRFunction *fn) {
  if (!fn || fn->num_vals == 0)
    return false;

  bool changed = false;
  bool *used = calloc(fn->num_vals, sizeof(bool));

  for (HLIRInsn *insn = fn->head; insn; insn = insn->next)
    hlir_for_each_use(insn, hlir_dce_mark_use, used);

  for (HLIRInsn *insn = fn->head; insn;) {
    HLIRInsn *next = insn->next;

    if (insn->dst && insn->dst->id < fn->num_vals && !used[insn->dst->id]) {
      if (hlir_is_pure(insn->kind)) {
        hlir_remove_insn(fn, insn);
        changed = true;
      }
    }

    insn = next;
  }

  free(used);
  return changed;
}

// Pass 8: Dead / Unreachable Code Elimination
bool hlir_opt_dead_code(HLIRFunction *fn) {
  if (!fn || !fn->head)
    return false;

  bool changed = false;

  // Eliminate instructions after unconditional jump or return until the next label
  for (HLIRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind == HLIR_JMP || insn->kind == HLIR_RET) {
      while (insn->next && insn->next->kind != HLIR_LABEL) {
        hlir_remove_insn(fn, insn->next);
        changed = true;
      }
    }
  }

  return changed;
}

// Pass 9: Function Inlining
bool hlir_opt_inlining(HLIRProg *prog) {
  if (!prog || prog->num_fns == 0)
    return false;

  bool changed = false;

  for (int i = 0; i < prog->num_fns; i++) {
    HLIRFunction *caller = prog->fns[i];
    if (!caller) continue;

    for (HLIRInsn *insn = caller->head; insn; insn = insn->next) {
      if (insn->kind != HLIR_CALL || !insn->src1 || !insn->src1->var)
        continue;

      Obj *callee_obj = insn->src1->var;
      if (!callee_obj->is_function || !callee_obj->is_definition)
        continue;

      HLIRFunction *callee = NULL;
      for (int j = 0; j < prog->num_fns; j++) {
        if (prog->fns[j]->fn_obj == callee_obj) {
          callee = prog->fns[j];
          break;
        }
      }

      if (!callee || callee == caller || callee->num_insns > 20)
        continue;

      int ret_count = 0;
      HLIRInsn *last_ret = NULL;
      for (HLIRInsn *ci = callee->head; ci; ci = ci->next) {
        if (ci->kind == HLIR_RET) {
          ret_count++;
          last_ret = ci;
        }
        if (ci->kind == HLIR_ASM || ci->kind == HLIR_ALLOCA) {
          ret_count = 999;
          break;
        }
      }

      if (ret_count != 1 || !last_ret || last_ret != callee->tail)
        continue;

      HLIRVal **val_map = calloc(callee->num_vals, sizeof(HLIRVal *));
      for (int v = 0; v < callee->num_vals; v++) {
        val_map[v] = hlir_new_val(caller, callee->vals[v]->ty);
        val_map[v]->var = callee->vals[v]->var;
      }

      Obj *param = callee->params;
      for (int a = 0; a < insn->num_args && param; a++, param = param->next) {
        HLIRInsn *param_store = hlir_new_insn(HLIR_STORE_VAR);
        param_store->var = param;
        param_store->src1 = insn->args[a];
        param_store->ty = param->ty;
        hlir_insert_before(caller, insn, param_store);
      }

      for (HLIRInsn *ci = callee->head; ci != last_ret; ci = ci->next) {
        HLIRInsn *cloned = hlir_new_insn(ci->kind);
        cloned->dst = ci->dst ? val_map[ci->dst->id] : NULL;
        cloned->src1 = ci->src1 ? val_map[ci->src1->id] : NULL;
        cloned->src2 = ci->src2 ? val_map[ci->src2->id] : NULL;
        cloned->src3 = ci->src3 ? val_map[ci->src3->id] : NULL;
        cloned->imm = ci->imm;
        cloned->fimm = ci->fimm;
        cloned->label = ci->label ? format("%s.inl.%d", ci->label, caller->num_insns) : NULL;
        cloned->var = ci->var;
        cloned->ty = ci->ty;
        cloned->num_args = ci->num_args;
        if (ci->num_args > 0 && ci->args) {
          cloned->args = calloc(ci->num_args, sizeof(HLIRVal *));
          for (int a = 0; a < ci->num_args; a++)
            cloned->args[a] = ci->args[a] ? val_map[ci->args[a]->id] : NULL;
        }
        cloned->asm_str = ci->asm_str;
        hlir_insert_before(caller, insn, cloned);
      }

      if (insn->dst && last_ret->src1) {
        HLIRInsn *ret_mov = hlir_new_insn(HLIR_CAST);
        ret_mov->dst = insn->dst;
        ret_mov->src1 = val_map[last_ret->src1->id];
        ret_mov->ty = insn->ty;
        hlir_insert_before(caller, insn, ret_mov);
      }

      HLIRInsn *to_remove = insn;
      insn = insn->prev ? insn->prev : caller->head;
      hlir_remove_insn(caller, to_remove);
      free(val_map);
      changed = true;
      if (!insn) break;
    }
  }

  return changed;
}

// ==============================================================================
// 6. Fixed-Point Optimization Pipeline Driver
// ==============================================================================

void hlir_optimize(HLIRProg *prog, int opt_level) {
  if (!prog)
    return;

  ir_init_pass_registry();

  int max_iter = (opt_level > 0) ? opt_max_passes : 1;

  if (opt_level >= 2) {
    hlir_opt_inlining(prog);
  }

  for (int i = 0; i < prog->num_fns; i++)
  {
    HLIRFunction *fn = prog->fns[i];
    if (!fn) continue;

    bool changed = true;


    // On higher performance machines, we can just do this indefinitely (although might be worth say stop at a certain number of iterations)

    const int total_max_iters = 500; // if we get this many iterations and are still changing, we are probably in some loop
    int iterCount = 0;
    do
    {
      changed = false;

      if (iterCount == total_max_iters)
        break;

      iterCount++;

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
    } while (changed);


    /*
    //for (int iter = 0; iter < max_iter; iter++)
    {

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

    if (changed) {
      printf("May be worth increasing optimization iteration count for function %s\n", fn->name);
      fflush(stdout);
    }
    */

  }
}
