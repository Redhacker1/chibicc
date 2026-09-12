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
  println("  push hl");
  depth++;
}

static void pop(char *arg) {
  println("  pop %s", arg);
  depth--;
}

static void gen_addr(Node *node, FILE *out) {
  (void)out;
  switch (node->kind) {
  case ND_VAR:
    if (node->var->is_local) {
      println("  ld hl, %d", node->var->offset);
      println("  add hl, ix");
      return;
    }
    println("  ld hl, _%s", node->var->name);
    return;
  case ND_DEREF:
    gen_expr(node->lhs, output_file);
    return;
  case ND_MEMBER:
    gen_addr(node->lhs, output_file);
    if (node->member->offset > 0) {
      println("  ld de, %d", node->member->offset);
      println("  add hl, de");
    }
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
  if (ty->size == 1) {
    println("  ld a, (hl)");
    println("  ld l, a");
    if (ty->is_unsigned)
      println("  ld h, 0");
    else
      println("  rla; sbc a, a; ld h, a");
  } else if (ty->size == 2) {
    println("  ld e, (hl)");
    println("  inc hl");
    println("  ld d, (hl)");
    println("  ex de, hl");
  }
}

static void store(Type *ty) {
  pop("de"); // destination address in DE, value in HL

  if (ty->size == 1) {
    println("  ld a, l");
    println("  ld (de), a");
  } else if (ty->size == 2) {
    println("  ld a, l");
    println("  ld (de), a");
    println("  inc de");
    println("  ld a, h");
    println("  ld (de), a");
  }
}

static void gen_expr(Node *node, FILE *out) {
  (void)out;
  switch (node->kind) {
  case ND_NULL_EXPR:
    return;
  case ND_NUM:
    println("  ld hl, %ld", node->val & 0xffff);
    return;
  case ND_NEG:
    gen_expr(node->lhs, output_file);
    println("  ld a, l; cpl; ld l, a; ld a, h; cpl; ld h, a; inc hl");
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
    load(node->ty);
    return;
  case ND_ADDR:
    gen_addr(node->lhs, output_file);
    return;
  case ND_ASSIGN:
    gen_addr(node->lhs, output_file);
    push();
    gen_expr(node->rhs, output_file);
    store(node->ty);
    return;
  case ND_FUNCALL: {
    int stack = 0;
    if (current_abi && current_abi->push_args)
      stack = current_abi->push_args(node, output_file, &depth);

    if (node->lhs->kind == ND_VAR)
      println("  call _%s", node->lhs->var->name);
    else {
      gen_expr(node->lhs, output_file);
      println("  call __call_hl");
    }

    if (stack > 0) {
      println("  ld de, %d", stack * 2);
      println("  add hl, sp; ex de, hl; add hl, de; ld sp, hl");
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
    println("  ld a, h; or l");
    println("  jp z, .L.else.%d", c);
    gen_expr(node->then, output_file);
    println("  jp .L.end.%d", c);
    println(".L.else.%d:", c);
    gen_expr(node->els, output_file);
    println(".L.end.%d:", c);
    return;
  }
  case ND_NOT:
    gen_expr(node->lhs, output_file);
    println("  ld a, h; or l");
    println("  ld hl, 0; jr nz, 1f; inc l; 1:");
    return;
  case ND_BITNOT:
    gen_expr(node->lhs, output_file);
    println("  ld a, l; cpl; ld l, a; ld a, h; cpl; ld h, a");
    return;
  default:
    break;
  }

  gen_expr(node->rhs, output_file);
  push();
  gen_expr(node->lhs, output_file);
  pop("de"); // lhs in HL, rhs in DE

  switch (node->kind) {
  case ND_ADD:
    println("  add hl, de");
    return;
  case ND_SUB:
    println("  or a; sbc hl, de");
    return;
  case ND_BITAND:
    println("  ld a, l; and e; ld l, a; ld a, h; and d; ld h, a");
    return;
  case ND_BITOR:
    println("  ld a, l; or e; ld l, a; ld a, h; or d; ld h, a");
    return;
  case ND_BITXOR:
    println("  ld a, l; xor e; ld l, a; ld a, h; xor d; ld h, a");
    return;
  case ND_EQ:
    println("  or a; sbc hl, de; ld hl, 0; jr nz, 1f; inc l; 1:");
    return;
  case ND_NE:
    println("  or a; sbc hl, de; ld hl, 1; jr nz, 1f; dec l; 1:");
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
    println("  ld a, h; or l");
    if (!node->els) {
      println("  jp z, .L.end.%d", c);
      gen_stmt(node->then, output_file);
      println(".L.end.%d:", c);
      return;
    }
    println("  jp z, .L.else.%d", c);
    gen_stmt(node->then, output_file);
    println("  jp .L.end.%d", c);
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
      println("  ld a, h; or l");
      println("  jp z, %s", node->brk_label);
    }
    gen_stmt(node->then, output_file);
    println("%s:", node->cont_label);
    if (node->inc)
      gen_expr(node->inc, output_file);
    println("  jp .L.begin.%d", c);
    println("%s:", node->brk_label);
    return;
  }
  case ND_BLOCK:
    for (Node *n = node->body; n; n = n->next)
      gen_stmt(n, output_file);
    return;
  case ND_GOTO:
    println("  jp %s", node->unique_label);
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
    println("  jp .L.return.%s", current_fn->name);
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

static void z80_emit_data(Obj *prog, FILE *out) {
  (void)out;
  for (Obj *var = prog; var; var = var->next) {
    if (var->is_function || !var->is_definition)
      continue;

    if (!var->is_static)
      println("  .globl _%s", var->name);

    if (var->init_data) {
      println("  .data");
      println("_%s:", var->name);
      for (int i = 0; i < var->ty->size; i++)
        println("  .byte 0x%02x", (unsigned char)var->init_data[i]);
    } else {
      println("  .bss");
      println("_%s:", var->name);
      println("  .ds %d", var->ty->size);
    }
  }
}

static void z80_load_vreg(LLIRVReg *v, const char *reg) {
  if (!v) return;
  int offset = v->spill_offset ? v->spill_offset : -((v->id + 1) * 2);
  int sz = v->ty ? v->ty->size : 2;
  if (strcmp(reg, "hl") == 0) {
    if (sz == 1)
      println("  ld l, (ix %d)\n  ld h, 0", offset);
    else
      println("  ld l, (ix %d)\n  ld h, (ix %d)", offset, offset + 1);
  } else if (strcmp(reg, "de") == 0) {
    if (sz == 1)
      println("  ld e, (ix %d)\n  ld d, 0", offset);
    else
      println("  ld e, (ix %d)\n  ld d, (ix %d)", offset, offset + 1);
  }
}

static void z80_store_vreg(const char *reg, LLIRVReg *v) {
  if (!v) return;
  int offset = v->spill_offset ? v->spill_offset : -((v->id + 1) * 2);
  int sz = v->ty ? v->ty->size : 2;
  if (strcmp(reg, "hl") == 0) {
    if (sz == 1)
      println("  ld (ix %d), l", offset);
    else
      println("  ld (ix %d), l\n  ld (ix %d), h", offset, offset + 1);
  } else if (strcmp(reg, "de") == 0) {
    if (sz == 1)
      println("  ld (ix %d), e", offset);
    else
      println("  ld (ix %d), e\n  ld (ix %d), d", offset, offset + 1);
  }
}

static void z80_gen_insn(LLIRInsn *insn, FILE *out) {
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
    println("  jp %s", insn->label ? insn->label : "");
    break;
  case LLIR_BR_COND:
    z80_load_vreg(insn->src1, "hl");
    println("  ld a, h\n  or l");
    if (insn->label_true && insn->label_false) {
      println("  jp nz, %s", insn->label_true);
      println("  jp %s", insn->label_false);
    } else if (insn->label_true) {
      println("  jp nz, %s", insn->label_true);
    } else if (insn->label_false) {
      println("  jp z, %s", insn->label_false);
    }
    break;
  case LLIR_IMM:
    println("  ld hl, %lld", (long long)insn->imm);
    z80_store_vreg("hl", insn->dst);
    break;
  case LLIR_LEA:
    if (insn->var) {
      if (insn->var->is_local) {
        println("  ld hl, %d", insn->var->offset + (int)insn->imm);
        println("  add hl, ix");
      } else if (insn->imm == 0) {
        println("  ld hl, _%s", insn->var->name);
      } else {
        println("  ld hl, _%s+%d", insn->var->name, (int)insn->imm);
      }
    } else if (insn->label) {
      if (insn->imm == 0)
        println("  ld hl, %s", insn->label);
      else
        println("  ld hl, %s+%d", insn->label, (int)insn->imm);
    }
    z80_store_vreg("hl", insn->dst);
    break;
  case LLIR_MOV:
  case LLIR_CAST:
    z80_load_vreg(insn->src1, "hl");
    z80_store_vreg("hl", insn->dst);
    break;
  case LLIR_LOAD: {
    z80_load_vreg(insn->src1, "hl");
    int sz = insn->ty ? insn->ty->size : 2;
    if (sz == 1)
      println("  ld a, (hl)\n  ld l, a\n  ld h, 0");
    else
      println("  ld e, (hl)\n  inc hl\n  ld d, (hl)\n  ex de, hl");
    z80_store_vreg("hl", insn->dst);
    break;
  }
  case LLIR_STORE: {
    z80_load_vreg(insn->src1, "hl");
    z80_load_vreg(insn->src2, "de");
    int sz = insn->ty ? insn->ty->size : 2;
    if (sz == 1)
      println("  ld (hl), e");
    else
      println("  ld (hl), e\n  inc hl\n  ld (hl), d");
    break;
  }
  case LLIR_ADD:
    z80_load_vreg(insn->src1, "hl");
    z80_load_vreg(insn->src2, "de");
    println("  add hl, de");
    z80_store_vreg("hl", insn->dst);
    break;
  case LLIR_SUB:
    z80_load_vreg(insn->src1, "hl");
    z80_load_vreg(insn->src2, "de");
    println("  or a\n  sbc hl, de");
    z80_store_vreg("hl", insn->dst);
    break;
  case LLIR_AND:
    z80_load_vreg(insn->src1, "hl");
    z80_load_vreg(insn->src2, "de");
    println("  ld a, h\n  and d\n  ld h, a\n  ld a, l\n  and e\n  ld l, a");
    z80_store_vreg("hl", insn->dst);
    break;
  case LLIR_OR:
    z80_load_vreg(insn->src1, "hl");
    z80_load_vreg(insn->src2, "de");
    println("  ld a, h\n  or d\n  ld h, a\n  ld a, l\n  or e\n  ld l, a");
    z80_store_vreg("hl", insn->dst);
    break;
  case LLIR_XOR:
    z80_load_vreg(insn->src1, "hl");
    z80_load_vreg(insn->src2, "de");
    println("  ld a, h\n  xor d\n  ld h, a\n  ld a, l\n  xor e\n  ld l, a");
    z80_store_vreg("hl", insn->dst);
    break;
  case LLIR_RET:
    if (insn->src1) {
      z80_load_vreg(insn->src1, "hl");
      ABI *fn_abi = (current_fn && current_fn->abi) ? current_fn->abi : current_abi;
      if (fn_abi && fn_abi->emit_return)
        fn_abi->emit_return(current_fn, insn->ty, output_file);
    }
    println("  jp .L.return.%s", current_fn->name);
    break;
  case LLIR_CALL:
    for (int a = insn->num_args - 1; a >= 0; a--) {
      z80_load_vreg(insn->args[a], "hl");
      println("  push hl");
    }
    if (insn->src1) {
      z80_load_vreg(insn->src1, "hl");
      println("  call __call_hl");
    } else if (insn->label) {
      println("  call _%s", insn->label);
    }
    if (insn->num_args > 0)
      println("  ld hl, %d\n  add hl, sp\n  ld sp, hl", insn->num_args * 2);
    if (insn->dst)
      z80_store_vreg("hl", insn->dst);
    break;
  case LLIR_ASM:
    println("  %s", insn->asm_str ? insn->asm_str : "");
    break;
  default:
    break;
  }
}

static void z80_gen_expr(LLIRInsn *insn, FILE *out) {
  z80_gen_insn(insn, out);
}

static void z80_emit_text(LLIRProg *prog, FILE *out) {
  (void)out;

  if (prog->fns && prog->num_fns > 0) {
    for (int i = 0; i < prog->num_fns; i++) {
      LLIRFunction *fn = prog->fns[i];
      Obj *fn_obj = fn->fn_obj;
      if (!fn_obj || !fn_obj->is_function || !fn_obj->is_definition || !fn_obj->is_live)
        continue;

      if (!fn_obj->is_static)
        println("  .globl _%s", fn->name);

      println("  .text");
      println("_%s:", fn->name);
      current_fn = fn_obj;

      ABI *fn_abi = fn->abi ? fn->abi : current_abi;

      if (fn_abi && fn_abi->emit_prologue)
        fn_abi->emit_prologue(fn_obj, output_file);

      for (LLIRInsn *insn = fn->head; insn; insn = insn->next) {
        z80_gen_insn(insn, output_file);
      }

      if (strcmp(fn->name, "main") == 0)
        println("  ld hl, 0");

      if (fn_abi && fn_abi->emit_epilogue)
        fn_abi->emit_epilogue(fn_obj, output_file);
    }
    return;
  }

  for (Obj *fn = prog->globals; fn; fn = fn->next) {
    if (!fn->is_function || !fn->is_definition || !fn->is_live)
      continue;

    if (!fn->is_static)
      println("  .globl _%s", fn->name);

    println("  .text");
    println("_%s:", fn->name);
    current_fn = fn;

    if (current_abi && current_abi->emit_prologue)
      current_abi->emit_prologue(fn, output_file);

    gen_stmt(fn->body, output_file);

    if (strcmp(fn->name, "main") == 0)
      println("  ld hl, 0");

    if (current_abi && current_abi->emit_epilogue)
      current_abi->emit_epilogue(fn, output_file);
  }
}

static void z80_init(FILE *out) {
  output_file = out;
  depth = 0;
}

static void z80_codegen(LLIRProg *prog, FILE *out) {
  output_file = out;
  depth = 0;

  if (current_abi && current_abi->assign_lvar_offsets)
    current_abi->assign_lvar_offsets(prog->globals);

  z80_emit_data(prog->globals, out);
  z80_emit_text(prog, out);
}

static void z80_codegen_llir(LLIRProg *prog, FILE *out) {
  if (prog)
    z80_codegen(prog, out);
}

Codegen codegen_z80 = {
  .name = "z80",
  .description = "Zilog Z80 retro microcomputer code generator",
  .default_abi_name = "z80",
  .init = z80_init,
  .codegen_llir = z80_codegen_llir,
  .emit_data = z80_emit_data,
  .emit_text = z80_emit_text,
  .gen_insn = z80_gen_insn,
  .gen_expr = z80_gen_expr,
};
