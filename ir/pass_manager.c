#include "ir/opt.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// Pass Manager and Registry Infrastructure
// ============================================================================
// Coordinates the registration, enabling/disabling via CLI flags, and
// execution of both High-Level IR (HLIR) and Low-Level IR (LLIR) passes.

// Global pass iteration limits and configuration
int opt_max_passes = 200;

void ir_opt_set_max_passes(int max_passes) {
  if (max_passes > 0)
    opt_max_passes = max_passes;
}

int ir_opt_get_max_passes(void) {
  return opt_max_passes;
}

// ----------------------------------------------------------------------------
// Pass Function Wrappers
// ----------------------------------------------------------------------------

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

static bool run_pass_legalize_isel(IRFunction *fn, IRPassContext *ctx) {
  (void)ctx;
  return ir_opt_legalize_isel(fn);
}

// ----------------------------------------------------------------------------
// Pass Definitions (Singletons)
// ----------------------------------------------------------------------------

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

IRPass pass_legalize_isel = {
  .name = "legalize-isel",
  .description = "Instruction selection and address mode / immediate legalization",
  .type = IR_PASS_FUNCTION,
  .enabled = true,
  .default_opt_level = 1,
  .run_on_function = run_pass_legalize_isel,
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

// ----------------------------------------------------------------------------
// Pass Registry
// ----------------------------------------------------------------------------

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
  ir_register_pass(&pass_legalize_isel);
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

// ----------------------------------------------------------------------------
// Pass Manager Implementation
// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------
// Pipeline Execution
// ----------------------------------------------------------------------------

IRPassManager *ir_create_opt_pipeline(int opt_level) {
  ir_init_pass_registry();
  IRPassManager *pm = ir_pass_manager_new();

  pm->fixed_point = true;
  pm->max_fixed_point_iterations = (opt_max_passes > 0) ? opt_max_passes : 2;

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
  if (!prog || opt_level <= 0)
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
  for (int i = 0; i < prog->num_fns; i++)
    ir_opt_legalize_isel(prog->fns[i]);
}
