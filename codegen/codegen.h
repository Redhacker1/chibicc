#ifndef CHIBICC_CODEGEN_H
#define CHIBICC_CODEGEN_H

#include <stdbool.h>
#include <stdio.h>
#include "abi/abi.h"

typedef struct Obj Obj;
typedef struct Node Node;
typedef struct Codegen Codegen;
typedef struct LLIRProg LLIRProg;
typedef struct LLIRInsn LLIRInsn;

struct Codegen {
  const char *name;
  const char *description;
  const char *default_abi_name;

  // Backend initialization
  void (*init)(FILE *out);

  // Top-level program code generation from Low-Level IR (LLIR)
  void (*codegen_llir)(LLIRProg *prog, FILE *out);

  // Sub-phases of code generation
  void (*emit_data)(Obj *prog, FILE *out);
  void (*emit_text)(LLIRProg *prog, FILE *out);

  // Individual instruction / expression code generators working on LLIR
  void (*gen_insn)(LLIRInsn *insn, FILE *out);
  void (*gen_expr)(LLIRInsn *insn, FILE *out);
};

extern Codegen *current_codegen;

void register_codegen(Codegen *cg);
Codegen *get_codegen(const char *name);
void init_codegens(void);
void set_codegen(const char *name);

// Standard codegen backends
extern Codegen codegen_x86_64;
extern Codegen codegen_m68k;
extern Codegen codegen_z80;

#endif // CHIBICC_CODEGEN_H
