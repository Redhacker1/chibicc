#include "ir/hlir_opt.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// HLIR Dead Code Elimination Passes
// ============================================================================
// 1. Dead Value Elimination: Eliminates pure instructions whose destination
//    value is never read or used anywhere in the function.
// 2. Unreachable Code Elimination: Prunes dead code instructions following
//    unconditional jumps and returns until the next reachable label.

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
