#include "ir/hlir_opt.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// HLIR Local Memory Optimization Pass (Store-Load Forwarding & Dead Stores)
// ============================================================================
// Tracks local variable values across stores and loads within basic blocks:
// 1. Forwards stored values to subsequent loads of the same variable (Store-to-Load)
// 2. Forwards previously loaded values to subsequent loads (Load-to-Load)
// 3. Eliminates overwritten stores that are never read before overwrite (Dead Store Elimination)

typedef struct {
  Obj2 *var;
  HLIRVal *avail_val;
  HLIRInsn *last_store;
} HLIRLocalVarEntry;

typedef struct {
  HLIRLocalVarEntry *entries;
  int count;
  int cap;
} HLIRLocalMemEnv;

static void hlir_local_mem_reset(HLIRLocalMemEnv *env) {
  env->count = 0;
}

static HLIRLocalVarEntry *hlir_local_mem_find(HLIRLocalMemEnv *env, Obj2 *var) {
  for (int i = 0; i < env->count; i++) {
    if (env->entries[i].var == var)
      return &env->entries[i];
  }
  return NULL;
}

static HLIRLocalVarEntry *hlir_local_mem_get_or_add(HLIRLocalMemEnv *env, Obj2 *var) {
  HLIRLocalVarEntry *e = hlir_local_mem_find(env, var);
  if (e) return e;
  if (env->count >= env->cap) {
    env->cap = env->cap ? env->cap * 2 : 16;
    env->entries = realloc(env->entries, sizeof(HLIRLocalVarEntry) * env->cap);
  }
  e = &env->entries[env->count++];
  e->var = var;
  e->avail_val = NULL;
  e->last_store = NULL;
  return e;
}

static void hlir_local_mem_remove(HLIRLocalMemEnv *env, Obj2 *var) {
  for (int i = 0; i < env->count; i++) {
    if (env->entries[i].var == var) {
      env->entries[i] = env->entries[--env->count];
      return;
    }
  }
}

bool hlir_opt_load_store(HLIRFunction *fn) {
  if (!fn || !fn->head)
    return false;

  bool changed = false;
  HLIRLocalMemEnv env = {0};

  for (HLIRInsn *insn = fn->head; insn; insn = insn->next) {
    if (hlir_is_bb_barrier(insn->kind) || insn->kind == HLIR_STORE_PTR ||
        insn->kind == HLIR_MEMCPY || insn->kind == HLIR_MEMZERO) {
      hlir_local_mem_reset(&env);
      continue;
    }

    if (insn->kind == HLIR_ADDR_VAR && insn->var) {
      hlir_local_mem_remove(&env, insn->var);
      continue;
    }

    if (insn->kind == HLIR_STORE_VAR && insn->var && insn->src1) {
      Obj2 *var = insn->var;
      HLIRLocalVarEntry *entry = hlir_local_mem_find(&env, var);
      if (entry && entry->last_store && var->is_local) {
        // Dead Store Elimination: previous store was never read
        HLIRInsn *prev_store = entry->last_store;
        hlir_remove_insn(fn, prev_store);
        changed = true;
      }
      entry = hlir_local_mem_get_or_add(&env, var);
      entry->avail_val = insn->src1;
      entry->last_store = var->is_local ? insn : NULL;
      continue;
    }

    if (insn->kind == HLIR_LOAD_VAR && insn->var && insn->dst) {
      Obj2 *var = insn->var;
      HLIRLocalVarEntry *entry = hlir_local_mem_find(&env, var);
      if (entry && entry->avail_val) {
        // Store-to-Load and Load-to-Load forwarding
        hlir_set_cast(insn, entry->avail_val);
        entry->last_store = NULL;
        changed = true;
      } else {
        entry = hlir_local_mem_get_or_add(&env, var);
        entry->avail_val = insn->dst;
        entry->last_store = NULL;
      }
      continue;
    }
  }

  free(env.entries);
  return changed;
}
