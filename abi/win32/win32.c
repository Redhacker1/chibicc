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

/*
 * Microsoft x86 __cdecl:
 *
 *   - arguments are passed right-to-left on the stack
 *   - every argument occupies at least one 32-bit stack slot
 *   - arguments smaller than 32 bits are widened to 32 bits
 *   - 64-bit scalar arguments occupy two 32-bit slots
 *   - 1/2/4-byte integer returns use EAX
 *   - 8-byte aggregate returns use EDX:EAX
 *   - other aggregate returns use a hidden return-buffer pointer
 *
 * See:
 *   https://learn.microsoft.com/en-us/cpp/cpp/argument-passing-and-naming-conventions
 */

static int win32_aggregate_return_kind(Type *ty) {
  if (ty->kind != TY_STRUCT && ty->kind != TY_UNION)
    return 0;

  if (ty->size == 1 || ty->size == 2 || ty->size == 4)
    return 1; // EAX

  if (ty->size == 8)
    return 2; // EDX:EAX

  return 3; // hidden pointer
}

static bool win32_returns_by_reference(Type *ty) {
  return win32_aggregate_return_kind(ty) == 3;
}

/*
 * There are no normal argument registers in Win32 __cdecl.
 *
 * Keep the classifier because the ABI interface expects one, but always
 * classify arguments as memory/stack arguments.
 */
static int win32_classify_reg(Type *ty) {
  (void)ty;
  return 2;
}

static int win32_stack_slot_size(Type *ty) {
  int size = ty->size;

  if (size < 4)
    size = 4;

  return align_to(size, 4);
}

static void win32_assign_lvar_offsets(Obj *fn) {
  if (!fn->is_function)
    return;

  /*
   * After:
   *
   *   push %ebp
   *   mov  %esp, %ebp
   *
   * 4(%ebp)  = return address
   * 8(%ebp)  = first stack argument
   */
  int top = 8;
  int bottom = 0;

  for (Obj *var = fn->params; var; var = var->next) {
    top = align_to(top, 4);
    var->offset = top;
    top += win32_stack_slot_size(var->ty);
  }

  for (Obj *var = fn->locals; var; var = var->next) {
    if (var->offset)
      continue;

    int align = var->align > 0 ? var->align : 4;

    /*
     * Win32 x86's fundamental stack alignment is 4 bytes. Individual
     * objects can still require stronger alignment if the type says so.
     */
    if (align < 4)
      align = 4;

    bottom += var->ty->size;
    bottom = align_to(bottom, align);
    var->offset = -bottom;
  }

  fn->stack_size = align_to(bottom, 4);
}

/*
 * Copy a scalar value from EAX into one 32-bit stack slot.
 *
 * The expression evaluator leaves scalar results in EAX for Win32.
 */
static void win32_push_scalar(Node *arg, FILE *out, int *depth) {
  if (arg->ty->size <= 4) {
    println_abi(out, "  push %%eax");
    (*depth)++;
    return;
  }

  /*
   * 64-bit integer / double values are represented in memory by the
   * expression generator. Copy the two 32-bit words explicitly.
   *
   * EAX contains the low word for integer-like values in the normal
   * scalar path. The backend's floating-point machinery may instead
   * materialize the value in memory, so use the common temporary stack
   * representation where appropriate.
   *
   * For now, use the address of the expression result when the node is
   * an lvalue/aggregate; ordinary 64-bit scalar expressions are handled
   * by the caller's normal value materialization.
   */
  if (arg->ty->kind == TY_DOUBLE || arg->ty->kind == TY_LDOUBLE ||
      arg->ty->size == 8) {
    /*
     * The common x86 code generator normally materializes an 8-byte
     * scalar on the stack for ABI argument passing. Copy it with two
     * 32-bit pushes.
     *
     * Do not use 64-bit registers here.
     */
    println_abi(out, "  pushl 4(%%eax)");
    println_abi(out, "  pushl (%%eax)");
    (*depth) += 2;
    return;
  }

  println_abi(out, "  push %%eax");
  (*depth)++;
}

