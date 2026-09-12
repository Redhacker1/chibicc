#include "ir/opt.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Analysis Infrastructure: CFG, Dominator Tree, Liveness
// ============================================================================

int opt_max_passes = 2;

void ir_opt_set_max_passes(int max_passes) {
  if (max_passes > 0)
    opt_max_passes = max_passes;
}

int ir_opt_get_max_passes(void) {
  return opt_max_passes;
}

void ir_invalidate_analyses(IRPassContext *ctx) {
  if (!ctx)
    return;
  if (ctx->cfg) {
    ir_free_cfg(ctx->cfg);
    ctx->cfg = NULL;
  }
  if (ctx->dom_tree) {
    ir_free_dom_tree(ctx->dom_tree);
    ctx->dom_tree = NULL;
  }
  if (ctx->liveness) {
    ir_free_liveness(ctx->liveness);
    ctx->liveness = NULL;
  }
}

static IRBasicBlock *new_basic_block(int id, char *label, IRInsn *first) {
  IRBasicBlock *bb = calloc(1, sizeof(IRBasicBlock));
  bb->id = id;
  bb->label = label;
  bb->first = first;
  bb->last = first;
  bb->pred_cap = 4;
  bb->preds = calloc(bb->pred_cap, sizeof(IRBasicBlock *));
  bb->succ_cap = 4;
  bb->succs = calloc(bb->succ_cap, sizeof(IRBasicBlock *));
  return bb;
}

static void bb_add_succ(IRBasicBlock *from, IRBasicBlock *to) {
  if (!from || !to)
    return;

  // Avoid duplicates
  for (int i = 0; i < from->num_succs; i++)
    if (from->succs[i] == to)
      return;

  if (from->num_succs >= from->succ_cap) {
    from->succ_cap *= 2;
    from->succs = realloc(from->succs, sizeof(IRBasicBlock *) * from->succ_cap);
  }
  from->succs[from->num_succs++] = to;

  if (to->num_preds >= to->pred_cap) {
    to->pred_cap *= 2;
    to->preds = realloc(to->preds, sizeof(IRBasicBlock *) * to->pred_cap);
  }
  to->preds[to->num_preds++] = from;
}

IRCFG *ir_build_cfg(IRFunction *fn) {
  if (!fn || !fn->head)
    return NULL;

  IRCFG *cfg = calloc(1, sizeof(IRCFG));
  cfg->fn = fn;
  cfg->block_cap = 16;
  cfg->blocks = calloc(cfg->block_cap, sizeof(IRBasicBlock *));

  // Step 1: Identify leaders and create basic blocks
  // A leader is:
  // 1) First instruction
  // 2) Any IR_LABEL
  // 3) Instruction immediately following a branch/jump/ret
  IRBasicBlock *cur_bb = NULL;
  int block_count = 0;

  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    bool is_leader = false;
    char *label = NULL;

    if (insn == fn->head) {
      is_leader = true;
    } else if (insn->kind == IR_LABEL) {
      is_leader = true;
      label = insn->label;
    } else if (insn->prev && ir_insn_is_terminator(insn->prev)) {
      is_leader = true;
    }

    if (is_leader || !cur_bb) {
      if (cur_bb)
        cur_bb->last = insn->prev;

      if (block_count >= cfg->block_cap) {
        cfg->block_cap *= 2;
        cfg->blocks = realloc(cfg->blocks, sizeof(IRBasicBlock *) * cfg->block_cap);
      }
      cur_bb = new_basic_block(block_count, label, insn);
      cfg->blocks[block_count++] = cur_bb;
    }
  }

  if (cur_bb)
    cur_bb->last = fn->tail;

  cfg->num_blocks = block_count;
  if (block_count > 0)
    cfg->entry_block = cfg->blocks[0];

  // Step 2: Connect CFG edges
  for (int i = 0; i < cfg->num_blocks; i++) {
    IRBasicBlock *bb = cfg->blocks[i];
    IRInsn *term = bb->last;

    if (!term)
      continue;

    if (term->kind == IR_JMP) {
      if (term->label) {
        for (int j = 0; j < cfg->num_blocks; j++) {
          if (cfg->blocks[j]->label && !strcmp(cfg->blocks[j]->label, term->label)) {
            bb_add_succ(bb, cfg->blocks[j]);
            break;
          }
        }
      }
    } else if (term->kind == IR_BR) {
      if (term->label_true) {
        for (int j = 0; j < cfg->num_blocks; j++) {
          if (cfg->blocks[j]->label && !strcmp(cfg->blocks[j]->label, term->label_true)) {
            bb_add_succ(bb, cfg->blocks[j]);
            break;
          }
        }
      }
      if (term->label_false) {
        for (int j = 0; j < cfg->num_blocks; j++) {
          if (cfg->blocks[j]->label && !strcmp(cfg->blocks[j]->label, term->label_false)) {
            bb_add_succ(bb, cfg->blocks[j]);
            break;
          }
        }
      } else if (i + 1 < cfg->num_blocks) {
        // Fallthrough
        bb_add_succ(bb, cfg->blocks[i + 1]);
      }
    } else if (term->kind == IR_RET) {
      // Exit point: no successor
    } else {
      // Normal fallthrough to next block
      if (i + 1 < cfg->num_blocks)
        bb_add_succ(bb, cfg->blocks[i + 1]);
    }
  }

  // Step 3: Compute reachability
  if (cfg->entry_block) {
    IRBasicBlock **queue = calloc(cfg->num_blocks, sizeof(IRBasicBlock *));
    int qhead = 0, qtail = 0;

    cfg->entry_block->reachable = true;
    queue[qtail++] = cfg->entry_block;

    while (qhead < qtail) {
      const IRBasicBlock *b = queue[qhead++];
      for (int s = 0; s < b->num_succs; s++) {
        IRBasicBlock *succ = b->succs[s];
        if (!succ->reachable) {
          succ->reachable = true;
          queue[qtail++] = succ;
        }
      }
    }
    free(queue);
  }

  return cfg;
}

void ir_free_cfg(IRCFG *cfg) {
  if (!cfg)
    return;
  for (int i = 0; i < cfg->num_blocks; i++) {
    IRBasicBlock *bb = cfg->blocks[i];
    if (bb) {
      free(bb->preds);
      free(bb->succs);
      free(bb);
    }
  }
  free(cfg->blocks);
  free(cfg);
}

void ir_dump_cfg(FILE *out, IRCFG *cfg) {
  if (!out || !cfg)
    return;

  fprintf(out, "CFG for %s (%d blocks):\n", cfg->fn ? cfg->fn->name : "(anon)", cfg->num_blocks);
  for (int i = 0; i < cfg->num_blocks; i++) {
    const IRBasicBlock *bb = cfg->blocks[i];
    fprintf(out, "  Block BB%d [label=%s, reachable=%s]:\n",
            bb->id, bb->label ? bb->label : "none", bb->reachable ? "true" : "false");
    fprintf(out, "    Preds: ");
    for (int p = 0; p < bb->num_preds; p++)
      fprintf(out, "BB%d ", bb->preds[p]->id);
    fprintf(out, "\n    Succs: ");
    for (int s = 0; s < bb->num_succs; s++)
      fprintf(out, "BB%d ", bb->succs[s]->id);
    fprintf(out, "\n");
  }
}

