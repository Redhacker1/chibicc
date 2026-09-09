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

  current_codegen->gen_expr(arg, out);
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
  .emit_prologue = z80_emit_prologue,
  .emit_epilogue = z80_emit_epilogue,
  .define_macros = z80_define_macros,
  .init_types = z80_init_types,
};
