#include "chibicc.h"
#include "abi/abi.h"
#include "codegen/common/common.h"

static void println_abi(FILE *out, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(out, fmt, ap);
  va_end(ap);
  fprintf(out, "\n");
}

static bool z80_returns_by_reference(Type *ty) {
  if (ty->kind != TY_STRUCT && ty->kind != TY_UNION)
    return false;
  return ty->size > 2;
}

static int z80_classify_reg(Type *ty) {
  if (is_integer(ty) || ty->kind == TY_PTR)
    return 0;
  if (is_flonum(ty))
    return 1;
  return 2;
}

static void z80_assign_lvar_offsets(Obj *prog) {
  for (Obj *fn = prog; fn; fn = fn->next) {
    if (!fn->is_function)
      continue;

    // Parameters start at IX + 4 (saved IX is 2 bytes, return PC is 2 bytes)
    int top = 4;
    int bottom = 0;

    for (Obj *var = fn->params; var; var = var->next) {
      var->offset = top;
      top += align_to(var->ty->size, 2);
    }

    for (Obj *var = fn->locals; var; var = var->next) {
      if (var->offset)
        continue;
      bottom += var->ty->size;
      var->offset = -bottom;
    }

    fn->stack_size = align_to(bottom, 2);
  }
}

static void z80_push_args_rev(Node *arg, FILE *out, int *depth) {
  if (!arg)
    return;
  z80_push_args_rev(arg->next, out, depth);

  if (current_codegen && current_codegen->gen_expr)
    ((void (*)(void *, FILE *))current_codegen->gen_expr)(arg, out);
  if (arg->ty->size <= 2) {
    println_abi(out, "  push hl");
    *depth += 1;
  } else if (arg->ty->size == 4) {
    println_abi(out, "  push de");
    println_abi(out, "  push hl");
    *depth += 2;
  } else {
    int sz = align_to(arg->ty->size, 2);
    println_abi(out, "  ld hl, -%d", sz);
    println_abi(out, "  add hl, sp");
    println_abi(out, "  ld sp, hl");
    *depth += sz / 2;
  }
}

static int z80_push_args(Node *node, FILE *out, int *depth) {
  int initial_depth = *depth;

  z80_push_args_rev(node->args, out, depth);

  if (node->ret_buffer && z80_returns_by_reference(node->ty)) {
    println_abi(out, "  ld hl, %d", node->ret_buffer->offset);
    println_abi(out, "  add hl, ix");
    println_abi(out, "  push hl");
    *depth += 1;
  }

  return *depth - initial_depth;
}

static void z80_copy_ret_buffer(Obj *var, FILE *out) {
  Type *ty = var->ty;
  if (ty->size == 1) {
    println_abi(out, "  ld (ix%+d), a", var->offset);
  } else if (ty->size == 2) {
    println_abi(out, "  ld (ix%+d), l", var->offset);
    println_abi(out, "  ld (ix%+d), h", var->offset + 1);
  }
}

static void z80_copy_struct_reg(Obj *fn, FILE *out) {
  Type *ty = fn->ty->return_ty;
  if (ty->size == 1) {
    println_abi(out, "  ld a, (hl)");
  } else if (ty->size == 2) {
    println_abi(out, "  ld e, (hl)");
    println_abi(out, "  inc hl");
    println_abi(out, "  ld d, (hl)");
    println_abi(out, "  ex de, hl");
  }
}

static void z80_copy_struct_mem(Obj *fn, FILE *out) {
  Type *ty = fn->ty->return_ty;
  Obj *var = fn->params;
  println_abi(out, "  ld e, (ix%+d)", var->offset);
  println_abi(out, "  ld d, (ix%+d)", var->offset + 1);
  for (int i = 0; i < ty->size; i++) {
    println_abi(out, "  ld a, (hl)");
    println_abi(out, "  ld (de), a");
    if (i + 1 < ty->size) {
      println_abi(out, "  inc hl");
      println_abi(out, "  inc de");
    }
  }
}