IRCFG *ir_get_or_build_cfg(IRFunction *fn, IRPassContext *ctx) {
  if (ctx && ctx->cfg)
    return ctx->cfg;
  IRCFG *cfg = ir_build_cfg(fn);
  if (ctx)
    ctx->cfg = cfg;
  return cfg;
}

// Dominator Tree Construction (iterative algorithm)
IRDomTree *ir_build_dom_tree(IRCFG *cfg) {
  if (!cfg || cfg->num_blocks == 0)
    return NULL;

  IRDomTree *dt = calloc(1, sizeof(IRDomTree));
  dt->cfg = cfg;
  dt->idom = calloc(cfg->num_blocks, sizeof(IRBasicBlock *));
  dt->dom_depth = calloc(cfg->num_blocks, sizeof(int));

  if (!cfg->entry_block)
    return dt;

  dt->idom[cfg->entry_block->id] = cfg->entry_block;

  bool changed = true;
  while (changed) {
    changed = false;
    for (int i = 0; i < cfg->num_blocks; i++) {
      const IRBasicBlock *b = cfg->blocks[i];
      if (b == cfg->entry_block || !b->reachable)
        continue;

      // Find first processed predecessor
      IRBasicBlock *new_idom = NULL;
      for (int p = 0; p < b->num_preds; p++) {
        IRBasicBlock *pred = b->preds[p];
        if (dt->idom[pred->id] != NULL) {
          new_idom = pred;
          break;
        }
      }

      if (!new_idom)
        continue;

      // Intersect with remaining processed predecessors
      for (int p = 0; p < b->num_preds; p++) {
        IRBasicBlock *pred = b->preds[p];
        if (pred != new_idom && dt->idom[pred->id] != NULL) {
          IRBasicBlock *finger1 = pred;
          IRBasicBlock *finger2 = new_idom;
          while (finger1 != finger2) {
            while (finger1 && finger2 && finger1->id > finger2->id)
              finger1 = (dt->idom[finger1->id] != finger1) ? dt->idom[finger1->id] : NULL;
            while (finger1 && finger2 && finger2->id > finger1->id)
              finger2 = (dt->idom[finger2->id] != finger2) ? dt->idom[finger2->id] : NULL;
            if (!finger1 || !finger2) {
              finger1 = finger2 = cfg->entry_block;
              break;
            }
          }
          new_idom = finger1;
        }
      }

      if (dt->idom[b->id] != new_idom) {
        dt->idom[b->id] = new_idom;
        changed = true;
      }
    }
  }

  // Compute depth
  for (int i = 0; i < cfg->num_blocks; i++) {
    IRBasicBlock *b = cfg->blocks[i];
    int depth = 0;
    const IRBasicBlock *curr = b;
    while (curr && dt->idom[curr->id] && dt->idom[curr->id] != curr) {
      depth++;
      curr = dt->idom[curr->id];
    }
    dt->dom_depth[b->id] = depth;
  }

  return dt;
}

void ir_free_dom_tree(IRDomTree *dt) {
  if (!dt)
    return;
  free(dt->idom);
  free(dt->dom_depth);
  free(dt);
}

bool ir_dom_dominates(IRDomTree *dt, IRBasicBlock *a, IRBasicBlock *b) {
  if (!dt || !a || !b)
    return false;
  if (a == b)
    return true;

  const IRBasicBlock *curr = b;
  while (curr && dt->idom[curr->id] && dt->idom[curr->id] != curr) {
    curr = dt->idom[curr->id];
    if (curr == a)
      return true;
  }
  return false;
}

// Liveness Analysis
IRLiveness *ir_compute_liveness(IRFunction *fn) {
  if (!fn)
    return NULL;

  IRLiveness *liv = calloc(1, sizeof(IRLiveness));
  liv->fn = fn;
  liv->num_vregs = fn->num_vregs;
  liv->def_pos = calloc(fn->num_vregs, sizeof(int));
  liv->last_use_pos = calloc(fn->num_vregs, sizeof(int));
  liv->is_live_across_calls = calloc(fn->num_vregs, sizeof(bool));

  for (int i = 0; i < fn->num_vregs; i++) {
    liv->def_pos[i] = -1;
    liv->last_use_pos[i] = -1;
  }

  int last_call_pos = -1;
  for (const IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind == IR_CALL)
      last_call_pos = insn->pos;

    if (insn->dst && insn->dst->id >= 0 && insn->dst->id < fn->num_vregs) {
      if (liv->def_pos[insn->dst->id] == -1)
        liv->def_pos[insn->dst->id] = insn->pos;
      liv->last_use_pos[insn->dst->id] = insn->pos;
    }

    IRVReg *srcs[] = {insn->src1, insn->src2, insn->src3};
    for (int s = 0; s < 3; s++) {
      const IRVReg *v = srcs[s];
      if (v && v->id >= 0 && v->id < fn->num_vregs) {
        if (liv->def_pos[v->id] == -1)
          liv->def_pos[v->id] = insn->pos;
        liv->last_use_pos[v->id] = insn->pos;
        if (last_call_pos > liv->def_pos[v->id])
          liv->is_live_across_calls[v->id] = true;
      }
    }

    for (int a = 0; a < insn->num_args; a++) {
      const IRVReg *v = insn->args[a];
      if (v && v->id >= 0 && v->id < fn->num_vregs) {
        if (liv->def_pos[v->id] == -1)
          liv->def_pos[v->id] = insn->pos;
        liv->last_use_pos[v->id] = insn->pos;
      }
    }
  }

  return liv;
}

void ir_free_liveness(IRLiveness *liveness) {
  if (!liveness)
    return;
  free(liveness->def_pos);
  free(liveness->last_use_pos);
  free(liveness->is_live_across_calls);
  free(liveness);
}

// ============================================================================
// IR Verifier
// ============================================================================

bool ir_verify_function(IRFunction *fn, char **err_out) {
  if (!fn) {
    if (err_out) *err_out = "Null IRFunction";
    return false;
  }

  const IRInsn *prev = NULL;
  int count = 0;

  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->prev != prev) {
      if (err_out) *err_out = "Broken prev link in instruction list";
      return false;
    }

    if (insn->dst && (insn->dst->id < 0 || insn->dst->id >= fn->num_vregs)) {
      if (err_out) *err_out = "Instruction dst vreg id out of range";
      return false;
    }
    if (insn->src1 && (insn->src1->id < 0 || insn->src1->id >= fn->num_vregs)) {
      if (err_out) *err_out = "Instruction src1 vreg id out of range";
      return false;
    }
    if (insn->src2 && (insn->src2->id < 0 || insn->src2->id >= fn->num_vregs)) {
      if (err_out) *err_out = "Instruction src2 vreg id out of range";
      return false;
    }
    if (insn->src3 && (insn->src3->id < 0 || insn->src3->id >= fn->num_vregs)) {
      if (err_out) *err_out = "Instruction src3 vreg id out of range";
      return false;
    }

    prev = insn;
    count++;
  }

  if (fn->tail != prev) {
    if (err_out) *err_out = "Function tail pointer does not match end of list";
    return false;
  }

  return true;
}

