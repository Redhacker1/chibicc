#include "ir/opt.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// LLIR Low-Level Peephole Optimizer
// ============================================================================
// Performs windowed peephole optimizations, algebraic simplifications,
// identity reductions, strength reductions, constant reassociations, and
// dead store / load-after-store forwarding on low-level IR.

static IRVReg *make_imm_vreg(IRFunction *fn, IRInsn *before, int64_t val, Type *ty) {
  IRVReg *v = ir_new_vreg(fn, ty ? ty : ty_long);
  IRInsn *imm_insn = ir_new_insn(IR_IMM);
  imm_insn->dst = v;
  imm_insn->imm = val;
  imm_insn->ty = ty ? ty : ty_long;
  v->def_insn = imm_insn;
  ir_insert_before(fn, before, imm_insn);
  return v;
}

static int get_power_of_two(int64_t val) {
  if (val <= 0 || (val & (val - 1)) != 0)
    return -1;
  int shift = 0;
  while (val > 1) {
    val >>= 1;
    shift++;
  }
  return shift;
}

IRKind ir_op_commute(IRKind kind) {
  switch (kind) {
  case IR_ADD:
  case IR_MUL:
  case IR_BITAND:
  case IR_BITOR:
  case IR_BITXOR:
  case IR_EQ:
  case IR_NE:
    return kind;
  case IR_LT: return IR_GT;
  case IR_LE: return IR_GE;
  case IR_GT: return IR_LT;
  case IR_GE: return IR_LE;
  default:
    return 0;
  }
}

bool ir_is_associative_binop(IRKind kind) {
  return kind == IR_ADD || kind == IR_MUL ||
         kind == IR_BITAND || kind == IR_BITOR || kind == IR_BITXOR;
}

bool ir_eval_unary_imm(IRKind kind, int64_t c, int64_t *out) {
  switch (kind) {
  case IR_NEG:    *out = -c; return true;
  case IR_BITNOT: *out = ~c; return true;
  case IR_LOGNOT: *out = !c; return true;
  default:        return false;
  }
}

bool ir_eval_binary_imm(IRKind kind, int64_t c1, int64_t c2, bool is_uns, int64_t *out) {
  switch (kind) {
  case IR_ADD:    *out = c1 + c2; return true;
  case IR_SUB:    *out = c1 - c2; return true;
  case IR_MUL:    *out = c1 * c2; return true;
  case IR_DIV:
    if (c2 == 0) return false;
    *out = is_uns ? (int64_t)((uint64_t)c1 / (uint64_t)c2) : (c1 / c2);
    return true;
  case IR_MOD:
    if (c2 == 0) return false;
    *out = is_uns ? (int64_t)((uint64_t)c1 % (uint64_t)c2) : (c1 % c2);
    return true;
  case IR_BITAND: *out = c1 & c2; return true;
  case IR_BITOR:  *out = c1 | c2; return true;
  case IR_BITXOR: *out = c1 ^ c2; return true;
  case IR_SHL:    *out = c1 << (c2 & 63); return true;
  case IR_SHR:    *out = is_uns ? (int64_t)((uint64_t)c1 >> (c2 & 63)) : (c1 >> (c2 & 63)); return true;
  case IR_EQ:     *out = (c1 == c2); return true;
  case IR_NE:     *out = (c1 != c2); return true;
  case IR_LT:     *out = is_uns ? ((uint64_t)c1 < (uint64_t)c2) : (c1 < c2); return true;
  case IR_LE:     *out = is_uns ? ((uint64_t)c1 <= (uint64_t)c2) : (c1 <= c2); return true;
  case IR_GT:     *out = is_uns ? ((uint64_t)c1 > (uint64_t)c2) : (c1 > c2); return true;
  case IR_GE:     *out = is_uns ? ((uint64_t)c1 >= (uint64_t)c2) : (c1 >= c2); return true;
  default:        return false;
  }
}

bool ir_is_right_identity(IRKind kind, int64_t c) {
  switch (kind) {
  case IR_ADD:
  case IR_SUB:
  case IR_BITOR:
  case IR_BITXOR:
    return c == 0;
  case IR_SHL:
  case IR_SHR:
    return (c & 63) == 0;
  case IR_MUL:
  case IR_DIV:
    return c == 1;
  case IR_BITAND:
    return c == -1;
  default:
    return false;
  }
}

