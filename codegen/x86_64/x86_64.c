#include "chibicc.h"
#include "codegen/codegen.h"
#include "codegen/common/common.h"
#include "ir/ir.h"
#include "ir/regalloc.h"

static FILE *output_file;
static int depth;
Obj *current_fn = NULL;
static bool *current_needs_mat = NULL;

static void compute_materialization(LLIRFunction *fn) {
  if (current_needs_mat) {
    free(current_needs_mat);
    current_needs_mat = NULL;
  }
  if (!fn || fn->num_vregs == 0)
    return;

  current_needs_mat = calloc(fn->num_vregs, sizeof(bool));
  for (int i = 0; i < fn->num_vregs; i++)
    current_needs_mat[i] = true;

  int *use_count = calloc(fn->num_vregs, sizeof(int));
  for (LLIRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->src1) use_count[insn->src1->id]++;
    if (insn->src2) use_count[insn->src2->id]++;
    if (insn->src3) use_count[insn->src3->id]++;
    for (int a = 0; a < insn->num_args; a++) {
      if (insn->args[a]) use_count[insn->args[a]->id]++;
    }
  }

  // Iteratively identify folded address computations, immediate operands, and dead pure vregs
  bool changed = true;
  while (changed) {
    changed = false;

    // 1. Foldable ADD / SUB offset calculations
    for (int i = 0; i < fn->num_vregs; i++) {
      LLIRVReg *v = fn->vregs[i];
      if (!v || !v->def_insn || !current_needs_mat[v->id])
        continue;

      LLIRInsn *def = v->def_insn;
      if (def->kind == LLIR_ADD && def->src1 && def->src2) {
        LLIRInsn *d1 = def->src1->def_insn;
        LLIRInsn *d2 = def->src2->def_insn;
        LLIRVReg *base = NULL;
        LLIRVReg *imm_vreg = NULL;
        if (d2 && d2->kind == LLIR_IMM) {
          base = def->src1;
          imm_vreg = def->src2;
        } else if (d1 && d1->kind == LLIR_IMM) {
          base = def->src2;
          imm_vreg = def->src1;
        }

        if (base && imm_vreg) {
          bool all_uses_mem = true;
          for (LLIRInsn *insn = fn->head; insn; insn = insn->next) {
            if (insn->src1 == v) {
              if (insn->kind != LLIR_LOAD && insn->kind != LLIR_STORE) {
                all_uses_mem = false;
                break;
              }
            }
            if (insn->src2 == v || insn->src3 == v) {
              all_uses_mem = false;
              break;
            }
            for (int a = 0; a < insn->num_args; a++) {
              if (insn->args[a] == v) {
                all_uses_mem = false;
                break;
              }
            }
          }

          if (all_uses_mem) {
            current_needs_mat[v->id] = false;
            changed = true;
          }
        }
      }

      // 2. Foldable LEA of variables
      if (def->kind == LLIR_LEA && def->var) {
        bool all_uses_foldable = true;
        for (LLIRInsn *insn = fn->head; insn; insn = insn->next) {
          if (insn->src1 == v) {
            if (insn->kind != LLIR_LOAD && insn->kind != LLIR_STORE) {
              // Could be used in a non-materialized ADD
              if (insn->kind == LLIR_ADD && insn->dst && !current_needs_mat[insn->dst->id]) {
                // folded
              } else {
                all_uses_foldable = false;
                break;
              }
            }
          }
          if (insn->src2 == v) {
            if (insn->kind == LLIR_ADD && insn->dst && !current_needs_mat[insn->dst->id]) {
              // folded
            } else {
              all_uses_foldable = false;
              break;
            }
          }
          if (insn->src3 == v) {
            all_uses_foldable = false;
            break;
          }
          for (int a = 0; a < insn->num_args; a++) {
            if (insn->args[a] == v) {
              all_uses_foldable = false;
              break;
            }
          }
        }

        if (all_uses_foldable) {
          current_needs_mat[v->id] = false;
          changed = true;
        }
      }

      // 3. Foldable IMM used in foldable immediate operations or address calculations
      if (def->kind == LLIR_IMM) {
        int64_t imm = def->imm;
        bool imm32 = ((int32_t)imm == imm);
        bool all_uses_folded = true;
        for (LLIRInsn *insn = fn->head; insn; insn = insn->next) {
          if (insn->src1 == v) {
            if (insn->kind == LLIR_ADD && insn->dst && !current_needs_mat[insn->dst->id]) {
              // address ADD
            } else if (imm32 && (insn->kind == LLIR_ADD || insn->kind == LLIR_MUL ||
                                 insn->kind == LLIR_AND || insn->kind == LLIR_OR ||
                                 insn->kind == LLIR_XOR) && (!insn->ty || !is_flonum(insn->ty))) {
              // commutative binop with imm
            } else {
              all_uses_folded = false;
              break;
            }
          }
          if (insn->src2 == v) {
            if (insn->kind == LLIR_ADD && insn->dst && !current_needs_mat[insn->dst->id]) {
              // address ADD
            } else if (imm32 && (insn->kind == LLIR_ADD || insn->kind == LLIR_SUB ||
                                 insn->kind == LLIR_MUL || insn->kind == LLIR_AND ||
                                 insn->kind == LLIR_OR || insn->kind == LLIR_XOR) && (!insn->ty || !is_flonum(insn->ty))) {
              // binop with imm
            } else if (insn->kind == LLIR_SHL || insn->kind == LLIR_SHR) {
              // shift with imm
            } else if (imm32 && (insn->kind == LLIR_CMP_EQ || insn->kind == LLIR_CMP_NE ||
                                 insn->kind == LLIR_CMP_LT || insn->kind == LLIR_CMP_LE ||
                                 insn->kind == LLIR_CMP_GT || insn->kind == LLIR_CMP_GE) &&
                       (!insn->src1 || !insn->src1->ty || !is_flonum(insn->src1->ty))) {
              // comparison with imm
            } else if (imm32 && insn->kind == LLIR_STORE && (!insn->ty || (insn->ty->kind != TY_STRUCT && insn->ty->kind != TY_UNION))) {
              // store imm
            } else {
              all_uses_folded = false;
              break;
            }
          }
          if (insn->src3 == v) {
            all_uses_folded = false;
            break;
          }
          for (int a = 0; a < insn->num_args; a++) {
            if (insn->args[a] == v) {
              all_uses_folded = false;
              break;
            }
          }
        }
        if (all_uses_folded) {
          current_needs_mat[v->id] = false;
          changed = true;
        }
      }
    }
  }

  free(use_count);
}

static void gen_expr(Node *node, FILE *out);
static void gen_stmt(Node *node, FILE *out);

__attribute__((format(printf, 1, 2)))
static void println(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(output_file, fmt, ap);
  va_end(ap);
  fputc('\n', output_file);
}

static int count(void) {
  return codegen_label_count();
}

static void push(void) {
  println("  push %%rax");
  depth++;
}

static void pop(char *arg) {
  println("  pop %s", arg);
  depth--;
}

static void pushf(void) {
  println("  sub $8, %%rsp");
  println("  movsd %%xmm0, (%%rsp)");
  depth++;
}

static void popf(int reg) {
  println("  movsd (%%rsp), %%xmm%d", reg);
  println("  add $8, %%rsp");
  depth--;
}

static char *reg_cx(int sz) {
  switch (sz) {
  case 1: return "%cl";
  case 2: return "%cx";
  case 4: return "%ecx";
  case 8: return "%rcx";
  }
  unreachable();
}

static char *reg_dx(int sz) {
  switch (sz) {
  case 1: return "%dl";
  case 2: return "%dx";
  case 4: return "%edx";
  case 8: return "%rdx";
  }
  unreachable();
}

static char *reg_ax(int sz) {
  switch (sz) {
  case 1: return "%al";
  case 2: return "%ax";
  case 4: return "%eax";
  case 8: return "%rax";
  }
  unreachable();
}

static char *reg_di(int sz) {
  switch (sz) {
  case 1: return "%dil";
  case 2: return "%di";
  case 4: return "%edi";
  case 8: return "%rdi";
  }
  unreachable();
}

static char *reg_si(int sz) {
  switch (sz) {
  case 1: return "%sil";
  case 2: return "%si";
  case 4: return "%esi";
  case 8: return "%rsi";
  }
  unreachable();
}

/*
 * Return the register width used for an integer expression.
 *
 * C integer expressions smaller than int have normally already undergone
 * integer promotion, so 1/2-byte objects are generally represented in eax.
 */
static char *int_reg(Type *ty) {
  if (!ty)
    return "%rax";

  if (ty->size <= 4 && !ty->base)
    return "%eax";

  return "%rax";
}

static bool is_simple_local(Node *node) {
  if (!node || node->kind != ND_VAR || !node->var)
    return false;

  Obj *var = node->var;
  Type *ty = var->ty;

  if (!var->is_local || !ty)
    return false;

  switch (ty->kind) {
  case TY_VLA:
  case TY_ARRAY:
  case TY_STRUCT:
  case TY_UNION:
  case TY_FUNC:
    return false;
  default:
    return !is_flonum(ty);
  }
}

static bool is_simple_integer_imm(Node *node) {
  if (!node || node->kind != ND_NUM || !node->ty)
    return false;

  if (is_flonum(node->ty))
    return false;

  return node->val >= -2147483648LL &&
         node->val <= 2147483647LL;
}

/*
 * Compute the absolute address of a given node.
 */
static void gen_addr(Node *node, FILE *out) {
  (void)out;
  assert(node != NULL);
  assert(node->tok != NULL);

  switch (node->kind) {
  case ND_VAR:
    /*
     * Variable-length array, which is always local.
     */
    if (node->var->ty->kind == TY_VLA) {
      println("  mov %d(%%rbp), %%rax", node->var->offset);
      return;
    }

    /*
     * Local variable.
     */
    if (node->var->is_local) {
      println("  lea %d(%%rbp), %%rax", node->var->offset);
      return;
    }

    if (opt_fpic) {
      /*
       * Thread-local variable.
       */
      if (node->var->is_tls) {
        println("  data16 lea %s@tlsgd(%%rip), %%rdi", node->var->name);
        println("  .value 0x6666");
        println("  rex64");
        println("  call __tls_get_addr@PLT");
        return;
      }

      /*
       * Function or global variable.
       */
      println("  mov %s@GOTPCREL(%%rip), %%rax", node->var->name);
      return;
    }

    /*
     * Thread-local variable.
     */
    if (node->var->is_tls) {
      println("  mov %%fs:0, %%rax");
      println("  add $%s@tpoff, %%rax", node->var->name);
      return;
    }

    /*
     * RIP-relative addressing for global variables/functions.
     */
    println("  lea %s(%%rip), %%rax", node->var->name);
    return;

  case ND_DEREF:
    gen_expr(node->lhs, output_file);
    return;

  case ND_COMMA:
    gen_expr(node->lhs, output_file);
    gen_addr(node->rhs, output_file);
    return;

  case ND_MEMBER:
    gen_addr(node->lhs, output_file);

    /*
     * Don't bother emitting an add for a zero-offset member.
     */
    if (node->member->offset != 0)
      println("  add $%d, %%rax", node->member->offset);

    return;

  case ND_FUNCALL:
    if (node->ret_buffer) {
      gen_expr(node, output_file);
      return;
    }
    break;

  case ND_ASSIGN:
  case ND_COND:
    if (node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION) {
      gen_expr(node, output_file);
      return;
    }
    break;

  case ND_VLA_PTR:
    println("  lea %d(%%rbp), %%rax", node->var->offset);
    return;
  }

  error_tok(node->tok, "not an lvalue");
}

/*
 * Load a value from where %rax is pointing to.
 */
static void load(Type *ty) {
  if (!ty)
    return;

  switch (ty->kind) {
  case TY_ARRAY:
  case TY_STRUCT:
  case TY_UNION:
  case TY_FUNC:
  case TY_VLA:
    return;

  case TY_FLOAT:
    println("  movss (%%rax), %%xmm0");
    return;

  case TY_DOUBLE:
    println("  movsd (%%rax), %%xmm0");
    return;

  case TY_LDOUBLE:
    println("  fldt (%%rax)");
    return;
  }

  char *insn = ty->is_unsigned ? "movz" : "movs";

  /*
   * char/short values are promoted to int.
   */
  if (ty->size == 1)
    println("  %sbl (%%rax), %%eax", insn);
  else if (ty->size == 2)
    println("  %swl (%%rax), %%eax", insn);
  else if (ty->size == 4) {
    /*
     * IMPORTANT:
     *
     * movsxd is wrong for uint32_t/int unsigned.
     *
     * A 32-bit register write also gives us the desired zero extension
     * for unsigned values.
     */
    if (ty->is_unsigned)
      println("  mov (%%rax), %%eax");
    else
      println("  movsxd (%%rax), %%rax");
  } else {
    println("  mov (%%rax), %%rax");
  }
}

static void store(Type *ty) {
  pop("%rdi");

  if (!ty)
    return;

  switch (ty->kind) {
  case TY_STRUCT:
  case TY_UNION: {
    /*
     * Small aggregates are common and don't need the generic copy loop.
     */
    if (ty->size == 1) {
      println("  mov (%%rax), %%r8b");
      println("  mov %%r8b, (%%rdi)");
      return;
    }

    if (ty->size == 2) {
      println("  mov (%%rax), %%r8w");
      println("  mov %%r8w, (%%rdi)");
      return;
    }

    if (ty->size == 4) {
      println("  mov (%%rax), %%r8d");
      println("  mov %%r8d, (%%rdi)");
      return;
    }

    if (ty->size == 8) {
      println("  mov (%%rax), %%r8");
      println("  mov %%r8, (%%rdi)");
      return;
    }

    int i = 0;

    while (ty->size - i >= 8) {
      println("  mov %d(%%rax), %%r8", i);
      println("  mov %%r8, %d(%%rdi)", i);
      i += 8;
    }

    while (ty->size - i >= 4) {
      println("  mov %d(%%rax), %%r8d", i);
      println("  mov %%r8d, %d(%%rdi)", i);
      i += 4;
    }

    while (ty->size - i >= 2) {
      println("  mov %d(%%rax), %%r8w", i);
      println("  mov %%r8w, %d(%%rdi)", i);
      i += 2;
    }

    while (i < ty->size) {
      println("  mov %d(%%rax), %%r8b", i);
      println("  mov %%r8b, %d(%%rdi)", i);
      i++;
    }

    return;
  }

  case TY_FLOAT:
    println("  movss %%xmm0, (%%rdi)");
    return;

  case TY_DOUBLE:
    println("  movsd %%xmm0, (%%rdi)");
    return;

  case TY_LDOUBLE:
    println("  fstpt (%%rdi)");
    return;
  }

  if (ty->size == 1)
    println("  mov %%al, (%%rdi)");
  else if (ty->size == 2)
    println("  mov %%ax, (%%rdi)");
  else if (ty->size == 4)
    println("  mov %%eax, (%%rdi)");
  else
    println("  mov %%rax, (%%rdi)");
}

