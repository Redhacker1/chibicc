#ifndef CHIBICC_IR_H
#define CHIBICC_IR_H

#include "chibicc.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// ==============================================================================
// 1. High-Level Bytecode Intermediate Representation (HLIR)
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

// HLIR API
HLIRProg *ast_to_hlir(Obj *prog);
void hlir_optimize(HLIRProg *prog, int opt_level);
void hlir_dump(FILE *out, HLIRProg *prog);
void hlir_dump_function(FILE *out, HLIRFunction *fn);

// ==============================================================================
// 2. Abstract SSA Low-Level Assembly Intermediate Representation (LLIR)
// ==============================================================================

typedef enum {
  LLIR_NOP,
  // SSA Phi Node
  LLIR_PHI,            // %dst = phi [%src_i, %bb_i]...
  // Assembly-like Constants & Address computation
  LLIR_IMM,            // %dst = imm
  LLIR_FIMM,           // %dst = fimm
  LLIR_LEA,            // %dst = &var + offset
  // Assembly-like 3-Address Arithmetic & Logic (maps cleanly to x86/ARM/m68k/Z80)
  LLIR_MOV,            // %dst = %src1
  LLIR_CAST,           // %dst = (%dst_ty)%src1
  LLIR_ADD,            // %dst = %src1 + %src2
  LLIR_SUB,            // %dst = %src1 - %src2
  LLIR_MUL,            // %dst = %src1 * %src2
  LLIR_DIV,            // %dst = %src1 / %src2
  LLIR_MOD,            // %dst = %src1 % %src2
  LLIR_AND,            // %dst = %src1 & %src2
  LLIR_OR,             // %dst = %src1 | %src2
  LLIR_XOR,            // %dst = %src1 ^ %src2
  LLIR_SHL,            // %dst = %src1 << %src2
  LLIR_SHR,            // %dst = %src1 >> %src2
  LLIR_NEG,            // %dst = -%src1
  LLIR_NOT,            // %dst = ~%src1
  LLIR_LOGNOT,         // %dst = !%src1
  // Explicit Comparisons & Condition Evaluation
  LLIR_CMP_EQ,         // %dst = (%src1 == %src2)
  LLIR_CMP_NE,         // %dst = (%src1 != %src2)
  LLIR_CMP_LT,         // %dst = (%src1 < %src2)
  LLIR_CMP_LE,         // %dst = (%src1 <= %src2)
  LLIR_CMP_GT,         // %dst = (%src1 > %src2)
  LLIR_CMP_GE,         // %dst = (%src1 >= %src2)
  // Memory Load & Store (explicit size/type)
  LLIR_LOAD,           // %dst = [%src1]
  LLIR_STORE,          // [%src1] = %src2
  LLIR_MEMCPY,         // memcpy(dst=%src1, src=%src2, bytes)
  LLIR_MEMZERO,        // memset(dst=%src1, 0, bytes)
  // Low-Level Abstract Control Flow & Jumps
  LLIR_LABEL,          // label:
  LLIR_JMP,            // jmp label
  LLIR_BR_COND,        // br %src1, true_label, false_label
  LLIR_RET,            // ret [%src1]
  // Abstract Call / Param ABI-independent lowering
  LLIR_ARG,            // push_arg %src1
  LLIR_CALL,           // %dst = call %src1(%args...)
  LLIR_PARAM,          // %dst = param[i]
  // Target & Low-level Intrinsics
  LLIR_ALLOCA,         // %dst = alloca(%src1)
  LLIR_ASM,            // inline asm string
  LLIR_CAS,            // cas(%addr=%src1, %old=%src2, %new=%src3)
  LLIR_EXCH
  bool is_spilled;,           // exch(%addr=%src1, %val=%src2)
} LLIRKind;

typedef struct LLIRVReg LLIRVReg;
typedef struct LLIRInsn LLIRInsn;
typedef struct LLIRBlock LLIRBlock;
typedef struct LLIRFunction LLIRFunction;
typedef struct LLIRProg LLIRProg;

struct LLIRVReg {
  int id;              // Unique SSA Virtual Register ID (%v0, %v1, ...)
  Type *ty;            // Value type
  LLIRInsn *def_insn;  // SSA single defining instruction
  int def_pos;         // Linear instruction position
  int last_use_pos;    // Last use position
  int phys_reg;        // Allocated physical register (-1 if none / spilled)
  int spill_offset;    // Stack frame spill slot
  bool is_float;
  bool is_pinned;
};

struct LLIRInsn {
  LLIRKind kind;
  LLIRInsn *prev;
  LLIRInsn *next;
  int pos;

  LLIRVReg *dst;       // Defined SSA register (Single Definition)
  LLIRVReg *src1;
  LLIRVReg *src2;
  LLIRVReg *src3;

