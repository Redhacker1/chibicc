#ifndef CHIBICC_ABI_H
#define CHIBICC_ABI_H

#include <stdbool.h>
#include <stdio.h>
#include "abi/objfmt.h"
#include "abi/callconv.h"

typedef struct Type Type;
typedef struct Node Node;
typedef struct Obj Obj;
typedef struct ABI ABI;

struct ABI {
  const char *name;
  const char *description;

  // Declarative calling convention specification
  const CallConv *callconv;

  // Default object / assembler format
  ObjFmt *default_objfmt;

  // Basic type sizes in bytes
  int size_bool;
  int size_char;
  int size_short;
  int size_int;
  int size_long;
  int size_llong;
  int size_ptr;
  int size_float;
  int size_double;
  int size_ldouble;

  // Basic type alignments in bytes
  int align_bool;
  int align_char;
  int align_short;
  int align_int;
  int align_long;
  int align_llong;
  int align_ptr;
  int align_float;
  int align_double;
  int align_ldouble;

  // Stack alignment in bytes
  int align_stack;

  // Variadic argument area
  int va_area_size;
  int va_area_align;

  // ABI classification & properties
  bool (*returns_by_reference)(Type *ty);
  int (*classify_reg)(Type *ty);

  // Local variable and parameter layout calculation
  void (*assign_lvar_offsets)(Obj *prog);

  // Function call parameter pushing / preparation. Returns stack depth words added.
  int (*push_args)(Node *node, FILE *out, int *depth);

  // Struct return copy helpers
  void (*copy_ret_buffer)(Obj *var, FILE *out);
  void (*copy_struct_reg)(Obj *fn, FILE *out);
  void (*copy_struct_mem)(Obj *fn, FILE *out);

  // Builtins (e.g. alloca)
  void (*builtin_alloca)(Obj *fn, FILE *out);

  // Pre-call hook (e.g. setting %al for SysV variadic calls)
  void (*pre_call)(Node *node, FILE *out);

  // Function prologue & epilogue helpers
  void (*emit_prologue)(Obj *fn, FILE *out);
  void (*emit_epilogue)(Obj *fn, FILE *out);

  // Preprocessor macro definitions
  void (*define_macros)(void);

  // Type initialization
  void (*init_types)(void);
};

extern ABI *current_abi;

void register_abi(ABI *abi);
ABI *get_abi(const char *name);
void init_abis(void);
void set_abi(const char *name);

// Standard ABI instances
extern ABI abi_sysv64;
extern ABI abi_win64;
extern ABI abi_win32;
extern ABI abi_sys6;
extern ABI abi_pascal;
extern ABI abi_z80;

#endif // CHIBICC_ABI_H