static void cmp_zero(Type *ty) {
  if (!ty) {
    println("  test %%rax, %%rax");
    return;
  }

  switch (ty->kind) {
  case TY_FLOAT:
    println("  xorps %%xmm1, %%xmm1");
    println("  ucomiss %%xmm1, %%xmm0");
    return;

  case TY_DOUBLE:
    println("  xorpd %%xmm1, %%xmm1");
    println("  ucomisd %%xmm1, %%xmm0");
    return;

  case TY_LDOUBLE:
    println("  fldz");
    println("  fucomip");
    println("  fstp %%st(0)");
    return;
  }

  if (is_integer(ty) && ty->size <= 4)
    println("  test %%eax, %%eax");
  else
    println("  test %%rax, %%rax");
}

enum {
  I8, I16, I32, I64,
  U8, U16, U32, U64,
  F32, F64, F80
};

static int getTypeId(Type *ty) {
  switch (ty->kind) {
  case TY_CHAR:
    return ty->is_unsigned ? U8 : I8;

  case TY_SHORT:
    return ty->is_unsigned ? U16 : I16;

  case TY_INT:
    return ty->is_unsigned ? U32 : I32;

  case TY_LONG:
    return ty->is_unsigned
      ? (ty->size == 4 ? U32 : U64)
      : (ty->size == 4 ? I32 : I64);

  case TY_LONGLONG:
    return ty->is_unsigned ? U64 : I64;

  case TY_FLOAT:
    return F32;

  case TY_DOUBLE:
    return F64;

  case TY_LDOUBLE:
    return F80;
  }

  return U64;
}

/*
 * The table for type casts.
 */
static char i32i8[] = "movsbl %al, %eax";
static char i32u8[] = "movzbl %al, %eax";
static char i32i16[] = "movswl %ax, %eax";
static char i32u16[] = "movzwl %ax, %eax";
static char i32f32[] = "cvtsi2ssl %eax, %xmm0";
static char i32i64[] = "movsxd %eax, %rax";
static char i32f64[] = "cvtsi2sdl %eax, %xmm0";
static char i32f80[] = "mov %eax, -4(%rsp); fildl -4(%rsp)";

static char u32f32[] = "mov %eax, %eax; cvtsi2ssq %rax, %xmm0";
static char u32i64[] = "mov %eax, %eax";
static char u32f64[] = "mov %eax, %eax; cvtsi2sdq %rax, %xmm0";
static char u32f80[] = "mov %eax, %eax; mov %rax, -8(%rsp); fildll -8(%rsp)";

static char i64f32[] = "cvtsi2ssq %rax, %xmm0";
static char i64f64[] = "cvtsi2sdq %rax, %xmm0";
static char i64f80[] = "movq %rax, -8(%rsp); fildll -8(%rsp)";

static char u64f32[] = "cvtsi2ssq %rax, %xmm0";
static char u64f64[] =
  "test %rax,%rax; js 1f; pxor %xmm0,%xmm0; cvtsi2sd %rax,%xmm0; jmp 2f; "
  "1: mov %rax,%rdi; and $1,%eax; pxor %xmm0,%xmm0; shr %rdi; "
  "or %rax,%rdi; cvtsi2sd %rdi,%xmm0; addsd %xmm0,%xmm0; 2:";
static char u64f80[] =
  "mov %rax, -8(%rsp); fildq -8(%rsp); test %rax, %rax; jns 1f;"
  "mov $1602224128, %eax; mov %eax, -4(%rsp); fadds -4(%rsp); 1:";

static char f32i8[] = "cvttss2sil %xmm0, %eax; movsbl %al, %eax";
static char f32u8[] = "cvttss2sil %xmm0, %eax; movzbl %al, %eax";
static char f32i16[] = "cvttss2sil %xmm0, %eax; movswl %ax, %eax";
static char f32u16[] = "cvttss2sil %xmm0, %eax; movzwl %ax, %eax";
static char f32i32[] = "cvttss2sil %xmm0, %eax";
static char f32u32[] = "cvttss2siq %xmm0, %rax";
static char f32i64[] = "cvttss2siq %xmm0, %rax";
static char f32u64[] = "cvttss2siq %xmm0, %rax";
static char f32f64[] = "cvtss2sd %xmm0, %xmm0";
static char f32f80[] = "movss %xmm0, -4(%rsp); flds -4(%rsp)";

static char f64i8[] = "cvttsd2sil %xmm0, %eax; movsbl %al, %eax";
static char f64u8[] = "cvttsd2sil %xmm0, %eax; movzbl %al, %eax";
static char f64i16[] = "cvttsd2sil %xmm0, %eax; movswl %ax, %eax";
static char f64u16[] = "cvttsd2sil %xmm0, %eax; movzwl %ax, %eax";
static char f64i32[] = "cvttsd2sil %xmm0, %eax";
static char f64u32[] = "cvttsd2siq %xmm0, %rax";
static char f64i64[] = "cvttsd2siq %xmm0, %rax";
static char f64u64[] = "cvttsd2siq %xmm0, %rax";
static char f64f32[] = "cvtsd2ss %xmm0, %xmm0";
static char f64f80[] = "movsd %xmm0, -8(%rsp); fldl -8(%rsp)";

#define FROM_F80_1                                           \
  "fnstcw -10(%rsp); movzwl -10(%rsp), %eax; or $12, %ah; " \
  "mov %ax, -12(%rsp); fldcw -12(%rsp); "

#define FROM_F80_2 " -24(%rsp); fldcw -10(%rsp); "

static char f80i8[] = FROM_F80_1 "fistps" FROM_F80_2 "movsbl -24(%rsp), %eax";
static char f80u8[] = FROM_F80_1 "fistps" FROM_F80_2 "movzbl -24(%rsp), %eax";
static char f80i16[] = FROM_F80_1 "fistps" FROM_F80_2 "movzbl -24(%rsp), %eax";
static char f80u16[] = FROM_F80_1 "fistpl" FROM_F80_2 "movswl -24(%rsp), %eax";
static char f80i32[] = FROM_F80_1 "fistpl" FROM_F80_2 "mov -24(%rsp), %eax";
static char f80u32[] = FROM_F80_1 "fistpl" FROM_F80_2 "mov -24(%rsp), %eax";
static char f80i64[] = FROM_F80_1 "fistpq" FROM_F80_2 "mov -24(%rsp), %rax";
static char f80u64[] = FROM_F80_1 "fistpq" FROM_F80_2 "mov -24(%rsp), %rax";
static char f80f32[] = "fstps -8(%rsp); movss -8(%rsp), %xmm0";
static char f80f64[] = "fstpl -8(%rsp); movsd -8(%rsp), %xmm0";

static char *cast_table[][11] = {
  {NULL,  NULL,   NULL,   i32i64, i32u8, i32u16, NULL,   i32i64, i32f32, i32f64, i32f80},
  {i32i8, NULL,   NULL,   i32i64, i32u8, i32u16, NULL,   i32i64, i32f32, i32f64, i32f80},
  {i32i8, i32i16, NULL,   i32i64, i32u8, i32u16, NULL,   i32i64, i32f32, i32f64, i32f80},
  {i32i8, i32i16, NULL,   NULL,   i32u8, i32u16, NULL,   NULL,   i64f32, i64f64, i64f80},

  {i32i8, NULL,   NULL,   i32i64, NULL,  NULL,   NULL,   i32i64, i32f32, i32f64, i32f80},
  {i32i8, i32i16, NULL,   i32i64, i32u8, i32u16, NULL,   i32i64, i32f32, i32f64, i32f80},
  {i32i8, i32i16, NULL,   u32i64, i32u8, i32u16, NULL,   u32i64, u32f32, u32f64, u32f80},
  {i32i8, i32i16, NULL,   NULL,   i32u8, i32u16, NULL,   NULL,   u64f32, u64f64, u64f80},

  {f32i8, f32i16, f32i32, f32i64, f32u8, f32u16, f32u32, f32u64, NULL,   f32f64, f32f80},
  {f64i8, f64i16, f64i32, f64i64, f64u8, f64u16, f64u32, f64u64, f64f32, NULL,   f64f80},
  {f80i8, f80i16, f80i32, f80i64, f80u8, f80u16, f80u32, f80u64, f80f32, f80f64, NULL},
};

static void cast(Type *from, Type *to) {
  if (to->kind == TY_VOID)
    return;

  if (to->kind == TY_BOOL) {
    cmp_zero(from);
    println("  setne %%al");
    println("  movzbl %%al, %%eax");
    return;
  }

  int t1 = getTypeId(from);
  int t2 = getTypeId(to);

  if (cast_table[t1][t2])
    println("  %s", cast_table[t1][t2]);
}

/*
 * Generate code for a given node.
 */