static void z80_builtin_alloca(Obj *fn, FILE *out) {
  println_abi(out, "  ld hl, 0");
  println_abi(out, "  add hl, sp");
  println_abi(out, "  sbc hl, de");
  println_abi(out, "  ld sp, hl");
}

static void z80_emit_prologue(Obj *fn, FILE *out) {
  println_abi(out, "  push ix");
  println_abi(out, "  ld ix, 0");
  println_abi(out, "  add ix, sp");
  if (fn->stack_size > 0) {
    println_abi(out, "  ld hl, -%d", fn->stack_size);
    println_abi(out, "  add hl, sp");
    println_abi(out, "  ld sp, hl");
  }
}

static void z80_emit_epilogue(Obj *fn, FILE *out) {
  println_abi(out, ".L.return.%s:", fn->name);
  println_abi(out, "  ld sp, ix");
  println_abi(out, "  pop ix");
  println_abi(out, "  ret");
}

static void z80_emit_return(Obj *fn, Type *return_ty, FILE *out) {
  if (!return_ty && fn && fn->ty)
    return_ty = fn->ty->return_ty;
  if (!return_ty)
    return;

  if (return_ty->kind == TY_STRUCT || return_ty->kind == TY_UNION) {
    if (z80_returns_by_reference(return_ty)) {
      z80_copy_struct_mem(fn, out);
    } else {
      z80_copy_struct_reg(fn, out);
    }
  }
}

static Node *z80_builtin_va_start(Node *ap, Node *last, Token *tok) {
  VarScope *sc = find_var(&(Token){.loc = "__va_area__", .len = 11});
  if (!sc || !sc->var)
    error_tok(tok, "__builtin_va_start used outside variadic function");
  Node *va_var = new_var_node(sc->var, tok);
  add_type(ap);
  if (ap->ty->kind == TY_PTR && (ap->ty->base->kind != TY_STRUCT && ap->ty->base->kind != TY_UNION)) {
    Node *addr = new_unary(ND_ADDR, va_var, tok);
    Node *val = new_unary(ND_DEREF, new_cast(addr, pointer_to(ap->ty)), tok);
    return new_binary(ND_ASSIGN, ap, val, tok);
  } else {
    Node *va_addr = new_unary(ND_ADDR, va_var, tok);
    Node *val = new_unary(ND_DEREF, new_cast(va_addr, pointer_to(pointer_to(ty_void))), tok);
    Node *of_addr = new_add(new_cast(ap, pointer_to(ty_char)), new_num(8, tok), tok);
    Node *of_ptr = new_cast(of_addr, pointer_to(pointer_to(ty_void)));
    return new_binary(ND_ASSIGN, new_unary(ND_DEREF, of_ptr, tok), val, tok);
  }
}

static Node *z80_builtin_va_arg(Node *ap, Type *ty, Token *tok) {
  add_type(ap);
  bool is_ptr = (ap->ty->kind == TY_PTR && (ap->ty->base->kind != TY_STRUCT && ap->ty->base->kind != TY_UNION));
  int sz = align_to(MAX(2, ty->size), 2);

  Obj *old_p = new_lvar("", pointer_to(ty_void));
  Node head = {};
  Node *cur = &head;

  if (is_ptr) {
    cur = cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, new_var_node(old_p, tok), new_cast(ap, pointer_to(ty_void)), tok), tok);
    cur = cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, ap, new_cast(new_add(new_cast(ap, pointer_to(ty_char)), new_num(sz, tok), tok), ap->ty), tok), tok);
  } else {
    Node *of_addr = new_add(new_cast(ap, pointer_to(ty_char)), new_num(8, tok), tok);
    Node *of_ptr = new_cast(of_addr, pointer_to(pointer_to(ty_void)));
    Node *load_of = new_unary(ND_DEREF, of_ptr, tok);
    cur = cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, new_var_node(old_p, tok), load_of, tok), tok);
    Node *new_of = new_add(new_cast(new_var_node(old_p, tok), pointer_to(ty_char)), new_num(sz, tok), tok);
    cur = cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, new_unary(ND_DEREF, of_ptr, tok), new_cast(new_of, pointer_to(ty_void)), tok), tok);
  }

  cur = cur->next = new_unary(ND_EXPR_STMT,
    new_unary(ND_DEREF, new_cast(new_var_node(old_p, tok), pointer_to(ty)), tok), tok);

  Node *node = new_node(ND_STMT_EXPR, tok);
  node->body = head.next;
  return node;
}