bool ir_is_absorbing_element(IRKind kind, int64_t c, bool is_uns, int64_t *out_val) {
  switch (kind) {
  case IR_MUL:
  case IR_BITAND:
    if (c == 0) {
      *out_val = 0;
      return true;
    }
    return false;
  case IR_MOD:
    if (c == 1 || (!is_uns && c == -1)) {
      *out_val = 0;
      return true;
    }
    return false;
  default:
    return false;
  }
}

bool ir_eval_same_operands(IRKind kind, int64_t *out_val) {
  switch (kind) {
  case IR_SUB:
  case IR_BITXOR:
  case IR_NE:
  case IR_LT:
  case IR_GT:
    *out_val = 0;
    return true;
  case IR_EQ:
  case IR_LE:
  case IR_GE:
    *out_val = 1;
    return true;
  default:
    return false;
  }
}

bool ir_is_involution_pair(IRKind outer, IRKind inner) {
  return (outer == IR_NEG && inner == IR_NEG) ||
         (outer == IR_BITNOT && inner == IR_BITNOT);
}

static bool ir_is_comparison(IRKind kind) {
  return kind == IR_EQ || kind == IR_NE || kind == IR_LT ||
         kind == IR_LE || kind == IR_GT || kind == IR_GE;
}

static IRKind ir_invert_cmp(IRKind kind) {
  switch (kind) {
  case IR_EQ: return IR_NE;
  case IR_NE: return IR_EQ;
  case IR_LT: return IR_GE;
  case IR_LE: return IR_GT;
  case IR_GT: return IR_LE;
  case IR_GE: return IR_LT;
  default: return 0;
  }
}