static void gen_expr(Node *node, FILE *out) {
  (void)out;

  assert(node != NULL);
  assert(node->tok != NULL);

  if (opt_g)
    println("  .loc %d %d",
            node->tok->file->file_no,
            node->tok->line_no);

  switch (node->kind) {
  case ND_NULL_EXPR:
    return;

  case ND_NUM: {
    switch (node->ty->kind) {
    case TY_FLOAT: {
      union {
        float f32;
        uint32_t u32;
      } u = { (float)node->fval };

      /*
       * movd is considerably smaller than:
       *
       *   mov imm32, eax
       *   movq rax, xmm0
       */
      println("  mov $%u, %%eax", u.u32);
      println("  movd %%eax, %%xmm0");
      return;
    }

    case TY_DOUBLE: {
      union {
        double f64;
        uint64_t u64;
      } u = { (double)node->fval };

      println("  mov $%llu, %%rax",
              (unsigned long long)u.u64);
      println("  movq %%rax, %%xmm0");
      return;
    }

    case TY_LDOUBLE: {
      union {
        long double f80;
        uint64_t u64[2];
      } u;

      memset(&u, 0, sizeof(u));
      u.f80 = node->fval;

      println("  mov $%llu, %%rax",
              (unsigned long long)u.u64[0]);
      println("  mov %%rax, -16(%%rsp)");

      println("  mov $%llu, %%rax",
              (unsigned long long)u.u64[1]);
      println("  mov %%rax, -8(%%rsp)");

      println("  fldt -16(%%rsp)");
      return;
    }
    }

    if (node->val == 0) {
      println("  xor %%eax, %%eax");
      return;
    }

    if (node->val >= -2147483648LL &&
        node->val <= 2147483647LL) {
      /*
       * For a 32-bit integer this is also the ideal representation.
       * For a 64-bit signed integer the assembler's 32-bit immediate
       * form sign-extends when targeting rax.
       */
      if (node->ty->size <= 4)
        println("  mov $%lld, %%eax", (long long)node->val);
      else
        println("  mov $%lld, %%rax", (long long)node->val);
      return;
    }

    println("  mov $%lld, %%rax", (long long)node->val);
    return;
  }

  case ND_NEG:
    gen_expr(node->lhs, output_file);

    switch (node->ty->kind) {
    case TY_FLOAT:
      /*
       * 0x80000000 is directly representable by movd.
       */
      println("  mov $0x80000000, %%eax");
      println("  movd %%eax, %%xmm1");
      println("  xorps %%xmm1, %%xmm0");
      return;

    case TY_DOUBLE:
      println("  mov $0x8000000000000000, %%rax");
      println("  movq %%rax, %%xmm1");
      println("  xorpd %%xmm1, %%xmm0");
      return;

    case TY_LDOUBLE:
      println("  fchs");
      return;
    }

    if (node->ty->size <= 4)
      println("  neg %%eax");
    else
      println("  neg %%rax");

    return;

  case ND_VAR:
    /*
     * Fast path for scalar locals.
     */
    if (node->var &&
        node->var->is_local &&
        node->var->ty &&
        node->var->ty->kind != TY_VLA &&
        node->var->ty->kind != TY_ARRAY &&
        node->var->ty->kind != TY_STRUCT &&
        node->var->ty->kind != TY_UNION &&
        node->var->ty->kind != TY_FUNC) {

      Type *ty = node->ty ? node->ty : node->var->ty;

      if (ty->kind == TY_FLOAT) {
        println("  movss %d(%%rbp), %%xmm0",
                node->var->offset);
        return;
      }

      if (ty->kind == TY_DOUBLE) {
        println("  movsd %d(%%rbp), %%xmm0",
                node->var->offset);
        return;
      }

      if (ty->kind == TY_LDOUBLE) {
        println("  fldt %d(%%rbp)",
                node->var->offset);
        return;
      }

      char *insn = ty->is_unsigned ? "movz" : "movs";

      if (ty->size == 1)
        println("  %sbl %d(%%rbp), %%eax",
                insn, node->var->offset);
      else if (ty->size == 2)
        println("  %swl %d(%%rbp), %%eax",
                insn, node->var->offset);
      else if (ty->size == 4) {
        /*
         * Do NOT use movsxd for unsigned 32-bit values.
         */
        if (ty->is_unsigned)
          println("  mov %d(%%rbp), %%eax",
                  node->var->offset);
        else
          println("  movsxd %d(%%rbp), %%rax",
                  node->var->offset);
      } else {
        println("  mov %d(%%rbp), %%rax",
                node->var->offset);
      }

      return;
    }

    gen_addr(node, output_file);
    load(node->ty);
    return;

  case ND_MEMBER: {
    gen_addr(node, output_file);
    load(node->ty);

    Member *mem = node->member;

    if (mem->is_bitfield) {
      /*
       * Extraction is performed after load() has already extended the
       * underlying integer into rax.
       */
      if (mem->bit_width != 64)
        println("  shl $%d, %%rax",
                64 - mem->bit_width - mem->bit_offset);

      if (mem->ty->is_unsigned) {
        if (mem->bit_width != 64)
          println("  shr $%d, %%rax",
                  64 - mem->bit_width);
      } else {
        if (mem->bit_width != 64)
          println("  sar $%d, %%rax",
                  64 - mem->bit_width);
      }
    }

    return;
  }

  case ND_DEREF:
    gen_expr(node->lhs, output_file);
    load(node->ty);
    return;

  case ND_ADDR:
    gen_addr(node->lhs, output_file);
    return;

  case ND_ASSIGN:
    /*
     * Direct scalar local assignment.
     */
    if (node->lhs &&
        node->lhs->kind == ND_VAR &&
        node->lhs->var &&
        node->lhs->var->is_local &&
        node->lhs->var->ty &&
        node->lhs->var->ty->kind != TY_VLA &&
        node->lhs->var->ty->kind != TY_STRUCT &&
        node->lhs->var->ty->kind != TY_UNION &&
        node->lhs->var->ty->kind != TY_ARRAY) {

      gen_expr(node->rhs, output_file);

      Type *ty = node->lhs->ty
        ? node->lhs->ty
        : node->lhs->var->ty;

      if (ty) {
        if (ty->kind == TY_FLOAT)
          println("  movss %%xmm0, %d(%%rbp)",
                  node->lhs->var->offset);
        else if (ty->kind == TY_DOUBLE)
          println("  movsd %%xmm0, %d(%%rbp)",
                  node->lhs->var->offset);
        else if (ty->kind == TY_LDOUBLE)
          println("  fstpt %d(%%rbp)",
                  node->lhs->var->offset);
        else if (ty->size == 1)
          println("  mov %%al, %d(%%rbp)",
                  node->lhs->var->offset);
        else if (ty->size == 2)
          println("  mov %%ax, %d(%%rbp)",
                  node->lhs->var->offset);
        else if (ty->size == 4)
          println("  mov %%eax, %d(%%rbp)",
                  node->lhs->var->offset);
        else
          println("  mov %%rax, %d(%%rbp)",
                  node->lhs->var->offset);
      }

      return;
    }

    gen_addr(node->lhs, output_file);
    push();

    gen_expr(node->rhs, output_file);

    /*
     * Bitfield assignment requires a read/modify/write.
     */
    if (node->lhs &&
        node->lhs->kind == ND_MEMBER &&
        node->lhs->member &&
        node->lhs->member->is_bitfield) {

      Member *mem = node->lhs->member;

      /*
       * Preserve the value being assigned.
       */
      println("  mov %%rax, %%r8");

      /*
       * Extract only the low bit_width bits of the new value.
       *
       * Avoid (1L << 64), which is undefined in C.
       */
      uint64_t value_mask =
        mem->bit_width >= 64
          ? UINT64_MAX
          : ((UINT64_C(1) << mem->bit_width) - 1);

      uint64_t field_mask =
        mem->bit_width >= 64
          ? UINT64_MAX
          : (value_mask << mem->bit_offset);

      println("  mov %%rax, %%rdi");
      println("  and $%llu, %%rdi",
              (unsigned long long)value_mask);
      println("  shl $%d, %%rdi",
              mem->bit_offset);

      /*
       * Address is still sitting underneath the value on the stack.
       */
      println("  mov (%%rsp), %%rax");
      load(mem->ty);

      println("  mov $%llu, %%r9",
              (unsigned long long)~field_mask);
      println("  and %%r9, %%rax");
      println("  or %%rdi, %%rax");

      store(node->ty);

      /*
       * Assignment expression evaluates to the assigned value.
       */
      println("  mov %%r8, %%rax");
      return;
    }

    store(node->ty);
    return;

  case ND_STMT_EXPR:
    for (Node *n = node->body; n; n = n->next)
      gen_stmt(n, output_file);
    return;

  case ND_COMMA:
    gen_expr(node->lhs, output_file);
    gen_expr(node->rhs, output_file);
    return;

  case ND_CAST:
    gen_expr(node->lhs, output_file);

    if (node->lhs &&
        node->lhs->ty &&
        node->ty)
      cast(node->lhs->ty, node->ty);

    return;

  case ND_MEMZERO:
    println("  mov $%d, %%ecx", node->var->ty->size);
    println("  lea %d(%%rbp), %%rdi", node->var->offset);
    println("  xor %%eax, %%eax");
    println("  rep stosb");
    return;

  case ND_LABEL_VAL:
    println("  lea %s(%%rip), %%rax",
            node->unique_label);
    return;

  case ND_COND: {
    int c = count();

    gen_expr(node->cond, output_file);
    cmp_zero(node->cond ? node->cond->ty : NULL);

    println("  je .L.else.%d", c);

    gen_expr(node->then, output_file);

    println("  jmp .L.end.%d", c);
    println(".L.else.%d:", c);

    gen_expr(node->els, output_file);

    println(".L.end.%d:", c);
    return;
  }

  case ND_NOT:
    gen_expr(node->lhs, output_file);
    cmp_zero(node->lhs ? node->lhs->ty : NULL);

    println("  sete %%al");
    println("  movzbl %%al, %%eax");
    return;

  case ND_BITNOT:
    gen_expr(node->lhs, output_file);

    /*
     * The operation is performed at the type width.
     * Using not rax for uint32_t can corrupt the upper 32 bits.
     */
    if (node->lhs->ty && node->lhs->ty->size <= 4)
      println("  not %%eax");
    else
      println("  not %%rax");

    return;

  case ND_LOGAND: {
    int c = count();

    gen_expr(node->lhs, output_file);
    cmp_zero(node->lhs ? node->lhs->ty : NULL);

    println("  je .L.false.%d", c);

    gen_expr(node->rhs, output_file);
    cmp_zero(node->rhs ? node->rhs->ty : NULL);

    println("  je .L.false.%d", c);

    println("  mov $1, %%eax");
    println("  jmp .L.end.%d", c);

    println(".L.false.%d:", c);
    println("  xor %%eax, %%eax");

    println(".L.end.%d:", c);
    return;
  }

  case ND_LOGOR: {
    int c = count();

    gen_expr(node->lhs, output_file);
    cmp_zero(node->lhs ? node->lhs->ty : NULL);

    println("  jne .L.true.%d", c);

    gen_expr(node->rhs, output_file);
    cmp_zero(node->rhs ? node->rhs->ty : NULL);

    println("  jne .L.true.%d", c);

    println("  xor %%eax, %%eax");
    println("  jmp .L.end.%d", c);

    println(".L.true.%d:", c);
    println("  mov $1, %%eax");

    println(".L.end.%d:", c);
    return;
  }

  case ND_FUNCALL: {
    ABI *fn_abi = get_node_abi(node);

    if (node->lhs->kind == ND_VAR &&
        node->lhs->var &&
        !strcmp(node->lhs->var->name, "alloca")) {

      gen_expr(node->args, output_file);
      println("  mov %%rax, %%rdi");

      if (fn_abi && fn_abi->builtin_alloca)
        fn_abi->builtin_alloca(current_fn, output_file);

      return;
    }

    int stack = 0;

    if (fn_abi && fn_abi->push_args)
      stack = fn_abi->push_args(node, output_file, &depth);

    /*
     * Direct function call.
     *
     * Previously even:
     *
     *     foo()
     *
     * became:
     *
     *     lea foo(%rip), %rax
     *     mov %rax, %r11
     *     call *%r11
     *
     * A direct call is smaller, faster to decode, and gives the CPU a
     * proper direct-call target for branch prediction.
     *
     * Only ND_VAR function references use this path; arbitrary function
     * pointer expressions still use the indirect path.
     */
    bool direct_call =
      node->lhs &&
      node->lhs->kind == ND_VAR &&
      node->lhs->var &&
      node->lhs->var->is_function;

    if (direct_call) {
      if (fn_abi && fn_abi->pre_call)
        fn_abi->pre_call(node, output_file);

      println("  call %s", node->lhs->var->name);
    } else {
      gen_expr(node->lhs, output_file);
      println("  mov %%rax, %%r11");

      if (fn_abi && fn_abi->pre_call)
        fn_abi->pre_call(node, output_file);

      println("  call *%%r11");
    }

    if (stack > 0) {
      println("  add $%d, %%rsp", stack * 8);
      depth -= stack;
    }

    if (node->ty->kind == TY_BOOL) {
      println("  movzbl %%al, %%eax");
    } else if (node->ty->size == 1) {
      if (node->ty->is_unsigned)
        println("  movzbl %%al, %%eax");
      else
        println("  movsbl %%al, %%eax");
    } else if (node->ty->size == 2) {
      if (node->ty->is_unsigned)
        println("  movzwl %%ax, %%eax");
      else
        println("  movswl %%ax, %%eax");
    }

    if (node->ret_buffer &&
        fn_abi &&
        fn_abi->copy_ret_buffer) {

      if (!fn_abi->returns_by_reference(node->ty)) {
        fn_abi->copy_ret_buffer(node->ret_buffer, output_file);
        println("  lea %d(%%rbp), %%rax",
                node->ret_buffer->offset);
      }
    }

    return;
  }

  case ND_CAS: {
    gen_expr(node->cas_addr, output_file);
    push();

    gen_expr(node->cas_new, output_file);
    push();

    gen_expr(node->cas_old, output_file);
    push();

    int sz = node->cas_addr->ty->base->size;

    if (node->cas_old->ty->kind == TY_PTR)
      println("  mov (%%rax), %s", reg_ax(sz));

    pop("%r8");
    pop("%rdx");
    pop("%rdi");

    println("  lock cmpxchg %s, (%%rdi)", reg_dx(sz));
    println("  sete %%cl");

    if (node->cas_old->ty->kind == TY_PTR)
      println("  mov %s, (%%r8)", reg_ax(sz));

    println("  movzbl %%cl, %%eax");
    return;
  }

  case ND_EXCH: {
    gen_expr(node->lhs, output_file);
    push();

    gen_expr(node->rhs, output_file);
    pop("%rdi");

    int sz =
      (node->lhs &&
       node->lhs->ty &&
       node->lhs->ty->base)
        ? node->lhs->ty->base->size
        : 8;

    println("  xchg %s, (%%rdi)", reg_ax(sz));
    return;
  }
  }

  Type *lhs_ty =
    (node->lhs && node->lhs->ty)
      ? node->lhs->ty
      : ty_long;

  /*
   * Floating point binary operations.
   */
  switch (lhs_ty->kind) {
  case TY_FLOAT:
  case TY_DOUBLE: {
    /*
     * Keep the existing stack strategy here because an arbitrary RHS
     * expression may clobber xmm0.
     *
     * Simple local RHS operands get a direct-memory fast path below.
     */
    if (is_simple_local(node->rhs) &&
        node->rhs->var->ty->kind == lhs_ty->kind) {

      int offset = node->rhs->var->offset;
      char *sz = lhs_ty->kind == TY_FLOAT ? "ss" : "sd";

      gen_expr(node->lhs, output_file);

      switch (node->kind) {
      case ND_ADD:
        println("  add%s %d(%%rbp), %%xmm0",
                sz, offset);
        return;

      case ND_SUB:
        println("  sub%s %d(%%rbp), %%xmm0",
                sz, offset);
        return;

      case ND_MUL:
        println("  mul%s %d(%%rbp), %%xmm0",
                sz, offset);
        return;

      case ND_DIV:
        println("  div%s %d(%%rbp), %%xmm0",
                sz, offset);
        return;

      case ND_EQ:
      case ND_NE:
      case ND_LT:
      case ND_LE:
        /*
         * AT&T syntax is source,destination. This therefore compares
         * the value in xmm0 against the memory RHS in the intended
         * direction.
         */
        println("  ucomi%s %d(%%rbp), %%xmm0",
                sz, offset);

        if (node->kind == ND_EQ) {
          println("  sete %%al");
          println("  setnp %%dl");
          println("  and %%dl, %%al");
        } else if (node->kind == ND_NE) {
          println("  setne %%al");
          println("  setp %%dl");
          println("  or %%dl, %%al");
        } else if (node->kind == ND_LT) {
          println("  setb %%al");
        } else {
          println("  setbe %%al");
        }

        println("  movzbl %%al, %%eax");
        return;

      default:
        break;
      }
    }

    gen_expr(node->rhs, output_file);
    pushf();

    gen_expr(node->lhs, output_file);
    popf(1);

    char *sz =
      lhs_ty->kind == TY_FLOAT ? "ss" : "sd";

    switch (node->kind) {
    case ND_ADD:
      println("  add%s %%xmm1, %%xmm0", sz);
      return;

    case ND_SUB:
      println("  sub%s %%xmm1, %%xmm0", sz);
      return;

    case ND_MUL:
      println("  mul%s %%xmm1, %%xmm0", sz);
      return;

    case ND_DIV:
      println("  div%s %%xmm1, %%xmm0", sz);
      return;

    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
      println("  ucomi%s %%xmm0, %%xmm1", sz);

      if (node->kind == ND_EQ) {
        println("  sete %%al");
        println("  setnp %%dl");
        println("  and %%dl, %%al");
      } else if (node->kind == ND_NE) {
        println("  setne %%al");
        println("  setp %%dl");
        println("  or %%dl, %%al");
      } else if (node->kind == ND_LT) {
        println("  seta %%al");
      } else {
        println("  setae %%al");
      }

      println("  movzbl %%al, %%eax");
      return;
    }

    error_tok(node->tok, "invalid expression");
  }

  case TY_LDOUBLE: {
    gen_expr(node->lhs, output_file);
    gen_expr(node->rhs, output_file);

    switch (node->kind) {
    case ND_ADD:
      println("  faddp");
      return;

    case ND_SUB:
      println("  fsubrp");
      return;

    case ND_MUL:
      println("  fmulp");
      return;

    case ND_DIV:
      println("  fdivrp");
      return;

    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
      println("  fcomip");
      println("  fstp %%st(0)");

      if (node->kind == ND_EQ)
        println("  sete %%al");
      else if (node->kind == ND_NE)
        println("  setne %%al");
      else if (node->kind == ND_LT)
        println("  seta %%al");
      else
        println("  setae %%al");

      println("  movzbl %%al, %%eax");
      return;
    }

    error_tok(node->tok, "invalid expression");
  }
  }

  char *ax;
  char *di;
  char *dx;

  if (lhs_ty->size == 8 || lhs_ty->base) {
    ax = "%rax";
    di = "%rdi";
    dx = "%rdx";
  } else {
    ax = "%eax";
    di = "%edi";
    dx = "%edx";
  }

  /*
   * Fast path: immediate RHS.
   */
  if (is_simple_integer_imm(node->rhs)) {
    long long imm = (long long)node->rhs->val;

    switch (node->kind) {
    case ND_ADD:
      gen_expr(node->lhs, output_file);
      println("  add $%lld, %s", imm, ax);
      return;

    case ND_SUB:
      gen_expr(node->lhs, output_file);
      println("  sub $%lld, %s", imm, ax);
      return;

    case ND_MUL:
      gen_expr(node->lhs, output_file);
      println("  imul $%lld, %s", imm, ax);
      return;

    case ND_BITAND:
      gen_expr(node->lhs, output_file);
      println("  and $%lld, %s", imm, ax);
      return;

    case ND_BITOR:
      gen_expr(node->lhs, output_file);
      println("  or $%lld, %s", imm, ax);
      return;

    case ND_BITXOR:
      gen_expr(node->lhs, output_file);
      println("  xor $%lld, %s", imm, ax);
      return;

    case ND_SHL:
      gen_expr(node->lhs, output_file);

      /*
       * x << 0 is exactly x.
       */
      if (imm == 0)
        return;

      println("  shl $%lld, %s", imm, ax);
      return;

    case ND_SHR:
      gen_expr(node->lhs, output_file);

      if (imm == 0)
        return;

      if (lhs_ty->is_unsigned)
        println("  shr $%lld, %s", imm, ax);
      else
        println("  sar $%lld, %s", imm, ax);

      return;

    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
      gen_expr(node->lhs, output_file);
      println("  cmp $%lld, %s", imm, ax);

      if (node->kind == ND_EQ)
        println("  sete %%al");
      else if (node->kind == ND_NE)
        println("  setne %%al");
      else if (lhs_ty->is_unsigned)
        println("  %s %%al",
                node->kind == ND_LT ? "setb" : "setbe");
      else
        println("  %s %%al",
                node->kind == ND_LT ? "setl" : "setle");

      println("  movzbl %%al, %%eax");
      return;

    default:
      break;
    }
  }

  /*
   * Fast path: local RHS can be used directly as a memory operand.
   */
  if (is_simple_local(node->rhs) &&
      node->rhs->var->ty->size == lhs_ty->size) {

    int offset = node->rhs->var->offset;

    switch (node->kind) {
    case ND_ADD:
      gen_expr(node->lhs, output_file);
      println("  add %d(%%rbp), %s", offset, ax);
      return;

    case ND_SUB:
      gen_expr(node->lhs, output_file);
      println("  sub %d(%%rbp), %s", offset, ax);
      return;

    case ND_MUL:
      gen_expr(node->lhs, output_file);
      println("  imul %d(%%rbp), %s", offset, ax);
      return;

    case ND_BITAND:
      gen_expr(node->lhs, output_file);
      println("  and %d(%%rbp), %s", offset, ax);
      return;

    case ND_BITOR:
      gen_expr(node->lhs, output_file);
      println("  or %d(%%rbp), %s", offset, ax);
      return;

    case ND_BITXOR:
      gen_expr(node->lhs, output_file);
      println("  xor %d(%%rbp), %s", offset, ax);
      return;

    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
      gen_expr(node->lhs, output_file);

      println("  cmp %d(%%rbp), %s",
              offset, ax);

      if (node->kind == ND_EQ)
        println("  sete %%al");
      else if (node->kind == ND_NE)
        println("  setne %%al");
      else if (lhs_ty->is_unsigned)
        println("  %s %%al",
                node->kind == ND_LT ? "setb" : "setbe");
      else
        println("  %s %%al",
                node->kind == ND_LT ? "setl" : "setle");

      println("  movzbl %%al, %%eax");
      return;

    default:
      break;
    }
  }

  /*
   * Generic integer binary operation.
   *
   * This remains the fallback because arbitrary expressions may
   * overwrite rax.
   */
  gen_expr(node->rhs, output_file);
  push();

  gen_expr(node->lhs, output_file);
  pop("%rdi");

  switch (node->kind) {
  case ND_ADD:
    println("  add %s, %s", di, ax);
    return;

  case ND_SUB:
    println("  sub %s, %s", di, ax);
    return;

  case ND_MUL:
    println("  imul %s, %s", di, ax);
    return;

  case ND_DIV:
  case ND_MOD:
    if (node->ty && node->ty->is_unsigned) {
      /*
       * xor edx,edx is smaller than mov $0,edx.
       */
      println("  xor %%edx, %%edx");
      println("  div %s", di);
    } else {
      if (lhs_ty->size == 8)
        println("  cqo");
      else
        println("  cdq");

      println("  idiv %s", di);
    }

    if (node->kind == ND_MOD) {
      /*
       * For 32-bit modulo, writing eax from edx is sufficient and
       * avoids an unnecessary 64-bit move.
       */
      if (lhs_ty->size <= 4)
        println("  mov %%edx, %%eax");
      else
        println("  mov %%rdx, %%rax");
    }

    return;

  case ND_BITAND:
    println("  and %s, %s", di, ax);
    return;

  case ND_BITOR:
    println("  or %s, %s", di, ax);
    return;

  case ND_BITXOR:
    println("  xor %s, %s", di, ax);
    return;

  case ND_SHL:
    /*
     * Only cl is required by the x86 variable shift encoding.
     * For 32-bit values, use edi/ecx instead of rdi/rcx.
     */
    if (lhs_ty->size <= 4) {
      println("  mov %%edi, %%ecx");
      println("  shl %%cl, %%eax");
    } else {
      println("  mov %%rdi, %%rcx");
      println("  shl %%cl, %%rax");
    }
    return;

  case ND_SHR:
    if (lhs_ty->size <= 4) {
      println("  mov %%edi, %%ecx");

      if (lhs_ty->is_unsigned)
        println("  shr %%cl, %%eax");
      else
        println("  sar %%cl, %%eax");
    } else {
      println("  mov %%rdi, %%rcx");

      if (lhs_ty->is_unsigned)
        println("  shr %%cl, %%rax");
      else
        println("  sar %%cl, %%rax");
    }

    return;

  case ND_EQ:
  case ND_NE:
  case ND_LT:
  case ND_LE:
    println("  cmp %s, %s", di, ax);

    if (node->kind == ND_EQ) {
      println("  sete %%al");
    } else if (node->kind == ND_NE) {
      println("  setne %%al");
    } else if (lhs_ty->is_unsigned) {
      if (node->kind == ND_LT)
        println("  setb %%al");
      else
        println("  setbe %%al");
    } else {
      if (node->kind == ND_LT)
        println("  setl %%al");
      else
        println("  setle %%al");
    }

    println("  movzbl %%al, %%eax");
    return;
  }

  error_tok(node->tok, "invalid expression");
}

