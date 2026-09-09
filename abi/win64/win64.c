#include "chibicc.h"
#include "abi/abi.h"
#include "codegen/common/common.h"

#define WIN64_REG_MAX 4
#define WIN64_SHADOW_SPACE 32

static char *win64_argreg8[] = {
  "%cl", "%dl", "%r8b", "%r9b"
};

static char *win64_argreg16[] = {
  "%cx", "%dx", "%r8w", "%r9w"
};

static char *win64_argreg32[] = {
  "%ecx", "%edx", "%r8d", "%r9d"
};

static char *win64_argreg64[] = {
  "%rcx", "%rdx", "%r8", "%r9"
};

static void println_abi(FILE *out, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(out, fmt, ap);
  va_end(ap);
  fprintf(out, "\n");
}

/*
 * Win64 has a very simple aggregate rule:
 *
 *   1, 2, 4, 8 bytes  -> passed/returned in a GP register
 *   everything else   -> passed/returned by reference
 *
 * This is intentionally a C-oriented interpretation.  The more complicated
 * C++ POD restrictions in Microsoft's ABI aren't relevant to chibicc.
 */
static bool win64_returns_by_reference(Type *ty) {
  if (ty->kind != TY_STRUCT && ty->kind != TY_UNION)
    return false;

  return ty->size != 1 &&
         ty->size != 2 &&
         ty->size != 4 &&
         ty->size != 8;
}

/*
 * This is intentionally only a coarse classification.
 *
 * Win64 does NOT have the SysV "eightbyte" aggregate classification scheme.
 * Every argument consumes exactly one positional argument slot.
 *
 * 0 = GP register
 * 1 = XMM register
 * 2 = indirect/by-reference
 */
static int win64_classify_reg(Type *ty) {
  if ((ty->kind == TY_STRUCT || ty->kind == TY_UNION) &&
      win64_returns_by_reference(ty))
    return 2;

  if (is_integer(ty) || ty->kind == TY_PTR)
    return 0;

  if (is_flonum(ty))
    return 1;

  if (ty->kind == TY_STRUCT || ty->kind == TY_UNION)
    return 0;

  return 2;
}

/*
 * Return the GP register name appropriate for a value of SIZE bytes.
 */
static char *win64_gp_reg(int idx, int size) {
  switch (size) {
  case 1:
    return win64_argreg8[idx];
  case 2:
    return win64_argreg16[idx];
  case 4:
    return win64_argreg32[idx];
  case 8:
    return win64_argreg64[idx];
  default:
    unreachable();
  }
}

/*
 * Store a register argument into its local/home location without writing
 * beyond the size of the actual C object.
 *
 * The previous implementation always used movq here, which is wrong for
 * 1/2/4-byte parameters because the home slot may overlap another local.
 */
static void win64_store_gp_param(int idx, int size, int offset, FILE *out) {
  println_abi(out, "  mov %s, %d(%%rbp)",
              win64_gp_reg(idx, size), offset);
}

/*
 * Copy SIZE bytes from R10 to R11.
 *
 * R10/R11 are volatile under Win64, unlike RDI/RSI.
 */
static void win64_copy_bytes(char *src, char *dst, int size, FILE *out) {
  int i = 0;

  while (size - i >= 8) {
    println_abi(out, "  mov %d(%%r10), %%rax", i);
    println_abi(out, "  mov %%rax, %d(%%r11)", i);
    i += 8;
  }

  while (size - i >= 4) {
    println_abi(out, "  mov %d(%%r10), %%eax", i);
    println_abi(out, "  mov %%eax, %d(%%r11)", i);
    i += 4;
  }

  while (size - i >= 2) {
    println_abi(out, "  mov %d(%%r10), %%ax", i);
    println_abi(out, "  mov %%ax, %d(%%r11)", i);
    i += 2;
  }

  while (i < size) {
    println_abi(out, "  mov %d(%%r10), %%al", i);
    println_abi(out, "  mov %%al, %d(%%r11)", i);
    i++;
  }
}