  int64_t imm;
  double fimm;
  char *label;
  char *label_true;
  char *label_false;
  char *asm_str;
  Obj *var;
  Type *ty;
  ABI *call_abi;

  int num_args;
  LLIRVReg **args;
  char **arg_labels;   // For SSA phi incoming block labels
};

struct LLIRBlock {
  int id;
  char *label;
  LLIRInsn *first;
  LLIRInsn *last;
  LLIRBlock **preds;
  int num_preds;
  LLIRBlock **succs;
  int num_succs;
  bool reachable;
};

struct LLIRFunction {
  Obj *fn_obj;
  char *name;
  Type *func_ty;
  ABI *abi;

  LLIRInsn *head;
  LLIRInsn *tail;
  int num_insns;

  LLIRVReg **vregs;
  int num_vregs;
  int vreg_capacity;

  LLIRBlock **blocks;
  int num_blocks;

  Obj *locals;
  Obj *params;
  int stack_size;
  int shadow_space;
};

struct LLIRProg {
  Obj *globals;
  LLIRFunction **fns;
  int num_fns;
};

// LLIR API
LLIRProg *hlir_to_llir(HLIRProg *hlir);
void llir_optimize(LLIRProg *prog, int opt_level);
void llir_dump(FILE *out, LLIRProg *prog);
void llir_dump_function(FILE *out, LLIRFunction *fn);

// Backward compatibility & Aliases for legacy IR typedefs
typedef LLIRProg IRProg;
typedef LLIRFunction IRFunction;
typedef LLIRInsn IRInsn;
typedef LLIRVReg IRVReg;
typedef LLIRKind IRKind;

#define IR_NOP LLIR_NOP
#define IR_PHI LLIR_PHI
#define IR_IMM LLIR_IMM
#define IR_FIMM LLIR_FIMM
#define IR_ADDR LLIR_LEA
#define IR_LOAD LLIR_LOAD
#define IR_STORE LLIR_STORE
#define IR_MEMCPY LLIR_MEMCPY
#define IR_MEMZERO LLIR_MEMZERO
#define IR_MOV LLIR_MOV
#define IR_CAST LLIR_CAST
#define IR_ADD LLIR_ADD
#define IR_SUB LLIR_SUB
#define IR_MUL LLIR_MUL
#define IR_DIV LLIR_DIV
#define IR_MOD LLIR_MOD
#define IR_BITAND LLIR_AND
#define IR_BITOR LLIR_OR
#define IR_BITXOR LLIR_XOR
#define IR_SHL LLIR_SHL
#define IR_SHR LLIR_SHR
#define IR_NEG LLIR_NEG
#define IR_BITNOT LLIR_NOT
#define IR_LOGNOT LLIR_LOGNOT
#define IR_EQ LLIR_CMP_EQ
#define IR_NE LLIR_CMP_NE
#define IR_LT LLIR_CMP_LT
#define IR_LE LLIR_CMP_LE
#define IR_GT LLIR_CMP_GT
#define IR_GE LLIR_CMP_GE
#define IR_LABEL LLIR_LABEL
#define IR_JMP LLIR_JMP
#define IR_BR LLIR_BR_COND
#define IR_RET LLIR_RET
#define IR_ARG LLIR_ARG
#define IR_CALL LLIR_CALL
#define IR_PARAM LLIR_PARAM
#define IR_ASM LLIR_ASM
#define IR_ALLOCA LLIR_ALLOCA
#define IR_CAS LLIR_CAS
#define IR_EXCH LLIR_EXCH

LLIRVReg *ir_new_vreg(LLIRFunction *fn, Type *ty);
LLIRInsn *ir_new_insn(LLIRKind kind);
void ir_append_insn(LLIRFunction *fn, LLIRInsn *insn);
void ir_insert_before(LLIRFunction *fn, LLIRInsn *target, LLIRInsn *new_insn);
void ir_insert_after(LLIRFunction *fn, LLIRInsn *target, LLIRInsn *new_insn);
void ir_remove_insn(LLIRFunction *fn, LLIRInsn *insn);
void ir_replace_insn(LLIRFunction *fn, LLIRInsn *old_insn, LLIRInsn *new_insn);
void ir_renumber_insns(LLIRFunction *fn);
bool ir_insn_has_side_effects(LLIRInsn *insn);
bool ir_insn_is_terminator(LLIRInsn *insn);
bool ir_insn_is_branch(LLIRInsn *insn);
bool ir_insn_is_commutative(LLIRKind kind);

LLIRFunction *ir_new_function(Obj *fn_obj);
LLIRProg *ast_to_ir(Obj *prog);
void ir_dump(FILE *out, LLIRProg *prog);
void ir_dump_function(FILE *out, LLIRFunction *fn);

#endif // CHIBICC_IR_H
