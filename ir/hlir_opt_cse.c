#include "ir/hlir_opt.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// HLIR Local Common Subexpression Elimination (CSE) Pass
// ============================================================================
// Detects identical pure computations within the same basic block and replaces
// redundant evaluations with copies of previously computed values.

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