/*
 * Count the temporary storage required for aggregate arguments which are
 * passed by reference.
 *
 * Microsoft requires caller-created temporaries for indirect aggregate
 * arguments to be 16-byte aligned.
 */
static int win64_temp_bytes(Node *args) {
  int size = 0;

  for (Node *arg = args; arg; arg = arg->next) {
    if ((arg->ty->kind == TY_STRUCT || arg->ty->kind == TY_UNION) &&
        win64_returns_by_reference(arg->ty)) {
      size = align_to(size, 16);
      size += align_to(arg->ty->size, 16);
    }
  }

  return size;
}

/*
 * Find the offset of ARG inside the temporary area.
 */
static int win64_temp_offset(Node *args, Node *target) {
  int offset = 0;

  for (Node *arg = args; arg; arg = arg->next) {
    if ((arg->ty->kind == TY_STRUCT || arg->ty->kind == TY_UNION) &&
        win64_returns_by_reference(arg->ty)) {
      offset = align_to(offset, 16);

      if (arg == target)
        return offset;

      offset += align_to(arg->ty->size, 16);
    }
  }

  unreachable();
}

/*
 * Push one logical argument.
 *
 * For large aggregates, the value is copied into a caller-owned, 16-byte
 * aligned temporary.  The address of that temporary is what is passed.
 *
 * TEMP_BYTES is the total temporary area allocated at the top of the
 * outgoing call frame.
 *
 * CALL_PAD is the padding between the argument slots and the temporary
 * area.
 *
 * PUSHED is the number of user arguments already pushed (arguments to the
 * right of ARG).
 */
static void win64_push_arg_expr(
    Node *arg,
    Node *all_args,
    FILE *out,
    int *depth,
    int temp_bytes,
    int call_pad,
    int extra_shadow,
    int pushed)
{
  current_codegen->gen_expr(arg, out);

  /*
   * Small structs/unions are passed as integers in a GP register.
   *
   * gen_expr() leaves the address of an aggregate in RAX, so load the
   * aggregate into RAX before putting it into its argument slot.
   */
  if (arg->ty->kind == TY_STRUCT || arg->ty->kind == TY_UNION) {
    if (arg->ty->size == 1) {
      println_abi(out, "  movzbq (%%rax), %%rax");
    } else if (arg->ty->size == 2) {
      println_abi(out, "  movzwq (%%rax), %%rax");
    } else if (arg->ty->size == 4) {
      println_abi(out, "  movl (%%rax), %%eax");
    } else if (arg->ty->size == 8) {
      println_abi(out, "  movq (%%rax), %%rax");
    }
  }

  /*
   * Large aggregates are passed indirectly.
   *
   * Make a real caller-owned copy rather than simply passing whatever
   * address happened to be returned by gen_expr().
   */
  if ((arg->ty->kind == TY_STRUCT || arg->ty->kind == TY_UNION) &&
      win64_returns_by_reference(arg->ty)) {
    int temp_off = win64_temp_offset(all_args, arg);

    /*
     * At this point:
     *
     *   rsp + (extra_shadow + call_pad + pushed) * 8
     *
     * points at the beginning of the temporary area.
     */
    int dst_off = (extra_shadow + call_pad + pushed) * 8 + temp_off;

    println_abi(out, "  mov %%rax, %%r10");
    println_abi(out, "  lea %d(%%rsp), %%r11", dst_off);

    win64_copy_bytes(NULL, NULL, arg->ty->size, out);

    println_abi(out, "  mov %%r11, %%rax");
    println_abi(out, "  push %%rax");
    (*depth)++;
    return;
  }

  /*
   * Floating point arguments occupy one 8-byte stack slot.
   */
  if (is_flonum(arg->ty)) {
    println_abi(out, "  sub $8, %%rsp");
    if (arg->ty->size == 4)
      println_abi(out, "  movss %%xmm0, (%%rsp)");
    else
      println_abi(out, "  movsd %%xmm0, (%%rsp)");
    (*depth)++;
    return;
  }

  println_abi(out, "  push %%rax");
  (*depth)++;
}

