#include "chibicc.h"
#include "codegen/codegen.h"
#include "abi/abi.h"
#include "ir/ir.h"
#include "ir/opt.h"

Codegen *current_codegen = NULL;

static Codegen *codegens[32];
static int codegen_count = 0;

void register_codegen(Codegen *cg) {
  for (int i = 0; i < codegen_count; i++) {
    if (!strcmp(codegens[i]->name, cg->name)) {
      codegens[i] = cg;
      return;
    }
  }
  if (codegen_count < 32)
    codegens[codegen_count++] = cg;
}

Codegen *get_codegen(const char *name) {
  for (int i = 0; i < codegen_count; i++) {
    if (!strcmp(codegens[i]->name, name))
      return codegens[i];
  }
  return NULL;
}

void set_codegen(const char *name) {
  Codegen *cg = get_codegen(name);
  if (!cg)
    error("unknown codegen target: %s", (char *)name);
  current_codegen = cg;
  if (cg->default_abi_name && !current_abi)
    set_abi(cg->default_abi_name);
}

void init_codegens(void) {
  codegen_count = 0;
  register_codegen(&codegen_x86_64);
  register_codegen(&codegen_m68k);
  register_codegen(&codegen_z80);

  // Default codegen is x86_64
  current_codegen = &codegen_x86_64;
}

void init_target(const char *target_name, const char *abi_name) {
  if (target_name) {
    if (!strcmp(target_name, "x86_64") || !strcmp(target_name, "x86-64") || !strcmp(target_name, "amd64")) {
      set_codegen("x86_64");
    } else if (!strcmp(target_name, "m68k") || !strcmp(target_name, "68k") || !strcmp(target_name, "mac")) {
      set_codegen("m68k");
      if (!abi_name)
        abi_name = "sys6";
    } else if (!strcmp(target_name, "z80")) {
      set_codegen("z80");
      if (!abi_name)
        abi_name = "z80";
    } else {
      set_codegen(target_name);
    }
  }

  if (abi_name) {
    set_abi(abi_name);
  } else if (!current_abi && current_codegen && current_codegen->default_abi_name) {
    set_abi(current_codegen->default_abi_name);
  }
}

void init_all_targets_and_abis(void) {
  init_abis();
  init_codegens();

#if _WIN32
  init_target("x86_64", "win64");
#else
  init_target("x86_64", "sysv64");
#endif
}

void codegen(Obj *prog, FILE *out) {
  if (!current_codegen)
    init_all_targets_and_abis();

  // 1. Lower AST to High-Level Bytecode Intermediate Representation (HLIR)
  HLIRProg *hlir = ast_to_hlir(prog);
  if (opt_O > 0)
    hlir_optimize(hlir, opt_O);

  // 2. Lower HLIR to Abstract SSA Low-Level Assembly Intermediate Representation (LLIR)
  LLIRProg *llir = hlir_to_llir(hlir);
  if (opt_O > 0)
    llir_optimize(llir, opt_O);

  if (opt_dump_ir)
    llir_dump(stderr, llir);

  // 3. Emit target machine assembly from Low-Level IR
  if (current_codegen->codegen_llir)
    current_codegen->codegen_llir(llir, out);
  else
    error("active codegen backend has no entry point");
}
