#include "ir/opt.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// LLIR Dead Code Elimination (DCE) Pass
// ============================================================================
// Performs iterative fixed-point dead code elimination.
// Instructions that define a destination register which is never used
// anywhere else in the function and have no side effects are eliminated.

bool ir_opt_dce(IRFunction *fn) {
  if (!fn || fn->num_vregs == 0)
    return false;

  bool overall_changed = false;
  bool changed = true;
  bool *used = calloc(fn->num_vregs, sizeof(bool));

  while (changed) {
    changed = false;
    memset(used, 0, fn->num_vregs * sizeof(bool));

    // Mark all used VRegs across instructions
    for (const IRInsn *insn = fn->head; insn; insn = insn->next) {
      if (insn->src1) used[insn->src1->id] = true;
      if (insn->src2) used[insn->src2->id] = true;
      if (insn->src3) used[insn->src3->id] = true;
      if (insn->base_reg) used[insn->base_reg->id] = true;
      if (insn->index_reg) used[insn->index_reg->id] = true;
      for (int i = 0; i < insn->num_args; i++) {
        if (insn->args[i])
          used[insn->args[i]->id] = true;
      }
    }

    // Remove dead instructions with no side effects whose dst is not used
    for (IRInsn *insn = fn->head; insn;) {
      IRInsn *next = insn->next;

      if (insn->dst && !used[insn->dst->id] && !ir_insn_has_side_effects(insn)) {
        ir_remove_insn(fn, insn);
        changed = true;
        overall_changed = true;
      }

      insn = next;
    }
  }

  free(used);
  return overall_changed;
}