static void win64_push_args_rev(
    Node *arg,
    Node *all_args,
    FILE *out,
    int *depth,
    int temp_bytes,
    int call_pad,
    int extra_shadow,
    int *pushed)
{
  if (!arg)
    return;

  /*
   * Evaluate right-to-left, as the existing chibicc codegen expects.
   */
  win64_push_args_rev(
      arg->next,
      all_args,
      out,
      depth,
      temp_bytes,
      call_pad,
      extra_shadow,
      pushed);

  win64_push_arg_expr(
      arg,
      all_args,
      out,
      depth,
      temp_bytes,
      call_pad,
      extra_shadow,
      *pushed);

  (*pushed)++;
}

static int win64_count_params(Type *ty) {
  int count = 0;

  if (!ty)
    return 0;

  for (Type *p = ty->params; p; p = p->next)
    count++;

  return count;
}

static void win64_assign_lvar_offsets(Obj *fn) {
  if (!fn->is_function)
    return;

  /*
   * +0  return address
   * +8  caller's RBP
   * +16 home slot for argument position 0
   * +24 home slot for argument position 1
   * +32 home slot for argument position 2
   * +40 home slot for argument position 3
   * +48 first stack argument
   *
   * This also naturally handles the hidden sret parameter because it is
   * present in fn->params and therefore consumes argument position 0.
   */
  int top = 48;
  int bottom = 0;
  int param_idx = 0;

  for (Obj *var = fn->params; var; var = var->next) {
    /*
     * Large aggregates are passed as pointers, but the C parameter itself
     * is still a local aggregate object. Allocate storage for the local
     * copy rather than assigning it a positive stack/home offset.
     */
    if ((var->ty->kind == TY_STRUCT || var->ty->kind == TY_UNION) &&
        win64_returns_by_reference(var->ty)) {
      int align = var->align;

      bottom += var->ty->size;
      bottom = align_to(bottom, align);
      var->offset = -bottom;
    } else if (param_idx < WIN64_REG_MAX) {
      /*
       * Register parameters have an ABI-mandated home slot.
       */
      var->offset = 16 + param_idx * 8;
    } else {
      /*
       * Every stack argument consumes exactly one 8-byte argument slot.
       *
       * The old code advanced by sizeof(type), which is wrong for indirect
       * aggregates such as a 16-byte struct: that parameter is a pointer and
       * consumes only one slot.
       */
      var->offset = top;
      top += 8;
    }

    param_idx++;
  }

  /*
   * Local variables.
   */
  for (Obj *var = fn->locals; var; var = var->next) {
    if (var->offset)
      continue;

    int align = var->align;

    bottom += var->ty->size;
    bottom = align_to(bottom, align);
    var->offset = -bottom;
  }

  /*
   * Reserve the minimum outgoing home space in the fixed frame.
   *
   * win64_push_args() may allocate additional stack argument slots
   * dynamically when needed.
   */
  fn->stack_size = align_to(bottom + WIN64_SHADOW_SPACE, 16);
}

/*
 * Win64 argument lowering.
 *
 * The resulting stack layout immediately before CALL is:
 *
 *   rsp +  0  home slot RCX
 *   rsp +  8  home slot RDX
 *   rsp + 16  home slot R8
 *   rsp + 24  home slot R9
 *   rsp + 32  argument slot #4
 *   rsp + 40  argument slot #5
 *   ...
 *
 * The call itself then pushes the return address.
 */
