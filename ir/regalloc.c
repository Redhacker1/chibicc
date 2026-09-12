#include "ir/regalloc.h"
#include "ir/llir.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * Simple linear-scan register allocator.
 *
 * Public API is intentionally unchanged:
 *
 *     void regalloc_function(IRFunction *fn, const RegAllocPool *pool);
 *     void regalloc_prog(IRProg *prog, const RegAllocPool *pool);
 *
 * The allocator currently uses one conservative live interval per vreg.
 * This is intentionally simpler than a full SSA/live-range-splitting
 * allocator, but the implementation is structured so those features can
 * be added later.
 */


/* ------------------------------------------------------------------------- */
/* Helpers                                                                   */
/* ------------------------------------------------------------------------- */

static int max_int(int a, int b) {
  return a > b ? a : b;
}

static int min_int(int a, int b) {
  return a < b ? a : b;
}

static int normalize_size(const Type *ty) {
  int size = ty ? ty->size : 8;

  /*
   * Keep stack slots naturally word-sized for now. This is an allocator
   * policy, not a C ABI requirement.
   */
  if (size < 8)
    size = 8;

  return size;
}

static int normalize_align(const Type *ty) {
  int align = ty ? ty->align : 8;

  if (align < 8)
    align = 8;

  return align;
}

static bool vreg_is_aggregate(const IRVReg *v) {
  if (!v)
    return false;

  if (v->is_struct_val || v->struct_ty)
    return true;

  if (!v->ty)
    return false;

  switch (v->ty->kind) {
  case TY_STRUCT:
  case TY_UNION:
  case TY_ARRAY:
  case TY_VLA:
    return true;

  default:
    return false;
  }
}


/*
 * Return the index of a physical register in a register pool.
 *
 * IMPORTANT:
 *
 * pool->gp_regs[] / fp_regs[] contain actual physical register numbers.
 * They are not necessarily dense or equal to their array index.
 *
 * The old allocator accidentally used:
 *
 *     gp_used[phys_reg]
 *
 * even though gp_used was sized by num_gp_regs.
 *
 * We avoid that bug by keeping all allocation state indexed by pool slot.
 */
static int find_gp_pool_index(const RegAllocPool *pool, int phys_reg) {
  if (!pool)
    return -1;

  for (int i = 0; i < pool->num_gp_regs; i++) {
    if (pool->gp_regs[i] == phys_reg)
      return i;
  }
  for (int i = 0; i < pool->num_scratch_gp_regs; i++) {
    if (pool->scratch_gp_regs[i] == phys_reg)
      return pool->num_gp_regs + i;
  }

  return -1;
}

static int find_fp_pool_index(const RegAllocPool *pool, int phys_reg) {
  if (!pool)
    return -1;

  for (int i = 0; i < pool->num_fp_regs; i++) {
    if (pool->fp_regs[i] == phys_reg)
      return i;
  }

  return -1;
}


/* ------------------------------------------------------------------------- */
/* Def/use tracking                                                          */
/* ------------------------------------------------------------------------- */

static void mark_vreg_def(IRVReg *v, int pos) {
  if (!v)
    return;

  /*
   * A vreg should normally have exactly one definition.
   *
   * Keep the first definition rather than silently moving the start of the
   * interval if malformed/non-SSA IR happens to define it again.
   */
  if (v->def_pos == -1)
    v->def_pos = pos;
}

static void mark_vreg_use(IRVReg *v, int pos) {
  if (!v)
    return;

  /*
   * Do not manufacture a definition from a use.
   *
   * A source appearing without a definition is potentially a live-in value
   * (e.g. an incoming argument) or malformed IR. For the purposes of this
   * allocator, treat it as live from the beginning of the function.
   *
   * This preserves support for existing IR that doesn't explicitly create
   * argument definitions.
   */
  if (v->def_pos == -1)
    v->def_pos = 0;

  if (pos > v->last_use_pos)
    v->last_use_pos = pos;
}


/* ------------------------------------------------------------------------- */
/* Labels / backward edges                                                   */
/* ------------------------------------------------------------------------- */

typedef struct {
  const char *name;
  int pos;
} LabelPos;

typedef struct {
  int branch_pos;
  int target_pos;
} BackEdge;


/*
 * Build a label table once rather than searching the complete instruction
 * list for every branch.
 */
