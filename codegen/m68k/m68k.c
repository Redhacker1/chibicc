#include "chibicc.h"
#include "codegen/codegen.h"
#include "codegen/common/common.h"
#include "ir/ir.h"

static FILE *output_file;
static int depth;
static Obj *current_fn;

static void gen_expr(Node *node, FILE *out);
static void gen_stmt(Node *node, FILE *out);

__attribute__((format(printf, 1, 2)))
static void println(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(output_file, fmt, ap);
  va_end(ap);
  fprintf(output_file, "\n");
}

static int count(void) {
  return codegen_label_count();
}

static void push(void) {
  println("  move.l %%d0, -(%%sp)");
  depth++;
}

static void pop(char *arg) {
  println("  move.l (%%sp)+, %s", arg);
  depth--;
}

static void gen_addr(Node *node, FILE *out) {
  (void)out;
  switch (node->kind) {
  case ND_VAR:
    if (node->var->is_local) {
      println("  lea %d(%%a6), %%a0", node->var->offset);
      return;
    }
    println("  lea %s, %%a0", node->var->name);
    return;
  case ND_DEREF:
    gen_expr(node->lhs, output_file);
    println("  movea.l %%d0, %%a0");
    return;
  case ND_MEMBER:
    gen_addr(node->lhs, output_file);
    println("  adda.w #%d, %%a0", node->member->offset);
    return;
  case ND_COMMA:
    gen_expr(node->lhs, output_file);
    gen_addr(node->rhs, output_file);
    return;
  default:
    break;
  }
  error_tok(node->tok, "not an lvalue");
}

static void load(Type *ty) {
  switch (ty->kind) {
  case TY_ARRAY:
  case TY_STRUCT:
  case TY_UNION:
  case TY_FUNC:
  case TY_VLA:
    println("  move.l %%a0, %%d0");
    return;
  default:
    break;
  }

  if (ty->size == 1) {
    if (ty->is_unsigned) {
      println("  clr.l %%d0");
      println("  move.b (%%a0), %%d0");
    } else {
      println("  move.b (%%a0), %%d0");
      println("  extb.l %%d0");
    }
  } else if (ty->size == 2) {
    if (ty->is_unsigned) {
      println("  clr.l %%d0");
      println("  move.w (%%a0), %%d0");
    } else {
      println("  move.w (%%a0), %%d0");
      println("  ext.l %%d0");
    }
  } else {
    println("  move.l (%%a0), %%d0");
  }
}

static void store(Type *ty) {
  pop("%a0");

  if (ty->size == 1)
    println("  move.b %%d0, (%%a0)");
  else if (ty->size == 2)
    println("  move.w %%d0, (%%a0)");
  else
    println("  move.l %%d0, (%%a0)");
}

