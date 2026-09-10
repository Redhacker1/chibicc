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

static bool sys6_returns_by_reference(Type *ty) {
  if (ty->kind != TY_STRUCT && ty->kind != TY_UNION)
    return false;
  return ty->size > 4;
}

static int sys6_classify_reg(Type *ty) {
  if (is_integer(ty) || ty->kind == TY_PTR)
    return 0;
  if (is_flonum(ty))
    return 1;
  return 2;
}

static void sys6_assign_lvar_offsets(Obj *prog) {
  for (Obj *fn = prog; fn; fn = fn->next) {
    if (!fn->is_function)
      continue;

    // Parameters start at A6 + 8 (saved A6 is 4 bytes, return PC is 4 bytes)
    int top = 8;
    int bottom = 0;

    for (Obj *var = fn->params; var; var = var->next) {
      top = align_to(top, 2);
      var->offset = top;
      // In 68k C, each stack parameter takes at least 2 or 4 bytes
      top += align_to(var->ty->size, 2);
    }

    for (Obj *var = fn->locals; var; var = var->next) {
      if (var->offset)
        continue;
      bottom += var->ty->size;
      bottom = align_to(bottom, 2);
      var->offset = -bottom;
    }

    fn->stack_size = align_to(bottom, 2);
  }
}

static void sys6_push_args_rev(Node *arg, FILE *out, int *depth) {
  if (!arg)
    return;
  sys6_push_args_rev(arg->next, out, depth);

  if (current_codegen && current_codegen->gen_expr)
    ((void (*)(void *, FILE *))current_codegen->gen_expr)(arg, out);
  if (arg->ty->kind == TY_STRUCT || arg->ty->kind == TY_UNION) {
    int sz = align_to(arg->ty->size, 2);
    println_abi(out, "  suba.w #%d, %%sp", sz);
    *depth += sz / 2;
    for (int i = 0; i < arg->ty->size; i++) {
      println_abi(out, "  move.b %d(%%a0), %d(%%sp)", i, i);
    }
  } else if (arg->ty->size == 4) {
    println_abi(out, "  move.l %%d0, -(%%sp)");
    *depth += 2;
  } else if (arg->ty->size == 2) {
    println_abi(out, "  move.w %%d0, -(%%sp)");
    *depth += 1;
  } else {
    println_abi(out, "  ext.w %%d0");
    println_abi(out, "  move.w %%d0, -(%%sp)");
    *depth += 1;
  }
}

static int sys6_push_args(Node *node, FILE *out, int *depth) {
  int initial_depth = *depth;

  sys6_push_args_rev(node->args, out, depth);

  if (node->ret_buffer && sys6_returns_by_reference(node->ty)) {
    println_abi(out, "  pea %d(%%a6)", node->ret_buffer->offset);
    *depth += 2;
  }

  return *depth - initial_depth;
}

static void sys6_copy_ret_buffer(Obj *var, FILE *out) {
  Type *ty = var->ty;
  if (ty->size <= 2) {
    println_abi(out, "  move.w %%d0, %d(%%a6)", var->offset);
  } else if (ty->size <= 4) {
    println_abi(out, "  move.l %%d0, %d(%%a6)", var->offset);
  }
}

static void sys6_copy_struct_reg(Obj *fn, FILE *out) {
  Type *ty = fn->ty->return_ty;
  if (ty->size <= 2) {
    println_abi(out, "  move.w (%%a0), %%d0");
  } else if (ty->size <= 4) {
    println_abi(out, "  move.l (%%a0), %%d0");
  }
}

static void sys6_copy_struct_mem(Obj *fn, FILE *out) {
  Type *ty = fn->ty->return_ty;
  Obj *var = fn->params;
  println_abi(out, "  movea.l %d(%%a6), %%a1", var->offset);
  for (int i = 0; i < ty->size; i++) {
    println_abi(out, "  move.b %d(%%a0), %d(%%a1)", i, i);
  }
}

static void sys6_builtin_alloca(Obj *fn, FILE *out) {
  println_abi(out, "  addq.l #1, %%d0");
  println_abi(out, "  bclr #0, %%d0");
  println_abi(out, "  suba.l %%d0, %%sp");
  println_abi(out, "  move.l %%sp, %%d0");
}