static LabelPos *build_label_table(IRFunction *fn, int *out_count) {
  int count = 0;

  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind == IR_LABEL && insn->label)
      count++;
  }

  *out_count = count;

  if (count == 0)
    return NULL;

  LabelPos *labels = calloc((size_t)count, sizeof(*labels));
  if (!labels)
    return NULL;

  int n = 0;

  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind != IR_LABEL || !insn->label)
      continue;

    labels[n].name = insn->label;
    labels[n].pos = insn->pos;
    n++;
  }

  return labels;
}

static int find_label_pos(const LabelPos *labels,
                          int num_labels,
                          const char *name) {
  if (!labels || !name)
    return -1;

  for (int i = 0; i < num_labels; i++) {
    if (labels[i].name && strcmp(labels[i].name, name) == 0)
      return labels[i].pos;
  }

  return -1;
}


/* ------------------------------------------------------------------------- */
/* Live interval computation                                                 */
/* ------------------------------------------------------------------------- */

static void compute_live_intervals(IRFunction *fn) {
  if (!fn)
    return;

  for (int i = 0; i < fn->num_vregs; i++) {
    IRVReg *v = fn->vregs[i];

    if (!v)
      continue;

    v->def_pos = -1;
    v->last_use_pos = -1;
  }

  /*
   * First pass: ordinary def/use intervals.
   *
   * A destination establishes a definition.
   * Sources establish uses.
   */
  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->dst) {
      insn->dst->def_insn = insn;
      mark_vreg_def(insn->dst, insn->pos);
    }

    if (insn->src1)
      mark_vreg_use(insn->src1, insn->pos);

    if (insn->src2)
      mark_vreg_use(insn->src2, insn->pos);

    if (insn->src3)
      mark_vreg_use(insn->src3, insn->pos);

    for (int i = 0; i < insn->num_args; i++) {
      if (insn->args[i])
        mark_vreg_use(insn->args[i], insn->pos);
    }
  }

  /*
   * Extend intervals across backward control flow.
   *
   * This is still intentionally conservative because this allocator has
   * not yet been given an explicit CFG. It is nevertheless considerably
   * cheaper than repeatedly searching the instruction list for labels.
   *
   * A vreg whose interval crosses the target of a backward branch must be
   * considered live across the back edge.
   */
  int num_labels = 0;
  LabelPos *labels = build_label_table(fn, &num_labels);

  if (!labels && num_labels != 0)
    return;

  int num_back_edges = 0;

  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind != IR_JMP && insn->kind != IR_BR && insn->kind != LLIR_BR_COND)
      continue;

    const char *target_labels[3] = { insn->label, insn->label_true, insn->label_false };
    for (int l = 0; l < 3; l++) {
      if (!target_labels[l])
        continue;
      int target_pos = find_label_pos(labels, num_labels, target_labels[l]);
      if (target_pos >= 0 && target_pos < insn->pos)
        num_back_edges++;
    }
  }

  if (num_back_edges == 0) {
    free(labels);
    return;
  }

  BackEdge *edges = calloc((size_t)num_back_edges, sizeof(*edges));

  if (!edges) {
    free(labels);
    return;
  }

  int edge_count = 0;

  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind != IR_JMP && insn->kind != IR_BR && insn->kind != LLIR_BR_COND)
      continue;

    const char *target_labels[3] = { insn->label, insn->label_true, insn->label_false };
    for (int l = 0; l < 3; l++) {
      if (!target_labels[l])
        continue;
      int target_pos = find_label_pos(labels, num_labels, target_labels[l]);
      if (target_pos >= 0 && target_pos < insn->pos) {
        edges[edge_count].branch_pos = insn->pos;
        edges[edge_count].target_pos = target_pos;
        edge_count++;
      }
    }
  }

  /*
   * Propagate loop liveness to a fixed point.
   *
   * This is conservative, but importantly it is now operating on a compact
   * edge list instead of repeatedly searching all instructions for labels.
   */
  bool changed;

  do {
    changed = false;

    for (int e = 0; e < edge_count; e++) {
      int branch_pos = edges[e].branch_pos;
      int target_pos = edges[e].target_pos;

      for (int i = 0; i < fn->num_vregs; i++) {
        IRVReg *v = fn->vregs[i];

        if (!v || v->def_pos < 0)
          continue;

        /*
         * If the value exists before the backedge and has a use somewhere
         * in/after the loop target, it must survive the backedge.
         */
        if (v->def_pos <= branch_pos &&
            v->last_use_pos >= target_pos &&
            v->last_use_pos < branch_pos) {
          v->last_use_pos = branch_pos;
          changed = true;
        }
      }
    }
  } while (changed);

  free(edges);
  free(labels);
}


