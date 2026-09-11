#include "ir/hlir_opt.h"
#include "ir/opt.h"
#include <stdlib.h>
#include <string.h>

// 1. HLIR Constant Folding Pass
bool hlir_opt_const_fold(HLIRFunction *fn) {
  if (!fn || fn->num_vals == 0)
    return false;

  bool changed = false;
  int64_t *const_vals = calloc(fn->num_vals, sizeof(int64_t));
  bool *is_const = calloc(fn->num_vals, sizeof(bool));

  for (HLIRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind == HLIR_LABEL || insn->kind == HLIR_JMP ||
        insn->kind == HLIR_JMP_IF_ZERO || insn->kind == HLIR_JMP_IF_NZ ||
        insn->kind == HLIR_CALL) {
      memset(is_const, 0, fn->num_vals * sizeof(bool));
      continue;
    }

    if (insn->kind == HLIR_ICONST && insn->dst) {
      is_const[insn->dst->id] = true;
      const_vals[insn->dst->id] = insn->imm;
      continue;
    }

    if (insn->src1 && is_const[insn->src1->id] && insn->src2 && is_const[insn->src2->id]) {
      int64_t c1 = const_vals[insn->src1->id];
      int64_t c2 = const_vals[insn->src2->id];
      int64_t res = 0;
      bool folded = true;

      switch (insn->kind) {
      case HLIR_ADD: res = c1 + c2; break;
      case HLIR_SUB: res = c1 - c2; break;
      case HLIR_MUL: res = c1 * c2; break;
      case HLIR_DIV: if (c2 != 0) res = c1 / c2; else folded = false; break;
      case HLIR_MOD: if (c2 != 0) res = c1 % c2; else folded = false; break;
      case HLIR_BITAND: res = c1 & c2; break;
      case HLIR_BITOR:  res = c1 | c2; break;
      case HLIR_BITXOR: res = c1 ^ c2; break;
      case HLIR_SHL: if (c2 >= 0 && c2 < 64) res = c1 << c2; else folded = false; break;
      case HLIR_SHR: if (c2 >= 0 && c2 < 64) res = c1 >> c2; else folded = false; break;
      case HLIR_CMP_EQ: res = (c1 == c2); break;
      case HLIR_CMP_NE: res = (c1 != c2); break;
      case HLIR_CMP_LT: res = (c1 < c2); break;
      case HLIR_CMP_LE: res = (c1 <= c2); break;
      case HLIR_CMP_GT: res = (c1 > c2); break;
      case HLIR_CMP_GE: res = (c1 >= c2); break;
      default: folded = false; break;
      }

      if (folded && insn->dst) {
        insn->kind = HLIR_ICONST;
        insn->imm = res;
        insn->src1 = NULL;
        insn->src2 = NULL;
        is_const[insn->dst->id] = true;
        const_vals[insn->dst->id] = res;
        changed = true;
        continue;
      }
    }

    if (insn->src1 && is_const[insn->src1->id] && !insn->src2 && insn->dst) {
      int64_t c = const_vals[insn->src1->id];
      int64_t res = 0;
      bool folded = true;

      switch (insn->kind) {
      case HLIR_NEG: res = -c; break;
      case HLIR_BITNOT: res = ~c; break;
      case HLIR_LOGNOT: res = !c; break;
      case HLIR_CAST: {
        if (insn->ty && is_flonum(insn->ty)) {
          folded = false;
        } else {
          int sz = insn->ty ? insn->ty->size : 8;
          bool is_unsigned = insn->ty ? insn->ty->is_unsigned : false;
          res = c;
          if (sz == 1) res = is_unsigned ? (uint8_t)c : (int8_t)c;
          else if (sz == 2) res = is_unsigned ? (uint16_t)c : (int16_t)c;
          else if (sz == 4) res = is_unsigned ? (uint32_t)c : (int32_t)c;
        }
        break;
      }
      default: folded = false; break;
      }

      if (folded) {
        insn->kind = HLIR_ICONST;
        insn->imm = res;
        insn->src1 = NULL;
        is_const[insn->dst->id] = true;
        const_vals[insn->dst->id] = res;
        changed = true;
        continue;
      }
    }

    // Canonicalize: place constant in src2 for commutative and relational operations
    if (insn->src1 && is_const[insn->src1->id] && insn->src2 && !is_const[insn->src2->id]) {
      if (insn->kind == HLIR_ADD || insn->kind == HLIR_MUL ||
          insn->kind == HLIR_BITAND || insn->kind == HLIR_BITOR || insn->kind == HLIR_BITXOR ||
          insn->kind == HLIR_CMP_EQ || insn->kind == HLIR_CMP_NE) {
        HLIRVal *tmp = insn->src1;
        insn->src1 = insn->src2;
        insn->src2 = tmp;
        changed = true;
      } else if (insn->kind == HLIR_CMP_LT) {
        insn->kind = HLIR_CMP_GT;
        HLIRVal *tmp = insn->src1;
        insn->src1 = insn->src2;
        insn->src2 = tmp;
        changed = true;
      } else if (insn->kind == HLIR_CMP_LE) {
        insn->kind = HLIR_CMP_GE;
        HLIRVal *tmp = insn->src1;
        insn->src1 = insn->src2;
        insn->src2 = tmp;
        changed = true;
      } else if (insn->kind == HLIR_CMP_GT) {
        insn->kind = HLIR_CMP_LT;
        HLIRVal *tmp = insn->src1;
        insn->src1 = insn->src2;
        insn->src2 = tmp;
        changed = true;
      } else if (insn->kind == HLIR_CMP_GE) {
        insn->kind = HLIR_CMP_LE;
        HLIRVal *tmp = insn->src1;
        insn->src1 = insn->src2;
        insn->src2 = tmp;
        changed = true;
      }
    }
  }

  free(const_vals);
  free(is_const);
  return changed;
}

