#include "ir/opt.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Analysis Infrastructure: CFG, Dominator Tree, Liveness
// ============================================================================

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
      IRBasicBlock *b = queue[qhead++];
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
    IRBasicBlock *bb = cfg->blocks[i];
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
      IRBasicBlock *b = cfg->blocks[i];
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
    IRBasicBlock *curr = b;
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

  IRBasicBlock *curr = b;
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
  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind == IR_CALL)
      last_call_pos = insn->pos;

    if (insn->dst && insn->dst->id >= 0 && insn->dst->id < fn->num_vregs) {
      if (liv->def_pos[insn->dst->id] == -1)
        liv->def_pos[insn->dst->id] = insn->pos;
      liv->last_use_pos[insn->dst->id] = insn->pos;
    }

    IRVReg *srcs[] = {insn->src1, insn->src2, insn->src3};
    for (int s = 0; s < 3; s++) {
      IRVReg *v = srcs[s];
      if (v && v->id >= 0 && v->id < fn->num_vregs) {
        if (liv->def_pos[v->id] == -1)
          liv->def_pos[v->id] = insn->pos;
        liv->last_use_pos[v->id] = insn->pos;
        if (last_call_pos > liv->def_pos[v->id])
          liv->is_live_across_calls[v->id] = true;
      }
    }

    for (int a = 0; a < insn->num_args; a++) {
      IRVReg *v = insn->args[a];
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

  IRInsn *prev = NULL;
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

// 1. Constant Folding & Algebraic Simplification Pass
bool ir_opt_const_fold(IRFunction *fn) {
  if (!fn || fn->num_vregs == 0)
    return false;

  bool changed = false;
  int64_t *const_vals = calloc(fn->num_vregs, sizeof(int64_t));
  bool *is_const = calloc(fn->num_vregs, sizeof(bool));

  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind == IR_LABEL || insn->kind == IR_BR || insn->kind == IR_JMP || insn->kind == IR_CALL) {
      memset(is_const, 0, fn->num_vregs * sizeof(bool));
      continue;
    }

    if (insn->kind == IR_IMM && insn->dst) {
      is_const[insn->dst->id] = true;
      const_vals[insn->dst->id] = insn->imm;
      continue;
    }

    // Both operands constant: compile-time evaluation
    if (insn->src1 && is_const[insn->src1->id] && insn->src2 && is_const[insn->src2->id]) {
      int64_t c1 = const_vals[insn->src1->id];
      int64_t c2 = const_vals[insn->src2->id];
      int64_t res = 0;
      bool folded = true;

      switch (insn->kind) {
      case IR_ADD: res = c1 + c2; break;
      case IR_SUB: res = c1 - c2; break;
      case IR_MUL: res = c1 * c2; break;
      case IR_DIV: if (c2 != 0) res = c1 / c2; else folded = false; break;
      case IR_MOD: if (c2 != 0) res = c1 % c2; else folded = false; break;
      case IR_BITAND: res = c1 & c2; break;
      case IR_BITOR:  res = c1 | c2; break;
      case IR_BITXOR: res = c1 ^ c2; break;
      case IR_SHL: if (c2 >= 0 && c2 < 64) res = c1 << c2; else folded = false; break;
      case IR_SHR: if (c2 >= 0 && c2 < 64) res = c1 >> c2; else folded = false; break;
      case IR_EQ: res = (c1 == c2); break;
      case IR_NE: res = (c1 != c2); break;
      case IR_LT: res = (c1 < c2); break;
      case IR_LE: res = (c1 <= c2); break;
      case IR_GT: res = (c1 > c2); break;
      case IR_GE: res = (c1 >= c2); break;
      default: folded = false; break;
      }

      if (folded && insn->dst) {
        insn->kind = IR_IMM;
        insn->imm = res;
        insn->src1 = NULL;
        insn->src2 = NULL;
        is_const[insn->dst->id] = true;
        const_vals[insn->dst->id] = res;
        changed = true;
        continue;
      }
    }

    // Unary constant folding
    if (insn->src1 && is_const[insn->src1->id] && !insn->src2 && insn->dst) {
      int64_t c = const_vals[insn->src1->id];
      int64_t res = 0;
      bool folded = true;

      switch (insn->kind) {
      case IR_NEG: res = -c; break;
      case IR_BITNOT: res = ~c; break;
      case IR_LOGNOT: res = !c; break;
      case IR_CAST: {
        if (insn->dst->is_float) {
          folded = false;
        } else {
          int sz = insn->dst->ty ? insn->dst->ty->size : 8;
          bool is_unsigned = insn->dst->ty ? insn->dst->ty->is_unsigned : false;
          res = c;
          if (sz == 1) res = is_unsigned ? (uint8_t)c : (int8_t)c;
          else if (sz == 2) res = is_unsigned ? (uint16_t)c : (int16_t)c;
          else if (sz == 4) res = is_unsigned ? (uint32_t)c : (int32_t)c;
        }
        break;
      }
      default: folded = false; break;
      }

      if (folded) {
        insn->kind = IR_IMM;
        insn->imm = res;
        insn->src1 = NULL;
        is_const[insn->dst->id] = true;
        const_vals[insn->dst->id] = res;
        changed = true;
        continue;
      }
    }

    // Algebraic identities
    // x + 0 = x, 0 + x = x
    if (insn->kind == IR_ADD && insn->dst && insn->src1 && insn->src2) {
      if (is_const[insn->src2->id] && const_vals[insn->src2->id] == 0) {
        insn->kind = IR_MOV;
        insn->src2 = NULL;
        changed = true;
      } else if (is_const[insn->src1->id] && const_vals[insn->src1->id] == 0) {
        insn->kind = IR_MOV;
        insn->src1 = insn->src2;
        insn->src2 = NULL;
        changed = true;
      }
    }
    // x - 0 = x, x - x = 0
    else if (insn->kind == IR_SUB && insn->dst && insn->src1 && insn->src2) {
      if (is_const[insn->src2->id] && const_vals[insn->src2->id] == 0) {
        insn->kind = IR_MOV;
        insn->src2 = NULL;
        changed = true;
      } else if (insn->src1 == insn->src2) { // x - x = 0
        insn->kind = IR_IMM;
        insn->imm = 0;
        insn->src1 = insn->src2 = NULL;
        is_const[insn->dst->id] = true;
        const_vals[insn->dst->id] = 0;
        changed = true;
      }
    }
    // x * 1 = x, 1 * x = x, x * 0 = 0, 0 * x = 0
    else if (insn->kind == IR_MUL && insn->dst && insn->src1 && insn->src2) {
      if (is_const[insn->src2->id] && const_vals[insn->src2->id] == 1) {
        insn->kind = IR_MOV;
        insn->src2 = NULL;
        changed = true;
      } else if (is_const[insn->src1->id] && const_vals[insn->src1->id] == 1) {
        insn->kind = IR_MOV;
        insn->src1 = insn->src2;
        insn->src2 = NULL;
        changed = true;
      } else if ((is_const[insn->src2->id] && const_vals[insn->src2->id] == 0) ||
                 (is_const[insn->src1->id] && const_vals[insn->src1->id] == 0)) {
        insn->kind = IR_IMM;
        insn->imm = 0;
        insn->src1 = insn->src2 = NULL;
        is_const[insn->dst->id] = true;
        const_vals[insn->dst->id] = 0;
        changed = true;
      }
    }
    // x / 1 = x, x / x = 1
    else if (insn->kind == IR_DIV && insn->dst && insn->src1 && insn->src2) {
      if (is_const[insn->src2->id] && const_vals[insn->src2->id] == 1) {
        insn->kind = IR_MOV;
        insn->src2 = NULL;
        changed = true;
      } else if (insn->src1 == insn->src2) {
        insn->kind = IR_IMM;
        insn->imm = 1;
        insn->src1 = insn->src2 = NULL;
        is_const[insn->dst->id] = true;
        const_vals[insn->dst->id] = 1;
        changed = true;
      }
    }
    // x ^ x = 0, x & 0 = 0, x & x = x, x | 0 = x, x | x = x, x ^ 0 = x
    else if (insn->kind == IR_BITXOR && insn->dst && insn->src1 && insn->src2) {
      if (insn->src1 == insn->src2) {
        insn->kind = IR_IMM;
        insn->imm = 0;
        insn->src1 = insn->src2 = NULL;
        is_const[insn->dst->id] = true;
        const_vals[insn->dst->id] = 0;
        changed = true;
      } else if (is_const[insn->src2->id] && const_vals[insn->src2->id] == 0) {
        insn->kind = IR_MOV;
        insn->src2 = NULL;
        changed = true;
      } else if (is_const[insn->src1->id] && const_vals[insn->src1->id] == 0) {
        insn->kind = IR_MOV;
        insn->src1 = insn->src2;
        insn->src2 = NULL;
        changed = true;
      }
    } else if (insn->kind == IR_BITAND && insn->dst && insn->src1 && insn->src2) {
      if (insn->src1 == insn->src2) {
        insn->kind = IR_MOV;
        insn->src2 = NULL;
        changed = true;
      } else if (is_const[insn->src2->id] && const_vals[insn->src2->id] == 0) {
        insn->kind = IR_IMM;
        insn->imm = 0;
        insn->src1 = insn->src2 = NULL;
        is_const[insn->dst->id] = true;
        const_vals[insn->dst->id] = 0;
        changed = true;
      } else if (is_const[insn->src1->id] && const_vals[insn->src1->id] == 0) {
        insn->kind = IR_IMM;
        insn->imm = 0;
        insn->src1 = insn->src2 = NULL;
        is_const[insn->dst->id] = true;
        const_vals[insn->dst->id] = 0;
        changed = true;
      }
    } else if (insn->kind == IR_BITOR && insn->dst && insn->src1 && insn->src2) {
      if (insn->src1 == insn->src2) {
        insn->kind = IR_MOV;
        insn->src2 = NULL;
        changed = true;
      } else if (is_const[insn->src2->id] && const_vals[insn->src2->id] == 0) {
        insn->kind = IR_MOV;
        insn->src2 = NULL;
        changed = true;
      } else if (is_const[insn->src1->id] && const_vals[insn->src1->id] == 0) {
        insn->kind = IR_MOV;
        insn->src1 = insn->src2;
        insn->src2 = NULL;
        changed = true;
      }
    } else if ((insn->kind == IR_SHL || insn->kind == IR_SHR) && insn->dst && insn->src1 && insn->src2) {
      if (is_const[insn->src2->id] && const_vals[insn->src2->id] == 0) {
        insn->kind = IR_MOV;
        insn->src2 = NULL;
        changed = true;
      }
    } else if ((insn->kind == IR_EQ || insn->kind == IR_LE || insn->kind == IR_GE) &&
               insn->dst && insn->src1 && insn->src2 && insn->src1 == insn->src2 &&
               !insn->src1->is_float) {
      insn->kind = IR_IMM;
      insn->imm = 1;
      insn->src1 = insn->src2 = NULL;
      is_const[insn->dst->id] = true;
      const_vals[insn->dst->id] = 1;
      changed = true;
    } else if ((insn->kind == IR_NE || insn->kind == IR_LT || insn->kind == IR_GT) &&
               insn->dst && insn->src1 && insn->src2 && insn->src1 == insn->src2 &&
               !insn->src1->is_float) {
      insn->kind = IR_IMM;
      insn->imm = 0;
      insn->src1 = insn->src2 = NULL;
      is_const[insn->dst->id] = true;
      const_vals[insn->dst->id] = 0;
      changed = true;
    }
  }

  free(const_vals);
  free(is_const);
  return changed;
}