/*
 * Format GCC-style inline assembly constraints.
 *
 * The old implementation allocated a buffer every time and never freed
 * it. Keep the same API, but return NULL for the trivial case so the
 * caller knows whether it owns the returned buffer.
 */
static char *format_asm_str(char *s) {
  if (!s)
    return NULL;

  /*
   * Most asm strings don't contain substitutions. Avoid an allocation
   * entirely in that common case.
   */
  if (!strchr(s, '{'))
    return s;

  char *buf = calloc(1, strlen(s) + 1);
  char *d = buf;

  for (char *p = s; *p; p++) {
    if (*p == '{') {
      p++;

      while (*p && *p != '}' && *p != '|')
        *d++ = *p++;

      while (*p && *p != '}')
        p++;

      if (!*p)
        break;
    } else {
      *d++ = *p;
    }
  }

  *d = '\0';
  return buf;
}

static void gen_stmt(Node *node, FILE *out) {
  (void)out;

  if (!node)
    return;

  assert(node->tok != NULL);

  if (opt_g)
    println("  .loc %d %d",
            node->tok->file->file_no,
            node->tok->line_no);

  switch (node->kind) {
  case ND_IF: {
    int c = count();

    gen_expr(node->cond, output_file);
    cmp_zero(node->cond->ty);

    if (!node->els) {
      println("  je .L.end.%d", c);
      gen_stmt(node->then, output_file);
      println(".L.end.%d:", c);
      return;
    }

    println("  je .L.else.%d", c);
    gen_stmt(node->then, output_file);

    println("  jmp .L.end.%d", c);
    println(".L.else.%d:", c);

    gen_stmt(node->els, output_file);

    println(".L.end.%d:", c);
    return;
  }

  case ND_FOR: {
    int c = count();

    if (node->init)
      gen_stmt(node->init, output_file);

    println(".L.begin.%d:", c);

    if (node->cond) {
      gen_expr(node->cond, output_file);
      cmp_zero(node->cond->ty);
      println("  je %s", node->brk_label);
    }

    gen_stmt(node->then, output_file);

    println("%s:", node->cont_label);

    if (node->inc)
      gen_expr(node->inc, output_file);

    println("  jmp .L.begin.%d", c);
    println("%s:", node->brk_label);
    return;
  }

  case ND_DO: {
    int c = count();

    println(".L.begin.%d:", c);

    gen_stmt(node->then, output_file);

    println("%s:", node->cont_label);

    gen_expr(node->cond, output_file);
    cmp_zero(node->cond->ty);

    println("  jne .L.begin.%d", c);
    println("%s:", node->brk_label);
    return;
  }

  case ND_SWITCH:
    gen_expr(node->cond, output_file);

    for (Node *n = node->case_next; n; n = n->case_next) {
      char *ax =
        node->cond->ty->size == 8 ? "%rax" : "%eax";

      char *di =
        node->cond->ty->size == 8 ? "%rdi" : "%edi";

      if (n->begin == n->end) {
        println("  cmp $%ld, %s", n->begin, ax);
        println("  je %s", n->label);
        continue;
      }

      /*
       * GNU case range:
       *
       *   begin <= value <= end
       *
       * Normalize to:
       *
       *   0 <= value-begin <= end-begin
       */
      println("  mov %s, %s", ax, di);
      println("  sub $%ld, %s",
              n->begin, di);
      println("  cmp $%ld, %s",
              n->end - n->begin, di);
      println("  jbe %s", n->label);
    }

    if (node->default_case)
      println("  jmp %s",
              node->default_case->label);

    println("  jmp %s", node->brk_label);

    gen_stmt(node->then, output_file);

    println("%s:", node->brk_label);
    return;

  case ND_CASE:
    println("%s:", node->label);
    gen_stmt(node->lhs, output_file);
    return;

  case ND_BLOCK:
    for (Node *n = node->body; n; n = n->next)
      gen_stmt(n, output_file);
    return;

  case ND_GOTO:
    println("  jmp %s", node->unique_label);
    return;

  case ND_GOTO_EXPR:
    gen_expr(node->lhs, output_file);
    println("  jmp *%%rax");
    return;

  case ND_LABEL:
    println("%s:", node->unique_label);
    gen_stmt(node->lhs, output_file);
    return;

  case ND_RETURN:
    if (node->lhs) {
      gen_expr(node->lhs, output_file);

      Type *ty = node->lhs->ty;
      ABI *fn_abi = get_fn_abi(current_fn);
      if (fn_abi && fn_abi->emit_return)
        fn_abi->emit_return(current_fn, ty, output_file);
    }

    println("  jmp .L.return.%s",
            current_fn->name);
    return;

  case ND_EXPR_STMT:
    gen_expr(node->lhs, output_file);
    return;

  case ND_ASM: {
    char *asm_str = format_asm_str(node->asm_str);

    if (asm_str)
      println("  %s", asm_str);

    /*
     * format_asm_str() returns the original string when no formatting
     * was required, otherwise it returns an allocated buffer.
     */
    if (asm_str && asm_str != node->asm_str)
      free(asm_str);

    return;
  }
  }

  error_tok(node->tok, "invalid statement");
}

static void emit_data(Obj *prog, FILE *out) {
  (void)out;

  for (Obj *var = prog; var; var = var->next) {
    if (var->is_function || !var->is_definition)
      continue;

    if (current_objfmt &&
        current_objfmt->emit_var_decl)
      current_objfmt->emit_var_decl(var, output_file);
    else if (!var->is_static)
      println("  .globl %s", var->name);

    int align =
      (opt_fpic && var->ty->kind == TY_ARRAY &&
       var->ty->size >= 16)
        ? MAX(16, var->align)
        : var->align;

    /*
     * Common symbol.
     */
    if (opt_fcommon && var->is_tentative) {
      println("  .comm %s, %d, %d",
              var->name,
              var->ty->size,
              align);
      continue;
    }

    /*
     * .data or .tdata
     */
    if (var->init_data) {
      if (var->is_tls) {
        if (current_objfmt == &objfmt_coff)
          println("  .section .tls$");
        else
          println("  .section .tdata,\"awT\",@progbits");
      } else {
        println("  .data");
      }

      if (current_objfmt &&
          current_objfmt->emit_var_type_size)
        current_objfmt->emit_var_type_size(var, output_file);

      println("  .align %d", align);
      println("%s:", var->name);

      Relocation *rel = var->rel;
      int pos = 0;

      while (pos < var->ty->size) {
        if (rel && rel->offset == pos) {
          println("  .quad %s%+ld",
                  *rel->label,
                  rel->addend);

          rel = rel->next;
          pos += 8;
        } else {
          int end = rel ? rel->offset : var->ty->size;
          if (end > var->ty->size)
            end = var->ty->size;

          int z = pos;
          while (z < end && var->init_data[z] == 0)
            z++;
          if (z - pos >= 4) {
            println("  .zero %d", z - pos);
            pos = z;
            continue;
          }

          int count = end - pos;
          if (count > 16)
            count = 16;

          for (int k = 1; k < count; k++) {
            if (pos + k + 3 < end &&
                var->init_data[pos + k] == 0 &&
                var->init_data[pos + k + 1] == 0 &&
                var->init_data[pos + k + 2] == 0 &&
                var->init_data[pos + k + 3] == 0) {
              count = k;
              break;
            }
          }

          char buf[512];
          int off = sprintf(buf, "  .byte %d", (unsigned char)var->init_data[pos]);
          for (int i = 1; i < count; i++) {
            off += sprintf(buf + off, ", %d", (unsigned char)var->init_data[pos + i]);
          }
          println("%s", buf);
          pos += count;
        }
      }

      continue;
    }

    /*
     * .bss or .tbss
     */
    if (var->is_tls) {
      if (current_objfmt == &objfmt_coff)
        println("  .section .tls$");
      else
        println("  .section .tbss,\"awT\",@nobits");
    } else {
      println("  .bss");
    }

    println("  .align %d", align);
    println("%s:", var->name);
    println("  .zero %d", var->ty->size);
  }
}

static const char *x86_reg32(const char *r64) {
  if (!strcmp(r64, "%rax")) return "%eax";
  if (!strcmp(r64, "%rcx")) return "%ecx";
  if (!strcmp(r64, "%rdx")) return "%edx";
  if (!strcmp(r64, "%rbx")) return "%ebx";
  if (!strcmp(r64, "%rsi")) return "%esi";
  if (!strcmp(r64, "%rdi")) return "%edi";
  if (!strcmp(r64, "%rbp")) return "%ebp";
  if (!strcmp(r64, "%rsp")) return "%esp";
  if (!strcmp(r64, "%r8"))  return "%r8d";
  if (!strcmp(r64, "%r9"))  return "%r9d";
  if (!strcmp(r64, "%r10")) return "%r10d";
  if (!strcmp(r64, "%r11")) return "%r11d";
  if (!strcmp(r64, "%r12")) return "%r12d";
  if (!strcmp(r64, "%r13")) return "%r13d";
  if (!strcmp(r64, "%r14")) return "%r14d";
  if (!strcmp(r64, "%r15")) return "%r15d";
  return r64;
}

static const char *x86_reg16(const char *r64) {
  if (!strcmp(r64, "%rax")) return "%ax";
  if (!strcmp(r64, "%rcx")) return "%cx";
  if (!strcmp(r64, "%rdx")) return "%dx";
  if (!strcmp(r64, "%rbx")) return "%bx";
  if (!strcmp(r64, "%rsi")) return "%si";
  if (!strcmp(r64, "%rdi")) return "%di";
  if (!strcmp(r64, "%rbp")) return "%bp";
  if (!strcmp(r64, "%rsp")) return "%sp";
  if (!strcmp(r64, "%r8"))  return "%r8w";
  if (!strcmp(r64, "%r9"))  return "%r9w";
  if (!strcmp(r64, "%r10")) return "%r10w";
  if (!strcmp(r64, "%r11")) return "%r11w";
  if (!strcmp(r64, "%r12")) return "%r12w";
  if (!strcmp(r64, "%r13")) return "%r13w";
  if (!strcmp(r64, "%r14")) return "%r14w";
  if (!strcmp(r64, "%r15")) return "%r15w";
  return r64;
}