// 2. HLIR Constant & Algebraic Optimizations Pass
bool hlir_opt_algebraic(HLIRFunction *fn) {
  if (!fn || fn->num_vals == 0)
    return false;

  bool changed = false;
  int orig_vals = fn->num_vals;
  int64_t *const_vals = calloc(orig_vals, sizeof(int64_t));
  bool *is_const = calloc(orig_vals, sizeof(bool));
  HLIRInsn **defs = calloc(orig_vals, sizeof(HLIRInsn *));

  for (HLIRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind == HLIR_LABEL || insn->kind == HLIR_JMP ||
        insn->kind == HLIR_JMP_IF_ZERO || insn->kind == HLIR_JMP_IF_NZ ||
        insn->kind == HLIR_CALL) {
      memset(is_const, 0, orig_vals * sizeof(bool));
      memset(defs, 0, orig_vals * sizeof(HLIRInsn *));
      continue;
    }

    if (insn->dst && insn->dst->id < orig_vals) {
      defs[insn->dst->id] = insn;
    }

    if (insn->kind == HLIR_ICONST && insn->dst && insn->dst->id < orig_vals) {
      is_const[insn->dst->id] = true;
      const_vals[insn->dst->id] = insn->imm;
      continue;
    }

    // Double unary operations: -(-x) = x, ~(~x) = x
    if (insn->kind == HLIR_NEG && insn->dst && insn->src1 && insn->src1->id < orig_vals && defs[insn->src1->id] && defs[insn->src1->id]->kind == HLIR_NEG) {
      insn->kind = HLIR_CAST;
      insn->src1 = defs[insn->src1->id]->src1;
      changed = true;
    } else if (insn->kind == HLIR_BITNOT && insn->dst && insn->src1 && insn->src1->id < orig_vals && defs[insn->src1->id] && defs[insn->src1->id]->kind == HLIR_BITNOT) {
      insn->kind = HLIR_CAST;
      insn->src1 = defs[insn->src1->id]->src1;
      changed = true;
    } else if (insn->kind == HLIR_LOGNOT && insn->dst && insn->src1 && insn->src1->id < orig_vals && defs[insn->src1->id]) {
      HLIRInsn *def = defs[insn->src1->id];
      if (def->kind == HLIR_CMP_EQ) {
        insn->kind = HLIR_CMP_NE; insn->src1 = def->src1; insn->src2 = def->src2; changed = true;
      } else if (def->kind == HLIR_CMP_NE) {
        insn->kind = HLIR_CMP_EQ; insn->src1 = def->src1; insn->src2 = def->src2; changed = true;
      } else if (def->kind == HLIR_CMP_LT) {
        insn->kind = HLIR_CMP_GE; insn->src1 = def->src1; insn->src2 = def->src2; changed = true;
      } else if (def->kind == HLIR_CMP_LE) {
        insn->kind = HLIR_CMP_GT; insn->src1 = def->src1; insn->src2 = def->src2; changed = true;
      } else if (def->kind == HLIR_CMP_GT) {
        insn->kind = HLIR_CMP_LE; insn->src1 = def->src1; insn->src2 = def->src2; changed = true;
      } else if (def->kind == HLIR_CMP_GE) {
        insn->kind = HLIR_CMP_LT; insn->src1 = def->src1; insn->src2 = def->src2; changed = true;
      }
    }

    // x + 0 = x, 0 + x = x
    if (insn->kind == HLIR_ADD && insn->dst && insn->src1 && insn->src2) {
      if (insn->src2->id < orig_vals && is_const[insn->src2->id] && const_vals[insn->src2->id] == 0) {
        insn->kind = HLIR_CAST;
        insn->src2 = NULL;
        changed = true;
      } else if (insn->src1->id < orig_vals && is_const[insn->src1->id] && const_vals[insn->src1->id] == 0) {
        insn->kind = HLIR_CAST;
        insn->src1 = insn->src2;
        insn->src2 = NULL;
        changed = true;
      }
    }
    // x - 0 = x, 0 - x = -x, x - x = 0
    else if (insn->kind == HLIR_SUB && insn->dst && insn->src1 && insn->src2) {
      if (insn->src2->id < orig_vals && is_const[insn->src2->id] && const_vals[insn->src2->id] == 0) {
        insn->kind = HLIR_CAST;
        insn->src2 = NULL;
        changed = true;
      } else if (insn->src1->id < orig_vals && is_const[insn->src1->id] && const_vals[insn->src1->id] == 0) {
        insn->kind = HLIR_NEG;
        insn->src1 = insn->src2;
        insn->src2 = NULL;
        changed = true;
      } else if (insn->src1 == insn->src2) {
        insn->kind = HLIR_ICONST;
        insn->imm = 0;
        insn->src1 = insn->src2 = NULL;
        if (insn->dst->id < orig_vals) {
          is_const[insn->dst->id] = true;
          const_vals[insn->dst->id] = 0;
        }
        changed = true;
      }
    }
    // x * 1 = x, 1 * x = x, x * 0 = 0, 0 * x = 0, x * -1 = -x, -1 * x = -x
    else if (insn->kind == HLIR_MUL && insn->dst && insn->src1 && insn->src2) {
      if (insn->src2->id < orig_vals && is_const[insn->src2->id] && const_vals[insn->src2->id] == 1) {
        insn->kind = HLIR_CAST;
        insn->src2 = NULL;
        changed = true;
      } else if (insn->src1->id < orig_vals && is_const[insn->src1->id] && const_vals[insn->src1->id] == 1) {
        insn->kind = HLIR_CAST;
        insn->src1 = insn->src2;
        insn->src2 = NULL;
        changed = true;
      } else if (insn->src2->id < orig_vals && is_const[insn->src2->id] && const_vals[insn->src2->id] == -1) {
        insn->kind = HLIR_NEG;
        insn->src2 = NULL;
        changed = true;
      } else if (insn->src1->id < orig_vals && is_const[insn->src1->id] && const_vals[insn->src1->id] == -1) {
        insn->kind = HLIR_NEG;
        insn->src1 = insn->src2;
        insn->src2 = NULL;
        changed = true;
      } else if ((insn->src2->id < orig_vals && is_const[insn->src2->id] && const_vals[insn->src2->id] == 0) ||
                 (insn->src1->id < orig_vals && is_const[insn->src1->id] && const_vals[insn->src1->id] == 0)) {
        insn->kind = HLIR_ICONST;
        insn->imm = 0;
        insn->src1 = insn->src2 = NULL;
        if (insn->dst->id < orig_vals) {
          is_const[insn->dst->id] = true;
          const_vals[insn->dst->id] = 0;
        }
        changed = true;
      } else if (insn->src2->id < orig_vals && is_const[insn->src2->id] && (!insn->ty || !is_flonum(insn->ty))) {
        // Strength reduction: multiply by power of 2 -> shift left
        int64_t val = const_vals[insn->src2->id];
        if (val > 0 && (val & (val - 1)) == 0) {
          int shift = 0;
          while ((1LL << shift) < val)
            shift++;
          if ((1LL << shift) == val) {
            HLIRVal *shift_val = hlir_new_val(fn, insn->src2->ty);
            HLIRInsn *imm_insn = hlir_new_insn(HLIR_ICONST);
            imm_insn->dst = shift_val;
            imm_insn->imm = shift;
            hlir_insert_before(fn, insn, imm_insn);

            insn->kind = HLIR_SHL;
            insn->src2 = shift_val;
            changed = true;
          }
        }
      }
    }
    // x / 1 = x, x / x = 1, unsigned x / (2^k) = x >> k
    else if (insn->kind == HLIR_DIV && insn->dst && insn->src1 && insn->src2) {
      if (insn->src2->id < orig_vals && is_const[insn->src2->id] && const_vals[insn->src2->id] == 1) {
        insn->kind = HLIR_CAST;
        insn->src2 = NULL;
        changed = true;
      } else if (insn->src1 == insn->src2) {
        insn->kind = HLIR_ICONST;
        insn->imm = 1;
        insn->src1 = insn->src2 = NULL;
        if (insn->dst->id < orig_vals) {
          is_const[insn->dst->id] = true;
          const_vals[insn->dst->id] = 1;
        }
        changed = true;
      } else if (insn->src2->id < orig_vals && is_const[insn->src2->id] &&
                 insn->ty && insn->ty->is_unsigned) {
        int64_t val = const_vals[insn->src2->id];
        if (val > 0 && (val & (val - 1)) == 0) {
          int shift = 0;
          while ((1LL << shift) < val)
            shift++;
          if ((1LL << shift) == val) {
            HLIRVal *shift_val = hlir_new_val(fn, insn->src2->ty);
            HLIRInsn *imm_insn = hlir_new_insn(HLIR_ICONST);
            imm_insn->dst = shift_val;
            imm_insn->imm = shift;
            hlir_insert_before(fn, insn, imm_insn);

            insn->kind = HLIR_SHR;
            insn->src2 = shift_val;
            changed = true;
          }
        }
      }
    }
    // unsigned x % (2^k) = x & (2^k - 1), x % 1 = 0
    else if (insn->kind == HLIR_MOD && insn->dst && insn->src1 && insn->src2) {
      if (insn->src2->id < orig_vals && is_const[insn->src2->id] && const_vals[insn->src2->id] == 1) {
        insn->kind = HLIR_ICONST;
        insn->imm = 0;
        insn->src1 = insn->src2 = NULL;
        if (insn->dst->id < orig_vals) {
          is_const[insn->dst->id] = true;
          const_vals[insn->dst->id] = 0;
        }
        changed = true;
      } else if (insn->src2->id < orig_vals && is_const[insn->src2->id] &&
                 insn->ty && insn->ty->is_unsigned) {
        int64_t val = const_vals[insn->src2->id];
        if (val > 0 && (val & (val - 1)) == 0) {
          HLIRVal *mask_val = hlir_new_val(fn, insn->src2->ty);
          HLIRInsn *imm_insn = hlir_new_insn(HLIR_ICONST);
          imm_insn->dst = mask_val;
          imm_insn->imm = val - 1;
          hlir_insert_before(fn, insn, imm_insn);

          insn->kind = HLIR_BITAND;
          insn->src2 = mask_val;
          changed = true;
        }
      }
    }
    // x ^ x = 0, x & 0 = 0, x & x = x, x | 0 = x, x | x = x, x ^ 0 = x, x ^ -1 = ~x
    else if (insn->kind == HLIR_BITXOR && insn->dst && insn->src1 && insn->src2) {
      if (insn->src1 == insn->src2) {
        insn->kind = HLIR_ICONST;
        insn->imm = 0;
        insn->src1 = insn->src2 = NULL;
        if (insn->dst->id < orig_vals) {
          is_const[insn->dst->id] = true;
          const_vals[insn->dst->id] = 0;
        }
        changed = true;
      } else if (insn->src2->id < orig_vals && is_const[insn->src2->id] && const_vals[insn->src2->id] == 0) {
        insn->kind = HLIR_CAST;
        insn->src2 = NULL;
        changed = true;
      } else if (insn->src1->id < orig_vals && is_const[insn->src1->id] && const_vals[insn->src1->id] == 0) {
        insn->kind = HLIR_CAST;
        insn->src1 = insn->src2;
        insn->src2 = NULL;
        changed = true;
      } else if (insn->src2->id < orig_vals && is_const[insn->src2->id] && const_vals[insn->src2->id] == -1) {
        insn->kind = HLIR_BITNOT;
        insn->src2 = NULL;
        changed = true;
      } else if (insn->src1->id < orig_vals && is_const[insn->src1->id] && const_vals[insn->src1->id] == -1) {
        insn->kind = HLIR_BITNOT;
        insn->src1 = insn->src2;
        insn->src2 = NULL;
        changed = true;
      }
    } else if (insn->kind == HLIR_BITAND && insn->dst && insn->src1 && insn->src2) {
      if (insn->src1 == insn->src2) {
        insn->kind = HLIR_CAST;
        insn->src2 = NULL;
        changed = true;
      } else if ((insn->src2->id < orig_vals && is_const[insn->src2->id] && const_vals[insn->src2->id] == 0) ||
                 (insn->src1->id < orig_vals && is_const[insn->src1->id] && const_vals[insn->src1->id] == 0)) {
        insn->kind = HLIR_ICONST;
        insn->imm = 0;
        insn->src1 = insn->src2 = NULL;
        if (insn->dst->id < orig_vals) {
          is_const[insn->dst->id] = true;
          const_vals[insn->dst->id] = 0;
        }
        changed = true;
      } else if (insn->src2->id < orig_vals && is_const[insn->src2->id] && const_vals[insn->src2->id] == -1) {
        insn->kind = HLIR_CAST;
        insn->src2 = NULL;
        changed = true;
      } else if (insn->src1->id < orig_vals && is_const[insn->src1->id] && const_vals[insn->src1->id] == -1) {
        insn->kind = HLIR_CAST;
        insn->src1 = insn->src2;
        insn->src2 = NULL;
        changed = true;
      }
    } else if (insn->kind == HLIR_BITOR && insn->dst && insn->src1 && insn->src2) {
      if (insn->src1 == insn->src2) {
        insn->kind = HLIR_CAST;
        insn->src2 = NULL;
        changed = true;
      } else if (insn->src2->id < orig_vals && is_const[insn->src2->id] && const_vals[insn->src2->id] == 0) {
        insn->kind = HLIR_CAST;
        insn->src2 = NULL;
        changed = true;
      } else if (insn->src1->id < orig_vals && is_const[insn->src1->id] && const_vals[insn->src1->id] == 0) {
        insn->kind = HLIR_CAST;
        insn->src1 = insn->src2;
        insn->src2 = NULL;
        changed = true;
      } else if ((insn->src2->id < orig_vals && is_const[insn->src2->id] && const_vals[insn->src2->id] == -1) ||
                 (insn->src1->id < orig_vals && is_const[insn->src1->id] && const_vals[insn->src1->id] == -1)) {
        insn->kind = HLIR_ICONST;
        insn->imm = -1;
        insn->src1 = insn->src2 = NULL;
        if (insn->dst->id < orig_vals) {
          is_const[insn->dst->id] = true;
          const_vals[insn->dst->id] = -1;
        }
        changed = true;
      }
    } else if ((insn->kind == HLIR_SHL || insn->kind == HLIR_SHR) && insn->dst && insn->src1 && insn->src2) {
      if (insn->src2->id < orig_vals && is_const[insn->src2->id] && const_vals[insn->src2->id] == 0) {
        insn->kind = HLIR_CAST;
        insn->src2 = NULL;
        changed = true;
      }
    } else if ((insn->kind == HLIR_CMP_EQ || insn->kind == HLIR_CMP_LE || insn->kind == HLIR_CMP_GE) &&
               insn->dst && insn->src1 && insn->src2 && insn->src1 == insn->src2 &&
               (!insn->src1->ty || !is_flonum(insn->src1->ty))) {
      insn->kind = HLIR_ICONST;
      insn->imm = 1;
      insn->src1 = insn->src2 = NULL;
      if (insn->dst->id < orig_vals) {
        is_const[insn->dst->id] = true;
        const_vals[insn->dst->id] = 1;
      }
      changed = true;
    } else if ((insn->kind == HLIR_CMP_NE || insn->kind == HLIR_CMP_LT || insn->kind == HLIR_CMP_GT) &&
               insn->dst && insn->src1 && insn->src2 && insn->src1 == insn->src2 &&
               (!insn->src1->ty || !is_flonum(insn->src1->ty))) {
      insn->kind = HLIR_ICONST;
      insn->imm = 0;
      insn->src1 = insn->src2 = NULL;
      if (insn->dst->id < orig_vals) {
        is_const[insn->dst->id] = true;
        const_vals[insn->dst->id] = 0;
      }
      changed = true;
    }
  }

  free(const_vals);
  free(is_const);
  free(defs);
  return changed;
}