static int win64_push_args(Node *node, FILE *out, int *depth) {
  int arg_count = 0;

  for (Node *arg = node->args; arg; arg = arg->next)
    arg_count++;

  bool ret_mem =
      node->ret_buffer &&
      win64_returns_by_reference(node->ty);

  /*
   * The hidden sret pointer is argument position zero.
   */
  int total_args = arg_count + (ret_mem ? 1 : 0);

  /*
   * Caller-owned temporaries for indirect aggregates.
   */
  int temp_bytes = win64_temp_bytes(node->args);
  int temp_words = temp_bytes / 8;

  /*
   * If we have aggregate temporaries, make their base 16-byte aligned.
   *
   * depth counts 8-byte stack units. The fixed function frame is 16-byte
   * aligned, so an odd depth means RSP is currently 8 bytes off alignment.
   */
  int pre_pad = 0;

  if (temp_bytes && (*depth & 1)) {
    println_abi(out, "  sub $8, %%rsp");
    (*depth)++;
    pre_pad = 1;
  }

  /*
   * Allocate temporary storage before pushing arguments.
   *
   * It ends up ABOVE the argument area, so it doesn't disturb the ABI
   * offsets used below.
   */
  if (temp_bytes) {
    println_abi(out, "  sub $%d, %%rsp", temp_bytes);
    (*depth) += temp_words;
  }

  /*
   * At least four home slots (32 bytes shadow space) are always required.
   * Total slots is max(total_args, 4).
   * We place the final alignment padding between the argument slots and
   * the temporary area.
   */
  int total_arg_slots = total_args > WIN64_REG_MAX ? total_args : WIN64_REG_MAX;
  int extra_shadow = total_arg_slots - total_args;

  int call_pad =
      (*depth + total_arg_slots) & 1;

  if (call_pad) {
    println_abi(out, "  sub $8, %%rsp");
    (*depth)++;
  }

  if (extra_shadow > 0) {
    println_abi(out, "  sub $%d, %%rsp", extra_shadow * 8);
    (*depth) += extra_shadow;
  }

  /*
   * Push user arguments right-to-left.
   */
  int pushed = 0;

  win64_push_args_rev(
      node->args,
      node->args,
      out,
      depth,
      temp_bytes,
      call_pad,
      extra_shadow,
      &pushed);

  /*
   * The hidden structure-return pointer is the FIRST argument, so it must
   * be pushed last. This puts it at slot 0 (rsp + 0).
   */
  if (ret_mem) {
    println_abi(
        out,
        "  lea %d(%%rbp), %%rax",
        node->ret_buffer->offset);
    println_abi(out, "  push %%rax");
    (*depth)++;
  }

  /*
   * Determine whether this call has a variadic/unprototyped tail.
   *
   * For a prototyped variadic function:
   *
   *   f(int, ...)
   *
   * only arguments after the named parameters need the GP/XMM duplication.
   *
   * For an unprototyped function (params == NULL), every argument is
   * treated as a variadic argument.
   */
  bool is_variadic =
      node->func_ty &&
      node->func_ty->is_variadic;

  int named_args =
      node->func_ty ?
      win64_count_params(node->func_ty) : 0;

  /*
   * Load register arguments from the stack slots we just constructed.
   *
   * slot 0 corresponds to the hidden sret pointer when present.
   */
  int slot = 0;

  if (ret_mem) {
    println_abi(out, "  mov 0(%%rsp), %%rcx");
    slot = 1;
  }

  int arg_index = 0;

  for (Node *arg = node->args;
       arg && slot < WIN64_REG_MAX;
       arg = arg->next, arg_index++, slot++) {

    int off = slot * 8;

    /*
     * A floating point argument uses the XMM register corresponding to
     * its POSITION, not the ordinal number of floating point arguments.
     */
    if (is_flonum(arg->ty)) {
      if (arg->ty->size == 4)
        println_abi(
            out,
            "  movss %d(%%rsp), %%xmm%d",
            off, slot);
      else
        println_abi(
            out,
            "  movsd %d(%%rsp), %%xmm%d",
            off, slot);

      /*
       * Win64 varargs/unprototyped calls duplicate FP values in the
       * corresponding GP register.
       */
      bool vararg_arg =
          is_variadic && arg_index >= named_args;

      if (vararg_arg) {
        if (arg->ty->size == 4) {
          println_abi(
              out,
              "  mov %d(%%rsp), %s",
              off,
              win64_argreg32[slot]);
        } else {
          println_abi(
              out,
              "  mov %d(%%rsp), %s",
              off,
              win64_argreg64[slot]);
        }
      }
    } else {
      println_abi(
          out,
          "  mov %d(%%rsp), %s",
          off,
          win64_argreg64[slot]);
    }
  }

  /*
   * Account for all the storage we created.
   *
   * This is useful to the caller-side cleanup logic.
   */
  return pre_pad +
         temp_words +
         call_pad +
         total_arg_slots;
}

