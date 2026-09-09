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

static bool win32_returns_by_reference(Type *ty) {
  if (ty->kind != TY_STRUCT && ty->kind != TY_UNION)
    return false;
  return ty->size > 4;
}

static int win32_classify_reg(Type *ty) {
  if (is_integer(ty) || ty->kind == TY_PTR)
    return 0;
  if (is_flonum(ty))
    return 1;
  return 2;
}

static void win32_assign_lvar_offsets(Obj *fn) {
  if (!fn->is_function)
    return;

  // Stack arguments start at RBP + 16 (RBP + return address)
  int top = 16;
  int bottom = 0;

  for (Obj *var = fn->params; var; var = var->next) {
    top = align_to(top, 8);
    var->offset = top;
    top += align_to(var->ty->size, 8);
  }

  for (Obj *var = fn->locals; var; var = var->next) {
    if (var->offset)
      continue;
    bottom += var->ty->size;
    bottom = align_to(bottom, var->align > 0 ? var->align : 8);
    var->offset = -bottom;
  }

  fn->stack_size = align_to(bottom, 16);
}

static void win32_push_args_rev(Node *arg, FILE *out, int *depth) {
  if (!arg)
    return;
  win32_push_args_rev(arg->next, out, depth);

  current_codegen->gen_expr(arg, out);
  if (arg->ty->kind == TY_STRUCT || arg->ty->kind == TY_UNION) {
    int sz = align_to(arg->ty->size, 8);
    println_abi(out, "  sub $%d, %%rsp", sz);
    *depth += sz / 8;
    for (int i = 0; i < arg->ty->size; i++) {
      println_abi(out, "  mov %d(%%rax), %%cl", i);
      println_abi(out, "  mov %%cl, %d(%%rsp)", i);
    }
  } else {
    println_abi(out, "  push %%rax");
    (*depth)++;
  }
}

static int win32_push_args(Node *node, FILE *out, int *depth) {
  int arg_count = 0;
  for (Node *arg = node->args; arg; arg = arg->next)
    arg_count++;

  bool ret_mem = node->ret_buffer && win32_returns_by_reference(node->ty);
  int total_words = arg_count + (ret_mem ? 1 : 0);

  int align_pad = (*depth + total_words) % 2;
  int total_stack_words = total_words + align_pad;

  if (align_pad > 0) {
    println_abi(out, "  sub $%d, %%rsp", align_pad * 8);
    *depth += align_pad;
  }

  win32_push_args_rev(node->args, out, depth);

  if (ret_mem) {
    println_abi(out, "  lea %d(%%rbp), %%rax", node->ret_buffer->offset);
    println_abi(out, "  push %%rax");
    (*depth)++;
  }

  return total_stack_words;
}

static void win32_copy_ret_buffer(Obj *var, FILE *out) {
  Type *ty = var->ty;
  if (ty->size <= 4) {
    for (int i = 0; i < ty->size; i++) {
      println_abi(out, "  mov %%al, %d(%%rbp)", var->offset + i);
      println_abi(out, "  shr $8, %%eax");
    }
  }
}

static void win32_copy_struct_reg(Obj *fn, FILE *out) {
  Type *ty = fn->ty->return_ty;
  println_abi(out, "  mov %%rax, %%rdi");
  println_abi(out, "  mov $0, %%eax");
  for (int i = ty->size - 1; i >= 0; i--) {
    println_abi(out, "  shl $8, %%eax");
    println_abi(out, "  mov %d(%%rdi), %%al", i);
  }
}

static void win32_copy_struct_mem(Obj *fn, FILE *out) {
  Type *ty = fn->ty->return_ty;
  Obj *var = fn->params;
  println_abi(out, "  mov %d(%%rbp), %%rdi", var->offset);
  for (int i = 0; i < ty->size; i++) {
    println_abi(out, "  mov %d(%%rax), %%dl", i);
    println_abi(out, "  mov %%dl, %d(%%rdi)", i);
  }
  println_abi(out, "  mov %d(%%rbp), %%rax", var->offset);
}