/* ------------------------------------------------------------------------- */
/* Interval sorting                                                          */
/* ------------------------------------------------------------------------- */

static int compare_vregs_by_def_pos(const void *a, const void *b) {
  const IRVReg *va = *(const IRVReg *const *)a;
  const IRVReg *vb = *(const IRVReg *const *)b;

  if (va->def_pos < vb->def_pos)
    return -1;

  if (va->def_pos > vb->def_pos)
    return 1;

  /*
   * Make ordering deterministic for vregs defined at the same instruction.
   */
  if (va < vb)
    return -1;

  if (va > vb)
    return 1;

  return 0;
}


/* ------------------------------------------------------------------------- */
/* Active intervals                                                          */
/* ------------------------------------------------------------------------- */

typedef struct {
  IRVReg *vreg;
  int pool_index;
} ActiveReg;


/*
 * Active list is maintained sorted by last_use_pos.
 *
 * Since the register count is tiny on the targets we're interested in,
 * insertion/removal from this list is effectively constant time while
 * being substantially simpler than repeatedly scanning every previous vreg.
 */
static void active_insert(ActiveReg *active,
                          int *num_active,
                          IRVReg *vreg,
                          int pool_index) {
  int n = *num_active;

  int insert = n;

  while (insert > 0 &&
         active[insert - 1].vreg->last_use_pos > vreg->last_use_pos) {
    active[insert] = active[insert - 1];
    insert--;
  }

  active[insert].vreg = vreg;
  active[insert].pool_index = pool_index;

  *num_active = n + 1;
}

static void active_remove(ActiveReg *active,
                          int *num_active,
                          int index) {
  int n = *num_active;

  if (index < 0 || index >= n)
    return;

  for (int i = index; i + 1 < n; i++)
    active[i] = active[i + 1];

  *num_active = n - 1;
}


/*
 * Expire every interval whose last use is strictly before start.
 */
static void expire_active(ActiveReg *active,
                          int *num_active,
                          int start,
                          bool *gp_used,
                          bool *fp_used) {
  int i = 0;

  while (i < *num_active) {
    IRVReg *v = active[i].vreg;

    if (v->last_use_pos >= start) {
      i++;
      continue;
    }

    if (v->is_float) {
      if (active[i].pool_index >= 0)
        fp_used[active[i].pool_index] = false;
    } else {
      if (active[i].pool_index >= 0)
        gp_used[active[i].pool_index] = false;
    }

    active_remove(active, num_active, i);
  }
}


/* ------------------------------------------------------------------------- */
/* Register selection                                                        */
/* ------------------------------------------------------------------------- */

static bool interval_spans_call(IRFunction *fn, int def_pos, int last_use_pos) {
  if (!fn)
    return false;

  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind == LLIR_CALL) {
      if (insn->pos >= def_pos && insn->pos <= last_use_pos)
        return true;
    }
  }

  return false;
}

static int find_free_gp_reg(const RegAllocPool *pool,
                            const bool *gp_used,
                            bool can_use_scratch,
                            int *out_pool_index) {
  if (!pool)
    return -1;

  if (can_use_scratch && pool->num_scratch_gp_regs > 0) {
    for (int i = 0; i < pool->num_scratch_gp_regs; i++) {
      int idx = pool->num_gp_regs + i;
      if (!gp_used[idx]) {
        if (out_pool_index)
          *out_pool_index = idx;

        return pool->scratch_gp_regs[i];
      }
    }
  }

  for (int i = 0; i < pool->num_gp_regs; i++) {
    if (!gp_used[i]) {
      if (out_pool_index)
        *out_pool_index = i;

      return pool->gp_regs[i];
    }
  }

  return -1;
}

static int find_free_fp_reg(const RegAllocPool *pool,
                            const bool *fp_used,
                            int *out_pool_index) {
  if (!pool)
    return -1;

  for (int i = 0; i < pool->num_fp_regs; i++) {
    if (!fp_used[i]) {
      if (out_pool_index)
        *out_pool_index = i;

      return pool->fp_regs[i];
    }
  }

  return -1;
}


/* ------------------------------------------------------------------------- */
/* Spill slots                                                               */
/* ------------------------------------------------------------------------- */

typedef struct {
  int offset;
  int size;
  int align;
  int last_use_pos;
  bool in_use;
} SpillSlot;