static void gen_expr(Node *node, FILE *out) {
  (void)out;
  switch (node->kind) {
  case ND_NULL_EXPR:
    return;
  case ND_NUM:
    println("  move.l #%ld, %%d0", node->val);
    return;
  case ND_NEG:
    gen_expr(node->lhs, output_file);
    println("  neg.l %%d0");
    return;
  case ND_VAR:
    if (node->var && node->var->is_local && node->var->ty &&
        node->var->ty->kind != TY_ARRAY && node->var->ty->kind != TY_STRUCT &&
        node->var->ty->kind != TY_UNION && node->var->ty->kind != TY_VLA &&
        node->var->ty->kind != TY_FUNC) {
      Type *ty = node->ty ? node->ty : node->var->ty;
      if (ty->size == 1) {
        if (ty->is_unsigned) {
          println("  clr.l %%d0");
          println("  move.b %d(%%a6), %%d0", node->var->offset);
        } else {
          println("  move.b %d(%%a6), %%d0", node->var->offset);
          println("  extb.l %%d0");
        }
      } else if (ty->size == 2) {
        if (ty->is_unsigned) {
          println("  clr.l %%d0");
          println("  move.w %d(%%a6), %%d0", node->var->offset);
        } else {
          println("  move.w %d(%%a6), %%d0", node->var->offset);
          println("  ext.l %%d0");
        }
      } else {
        println("  move.l %d(%%a6), %%d0", node->var->offset);
      }
      return;
    }
    gen_addr(node, output_file);
    load(node->ty);
    return;
  case ND_MEMBER:
    gen_addr(node, output_file);
    load(node->ty);
    return;
  case ND_DEREF:
    gen_expr(node->lhs, output_file);
    println("  movea.l %%d0, %%a0");
    load(node->ty);
    return;
  case ND_ADDR:
    gen_addr(node->lhs, output_file);
    println("  move.l %%a0, %%d0");
    return;
  case ND_ASSIGN:
    if (node->lhs && node->lhs->kind == ND_VAR && node->lhs->var && node->lhs->var->is_local &&
        node->lhs->var->ty && node->lhs->var->ty->kind != TY_VLA &&
        node->lhs->var->ty->kind != TY_STRUCT && node->lhs->var->ty->kind != TY_UNION &&
        node->lhs->var->ty->kind != TY_ARRAY) {
      gen_expr(node->rhs, output_file);
      Type *ty = node->lhs->ty ? node->lhs->ty : node->lhs->var->ty;
      if (ty->size == 1)
        println("  move.b %%d0, %d(%%a6)", node->lhs->var->offset);
      else if (ty->size == 2)
        println("  move.w %%d0, %d(%%a6)", node->lhs->var->offset);
      else
        println("  move.l %%d0, %d(%%a6)", node->lhs->var->offset);
      return;
    }
    gen_addr(node->lhs, output_file);
    println("  move.l %%a0, -(%%sp)");
    depth++;
    gen_expr(node->rhs, output_file);
    store(node->ty);
    return;
  case ND_FUNCALL: {
    int stack = 0;
    if (current_abi && current_abi->push_args)
      stack = current_abi->push_args(node, output_file, &depth);

    if (node->lhs->kind == ND_VAR)
      println("  jsr %s", node->lhs->var->name);
    else {
      gen_expr(node->lhs, output_file);
      println("  movea.l %%d0, %%a0");
      println("  jsr (%%a0)");
    }

    if (stack > 0) {
      println("  adda.w #%d, %%sp", stack * 2);
      depth -= stack;
    }
    return;
  }
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
    return;
  case ND_COND: {
    int c = count();
    gen_expr(node->cond, output_file);
    println("  tst.l %%d0");
    println("  beq .L.else.%d", c);
    gen_expr(node->then, output_file);
    println("  bra .L.end.%d", c);
    println(".L.else.%d:", c);
    gen_expr(node->els, output_file);
    println(".L.end.%d:", c);
    return;
  }
  case ND_NOT:
    gen_expr(node->lhs, output_file);
    println("  tst.l %%d0");
    println("  seq %%d0");
    println("  ext.w %%d0");
    println("  ext.l %%d0");
    return;
  case ND_BITNOT:
    gen_expr(node->lhs, output_file);
    println("  not.l %%d0");
    return;
  default:
    break;
  }

  if (node->rhs && node->rhs->kind == ND_NUM) {
    long val = (long)node->rhs->val;
    switch (node->kind) {
    case ND_ADD:
      gen_expr(node->lhs, output_file);
      println("  addi.l #%ld, %%d0", val);
      return;
    case ND_SUB:
      gen_expr(node->lhs, output_file);
      println("  subi.l #%ld, %%d0", val);
      return;
    case ND_BITAND:
      gen_expr(node->lhs, output_file);
      println("  andi.l #%ld, %%d0", val);
      return;
    case ND_BITOR:
      gen_expr(node->lhs, output_file);
      println("  ori.l #%ld, %%d0", val);
      return;
    case ND_BITXOR:
      gen_expr(node->lhs, output_file);
      println("  eori.l #%ld, %%d0", val);
      return;
    case ND_EQ:
      gen_expr(node->lhs, output_file);
      println("  cmpi.l #%ld, %%d0", val);
      println("  seq %%d0");
      println("  andi.l #1, %%d0");
      return;
    case ND_NE:
      gen_expr(node->lhs, output_file);
      println("  cmpi.l #%ld, %%d0", val);
      println("  sne %%d0");
      println("  andi.l #1, %%d0");
      return;
    case ND_LT:
      gen_expr(node->lhs, output_file);
      println("  cmpi.l #%ld, %%d0", val);
      println("  slt %%d0");
      println("  andi.l #1, %%d0");
      return;
    case ND_LE:
      gen_expr(node->lhs, output_file);
      println("  cmpi.l #%ld, %%d0", val);
      println("  sle %%d0");
      println("  andi.l #1, %%d0");
      return;
    default:
      break;
    }
  }

  gen_expr(node->rhs, output_file);
  push();
  gen_expr(node->lhs, output_file);
  pop("%d1");

  switch (node->kind) {
  case ND_ADD:
    println("  add.l %%d1, %%d0");
    return;
  case ND_SUB:
    println("  sub.l %%d1, %%d0");
    return;
  case ND_MUL:
    println("  muls.w %%d1, %%d0");
    return;
  case ND_DIV:
    println("  divs.w %%d1, %%d0");
    println("  ext.l %%d0");
    return;
  case ND_MOD:
    println("  divs.w %%d1, %%d0");
    println("  swap %%d0");
    println("  ext.l %%d0");
    return;
  case ND_BITAND:
    println("  and.l %%d1, %%d0");
    return;
  case ND_BITOR:
    println("  or.l %%d1, %%d0");
    return;
  case ND_BITXOR:
    println("  eor.l %%d1, %%d0");
    return;
  case ND_SHL:
    println("  asl.l %%d1, %%d0");
    return;
  case ND_SHR:
    println("  asr.l %%d1, %%d0");
    return;
  case ND_EQ:
    println("  cmp.l %%d1, %%d0");
    println("  seq %%d0");
    println("  andi.l #1, %%d0");
    return;
  case ND_NE:
    println("  cmp.l %%d1, %%d0");
    println("  sne %%d0");
    println("  andi.l #1, %%d0");
    return;
  case ND_LT:
    println("  cmp.l %%d1, %%d0");
    println("  slt %%d0");
    println("  andi.l #1, %%d0");
    return;
  case ND_LE:
    println("  cmp.l %%d1, %%d0");
    println("  sle %%d0");
    println("  andi.l #1, %%d0");
    return;
  default:
    break;
  }

  error_tok(node->tok, "invalid expression");
}

