#ifndef CHIBICC_CODEGEN_H
#define CHIBICC_CODEGEN_H

#include <stdbool.h>
#include <stdio.h>
#include "abi/abi.h"

typedef struct Obj Obj;
typedef struct Node Node;
typedef struct Codegen Codegen;

struct Codegen {
  const char *name;
  const char *description;
  const char *default_abi_name;

  // Backend initialization
  void (*init)(FILE *out);

  // Top-level program code generation
  void (*codegen)(Obj *prog, FILE *out);

  // Sub-phases of code generation
  void (*emit_data)(Obj *prog, FILE *out);
  void (*emit_text)(Obj *prog, FILE *out);

  // Individual statement/expression code generators
  void (*gen_stmt)(Node *node, FILE *out);
  void (*gen_expr)(Node *node, FILE *out);
  void (*gen_addr)(Node *node, FILE *out);
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