/*
 * Copy an RAX integer aggregate result into the caller's return buffer.
 */
static void win64_copy_ret_buffer(Obj *var, FILE *out) {
  Type *ty = var->ty;

  if (ty->size <= 8) {
    for (int i = 0; i < ty->size; i++) {
      println_abi(
          out,
          "  mov %%al, %d(%%rbp)",
          var->offset + i);
      println_abi(out, "  shr $8, %%rax");
    }
  }
}

/*
 * Convert the address in RAX, which points at a small returned aggregate,
 * into the integer representation required by Win64.
 *
 * Do NOT use RDI/RSI here. They are nonvolatile on Win64.
 */
static void win64_copy_struct_reg(Obj *fn, FILE *out) {
  Type *ty = fn->ty->return_ty;

  println_abi(out, "  mov %%rax, %%r10");
  println_abi(out, "  xor %%eax, %%eax");

  for (int i = ty->size - 1; i >= 0; i--) {
    println_abi(out, "  shl $8, %%rax");
    println_abi(out, "  mov %d(%%r10), %%al", i);
  }
}

/*
 * Copy a large returned aggregate into the caller-provided return buffer.
 *
 * fn->params->offset is the hidden sret parameter.
 */
static void win64_copy_struct_mem(Obj *fn, FILE *out) {
  Type *ty = fn->ty->return_ty;
  Obj *var = fn->params;

  /*
   * RAX contains the source address.
   *
   * The hidden return-buffer pointer is the first parameter, hence its
   * home location is var->offset.
   */
  println_abi(out, "  mov %d(%%rbp), %%r11", var->offset);

  /*
   * Preserve the source pointer in R10 and use R11 as destination.
   */
  println_abi(out, "  mov %%rax, %%r10");

  win64_copy_bytes(NULL, NULL, ty->size, out);

  /*
   * Microsoft x64 ABI requires the hidden return-buffer pointer in RAX on return.
   */
  println_abi(out, "  mov %d(%%rbp), %%rax", var->offset);
}

/*
 * Win64 alloca().
 *
 * RSP is kept relative to RBP, so normal parameter references remain
 * stable after the dynamic allocation.
 */
static void win64_builtin_alloca(Obj *fn, FILE *out) {
  (void)fn;

  println_abi(out, "  add $15, %%rcx");
  println_abi(out, "  and $0xfffffff0, %%rcx");
  println_abi(out, "  sub %%rcx, %%rsp");
  println_abi(out, "  mov %%rsp, %%rax");
}

/*
 * Emit the Win64 prologue.
 */