static void gen_stmt(Node *node, FILE *out) {
  (void)out;
  switch (node->kind) {
  case ND_IF: {
    int c = count();
    gen_expr(node->cond, output_file);
    println("  tst.l %%d0");
    if (!node->els) {
      println("  beq .L.end.%d", c);
      gen_stmt(node->then, output_file);
      println(".L.end.%d:", c);
      return;
    }
    println("  beq .L.else.%d", c);
    gen_stmt(node->then, output_file);
    println("  bra .L.end.%d", c);
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
      println("  tst.l %%d0");
      println("  beq %s", node->brk_label);
    }
    gen_stmt(node->then, output_file);
    println("%s:", node->cont_label);
    if (node->inc)
      gen_expr(node->inc, output_file);
    println("  bra .L.begin.%d", c);
    println("%s:", node->brk_label);
    return;
  }
  case ND_BLOCK:
    for (Node *n = node->body; n; n = n->next)
      gen_stmt(n, output_file);
    return;
  case ND_GOTO:
    println("  bra %s", node->unique_label);
    return;
  case ND_LABEL:
    println("%s:", node->unique_label);
    gen_stmt(node->lhs, output_file);
    return;
  case ND_RETURN:
    if (node->lhs) {
      gen_expr(node->lhs, output_file);
      ABI *fn_abi = (current_fn && current_fn->abi) ? current_fn->abi : current_abi;
      if (fn_abi && fn_abi->emit_return)
        fn_abi->emit_return(current_fn, node->lhs->ty, output_file);
    }
    println("  bra .L.return.%s", current_fn->name);
    return;
  case ND_EXPR_STMT:
    gen_expr(node->lhs, output_file);
    return;
  case ND_ASM:
    println("  %s", node->asm_str);
    return;
  default:
    break;
  }

  error_tok(node->tok, "invalid statement");
}

static void m68k_emit_data(Obj *prog, FILE *out) {
  (void)out;
  for (Obj *var = prog; var; var = var->next) {
    if (var->is_function || !var->is_definition)
      continue;

    if (!var->is_static)
      println("  .globl %s", var->name);

    if (var->init_data) {
      println("  .data");
      println("  .even");
      println("%s:", var->name);
      for (int i = 0; i < var->ty->size; i++)
        println("  .byte %d", var->init_data[i]);
    } else {
      println("  .bss");
      println("  .even");
      println("%s:", var->name);
      println("  .space %d", var->ty->size);
    }
  }
}

static void m68k_load_vreg(LLIRVReg *v, const char *reg) {
  if (!v) return;
  int offset = v->spill_offset ? v->spill_offset : -((v->id + 1) * 4);
  int sz = v->ty ? v->ty->size : 4;
  if (sz == 1)
    println("  move.b %d(%%fp), %s", offset, reg);
  else if (sz == 2)
    println("  move.w %d(%%fp), %s", offset, reg);
  else
    println("  move.l %d(%%fp), %s", offset, reg);
}

