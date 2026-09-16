#include "ir/hlir_opt.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// HLIR Control Flow Simplification Pass
// ============================================================================
// Optimizes high-level branches and jumps:
// 1. Prunes jumps targeting the immediate sequential fallthrough label
// 2. Performs jump threading across intermediate jump chains
// 3. Evaluates conditional branches on known compile-time constants
// 4. Simplifies conditional branch conditions (e.g. branch on !cond or comparison with 0)

static char *hlir_find_forwarded_label(HLIRFunction *fn, const char *label) {
  if (!label) return NULL;
  for (HLIRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind == HLIR_LABEL && insn->label && !strcmp(insn->label, label)) {
      HLIRInsn *nxt = insn->next;
      while (nxt && nxt->kind == HLIR_NOP)
        nxt = nxt->next;
      if (nxt && nxt->kind == HLIR_JMP && nxt->label && strcmp(nxt->label, label) != 0)
        return nxt->label;
      break;
    }
  }
  return NULL;
}

bool hlir_opt_control_flow(HLIRFunction *fn) {
  if (!fn || !fn->head)
    return false;

  bool changed = false;
  HLIREnv env;
  hlir_env_init(&env, fn->num_vals, true);

  for (HLIRInsn *insn = fn->head; insn; insn = insn->next) {
    // 1. Eliminate jump to immediate next label
    if (insn->kind == HLIR_JMP && insn->label) {
      HLIRInsn *nxt = insn->next;
      while (nxt && nxt->kind == HLIR_NOP)
        nxt = nxt->next;
      if (nxt && nxt->kind == HLIR_LABEL && nxt->label && !strcmp(insn->label, nxt->label)) {
        HLIRInsn *del = insn;
        insn = insn->prev ? insn->prev : fn->head;
        hlir_remove_insn(fn, del);
        changed = true;
        hlir_env_reset(&env);
        if (!insn) break;
        continue;
      }
    }

    // 2. Jump threading (target label forwarding)
    if ((insn->kind == HLIR_JMP || insn->kind == HLIR_JMP_IF_ZERO || insn->kind == HLIR_JMP_IF_NZ) && insn->label) {
      char *forwarded = hlir_find_forwarded_label(fn, insn->label);
      if (forwarded) {
        insn->label = forwarded;
        changed = true;
      }
    }

    if (insn->kind == HLIR_LABEL || insn->kind == HLIR_JMP || insn->kind == HLIR_CALL) {
      hlir_env_reset(&env);
      continue;
    }

    if (insn->dst)
      hlir_env_set_def(&env, insn->dst, insn);

    if (insn->kind == HLIR_ICONST && insn->dst) {
      hlir_env_set_const(&env, insn->dst, insn->imm);
      continue;
    }

    // 3. Branch on constant values
    int64_t c;
    if ((insn->kind == HLIR_JMP_IF_ZERO || insn->kind == HLIR_JMP_IF_NZ) &&
        hlir_env_get_const(&env, insn->src1, &c)) {
      bool taken = (insn->kind == HLIR_JMP_IF_ZERO) ? (c == 0) : (c != 0);
      if (taken) {
        insn->kind = HLIR_JMP;
        insn->src1 = NULL;
        changed = true;
      } else {
        HLIRInsn *del = insn;
        insn = insn->prev ? insn->prev : fn->head;
        hlir_remove_insn(fn, del);
        changed = true;
        if (!insn) break;
        continue;
      }
    }

    // 4. Conditional branch simplification on logical/relational inversions
    if ((insn->kind == HLIR_JMP_IF_ZERO || insn->kind == HLIR_JMP_IF_NZ) && insn->src1) {
      HLIRInsn *def = hlir_env_get_def(&env, insn->src1);
      if (def) {
        if (def->kind == HLIR_LOGNOT && def->src1) {
          // JMP_IF_ZERO (!x) -> JMP_IF_NZ (x); JMP_IF_NZ (!x) -> JMP_IF_ZERO (x)
          insn->kind = (insn->kind == HLIR_JMP_IF_ZERO) ? HLIR_JMP_IF_NZ : HLIR_JMP_IF_ZERO;
          insn->src1 = def->src1;
          changed = true;
        } else if (def->src1 && def->src2) {
          int64_t c1 = 0, c2 = 0;
          bool right_zero = hlir_env_get_const(&env, def->src2, &c2) && c2 == 0;
          bool left_zero = hlir_env_get_const(&env, def->src1, &c1) && c1 == 0;
          if (def->kind == HLIR_CMP_EQ && (right_zero || left_zero)) {
            insn->kind = (insn->kind == HLIR_JMP_IF_ZERO) ? HLIR_JMP_IF_NZ : HLIR_JMP_IF_ZERO;
            insn->src1 = right_zero ? def->src1 : def->src2;
            changed = true;
          } else if (def->kind == HLIR_CMP_NE && (right_zero || left_zero)) {
            insn->src1 = right_zero ? def->src1 : def->src2;
            changed = true;
          }
        }
      }
    }
  }

  hlir_env_free(&env);
  return changed;
}