// 3. HLIR Copy Propagation Pass
bool hlir_opt_copy_prop(HLIRFunction *fn) {
  if (!fn || fn->num_vals == 0)
    return false;

  bool changed = false;
  HLIRVal **aliases = calloc(fn->num_vals, sizeof(HLIRVal *));

  for (HLIRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind == HLIR_LABEL || insn->kind == HLIR_JMP ||
        insn->kind == HLIR_JMP_IF_ZERO || insn->kind == HLIR_JMP_IF_NZ ||
        insn->kind == HLIR_CALL) {
      memset(aliases, 0, fn->num_vals * sizeof(HLIRVal *));
      continue;
    }

    if (insn->src1 && insn->src1->id < fn->num_vals && aliases[insn->src1->id]) {
      insn->src1 = aliases[insn->src1->id];
      changed = true;
    }
    if (insn->src2 && insn->src2->id < fn->num_vals && aliases[insn->src2->id]) {
      insn->src2 = aliases[insn->src2->id];
      changed = true;
    }
    if (insn->src3 && insn->src3->id < fn->num_vals && aliases[insn->src3->id]) {
      insn->src3 = aliases[insn->src3->id];
      changed = true;
    }
    for (int i = 0; i < insn->num_args; i++) {
      if (insn->args[i] && insn->args[i]->id < fn->num_vals && aliases[insn->args[i]->id]) {
        insn->args[i] = aliases[insn->args[i]->id];
        changed = true;
      }
    }

    if (insn->kind == HLIR_CAST && insn->dst && insn->src1) {
      Type *t1 = insn->dst->ty;
      Type *t2 = insn->src1->ty;
      if (t1 && t2 && t1->size == t2->size && is_flonum(t1) == is_flonum(t2) && t1->is_unsigned == t2->is_unsigned) {
        HLIRVal *root = insn->src1;
        while (root->id < fn->num_vals && aliases[root->id])
          root = aliases[root->id];
        if (root != insn->dst && insn->dst->id < fn->num_vals)
          aliases[insn->dst->id] = root;
      }
    }
  }

  free(aliases);
  return changed;
}