static void win32_push_struct(Node *arg, FILE *out, int *depth) {
  int size = align_to(arg->ty->size, 4);

  /*
   * The expression result for an aggregate is an address in EAX.
   *
   * Copy in 4/2/1 byte chunks. This intentionally avoids 64-bit
   * instructions entirely.
   */
  println_abi(out, "  sub $%d, %%esp", size);
  *depth += size / 4;

  int i = 0;

  while (arg->ty->size - i >= 4) {
    println_abi(out, "  mov %d(%%eax), %%ecx", i);
    println_abi(out, "  mov %%ecx, %d(%%esp)", i);
    i += 4;
  }

  while (arg->ty->size - i >= 2) {
    println_abi(out, "  mov %d(%%eax), %%cx", i);
    println_abi(out, "  mov %%cx, %d(%%esp)", i);
    i += 2;
  }

  while (i < arg->ty->size) {
    println_abi(out, "  mov %d(%%eax), %%cl", i);
    println_abi(out, "  mov %%cl, %d(%%esp)", i);
    i++;
  }
}

static void win32_push_args_rev(Node *arg, FILE *out, int *depth) {
  if (!arg)
    return;

  win32_push_args_rev(arg->next, out, depth);

  if (current_codegen && current_codegen->gen_expr)
    ((void (*)(void *, FILE *))current_codegen->gen_expr)(arg, out);

  if (arg->ty->kind == TY_STRUCT || arg->ty->kind == TY_UNION)
    win32_push_struct(arg, out, depth);
  else
    win32_push_scalar(arg, out, depth);
}

static int win32_push_args(Node *node, FILE *out, int *depth) {
  int total_bytes = 0;

  /*
   * Calculate the actual outgoing argument area rather than assuming
   * one machine word per C argument.
   */
  for (Node *arg = node->args; arg; arg = arg->next)
    total_bytes += win32_stack_slot_size(arg->ty);

  bool ret_mem = node->ret_buffer &&
                 win32_returns_by_reference(node->ty);

  if (ret_mem)
    total_bytes += 4;

  /*
   * Win32 x86 only requires 4-byte stack alignment for this ABI.
   * No SysV/AMD64-style 16-byte adjustment is needed.
   *
   * Keep the existing expression-stack depth invariant by expressing
   * the outgoing area in 32-bit words.
   */
  win32_push_args_rev(node->args, out, depth);

  if (ret_mem) {
    println_abi(out, "  lea %d(%%ebp), %%eax",
                node->ret_buffer->offset);
    println_abi(out, "  push %%eax");
    (*depth)++;
  }

  return total_bytes / 4;
}

static void win32_copy_ret_buffer(Obj *var, FILE *out) {
  Type *ty = var->ty;

  /*
   * For small register returns, EAX contains the value.
   *
   * Only aggregate sizes 1/2/4 reach this path.
   */
  if (ty->size == 1) {
    println_abi(out, "  mov %%al, %d(%%ebp)", var->offset);
    return;
  }

  if (ty->size == 2) {
    println_abi(out, "  mov %%ax, %d(%%ebp)", var->offset);
    return;
  }

  if (ty->size == 4) {
    println_abi(out, "  mov %%eax, %d(%%ebp)", var->offset);
    return;
  }

  /*
   * Larger aggregates are returned through the hidden pointer and
   * therefore should not normally reach this helper.
   */
}

static void win32_copy_struct_reg(Obj *fn, FILE *out) {
  Type *ty = fn->ty->return_ty;

  if (ty->size == 1) {
    /*
     * EAX already contains the low byte.
     */
    return;
  }

  if (ty->size == 2) {
    /*
     * EAX already contains the low word.
     */
    return;
  }

  if (ty->size == 4) {
    /*
     * EAX already contains the complete aggregate.
     */
    return;
  }

  if (ty->size == 8) {
    /*
     * An 8-byte aggregate is returned in EDX:EAX.
     *
     * The common aggregate expression machinery produces the value in
     * memory at EAX. Load the low and high words from that address.
     */
    println_abi(out, "  mov 4(%%eax), %%edx");
    println_abi(out, "  mov (%%eax), %%eax");
    return;
  }
}