static void win64_emit_prologue(Obj *fn, FILE *out) {
  println_abi(out, "  push %%rbp");
  println_abi(out, "  mov %%rsp, %%rbp");

  /*
   * Windows requires stack probing for sufficiently large fixed stack
   * allocations.
   *
   * __chkstk is a special prologue helper and preserves the argument
   * registers and RAX while probing the allocation.
   */
  if (fn->stack_size >= 4096) {
    println_abi(out, "  mov $%d, %%rax", fn->stack_size);
    println_abi(out, "  call __chkstk");
    println_abi(out, "  sub %%rax, %%rsp");
  } else {
    println_abi(
        out,
        "  sub $%d, %%rsp",
        fn->stack_size);
  }

  println_abi(
      out,
      "  mov %%rsp, %d(%%rbp)",
      fn->alloca_bottom->offset);

  /*
   * For variadic functions, save all four GP argument registers into
   * their home slots.
   *
   * Win64 varargs use the home/stack argument area as one linear sequence.
   * Floating-point varargs are duplicated into the GP registers by the
   * caller, so saving the GP registers is sufficient for va_arg().
   */
  if (fn->va_area) {
    for (int i = 0; i < WIN64_REG_MAX; i++) {
      println_abi(
          out,
          "  mov %s, %d(%%rbp)",
          win64_argreg64[i],
          16 + i * 8);
    }
  }

  /*
   * Spill named register parameters.
   *
   * fn->params already contains the hidden sret parameter when one exists,
   * so its presence automatically shifts all subsequent parameters by one
   * positional slot.
   */
  int idx = 0;

  for (Obj *var = fn->params; var; var = var->next, idx++) {
    if (idx >= WIN64_REG_MAX)
      break;

    if ((var->ty->kind == TY_STRUCT ||
         var->ty->kind == TY_UNION) &&
        win64_returns_by_reference(var->ty)) {

      /*
       * The register contains a pointer to the caller-created temporary.
       * Copy the aggregate into the local C parameter object.
       */
      println_abi(
          out,
          "  mov %s, %%r10",
          win64_argreg64[idx]);

      println_abi(
          out,
          "  lea %d(%%rbp), %%r11",
          var->offset);

      win64_copy_bytes(
          NULL,
          NULL,
          var->ty->size,
          out);

      continue;
    }

    if (is_flonum(var->ty)) {
      if (var->ty->size == 4) {
        println_abi(
            out,
            "  movss %%xmm%d, %d(%%rbp)",
            idx,
            var->offset);
      } else {
        println_abi(
            out,
            "  movsd %%xmm%d, %d(%%rbp)",
            idx,
            var->offset);
      }

      continue;
    }

    /*
     * Integer/pointer/small aggregate parameters are stored using their
     * actual size rather than blindly writing eight bytes.
     */
    win64_store_gp_param(
        idx,
        var->ty->size,
        var->offset,
        out);
  }

  /*
   * Stack-passed large aggregates are passed as pointers too.
   *
   * Every stack parameter consumes exactly one 8-byte slot.
   */
  idx = 0;

  for (Obj *var = fn->params; var; var = var->next, idx++) {
    if (idx < WIN64_REG_MAX)
      continue;

    if ((var->ty->kind == TY_STRUCT ||
         var->ty->kind == TY_UNION) &&
        win64_returns_by_reference(var->ty)) {

      int stack_off =
          48 + (idx - WIN64_REG_MAX) * 8;

      println_abi(
          out,
          "  mov %d(%%rbp), %%r10",
          stack_off);

      println_abi(
          out,
          "  lea %d(%%rbp), %%r11",
          var->offset);

      win64_copy_bytes(
          NULL,
          NULL,
          var->ty->size,
          out);
    }
  }

  /*
   * Initialize our internal Win64 va_list representation.
   *
   * The first word is a pointer to the first unnamed argument.
   *
   * Your ABI descriptor deliberately remains:
   *
   *     .va_area_size = 24
   *     .va_area_align = 8
   *
   * so we only use the first eight bytes here and leave the remaining
   * 16 bytes available to the generic ABI machinery.
   */
  if (fn->va_area) {
    int named = 0;

    for (Obj *var = fn->params; var; var = var->next)
      named++;

    int first_off = 16 + named * 8;
    int off = fn->va_area->offset;

    println_abi(out, "  movl $48, %d(%%rbp)", off);
    println_abi(out, "  movl $176, %d(%%rbp)", off + 4);
    println_abi(out, "  lea %d(%%rbp), %%rax", first_off);
    println_abi(out, "  movq %%rax, %d(%%rbp)", off + 8);
    println_abi(out, "  movq $0, %d(%%rbp)", off + 16);
  }
}