// 4. HLIR Local Common Subexpression Elimination (CSE)
bool hlir_opt_local_cse(HLIRFunction *fn) {
  if (!fn || !fn->head)
    return false;

  bool changed = false;

  for (HLIRInsn *insn = fn->head; insn; insn = insn->next) {
    if (!insn->dst || insn->kind == HLIR_NOP || insn->kind == HLIR_CAST)
      continue;

    bool is_arith = (insn->kind == HLIR_ADD || insn->kind == HLIR_SUB ||
                     insn->kind == HLIR_MUL || insn->kind == HLIR_BITAND ||
                     insn->kind == HLIR_BITOR || insn->kind == HLIR_BITXOR ||
                     insn->kind == HLIR_SHL || insn->kind == HLIR_SHR ||
                     insn->kind == HLIR_CMP_EQ || insn->kind == HLIR_CMP_NE ||
                     insn->kind == HLIR_CMP_LT || insn->kind == HLIR_CMP_LE ||
                     insn->kind == HLIR_CMP_GT || insn->kind == HLIR_CMP_GE);
    bool is_unary = (insn->kind == HLIR_NEG || insn->kind == HLIR_BITNOT || insn->kind == HLIR_LOGNOT);
    bool is_addr = (insn->kind == HLIR_ADDR_VAR);
    bool is_iconst = (insn->kind == HLIR_ICONST);

    if (!is_arith && !is_unary && !is_addr && !is_iconst)
      continue;

    for (HLIRInsn *sub = insn->next; sub; sub = sub->next) {
      if (sub->kind == HLIR_LABEL || sub->kind == HLIR_JMP ||
          sub->kind == HLIR_JMP_IF_ZERO || sub->kind == HLIR_JMP_IF_NZ ||
          sub->kind == HLIR_CALL || sub->kind == HLIR_RET ||
          sub->kind == HLIR_ASM || sub->kind == HLIR_CAS || sub->kind == HLIR_EXCH)
        break;

      if (sub->kind == insn->kind && sub->dst) {
        bool match = false;
        if (is_arith) {
          if (sub->src1 == insn->src1 && sub->src2 == insn->src2)
            match = true;
          else if ((insn->kind == HLIR_ADD || insn->kind == HLIR_MUL ||
                    insn->kind == HLIR_BITAND || insn->kind == HLIR_BITOR ||
                    insn->kind == HLIR_BITXOR || insn->kind == HLIR_CMP_EQ ||
                    insn->kind == HLIR_CMP_NE) &&
                   sub->src1 == insn->src2 && sub->src2 == insn->src1)
            match = true;
        } else if (is_unary) {
          if (sub->src1 == insn->src1)
            match = true;
        } else if (is_addr) {
          if (sub->var == insn->var)
            match = true;
        } else if (is_iconst) {
          if (sub->imm == insn->imm)
            match = true;
        }

        if (match) {
          sub->kind = HLIR_CAST;
          sub->src1 = insn->dst;
          sub->src2 = NULL;
          sub->src3 = NULL;
          changed = true;
        }
      }
    }
  }

  return changed;
}

