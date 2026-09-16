#include "ir/hlir_opt.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// HLIR Copy Propagation Pass
// ============================================================================
// Tracks value aliases through casts / copies and propagates the original
// root values across uses within basic blocks.

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

    const HLIRVal *s1 = insn->src1;
    const HLIRVal *s2 = insn->src2;
    const HLIRVal *s3 = insn->src3;
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
