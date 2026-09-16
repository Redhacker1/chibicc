#include "ir/opt.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// Analysis Infrastructure: CFG, Dominator Tree, Liveness, and Verification
// ============================================================================

// Allocate a new basic block with an initial capacity for predecessor/successor edges.
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

// Adds a directed control-flow edge from `from` basic block to `to` basic block.
static void bb_add_succ(IRBasicBlock *from, IRBasicBlock *to) {
  if (!from || !to)
    return;

  // Avoid duplicate edges between the same pair of blocks
  for (int i = 0; i < from->num_succs; i++) {
    if (from->succs[i] == to)
      return;
  }

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

// ----------------------------------------------------------------------------
// Control Flow Graph (CFG) Construction
// ----------------------------------------------------------------------------
// Identifies basic block leaders (start of function, labels, instructions
// following terminators) and resolves branches/jumps to connect the graph.
IRCFG *ir_build_cfg(IRFunction *fn) {
  if (!fn || !fn->head)
    return NULL;

  IRCFG *cfg = calloc(1, sizeof(IRCFG));
  cfg->fn = fn;
  cfg->block_cap = 16;
  cfg->blocks = calloc(cfg->block_cap, sizeof(IRBasicBlock *));

  // Step 1: Identify leaders and create basic blocks
  // A leader is:
  //   1) The function entry instruction
  //   2) Any IR_LABEL
  //   3) The instruction immediately following a branch/jump/return
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

  // Step 2: Connect CFG edges based on terminators
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
        // Fallthrough branch
        bb_add_succ(bb, cfg->blocks[i + 1]);
      }
    } else if (term->kind == IR_RET) {
      // Exit point: no successor
    } else {
      // Normal fallthrough to next sequential block
      if (i + 1 < cfg->num_blocks)
        bb_add_succ(bb, cfg->blocks[i + 1]);
    }
  }

  // Step 3: Compute block reachability starting from the entry block
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

// Deallocates a CFG structure and its internal basic block list.
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

// Pretty prints the CFG connectivity for debugging and diagnostic purposes.
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

// Lazy cache retrieval for CFG in an IRPassContext.
IRCFG *ir_get_or_build_cfg(IRFunction *fn, IRPassContext *ctx) {
  if (ctx && ctx->cfg)
    return ctx->cfg;
  IRCFG *cfg = ir_build_cfg(fn);
  if (ctx)
    ctx->cfg = cfg;
  return cfg;
}

// ----------------------------------------------------------------------------
// Dominator Tree Construction (Cooper-Harvey-Kennedy iterative algorithm)
// ----------------------------------------------------------------------------
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

  // Compute dominator tree depth for fast dominance relationship checks
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

// Queries if basic block `a` dominates basic block `b`.
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

// ----------------------------------------------------------------------------
// Liveness Analysis
// ----------------------------------------------------------------------------
// Computes live ranges, first definition positions, last usage positions,
// and detects variables live across function calls.
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

    IRVReg *srcs[] = {insn->src1, insn->src2, insn->src3, insn->base_reg, insn->index_reg};
    for (int s = 0; s < 5; s++) {
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

// Invalidate cached analyses when an optimization mutates the IR.
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

// ----------------------------------------------------------------------------
// IR Verifier
// ----------------------------------------------------------------------------
// Validates structural integrity of doubly-linked instruction list,
// variable ID bounds, and basic sanity invariants.
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
    if (insn->base_reg && (insn->base_reg->id < 0 || insn->base_reg->id >= fn->num_vregs)) {
      if (err_out) *err_out = "Instruction base_reg vreg id out of range";
      return false;
    }
    if (insn->index_reg && (insn->index_reg->id < 0 || insn->index_reg->id >= fn->num_vregs)) {
      if (err_out) *err_out = "Instruction index_reg vreg id out of range";
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