// 5. HLIR Redundant Load-After-Store & Dead Store Elimination
bool hlir_opt_load_store(HLIRFunction *fn) {
  if (!fn || !fn->head)
    return false;

  bool changed = false;

  for (HLIRInsn *insn = fn->head; insn; insn = insn->next) {
    // 1. Redundant load-after-store: STORE_VAR v, val  followed by  dst = LOAD_VAR v
    if (insn->kind == HLIR_STORE_VAR && insn->var && insn->src1) {
      Obj *var = insn->var;
      HLIRVal *val = insn->src1;

      for (HLIRInsn *cur = insn->next; cur; cur = cur->next) {
        if (cur->kind == HLIR_LABEL || cur->kind == HLIR_JMP ||
            cur->kind == HLIR_JMP_IF_ZERO || cur->kind == HLIR_JMP_IF_NZ ||
            cur->kind == HLIR_CALL || cur->kind == HLIR_RET ||
            cur->kind == HLIR_ASM || cur->kind == HLIR_CAS || cur->kind == HLIR_EXCH ||
            cur->kind == HLIR_STORE_PTR || cur->kind == HLIR_MEMCPY || cur->kind == HLIR_MEMZERO)
          break;

        if (cur->kind == HLIR_ADDR_VAR && cur->var == var)
          break;

        if (cur->kind == HLIR_STORE_VAR && cur->var == var)
          break;

        if (cur->kind == HLIR_LOAD_VAR && cur->var == var && cur->dst) {
          cur->kind = HLIR_CAST;
          cur->src1 = val;
          cur->src2 = NULL;
          changed = true;
          break;
        }
      }
    }

    // 2. Dead store elimination: STORE_VAR v, val1 followed by STORE_VAR v, val2
    if (insn->kind == HLIR_STORE_VAR && insn->var && insn->var->is_local) {
      Obj *var = insn->var;

      for (HLIRInsn *cur = insn->next; cur; cur = cur->next) {
        if (cur->kind == HLIR_LABEL || cur->kind == HLIR_JMP ||
            cur->kind == HLIR_JMP_IF_ZERO || cur->kind == HLIR_JMP_IF_NZ ||
            cur->kind == HLIR_CALL || cur->kind == HLIR_RET ||
            cur->kind == HLIR_ASM || cur->kind == HLIR_CAS || cur->kind == HLIR_EXCH ||
            cur->kind == HLIR_LOAD_PTR || cur->kind == HLIR_STORE_PTR ||
            cur->kind == HLIR_MEMCPY || cur->kind == HLIR_MEMZERO)
          break;

        if (cur->kind == HLIR_ADDR_VAR && cur->var == var)
          break;

        if (cur->kind == HLIR_LOAD_VAR && cur->var == var)
          break;

        if (cur->kind == HLIR_STORE_VAR && cur->var == var) {
          HLIRInsn *to_remove = insn;
          insn = insn->prev ? insn->prev : fn->head;
          hlir_remove_insn(fn, to_remove);
          changed = true;
          break;
        }
      }
      if (changed && !insn) break;
    }
  }

  return changed;
}