static Node *z80_builtin_va_copy(Node *dest, Node *src, Token *tok) {
  add_type(dest);
  if (dest->ty->kind == TY_PTR && (dest->ty->base->kind != TY_STRUCT && dest->ty->base->kind != TY_UNION)) {
    return new_binary(ND_ASSIGN, dest, src, tok);
  } else {
    Node *deref_dest = new_unary(ND_DEREF, dest, tok);
    Node *deref_src = new_unary(ND_DEREF, src, tok);
    return new_binary(ND_ASSIGN, deref_dest, deref_src, tok);
  }
}

static Node *z80_builtin_va_end(Node *ap, Token *tok) {
  Node *node = new_node(ND_NULL_EXPR, tok);
  node->ty = ty_void;
  return node;
}

static void z80_define_macros(void) {
  define_macro("__z80__", "1");
  define_macro("__z80", "1");
}

static void z80_init_types(void) {
  ty_char->size = 1; ty_char->align = 1;
  ty_short->size = 2; ty_short->align = 1;
  ty_int->size = 2; ty_int->align = 1;
  ty_long->size = 4; ty_long->align = 1;
  ty_uchar->size = 1; ty_uchar->align = 1;
  ty_ushort->size = 2; ty_ushort->align = 1;
  ty_uint->size = 2; ty_uint->align = 1;
  ty_ulong->size = 4; ty_ulong->align = 1;
  ty_float->size = 4; ty_float->align = 1;
  ty_double->size = 4; ty_double->align = 1;
  ty_ldouble->size = 4; ty_ldouble->align = 1;
}

static void z80_declare_builtin_types(void) {
  push_scope("__builtin_va_list")->type_def = pointer_to(ty_char);
}

ABI abi_z80 = {
  .name = "z80",
  .description = "Zilog Z80 Retro Microcomputer ABI",
  .default_objfmt = &objfmt_flat,
  .size_bool = 1, .align_bool = 1,
  .size_char = 1, .align_char = 1,
  .size_short = 2, .align_short = 1,
  .size_int = 2, .align_int = 1,
  .size_long = 4, .align_long = 1,
  .size_llong = 4, .align_llong = 1,
  .size_ptr = 2, .align_ptr = 1,
  .size_float = 4, .align_float = 1,
  .size_double = 4, .align_double = 1,
  .size_ldouble = 4, .align_ldouble = 1,
  .align_stack = 1,
  .va_area_size = 2,
  .va_area_align = 1,
  .returns_by_reference = z80_returns_by_reference,
  .classify_reg = z80_classify_reg,
  .assign_lvar_offsets = z80_assign_lvar_offsets,
  .push_args = z80_push_args,
  .copy_ret_buffer = z80_copy_ret_buffer,
  .copy_struct_reg = z80_copy_struct_reg,
  .copy_struct_mem = z80_copy_struct_mem,
  .builtin_alloca = z80_builtin_alloca,
  .builtin_va_start = z80_builtin_va_start,
  .builtin_va_arg = z80_builtin_va_arg,
  .builtin_va_copy = z80_builtin_va_copy,
  .builtin_va_end = z80_builtin_va_end,
  .emit_prologue = z80_emit_prologue,
  .emit_epilogue = z80_emit_epilogue,
  .emit_return = z80_emit_return,
  .define_macros = z80_define_macros,
  .init_types = z80_init_types,
  .declare_builtin_types = z80_declare_builtin_types,
};