bool ir_opt_peephole(IRFunction *fn) {
  if (!fn || fn->num_vregs == 0)
    return false;

  bool changed = false;

  int *def_count = calloc(fn->num_vregs, sizeof(int));
  for (int i = 0; i < fn->num_vregs; i++) {
    if (fn->vregs[i])
      fn->vregs[i]->def_insn = NULL;
  }
  for (IRInsn *i = fn->head; i; i = i->next) {
    if (i->dst) {
      def_count[i->dst->id]++;
      i->dst->def_insn = i;
    }
  }
  for (int i = 0; i < fn->num_vregs; i++) {
    if (def_count[i] != 1 && fn->vregs[i])
      fn->vregs[i]->def_insn = NULL;
  }
  free(def_count);

  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    // 0. Branch condition simplification (e.g. branch on !cond)
    if (insn->kind == IR_BR && insn->src1 && insn->src1->def_insn) {
      IRInsn *def = insn->src1->def_insn;
      if (def->kind == IR_LOGNOT && def->src1 && insn->label_true && insn->label_false) {
        insn->src1 = def->src1;
        char *tmp = insn->label_true;
        insn->label_true = insn->label_false;
        insn->label_false = tmp;
        changed = true;
      } else if (def->kind == IR_NE && def->src2 && def->src2->def_insn &&
                 def->src2->def_insn->kind == IR_IMM && def->src2->def_insn->imm == 0 &&
                 def->src1 && def->src1->def_insn && ir_is_comparison(def->src1->def_insn->kind)) {
        insn->src1 = def->src1;
        changed = true;
      }
    }

    // 1. Redundant self-move: dst = mov dst
    if (insn->kind == IR_MOV && insn->dst && insn->src1 && insn->dst == insn->src1) {
      const IRInsn *next = insn->next;
      ir_remove_insn(fn, insn);
      insn = next ? next->prev : fn->tail;
      changed = true;
      if (!insn) break;
      continue;
    }

    // 2. Redundant load-after-store elimination:
    // STORE addr, val  followed closely by  dst = LOAD addr
    if (insn->kind == IR_STORE && insn->src1 && insn->src2) {
      const IRVReg *addr = insn->src1;
      IRVReg *val = insn->src2;

      for (IRInsn *cur = insn->next; cur; cur = cur->next) {
        if (ir_is_cf_barrier(cur) || ir_is_mem_clobber(cur))
          break; // Barrier or memory clobber

        if (cur->kind == IR_LOAD && cur->src1 == addr && cur->dst && val &&
            ir_vregs_same_size_and_float(cur->dst, val)) {
          cur->kind = IR_MOV;
          cur->src1 = val;
          changed = true;
          break;
        }
      }
    }

    // 3. Dead store elimination:
    // STORE addr, val1 followed by STORE addr, val2 with no reads or barriers in between
    if (insn->kind == IR_STORE && insn->src1 && insn->src2) {
      const IRVReg *addr = insn->src1;
      for (const IRInsn *cur = insn->next; cur; cur = cur->next) {
        if (ir_is_cf_barrier(cur) || ir_is_mem_read(cur))
          break; // Barrier or reads memory
        if (cur->kind == IR_STORE && cur->src1 == addr) {
          // Address overwritten before being read
          IRInsn *to_remove = insn;
          insn = insn->prev ? insn->prev : fn->head;
          ir_remove_insn(fn, to_remove);
          changed = true;
          break;
        }
      }
      if (changed && !insn) break;
    }

    // 4. Redundant sign/zero extension and cast simplifications:
    if (insn->kind == IR_CAST && insn->dst && insn->src1) {
      if (ir_vregs_same_size_and_float(insn->dst, insn->src1)) {
        insn->kind = IR_MOV;
        changed = true;
      } else if (!insn->dst->is_float && !insn->src1->is_float) {
        int64_t imm_val = 0;
        if (ir_vreg_is_imm(insn->src1, &imm_val)) {
          int dsz = insn->dst->ty ? insn->dst->ty->size : 8;
          bool duns = insn->dst->ty ? insn->dst->ty->is_unsigned : false;
          int64_t folded = imm_val;
          if (dsz == 1) folded = duns ? (uint8_t)imm_val : (int8_t)imm_val;
          else if (dsz == 2) folded = duns ? (uint16_t)imm_val : (int16_t)imm_val;
          else if (dsz == 4) folded = duns ? (uint32_t)imm_val : (int32_t)imm_val;
          insn->kind = IR_IMM;
          insn->imm = folded;
          insn->src1 = NULL;
          changed = true;
          continue;
        }

        IRInsn *def1 = insn->src1->def_insn;
        if (def1 && def1->kind == IR_CAST && def1->src1 && def1->dst &&
            !def1->src1->is_float && !def1->dst->is_float) {
          int sz_orig = def1->src1->ty ? def1->src1->ty->size : 8;
          int sz_mid = def1->dst->ty ? def1->dst->ty->size : 8;
          int sz_final = insn->dst->ty ? insn->dst->ty->size : 8;
          if (sz_mid >= sz_orig && sz_mid >= sz_final) {
            insn->src1 = def1->src1;
            changed = true;
            continue;
          }
        }
      }
    }

    // 5. Unary simplifications: constant folding and double-involution
    if (insn->src1 && !insn->src2 && insn->dst && !insn->dst->is_float) {
      IRInsn *def1 = insn->src1->def_insn;
      if (def1 && ir_is_involution_pair(insn->kind, def1->kind) && def1->src1) {
        insn->kind = IR_MOV;
        insn->src1 = def1->src1;
        changed = true;
        continue;
      }
      int64_t imm1;
      if (ir_vreg_is_imm(insn->src1, &imm1)) {
        int64_t res = 0;
        if (ir_eval_unary_imm(insn->kind, imm1, &res)) {
          insn->kind = IR_IMM;
          insn->imm = res;
          insn->src1 = NULL;
          changed = true;
          continue;
        }
      }
    }

    // 6. Binary operations: canonicalization, constant folding, and generalized algebraic rules
    if (insn->src1 && insn->src2 && !insn->src1->is_float && !insn->src2->is_float) {
      int64_t c1 = 0, c2 = 0;
      bool s1_imm = ir_vreg_is_imm(insn->src1, &c1);
      bool s2_imm = ir_vreg_is_imm(insn->src2, &c2);
      bool is_uns = ir_insn_is_unsigned(insn);

      // 6a. Canonicalize commutative & relational operations to place immediate in src2
      if (s1_imm && !s2_imm) {
        IRKind comm_kind = ir_op_commute(insn->kind);
        if (comm_kind) {
          insn->kind = comm_kind;
          IRVReg *tmp = insn->src1;
          insn->src1 = insn->src2;
          insn->src2 = tmp;
          s1_imm = false;
          s2_imm = true;
          c2 = c1;
          changed = true;
        }
      }

      // 6b. Binary constant folding on LLIR
      if (s1_imm && s2_imm && insn->dst && !insn->dst->is_float) {
        int64_t res = 0;
        if (ir_eval_binary_imm(insn->kind, c1, c2, is_uns, &res)) {
          insn->kind = IR_IMM;
          insn->imm = res;
          insn->src1 = NULL;
          insn->src2 = NULL;
          changed = true;
          continue;
        }
      }

      // 6c. Generalized rules with constant operand in src2
      if (s2_imm && insn->dst && !insn->dst->is_float) {
        IRInsn *def1 = insn->src1 ? insn->src1->def_insn : NULL;
        int64_t absorb_val = 0;

        // Comparison with 0 / 1 boolean simplifications
        if (def1 && ir_is_comparison(def1->kind)) {
          if (insn->kind == IR_NE && c2 == 0) {
            insn->kind = def1->kind;
            insn->src1 = def1->src1;
            insn->src2 = def1->src2;
            changed = true;
            continue;
          } else if (insn->kind == IR_EQ && c2 == 0) {
            insn->kind = ir_invert_cmp(def1->kind);
            insn->src1 = def1->src1;
            insn->src2 = def1->src2;
            changed = true;
            continue;
          } else if (insn->kind == IR_EQ && c2 == 1) {
            insn->kind = def1->kind;
            insn->src1 = def1->src1;
            insn->src2 = def1->src2;
            changed = true;
            continue;
          } else if (insn->kind == IR_NE && c2 == 1) {
            insn->kind = ir_invert_cmp(def1->kind);
            insn->src1 = def1->src1;
            insn->src2 = def1->src2;
            changed = true;
            continue;
          }
        }

        // Identity rule: x op c -> x
        if (ir_is_right_identity(insn->kind, c2)) {
          insn->kind = IR_MOV;
          insn->src2 = NULL;
          changed = true;
          continue;
        }

        // Absorbing rule: x op c -> constant
        if (ir_is_absorbing_element(insn->kind, c2, is_uns, &absorb_val)) {
          insn->kind = IR_IMM;
          insn->imm = absorb_val;
          insn->src1 = NULL;
          insn->src2 = NULL;
          changed = true;
          continue;
        }

        // Power-of-two strength reduction
        int shift = get_power_of_two(c2);
        if (shift >= 0) {
          if (insn->kind == IR_MUL) {
            insn->kind = IR_SHL;
            insn->src2 = make_imm_vreg(fn, insn, shift, insn->src2->ty);
            changed = true;
            continue;
          } else if (insn->kind == IR_DIV && is_uns) {
            if (shift == 0) {
              insn->kind = IR_MOV;
              insn->src2 = NULL;
            } else {
              insn->kind = IR_SHR;
              insn->src2 = make_imm_vreg(fn, insn, shift, insn->src2->ty);
            }
            changed = true;
            continue;
          } else if (insn->kind == IR_MOD && is_uns) {
            insn->kind = IR_BITAND;
            insn->src2 = make_imm_vreg(fn, insn, c2 - 1, insn->src2->ty);
            changed = true;
            continue;
          }
        }

        // Associative constant reassociation: (x op c1) op c2 -> x op (c1 op c2)
        int64_t def_c1 = 0;
        if (def1 && def1->kind == insn->kind && def1->src2 && ir_vreg_is_imm(def1->src2, &def_c1)) {
          if (ir_is_associative_binop(insn->kind)) {
            // Fold LEA(var, c1) + c2 if ADD
            if (insn->kind == IR_ADD && def1->src1 && def1->src1->def_insn &&
                def1->src1->def_insn->kind == IR_ADDR) {
              IRInsn *addr_def = def1->src1->def_insn;
              insn->kind = IR_ADDR;
              insn->var = addr_def->var;
              insn->label = addr_def->label;
              insn->imm = addr_def->imm + def_c1 + c2;
              insn->src1 = NULL;
              insn->src2 = NULL;
              changed = true;
              continue;
            }
            int64_t combined = 0;
            if (ir_eval_binary_imm(insn->kind, def_c1, c2, is_uns, &combined)) {
              insn->src1 = def1->src1;
              insn->src2 = make_imm_vreg(fn, insn, combined, insn->src2->ty);
              changed = true;
              continue;
            }
          } else if (insn->kind == IR_SHL || insn->kind == IR_SHR) {
            if (def_c1 + c2 < 64) {
              insn->src1 = def1->src1;
              insn->src2 = make_imm_vreg(fn, insn, def_c1 + c2, insn->src2->ty);
              changed = true;
              continue;
            }
          }
        }

        // Address constant folding: LEA(var, c1) + c2 -> LEA(var, c1 + c2)
        if (insn->kind == IR_ADD && def1 && def1->kind == IR_ADDR) {
          insn->kind = IR_ADDR;
          insn->var = def1->var;
          insn->label = def1->label;
          insn->imm = def1->imm + c2;
          insn->src1 = NULL;
          insn->src2 = NULL;
          changed = true;
          continue;
        }

        // Add/Sub cross constant reassociation
        if (def1 && def1->src2 && ir_vreg_is_imm(def1->src2, &def_c1)) {
          if (insn->kind == IR_ADD && def1->kind == IR_SUB) {
            // (x - c1) + c2 = x + (c2 - c1)
            insn->src1 = def1->src1;
            insn->src2 = make_imm_vreg(fn, insn, c2 - def_c1, insn->src2->ty);
            changed = true;
            continue;
          } else if (insn->kind == IR_SUB && def1->kind == IR_ADD) {
            // (x + c1) - c2 = x + (def_c1 - c2)
            insn->kind = IR_ADD;
            insn->src1 = def1->src1;
            insn->src2 = make_imm_vreg(fn, insn, def_c1 - c2, insn->src2->ty);
            changed = true;
            continue;
          } else if (insn->kind == IR_SUB && def1->kind == IR_SUB) {
            // (x - c1) - c2 = x - (def_c1 + c2)
            insn->src1 = def1->src1;
            insn->src2 = make_imm_vreg(fn, insn, def_c1 + c2, insn->src2->ty);
            changed = true;
            continue;
          }
        }
      }

      // 6d. Operations on identical operands: x op x
      if (insn->src1 && insn->src2 && insn->src1 == insn->src2 && insn->dst && !insn->dst->is_float) {
        int64_t same_val = 0;
        if (ir_eval_same_operands(insn->kind, &same_val)) {
          insn->kind = IR_IMM;
          insn->imm = same_val;
          insn->src1 = NULL;
          insn->src2 = NULL;
          changed = true;
          continue;
        } else if (insn->kind == IR_ADD) {
          // x + x = x << 1
          insn->kind = IR_SHL;
          insn->src2 = make_imm_vreg(fn, insn, 1, insn->src2->ty);
          changed = true;
          continue;
        }
      }

      // 6e. Cancellation & inverse algebraic identities
      if (insn->src1 && insn->src2 && insn->dst && !insn->dst->is_float) {
        IRInsn *def1 = insn->src1->def_insn;
        IRInsn *def2 = insn->src2->def_insn;

        if (insn->kind == IR_ADD && def2 && def2->kind == IR_NEG && def2->src1) {
          // x + (-y) = x - y
          insn->kind = IR_SUB;
          insn->src2 = def2->src1;
          changed = true;
          continue;
        } else if (insn->kind == IR_SUB && def2 && def2->kind == IR_NEG && def2->src1) {
          // x - (-y) = x + y
          insn->kind = IR_ADD;
          insn->src2 = def2->src1;
          changed = true;
          continue;
        } else if ((insn->kind == IR_SUB && def1 && def1->kind == IR_ADD) ||
                   (insn->kind == IR_BITXOR && def1 && def1->kind == IR_BITXOR)) {
          // (x + y) - y = x, (x + y) - x = y
          // (x ^ y) ^ y = x, (x ^ y) ^ x = y
          if (def1->src2 == insn->src2) {
            insn->kind = IR_MOV;
            insn->src1 = def1->src1;
            insn->src2 = NULL;
            changed = true;
            continue;
          } else if (def1->src1 == insn->src2) {
            insn->kind = IR_MOV;
            insn->src1 = def1->src2;
            insn->src2 = NULL;
            changed = true;
            continue;
          }
        }
      }
    }
  }

  return changed;
}
