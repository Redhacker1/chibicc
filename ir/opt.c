#include "ir/opt.h"

// ============================================================================
// LLIR Optimization Framework
// ============================================================================
// The Low-Level Intermediate Representation (LLIR) optimizer is decomposed into
// modular components:
//
// 1. Analysis Infrastructure (ir/analysis.c):
//    - Control Flow Graph (CFG) construction and reachability analysis
//    - Dominator tree computation (Cooper-Harvey-Kennedy iterative algorithm)
//    - Liveness analysis (def-use intervals, variables crossing function calls)
//    - Program & function integrity verifiers
//
// 2. Pass Manager & Registry (ir/pass_manager.c):
//    - Central pass registry for dynamic discovery and fine-tuning via flags
//    - Pass execution context and analysis cache invalidation
//    - Fixed-point iteration pipeline runner
//
// 3. Transformation Passes:
//    - Copy Propagation (ir/opt_copy_prop.c): eliminates redundant register copies
//    - Dead Code Elimination (ir/opt_dce.c): removes unused side-effect free instructions
//    - CFG Simplification (ir/opt_cfg.c): folds constant branches, merges blocks, cleans jumps
//    - Peephole Optimizer (ir/opt_peephole.c): algebraic identities, strength reduction, memory forwarding
//    - Legalization / ISEL (ir/opt_legalize.c): prepares addressing modes and immediates for code generator
//
// See docs/ARCHITECTURE.md for an in-depth guide on the intermediate representations
// and instructions on adding new optimization passes.
// ============================================================================