// 2. Copy Propagation Pass
bool ir_opt_copy_prop(IRFunction *fn) {
  if (!fn || fn->num_vregs == 0)
    return false;

  bool changed = false;
  IRVReg **aliases = calloc(fn->num_vregs, sizeof(IRVReg *));

  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind == IR_LABEL || insn->kind == IR_BR || insn->kind == IR_JMP) {
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
      if (insn->dst->ty && insn->src1->ty && insn->dst->ty->size == insn->src1->ty->size) {
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
    for (IRInsn *insn = fn->head; insn; insn = insn->next) {
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

  // Pass 1: Eliminate unreachable instructions following unconditional jumps/returns
  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind == IR_JMP || insn->kind == IR_RET) {
      while (insn->next && insn->next->kind != IR_LABEL) {
        ir_remove_insn(fn, insn->next);
        changed = true;
      }
    }
  }

  // Pass 2: Eliminate redundant unconditional jumps immediately preceding their target label
  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind == IR_JMP && insn->label && insn->next && insn->next->kind == IR_LABEL) {
      if (insn->next->label && !strcmp(insn->label, insn->next->label)) {
        IRInsn *to_remove = insn;
        insn = insn->prev;
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

  return changed;
}

// 5. Peephole Optimization & Strength Reduction Pass
bool ir_opt_peephole(IRFunction *fn) {
  if (!fn || fn->num_vregs == 0)
    return false;

  bool changed = false;

  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    // 1. Redundant self-move: dst = mov dst
    if (insn->kind == IR_MOV && insn->dst && insn->src1 && insn->dst == insn->src1) {
      IRInsn *next = insn->next;
      ir_remove_insn(fn, insn);
      insn = next ? next->prev : fn->tail;
      changed = true;
      if (!insn) break;
      continue;
    }

    // 2. Dead jump to immediately following label: jmp L; L:
    if (insn->kind == IR_JMP && insn->label) {
      IRInsn *next = insn->next;
      while (next && next->kind == IR_NOP)
        next = next->next;
      if (next && next->kind == IR_LABEL && next->label && !strcmp(insn->label, next->label)) {
        IRInsn *del = insn;
        insn = insn->prev ? insn->prev : fn->head;
        ir_remove_insn(fn, del);
        changed = true;
        if (!insn) break;
        continue;
      }
    }

    // 3. Redundant self-cancellation: sub x, x -> 0, xor x, x -> 0
    if ((insn->kind == IR_SUB || insn->kind == IR_BITXOR) && insn->dst && insn->src1 && insn->src2 &&
        insn->src1 == insn->src2 && !insn->dst->is_float) {
      insn->kind = IR_IMM;
      insn->imm = 0;
      insn->src1 = NULL;
      insn->src2 = NULL;
      changed = true;
      continue;
    }

    // 4. Strength reduction: multiply by power of 2 -> shift left
    if (insn->kind == IR_MUL && insn->dst && insn->src1 && insn->src2 && !insn->dst->is_float) {
      // Check if src2 was defined by IR_IMM
      for (IRInsn *def = fn->head; def != insn; def = def->next) {
        if (def->kind == IR_IMM && def->dst == insn->src2) {
          int64_t val = def->imm;
          if (val > 0 && (val & (val - 1)) == 0) {
            // Compute log2(val)
            int shift = 0;
            while ((1LL << shift) < val)
              shift++;
            if ((1LL << shift) == val) {
              // Replace src2 with a shift immediate
              IRVReg *shift_vreg = ir_new_vreg(fn, def->dst->ty);
              IRInsn *imm_insn = ir_new_insn(IR_IMM);
              imm_insn->dst = shift_vreg;
              imm_insn->imm = shift;
              ir_insert_before(fn, insn, imm_insn);

              insn->kind = IR_SHL;
              insn->src2 = shift_vreg;
              changed = true;
            }
          }
          break;
        }
      }
    }

    // 5. Redundant load-after-store elimination:
    // STORE addr, val  followed closely by  dst = LOAD addr
    if (insn->kind == IR_STORE && insn->src1 && insn->src2) {
      IRVReg *addr = insn->src1;
      IRVReg *val = insn->src2;

      for (IRInsn *cur = insn->next; cur; cur = cur->next) {
        if (cur->kind == IR_CALL || cur->kind == IR_LABEL || cur->kind == IR_JMP || cur->kind == IR_BR)
          break; // Barrier
        if (cur->kind == IR_STORE || cur->kind == IR_MEMCPY || cur->kind == IR_MEMZERO || cur->kind == IR_CAS || cur->kind == IR_EXCH)
          break; // Any store or memory clobber breaks alias safety

        if (cur->kind == IR_LOAD && cur->src1 == addr && cur->dst && val &&
            cur->dst->ty && val->ty && cur->dst->ty->size == val->ty->size &&
            cur->dst->is_float == val->is_float) {
          cur->kind = IR_MOV;
          cur->src1 = val;
          changed = true;
          break;
        }
      }
    }

    // 6. Dead store elimination:
    // STORE addr, val1 followed by STORE addr, val2 with no reads or barriers in between
    if (insn->kind == IR_STORE && insn->src1 && insn->src2) {
      IRVReg *addr = insn->src1;
      for (IRInsn *cur = insn->next; cur; cur = cur->next) {
        if (cur->kind == IR_CALL || cur->kind == IR_LABEL || cur->kind == IR_JMP || cur->kind == IR_BR || cur->kind == IR_RET)
          break; // Barrier
        if (cur->kind == IR_LOAD || cur->kind == IR_MEMCPY || cur->kind == IR_MEMZERO || cur->kind == IR_CAS || cur->kind == IR_EXCH)
          break; // Reads memory or complex clobber
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

    // 7. Redundant sign/zero extension:
    // If src was an IMM that fits in the target size or already sign-extended
    if (insn->kind == IR_CAST && insn->dst && insn->src1) {
      if (insn->dst->ty && insn->src1->ty &&
          insn->dst->ty->size == insn->src1->ty->size &&
          insn->dst->is_float == insn->src1->is_float) {
        insn->kind = IR_MOV;
        changed = true;
      }
    }
  }

  return changed;
}

// 6. Local Common Subexpression Elimination (within straight-line code)
bool ir_opt_local_cse(IRFunction *fn) {
  if (!fn || !fn->head)
    return false;

  bool changed = false;

  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    if (ir_insn_has_side_effects(insn) || !insn->dst || insn->kind == IR_NOP)
      continue;

    // Check subsequent instructions in same basic block (stop at labels, branches, calls, stores)
    for (IRInsn *sub = insn->next; sub; sub = sub->next) {
      if (sub->kind == IR_LABEL || sub->kind == IR_JMP || sub->kind == IR_BR || sub->kind == IR_RET || sub->kind == IR_CALL)
        break;

      if (insn->kind == IR_LOAD && (sub->kind == IR_STORE || sub->kind == IR_MEMCPY || sub->kind == IR_MEMZERO))
        break; // Memory dependency

      if (sub->kind == insn->kind && sub->dst) {
        if (sub->var != insn->var)
          continue;
        if ((sub->label || insn->label) && (!sub->label || !insn->label || strcmp(sub->label, insn->label) != 0))
          continue;

        bool match = false;
        if (sub->src1 == insn->src1 && sub->src2 == insn->src2 && sub->src3 == insn->src3 && sub->imm == insn->imm && sub->fimm == insn->fimm) {
          match = true;
        } else if (ir_insn_is_commutative(insn->kind) && sub->src1 == insn->src2 && sub->src2 == insn->src1) {
          match = true;
        }

        if (match) {
          sub->kind = IR_MOV;
          sub->src1 = insn->dst;
          sub->src2 = NULL;
          sub->src3 = NULL;
          changed = true;
        }
      }
    }
  }

  return changed;
}

// ============================================================================
// Pass Wrappers & Pass Singletons
// ============================================================================

static bool run_pass_const_fold(IRFunction *fn, IRPassContext *ctx) {
  (void)ctx;
  return ir_opt_const_fold(fn);
}

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

static bool run_pass_local_cse(IRFunction *fn, IRPassContext *ctx) {
  (void)ctx;
  return ir_opt_local_cse(fn);
}

static bool run_pass_verifier(IRFunction *fn, IRPassContext *ctx) {
  (void)ctx;
  char *err = NULL;
  return ir_verify_function(fn, &err);
}

IRPass pass_const_fold = {
  .name = "const-fold",
  .description = "Constant folding and algebraic simplification",
  .type = IR_PASS_FUNCTION,
  .run_on_function = run_pass_const_fold,
};

IRPass pass_copy_prop = {
  .name = "copy-prop",
  .description = "Copy propagation",
  .type = IR_PASS_FUNCTION,
  .run_on_function = run_pass_copy_prop,
};

IRPass pass_dce = {
  .name = "dce",
  .description = "Dead code elimination",
  .type = IR_PASS_FUNCTION,
  .run_on_function = run_pass_dce,
};

IRPass pass_cfg_simplify = {
  .name = "cfg-simplify",
  .description = "Control flow simplification & unreachable code elimination",
  .type = IR_PASS_FUNCTION,
  .run_on_function = run_pass_cfg_simplify,
};

IRPass pass_peephole = {
  .name = "peephole",
  .description = "IR peephole optimizations and strength reduction",
  .type = IR_PASS_FUNCTION,
  .run_on_function = run_pass_peephole,
};

IRPass pass_local_cse = {
  .name = "local-cse",
  .description = "Local common subexpression elimination",
  .type = IR_PASS_FUNCTION,
  .run_on_function = run_pass_local_cse,
};

IRPass pass_verifier = {
  .name = "verifier",
  .description = "IR integrity verification pass",
  .type = IR_PASS_FUNCTION,
  .run_on_function = run_pass_verifier,
};

// ============================================================================
// Pass Registry
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

  ir_register_pass(&pass_const_fold);
  ir_register_pass(&pass_copy_prop);
  ir_register_pass(&pass_dce);
  ir_register_pass(&pass_cfg_simplify);
  ir_register_pass(&pass_peephole);
  ir_register_pass(&pass_local_cse);
  ir_register_pass(&pass_verifier);
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
  int max_iter = pm->fixed_point ? pm->max_fixed_point_iterations : 1;

  for (int iter = 0; iter < max_iter; iter++) {
    bool iter_changed = false;

    for (int p = 0; p < pm->num_passes; p++) {
      IRPass *pass = pm->passes[p];
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
    IRPass *pass = pm->passes[p];
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

  if (opt_level <= 0) {
    // -O0: Basic cleanup only
    pm->fixed_point = false;
    ir_pass_manager_add(pm, &pass_verifier);
    return pm;
  }

  // -O1, -O2, -O3, -Os
  pm->fixed_point = true;
  pm->max_fixed_point_iterations = (opt_level >= 2) ? 8 : 4;

  ir_pass_manager_add(pm, &pass_const_fold);
  ir_pass_manager_add(pm, &pass_copy_prop);
  ir_pass_manager_add(pm, &pass_local_cse);
  ir_pass_manager_add(pm, &pass_peephole);
  ir_pass_manager_add(pm, &pass_cfg_simplify);
  ir_pass_manager_add(pm, &pass_dce);
  ir_pass_manager_add(pm, &pass_verifier);

  return pm;
}

void ir_optimize_level(IRProg *prog, int opt_level) {
  if (!prog)
    return;

  IRPassManager *pm = ir_create_opt_pipeline(opt_level);
  IRPassContext ctx = {
    .opt_level = opt_level,
    .verify_each = true,
  };

  ir_pass_manager_run_prog(pm, prog, &ctx);

  ir_invalidate_analyses(&ctx);
  ir_pass_manager_free(pm);
}

void ir_optimize(IRProg *prog) {
  ir_optimize_level(prog, 2);
}
