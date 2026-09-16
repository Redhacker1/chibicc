#include "ir/opt.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Instruction Selection & Legalization Pass
// ============================================================================
// Prepares low-level IR for architecture backends (e.g. x86_64, m68k, z80) by:
// 1. Folding 32-bit immediate operands directly into binary and comparison ops
// 2. Standardizing base + index + displacement complex memory addressing modes
//    for LOAD and STORE operations
// 3. Commuting immediate operands to the right-hand side for commutative operations

bool ir_opt_legalize_isel(IRFunction *fn) {
  if (!fn) return false;
  bool changed = false;

  // Re-establish def_insn pointers for single definitions
  for (int i = 0; i < fn->num_vregs; i++) {
    if (fn->vregs[i])
      fn->vregs[i]->def_insn = NULL;
  }
  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->dst)
      insn->dst->def_insn = insn;
  }

  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    // 1. Fold immediate operands into binary instructions (ADD, SUB, AND, OR, XOR, SHL, SHR, CMP_*)
    if (insn->src2 && insn->src2->def_insn && insn->src2->def_insn->kind == IR_IMM) {
      int64_t imm = insn->src2->def_insn->imm;
      if (imm >= INT32_MIN && imm <= INT32_MAX) {
        switch (insn->kind) {
        case IR_ADD:
        case IR_SUB:
        case IR_MUL:
        case IR_BITAND:
        case IR_BITOR:
        case IR_BITXOR:
        case IR_SHL:
        case IR_SHR:
        case IR_EQ:
        case IR_NE:
        case IR_LT:
        case IR_LE:
        case IR_GT:
        case IR_GE:
          if (!insn->ty || !is_flonum(insn->ty)) {
            insn->is_imm_op = true;
            insn->imm = imm;
            changed = true;
          }
          break;
        default:
          break;
        }
      }
    } else if (insn->src1 && insn->src1->def_insn && insn->src1->def_insn->kind == IR_IMM &&
               ir_insn_is_commutative(insn->kind) && (!insn->ty || !is_flonum(insn->ty))) {
      int64_t imm = insn->src1->def_insn->imm;
      if (imm >= INT32_MIN && imm <= INT32_MAX) {
        insn->src1 = insn->src2;
        insn->src2 = NULL;
        insn->is_imm_op = true;
        insn->imm = imm;
        changed = true;
      }
    }

    // 2. Standardize complex memory addressing modes for LOAD and STORE
    if (insn->kind == IR_LOAD || insn->kind == IR_STORE) {
      IRVReg *addr_vreg = insn->src1;
      if (addr_vreg && addr_vreg->def_insn) {
        IRInsn *def = addr_vreg->def_insn;
        if (def->kind == IR_ADDR && (def->var || def->label)) {
          insn->has_mem_op = true;
          insn->var = def->var;
          insn->label = def->label;
          insn->disp = def->imm;
          insn->scale = 1;
          changed = true;
        } else if (def->kind == IR_ADD && def->src1 && def->src2) {
          IRInsn *d1 = def->src1->def_insn;
          IRInsn *d2 = def->src2->def_insn;
          if (d2 && d2->kind == IR_IMM && d2->imm >= INT32_MIN && d2->imm <= INT32_MAX) {
            if (d1 && d1->kind == IR_ADDR && (d1->var || d1->label)) {
              insn->has_mem_op = true;
              insn->var = d1->var;
              insn->label = d1->label;
              insn->disp = d1->imm + d2->imm;
              insn->scale = 1;
            } else {
              insn->has_mem_op = true;
              insn->base_reg = def->src1;
              insn->disp = d2->imm;
              insn->scale = 1;
            }
            changed = true;
          } else if (d1 && d1->kind == IR_IMM && d1->imm >= INT32_MIN && d1->imm <= INT32_MAX) {
            if (d2 && d2->kind == IR_ADDR && (d2->var || d2->label)) {
              insn->has_mem_op = true;
              insn->var = d2->var;
              insn->label = d2->label;
              insn->disp = d2->imm + d1->imm;
              insn->scale = 1;
            } else {
              insn->has_mem_op = true;
              insn->base_reg = def->src2;
              insn->disp = d1->imm;
              insn->scale = 1;
            }
            changed = true;
          }
        }
      }
    }
  }

  return changed;
}