static void win32_copy_struct_mem(Obj *fn, FILE *out) {
  Type *ty = fn->ty->return_ty;
  Obj *var = fn->params;

  /*
   * The hidden return buffer is the first parameter.
   *
   * The caller passes its address as the first stack argument:
   *
   *   8(%ebp)
   */
  if (!var) {
    println_abi(out, "  ret");
    return;
  }

  println_abi(out, "  mov %d(%%ebp), %%edi", var->offset);

  int i = 0;

  while (ty->size - i >= 4) {
    println_abi(out, "  mov %d(%%eax), %%ecx", i);
    println_abi(out, "  mov %%ecx, %d(%%edi)", i);
    i += 4;
  }

  while (ty->size - i >= 2) {
    println_abi(out, "  mov %d(%%eax), %%cx", i);
    println_abi(out, "  mov %%cx, %d(%%edi)", i);
    i += 2;
  }

  while (i < ty->size) {
    println_abi(out, "  mov %d(%%eax), %%cl", i);
    println_abi(out, "  mov %%cl, %d(%%edi)", i);
    i++;
  }

  /*
   * Microsoft x86 aggregate-return convention uses the hidden pointer
   * as the return value in EAX as well.
   */
  println_abi(out, "  mov %%edi, %%eax");
}

static void win32_builtin_alloca(Obj *fn, FILE *out) {
  (void)fn;

  /*
   * __builtin_alloca(size)
   *
   * Keep the stack 4-byte aligned.
   */
  println_abi(out, "  add $3, %%eax");
  println_abi(out, "  and $-4, %%eax");
  println_abi(out, "  sub %%eax, %%esp");
  println_abi(out, "  mov %%esp, %%eax");
}

static void win32_emit_prologue(Obj *fn, FILE *out) {
  println_abi(out, "  push %%ebp");
  println_abi(out, "  mov %%esp, %%ebp");

  if (fn->stack_size > 0)
    println_abi(out, "  sub $%d, %%esp", fn->stack_size);

  if (fn->alloca_bottom) {
    println_abi(out, "  mov %%esp, %d(%%ebp)",
                fn->alloca_bottom->offset);
  }

  /*
   * Win32 x86 cdecl varargs are stack based.
   *
   * There is no SysV AMD64 gp_offset/fp_offset/register-save-area.
   * Store a pointer to the first unnamed argument instead.
   *
   * va_area is treated as a 4-byte pointer-sized object.
   */
  if (fn->va_area) {
    int named = 0;

    for (Obj *var = fn->params; var; var = var->next)
      named += win32_stack_slot_size(var->ty);

    /*
     * The first argument starts at 8(%ebp). Therefore the first
     * unnamed argument is:
     *
     *   8 + total size of named arguments
     */
    int off = fn->va_area->offset;

    println_abi(out, "  lea %d(%%ebp), %%eax",
                8 + named);
    println_abi(out, "  mov %%eax, %d(%%ebp)", off);
  }
}

static void win32_emit_epilogue(Obj *fn, FILE *out) {
  println_abi(out, ".L.return.%s:", fn->name);
  println_abi(out, "  mov %%ebp, %%esp");
  println_abi(out, "  pop %%ebp");
  println_abi(out, "  ret");
}

static void win32_emit_return(Obj *fn, Type *return_ty, FILE *out) {
  if (!return_ty && fn && fn->ty)
    return_ty = fn->ty->return_ty;
  if (!return_ty)
    return;

  if (return_ty->kind == TY_STRUCT || return_ty->kind == TY_UNION) {
    if (win32_returns_by_reference(return_ty)) {
      win32_copy_struct_mem(fn, out);
    } else {
      win32_copy_struct_reg(fn, out);
    }
  }
}

static Node *win32_builtin_va_arg(Node *ap, Type *ty, Token *tok) {
  int sz = align_to(MAX(4, ty->size), 4);
  return abi_va_arg_ptr(ap, ty, sz, tok);
}

