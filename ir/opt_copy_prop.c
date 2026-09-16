#include "ir/opt.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// LLIR Copy Propagation Optimization Pass
// ============================================================================
// Eliminates redundant intermediate copies (IR_MOV) by tracking register
// aliases and replacing source operands with their root origin vreg.
// Resets aliases across control flow barriers (calls, jumps, branches).

bool ir_opt_copy_prop(IRFunction *fn) {
  if (!fn || fn->num_vregs == 0)
    return false;

  bool changed = false;
  IRVReg **aliases = calloc(fn->num_vregs, sizeof(IRVReg *));

  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (ir_is_cf_barrier(insn)) {
      memset(aliases, 0, fn->num_vregs * sizeof(IRVReg *));
      continue;
    }

    if (insn->src1 && aliases[insn->src1->id]) {
      insn->src1 = aliases[insn->src1->id];
      changed = true;
    }
    if (insn->src2 && aliases[insn->src2->id]) {
      insn->src2 = aliases[insn->src2->id];
      changed = true;
    }
    if (insn->src3 && aliases[insn->src3->id]) {
      insn->src3 = aliases[insn->src3->id];
      changed = true;
    }
    if (insn->base_reg && aliases[insn->base_reg->id]) {
      insn->base_reg = aliases[insn->base_reg->id];
      changed = true;
    }
    if (insn->index_reg && aliases[insn->index_reg->id]) {
      insn->index_reg = aliases[insn->index_reg->id];
      changed = true;
    }
    for (int i = 0; i < insn->num_args; i++) {
      if (insn->args[i] && aliases[insn->args[i]->id]) {
        insn->args[i] = aliases[insn->args[i]->id];
        changed = true;
      }
    }

    // Invalidate any aliases where the modified dst was either the key or the value
    if (insn->dst) {
      aliases[insn->dst->id] = NULL;
      for (int i = 0; i < fn->num_vregs; i++) {
        if (aliases[i] == insn->dst)
          aliases[i] = NULL;
      }
    }

    // Record copy if moving integer registers of the same size
    if (insn->kind == IR_MOV && insn->dst && insn->src1 && !insn->dst->is_float && !insn->src1->is_float) {
      if (ir_vregs_same_size_and_float(insn->dst, insn->src1)) {
        IRVReg *root = insn->src1;
        while (aliases[root->id])
          root = aliases[root->id];
        if (root != insn->dst)
          aliases[insn->dst->id] = root;
      }
    }
  }

  free(aliases);
  return changed;
}
