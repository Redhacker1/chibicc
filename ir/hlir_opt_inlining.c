#include "ir/hlir_opt.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// HLIR Function Inlining Pass
// ============================================================================
// Inlines small, leaf or simple single-return functions into calling functions,
// eliminating call overhead and opening up cross-procedural optimization opportunities.

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

      Obj2 *callee_obj = insn->src1->var;
      if (!callee_obj->is_function || !callee_obj->is_definition)
        continue;

      HLIRFunction *callee = NULL;
      for (int j = 0; j < prog->num_fns; j++) {
        if (prog->fns[j]->fn_obj == callee_obj) {
          callee = prog->fns[j];
          break;
        }
      }

      if (!callee || callee == caller || callee->num_insns > 20)
        continue;

      int ret_count = 0;
      HLIRInsn *last_ret = NULL;
      for (HLIRInsn *ci = callee->head; ci; ci = ci->next) {
        if (ci->kind == HLIR_RET) {
          ret_count++;
          last_ret = ci;
        }
        if (ci->kind == HLIR_ASM || ci->kind == HLIR_ALLOCA || ci->kind == HLIR_CALL) {
          ret_count = 999;
          break;
        }
      }

      if (ret_count != 1 || !last_ret || last_ret != callee->tail)
        continue;

      // Check if callee has locals; if so, skip inlining to preserve frame layout
      if (callee->locals)
        continue;

      HLIRVal **val_map = calloc(callee->num_vals, sizeof(HLIRVal *));
      for (int v = 0; v < callee->num_vals; v++) {
        val_map[v] = hlir_new_val(caller, callee->vals[v]->ty);
        val_map[v]->var = callee->vals[v]->var;
      }

      // Allocate new local variables for each parameter in the caller
      Obj2 *param = callee->params;
      for (int a = 0; a < insn->num_args && param; a++, param = param->next) {
        Obj2 *local_param = calloc(1, sizeof(Obj2));
        local_param->name = param->name;
        local_param->ty = param->ty;
        local_param->is_local = true;
        local_param->align = param->ty->align;
        if (caller->fn_obj) {
          local_param->next = caller->fn_obj->locals;
          caller->fn_obj->locals = local_param;
        }
        HLIRInsn *param_store = hlir_new_insn(HLIR_STORE_VAR);
        param_store->var = local_param;
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
