#ifndef CHIBICC_OPT_H
#define CHIBICC_OPT_H

#include "ir/ir.h"

void ir_optimize(IRProg *prog);
void ir_opt_const_fold(IRFunction *fn);
void ir_opt_copy_prop(IRFunction *fn);
void ir_opt_dce(IRFunction *fn);

#endif // CHIBICC_OPT_H
