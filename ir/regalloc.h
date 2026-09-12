#ifndef CHIBICC_REGALLOC_H
#define CHIBICC_REGALLOC_H

#include "ir/ir.h"

typedef struct RegAllocPool RegAllocPool;

struct RegAllocPool {
  int num_gp_regs;
  const int *gp_regs;       // List of available GP register IDs (callee-saved)
  int num_scratch_gp_regs;
  const int *scratch_gp_regs; // List of scratch/volatile GP register IDs (non-call-spanning)
  int num_fp_regs;
  const int *fp_regs;       // List of available FP register IDs
  int spill_base_offset;
  int spill_align;
};

void regalloc_function(IRFunction *fn, const RegAllocPool *pool);
void regalloc_prog(IRProg *prog, const RegAllocPool *pool);

#endif // CHIBICC_REGALLOC_H
