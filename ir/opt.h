#ifndef CHIBICC_OPT_H
#define CHIBICC_OPT_H

#include "ir/ir.h"
#include "ir/hlir_opt.h"
#include <stdbool.h>
#include <stdio.h>

// Forward declarations
typedef struct IRPass IRPass;
typedef struct IRPassContext IRPassContext;
typedef struct IRPassManager IRPassManager;
typedef struct IRBasicBlock IRBasicBlock;
typedef struct IRCFG IRCFG;
typedef struct IRDomTree IRDomTree;
typedef struct IRLiveness IRLiveness;

typedef enum {
  IR_PASS_FUNCTION,
  IR_PASS_PROG,
  IR_PASS_ANALYSIS,
} IRPassType;

// Control Flow Graph representation
struct IRBasicBlock {
  int id;
  char *label;
  IRInsn *first;
  IRInsn *last;

  IRBasicBlock **preds;
  int num_preds;
  int pred_cap;

  IRBasicBlock **succs;
  int num_succs;
  int succ_cap;

  bool reachable;
  int rpo_index;
};

struct IRCFG {
  IRFunction *fn;
  IRBasicBlock **blocks;
  int num_blocks;
  int block_cap;
  IRBasicBlock *entry_block;
};

// Dominator Tree
struct IRDomTree {
  IRCFG *cfg;
  IRBasicBlock **idom;   // indexed by block id
  int *dom_depth;        // dominance tree depth
};

// Liveness Analysis
struct IRLiveness {
  IRFunction *fn;
  int num_vregs;
  int *def_pos;
  int *last_use_pos;
  bool *is_live_across_calls;
};

// Pass Execution Context & Analysis Cache
struct IRPassContext {
  int opt_level;
  bool dump_ir;
  bool verify_each;
  int iterations_run;
  int changes_count;

  // Cached analyses (computed lazily, invalidated when IR is mutated)
  IRCFG *cfg;
  IRDomTree *dom_tree;
  IRLiveness *liveness;

  void *user_data;
};

// Optimization Pass Definition
struct IRPass {
  const char *name;
  const char *description;
  IRPassType type;
  bool enabled;
  int default_opt_level;
  bool (*run_on_function)(IRFunction *fn, IRPassContext *ctx);
  bool (*run_on_prog)(IRProg *prog, IRPassContext *ctx);
};

// Pass Manager
struct IRPassManager {
  IRPass **passes;
  int num_passes;
  int capacity;
  int max_fixed_point_iterations;
  bool fixed_point;
};

// Pass Manager API
IRPassManager *ir_pass_manager_new(void);
void ir_pass_manager_free(IRPassManager *pm);
void ir_pass_manager_add(IRPassManager *pm, IRPass *pass);
bool ir_pass_manager_add_by_name(IRPassManager *pm, const char *name);
bool ir_pass_manager_run_function(IRPassManager *pm, IRFunction *fn, IRPassContext *ctx);
bool ir_pass_manager_run_prog(IRPassManager *pm, IRProg *prog, IRPassContext *ctx);

// Pass Registry & Fine-Tuning API
extern int opt_max_passes;
void ir_register_pass(IRPass *pass);
IRPass *ir_find_pass(const char *name);
IRPass **ir_get_all_passes(int *count);
void ir_init_pass_registry(void);
void ir_opt_init(void);
void ir_opt_set_level(int opt_level);
void ir_opt_set_max_passes(int max_passes);
int ir_opt_get_max_passes(void);
bool ir_opt_set_pass_enabled(const char *name, bool enabled);
bool ir_opt_is_pass_enabled(const char *name);
void ir_opt_print_passes(FILE *out);

// Pipeline Creation & Invocation
IRPassManager *ir_create_opt_pipeline(int opt_level);
void ir_optimize(IRProg *prog);
void ir_optimize_level(IRProg *prog, int opt_level);
void llir_optimize(LLIRProg *prog, int opt_level);

// Analysis API
IRCFG *ir_build_cfg(IRFunction *fn);
void ir_free_cfg(IRCFG *cfg);
void ir_dump_cfg(FILE *out, IRCFG *cfg);
void ir_invalidate_analyses(IRPassContext *ctx);
IRCFG *ir_get_or_build_cfg(IRFunction *fn, IRPassContext *ctx);

IRDomTree *ir_build_dom_tree(IRCFG *cfg);
void ir_free_dom_tree(IRDomTree *dt);
bool ir_dom_dominates(IRDomTree *dt, IRBasicBlock *a, IRBasicBlock *b);

IRLiveness *ir_compute_liveness(IRFunction *fn);
void ir_free_liveness(IRLiveness *liveness);

// IR Verifier
bool ir_verify_function(IRFunction *fn, char **err_out);
bool ir_verify_prog(IRProg *prog, char **err_out);

// Individual Low-Level (LLIR) Pass Functions
bool ir_opt_copy_prop(IRFunction *fn);
bool ir_opt_dce(IRFunction *fn);
bool ir_opt_cfg_simplify(IRFunction *fn);
bool ir_opt_peephole(IRFunction *fn);

// Built-in Pass Singletons
extern IRPass pass_copy_prop;
extern IRPass pass_dce;
extern IRPass pass_cfg_simplify;
extern IRPass pass_peephole;
extern IRPass pass_verifier;
extern IRPass pass_hlir_const_fold;
extern IRPass pass_hlir_algebraic;
extern IRPass pass_hlir_copy_prop;
extern IRPass pass_hlir_local_cse;
extern IRPass pass_hlir_load_store;
extern IRPass pass_hlir_control_flow;
extern IRPass pass_hlir_dead_code;
extern IRPass pass_hlir_dce;
extern IRPass pass_hlir_inlining;

#endif // CHIBICC_OPT_H
