#include "ir/regalloc.h"
#include <stdlib.h>

static void compute_live_intervals(IRFunction *fn) {
  for (int i = 0; i < fn->num_vregs; i++) {
    fn->vregs[i]->def_pos = -1;
    fn->vregs[i]->last_use_pos = -1;
  }

  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->dst) {
      if (insn->dst->def_pos == -1)
        insn->dst->def_pos = insn->pos;
      insn->dst->last_use_pos = insn->pos;
    }
    if (insn->src1) {
      if (insn->src1->def_pos == -1)
        insn->src1->def_pos = insn->pos;
      insn->src1->last_use_pos = insn->pos;
    }
    if (insn->src2) {
      if (insn->src2->def_pos == -1)
        insn->src2->def_pos = insn->pos;
      insn->src2->last_use_pos = insn->pos;
    }
    if (insn->src3) {
      if (insn->src3->def_pos == -1)
        insn->src3->def_pos = insn->pos;
      insn->src3->last_use_pos = insn->pos;
    }
    for (int i = 0; i < insn->num_args; i++) {
      if (insn->args[i]) {
        if (insn->args[i]->def_pos == -1)
          insn->args[i]->def_pos = insn->pos;
        insn->args[i]->last_use_pos = insn->pos;
      }
    }
  }

  // Extend live intervals across backward control flow (loops)
  bool loop_changed = true;
  while (loop_changed) {
    loop_changed = false;
    for (IRInsn *insn = fn->head; insn; insn = insn->next) {
      if ((insn->kind == IR_JMP || insn->kind == IR_BR) && insn->label) {
        int target_pos = -1;
        for (IRInsn *t = fn->head; t; t = t->next) {
          if (t->kind == IR_LABEL && t->label && !strcmp(t->label, insn->label)) {
            target_pos = t->pos;
            break;
          }
        }
        if (target_pos >= 0 && target_pos < insn->pos) {
          for (int i = 0; i < fn->num_vregs; i++) {
            IRVReg *v = fn->vregs[i];
            if (v->def_pos >= 0 && v->def_pos <= insn->pos && v->last_use_pos >= target_pos) {
              if (v->last_use_pos < insn->pos) {
                v->last_use_pos = insn->pos;
                loop_changed = true;
              }
            }
          }
        }
      }
    }
  }
}

void regalloc_function(IRFunction *fn, const RegAllocPool *pool) {
  if (!fn || fn->num_vregs == 0)
    return;

  ir_renumber_insns(fn);
  compute_live_intervals(fn);

  bool *gp_used = calloc(pool ? pool->num_gp_regs : 16, sizeof(bool));
  bool *fp_used = calloc(pool ? pool->num_fp_regs : 16, sizeof(bool));

  Obj *fn_obj = fn->fn_obj;
  ABI *abi = fn_obj ? get_fn_abi(fn_obj) : current_abi;

  int base = 0;
  if (abi && abi->get_spill_base)
    base = abi->get_spill_base(fn_obj);
  else if (fn_obj)
    base = fn_obj->stack_size;
  else
    base = fn->stack_size;

  int spill_offset = base;
  if (pool && pool->spill_base_offset > spill_offset)
    spill_offset = pool->spill_base_offset;

  typedef struct {
    int offset;
    int size;
    int last_use_pos;
  } SpillSlot;

  SpillSlot *slots = calloc(fn->num_vregs, sizeof(SpillSlot));
  int num_slots = 0;

  for (int i = 0; i < fn->num_vregs; i++) {
    IRVReg *v = fn->vregs[i];
    if (v->def_pos == -1)
      continue; // Unused vreg

    // Release physical registers from expired intervals
    if (pool) {
      for (int j = 0; j < i; j++) {
        IRVReg *prev = fn->vregs[j];
        if (prev->phys_reg >= 0 && prev->last_use_pos < v->def_pos) {
          if (prev->is_float && prev->phys_reg < pool->num_fp_regs)
            fp_used[prev->phys_reg] = false;
          else if (!prev->is_float && prev->phys_reg < pool->num_gp_regs)
            gp_used[prev->phys_reg] = false;
        }
      }
    }

    // Try to allocate an available physical register
    int allocated_reg = -1;
    if (pool) {
      if (v->is_float) {
        for (int r = 0; r < pool->num_fp_regs; r++) {
          if (!fp_used[r]) {
            allocated_reg = pool->fp_regs[r];
            fp_used[r] = true;
            break;
          }
        }
      } else {
        for (int r = 0; r < pool->num_gp_regs; r++) {
          if (!gp_used[r]) {
            allocated_reg = pool->gp_regs[r];
            gp_used[r] = true;
            break;
          }
        }
      }
    }

    if (allocated_reg >= 0) {
      v->phys_reg = allocated_reg;
      v->is_spilled = false;
    } else {
      int sz = v->ty ? v->ty->size : 8;
      int align = v->ty ? v->ty->align : 8;
      if (sz < 8) sz = 8;
      if (align < 8) align = 8;

      spill_offset += sz;
      spill_offset = align_to(spill_offset, align);
      v->spill_offset = -spill_offset;
      v->phys_reg = -1;
      v->is_spilled = true;
    }
  }

  free(slots);

  int align = (pool && pool->spill_align) ? pool->spill_align : 16;
  fn->stack_size = align_to(spill_offset, align);
  if (abi && abi->finalize_stack) {
    abi->finalize_stack(fn_obj, spill_offset);
  } else if (fn_obj) {
    fn_obj->stack_size = align_to(spill_offset, 16);
  }

  free(gp_used);
  free(fp_used);
}

void regalloc_prog(IRProg *prog, const RegAllocPool *pool) {
  if (!prog)
    return;

  for (int i = 0; i < prog->num_fns; i++) {
    regalloc_function(prog->fns[i], pool);
  }
}
