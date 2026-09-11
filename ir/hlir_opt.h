#ifndef CHIBICC_HLIR_OPT_H
#define CHIBICC_HLIR_OPT_H

#include "ir/hlir.h"
#include <stdbool.h>

// HLIR Optimization Passes
bool hlir_opt_const_fold(HLIRFunction *fn);
bool hlir_opt_algebraic(HLIRFunction *fn);
bool hlir_opt_copy_prop(HLIRFunction *fn);
bool hlir_opt_local_cse(HLIRFunction *fn);
bool hlir_opt_load_store(HLIRFunction *fn);
bool hlir_opt_control_flow(HLIRFunction *fn);
bool hlir_opt_dce(HLIRFunction *fn);
bool hlir_opt_dead_code(HLIRFunction *fn);
bool hlir_opt_inlining(HLIRProg *prog);

// HLIR Fixed-Point Optimization Pipeline Driver
void hlir_optimize(HLIRProg *prog, int opt_level);

#endif // CHIBICC_HLIR_OPT_H