static void win64_emit_epilogue(Obj *fn, FILE *out) {
  println_abi(out, ".L.return.%s:", fn->name);
  println_abi(out, "  mov %%rbp, %%rsp");
  println_abi(out, "  pop %%rbp");
  println_abi(out, "  ret");
}

static void win64_define_macros(void) {
  define_macro("_WIN64", "1");
  define_macro("_WIN32", "1");
  define_macro("_M_X64", "100");
  define_macro("_M_AMD64", "100");
  define_macro("__x86_64", "1");
  define_macro("__x86_64__", "1");
}

static void win64_init_types(void) {
  ty_char->size = 1; ty_char->align = 1;
  ty_short->size = 2; ty_short->align = 2;
  ty_int->size = 4; ty_int->align = 4;
  ty_long->size = 4; ty_long->align = 4;
  ty_llong->size = 8; ty_llong->align = 8;

  ty_uchar->size = 1; ty_uchar->align = 1;
  ty_ushort->size = 2; ty_ushort->align = 2;
  ty_uint->size = 4; ty_uint->align = 4;
  ty_ulong->size = 4; ty_ulong->align = 4;
  ty_ullong->size = 8; ty_ullong->align = 8;

  ty_float->size = 4; ty_float->align = 4;
  ty_double->size = 8; ty_double->align = 8;
  ty_ldouble->size = 8; ty_ldouble->align = 8;
}

static const CallConv win64_callconv = {
  .name = "win64",
  .num_gp_regs = 4,
  .gp_regs64 = { "%rcx", "%rdx", "%r8", "%r9" },
  .gp_regs32 = { "%ecx", "%edx", "%r8d", "%r9d" },
  .gp_regs16 = { "%cx", "%dx", "%r8w", "%r9w" },
  .gp_regs8  = { "%cl", "%dl", "%r8b", "%r9b" },
  .num_fp_regs = 4,
  .shadow_space = 32,
  .paired_slots = true,
  .pass_struct_by_ref = true,
  .stack_align = 16,
};

ABI abi_win64 = {
  .name = "win64",
  .description = "Microsoft Windows x64 ABI",
  .callconv = &win64_callconv,
  .default_objfmt = &objfmt_coff,
  .size_bool = 1, .align_bool = 1,
  .size_char = 1, .align_char = 1,
  .size_short = 2, .align_short = 2,
  .size_int = 4, .align_int = 4,
  .size_long = 4, .align_long = 4,
  .size_llong = 8, .align_llong = 8,
  .size_ptr = 8, .align_ptr = 8,
  .size_float = 4, .align_float = 4,
  .size_double = 8, .align_double = 8,
  .size_ldouble = 8, .align_ldouble = 8,
  .align_stack = 16,
  .va_area_size = 24,
  .va_area_align = 8,
  .returns_by_reference = win64_returns_by_reference,
  .classify_reg = win64_classify_reg,
  .assign_lvar_offsets = win64_assign_lvar_offsets,
  .push_args = win64_push_args,
  .copy_ret_buffer = win64_copy_ret_buffer,
  .copy_struct_reg = win64_copy_struct_reg,
  .copy_struct_mem = win64_copy_struct_mem,
  .builtin_alloca = win64_builtin_alloca,
  .emit_prologue = win64_emit_prologue,
  .emit_epilogue = win64_emit_epilogue,
  .define_macros = win64_define_macros,
  .init_types = win64_init_types,
};
