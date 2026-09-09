#include "ir/opt.h"
#include <stdlib.h>

// Constant folding pass
void ir_opt_const_fold(IRFunction *fn) {
  // Track constants assigned to VRegs
  int64_t *const_vals = calloc(fn->num_vregs, sizeof(int64_t));
  bool *is_const = calloc(fn->num_vregs, sizeof(bool));

  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind == IR_IMM && insn->dst) {
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
      case IR_ADD: res = c1 + c2; break;
      case IR_SUB: res = c1 - c2; break;
      case IR_MUL: res = c1 * c2; break;
      case IR_DIV: if (c2 != 0) res = c1 / c2; else folded = false; break;
      case IR_MOD: if (c2 != 0) res = c1 % c2; else folded = false; break;
      case IR_BITAND: res = c1 & c2; break;
      case IR_BITOR:  res = c1 | c2; break;
      case IR_BITXOR: res = c1 ^ c2; break;
      case IR_SHL: res = c1 << c2; break;
      case IR_SHR: res = c1 >> c2; break;
      case IR_EQ: res = (c1 == c2); break;
      case IR_NE: res = (c1 != c2); break;
      case IR_LT: res = (c1 < c2); break;
      case IR_LE: res = (c1 <= c2); break;
      case IR_GT: res = (c1 > c2); break;
      case IR_GE: res = (c1 >= c2); break;
      default: folded = false; break;
      }

      if (folded && insn->dst) {
        insn->kind = IR_IMM;
        insn->imm = res;
        insn->src1 = NULL;
        insn->src2 = NULL;
        is_const[insn->dst->id] = true;
        const_vals[insn->dst->id] = res;
      }
    }
  }

  free(const_vals);
  free(is_const);
}

// Copy propagation pass
void ir_opt_copy_prop(IRFunction *fn) {
  IRVReg **aliases = calloc(fn->num_vregs, sizeof(IRVReg *));

  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->src1 && aliases[insn->src1->id])
      insn->src1 = aliases[insn->src1->id];
    if (insn->src2 && aliases[insn->src2->id])
      insn->src2 = aliases[insn->src2->id];
    if (insn->src3 && aliases[insn->src3->id])
      insn->src3 = aliases[insn->src3->id];

    if (insn->kind == IR_MOV && insn->dst && insn->src1 && !insn->dst->is_float && !insn->src1->is_float) {
      aliases[insn->dst->id] = insn->src1;
    }
  }

  free(aliases);
}

// Dead code elimination pass
void ir_opt_dce(IRFunction *fn) {
  bool *used = calloc(fn->num_vregs, sizeof(bool));

  // Mark all used VRegs
  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->src1) used[insn->src1->id] = true;
    if (insn->src2) used[insn->src2->id] = true;
    if (insn->src3) used[insn->src3->id] = true;
    for (int i = 0; i < insn->num_args; i++)
      if (insn->args[i])
        used[insn->args[i]->id] = true;
  }

  // Remove dead instructions with no side effects whose dst is not used
  for (IRInsn *insn = fn->head; insn;) {
    IRInsn *next = insn->next;

    if (insn->dst && !used[insn->dst->id]) {
      switch (insn->kind) {
      case IR_IMM:
      case IR_FIMM:
      case IR_MOV:
      case IR_ADD:
      case IR_SUB:
      case IR_MUL:
      case IR_DIV:
      case IR_MOD:
      case IR_BITAND:
      case IR_BITOR:
      case IR_BITXOR:
      case IR_SHL:
      case IR_SHR:
      case IR_NEG:
      case IR_BITNOT:
      case IR_LOGNOT:
      case IR_EQ:
      case IR_NE:
      case IR_LT:
      case IR_LE:
      case IR_GT:
      case IR_GE:
      case IR_CAST:
        // Remove dead insn
        if (insn->prev)
          insn->prev->next = insn->next;
        else
          fn->head = insn->next;
        if (insn->next)
          insn->next->prev = insn->prev;
        else
          fn->tail = insn->prev;
        break;
      default:
        break;
      }
    }

    insn = next;
  }

  free(used);
}

void ir_optimize(IRProg *prog) {
  if (!prog)
    return;

  for (int i = 0; i < prog->num_fns; i++) {
    IRFunction *fn = prog->fns[i];
    ir_opt_const_fold(fn);
    ir_opt_copy_prop(fn);
    ir_opt_dce(fn);
  }
}