static const char *x86_reg8(const char *r64) {
  if (!strcmp(r64, "%rax")) return "%al";
  if (!strcmp(r64, "%rcx")) return "%cl";
  if (!strcmp(r64, "%rdx")) return "%dl";
  if (!strcmp(r64, "%rbx")) return "%bl";
  if (!strcmp(r64, "%rsi")) return "%sil";
  if (!strcmp(r64, "%rdi")) return "%dil";
  if (!strcmp(r64, "%rbp")) return "%bpl";
  if (!strcmp(r64, "%rsp")) return "%spl";
  if (!strcmp(r64, "%r8"))  return "%r8b";
  if (!strcmp(r64, "%r9"))  return "%r9b";
  if (!strcmp(r64, "%r10")) return "%r10b";
  if (!strcmp(r64, "%r11")) return "%r11b";
  if (!strcmp(r64, "%r12")) return "%r12b";
  if (!strcmp(r64, "%r13")) return "%r13b";
  if (!strcmp(r64, "%r14")) return "%r14b";
  if (!strcmp(r64, "%r15")) return "%r15b";
  return r64;
}

static LLIRVReg *cached_rax_vreg = NULL;

static void invalidate_cached_regs(void) {
  cached_rax_vreg = NULL;
}

static void load_vreg(LLIRVReg *v, const char *reg) {
  if (!v) return;
  if (!strcmp(reg, "%rax") && cached_rax_vreg == v)
    return;
  int offset = v->spill_offset ? v->spill_offset : -((v->id + 1) * 8);
  if (v->is_float) {
    if (reg[1] == 'x') { // %xmm...
      if (v->ty && v->ty->kind == TY_FLOAT)
        println("  movss %d(%%rbp), %s", offset, reg);
      else
        println("  movsd %d(%%rbp), %s", offset, reg);
    } else {
      if (v->ty && v->ty->kind == TY_FLOAT)
        println("  movl %d(%%rbp), %s", offset, x86_reg32(reg));
      else
        println("  movq %d(%%rbp), %s", offset, reg);
    }
    cached_rax_vreg = (!strcmp(reg, "%rax")) ? v : NULL;
  } else {
    int sz = v->ty ? v->ty->size : 8;
    if (reg[1] == 'x') { // %xmm...
      println("  movq %d(%%rbp), %s", offset, reg);
      cached_rax_vreg = NULL;
    } else if (sz == 1) {
      if (v->ty && v->ty->is_unsigned)
        println("  movzbl %d(%%rbp), %s", offset, x86_reg32(reg));
      else
        println("  movsbq %d(%%rbp), %s", offset, reg);
      cached_rax_vreg = (!strcmp(reg, "%rax")) ? v : NULL;
    } else if (sz == 2) {
      if (v->ty && v->ty->is_unsigned)
        println("  movzwl %d(%%rbp), %s", offset, x86_reg32(reg));
      else
        println("  movswq %d(%%rbp), %s", offset, reg);
      cached_rax_vreg = (!strcmp(reg, "%rax")) ? v : NULL;
    } else if (sz == 4) {
      if (v->ty && v->ty->is_unsigned)
        println("  movl %d(%%rbp), %s", offset, x86_reg32(reg));
      else
        println("  movslq %d(%%rbp), %s", offset, reg);
      cached_rax_vreg = (!strcmp(reg, "%rax")) ? v : NULL;
    } else {
      if (cached_rax_vreg == v && strcmp(reg, "%rax") != 0) {
        println("  movq %%rax, %s", reg);
      } else {
        println("  movq %d(%%rbp), %s", offset, reg);
        if (!strcmp(reg, "%rax"))
          cached_rax_vreg = v;
      }
    }
  }
}

static void store_vreg(const char *reg, LLIRVReg *v) {
  if (!v) return;
  if (current_needs_mat && !current_needs_mat[v->id]) {
    if (!strcmp(reg, "%rax"))
      cached_rax_vreg = v;
    else
      cached_rax_vreg = NULL;
    return;
  }
  int offset = v->spill_offset ? v->spill_offset : -((v->id + 1) * 8);
  if (v->is_float) {
    if (reg[1] == 'x') {
      if (v->ty && v->ty->kind == TY_FLOAT)
        println("  movss %s, %d(%%rbp)", reg, offset);
      else
        println("  movsd %s, %d(%%rbp)", reg, offset);
    } else {
      if (v->ty && v->ty->kind == TY_FLOAT)
        println("  movl %s, %d(%%rbp)", x86_reg32(reg), offset);
      else
        println("  movq %s, %d(%%rbp)", reg, offset);
    }
    cached_rax_vreg = NULL;
  } else {
    if (reg[1] == 'x') {
      println("  movq %s, %d(%%rbp)", reg, offset);
      cached_rax_vreg = NULL;
    } else {
      println("  movq %s, %d(%%rbp)", reg, offset);
      if (!strcmp(reg, "%rax"))
        cached_rax_vreg = v;
    }
  }
}

static int x86_int_size(Type *ty) {
  if (!ty)
    return 8;

  switch (ty->size) {
    case 1:
    case 2:
    case 4:
    case 8:
      return ty->size;
    default:
      return 8;
  }
}

static void x86_emit_set_bool(const char *cc) {
  println("  set%s %%al", cc);
  println("  movzbl %%al, %%eax");
}

static void x86_emit_int_unary(Type *ty, const char *op) {
  switch (x86_int_size(ty)) {
    case 1:
      println("  %s %%al", op);
      break;
    case 2:
      println("  %s %%ax", op);
      break;
    case 4:
      println("  %s %%eax", op);
      break;
    default:
      println("  %s %%rax", op);
      break;
  }
}

static void x86_emit_shift(Type *ty, const char *op) {
  int size = x86_int_size(ty);

  if (size == 1)
    println("  %s %%cl, %%al", op);
  else if (size == 2)
    println("  %s %%cl, %%ax", op);
  else if (size == 4)
    println("  %s %%cl, %%eax", op);
  else
    println("  %s %%cl, %%rax", op);
}

static void x86_64_emit_int_result(LLIRInsn *insn, const char *op) {
  load_vreg(insn->src1, "%rax");
  load_vreg(insn->src2, "%rdx");
  println("  %s %%rdx, %%rax", op);
  store_vreg("%rax", insn->dst);
}

static void x86_64_emit_float_binop(LLIRInsn *insn,
                                    const char *f32,
                                    const char *f64) {
  load_vreg(insn->src1, "%rax");
  load_vreg(insn->src2, "%rdx");
  println("  movq %%rax, %%xmm0");
  println("  movq %%rdx, %%xmm1");
  println("  %s %%xmm1, %%xmm0",
          insn->ty->kind == TY_FLOAT ? f32 : f64);
  println("  movq %%xmm0, %%rax");
  store_vreg("%rax", insn->dst);
}

static void x86_64_emit_binop(LLIRInsn *insn,
                              const char *iop,
                              const char *f32,
                              const char *f64) {
  if (insn->ty && is_flonum(insn->ty)) {
    x86_64_emit_float_binop(insn, f32, f64);
    return;
  }

  x86_64_emit_int_result(insn, iop);
}

static void x86_64_emit_cmp_result(const char *cc) {
  println("  set%s %%al", cc);
  println("  movzbl %%al, %%eax");
}

static void x86_64_emit_int_cmp(LLIRInsn *insn,
                                const char *signed_cc,
                                const char *unsigned_cc) {
  bool s1_imm = insn->src1 && insn->src1->def_insn && insn->src1->def_insn->kind == LLIR_IMM;
  bool s2_imm = insn->src2 && insn->src2->def_insn && insn->src2->def_insn->kind == LLIR_IMM;
  if (s1_imm && s2_imm) {
    int64_t v1 = insn->src1->def_insn->imm;
    int64_t v2 = insn->src2->def_insn->imm;
    if (v1 == 0)
      println("  xor %%eax, %%eax");
    else if (v1 > 0 && (uint64_t)v1 <= 0xFFFFFFFFULL)
      println("  mov $%u, %%eax", (uint32_t)v1);
    else if ((int64_t)(int32_t)v1 == v1)
      println("  mov $%lld, %%rax", (long long)v1);
    else
      println("  movabs $%lld, %%rax", (long long)v1);

    if (v2 == 0)
      println("  test %%rax, %%rax");
    else
      println("  cmp $%lld, %%rax", (long long)v2);
  } else if (s2_imm && (int32_t)insn->src2->def_insn->imm == insn->src2->def_insn->imm) {
    int64_t imm = insn->src2->def_insn->imm;
    load_vreg(insn->src1, "%rax");
    if (imm == 0)
      println("  test %%rax, %%rax");
    else
      println("  cmp $%lld, %%rax", (long long)imm);
  } else if (s1_imm && (int32_t)insn->src1->def_insn->imm == insn->src1->def_insn->imm) {
    int64_t imm = insn->src1->def_insn->imm;
    load_vreg(insn->src2, "%rax");
    if (imm == 0)
      println("  test %%rax, %%rax");
    else
      println("  cmp $%lld, %%rax", (long long)imm);

    bool uns = insn->src1 &&
               insn->src1->ty &&
               insn->src1->ty->is_unsigned;
    const char *cc = uns ? unsigned_cc : signed_cc;
    const char *swapped_cc = cc;
    if (!strcmp(cc, "l")) swapped_cc = "g";
    else if (!strcmp(cc, "le")) swapped_cc = "ge";
    else if (!strcmp(cc, "g")) swapped_cc = "l";
    else if (!strcmp(cc, "ge")) swapped_cc = "le";
    else if (!strcmp(cc, "b")) swapped_cc = "a";
    else if (!strcmp(cc, "be")) swapped_cc = "ae";
    else if (!strcmp(cc, "a")) swapped_cc = "b";
    else if (!strcmp(cc, "ae")) swapped_cc = "be";
    x86_64_emit_cmp_result(swapped_cc);
    store_vreg("%rax", insn->dst);
    return;
  } else {
    load_vreg(insn->src1, "%rax");
    load_vreg(insn->src2, "%rdx");
    println("  cmp %%rdx, %%rax");
  }

  bool uns = insn->src1 &&
             insn->src1->ty &&
             insn->src1->ty->is_unsigned;

  x86_64_emit_cmp_result(uns ? unsigned_cc : signed_cc);
  store_vreg("%rax", insn->dst);
}

static void x86_64_emit_float_cmp(LLIRInsn *insn, const char *cc) {
  load_vreg(insn->src1, "%rax");
  load_vreg(insn->src2, "%rdx");

  println("  movq %%rax, %%xmm0");
  println("  movq %%rdx, %%xmm1");

  if (insn->src1->ty->kind == TY_FLOAT)
    println("  ucomiss %%xmm1, %%xmm0");
  else
    println("  ucomisd %%xmm1, %%xmm0");

  x86_64_emit_cmp_result(cc);
  store_vreg("%rax", insn->dst);
}

static void x86_64_emit_float_eq(LLIRInsn *insn, bool ne) {
  load_vreg(insn->src1, "%rax");
  load_vreg(insn->src2, "%rdx");

  println("  movq %%rax, %%xmm0");
  println("  movq %%rdx, %%xmm1");

  if (insn->src1->ty->kind == TY_FLOAT)
    println("  ucomiss %%xmm1, %%xmm0");
  else
    println("  ucomisd %%xmm1, %%xmm0");

  if (ne) {
    println("  setne %%al");
    println("  setp %%dl");
    println("  or %%dl, %%al");
  } else {
    println("  sete %%al");
    println("  setnp %%dl");
    println("  and %%dl, %%al");
  }

  println("  movzbl %%al, %%eax");
  store_vreg("%rax", insn->dst);
}

static void x86_64_emit_cmp(LLIRInsn *insn,
                            const char *signed_cc,
                            const char *unsigned_cc,
                            const char *float_cc) {
  if (insn->src1 && insn->src1->ty &&
      is_flonum(insn->src1->ty)) {
    if (!strcmp(signed_cc, "e"))
      x86_64_emit_float_eq(insn, false);
    else if (!strcmp(signed_cc, "ne"))
      x86_64_emit_float_eq(insn, true);
    else
      x86_64_emit_float_cmp(insn, float_cc);
    return;
  }

  x86_64_emit_int_cmp(insn, signed_cc, unsigned_cc);
}

static void x86_64_emit_unary(LLIRInsn *insn, const char *op) {
  load_vreg(insn->src1, "%rax");
  println("  %s %%rax", op);
  store_vreg("%rax", insn->dst);
}

typedef struct {
  const char *base_reg;
  int offset;
  char buf[64];
} X86Addr;

static void x86_get_addr(LLIRVReg *addr_vreg, X86Addr *addr) {
  if (!addr_vreg) {
    addr->base_reg = "%rax";
    addr->offset = 0;
    sprintf(addr->buf, "(%%rax)");
    return;
  }

  // Case 1: addr_vreg is LEA of a local or global variable
  LLIRInsn *def = addr_vreg->def_insn;
  if (def && def->kind == LLIR_LEA) {
    if (def->var) {
      if (def->var->is_local) {
        addr->base_reg = "%rbp";
        addr->offset = def->var->offset;
        sprintf(addr->buf, "%d(%%rbp)", def->var->offset);
        return;
      } else {
        addr->base_reg = "%rip";
        addr->offset = 0;
        sprintf(addr->buf, "%s(%%rip)", def->var->name);
        return;
      }
    } else if (def->label) {
      addr->base_reg = "%rip";
      addr->offset = 0;
      sprintf(addr->buf, "%s(%%rip)", def->label);
      return;
    }
  }

  // Case 2: addr_vreg is ADD(base, IMM) or ADD(IMM, base)
  if (def && def->kind == LLIR_ADD && def->src1 && def->src2) {
    LLIRInsn *def1 = def->src1->def_insn;
    LLIRInsn *def2 = def->src2->def_insn;
    LLIRVReg *base = NULL;
    int64_t imm = 0;
    bool has_imm = false;

    if (def2 && def2->kind == LLIR_IMM) {
      base = def->src1;
      imm = def2->imm;
      has_imm = true;
    } else if (def1 && def1->kind == LLIR_IMM) {
      base = def->src2;
      imm = def1->imm;
      has_imm = true;
    }

    if (has_imm) {
      LLIRInsn *base_def = base->def_insn;
      if (base_def && base_def->kind == LLIR_LEA) {
        if (base_def->var) {
          if (base_def->var->is_local) {
            int total_off = base_def->var->offset + (int)imm;
            addr->base_reg = "%rbp";
            addr->offset = total_off;
            sprintf(addr->buf, "%d(%%rbp)", total_off);
            return;
          } else {
            addr->base_reg = "%rip";
            addr->offset = (int)imm;
            if (imm == 0)
              sprintf(addr->buf, "%s(%%rip)", base_def->var->name);
            else
              sprintf(addr->buf, "%s%+d(%%rip)", base_def->var->name, (int)imm);
            return;
          }
        }
      }
      load_vreg(base, "%rax");
      addr->base_reg = "%rax";
      addr->offset = (int)imm;
      if (imm == 0)
        sprintf(addr->buf, "(%%rax)");
      else
        sprintf(addr->buf, "%d(%%rax)", (int)imm);
      return;
    }
  }

  // Default: load address into %rax
  load_vreg(addr_vreg, "%rax");
  addr->base_reg = "%rax";
  addr->offset = 0;
  sprintf(addr->buf, "(%%rax)");
}