// 6. HLIR Dead Branch Elimination & Control Flow Simplification
bool hlir_opt_control_flow(HLIRFunction *fn) {
  if (!fn || !fn->head)
    return false;

  bool changed = false;
  int orig_vals = fn->num_vals ? fn->num_vals : 1;
  int64_t *const_vals = calloc(orig_vals, sizeof(int64_t));
  bool *is_const = calloc(orig_vals, sizeof(bool));
  HLIRInsn **defs = calloc(orig_vals, sizeof(HLIRInsn *));

  for (HLIRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind == HLIR_LABEL || insn->kind == HLIR_JMP || insn->kind == HLIR_CALL) {
      memset(is_const, 0, orig_vals * sizeof(bool));
      memset(defs, 0, orig_vals * sizeof(HLIRInsn *));
      continue;
    }

    if (insn->dst && insn->dst->id < orig_vals) {
      defs[insn->dst->id] = insn;
    }

    if (insn->kind == HLIR_ICONST && insn->dst && insn->dst->id < orig_vals) {
      is_const[insn->dst->id] = true;
      const_vals[insn->dst->id] = insn->imm;
      continue;
    }

    if (insn->kind == HLIR_JMP_IF_ZERO && insn->src1 && insn->src1->id < orig_vals && is_const[insn->src1->id]) {
      if (const_vals[insn->src1->id] == 0) {
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

    if (insn->kind == HLIR_JMP_IF_NZ && insn->src1 && insn->src1->id < orig_vals && is_const[insn->src1->id]) {
      if (const_vals[insn->src1->id] != 0) {
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

    // Conditional branch simplification for comparison with 0
    if (insn->kind == HLIR_JMP_IF_ZERO && insn->src1 && insn->src1->id < orig_vals && defs[insn->src1->id]) {
      HLIRInsn *cmp = defs[insn->src1->id];
      if (cmp->kind == HLIR_CMP_EQ && cmp->src1 && cmp->src2 && cmp->src2->id < orig_vals && is_const[cmp->src2->id] && const_vals[cmp->src2->id] == 0) {
        insn->kind = HLIR_JMP_IF_NZ;
        insn->src1 = cmp->src1;
        changed = true;
      } else if (cmp->kind == HLIR_CMP_NE && cmp->src1 && cmp->src2 && cmp->src2->id < orig_vals && is_const[cmp->src2->id] && const_vals[cmp->src2->id] == 0) {
        insn->kind = HLIR_JMP_IF_ZERO;
        insn->src1 = cmp->src1;
        changed = true;
      }
    } else if (insn->kind == HLIR_JMP_IF_NZ && insn->src1 && insn->src1->id < orig_vals && defs[insn->src1->id]) {
      HLIRInsn *cmp = defs[insn->src1->id];
      if (cmp->kind == HLIR_CMP_EQ && cmp->src1 && cmp->src2 && cmp->src2->id < orig_vals && is_const[cmp->src2->id] && const_vals[cmp->src2->id] == 0) {
        insn->kind = HLIR_JMP_IF_ZERO;
        insn->src1 = cmp->src1;
        changed = true;
      } else if (cmp->kind == HLIR_CMP_NE && cmp->src1 && cmp->src2 && cmp->src2->id < orig_vals && is_const[cmp->src2->id] && const_vals[cmp->src2->id] == 0) {
        insn->kind = HLIR_JMP_IF_NZ;
        insn->src1 = cmp->src1;
        changed = true;
      }
    }

    // Eliminate jump to immediate next label
    if (insn->kind == HLIR_JMP && insn->label) {
      HLIRInsn *nxt = insn->next;
      while (nxt && nxt->kind == HLIR_NOP)
        nxt = nxt->next;
      if (nxt && nxt->kind == HLIR_LABEL && nxt->label && !strcmp(insn->label, nxt->label)) {
        HLIRInsn *del = insn;
        insn = insn->prev ? insn->prev : fn->head;
        hlir_remove_insn(fn, del);
        changed = true;
        if (!insn) break;
        continue;
      }
    }
  }

  free(const_vals);
  free(is_const);
  free(defs);
  return changed;
}

// 7. HLIR Dead Value Elimination (Remove unused values)
bool hlir_opt_dce(HLIRFunction *fn) {
  if (!fn || fn->num_vals == 0)
    return false;

  bool changed = false;
  bool *used = calloc(fn->num_vals, sizeof(bool));

  for (HLIRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->src1 && insn->src1->id < fn->num_vals) used[insn->src1->id] = true;
    if (insn->src2 && insn->src2->id < fn->num_vals) used[insn->src2->id] = true;
    if (insn->src3 && insn->src3->id < fn->num_vals) used[insn->src3->id] = true;
    for (int i = 0; i < insn->num_args; i++) {
      if (insn->args[i] && insn->args[i]->id < fn->num_vals)
        used[insn->args[i]->id] = true;
    }
  }

  for (HLIRInsn *insn = fn->head; insn;) {
    HLIRInsn *next = insn->next;

    if (insn->dst && insn->dst->id < fn->num_vals && !used[insn->dst->id]) {
      bool can_remove = false;
      switch (insn->kind) {
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
        can_remove = true;
        break;
      default:
        break;
      }

      if (can_remove) {
        hlir_remove_insn(fn, insn);
        changed = true;
      }
    }

    insn = next;
  }

  free(used);
  return changed;
}

// 8. HLIR Dead Code / Unreachable Code Elimination
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

// 9. HLIR Function Inlining Pass
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

      if (!callee || callee == caller)
        continue;

      if (callee->num_insns > 20)
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

// HLIR Optimization Driver
void hlir_optimize(HLIRProg *prog, int opt_level) {
  if (!prog)
    return;

  ir_init_pass_registry();

  int max_iter = (opt_level > 0) ? opt_max_passes : 1;

  if (opt_level >= 2) {
    hlir_opt_inlining(prog);
  }

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
