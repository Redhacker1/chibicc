#include "chibicc.h"
#include "codegen/codegen.h"
#include "codegen/common/common.h"

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
    println("  beq .L.else.%d", c);
    gen_stmt(node->then, output_file);
    println("  bra .L.end.%d", c);
    println(".L.else.%d:", c);
    if (node->els)
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
    if (node->lhs)
      gen_expr(node->lhs, output_file);
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

static void m68k_emit_text(Obj *prog, FILE *out) {
  (void)out;
  for (Obj *fn = prog; fn; fn = fn->next) {
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

static void m68k_codegen(Obj *prog, FILE *out) {
  output_file = out;
  depth = 0;

  if (current_abi && current_abi->assign_lvar_offsets)
    current_abi->assign_lvar_offsets(prog);

  m68k_emit_data(prog, out);
  m68k_emit_text(prog, out);
}

Codegen codegen_m68k = {
  .name = "m68k",
  .description = "Motorola 68000 code generator (Macintosh System 6 / retro)",
  .default_abi_name = "sys6",
  .init = m68k_init,
  .codegen = m68k_codegen,
  .emit_data = m68k_emit_data,
  .emit_text = m68k_emit_text,
  .gen_stmt = gen_stmt,
  .gen_expr = gen_expr,
  .gen_addr = gen_addr,
};