static void x86_64_emit_load(LLIRInsn *insn) {
  X86Addr addr;
  x86_get_addr(insn->src1, &addr);

  Type *ty = insn->ty;
  int sz = ty ? ty->size : 8;

  switch (sz) {
  case 1:
    if (ty && ty->is_unsigned)
      println("  movzbl %s, %%eax", addr.buf);
    else
      println("  movsbl %s, %%eax", addr.buf);
    break;

  case 2:
    if (ty && ty->is_unsigned)
      println("  movzwl %s, %%eax", addr.buf);
    else
      println("  movswl %s, %%eax", addr.buf);
    break;

  case 4:
    if (ty && ty->is_unsigned)
      println("  movl %s, %%eax", addr.buf);
    else
      println("  movslq %s, %%rax", addr.buf);
    break;

  default:
    println("  movq %s, %%rax", addr.buf);
    break;
  }

  store_vreg("%rax", insn->dst);
}

typedef enum {
  EIGHTBYTE_NONE,
  EIGHTBYTE_INT,
  EIGHTBYTE_FLOAT,
} EightbyteClass;

static void classify_struct_eightbytes(Type *ty, EightbyteClass *cls0, EightbyteClass *cls1) {
  *cls0 = EIGHTBYTE_NONE;
  *cls1 = EIGHTBYTE_NONE;
  if (!ty || (ty->kind != TY_STRUCT && ty->kind != TY_UNION)) {
    if (ty && is_flonum(ty))
      *cls0 = EIGHTBYTE_FLOAT;
    else if (ty)
      *cls0 = EIGHTBYTE_INT;
    return;
  }
  for (Member *mem = ty->members; mem; mem = mem->next) {
    if (mem->offset < 8) {
      if (!is_flonum(mem->ty))
        *cls0 = EIGHTBYTE_INT;
      else if (*cls0 != EIGHTBYTE_INT)
        *cls0 = EIGHTBYTE_FLOAT;
    } else if (mem->offset < 16) {
      if (!is_flonum(mem->ty))
        *cls1 = EIGHTBYTE_INT;
      else if (*cls1 != EIGHTBYTE_INT)
        *cls1 = EIGHTBYTE_FLOAT;
    }
  }
  if (*cls0 == EIGHTBYTE_NONE) *cls0 = EIGHTBYTE_INT;
  if (ty->size > 8 && *cls1 == EIGHTBYTE_NONE) *cls1 = EIGHTBYTE_INT;
}

static void x86_64_emit_store(LLIRInsn *insn) {
  X86Addr addr;
  if (insn->ty && (insn->ty->kind == TY_STRUCT || insn->ty->kind == TY_UNION)) {
    int src2_offset = insn->src2 ? (insn->src2->spill_offset ? insn->src2->spill_offset : -((insn->src2->id + 1) * 8)) : 0;
    if (insn->ty->size <= 8) {
      load_vreg(insn->src2, "%rcx");
      x86_get_addr(insn->src1, &addr);
      switch (insn->ty->size) {
      case 1: println("  movb %%cl, %s", addr.buf); break;
      case 2: println("  movw %%cx, %s", addr.buf); break;
      case 4: println("  movl %%ecx, %s", addr.buf); break;
      default: println("  movq %%rcx, %s", addr.buf); break;
      }
      return;
    } else if (insn->ty->size <= 16) {
      x86_get_addr(insn->src1, &addr);
      println("  movq %d(%%rbp), %%rcx", src2_offset);
      println("  movq %%rcx, %s", addr.buf);
      if (!strcmp(addr.base_reg, "%rbp")) {
        println("  movq %d(%%rbp), %%rcx", src2_offset + 8);
        println("  movq %%rcx, %d(%%rbp)", addr.offset + 8);
      } else {
        println("  movq %d(%%rbp), %%rcx", src2_offset + 8);
        println("  movq %%rcx, %d(%%rax)", addr.offset + 8);
      }
      return;
    }
  }
  if (insn->src2 && insn->src2->def_insn && insn->src2->def_insn->kind == LLIR_IMM &&
      (int32_t)insn->src2->def_insn->imm == insn->src2->def_insn->imm) {
    int64_t imm = insn->src2->def_insn->imm;
    x86_get_addr(insn->src1, &addr);
    switch (insn->ty ? insn->ty->size : 8) {
    case 1:
      println("  movb $%lld, %s", (long long)imm, addr.buf);
      return;
    case 2:
      println("  movw $%lld, %s", (long long)imm, addr.buf);
      return;
    case 4:
      println("  movl $%lld, %s", (long long)imm, addr.buf);
      return;
    default:
      println("  movq $%lld, %s", (long long)imm, addr.buf);
      return;
    }
  }

  load_vreg(insn->src2, "%rdx");
  x86_get_addr(insn->src1, &addr);

  switch (insn->ty ? insn->ty->size : 8) {
  case 1:
    println("  movb %%dl, %s", addr.buf);
    break;
  case 2:
    println("  movw %%dx, %s", addr.buf);
    break;
  case 4:
    println("  movl %%edx, %s", addr.buf);
    break;
  default:
    println("  movq %%rdx, %s", addr.buf);
    break;
  }
}

static const char *get_next_label(LLIRInsn *insn) {
  if (!insn) return NULL;
  for (LLIRInsn *nxt = insn->next; nxt; nxt = nxt->next) {
    if (nxt->kind == LLIR_NOP)
      continue;
    if (nxt->kind == LLIR_LABEL && nxt->label)
      return nxt->label;
    break;
  }
  return NULL;
}

static void emit_branch(LLIRInsn *insn, const char *cc, const char *inv_cc) {
  const char *next_lbl = get_next_label(insn);
  if (insn->label_true && insn->label_false) {
    if (next_lbl && !strcmp(insn->label_false, next_lbl)) {
      println("  j%s %s", cc, insn->label_true);
    } else if (next_lbl && !strcmp(insn->label_true, next_lbl)) {
      println("  j%s %s", inv_cc, insn->label_false);
    } else {
      println("  j%s %s", cc, insn->label_true);
      println("  jmp %s", insn->label_false);
    }
  } else if (insn->label_true) {
    if (!next_lbl || strcmp(insn->label_true, next_lbl) != 0)
      println("  j%s %s", cc, insn->label_true);
  } else if (insn->label_false) {
    if (!next_lbl || strcmp(insn->label_false, next_lbl) != 0)
      println("  j%s %s", inv_cc, insn->label_false);
  }
}

static void load_call_arg(LLIRVReg *arg, const char *reg) {
  if (!arg) return;
  if (arg->is_struct_val && arg->struct_size <= 8) {
    load_vreg(arg, "%rax");
    if (arg->struct_size == 1)
      println("  movzbl (%%rax), %%eax");
    else if (arg->struct_size == 2)
      println("  movzwl (%%rax), %%eax");
    else if (arg->struct_size == 4)
      println("  movl (%%rax), %%eax");
    else
      println("  movq (%%rax), %%rax");
    if (strcmp(reg, "%rax") != 0)
      println("  movq %%rax, %s", reg);
  } else {
    load_vreg(arg, reg);
  }
}

