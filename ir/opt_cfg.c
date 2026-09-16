#include "ir/opt.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// LLIR Control Flow Simplification Pass
// ============================================================================
// Simplifies control flow by:
// 1. Evaluating constant branches and rewriting to unconditional jumps
// 2. Removing unreachable dead code following unconditional jumps or returns
// 3. Eliminating redundant unconditional jumps immediately preceding target label
// 4. Merging conditional branches where true and false labels are identical
// 5. Eliminating redundant false branch targets matching immediate fallthrough label

bool ir_opt_cfg_simplify(IRFunction *fn) {
  if (!fn || !fn->head)
    return false;

  bool changed = false;

  // Pass 0: Simplify conditional branches with constant condition within same basic block
  const int num_v = fn->num_vregs ? fn->num_vregs : 1;
  int64_t *const_vals = calloc(num_v, sizeof(int64_t));
  bool *is_const = calloc(num_v, sizeof(bool));

  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind == IR_BR && insn->src1 && is_const[insn->src1->id]) {
      const int64_t c = const_vals[insn->src1->id];
      char *target = (c != 0) ? insn->label_true : insn->label_false;
      if (target) {
        insn->kind = IR_JMP;
        insn->label = target;
        insn->label_true = NULL;
        insn->label_false = NULL;
        insn->src1 = NULL;
        changed = true;
      } else {
        IRInsn *to_remove = insn;
        insn = insn->prev ? insn->prev : fn->head;
        ir_remove_insn(fn, to_remove);
        changed = true;
        if (!insn) break;
      }
    }

    if (ir_is_cf_barrier(insn)) {
      memset(is_const, 0, num_v * sizeof(bool));
      continue;
    }

    if (insn->kind == IR_IMM && insn->dst) {
      is_const[insn->dst->id] = true;
      const_vals[insn->dst->id] = insn->imm;
      continue;
    }
  }

  free(const_vals);
  free(is_const);

  // Pass 1: Eliminate unreachable instructions following unconditional jumps/returns
  for (const IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind == IR_JMP || insn->kind == IR_RET) {
      while (insn->next && insn->next->kind != IR_LABEL) {
        ir_remove_insn(fn, insn->next);
        changed = true;
      }
    }
  }

  // Pass 2: Eliminate redundant unconditional jumps immediately preceding their target label
  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind == IR_JMP && insn->label) {
      const IRInsn *nxt = ir_next_non_nop_const(insn->next);
      if (ir_is_label_match(nxt, insn->label)) {
        IRInsn *to_remove = insn;
        insn = insn->prev ? insn->prev : fn->head;
        ir_remove_insn(fn, to_remove);
        changed = true;
        if (!insn)
          break;
      }
    }
  }

  // Pass 3: Simplify conditional branches where true and false target are identical
  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind == IR_BR && insn->label_true && insn->label_false) {
      if (!strcmp(insn->label_true, insn->label_false)) {
        insn->kind = IR_JMP;
        insn->label = insn->label_true;
        insn->label_true = NULL;
        insn->label_false = NULL;
        insn->src1 = NULL;
        changed = true;
      }
    }
  }

  // Pass 4: Simplify conditional branches where false target is the immediate next label
  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind == IR_BR && insn->label_false) {
      const IRInsn *nxt = ir_next_non_nop_const(insn->next);
      if (ir_is_label_match(nxt, insn->label_false)) {
        insn->label_false = NULL;
        changed = true;
      }
    }
  }

  return changed;
}