static void win32_define_macros(void) {
  define_macro("_WIN32", "1");
  define_macro("_M_IX86", "600");
  define_macro("__i386", "1");
  define_macro("__i386__", "1");

  define_macro("__SIZEOF_LONG__", "4");
  define_macro("__SIZEOF_POINTER__", "4");
  define_macro("__SIZEOF_PTRDIFF_T__", "4");
  define_macro("__SIZEOF_SIZE_T__", "4");

  define_macro("__SIZE_TYPE__", "unsigned int");
  define_macro("__PTRDIFF_TYPE__", "int");
  define_macro("__INTPTR_TYPE__", "int");
  define_macro("__UINTPTR_TYPE__", "unsigned int");

  define_macro("__INTMAX_TYPE__", "long long");
  define_macro("__UINTMAX_TYPE__", "unsigned long long");

  define_macro("__WCHAR_TYPE__", "unsigned short");
  define_macro("__WINT_TYPE__", "unsigned short");
}

static void win32_init_types(void) {
  ty_char->size = 1;
  ty_char->align = 1;

  ty_short->size = 2;
  ty_short->align = 2;

  ty_int->size = 4;
  ty_int->align = 4;

  ty_long->size = 4;
  ty_long->align = 4;

  ty_llong->size = 8;
  ty_llong->align = 4;

  ty_uchar->size = 1;
  ty_uchar->align = 1;

  ty_ushort->size = 2;
  ty_ushort->align = 2;

  ty_uint->size = 4;
  ty_uint->align = 4;

  ty_ulong->size = 4;
  ty_ulong->align = 4;

  ty_ullong->size = 8;
  ty_ullong->align = 4;

  ty_float->size = 4;
  ty_float->align = 4;

  ty_double->size = 8;
  ty_double->align = 4;

  /*
   * MSVC's traditional x86 long double is the same size/alignment
   * as double.
   */
  ty_ldouble->size = 8;
  ty_ldouble->align = 4;
}

static void win32_declare_builtin_types(void) {
  push_scope("__builtin_va_list")->type_def = pointer_to(ty_char);
}

ABI abi_win32 = {
  .name = "win32",
  .description = "Microsoft Windows x86 (32-bit cdecl) ABI",

  .default_objfmt = &objfmt_coff,

  .size_bool = 1,
  .align_bool = 1,

  .size_char = 1,
  .align_char = 1,

  .size_short = 2,
  .align_short = 2,

  .size_int = 4,
  .align_int = 4,

  .size_long = 4,
  .align_long = 4,

  .size_llong = 8,
  .align_llong = 4,

  .size_ptr = 4,
  .align_ptr = 4,

  .size_float = 4,
  .align_float = 4,

  .size_double = 8,
  .align_double = 4,

  .size_ldouble = 8,
  .align_ldouble = 4,

  .align_stack = 4,

  /*
   * Win32 va_list is fundamentally just a pointer into the argument
   * stack. The storage is therefore pointer-sized.
   */
  .va_area_size = 4,
  .va_area_align = 4,

  .returns_by_reference = win32_returns_by_reference,
  .classify_reg = win32_classify_reg,

  .assign_lvar_offsets = win32_assign_lvar_offsets,
  .push_args = win32_push_args,

  .copy_ret_buffer = win32_copy_ret_buffer,
  .copy_struct_reg = win32_copy_struct_reg,
  .copy_struct_mem = win32_copy_struct_mem,

  .builtin_alloca = win32_builtin_alloca,

  .builtin_va_start = abi_va_start_ptr,
  .builtin_va_arg = win32_builtin_va_arg,
  .builtin_va_copy = abi_va_copy_ptr,
  .builtin_va_end = abi_va_end_nop,

  .emit_prologue = win32_emit_prologue,
  .emit_epilogue = win32_emit_epilogue,
  .emit_return = win32_emit_return,

  .define_macros = win32_define_macros,
  .init_types = win32_init_types,
  .declare_builtin_types = win32_declare_builtin_types,
};