static void x86_64_gen_insn(LLIRInsn *insn, FILE *out) {
  if (!insn)
    return;

  (void)out;

  if (insn->dst && current_needs_mat && !current_needs_mat[insn->dst->id]) {
    switch (insn->kind) {
    case LLIR_IMM:
    case LLIR_FIMM:
    case LLIR_LEA:
    case LLIR_MOV:
    case LLIR_CAST:
    case LLIR_ADD:
    case LLIR_SUB:
    case LLIR_MUL:
    case LLIR_DIV:
    case LLIR_MOD:
    case LLIR_AND:
    case LLIR_OR:
    case LLIR_XOR:
    case LLIR_SHL:
    case LLIR_SHR:
    case LLIR_NEG:
    case LLIR_NOT:
    case LLIR_LOGNOT:
    case LLIR_CMP_EQ:
    case LLIR_CMP_NE:
    case LLIR_CMP_LT:
    case LLIR_CMP_LE:
    case LLIR_CMP_GT:
    case LLIR_CMP_GE:
      return;
    default:
      break;
    }
  }

  switch (insn->kind) {
  case LLIR_NOP:
  case LLIR_PHI:
    break;

  case LLIR_LABEL:
    invalidate_cached_regs();
    println("%s:", insn->label ? insn->label : "");
    break;

  case LLIR_JMP: {
    invalidate_cached_regs();
    const char *next_lbl = get_next_label(insn);
    if (insn->label && next_lbl && !strcmp(insn->label, next_lbl))
      break;
    println("  jmp %s", insn->label ? insn->label : "");
    break;
  }

  case LLIR_BR_COND: {
    LLIRInsn *def = insn->src1 ? insn->src1->def_insn : NULL;
    invalidate_cached_regs();

    if (def && (!def->src1 || !def->src1->ty || !is_flonum(def->src1->ty)) &&
        (def->kind == LLIR_CMP_EQ || def->kind == LLIR_CMP_NE ||
         def->kind == LLIR_CMP_LT || def->kind == LLIR_CMP_LE ||
         def->kind == LLIR_CMP_GT || def->kind == LLIR_CMP_GE)) {
      bool s1_imm = def->src1 && def->src1->def_insn && def->src1->def_insn->kind == LLIR_IMM;
      bool s2_imm = def->src2 && def->src2->def_insn && def->src2->def_insn->kind == LLIR_IMM;
      const char *cc = "e";
      const char *inv_cc = "ne";

      if (s1_imm && s2_imm) {
        int64_t v1 = def->src1->def_insn->imm;
        int64_t v2 = def->src2->def_insn->imm;
        if (v1 == 0)
          println("  xor %%eax, %%eax");
        else if (v1 > 0 && (uint64_t)v1 <= 0xFFFFFFFFULL)
          println("  mov $%u, %%eax", (uint32_t)v1);
        else if ((int64_t)(int32_t)v1 == v1)
          println("  mov $%lld, %%rax", (long long)v1);
        else
          println("  movabs $%lld, %%rax", (long long)v1);

        if (v2 == 0)
          println("  test %%rax, %%rax");
        else
          println("  cmp $%lld, %%rax", (long long)v2);

        bool uns = def->src1 && def->src1->ty && def->src1->ty->is_unsigned;
        switch (def->kind) {
        case LLIR_CMP_EQ: cc = "e";  inv_cc = "ne"; break;
        case LLIR_CMP_NE: cc = "ne"; inv_cc = "e";  break;
        case LLIR_CMP_LT: cc = uns ? "b" : "l";   inv_cc = uns ? "ae" : "ge"; break;
        case LLIR_CMP_LE: cc = uns ? "be" : "le"; inv_cc = uns ? "a" : "g";   break;
        case LLIR_CMP_GT: cc = uns ? "a" : "g";   inv_cc = uns ? "be" : "le"; break;
        case LLIR_CMP_GE: cc = uns ? "ae" : "ge"; inv_cc = uns ? "b" : "l";   break;
        default: break;
        }
      } else if (s2_imm && (int32_t)def->src2->def_insn->imm == def->src2->def_insn->imm) {
        int64_t imm = def->src2->def_insn->imm;
        load_vreg(def->src1, "%rax");
        if (imm == 0)
          println("  test %%rax, %%rax");
        else
          println("  cmp $%lld, %%rax", (long long)imm);

        bool uns = def->src1 && def->src1->ty && def->src1->ty->is_unsigned;
        switch (def->kind) {
        case LLIR_CMP_EQ: cc = "e";  inv_cc = "ne"; break;
        case LLIR_CMP_NE: cc = "ne"; inv_cc = "e";  break;
        case LLIR_CMP_LT: cc = uns ? "b" : "l";   inv_cc = uns ? "ae" : "ge"; break;
        case LLIR_CMP_LE: cc = uns ? "be" : "le"; inv_cc = uns ? "a" : "g";   break;
        case LLIR_CMP_GT: cc = uns ? "a" : "g";   inv_cc = uns ? "be" : "le"; break;
        case LLIR_CMP_GE: cc = uns ? "ae" : "ge"; inv_cc = uns ? "b" : "l";   break;
        default: break;
        }
      } else if (s1_imm && (int32_t)def->src1->def_insn->imm == def->src1->def_insn->imm) {
        int64_t imm = def->src1->def_insn->imm;
        load_vreg(def->src2, "%rax");
        if (imm == 0)
          println("  test %%rax, %%rax");
        else
          println("  cmp $%lld, %%rax", (long long)imm);

        bool uns = def->src1 && def->src1->ty && def->src1->ty->is_unsigned;
        switch (def->kind) {
        case LLIR_CMP_EQ: cc = "e";  inv_cc = "ne"; break;
        case LLIR_CMP_NE: cc = "ne"; inv_cc = "e";  break;
        case LLIR_CMP_LT: cc = uns ? "a" : "g";   inv_cc = uns ? "be" : "le"; break;
        case LLIR_CMP_LE: cc = uns ? "ae" : "ge"; inv_cc = uns ? "b" : "l";   break;
        case LLIR_CMP_GT: cc = uns ? "b" : "l";   inv_cc = uns ? "ae" : "ge"; break;
        case LLIR_CMP_GE: cc = uns ? "be" : "le"; inv_cc = uns ? "a" : "g";   break;
        default: break;
        }
      } else {
        load_vreg(def->src1, "%rax");
        load_vreg(def->src2, "%rdx");
        println("  cmp %%rdx, %%rax");

        bool uns = def->src1 && def->src1->ty && def->src1->ty->is_unsigned;
        switch (def->kind) {
        case LLIR_CMP_EQ: cc = "e";  inv_cc = "ne"; break;
        case LLIR_CMP_NE: cc = "ne"; inv_cc = "e";  break;
        case LLIR_CMP_LT: cc = uns ? "b" : "l";   inv_cc = uns ? "ae" : "ge"; break;
        case LLIR_CMP_LE: cc = uns ? "be" : "le"; inv_cc = uns ? "a" : "g";   break;
        case LLIR_CMP_GT: cc = uns ? "a" : "g";   inv_cc = uns ? "be" : "le"; break;
        case LLIR_CMP_GE: cc = uns ? "ae" : "ge"; inv_cc = uns ? "b" : "l";   break;
        default: break;
        }
      }

      emit_branch(insn, cc, inv_cc);
      break;
    }

    if (def && def->kind == LLIR_LOGNOT) {
      load_vreg(def->src1, "%rax");
      println("  test %%rax, %%rax");
      emit_branch(insn, "e", "ne");
      break;
    }

    load_vreg(insn->src1, "%rax");
    println("  test %%rax, %%rax");
    emit_branch(insn, "ne", "e");
    break;
  }

  case LLIR_IMM:
    if (insn->dst && current_needs_mat && !current_needs_mat[insn->dst->id])
      break;
    if (insn->imm == 0)
      println("  xor %%eax, %%eax");
    else if (insn->imm > 0 && (uint64_t)insn->imm <= 0xFFFFFFFFULL)
      println("  mov $%u, %%eax", (uint32_t)insn->imm);
    else if ((int64_t)(int32_t)insn->imm == insn->imm)
      println("  mov $%lld, %%rax", (long long)insn->imm);
    else
      println("  movabs $%lld, %%rax", (long long)insn->imm);

    store_vreg("%rax", insn->dst);
    break;

  case LLIR_FIMM: {
    if (insn->dst && current_needs_mat && !current_needs_mat[insn->dst->id])
      break;
    if (insn->ty && insn->ty->kind == TY_FLOAT) {
      union {
        float f;
        uint32_t u;
      } u = { .f = (float)insn->fimm };

      println("  mov $%u, %%eax", u.u);
      println("  movd %%eax, %%xmm0");
    } else {
      union {
        double d;
        uint64_t u;
      } u = { .d = insn->fimm };

      println("  movabs $%llu, %%rax",
              (unsigned long long)u.u);
      println("  movq %%rax, %%xmm0");
    }

    store_vreg("%xmm0", insn->dst);
    break;
  }

  case LLIR_LEA:
    if (insn->dst && current_needs_mat && !current_needs_mat[insn->dst->id])
      break;
    if (insn->var) {
      if (insn->var->is_local)
        println("  lea %d(%%rbp), %%rax", insn->var->offset);
      else
        println("  lea %s(%%rip), %%rax", insn->var->name);
    } else if (insn->label) {
      println("  lea %s(%%rip), %%rax", insn->label);
    }

    store_vreg("%rax", insn->dst);
    break;

  case LLIR_MOV:
    load_vreg(insn->src1, "%rax");
    store_vreg("%rax", insn->dst);
    break;

  case LLIR_CAST: {
    Type *from = insn->src1 ? insn->src1->ty : NULL;
    Type *to = insn->ty;

    load_vreg(insn->src1, "%rax");

    if (!from || !to || (from->size == to->size && from->is_unsigned == to->is_unsigned && is_flonum(from) == is_flonum(to))) {
      store_vreg("%rax", insn->dst);
      break;
    }

    if (to->kind == TY_BOOL) {
      println("  test %%rax, %%rax");
      x86_64_emit_cmp_result("ne");
      store_vreg("%rax", insn->dst);
      break;
    }

    if (is_flonum(from) && is_integer(to)) {
      println("  movq %%rax, %%xmm0");

      if (from->kind == TY_FLOAT)
        println("  cvttss2si %%xmm0, %%rax");
      else
        println("  cvttsd2si %%xmm0, %%rax");

      store_vreg("%rax", insn->dst);
      break;
    }

    if (is_integer(from) && is_flonum(to)) {
      if (to->kind == TY_FLOAT)
        println("  cvtsi2ss %%rax, %%xmm0");
      else
        println("  cvtsi2sd %%rax, %%xmm0");

      println("  movq %%xmm0, %%rax");
      store_vreg("%rax", insn->dst);
      break;
    }

    if (is_flonum(from) && is_flonum(to)) {
      println("  movq %%rax, %%xmm0");

      if (from->kind == TY_FLOAT && to->kind == TY_DOUBLE)
        println("  cvtss2sd %%xmm0, %%xmm0");
      else if (from->kind == TY_DOUBLE && to->kind == TY_FLOAT)
        println("  cvtsd2ss %%xmm0, %%xmm0");

      println("  movq %%xmm0, %%rax");
      store_vreg("%rax", insn->dst);
      break;
    }

    switch (to->size) {
    case 1:
      if (to->is_unsigned)
        println("  movzbl %%al, %%eax");
      else
        println("  movsbl %%al, %%eax");
      break;

    case 2:
      if (to->is_unsigned)
        println("  movzwl %%ax, %%eax");
      else
        println("  movswl %%ax, %%eax");
      break;

    case 4:
      if (to->is_unsigned)
        println("  movl %%eax, %%eax");
      else
        println("  cdqe");
      break;

    default:
      break;
    }

    store_vreg("%rax", insn->dst);
    break;
  }

  case LLIR_LOAD:
    x86_64_emit_load(insn);
    break;

  case LLIR_STORE:
    x86_64_emit_store(insn);
    break;

  case LLIR_ADD: {
    if (!insn->ty || !is_flonum(insn->ty)) {
      bool s1_imm = insn->src1 && insn->src1->def_insn && insn->src1->def_insn->kind == LLIR_IMM;
      bool s2_imm = insn->src2 && insn->src2->def_insn && insn->src2->def_insn->kind == LLIR_IMM;
      if (s1_imm && s2_imm) {
        int64_t val = insn->src1->def_insn->imm + insn->src2->def_insn->imm;
        if (val == 0)
          println("  xor %%eax, %%eax");
        else if (val > 0 && (uint64_t)val <= 0xFFFFFFFFULL)
          println("  mov $%u, %%eax", (uint32_t)val);
        else if ((int64_t)(int32_t)val == val)
          println("  mov $%lld, %%rax", (long long)val);
        else
          println("  movabs $%lld, %%rax", (long long)val);
        store_vreg("%rax", insn->dst);
        break;
      }
      LLIRVReg *imm_v = NULL;
      LLIRVReg *base = NULL;
      if (s2_imm) {
        imm_v = insn->src2; base = insn->src1;
      } else if (s1_imm) {
        imm_v = insn->src1; base = insn->src2;
      }
      if (imm_v && base && (int32_t)imm_v->def_insn->imm == imm_v->def_insn->imm) {
        int64_t imm = imm_v->def_insn->imm;
        load_vreg(base, "%rax");
        if (imm == 1) println("  inc %%rax");
        else if (imm == -1) println("  dec %%rax");
        else if (imm != 0) println("  add $%lld, %%rax", (long long)imm);
        store_vreg("%rax", insn->dst);
        break;
      }
    }
    x86_64_emit_binop(insn, "add", "addss", "addsd");
    break;
  }

  case LLIR_SUB: {
    if (!insn->ty || !is_flonum(insn->ty)) {
      bool s1_imm = insn->src1 && insn->src1->def_insn && insn->src1->def_insn->kind == LLIR_IMM;
      bool s2_imm = insn->src2 && insn->src2->def_insn && insn->src2->def_insn->kind == LLIR_IMM;
      if (s1_imm && s2_imm) {
        int64_t val = insn->src1->def_insn->imm - insn->src2->def_insn->imm;
        if (val == 0)
          println("  xor %%eax, %%eax");
        else if (val > 0 && (uint64_t)val <= 0xFFFFFFFFULL)
          println("  mov $%u, %%eax", (uint32_t)val);
        else if ((int64_t)(int32_t)val == val)
          println("  mov $%lld, %%rax", (long long)val);
        else
          println("  movabs $%lld, %%rax", (long long)val);
        store_vreg("%rax", insn->dst);
        break;
      }
      if (s2_imm && (int32_t)insn->src2->def_insn->imm == insn->src2->def_insn->imm) {
        int64_t imm = insn->src2->def_insn->imm;
        load_vreg(insn->src1, "%rax");
        if (imm == 1) println("  dec %%rax");
        else if (imm == -1) println("  inc %%rax");
        else if (imm != 0) println("  sub $%lld, %%rax", (long long)imm);
        store_vreg("%rax", insn->dst);
        break;
      }
    }
    x86_64_emit_binop(insn, "sub", "subss", "subsd");
    break;
  }

  case LLIR_MUL: {
    if (!insn->ty || !is_flonum(insn->ty)) {
      bool s1_imm = insn->src1 && insn->src1->def_insn && insn->src1->def_insn->kind == LLIR_IMM;
      bool s2_imm = insn->src2 && insn->src2->def_insn && insn->src2->def_insn->kind == LLIR_IMM;
      if (s1_imm && s2_imm) {
        int64_t val = insn->src1->def_insn->imm * insn->src2->def_insn->imm;
        if (val == 0)
          println("  xor %%eax, %%eax");
        else if (val > 0 && (uint64_t)val <= 0xFFFFFFFFULL)
          println("  mov $%u, %%eax", (uint32_t)val);
        else if ((int64_t)(int32_t)val == val)
          println("  mov $%lld, %%rax", (long long)val);
        else
          println("  movabs $%lld, %%rax", (long long)val);
        store_vreg("%rax", insn->dst);
        break;
      }
      LLIRVReg *imm_v = NULL;
      LLIRVReg *base = NULL;
      if (s2_imm) {
        imm_v = insn->src2; base = insn->src1;
      } else if (s1_imm) {
        imm_v = insn->src1; base = insn->src2;
      }
      if (imm_v && base && (int32_t)imm_v->def_insn->imm == imm_v->def_insn->imm) {
        int64_t imm = imm_v->def_insn->imm;
        if (imm == 0) {
          println("  xor %%eax, %%eax");
          store_vreg("%rax", insn->dst);
          break;
        }
        if (imm == 1) {
          load_vreg(base, "%rax");
          store_vreg("%rax", insn->dst);
          break;
        }
        if (imm == 2) {
          load_vreg(base, "%rax");
          println("  add %%rax, %%rax");
          store_vreg("%rax", insn->dst);
          break;
        }
        if (imm > 0 && (imm & (imm - 1)) == 0) {
          int shift = 0;
          while ((1LL << shift) < imm) shift++;
          load_vreg(base, "%rax");
          if (shift == 1)
            println("  shl $1, %%rax");
          else
            println("  shl $%d, %%rax", shift);
          store_vreg("%rax", insn->dst);
          break;
        }
        load_vreg(base, "%rax");
        println("  imul $%lld, %%rax, %%rax", (long long)imm);
        store_vreg("%rax", insn->dst);
        break;
      }
    }
    x86_64_emit_binop(insn, "imul", "mulss", "mulsd");
    break;
  }

  case LLIR_DIV:
    if (insn->ty && is_flonum(insn->ty)) {
      x86_64_emit_float_binop(insn, "divss", "divsd");
    } else {
      load_vreg(insn->src1, "%rax");
      load_vreg(insn->src2, "%rcx");

      if (insn->ty && insn->ty->size <= 4) {
        if (insn->ty->is_unsigned) {
          println("  xor %%edx, %%edx");
          println("  div %%ecx");
        } else {
          println("  cdq");
          println("  idiv %%ecx");
        }
      } else {
        if (insn->ty && insn->ty->is_unsigned) {
          println("  xor %%edx, %%edx");
          println("  div %%rcx");
        } else {
          println("  cqo");
          println("  idiv %%rcx");
        }
      }

      store_vreg("%rax", insn->dst);
    }
    break;

  case LLIR_MOD:
    load_vreg(insn->src1, "%rax");
    load_vreg(insn->src2, "%rcx");

    if (insn->ty && insn->ty->size <= 4) {
      if (insn->ty->is_unsigned) {
        println("  xor %%edx, %%edx");
        println("  div %%ecx");
      } else {
        println("  cdq");
        println("  idiv %%ecx");
      }
    } else {
      if (insn->ty && insn->ty->is_unsigned) {
        println("  xor %%edx, %%edx");
        println("  div %%rcx");
      } else {
        println("  cqo");
        println("  idiv %%rcx");
      }
    }

    store_vreg("%rdx", insn->dst);
    break;

  case LLIR_AND: {
    bool s1_imm = insn->src1 && insn->src1->def_insn && insn->src1->def_insn->kind == LLIR_IMM;
    bool s2_imm = insn->src2 && insn->src2->def_insn && insn->src2->def_insn->kind == LLIR_IMM;
    if (s1_imm && s2_imm) {
      int64_t val = insn->src1->def_insn->imm & insn->src2->def_insn->imm;
      if (val == 0)
        println("  xor %%eax, %%eax");
      else if (val > 0 && (uint64_t)val <= 0xFFFFFFFFULL)
        println("  mov $%u, %%eax", (uint32_t)val);
      else if ((int64_t)(int32_t)val == val)
        println("  mov $%lld, %%rax", (long long)val);
      else
        println("  movabs $%lld, %%rax", (long long)val);
      store_vreg("%rax", insn->dst);
      break;
    }
    LLIRVReg *imm_v = NULL;
    LLIRVReg *base = NULL;
    if (s2_imm) {
      imm_v = insn->src2; base = insn->src1;
    } else if (s1_imm) {
      imm_v = insn->src1; base = insn->src2;
    }
    if (imm_v && base && (int32_t)imm_v->def_insn->imm == imm_v->def_insn->imm) {
      int64_t imm = imm_v->def_insn->imm;
      if (imm == 0) {
        println("  xor %%eax, %%eax");
        store_vreg("%rax", insn->dst);
        break;
      }
      if (imm == -1) {
        load_vreg(base, "%rax");
        store_vreg("%rax", insn->dst);
        break;
      }
      load_vreg(base, "%rax");
      println("  and $%lld, %%rax", (long long)imm);
      store_vreg("%rax", insn->dst);
      break;
    }
    x86_64_emit_int_result(insn, "and");
    break;
  }

  case LLIR_OR: {
    bool s1_imm = insn->src1 && insn->src1->def_insn && insn->src1->def_insn->kind == LLIR_IMM;
    bool s2_imm = insn->src2 && insn->src2->def_insn && insn->src2->def_insn->kind == LLIR_IMM;
    if (s1_imm && s2_imm) {
      int64_t val = insn->src1->def_insn->imm | insn->src2->def_insn->imm;
      if (val == 0)
        println("  xor %%eax, %%eax");
      else if (val > 0 && (uint64_t)val <= 0xFFFFFFFFULL)
        println("  mov $%u, %%eax", (uint32_t)val);
      else if ((int64_t)(int32_t)val == val)
        println("  mov $%lld, %%rax", (long long)val);
      else
        println("  movabs $%lld, %%rax", (long long)val);
      store_vreg("%rax", insn->dst);
      break;
    }
    LLIRVReg *imm_v = NULL;
    LLIRVReg *base = NULL;
    if (s2_imm) {
      imm_v = insn->src2; base = insn->src1;
    } else if (s1_imm) {
      imm_v = insn->src1; base = insn->src2;
    }
    if (imm_v && base && (int32_t)imm_v->def_insn->imm == imm_v->def_insn->imm) {
      int64_t imm = imm_v->def_insn->imm;
      if (imm == 0) {
        load_vreg(base, "%rax");
        store_vreg("%rax", insn->dst);
        break;
      }
      load_vreg(base, "%rax");
      println("  or $%lld, %%rax", (long long)imm);
      store_vreg("%rax", insn->dst);
      break;
    }
    x86_64_emit_int_result(insn, "or");
    break;
  }

  case LLIR_XOR: {
    bool s1_imm = insn->src1 && insn->src1->def_insn && insn->src1->def_insn->kind == LLIR_IMM;
    bool s2_imm = insn->src2 && insn->src2->def_insn && insn->src2->def_insn->kind == LLIR_IMM;
    if (s1_imm && s2_imm) {
      int64_t val = insn->src1->def_insn->imm ^ insn->src2->def_insn->imm;
      if (val == 0)
        println("  xor %%eax, %%eax");
      else if (val > 0 && (uint64_t)val <= 0xFFFFFFFFULL)
        println("  mov $%u, %%eax", (uint32_t)val);
      else if ((int64_t)(int32_t)val == val)
        println("  mov $%lld, %%rax", (long long)val);
      else
        println("  movabs $%lld, %%rax", (long long)val);
      store_vreg("%rax", insn->dst);
      break;
    }
    LLIRVReg *imm_v = NULL;
    LLIRVReg *base = NULL;
    if (s2_imm) {
      imm_v = insn->src2; base = insn->src1;
    } else if (s1_imm) {
      imm_v = insn->src1; base = insn->src2;
    }
    if (imm_v && base && (int32_t)imm_v->def_insn->imm == imm_v->def_insn->imm) {
      int64_t imm = imm_v->def_insn->imm;
      if (imm == 0) {
        load_vreg(base, "%rax");
        store_vreg("%rax", insn->dst);
        break;
      }
      if (imm == -1) {
        load_vreg(base, "%rax");
        println("  not %%rax");
        store_vreg("%rax", insn->dst);
        break;
      }
      load_vreg(base, "%rax");
      println("  xor $%lld, %%rax", (long long)imm);
      store_vreg("%rax", insn->dst);
      break;
    }
    x86_64_emit_int_result(insn, "xor");
    break;
  }

  case LLIR_SHL: {
    bool s1_imm = insn->src1 && insn->src1->def_insn && insn->src1->def_insn->kind == LLIR_IMM;
    bool s2_imm = insn->src2 && insn->src2->def_insn && insn->src2->def_insn->kind == LLIR_IMM;
    if (s1_imm && s2_imm) {
      int64_t val = insn->src1->def_insn->imm << (insn->src2->def_insn->imm & 63);
      if (val == 0)
        println("  xor %%eax, %%eax");
      else if (val > 0 && (uint64_t)val <= 0xFFFFFFFFULL)
        println("  mov $%u, %%eax", (uint32_t)val);
      else if ((int64_t)(int32_t)val == val)
        println("  mov $%lld, %%rax", (long long)val);
      else
        println("  movabs $%lld, %%rax", (long long)val);
      store_vreg("%rax", insn->dst);
      break;
    }
    if (s2_imm) {
      int64_t imm = insn->src2->def_insn->imm & 63;
      load_vreg(insn->src1, "%rax");
      if (imm == 1)
        println("  shl $1, %%rax");
      else if (imm > 1)
        println("  shl $%lld, %%rax", (long long)imm);
      store_vreg("%rax", insn->dst);
      break;
    }
    load_vreg(insn->src1, "%rax");
    load_vreg(insn->src2, "%rcx");
    println("  shl %%cl, %%rax");
    store_vreg("%rax", insn->dst);
    break;
  }

  case LLIR_SHR: {
    bool s1_imm = insn->src1 && insn->src1->def_insn && insn->src1->def_insn->kind == LLIR_IMM;
    bool s2_imm = insn->src2 && insn->src2->def_insn && insn->src2->def_insn->kind == LLIR_IMM;
    if (s1_imm && s2_imm) {
      int64_t val;
      if (insn->ty && insn->ty->is_unsigned)
        val = (uint64_t)insn->src1->def_insn->imm >> (insn->src2->def_insn->imm & 63);
      else
        val = insn->src1->def_insn->imm >> (insn->src2->def_insn->imm & 63);
      if (val == 0)
        println("  xor %%eax, %%eax");
      else if (val > 0 && (uint64_t)val <= 0xFFFFFFFFULL)
        println("  mov $%u, %%eax", (uint32_t)val);
      else if ((int64_t)(int32_t)val == val)
        println("  mov $%lld, %%rax", (long long)val);
      else
        println("  movabs $%lld, %%rax", (long long)val);
      store_vreg("%rax", insn->dst);
      break;
    }
    if (s2_imm) {
      int64_t imm = insn->src2->def_insn->imm & 63;
      load_vreg(insn->src1, "%rax");
      if (insn->ty && insn->ty->is_unsigned) {
        if (imm == 1)
          println("  shr $1, %%rax");
        else if (imm > 1)
          println("  shr $%lld, %%rax", (long long)imm);
      } else {
        if (imm == 1)
          println("  sar $1, %%rax");
        else if (imm > 1)
          println("  sar $%lld, %%rax", (long long)imm);
      }
      store_vreg("%rax", insn->dst);
      break;
    }
    load_vreg(insn->src1, "%rax");
    load_vreg(insn->src2, "%rcx");

    if (insn->ty && insn->ty->is_unsigned)
      println("  shr %%cl, %%rax");
    else
      println("  sar %%cl, %%rax");

    store_vreg("%rax", insn->dst);
    break;
  }

  case LLIR_NEG:
    x86_64_emit_unary(insn, "neg");
    break;

  case LLIR_NOT:
    x86_64_emit_unary(insn, "not");
    break;

  case LLIR_LOGNOT:
    load_vreg(insn->src1, "%rax");
    println("  test %%rax, %%rax");
    x86_64_emit_cmp_result("e");
    store_vreg("%rax", insn->dst);
    break;

  case LLIR_CMP_EQ:
    x86_64_emit_cmp(insn, "e", "e", "e");
    break;

  case LLIR_CMP_NE:
    x86_64_emit_cmp(insn, "ne", "ne", "ne");
    break;

  case LLIR_CMP_LT:
    x86_64_emit_cmp(insn, "l", "b", "b");
    break;

  case LLIR_CMP_LE:
    x86_64_emit_cmp(insn, "le", "be", "be");
    break;

  case LLIR_CMP_GT:
    x86_64_emit_cmp(insn, "g", "a", "a");
    break;

  case LLIR_CMP_GE:
    x86_64_emit_cmp(insn, "ge", "ae", "ae");
    break;

  case LLIR_RET: {
    invalidate_cached_regs();
    if (insn->src1) {
      load_vreg(insn->src1, "%rax");

      if (insn->ty && is_flonum(insn->ty)) {
        println("  movq %%rax, %%xmm0");
      } else if (insn->ty &&
          (insn->ty->kind == TY_STRUCT ||
           insn->ty->kind == TY_UNION)) {
        ABI *fn_abi =
            (current_fn && current_fn->abi) ? current_fn->abi : current_abi;

        if (fn_abi && fn_abi->emit_return)
          fn_abi->emit_return(current_fn, insn->ty, output_file);
      }
    }

    LLIRInsn *nxt = insn->next;
    while (nxt && (nxt->kind == LLIR_NOP || nxt->kind == LLIR_LABEL))
      nxt = nxt->next;
    if (nxt)
      println("  jmp .L.return.%s", current_fn->name);
    break;
  }

  case LLIR_CALL: {
    invalidate_cached_regs();
    ABI *callee_abi = insn->call_abi ? insn->call_abi : ((current_fn && current_fn->abi) ? current_fn->abi : current_abi);
    if (callee_abi && callee_abi->emit_call) {
      callee_abi->emit_call(insn, output_file);
    }
    break;
  }

  case LLIR_ASM:
    invalidate_cached_regs();
    println("  %s", insn->asm_str ? insn->asm_str : "");
    break;

  case LLIR_ALLOCA:
    load_vreg(insn->src1, "%rax");
    println("  add $15, %%rax");
    println("  and $-16, %%rax");
    println("  sub %%rax, %%rsp");
    println("  mov %%rsp, %%rax");
    store_vreg("%rax", insn->dst);
    break;

  case LLIR_CAS: {
    load_vreg(insn->src1, "%rdi");
    load_vreg(insn->src2, "%r8");
    load_vreg(insn->src3, "%rdx");

    switch (insn->ty ? insn->ty->size : 8) {
    case 1:
      println("  movb (%%r8), %%al");
      println("  lock cmpxchgb %%dl, (%%rdi)");
      println("  sete %%cl");
      println("  movb %%al, (%%r8)");
      break;

    case 2:
      println("  movw (%%r8), %%ax");
      println("  lock cmpxchgw %%dx, (%%rdi)");
      println("  sete %%cl");
      println("  movw %%ax, (%%r8)");
      break;

    case 4:
      println("  movl (%%r8), %%eax");
      println("  lock cmpxchgl %%edx, (%%rdi)");
      println("  sete %%cl");
      println("  movl %%eax, (%%r8)");
      break;

    default:
      println("  movq (%%r8), %%rax");
      println("  lock cmpxchgq %%rdx, (%%rdi)");
      println("  sete %%cl");
      println("  movq %%rax, (%%r8)");
      break;
    }

    println("  movzbl %%cl, %%eax");

    if (insn->dst)
      store_vreg("%rax", insn->dst);

    break;
  }

  case LLIR_EXCH:
    load_vreg(insn->src1, "%rdi");
    load_vreg(insn->src2, "%rax");

    switch (insn->ty ? insn->ty->size : 8) {
    case 1:
      println("  xchgb %%al, (%%rdi)");
      break;
    case 2:
      println("  xchgw %%ax, (%%rdi)");
      break;
    case 4:
      println("  xchgl %%eax, (%%rdi)");
      break;
    default:
      println("  xchgq %%rax, (%%rdi)");
      break;
    }

    if (insn->dst)
      store_vreg("%rax", insn->dst);

    break;

  case LLIR_MEMCPY: {
    invalidate_cached_regs();
    int64_t sz = insn->imm;
    if (sz == 1) {
      load_vreg(insn->src2, "%rax");
      load_vreg(insn->src1, "%rdx");
      println("  movb (%%rax), %%cl");
      println("  movb %%cl, (%%rdx)");
    } else if (sz == 2) {
      load_vreg(insn->src2, "%rax");
      load_vreg(insn->src1, "%rdx");
      println("  movw (%%rax), %%cx");
      println("  movw %%cx, (%%rdx)");
    } else if (sz == 4) {
      load_vreg(insn->src2, "%rax");
      load_vreg(insn->src1, "%rdx");
      println("  movl (%%rax), %%ecx");
      println("  movl %%ecx, (%%rdx)");
    } else if (sz == 8) {
      load_vreg(insn->src2, "%rax");
      load_vreg(insn->src1, "%rdx");
      println("  movq (%%rax), %%rcx");
      println("  movq %%rcx, (%%rdx)");
    } else if (sz == 16) {
      load_vreg(insn->src2, "%rax");
      load_vreg(insn->src1, "%rdx");
      println("  movq (%%rax), %%rcx");
      println("  movq %%rcx, (%%rdx)");
      println("  movq 8(%%rax), %%rcx");
      println("  movq %%rcx, 8(%%rdx)");
    } else {
      load_vreg(insn->src1, "%rdi");
      load_vreg(insn->src2, "%rsi");
      if ((uint64_t)insn->imm <= 0xFFFFFFFFULL)
        println("  mov $%u, %%ecx", (uint32_t)insn->imm);
      else
        println("  mov $%lld, %%rcx", (long long)insn->imm);
      println("  rep movsb");
    }
    break;
  }

  case LLIR_MEMZERO: {
    invalidate_cached_regs();
    int64_t sz = insn->imm;
    load_vreg(insn->src1, "%rax");
    if (sz == 1) {
      println("  movb $0, (%%rax)");
    } else if (sz == 2) {
      println("  movw $0, (%%rax)");
    } else if (sz == 4) {
      println("  movl $0, (%%rax)");
    } else if (sz == 8) {
      println("  movq $0, (%%rax)");
    } else if (sz == 16) {
      println("  movq $0, (%%rax)");
      println("  movq $0, 8(%%rax)");
    } else {
      println("  mov %%rax, %%rdi");
      println("  xor %%eax, %%eax");
      if ((uint64_t)insn->imm <= 0xFFFFFFFFULL)
        println("  mov $%u, %%ecx", (uint32_t)insn->imm);
      else
        println("  mov $%lld, %%rcx", (long long)insn->imm);
      println("  rep stosb");
    }
    break;
  }

  default:
    break;
  }
}

