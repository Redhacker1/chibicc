#ifndef CHIBICC_HLIR_H
#define CHIBICC_HLIR_H

#include "chibicc.h"
#include <stdint.h>
#include <stdio.h>

// ==============================================================================
// High-Level Bytecode Intermediate Representation (HLIR)
// ==============================================================================

typedef enum {
  HLIR_NOP,
  // High-level Constants
  HLIR_ICONST,         // dst = int_const
  HLIR_FCONST,         // dst = float_const
  HLIR_SCONST,         // dst = string_literal_addr
  // High-level Variables & Memory
  HLIR_LOAD_VAR,       // dst = var
  HLIR_STORE_VAR,      // var = src1
  HLIR_ADDR_VAR,       // dst = &var
  HLIR_LOAD_MEMBER,    // dst = src1.member_offset
  HLIR_STORE_MEMBER,   // src1.member_offset = src2
  HLIR_LOAD_PTR,       // dst = *src1
  HLIR_STORE_PTR,      // *src1 = src2
  HLIR_MEMCPY,         // memcpy(dst=src1, src=src2, size)
  HLIR_MEMZERO,        // memset(dst=src1, 0, size)
  // High-level Operations
  HLIR_CAST,           // dst = (type)src1
  HLIR_ADD,            // dst = src1 + src2
  HLIR_SUB,            // dst = src1 - src2
  HLIR_MUL,            // dst = src1 * src2
  HLIR_DIV,            // dst = src1 / src2
  HLIR_MOD,            // dst = src1 % src2
  HLIR_BITAND,         // dst = src1 & src2
  HLIR_BITOR,          // dst = src1 | src2
  HLIR_BITXOR,         // dst = src1 ^ src2
  HLIR_SHL,            // dst = src1 << src2
  HLIR_SHR,            // dst = src1 >> src2
  HLIR_NEG,            // dst = -src1
  HLIR_BITNOT,         // dst = ~src1
  HLIR_LOGNOT,         // dst = !src1
  HLIR_CMP_EQ,         // dst = (src1 == src2)
  HLIR_CMP_NE,         // dst = (src1 != src2)
  HLIR_CMP_LT,         // dst = (src1 < src2)
  HLIR_CMP_LE,         // dst = (src1 <= src2)
  HLIR_CMP_GT,         // dst = (src1 > src2)
  HLIR_CMP_GE,         // dst = (src1 >= src2)
  // High-level Control Flow
  HLIR_LABEL,          // label:
  HLIR_JMP,            // goto label
  HLIR_JMP_IF_ZERO,    // if (!src1) goto label
  HLIR_JMP_IF_NZ,      // if (src1) goto label
  HLIR_RET,            // return [src1]
  HLIR_CALL,           // dst = call src1(args...)
  HLIR_PARAM,          // dst = param_index
  // High-level System / Intrinsics
  HLIR_ALLOCA,         // dst = alloca(src1)
  HLIR_ASM,            // inline assembly
  HLIR_CAS,            // cas(src1, src2, src3)
  HLIR_EXCH,           // exch(src1, src2)
} HLIRKind;

typedef struct HLIRVal HLIRVal;
typedef struct HLIRInsn HLIRInsn;
typedef struct HLIRFunction HLIRFunction;
typedef struct HLIRProg HLIRProg;

struct HLIRVal {
  int id;
  Type *ty;
  Obj *var;
};

struct HLIRInsn {
  HLIRKind kind;
  HLIRInsn *prev;
  HLIRInsn *next;
  int pos;

  HLIRVal *dst;
  HLIRVal *src1;
  HLIRVal *src2;
  HLIRVal *src3;

  int64_t imm;
  double fimm;
  char *label;
  Obj *var;
  Type *ty;
  int num_args;
  HLIRVal **args;
  char *asm_str;
};

struct HLIRFunction {
  Obj *fn_obj;
  char *name;
  Type *func_ty;

  HLIRInsn *head;
  HLIRInsn *tail;
  int num_insns;

  HLIRVal **vals;
  int num_vals;
  int val_cap;

  Obj *locals;
  Obj *params;
  int stack_size;
};

struct HLIRProg {
  Obj *globals;
  HLIRFunction **fns;
  int num_fns;
};

// Builder & Manipulation API
HLIRVal *hlir_new_val(HLIRFunction *fn, Type *ty);
HLIRInsn *hlir_new_insn(HLIRKind kind);
void hlir_append_insn(HLIRFunction *fn, HLIRInsn *insn);
void hlir_insert_before(HLIRFunction *fn, HLIRInsn *target, HLIRInsn *new_insn);
void hlir_insert_after(HLIRFunction *fn, HLIRInsn *target, HLIRInsn *new_insn);
void hlir_remove_insn(HLIRFunction *fn, HLIRInsn *insn);
void hlir_replace_insn(HLIRFunction *fn, HLIRInsn *old_insn, HLIRInsn *new_insn);

// Generation & Printing
HLIRProg *ast_to_hlir(Obj *prog);
void hlir_dump_function(FILE *out, HLIRFunction *fn);
void hlir_dump(FILE *out, HLIRProg *prog);

#endif // CHIBICC_HLIR_H
