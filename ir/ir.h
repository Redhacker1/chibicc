#ifndef CHIBICC_IR_H
#define CHIBICC_IR_H

#include "chibicc.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct IRVReg IRVReg;
typedef struct IRInsn IRInsn;
typedef struct IRFunction IRFunction;
typedef struct IRProg IRProg;

typedef enum {
  IR_NOP,
  // Constants
  IR_IMM,        // dst = imm
  IR_FIMM,       // dst = fimm
  IR_ADDR,       // dst = &var or &label
  // Memory
  IR_LOAD,       // dst = *src1
  IR_STORE,      // *src1 = src2
  IR_MEMCPY,     // memcpy(src1, src2, imm)
  IR_MEMZERO,    // memset(src1, 0, imm)
  // Register move & cast
  IR_MOV,        // dst = src1
  IR_CAST,       // dst = (dst_ty)src1
  // Arithmetic & Binary Logic
  IR_ADD,        // dst = src1 + src2
  IR_SUB,        // dst = src1 - src2
  IR_MUL,        // dst = src1 * src2
  IR_DIV,        // dst = src1 / src2
  IR_MOD,        // dst = src1 % src2
  IR_BITAND,     // dst = src1 & src2
  IR_BITOR,      // dst = src1 | src2
  IR_BITXOR,     // dst = src1 ^ src2
  IR_SHL,        // dst = src1 << src2
  IR_SHR,        // dst = src1 >> src2
  // Unary
  IR_NEG,        // dst = -src1
  IR_BITNOT,     // dst = ~src1
  IR_LOGNOT,     // dst = !src1
  // Comparisons
  IR_EQ,         // dst = (src1 == src2)
  IR_NE,         // dst = (src1 != src2)
  IR_LT,         // dst = (src1 < src2)
  IR_LE,         // dst = (src1 <= src2)
  IR_GT,         // dst = (src1 > src2)
  IR_GE,         // dst = (src1 >= src2)
  // Control Flow
  IR_LABEL,      // label:
  IR_JMP,        // jmp label
  IR_BR,         // if (src1) jmp label_true else jmp label_false
  IR_RET,        // ret [src1]
  // Function Calls & ABI
  IR_ARG,        // push arg for next call
  IR_CALL,       // dst = call src1(args...)
  IR_PARAM,      // dst = param_i
  // Intrinsics / Inline Asm
  IR_ASM,        // inline asm string
  IR_ALLOCA,     // dst = alloca(src1)
  IR_CAS,        // cas(addr=src1, old=src2, new=src3)
  IR_EXCH,       // exch(addr=src1, val=src2)
} IRKind;

struct IRVReg {
  int id;
  Type *ty;
  int phys_reg;     // Assigned physical register (-1 if none / spilled)
  int spill_offset; // Stack frame spill offset if spilled
  int def_pos;      // Instruction index where defined
  int last_use_pos; // Instruction index where last used
  bool is_float;
  bool is_spilled;
  bool is_pinned;   // If pre-assigned to a specific physical register
};

struct IRInsn {
  IRKind kind;
  IRInsn *prev;
  IRInsn *next;
  int pos;           // Instruction linear sequence number

  IRVReg *dst;
  IRVReg *src1;
  IRVReg *src2;
  IRVReg *src3;

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
  IRVReg **args;
};

struct IRFunction {
  Obj *fn_obj;
  char *name;
  Type *func_ty;
  ABI *abi;

  IRInsn *head;
  IRInsn *tail;
  int num_insns;

  IRVReg **vregs;
  int num_vregs;
  int vreg_capacity;

  Obj *locals;
  Obj *params;
  int stack_size;
  int shadow_space;
};

struct IRProg {
  Obj *globals;
  IRFunction **fns;
  int num_fns;
};

IRVReg *ir_new_vreg(IRFunction *fn, Type *ty);
IRInsn *ir_new_insn(IRKind kind);
void ir_append_insn(IRFunction *fn, IRInsn *insn);
IRFunction *ir_new_function(Obj *fn_obj);
IRProg *ast_to_ir(Obj *prog);
void ir_dump(FILE *out, IRProg *prog);

#endif // CHIBICC_IR_H