static void m68k_store_vreg(const char *reg, LLIRVReg *v) {
  if (!v) return;
  int offset = v->spill_offset ? v->spill_offset : -((v->id + 1) * 4);
  int sz = v->ty ? v->ty->size : 4;
  if (sz == 1)
    println("  move.b %s, %d(%%fp)", reg, offset);
  else if (sz == 2)
    println("  move.w %s, %d(%%fp)", reg, offset);
  else
    println("  move.l %s, %d(%%fp)", reg, offset);
}

static void m68k_gen_insn(LLIRInsn *insn, FILE *out) {
  if (!insn) return;
  (void)out;

  switch (insn->kind) {
  case LLIR_NOP:
  case LLIR_PHI:
    break;
  case LLIR_LABEL:
    println("%s:", insn->label ? insn->label : "");
    break;
  case LLIR_JMP:
    println("  bra %s", insn->label ? insn->label : "");
    break;
  case LLIR_BR_COND:
    m68k_load_vreg(insn->src1, "%d0");
    println("  tst.l %%d0");
    if (insn->label_true && insn->label_false) {
      println("  bne %s", insn->label_true);
      println("  bra %s", insn->label_false);
    } else if (insn->label_true) {
      println("  bne %s", insn->label_true);
    } else if (insn->label_false) {
      println("  beq %s", insn->label_false);
    }
    break;
  case LLIR_IMM:
    if (insn->imm == 0)
      println("  clr.l %%d0");
    else
      println("  move.l #%lld, %%d0", (long long)insn->imm);
    m68k_store_vreg("%d0", insn->dst);
    break;
  case LLIR_LEA:
    if (insn->var) {
      if (insn->var->is_local)
        println("  lea %d(%%fp), %%a0", insn->var->offset + (int)insn->imm);
      else if (insn->imm == 0)
        println("  lea %s, %%a0", insn->var->name);
      else
        println("  lea %s+%d, %%a0", insn->var->name, (int)insn->imm);
    } else if (insn->label) {
      if (insn->imm == 0)
        println("  lea %s, %%a0", insn->label);
      else
        println("  lea %s+%d, %%a0", insn->label, (int)insn->imm);
    }
    println("  move.l %%a0, %%d0");
    m68k_store_vreg("%d0", insn->dst);
    break;
  case LLIR_MOV:
  case LLIR_CAST:
    m68k_load_vreg(insn->src1, "%d0");
    m68k_store_vreg("%d0", insn->dst);
    break;
  case LLIR_LOAD: {
    m68k_load_vreg(insn->src1, "%d0");
    println("  move.l %%d0, %%a0");
    int sz = insn->ty ? insn->ty->size : 4;
    if (sz == 1)
      println("  move.b (%%a0), %%d0");
    else if (sz == 2)
      println("  move.w (%%a0), %%d0");
    else
      println("  move.l (%%a0), %%d0");
    m68k_store_vreg("%d0", insn->dst);
    break;
  }
  case LLIR_STORE: {
    m68k_load_vreg(insn->src1, "%d0");
    println("  move.l %%d0, %%a0");
    m68k_load_vreg(insn->src2, "%d0");
    int sz = insn->ty ? insn->ty->size : 4;
    if (sz == 1)
      println("  move.b %%d0, (%%a0)");
    else if (sz == 2)
      println("  move.w %%d0, (%%a0)");
    else
      println("  move.l %%d0, (%%a0)");
    break;
  }
  case LLIR_ADD:
    m68k_load_vreg(insn->src1, "%d0");
    m68k_load_vreg(insn->src2, "%d1");
    println("  add.l %%d1, %%d0");
    m68k_store_vreg("%d0", insn->dst);
    break;
  case LLIR_SUB:
    m68k_load_vreg(insn->src1, "%d0");
    m68k_load_vreg(insn->src2, "%d1");
    println("  sub.l %%d1, %%d0");
    m68k_store_vreg("%d0", insn->dst);
    break;
  case LLIR_MUL:
    m68k_load_vreg(insn->src1, "%d0");
    m68k_load_vreg(insn->src2, "%d1");
    println("  muls.w %%d1, %%d0");
    m68k_store_vreg("%d0", insn->dst);
    break;
  case LLIR_DIV:
    m68k_load_vreg(insn->src1, "%d0");
    m68k_load_vreg(insn->src2, "%d1");
    println("  divs.w %%d1, %%d0");
    m68k_store_vreg("%d0", insn->dst);
    break;
  case LLIR_MOD:
    m68k_load_vreg(insn->src1, "%d0");
    m68k_load_vreg(insn->src2, "%d1");
    println("  divs.w %%d1, %%d0");
    println("  swap %%d0");
    m68k_store_vreg("%d0", insn->dst);
    break;
  case LLIR_AND:
    m68k_load_vreg(insn->src1, "%d0");
    m68k_load_vreg(insn->src2, "%d1");
    println("  and.l %%d1, %%d0");
    m68k_store_vreg("%d0", insn->dst);
    break;
  case LLIR_OR:
    m68k_load_vreg(insn->src1, "%d0");
    m68k_load_vreg(insn->src2, "%d1");
    println("  or.l %%d1, %%d0");
    m68k_store_vreg("%d0", insn->dst);
    break;
  case LLIR_XOR:
    m68k_load_vreg(insn->src1, "%d0");
    m68k_load_vreg(insn->src2, "%d1");
    println("  eor.l %%d1, %%d0");
    m68k_store_vreg("%d0", insn->dst);
    break;
  case LLIR_SHL:
    m68k_load_vreg(insn->src1, "%d0");
    m68k_load_vreg(insn->src2, "%d1");
    println("  lsl.l %%d1, %%d0");
    m68k_store_vreg("%d0", insn->dst);
    break;
  case LLIR_SHR:
    m68k_load_vreg(insn->src1, "%d0");
    m68k_load_vreg(insn->src2, "%d1");
    println("  asr.l %%d1, %%d0");
    m68k_store_vreg("%d0", insn->dst);
    break;
  case LLIR_NEG:
    m68k_load_vreg(insn->src1, "%d0");
    println("  neg.l %%d0");
    m68k_store_vreg("%d0", insn->dst);
    break;
  case LLIR_NOT:
    m68k_load_vreg(insn->src1, "%d0");
    println("  not.l %%d0");
    m68k_store_vreg("%d0", insn->dst);
    break;
  case LLIR_LOGNOT:
    m68k_load_vreg(insn->src1, "%d0");
    println("  tst.l %%d0");
    println("  seq %%d0");
    println("  ext.w %%d0");
    println("  ext.l %%d0");
    m68k_store_vreg("%d0", insn->dst);
    break;
  case LLIR_CMP_EQ:
    m68k_load_vreg(insn->src1, "%d0");
    m68k_load_vreg(insn->src2, "%d1");
    println("  cmp.l %%d1, %%d0");
    println("  seq %%d0");
    println("  ext.w %%d0");
    println("  ext.l %%d0");
    m68k_store_vreg("%d0", insn->dst);
    break;
  case LLIR_CMP_NE:
    m68k_load_vreg(insn->src1, "%d0");
    m68k_load_vreg(insn->src2, "%d1");
    println("  cmp.l %%d1, %%d0");
    println("  sne %%d0");
    println("  ext.w %%d0");
    println("  ext.l %%d0");
    m68k_store_vreg("%d0", insn->dst);
    break;
  case LLIR_CMP_LT:
    m68k_load_vreg(insn->src1, "%d0");
    m68k_load_vreg(insn->src2, "%d1");
    println("  cmp.l %%d1, %%d0");
    println("  slt %%d0");
    println("  ext.w %%d0");
    println("  ext.l %%d0");
    m68k_store_vreg("%d0", insn->dst);
    break;
  case LLIR_CMP_LE:
    m68k_load_vreg(insn->src1, "%d0");
    m68k_load_vreg(insn->src2, "%d1");
    println("  cmp.l %%d1, %%d0");
    println("  sle %%d0");
    println("  ext.w %%d0");
    println("  ext.l %%d0");
    m68k_store_vreg("%d0", insn->dst);
    break;
  case LLIR_CMP_GT:
    m68k_load_vreg(insn->src1, "%d0");
    m68k_load_vreg(insn->src2, "%d1");
    println("  cmp.l %%d1, %%d0");
    println("  sgt %%d0");
    println("  ext.w %%d0");
    println("  ext.l %%d0");
    m68k_store_vreg("%d0", insn->dst);
    break;
  case LLIR_CMP_GE:
    m68k_load_vreg(insn->src1, "%d0");
    m68k_load_vreg(insn->src2, "%d1");
    println("  cmp.l %%d1, %%d0");
    println("  sge %%d0");
    println("  ext.w %%d0");
    println("  ext.l %%d0");
    m68k_store_vreg("%d0", insn->dst);
    break;
  case LLIR_RET:
    if (insn->src1) {
      m68k_load_vreg(insn->src1, "%d0");
      ABI *fn_abi = (current_fn && current_fn->abi) ? current_fn->abi : current_abi;
      if (fn_abi && fn_abi->emit_return)
        fn_abi->emit_return(current_fn, insn->ty, output_file);
    }
    println("  bra .L.return.%s", current_fn->name);
    break;
  case LLIR_CALL:
    for (int a = insn->num_args - 1; a >= 0; a--) {
      m68k_load_vreg(insn->args[a], "%d0");
      println("  move.l %%d0, -(%%sp)");
    }
    if (insn->src1) {
      m68k_load_vreg(insn->src1, "%d0");
      println("  move.l %%d0, %%a0");
      println("  jsr (%%a0)");
    } else if (insn->label) {
      println("  jsr %s", insn->label);
    }
    if (insn->num_args > 0)
      println("  adda.w #%d, %%sp", insn->num_args * 4);
    if (insn->dst)
      m68k_store_vreg("%d0", insn->dst);
    break;
  case LLIR_ASM:
    println("  %s", insn->asm_str ? insn->asm_str : "");
    break;
  default:
    break;
  }
}