/*
 * Allocate a spill slot.
 *
 * Reuses an existing slot when:
 *
 *   - its previous value is dead
 *   - it is large enough
 *   - its alignment is sufficient
 *
 * First-fit is sufficient here. Spill-slot allocation is not generally the
 * dominant cost of a compiler.
 */
static int allocate_spill_slot(SpillSlot *slots,
                               int *num_slots,
                               int max_slots,
                               IRVReg *v,
                               int *spill_offset) {
  int size = normalize_size(v->ty);
  int align = normalize_align(v->ty);

  /*
   * Prefer a dead compatible slot.
   */
  for (int i = 0; i < *num_slots; i++) {
    SpillSlot *slot = &slots[i];

    if (slot->in_use)
      continue;

    if (slot->size < size)
      continue;

    /*
     * The original slot offset must satisfy the requested alignment.
     */
    int absolute_offset = slot->offset < 0 ? -slot->offset : slot->offset;

    if ((absolute_offset % align) != 0)
      continue;

    slot->last_use_pos = v->last_use_pos;
    slot->in_use = true;

    v->spill_offset = slot->offset;
    v->phys_reg = -1;
    v->is_spilled = true;

    return i;
  }

  if (*num_slots >= max_slots)
    return -1;

  /*
   * Align before allocation, not after it.
   */
  *spill_offset = align_to(*spill_offset, align);
  *spill_offset += size;

  SpillSlot *slot = &slots[*num_slots];

  slot->offset = -*spill_offset;
  slot->size = size;
  slot->align = align;
  slot->last_use_pos = v->last_use_pos;
  slot->in_use = true;

  v->spill_offset = slot->offset;
  v->phys_reg = -1;
  v->is_spilled = true;

  (*num_slots)++;

  return *num_slots - 1;
}


/*
 * Mark slots whose value is dead before start as reusable.
 */
static void expire_spill_slots(SpillSlot *slots,
                               int num_slots,
                               int start) {
  for (int i = 0; i < num_slots; i++) {
    if (slots[i].in_use &&
        slots[i].last_use_pos < start) {
      slots[i].in_use = false;
    }
  }
}


/* ------------------------------------------------------------------------- */
/* Interval spilling                                                         */
/* ------------------------------------------------------------------------- */


/*
 * Spill an already-active interval.
 *
 * This is the important improvement over simply spilling the current
 * interval whenever all registers are occupied.
 *
 * If an active value lives much farther into the future than the new value,
 * it is generally better to evict the long-lived value and give its register
 * to the short-lived value.
 */
static bool spill_active_interval(ActiveReg *active,
                                  int *num_active,
                                  int active_index,
                                  SpillSlot *slots,
                                  int *num_slots,
                                  int max_slots,
                                  int *spill_offset,
                                  bool *gp_used,
                                  bool *fp_used) {
  if (active_index < 0 || active_index >= *num_active)
    return false;

  IRVReg *victim = active[active_index].vreg;

  if (!victim)
    return false;

  /*
   * Free the victim's physical register.
   */
  if (victim->is_float) {
    if (active[active_index].pool_index >= 0)
      fp_used[active[active_index].pool_index] = false;
  } else {
    if (active[active_index].pool_index >= 0)
      gp_used[active[active_index].pool_index] = false;
  }

  victim->phys_reg = -1;
  victim->is_spilled = true;

  /*
   * Give the victim a spill slot.
   *
   * The victim remains logically live, so its slot must remain occupied.
   */
  if (allocate_spill_slot(slots,
                          num_slots,
                          max_slots,
                          victim,
                          spill_offset) < 0) {
    /*
     * If we cannot allocate a spill slot, restore the register state.
     */
    if (victim->is_float) {
      if (active[active_index].pool_index >= 0)
        fp_used[active[active_index].pool_index] = true;
    } else {
      if (active[active_index].pool_index >= 0)
        gp_used[active[active_index].pool_index] = true;
    }

    return false;
  }

  /*
   * Remove victim from active.
   */
  active_remove(active, num_active, active_index);

  return true;
}


/* ------------------------------------------------------------------------- */
/* Main allocator                                                            */
/* ------------------------------------------------------------------------- */