bool ir_verify_prog(IRProg *prog, char **err_out) {
  if (!prog)
    return true;

  for (int i = 0; i < prog->num_fns; i++) {
    if (!ir_verify_function(prog->fns[i], err_out))
      return false;
  }
  return true;
}

// ============================================================================
// Optimization Passes
// ============================================================================

// 1. Copy Propagation Pass
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
    for (int i = 0; i < insn->num_args; i++) {
      if (insn->args[i] && aliases[insn->args[i]->id]) {
        insn->args[i] = aliases[insn->args[i]->id];
        changed = true;
      }
    }

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

// 3. Dead Code Elimination Pass
bool ir_opt_dce(IRFunction *fn) {
  if (!fn || fn->num_vregs == 0)
    return false;

  bool overall_changed = false;
  bool changed = true;
  bool *used = calloc(fn->num_vregs, sizeof(bool));

  while (changed) {
    changed = false;
    memset(used, 0, fn->num_vregs * sizeof(bool));

    // Mark all used VRegs
    for (const IRInsn *insn = fn->head; insn; insn = insn->next) {
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

// 4. Control Flow Simplification Pass
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

static IRVReg *make_imm_vreg(IRFunction *fn, IRInsn *before, int64_t val, Type *ty) {
  IRVReg *v = ir_new_vreg(fn, ty ? ty : ty_long);
  IRInsn *imm_insn = ir_new_insn(IR_IMM);
  imm_insn->dst = v;
  imm_insn->imm = val;
  imm_insn->ty = ty ? ty : ty_long;
  v->def_insn = imm_insn;
  ir_insert_before(fn, before, imm_insn);
  return v;
}

static int get_power_of_two(int64_t val) {
  if (val <= 0 || (val & (val - 1)) != 0)
    return -1;
  int shift = 0;
  while (val > 1) {
    val >>= 1;
    shift++;
  }
  return shift;
}

IRKind ir_op_commute(IRKind kind) {
  switch (kind) {
  case IR_ADD:
  case IR_MUL:
  case IR_BITAND:
  case IR_BITOR:
  case IR_BITXOR:
  case IR_EQ:
  case IR_NE:
    return kind;
  case IR_LT: return IR_GT;
  case IR_LE: return IR_GE;
  case IR_GT: return IR_LT;
  case IR_GE: return IR_LE;
  default:
    return 0;
  }
}

bool ir_is_associative_binop(IRKind kind) {
  return kind == IR_ADD || kind == IR_MUL ||
         kind == IR_BITAND || kind == IR_BITOR || kind == IR_BITXOR;
}

bool ir_eval_unary_imm(IRKind kind, int64_t c, int64_t *out) {
  switch (kind) {
  case IR_NEG:    *out = -c; return true;
  case IR_BITNOT: *out = ~c; return true;
  case IR_LOGNOT: *out = !c; return true;
  default:        return false;
  }
}

bool ir_eval_binary_imm(IRKind kind, int64_t c1, int64_t c2, bool is_uns, int64_t *out) {
  switch (kind) {
  case IR_ADD:    *out = c1 + c2; return true;
  case IR_SUB:    *out = c1 - c2; return true;
  case IR_MUL:    *out = c1 * c2; return true;
  case IR_DIV:
    if (c2 == 0) return false;
    *out = is_uns ? (int64_t)((uint64_t)c1 / (uint64_t)c2) : (c1 / c2);
    return true;
  case IR_MOD:
    if (c2 == 0) return false;
    *out = is_uns ? (int64_t)((uint64_t)c1 % (uint64_t)c2) : (c1 % c2);
    return true;
  case IR_BITAND: *out = c1 & c2; return true;
  case IR_BITOR:  *out = c1 | c2; return true;
  case IR_BITXOR: *out = c1 ^ c2; return true;
  case IR_SHL:    *out = c1 << (c2 & 63); return true;
  case IR_SHR:    *out = is_uns ? (int64_t)((uint64_t)c1 >> (c2 & 63)) : (c1 >> (c2 & 63)); return true;
  case IR_EQ:     *out = (c1 == c2); return true;
  case IR_NE:     *out = (c1 != c2); return true;
  case IR_LT:     *out = is_uns ? ((uint64_t)c1 < (uint64_t)c2) : (c1 < c2); return true;
  case IR_LE:     *out = is_uns ? ((uint64_t)c1 <= (uint64_t)c2) : (c1 <= c2); return true;
  case IR_GT:     *out = is_uns ? ((uint64_t)c1 > (uint64_t)c2) : (c1 > c2); return true;
  case IR_GE:     *out = is_uns ? ((uint64_t)c1 >= (uint64_t)c2) : (c1 >= c2); return true;
  default:        return false;
  }
}

bool ir_is_right_identity(IRKind kind, int64_t c) {
  switch (kind) {
  case IR_ADD:
  case IR_SUB:
  case IR_BITOR:
  case IR_BITXOR:
    return c == 0;
  case IR_SHL:
  case IR_SHR:
    return (c & 63) == 0;
  case IR_MUL:
  case IR_DIV:
    return c == 1;
  case IR_BITAND:
    return c == -1;
  default:
    return false;
  }
}

bool ir_is_absorbing_element(IRKind kind, int64_t c, bool is_uns, int64_t *out_val) {
  switch (kind) {
  case IR_MUL:
  case IR_BITAND:
    if (c == 0) {
      *out_val = 0;
      return true;
    }
    return false;
  case IR_MOD:
    if (c == 1 || (!is_uns && c == -1)) {
      *out_val = 0;
      return true;
    }
    return false;
  default:
    return false;
  }
}

bool ir_eval_same_operands(IRKind kind, int64_t *out_val) {
  switch (kind) {
  case IR_SUB:
  case IR_BITXOR:
  case IR_NE:
  case IR_LT:
  case IR_GT:
    *out_val = 0;
    return true;
  case IR_EQ:
  case IR_LE:
  case IR_GE:
    *out_val = 1;
    return true;
  default:
    return false;
  }
}

bool ir_is_involution_pair(IRKind outer, IRKind inner) {
  return (outer == IR_NEG && inner == IR_NEG) ||
         (outer == IR_BITNOT && inner == IR_BITNOT);
}

static bool ir_is_comparison(IRKind kind) {
  return kind == IR_EQ || kind == IR_NE || kind == IR_LT ||
         kind == IR_LE || kind == IR_GT || kind == IR_GE;
}

static IRKind ir_invert_cmp(IRKind kind) {
  switch (kind) {
  case IR_EQ: return IR_NE;
  case IR_NE: return IR_EQ;
  case IR_LT: return IR_GE;
  case IR_LE: return IR_GT;
  case IR_GT: return IR_LE;
  case IR_GE: return IR_LT;
  default: return 0;
  }
}

// 5. Low-Level Peephole Optimization Pass
bool ir_opt_peephole(IRFunction *fn) {
  if (!fn || fn->num_vregs == 0)
    return false;

  bool changed = false;

  int *def_count = calloc(fn->num_vregs, sizeof(int));
  for (int i = 0; i < fn->num_vregs; i++) {
    if (fn->vregs[i])
      fn->vregs[i]->def_insn = NULL;
  }
  for (IRInsn *i = fn->head; i; i = i->next) {
    if (i->dst) {
      def_count[i->dst->id]++;
      i->dst->def_insn = i;
    }
  }
  for (int i = 0; i < fn->num_vregs; i++) {
    if (def_count[i] != 1 && fn->vregs[i])
      fn->vregs[i]->def_insn = NULL;
  }
  free(def_count);

  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    // 0. Branch condition simplification (e.g. branch on !cond)
    if (insn->kind == IR_BR && insn->src1 && insn->src1->def_insn) {
      IRInsn *def = insn->src1->def_insn;
      if (def->kind == IR_LOGNOT && def->src1 && insn->label_true && insn->label_false) {
        insn->src1 = def->src1;
        char *tmp = insn->label_true;
        insn->label_true = insn->label_false;
        insn->label_false = tmp;
        changed = true;
      } else if (def->kind == IR_NE && def->src2 && def->src2->def_insn &&
                 def->src2->def_insn->kind == IR_IMM && def->src2->def_insn->imm == 0 &&
                 def->src1 && def->src1->def_insn && ir_is_comparison(def->src1->def_insn->kind)) {
        insn->src1 = def->src1;
        changed = true;
      }
    }

    // 1. Redundant self-move: dst = mov dst
    if (insn->kind == IR_MOV && insn->dst && insn->src1 && insn->dst == insn->src1) {
      const IRInsn *next = insn->next;
      ir_remove_insn(fn, insn);
      insn = next ? next->prev : fn->tail;
      changed = true;
      if (!insn) break;
      continue;
    }

    // 2. Redundant load-after-store elimination:
    // STORE addr, val  followed closely by  dst = LOAD addr
    if (insn->kind == IR_STORE && insn->src1 && insn->src2) {
      const IRVReg *addr = insn->src1;
      IRVReg *val = insn->src2;

      for (IRInsn *cur = insn->next; cur; cur = cur->next) {
        if (ir_is_cf_barrier(cur) || ir_is_mem_clobber(cur))
          break; // Barrier or memory clobber

        if (cur->kind == IR_LOAD && cur->src1 == addr && cur->dst && val &&
            ir_vregs_same_size_and_float(cur->dst, val)) {
          cur->kind = IR_MOV;
          cur->src1 = val;
          changed = true;
          break;
        }
      }
    }

    // 3. Dead store elimination:
    // STORE addr, val1 followed by STORE addr, val2 with no reads or barriers in between
    if (insn->kind == IR_STORE && insn->src1 && insn->src2) {
      const IRVReg *addr = insn->src1;
      for (const IRInsn *cur = insn->next; cur; cur = cur->next) {
        if (ir_is_cf_barrier(cur) || ir_is_mem_read(cur))
          break; // Barrier or reads memory
        if (cur->kind == IR_STORE && cur->src1 == addr) {
          // Address overwritten before being read
          IRInsn *to_remove = insn;
          insn = insn->prev ? insn->prev : fn->head;
          ir_remove_insn(fn, to_remove);
          changed = true;
          break;
        }
      }
      if (changed && !insn) break;
    }

    // 4. Redundant sign/zero extension and cast simplifications:
    if (insn->kind == IR_CAST && insn->dst && insn->src1) {
      if (ir_vregs_same_size_and_float(insn->dst, insn->src1)) {
        insn->kind = IR_MOV;
        changed = true;
      } else if (!insn->dst->is_float && !insn->src1->is_float) {
        int64_t imm_val = 0;
        if (ir_vreg_is_imm(insn->src1, &imm_val)) {
          int dsz = insn->dst->ty ? insn->dst->ty->size : 8;
          bool duns = insn->dst->ty ? insn->dst->ty->is_unsigned : false;
          int64_t folded = imm_val;
          if (dsz == 1) folded = duns ? (uint8_t)imm_val : (int8_t)imm_val;
          else if (dsz == 2) folded = duns ? (uint16_t)imm_val : (int16_t)imm_val;
          else if (dsz == 4) folded = duns ? (uint32_t)imm_val : (int32_t)imm_val;
          insn->kind = IR_IMM;
          insn->imm = folded;
          insn->src1 = NULL;
          changed = true;
          continue;
        }

        IRInsn *def1 = insn->src1->def_insn;
        if (def1 && def1->kind == IR_CAST && def1->src1 && def1->dst &&
            !def1->src1->is_float && !def1->dst->is_float) {
          int sz_orig = def1->src1->ty ? def1->src1->ty->size : 8;
          int sz_mid = def1->dst->ty ? def1->dst->ty->size : 8;
          int sz_final = insn->dst->ty ? insn->dst->ty->size : 8;
          if (sz_mid >= sz_orig && sz_mid >= sz_final) {
            insn->src1 = def1->src1;
            changed = true;
            continue;
          }
        }
      }
    }

    // 5. Unary simplifications: constant folding and double-involution
    if (insn->src1 && !insn->src2 && insn->dst && !insn->dst->is_float) {
      IRInsn *def1 = insn->src1->def_insn;
      if (def1 && ir_is_involution_pair(insn->kind, def1->kind) && def1->src1) {
        insn->kind = IR_MOV;
        insn->src1 = def1->src1;
        changed = true;
        continue;
      }
      int64_t imm1;
      if (ir_vreg_is_imm(insn->src1, &imm1)) {
        int64_t res = 0;
        if (ir_eval_unary_imm(insn->kind, imm1, &res)) {
          insn->kind = IR_IMM;
          insn->imm = res;
          insn->src1 = NULL;
          changed = true;
          continue;
        }
      }
    }

    // 6. Binary operations: canonicalization, constant folding, and generalized algebraic rules
    if (insn->src1 && insn->src2 && !insn->src1->is_float && !insn->src2->is_float) {
      int64_t c1 = 0, c2 = 0;
      bool s1_imm = ir_vreg_is_imm(insn->src1, &c1);
      bool s2_imm = ir_vreg_is_imm(insn->src2, &c2);
      bool is_uns = ir_insn_is_unsigned(insn);

      // 6a. Canonicalize commutative & relational operations to place immediate in src2
      if (s1_imm && !s2_imm) {
        IRKind comm_kind = ir_op_commute(insn->kind);
        if (comm_kind) {
          insn->kind = comm_kind;
          IRVReg *tmp = insn->src1;
          insn->src1 = insn->src2;
          insn->src2 = tmp;
          s1_imm = false;
          s2_imm = true;
          c2 = c1;
          changed = true;
        }
      }

      // 6b. Binary constant folding on LLIR
      if (s1_imm && s2_imm && insn->dst && !insn->dst->is_float) {
        int64_t res = 0;
        if (ir_eval_binary_imm(insn->kind, c1, c2, is_uns, &res)) {
          insn->kind = IR_IMM;
          insn->imm = res;
          insn->src1 = NULL;
          insn->src2 = NULL;
          changed = true;
          continue;
        }
      }

      // 6c. Generalized rules with constant operand in src2
      if (s2_imm && insn->dst && !insn->dst->is_float) {
        IRInsn *def1 = insn->src1 ? insn->src1->def_insn : NULL;
        int64_t absorb_val = 0;

        // Comparison with 0 / 1 boolean simplifications
        if (def1 && ir_is_comparison(def1->kind)) {
          if (insn->kind == IR_NE && c2 == 0) {
            insn->kind = def1->kind;
            insn->src1 = def1->src1;
            insn->src2 = def1->src2;
            changed = true;
            continue;
          } else if (insn->kind == IR_EQ && c2 == 0) {
            insn->kind = ir_invert_cmp(def1->kind);
            insn->src1 = def1->src1;
            insn->src2 = def1->src2;
            changed = true;
            continue;
          } else if (insn->kind == IR_EQ && c2 == 1) {
            insn->kind = def1->kind;
            insn->src1 = def1->src1;
            insn->src2 = def1->src2;
            changed = true;
            continue;
          } else if (insn->kind == IR_NE && c2 == 1) {
            insn->kind = ir_invert_cmp(def1->kind);
            insn->src1 = def1->src1;
            insn->src2 = def1->src2;
            changed = true;
            continue;
          }
        }

        // Identity rule: x op c -> x
        if (ir_is_right_identity(insn->kind, c2)) {
          insn->kind = IR_MOV;
          insn->src2 = NULL;
          changed = true;
          continue;
        }

        // Absorbing rule: x op c -> constant
        if (ir_is_absorbing_element(insn->kind, c2, is_uns, &absorb_val)) {
          insn->kind = IR_IMM;
          insn->imm = absorb_val;
          insn->src1 = NULL;
          insn->src2 = NULL;
          changed = true;
          continue;
        }

        // Power-of-two strength reduction
        int shift = get_power_of_two(c2);
        if (shift >= 0) {
          if (insn->kind == IR_MUL) {
            insn->kind = IR_SHL;
            insn->src2 = make_imm_vreg(fn, insn, shift, insn->src2->ty);
            changed = true;
            continue;
          } else if (insn->kind == IR_DIV && is_uns) {
            if (shift == 0) {
              insn->kind = IR_MOV;
              insn->src2 = NULL;
            } else {
              insn->kind = IR_SHR;
              insn->src2 = make_imm_vreg(fn, insn, shift, insn->src2->ty);
            }
            changed = true;
            continue;
          } else if (insn->kind == IR_MOD && is_uns) {
            insn->kind = IR_BITAND;
            insn->src2 = make_imm_vreg(fn, insn, c2 - 1, insn->src2->ty);
            changed = true;
            continue;
          }
        }

        // Associative constant reassociation: (x op c1) op c2 -> x op (c1 op c2)
        int64_t def_c1 = 0;
        if (def1 && def1->kind == insn->kind && def1->src2 && ir_vreg_is_imm(def1->src2, &def_c1)) {
          if (ir_is_associative_binop(insn->kind)) {
            // Fold LEA(var, c1) + c2 if ADD
            if (insn->kind == IR_ADD && def1->src1 && def1->src1->def_insn &&
                def1->src1->def_insn->kind == IR_ADDR) {
              IRInsn *addr_def = def1->src1->def_insn;
              insn->kind = IR_ADDR;
              insn->var = addr_def->var;
              insn->label = addr_def->label;
              insn->imm = addr_def->imm + def_c1 + c2;
              insn->src1 = NULL;
              insn->src2 = NULL;
              changed = true;
              continue;
            }
            int64_t combined = 0;
            if (ir_eval_binary_imm(insn->kind, def_c1, c2, is_uns, &combined)) {
              insn->src1 = def1->src1;
              insn->src2 = make_imm_vreg(fn, insn, combined, insn->src2->ty);
              changed = true;
              continue;
            }
          } else if (insn->kind == IR_SHL || insn->kind == IR_SHR) {
            if (def_c1 + c2 < 64) {
              insn->src1 = def1->src1;
              insn->src2 = make_imm_vreg(fn, insn, def_c1 + c2, insn->src2->ty);
              changed = true;
              continue;
            }
          }
        }

        // Address constant folding: LEA(var, c1) + c2 -> LEA(var, c1 + c2)
        if (insn->kind == IR_ADD && def1 && def1->kind == IR_ADDR) {
          insn->kind = IR_ADDR;
          insn->var = def1->var;
          insn->label = def1->label;
          insn->imm = def1->imm + c2;
          insn->src1 = NULL;
          insn->src2 = NULL;
          changed = true;
          continue;
        }

        // Add/Sub cross constant reassociation
        if (def1 && def1->src2 && ir_vreg_is_imm(def1->src2, &def_c1)) {
          if (insn->kind == IR_ADD && def1->kind == IR_SUB) {
            // (x - c1) + c2 = x + (c2 - c1)
            insn->src1 = def1->src1;
            insn->src2 = make_imm_vreg(fn, insn, c2 - def_c1, insn->src2->ty);
            changed = true;
            continue;
          } else if (insn->kind == IR_SUB && def1->kind == IR_ADD) {
            // (x + c1) - c2 = x + (def_c1 - c2)
            insn->kind = IR_ADD;
            insn->src1 = def1->src1;
            insn->src2 = make_imm_vreg(fn, insn, def_c1 - c2, insn->src2->ty);
            changed = true;
            continue;
          } else if (insn->kind == IR_SUB && def1->kind == IR_SUB) {
            // (x - c1) - c2 = x - (def_c1 + c2)
            insn->src1 = def1->src1;
            insn->src2 = make_imm_vreg(fn, insn, def_c1 + c2, insn->src2->ty);
            changed = true;
            continue;
          }
        }
      }

      // 6d. Operations on identical operands: x op x
      if (insn->src1 && insn->src2 && insn->src1 == insn->src2 && insn->dst && !insn->dst->is_float) {
        int64_t same_val = 0;
        if (ir_eval_same_operands(insn->kind, &same_val)) {
          insn->kind = IR_IMM;
          insn->imm = same_val;
          insn->src1 = NULL;
          insn->src2 = NULL;
          changed = true;
          continue;
        } else if (insn->kind == IR_ADD) {
          // x + x = x << 1
          insn->kind = IR_SHL;
          insn->src2 = make_imm_vreg(fn, insn, 1, insn->src2->ty);
          changed = true;
          continue;
        }
      }

      // 6e. Cancellation & inverse algebraic identities
      if (insn->src1 && insn->src2 && insn->dst && !insn->dst->is_float) {
        IRInsn *def1 = insn->src1->def_insn;
        IRInsn *def2 = insn->src2->def_insn;

        if (insn->kind == IR_ADD && def2 && def2->kind == IR_NEG && def2->src1) {
          // x + (-y) = x - y
          insn->kind = IR_SUB;
          insn->src2 = def2->src1;
          changed = true;
          continue;
        } else if (insn->kind == IR_SUB && def2 && def2->kind == IR_NEG && def2->src1) {
          // x - (-y) = x + y
          insn->kind = IR_ADD;
          insn->src2 = def2->src1;
          changed = true;
          continue;
        } else if ((insn->kind == IR_SUB && def1 && def1->kind == IR_ADD) ||
                   (insn->kind == IR_BITXOR && def1 && def1->kind == IR_BITXOR)) {
          // (x + y) - y = x, (x + y) - x = y
          // (x ^ y) ^ y = x, (x ^ y) ^ x = y
          if (def1->src2 == insn->src2) {
            insn->kind = IR_MOV;
            insn->src1 = def1->src1;
            insn->src2 = NULL;
            changed = true;
            continue;
          } else if (def1->src1 == insn->src2) {
            insn->kind = IR_MOV;
            insn->src1 = def1->src2;
            insn->src2 = NULL;
            changed = true;
            continue;
          }
        }
      }
    }
  }

  return changed;
}

// ============================================================================
// Pass Wrappers & Pass Singletons
// ============================================================================

static bool run_pass_copy_prop(IRFunction *fn, IRPassContext *ctx) {
  (void)ctx;
  return ir_opt_copy_prop(fn);
}

static bool run_pass_dce(IRFunction *fn, IRPassContext *ctx) {
  (void)ctx;
  return ir_opt_dce(fn);
}

static bool run_pass_cfg_simplify(IRFunction *fn, IRPassContext *ctx) {
  (void)ctx;
  return ir_opt_cfg_simplify(fn);
}

static bool run_pass_peephole(IRFunction *fn, IRPassContext *ctx) {
  (void)ctx;
  return ir_opt_peephole(fn);
}

static bool run_pass_verifier(IRFunction *fn, IRPassContext *ctx) {
  (void)ctx;
  char *err = NULL;
  return ir_verify_function(fn, &err);
}

IRPass pass_copy_prop = {
  .name = "copy-prop",
  .description = "Copy propagation",
  .type = IR_PASS_FUNCTION,
  .enabled = false,
  .default_opt_level = 1,
  .run_on_function = run_pass_copy_prop,
};

IRPass pass_dce = {
  .name = "dce",
  .description = "Dead code elimination",
  .type = IR_PASS_FUNCTION,
  .enabled = false,
  .default_opt_level = 1,
  .run_on_function = run_pass_dce,
};

IRPass pass_cfg_simplify = {
  .name = "cfg-simplify",
  .description = "Control flow simplification & unreachable code elimination",
  .type = IR_PASS_FUNCTION,
  .enabled = false,
  .default_opt_level = 1,
  .run_on_function = run_pass_cfg_simplify,
};

IRPass pass_peephole = {
  .name = "peephole",
  .description = "IR peephole optimizations and strength reduction",
  .type = IR_PASS_FUNCTION,
  .enabled = false,
  .default_opt_level = 2,
  .run_on_function = run_pass_peephole,
};

IRPass pass_verifier = {
  .name = "verifier",
  .description = "IR integrity verification pass",
  .type = IR_PASS_FUNCTION,
  .enabled = true,
  .default_opt_level = 0,
  .run_on_function = run_pass_verifier,
};

IRPass pass_hlir_const_fold = {
  .name = "hlir-const-fold",
  .description = "High-Level IR constant folding",
  .type = IR_PASS_FUNCTION,
  .enabled = false,
  .default_opt_level = 1,
};

IRPass pass_hlir_algebraic = {
  .name = "hlir-algebraic",
  .description = "High-Level IR algebraic simplification",
  .type = IR_PASS_FUNCTION,
  .enabled = false,
  .default_opt_level = 1,
};

IRPass pass_hlir_copy_prop = {
  .name = "hlir-copy-prop",
  .description = "High-Level IR copy propagation",
  .type = IR_PASS_FUNCTION,
  .enabled = false,
  .default_opt_level = 1,
};

IRPass pass_hlir_local_cse = {
  .name = "hlir-local-cse",
  .description = "High-Level IR local common subexpression elimination",
  .type = IR_PASS_FUNCTION,
  .enabled = false,
  .default_opt_level = 2,
};

IRPass pass_hlir_load_store = {
  .name = "hlir-load-store",
  .description = "High-Level IR redundant load and dead store elimination",
  .type = IR_PASS_FUNCTION,
  .enabled = false,
  .default_opt_level = 2,
};

IRPass pass_hlir_control_flow = {
  .name = "hlir-control-flow",
  .description = "High-Level IR control flow simplification",
  .type = IR_PASS_FUNCTION,
  .enabled = false,
  .default_opt_level = 1,
};

IRPass pass_hlir_dead_code = {
  .name = "hlir-dead-code",
  .description = "High-Level IR dead code elimination",
  .type = IR_PASS_FUNCTION,
  .enabled = false,
  .default_opt_level = 1,
};

IRPass pass_hlir_dce = {
  .name = "hlir-dce",
  .description = "High-Level IR dead value elimination",
  .type = IR_PASS_FUNCTION,
  .enabled = false,
  .default_opt_level = 1,
};

IRPass pass_hlir_inlining = {
  .name = "hlir-inlining",
  .description = "High-Level IR function inlining",
  .type = IR_PASS_PROG,
  .enabled = false,
  .default_opt_level = 2,
};

// ============================================================================
// Pass Registry & Fine-Tuning API
// ============================================================================

static IRPass *pass_registry[64];
static int pass_registry_count = 0;
static bool registry_initialized = false;

void ir_register_pass(IRPass *pass) {
  if (!pass || !pass->name)
    return;

  for (int i = 0; i < pass_registry_count; i++) {
    if (!strcmp(pass_registry[i]->name, pass->name)) {
      pass_registry[i] = pass;
      return;
    }
  }

  if (pass_registry_count < 64)
    pass_registry[pass_registry_count++] = pass;
}

IRPass *ir_find_pass(const char *name) {
  if (!name)
    return NULL;
  ir_init_pass_registry();

  for (int i = 0; i < pass_registry_count; i++) {
    if (!strcmp(pass_registry[i]->name, name))
      return pass_registry[i];
  }
  return NULL;
}

IRPass **ir_get_all_passes(int *count) {
  ir_init_pass_registry();
  if (count)
    *count = pass_registry_count;
  return pass_registry;
}

void ir_init_pass_registry(void) {
  if (registry_initialized)
    return;
  registry_initialized = true;

  ir_register_pass(&pass_copy_prop);
  ir_register_pass(&pass_peephole);
  ir_register_pass(&pass_cfg_simplify);
  ir_register_pass(&pass_dce);
  ir_register_pass(&pass_verifier);
  ir_register_pass(&pass_hlir_const_fold);
  ir_register_pass(&pass_hlir_algebraic);
  ir_register_pass(&pass_hlir_copy_prop);
  ir_register_pass(&pass_hlir_local_cse);
  ir_register_pass(&pass_hlir_load_store);
  ir_register_pass(&pass_hlir_control_flow);
  ir_register_pass(&pass_hlir_dead_code);
  ir_register_pass(&pass_hlir_dce);
  ir_register_pass(&pass_hlir_inlining);
}

void ir_opt_set_level(int opt_level) {
  ir_init_pass_registry();
  if (opt_level <= 0) {
    for (int i = 0; i < pass_registry_count; i++) {
      if (pass_registry[i] != &pass_verifier)
        pass_registry[i]->enabled = false;
    }

    // Common-sense optimizations enabled even at -O0:
    // Safe, preserve full debuggability and variable lifetime mapping,
    // while offering executable size reductions and runtime performance boosts.
    pass_hlir_const_fold.enabled = true;
    pass_hlir_algebraic.enabled = true;
    pass_hlir_copy_prop.enabled = true;
    pass_hlir_control_flow.enabled = true;
    pass_hlir_dead_code.enabled = true;
    pass_hlir_dce.enabled = true;
    pass_copy_prop.enabled = true;
    pass_cfg_simplify.enabled = true;
    pass_peephole.enabled = true;
    pass_dce.enabled = true;
    pass_verifier.enabled = true;
  } else if (opt_level == 1) {
    // HLIR tier: High-level type/value/control-flow optimizations
    pass_hlir_const_fold.enabled = true;
    pass_hlir_algebraic.enabled = true;
    pass_hlir_copy_prop.enabled = true;
    pass_hlir_local_cse.enabled = false;
    pass_hlir_load_store.enabled = false;
    pass_hlir_control_flow.enabled = true;
    pass_hlir_dead_code.enabled = true;
    pass_hlir_dce.enabled = true;
    pass_hlir_inlining.enabled = false;

    // LLIR tier: Machine-independent low-level transformations & cleanup
    pass_copy_prop.enabled = true;
    pass_peephole.enabled = true;
    pass_cfg_simplify.enabled = true;
    pass_dce.enabled = true;
    pass_verifier.enabled = true;
  } else if (opt_level >= 2) {
    // Enable HLIR and LLIR passes
    for (int i = 0; i < pass_registry_count; i++)
      pass_registry[i]->enabled = true;
  }
}

static bool str_case_hyphen_equal(const char *a, const char *b) {
  if (!a || !b) return false;
  while (*a && *b) {
    char ca = *a;
    char cb = *b;
    if (ca >= 'A' && ca <= 'Z') ca += 'a' - 'A';
    if (cb >= 'A' && cb <= 'Z') cb += 'a' - 'A';
    if (ca == '_') ca = '-';
    if (cb == '_') cb = '-';
    if (ca != cb) return false;
    a++;
    b++;
  }
  return *a == '\0' && *b == '\0';
}

bool ir_opt_set_pass_enabled(const char *name, bool enabled) {
  if (!name) return false;
  ir_init_pass_registry();

  if (str_case_hyphen_equal(name, "all")) {
    for (int i = 0; i < pass_registry_count; i++) {
      if (pass_registry[i] != &pass_verifier)
        pass_registry[i]->enabled = enabled;
    }
    return true;
  }

  if (str_case_hyphen_equal(name, "hlir") ||
      str_case_hyphen_equal(name, "hlir-opt") ||
      str_case_hyphen_equal(name, "hlir-all")) {
    pass_hlir_const_fold.enabled = enabled;
    pass_hlir_algebraic.enabled = enabled;
    pass_hlir_copy_prop.enabled = enabled;
    pass_hlir_local_cse.enabled = enabled;
    pass_hlir_load_store.enabled = enabled;
    pass_hlir_control_flow.enabled = enabled;
    pass_hlir_dead_code.enabled = enabled;
    pass_hlir_dce.enabled = enabled;
    pass_hlir_inlining.enabled = enabled;
    return true;
  }

  if (str_case_hyphen_equal(name, "const-fold") ||
      str_case_hyphen_equal(name, "constant-folding") ||
      str_case_hyphen_equal(name, "tree-ccp") ||
      str_case_hyphen_equal(name, "ccp") ||
      str_case_hyphen_equal(name, "hlir-const-fold")) {
    pass_hlir_const_fold.enabled = enabled;
    return true;
  }

  if (str_case_hyphen_equal(name, "copy-prop") ||
      str_case_hyphen_equal(name, "copy-propagation") ||
      str_case_hyphen_equal(name, "tree-copy-prop")) {
    pass_copy_prop.enabled = enabled;
    return true;
  }

  if (str_case_hyphen_equal(name, "dce") ||
      str_case_hyphen_equal(name, "dead-code-elimination") ||
      str_case_hyphen_equal(name, "tree-dce")) {
    pass_dce.enabled = enabled;
    return true;
  }

  if (str_case_hyphen_equal(name, "cfg-simplify") ||
      str_case_hyphen_equal(name, "simplify-cfg") ||
      str_case_hyphen_equal(name, "tree-dominator-opts")) {
    pass_cfg_simplify.enabled = enabled;
    return true;
  }

  if (str_case_hyphen_equal(name, "peephole") ||
      str_case_hyphen_equal(name, "peephole2") ||
      str_case_hyphen_equal(name, "strength-reduce") ||
      str_case_hyphen_equal(name, "strength-reduction")) {
    pass_peephole.enabled = enabled;
    return true;
  }

  if (str_case_hyphen_equal(name, "local-cse") ||
      str_case_hyphen_equal(name, "cse") ||
      str_case_hyphen_equal(name, "tree-cse") ||
      str_case_hyphen_equal(name, "hlir-local-cse") ||
      str_case_hyphen_equal(name, "hlir-cse")) {
    pass_hlir_local_cse.enabled = enabled;
    return true;
  }

  if (str_case_hyphen_equal(name, "verifier")) {
    pass_verifier.enabled = enabled;
    return true;
  }

  if (str_case_hyphen_equal(name, "hlir-algebraic")) {
    pass_hlir_algebraic.enabled = enabled;
    return true;
  }

  if (str_case_hyphen_equal(name, "hlir-copy-prop") ||
      str_case_hyphen_equal(name, "hlir-copy-propagation")) {
    pass_hlir_copy_prop.enabled = enabled;
    return true;
  }

  if (str_case_hyphen_equal(name, "hlir-load-store") ||
      str_case_hyphen_equal(name, "hlir-dse") ||
      str_case_hyphen_equal(name, "hlir-store-elim")) {
    pass_hlir_load_store.enabled = enabled;
    return true;
  }

  if (str_case_hyphen_equal(name, "hlir-control-flow") ||
      str_case_hyphen_equal(name, "hlir-cfg")) {
    pass_hlir_control_flow.enabled = enabled;
    return true;
  }

  if (str_case_hyphen_equal(name, "hlir-dead-code") ||
      str_case_hyphen_equal(name, "hlir-deadcode")) {
    pass_hlir_dead_code.enabled = enabled;
    return true;
  }

  if (str_case_hyphen_equal(name, "hlir-dce") ||
      str_case_hyphen_equal(name, "hlir-dead-val") ||
      str_case_hyphen_equal(name, "hlir-dead-value") ||
      str_case_hyphen_equal(name, "hlir-unused-val") ||
      str_case_hyphen_equal(name, "hlir-unused-value")) {
    pass_hlir_dce.enabled = enabled;
    return true;
  }

  if (str_case_hyphen_equal(name, "hlir-inlining") ||
      str_case_hyphen_equal(name, "hlir-inline") ||
      str_case_hyphen_equal(name, "inlining") ||
      str_case_hyphen_equal(name, "inline") ||
      str_case_hyphen_equal(name, "inline-functions")) {
    pass_hlir_inlining.enabled = enabled;
    return true;
  }

  // Check registry
  for (int i = 0; i < pass_registry_count; i++) {
    if (str_case_hyphen_equal(pass_registry[i]->name, name)) {
      pass_registry[i]->enabled = enabled;
      return true;
    }
  }

  return false;
}

bool ir_opt_is_pass_enabled(const char *name) {
  if (!name) return false;
  ir_init_pass_registry();

  if (str_case_hyphen_equal(name, "const-fold") || str_case_hyphen_equal(name, "tree-ccp") || str_case_hyphen_equal(name, "hlir-const-fold"))
    return pass_hlir_const_fold.enabled;
  if (str_case_hyphen_equal(name, "copy-prop") || str_case_hyphen_equal(name, "tree-copy-prop"))
    return pass_copy_prop.enabled;
  if (str_case_hyphen_equal(name, "dce") || str_case_hyphen_equal(name, "tree-dce"))
    return pass_dce.enabled;
  if (str_case_hyphen_equal(name, "cfg-simplify") || str_case_hyphen_equal(name, "simplify-cfg"))
    return pass_cfg_simplify.enabled;
  if (str_case_hyphen_equal(name, "peephole") || str_case_hyphen_equal(name, "strength-reduce"))
    return pass_peephole.enabled;
  if (str_case_hyphen_equal(name, "local-cse") || str_case_hyphen_equal(name, "cse") || str_case_hyphen_equal(name, "hlir-local-cse"))
    return pass_hlir_local_cse.enabled;
  if (str_case_hyphen_equal(name, "hlir-algebraic"))
    return pass_hlir_algebraic.enabled;
  if (str_case_hyphen_equal(name, "hlir-control-flow"))
    return pass_hlir_control_flow.enabled;
  if (str_case_hyphen_equal(name, "hlir-dead-code"))
    return pass_hlir_dead_code.enabled;
  if (str_case_hyphen_equal(name, "verifier"))
    return pass_verifier.enabled;

  for (int i = 0; i < pass_registry_count; i++) {
    if (str_case_hyphen_equal(pass_registry[i]->name, name))
      return pass_registry[i]->enabled;
  }
  return false;
}

void ir_opt_print_passes(FILE *out) {
  if (!out) out = stdout;
  ir_init_pass_registry();
  fprintf(out, "Registered Optimization Passes:\n");
  for (int i = 0; i < pass_registry_count; i++) {
    fprintf(out, "  %-20s [%s] (default >= -O%d): %s\n",
            pass_registry[i]->name,
            pass_registry[i]->enabled ? "ENABLED " : "DISABLED",
            pass_registry[i]->default_opt_level,
            pass_registry[i]->description ? pass_registry[i]->description : "");
  }
}

void ir_opt_init(void) {
  ir_init_pass_registry();
  ir_opt_set_level(opt_O);
}

// ============================================================================
// Pass Manager Implementation
// ============================================================================

IRPassManager *ir_pass_manager_new(void) {
  IRPassManager *pm = calloc(1, sizeof(IRPassManager));
  pm->capacity = 16;
  pm->passes = calloc(pm->capacity, sizeof(IRPass *));
  pm->max_fixed_point_iterations = 10;
  pm->fixed_point = true;
  return pm;
}

void ir_pass_manager_free(IRPassManager *pm) {
  if (!pm)
    return;
  free(pm->passes);
  free(pm);
}

void ir_pass_manager_add(IRPassManager *pm, IRPass *pass) {
  if (!pm || !pass)
    return;

  if (pm->num_passes >= pm->capacity) {
    pm->capacity *= 2;
    pm->passes = realloc(pm->passes, sizeof(IRPass *) * pm->capacity);
  }
  pm->passes[pm->num_passes++] = pass;
}

bool ir_pass_manager_add_by_name(IRPassManager *pm, const char *name) {
  IRPass *pass = ir_find_pass(name);
  if (!pass)
    return false;
  ir_pass_manager_add(pm, pass);
  return true;
}

bool ir_pass_manager_run_function(IRPassManager *pm, IRFunction *fn, IRPassContext *ctx) {
  if (!pm || !fn || pm->num_passes == 0)
    return false;

  bool fn_changed = false;
  const int max_iter = pm->fixed_point ? pm->max_fixed_point_iterations : 1;

  for (int iter = 0; iter < max_iter; iter++) {
    bool iter_changed = false;

    for (int p = 0; p < pm->num_passes; p++) {
      const IRPass *pass = pm->passes[p];
      if (pass->type == IR_PASS_FUNCTION && pass->run_on_function) {
        bool pass_changed = pass->run_on_function(fn, ctx);
        if (pass_changed) {
          iter_changed = true;
          fn_changed = true;
          ir_invalidate_analyses(ctx);
          if (ctx)
            ctx->changes_count++;
        }
      }
    }

    if (ctx)
      ctx->iterations_run++;

    if (!iter_changed)
      break; // Fixed point reached
  }

  ir_renumber_insns(fn);
  return fn_changed;
}

bool ir_pass_manager_run_prog(IRPassManager *pm, IRProg *prog, IRPassContext *ctx) {
  if (!pm || !prog)
    return false;

  bool prog_changed = false;

  // Run program-level passes
  for (int p = 0; p < pm->num_passes; p++) {
    const IRPass *pass = pm->passes[p];
    if (pass->type == IR_PASS_PROG && pass->run_on_prog) {
      if (pass->run_on_prog(prog, ctx))
        prog_changed = true;
    }
  }

  // Run function-level passes
  for (int i = 0; i < prog->num_fns; i++) {
    IRFunction *fn = prog->fns[i];
    if (ir_pass_manager_run_function(pm, fn, ctx))
      prog_changed = true;
  }

  return prog_changed;
}

// Pipeline Construction
IRPassManager *ir_create_opt_pipeline(int opt_level) {
  ir_init_pass_registry();
  IRPassManager *pm = ir_pass_manager_new();

  pm->fixed_point = true;
  pm->max_fixed_point_iterations = (opt_level <= 0) ? 4 : opt_max_passes;

  if (pass_copy_prop.enabled)
    ir_pass_manager_add(pm, &pass_copy_prop);
  if (pass_peephole.enabled)
    ir_pass_manager_add(pm, &pass_peephole);
  if (pass_cfg_simplify.enabled)
    ir_pass_manager_add(pm, &pass_cfg_simplify);
  if (pass_dce.enabled)
    ir_pass_manager_add(pm, &pass_dce);
  if (pass_verifier.enabled)
    ir_pass_manager_add(pm, &pass_verifier);

  return pm;
}

void ir_optimize_level(IRProg *prog, int opt_level) {
  if (!prog)
    return;

  IRPassManager *pm = ir_create_opt_pipeline(opt_level);
  if (pm->num_passes == 0) {
    ir_pass_manager_free(pm);
    return;
  }

  IRPassContext ctx = {
    .opt_level = opt_level,
    .verify_each = pass_verifier.enabled,
  };

  ir_pass_manager_run_prog(pm, prog, &ctx);

  ir_invalidate_analyses(&ctx);
  ir_pass_manager_free(pm);
}

void ir_optimize(IRProg *prog) {
  ir_optimize_level(prog, 2);
}

void llir_optimize(LLIRProg *prog, int opt_level) {
  if (!prog)
    return;
  ir_optimize_level((IRProg *)prog, opt_level);
}