static void x86_64_gen_expr(LLIRInsn *insn, FILE *out) {
  x86_64_gen_insn(insn, out);
}

static void emit_text(LLIRProg *prog, FILE *out) {
  (void)out;

  if (!prog || !prog->fns)
    return;

  for (int i = 0; i < prog->num_fns; i++) {
    LLIRFunction *fn = prog->fns[i];
    Obj *fn_obj = fn->fn_obj;
    if (!fn_obj || !fn_obj->is_function || !fn_obj->is_definition || !fn_obj->is_live)
      continue;

    if (current_objfmt && current_objfmt->emit_fn_decl)
      current_objfmt->emit_fn_decl(fn_obj, output_file);
    else if (!fn_obj->is_static)
      println("  .globl %s", fn->name);

    println("  .text");

    if (current_objfmt && current_objfmt->emit_fn_type)
      current_objfmt->emit_fn_type(fn_obj, output_file);

    println("%s:", fn->name);

    current_fn = fn_obj;

    ABI *fn_abi = fn->abi ? fn->abi : get_fn_abi(fn_obj);

    if (fn_abi && fn_abi->emit_prologue)
      fn_abi->emit_prologue(fn_obj, output_file);

    compute_materialization(fn);
    invalidate_cached_regs();

    // Assemble LLIR instructions directly from LLIRFunction
    for (LLIRInsn *insn = fn->head; insn; insn = insn->next) {
      x86_64_gen_insn(insn, output_file);
    }

    if (strcmp(fn->name, "main") == 0)
      println("  xor %%eax, %%eax");

    if (fn_abi && fn_abi->emit_epilogue)
      fn_abi->emit_epilogue(fn_obj, output_file);
  }
}

static void x86_64_init(FILE *out) {
  output_file = out;
  depth = 0;
}

static void x86_64_codegen(LLIRProg *prog, FILE *out) {
  output_file = out;
  depth = 0;

  if (opt_g) {
    File **files = get_input_files();

    for (int i = 0; files[i]; i++) {
      char *name = strdup(files[i]->name);

      for (char *p = name; *p; p++) {
        if (*p == '\\')
          *p = '/';
      }

      println("  .file %d \"%s\"",
              files[i]->file_no,
              name);

      free(name);
    }
  }

  /*
   * Reset local offsets before the ABI assigns them.
   */
  for (Obj *fn = prog->globals; fn; fn = fn->next)
  {
    if (!fn->is_function)
      continue;

    for (Obj *v = fn->locals; v; v = v->next)
      v->offset = 0;

    for (Obj *v = fn->params; v; v = v->next)
      v->offset = 0;
  }

  /*
   * ABI-specific local/parameter layout.
   */
  for (Obj *fn = prog->globals; fn; fn = fn->next) {
    if (!fn->is_function || !fn->is_definition)
      continue;

    const ABI *fn_abi = get_fn_abi(fn);

    if (fn_abi && fn_abi->assign_lvar_offsets)
      fn_abi->assign_lvar_offsets(fn);
  }

  // Allocate stack spill offsets for all LLIR virtual registers
  regalloc_prog((IRProg *)prog, NULL);

  emit_data(prog->globals, out);
  emit_text(prog, out);
}

static void x86_64_codegen_llir(LLIRProg *prog, FILE *out) {
  if (prog)
    x86_64_codegen(prog, out);
}

Codegen codegen_x86_64 =
{
  .name = "x86_64",
  .description = "x86-64 code generator",
  .default_abi_name = "sysv64",
  .init = x86_64_init,
  .codegen_llir = x86_64_codegen_llir,
  .emit_data = emit_data,
  .emit_text = emit_text,
  .gen_insn = x86_64_gen_insn,
  .gen_expr = x86_64_gen_expr,
};