void regalloc_function(IRFunction *fn, const RegAllocPool *pool) {
  if (!fn || fn->num_vregs <= 0)
    return;

  /*
   * Number instructions before calculating intervals.
   */
  ir_renumber_insns(fn);
  compute_live_intervals(fn);

  /*
   * Without a register pool there is nothing to allocate. Still compute
   * intervals above because callers may inspect them.
   */
  if (!pool)
    return;

  int gp_count = pool->num_gp_regs + pool->num_scratch_gp_regs;
  int fp_count = pool->num_fp_regs;

  /*
   * calloc(0, ...) is implementation-defined enough to not be worth
   * depending on for compiler infrastructure.
   */
  bool *gp_used = gp_count > 0
      ? calloc((size_t)gp_count, sizeof(bool))
      : NULL;

  bool *fp_used = fp_count > 0
      ? calloc((size_t)fp_count, sizeof(bool))
      : NULL;

  if ((gp_count > 0 && !gp_used) ||
      (fp_count > 0 && !fp_used)) {
    free(gp_used);
    free(fp_used);
    return;
  }

  /*
   * Determine the starting spill offset using the same ABI hooks as the
   * original implementation.
   */
  Obj *fn_obj = fn->fn_obj;
  ABI *abi = fn_obj ? get_fn_abi(fn_obj) : current_abi;

  int base = 0;

  if (abi && abi->get_spill_base) {
    base = abi->get_spill_base(fn_obj);
  } else if (fn_obj) {
    base = fn_obj->stack_size;
  } else {
    base = fn->stack_size;
  }

  int spill_offset = base;

  if (pool->spill_base_offset > spill_offset)
    spill_offset = pool->spill_base_offset;

  /*
   * There can be at most num_vregs simultaneously distinct spill slots.
   */
  SpillSlot *slots = calloc((size_t)fn->num_vregs, sizeof(*slots));

  /*
   * Active intervals cannot exceed the number of physical registers.
   * Allocate enough room for the larger register class.
   */
  int max_active = max_int(gp_count, fp_count);

  ActiveReg *active = max_active > 0
      ? calloc((size_t)max_active, sizeof(*active))
      : NULL;

  IRVReg **sorted_vregs = malloc(
      (size_t)fn->num_vregs * sizeof(*sorted_vregs));

  if (!slots || (max_active > 0 && !active) || !sorted_vregs) {
    free(sorted_vregs);
    free(active);
    free(slots);
    free(gp_used);
    free(fp_used);
    return;
  }

  /*
   * Initialize all vreg allocation fields.
   */
  for (int i = 0; i < fn->num_vregs; i++) {
    IRVReg *v = fn->vregs[i];
    if (v) {
      v->phys_reg = -1;
      v->is_spilled = false;
      v->spill_offset = 0;
    }
  }

  /*
   * Collect defined/live vregs.
   */
  int count = 0;

  for (int i = 0; i < fn->num_vregs; i++) {
    IRVReg *v = fn->vregs[i];

    if (!v)
      continue;

    if (v->def_pos >= 0)
      sorted_vregs[count++] = v;
  }

  /*
   * O(N log N), replacing the old quadratic bubble sort.
   */
  qsort(sorted_vregs,
        (size_t)count,
        sizeof(*sorted_vregs),
        compare_vregs_by_def_pos);

  int num_slots = 0;
  int num_active = 0;

  /*
   * Process intervals in increasing start order.
   */
  for (int i = 0; i < count; i++) {
    IRVReg *v = sorted_vregs[i];

    if (!v)
      continue;

    /*
     * Expire intervals that have ended before this one starts.
     */
    expire_active(active,
                  &num_active,
                  v->def_pos,
                  gp_used,
                  fp_used);

    /*
     * Spill slots become reusable at the same boundary.
     */
    expire_spill_slots(slots, num_slots, v->def_pos);

    /*
     * Reset allocation state in case this allocator is run more than once
     * on the same IR.
     */
    v->phys_reg = -1;
    v->is_spilled = false;
    v->spill_offset = 0;

    /*
     * Aggregates remain conservative for now.
     *
     * Full aggregate register classification belongs in the ABI layer,
     * not in this generic allocator.
     */
    bool is_aggregate = vreg_is_aggregate(v);
    bool can_use_scratch = !interval_spans_call(fn, v->def_pos, v->last_use_pos);

    if (!is_aggregate && v->def_insn != NULL) {
      int pool_index = -1;
      int reg = -1;

      if (v->is_float) {
        reg = find_free_fp_reg(pool, fp_used, &pool_index);
      } else {
        reg = find_free_gp_reg(pool, gp_used, can_use_scratch, &pool_index);
      }

      if (reg >= 0) {
        v->phys_reg = reg;
        v->is_spilled = false;

        if (v->is_float)
          fp_used[pool_index] = true;
        else
          gp_used[pool_index] = true;

        if (num_active < max_active) {
          active_insert(active,
                        &num_active,
                        v,
                        pool_index);
        }

        continue;
      }
    }

    /*
     * No free register.
     *
     * Look for the active interval with the farthest end point in the same
     * register class. If that interval lives longer than the current value,
     * evict it and use its register for the current value.
     */
    int victim_index = -1;
    int victim_end = v->last_use_pos;

    if (!is_aggregate && v->def_insn != NULL) {
      for (int a = 0; a < num_active; a++) {
        IRVReg *candidate = active[a].vreg;

        if (!candidate)
          continue;

        if (candidate->is_float != v->is_float)
          continue;

        if (!v->is_float && !can_use_scratch && active[a].pool_index >= pool->num_gp_regs)
          continue;

        if (candidate->last_use_pos > victim_end) {
          victim_index = a;
          victim_end = candidate->last_use_pos;
        }
      }
    }

    if (victim_index >= 0) {
      IRVReg *victim = active[victim_index].vreg;
      int victim_pool_index = active[victim_index].pool_index;
      int victim_phys_reg = victim->phys_reg;

      /*
       * Only evict the victim when it is actually more long-lived than the
       * current interval.
       */
      if (victim->last_use_pos > v->last_use_pos) {
        bool victim_is_float = victim->is_float;

        if (spill_active_interval(active,
                                  &num_active,
                                  victim_index,
                                  slots,
                                  &num_slots,
                                  fn->num_vregs,
                                  &spill_offset,
                                  gp_used,
                                  fp_used)) {
          /*
           * The victim's register is now free.
           */
          v->phys_reg = victim_phys_reg;
          v->is_spilled = false;

          if (victim_is_float)
            fp_used[victim_pool_index] = true;
          else
            gp_used[victim_pool_index] = true;

          active_insert(active,
                        &num_active,
                        v,
                        victim_pool_index);

          continue;
        }
      }
    }

    /*
     * Either this value is an aggregate, or spilling the current value is
     * preferable.
     */
    if (allocate_spill_slot(slots,
                            &num_slots,
                            fn->num_vregs,
                            v,
                            &spill_offset) < 0) {
      /*
       * This should only be reachable under allocation failure / pathological
       * conditions because there can be at most num_vregs distinct slots.
       *
       * Leave the vreg unallocated rather than corrupting the frame.
       */
      v->phys_reg = -1;
      v->is_spilled = true;
      v->spill_offset = 0;
    }
  }

  /*
   * Compute the set of physical registers used by this function.
   *
   * NOTE:
   * The existing public API exposes only callee_saved_mask through fn_obj,
   * and the ABI fields visible in the original implementation don't expose
   * a generic "is this register callee-saved?" callback.
   *
   * Preserve the original behavior here rather than inventing an ABI API.
   *
   * If your ABI structure has a callee-saved-register mask already, this
   * should be intersected with that mask.
   */
  uint64_t used_mask = 0;

  for (int i = 0; i < fn->num_vregs; i++) {
    IRVReg *v = fn->vregs[i];

    if (!v || v->phys_reg < 0 || v->is_float)
      continue;

    for (int r = 0; r < pool->num_gp_regs; r++) {
      if (pool->gp_regs[r] == v->phys_reg) {
        used_mask |= UINT64_C(1) << r;
        break;
      }
    }
  }

  if (fn_obj) {
    /*
     * The existing field is an int, so don't change the public structure
     * contract here.
     */
    fn_obj->callee_saved_mask = (int)(uint32_t)used_mask;
  }

  /*
   * Final stack size.
   *
   * spill_offset is the highest byte used by the allocator, including the
   * existing spill base.
   */
  int stack_align = pool->spill_align;

  if (stack_align <= 0)
    stack_align = 16;

  fn->stack_size = align_to(spill_offset, stack_align);

  if (abi && abi->finalize_stack) {
    abi->finalize_stack(fn_obj, spill_offset);
  } else if (fn_obj) {
    fn_obj->stack_size = align_to(spill_offset, 16);
  }

  free(sorted_vregs);
  free(active);
  free(slots);
  free(gp_used);
  free(fp_used);
}


void regalloc_prog(IRProg *prog, const RegAllocPool *pool) {
  if (!prog)
    return;

  for (int i = 0; i < prog->num_fns; i++) {
    if (prog->fns[i])
      regalloc_function(prog->fns[i], pool);
  }
}