static void m68k_gen_expr(LLIRInsn *insn, FILE *out) {
  m68k_gen_insn(insn, out);
}

static void m68k_emit_text(LLIRProg *prog, FILE *out) {
  (void)out;

  if (prog->fns && prog->num_fns > 0) {
    for (int i = 0; i < prog->num_fns; i++) {
      LLIRFunction *fn = prog->fns[i];
      Obj *fn_obj = fn->fn_obj;
      if (!fn_obj || !fn_obj->is_function || !fn_obj->is_definition || !fn_obj->is_live)
        continue;

      if (!fn_obj->is_static)
        println("  .globl %s", fn->name);

      println("  .text");
      println("  .even");
      println("%s:", fn->name);
      current_fn = fn_obj;

      ABI *fn_abi = fn->abi ? fn->abi : current_abi;

      if (fn_abi && fn_abi->emit_prologue)
        fn_abi->emit_prologue(fn_obj, output_file);

      for (LLIRInsn *insn = fn->head; insn; insn = insn->next) {
        m68k_gen_insn(insn, output_file);
      }

      if (strcmp(fn->name, "main") == 0)
        println("  clr.l %%d0");

      if (fn_abi && fn_abi->emit_epilogue)
        fn_abi->emit_epilogue(fn_obj, output_file);
    }
    return;
  }

  for (Obj *fn = prog->globals; fn; fn = fn->next) {
    if (!fn->is_function || !fn->is_definition || !fn->is_live)
      continue;

    if (!fn->is_static)
      println("  .globl %s", fn->name);

    println("  .text");
    println("  .even");
    println("%s:", fn->name);
    current_fn = fn;

    if (current_abi && current_abi->emit_prologue)
      current_abi->emit_prologue(fn, output_file);

    gen_stmt(fn->body, output_file);

    if (strcmp(fn->name, "main") == 0)
      println("  clr.l %%d0");

    if (current_abi && current_abi->emit_epilogue)
      current_abi->emit_epilogue(fn, output_file);
  }
}

static void m68k_init(FILE *out) {
  output_file = out;
  depth = 0;
}

static void m68k_codegen(LLIRProg *prog, FILE *out) {
  output_file = out;
  depth = 0;

  if (current_abi && current_abi->assign_lvar_offsets)
    current_abi->assign_lvar_offsets(prog->globals);

  m68k_emit_data(prog->globals, out);
  m68k_emit_text(prog, out);
}

static void m68k_codegen_llir(LLIRProg *prog, FILE *out) {
  if (prog)
    m68k_codegen(prog, out);
}

Codegen codegen_m68k = {
  .name = "m68k",
  .description = "Motorola 68000 code generator (Macintosh System 6 / retro)",
  .default_abi_name = "sys6",
  .init = m68k_init,
  .codegen_llir = m68k_codegen_llir,
  .emit_data = m68k_emit_data,
  .emit_text = m68k_emit_text,
  .gen_insn = m68k_gen_insn,
  .gen_expr = m68k_gen_expr,
};