static void win32_builtin_alloca(Obj *fn, FILE *out) {
  println_abi(out, "  add $3, %%eax");
  println_abi(out, "  and $0xfffffffc, %%eax");
  println_abi(out, "  sub %%eax, %%esp");
  println_abi(out, "  mov %%esp, %%eax");
}

static void win32_emit_prologue(Obj *fn, FILE *out) {
  println_abi(out, "  push %%rbp");
  println_abi(out, "  mov %%rsp, %%rbp");
  if (fn->stack_size > 0)
    println_abi(out, "  sub $%d, %%rsp", fn->stack_size);
  println_abi(out, "  mov %%rsp, %d(%%rbp)", fn->alloca_bottom->offset);

  if (fn->va_area) {
    int named = 0;
    for (Obj *var = fn->params; var; var = var->next)
      named++;

    int off = fn->va_area->offset;
    println_abi(out, "  movl $48, %d(%%rbp)", off);
    println_abi(out, "  movl $176, %d(%%rbp)", off + 4);
    println_abi(out, "  lea %d(%%rbp), %%rax", 16 + named * 8);
    println_abi(out, "  movq %%rax, %d(%%rbp)", off + 8);
    println_abi(out, "  movq $0, %d(%%rbp)", off + 16);
  }
}

static void win32_emit_epilogue(Obj *fn, FILE *out) {
  println_abi(out, ".L.return.%s:", fn->name);
  println_abi(out, "  mov %%rbp, %%rsp");
  println_abi(out, "  pop %%rbp");
  println_abi(out, "  ret");
}

static void win32_define_macros(void) {
  define_macro("_WIN32", "1");
  define_macro("_M_IX86", "600");
  define_macro("__i386", "1");
  define_macro("__i386__", "1");
}

static void win32_init_types(void) {
  ty_char->size = 1; ty_char->align = 1;
  ty_short->size = 2; ty_short->align = 2;
  ty_int->size = 4; ty_int->align = 4;
  ty_long->size = 4; ty_long->align = 4;
  ty_llong->size = 8; ty_llong->align = 4;
  ty_uchar->size = 1; ty_uchar->align = 1;
  ty_ushort->size = 2; ty_ushort->align = 2;
  ty_uint->size = 4; ty_uint->align = 4;
  ty_ulong->size = 4; ty_ulong->align = 4;
  ty_ullong->size = 8; ty_ullong->align = 4;
  ty_float->size = 4; ty_float->align = 4;
  ty_double->size = 8; ty_double->align = 4;
  ty_ldouble->size = 8; ty_ldouble->align = 4;
}

ABI abi_win32 = {
  .name = "win32",
  .description = "Microsoft Windows x86 (32-bit cdecl) ABI",
  .default_objfmt = &objfmt_coff,
  .size_bool = 1, .align_bool = 1,
  .size_char = 1, .align_char = 1,
  .size_short = 2, .align_short = 2,
  .size_int = 4, .align_int = 4,
  .size_long = 4, .align_long = 4,
  .size_llong = 8, .align_llong = 4,
  .size_ptr = 4, .align_ptr = 4,
  .size_float = 4, .align_float = 4,
  .size_double = 8, .align_double = 4,
  .size_ldouble = 8, .align_ldouble = 4,
  .align_stack = 4,
  .va_area_size = 24,
  .va_area_align = 4,
  .returns_by_reference = win32_returns_by_reference,
  .classify_reg = win32_classify_reg,
  .assign_lvar_offsets = win32_assign_lvar_offsets,
  .push_args = win32_push_args,
  .copy_ret_buffer = win32_copy_ret_buffer,
  .copy_struct_reg = win32_copy_struct_reg,
  .copy_struct_mem = win32_copy_struct_mem,
  .builtin_alloca = win32_builtin_alloca,
  .emit_prologue = win32_emit_prologue,
  .emit_epilogue = win32_emit_epilogue,
  .define_macros = win32_define_macros,
  .init_types = win32_init_types,
};