static void sys6_emit_prologue(Obj *fn, FILE *out) {
  println_abi(out, "  link %%a6, #-%d", fn->stack_size);
  println_abi(out, "  move.l %%sp, %d(%%a6)", fn->alloca_bottom->offset);
}

static void sys6_emit_epilogue(Obj *fn, FILE *out) {
  println_abi(out, ".L.return.%s:", fn->name);
  println_abi(out, "  unlk %%a6");
  println_abi(out, "  rts");
}

static void sys6_emit_return(Obj *fn, Type *return_ty, FILE *out) {
  if (!return_ty && fn && fn->ty)
    return_ty = fn->ty->return_ty;
  if (!return_ty)
    return;

  if (return_ty->kind == TY_STRUCT || return_ty->kind == TY_UNION) {
    if (sys6_returns_by_reference(return_ty)) {
      sys6_copy_struct_mem(fn, out);
    } else {
      sys6_copy_struct_reg(fn, out);
    }
  }
}

static Node *sys6_builtin_va_start(Node *ap, Node *last, Token *tok) {
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

static Node *sys6_builtin_va_arg(Node *ap, Type *ty, Token *tok) {
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

static Node *sys6_builtin_va_copy(Node *dest, Node *src, Token *tok) {
  add_type(dest);
  if (dest->ty->kind == TY_PTR && (dest->ty->base->kind != TY_STRUCT && dest->ty->base->kind != TY_UNION)) {
    return new_binary(ND_ASSIGN, dest, src, tok);
  } else {
    Node *deref_dest = new_unary(ND_DEREF, dest, tok);
    Node *deref_src = new_unary(ND_DEREF, src, tok);
    return new_binary(ND_ASSIGN, deref_dest, deref_src, tok);
  }
}

static Node *sys6_builtin_va_end(Node *ap, Token *tok) {
  Node *node = new_node(ND_NULL_EXPR, tok);
  node->ty = ty_void;
  return node;
}

static void sys6_define_macros(void) {
  define_macro("__m68k__", "1");
  define_macro("__mc68000__", "1");
  define_macro("__mc68000", "1");
  define_macro("__macintosh__", "1");
  define_macro("__sys6__", "1");
}

static void sys6_init_types(void) {
  ty_char->size = 1; ty_char->align = 1;
  ty_short->size = 2; ty_short->align = 2;
  ty_int->size = 4; ty_int->align = 2;
  ty_long->size = 4; ty_long->align = 2;
  ty_uchar->size = 1; ty_uchar->align = 1;
  ty_ushort->size = 2; ty_ushort->align = 2;
  ty_uint->size = 4; ty_uint->align = 2;
  ty_ulong->size = 4; ty_ulong->align = 2;
  ty_float->size = 4; ty_float->align = 2;
  ty_double->size = 8; ty_double->align = 2;
  ty_ldouble->size = 8; ty_ldouble->align = 2;
}

static void sys6_declare_builtin_types(void) {
  push_scope("__builtin_va_list")->type_def = pointer_to(ty_char);
}

ABI abi_sys6 = {
  .name = "sys6",
  .description = "Macintosh System 6 (Motorola 68000) ABI",
  .default_objfmt = &objfmt_flat,
  .size_bool = 1, .align_bool = 1,
  .size_char = 1, .align_char = 1,
  .size_short = 2, .align_short = 2,
  .size_int = 4, .align_int = 2,
  .size_long = 4, .align_long = 2,
  .size_llong = 8, .align_llong = 2,
  .size_ptr = 4, .align_ptr = 2,
  .size_float = 4, .align_float = 2,
  .size_double = 8, .align_double = 2,
  .size_ldouble = 8, .align_ldouble = 2,
  .align_stack = 2,
  .va_area_size = 4,
  .va_area_align = 2,
  .returns_by_reference = sys6_returns_by_reference,
  .classify_reg = sys6_classify_reg,
  .assign_lvar_offsets = sys6_assign_lvar_offsets,
  .push_args = sys6_push_args,
  .copy_ret_buffer = sys6_copy_ret_buffer,
  .copy_struct_reg = sys6_copy_struct_reg,
  .copy_struct_mem = sys6_copy_struct_mem,
  .builtin_alloca = sys6_builtin_alloca,
  .builtin_va_start = sys6_builtin_va_start,
  .builtin_va_arg = sys6_builtin_va_arg,
  .builtin_va_copy = sys6_builtin_va_copy,
  .builtin_va_end = sys6_builtin_va_end,
  .emit_prologue = sys6_emit_prologue,
  .emit_epilogue = sys6_emit_epilogue,
  .emit_return = sys6_emit_return,
  .define_macros = sys6_define_macros,
  .init_types = sys6_init_types,
  .declare_builtin_types = sys6_declare_builtin_types,
};

static int pascal_push_args(Node *node, FILE *out, int *depth) {
  int initial_depth = *depth;

  // In Pascal, if function returns a value, caller reserves space first
  if (node->ty->kind != TY_VOID) {
    int ret_sz = align_to(node->ty->size, 2);
    println_abi(out, "  suba.w #%d, %%sp", ret_sz);
    *depth += ret_sz / 2;
  }

  // Push arguments in left-to-right order
  for (Node *arg = node->args; arg; arg = arg->next) {
    if (current_codegen && current_codegen->gen_expr)
      ((void (*)(void *, FILE *))current_codegen->gen_expr)(arg, out);
    if (arg->ty->kind == TY_STRUCT || arg->ty->kind == TY_UNION) {
      int sz = align_to(arg->ty->size, 2);
      println_abi(out, "  suba.w #%d, %%sp", sz);
      *depth += sz / 2;
      for (int i = 0; i < arg->ty->size; i++) {
        println_abi(out, "  move.b %d(%%a0), %d(%%sp)", i, i);
      }
    } else if (arg->ty->size == 4) {
      println_abi(out, "  move.l %%d0, -(%%sp)");
      *depth += 2;
    } else if (arg->ty->size == 2) {
      println_abi(out, "  move.w %%d0, -(%%sp)");
      *depth += 1;
    } else {
      println_abi(out, "  ext.w %%d0");
      println_abi(out, "  move.w %%d0, -(%%sp)");
      *depth += 1;
    }
  }

  return *depth - initial_depth;
}

static void pascal_emit_epilogue(Obj *fn, FILE *out) {
  println_abi(out, ".L.return.%s:", fn->name);
  int param_bytes = 0;
  for (Obj *var = fn->params; var; var = var->next)
    param_bytes += align_to(var->ty->size, 2);

  println_abi(out, "  unlk %%a6");
  if (param_bytes > 0) {
    println_abi(out, "  move.l (%%sp)+, %%a0");
    println_abi(out, "  adda.w #%d, %%sp", param_bytes);
    println_abi(out, "  jmp (%%a0)");
  } else {
    println_abi(out, "  rts");
  }
}

ABI abi_pascal = {
  .name = "pascal",
  .description = "Macintosh Toolbox Pascal Calling Convention",
  .default_objfmt = &objfmt_flat,
  .size_bool = 1, .align_bool = 1,
  .size_char = 1, .align_char = 1,
  .size_short = 2, .align_short = 2,
  .size_int = 4, .align_int = 2,
  .size_long = 4, .align_long = 2,
  .size_llong = 8, .align_llong = 2,
  .size_ptr = 4, .align_ptr = 2,
  .size_float = 4, .align_float = 2,
  .size_double = 8, .align_double = 2,
  .size_ldouble = 8, .align_ldouble = 2,
  .align_stack = 2,
  .va_area_size = 4,
  .va_area_align = 2,
  .returns_by_reference = sys6_returns_by_reference,
  .classify_reg = sys6_classify_reg,
  .assign_lvar_offsets = sys6_assign_lvar_offsets,
  .push_args = pascal_push_args,
  .copy_ret_buffer = sys6_copy_ret_buffer,
  .copy_struct_reg = sys6_copy_struct_reg,
  .copy_struct_mem = sys6_copy_struct_mem,
  .builtin_alloca = sys6_builtin_alloca,
  .emit_prologue = sys6_emit_prologue,
  .emit_epilogue = pascal_emit_epilogue,
  .emit_return = sys6_emit_return,
  .define_macros = sys6_define_macros,
  .init_types = sys6_init_types,
  .declare_builtin_types = sys6_declare_builtin_types,
};
