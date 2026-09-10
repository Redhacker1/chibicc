#include "chibicc.h"
#include "abi/abi.h"
#include "codegen/common/common.h"
#include "ir/ir.h"

extern Obj *current_fn;

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
  char buf[2048];
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  codegen_println(out, "%s", buf);
}

/*
 * Microsoft x64:
 *
 *   struct/union of 1, 2, 4, or 8 bytes:
 *       passed/returned as an integer value.
 *
 *   all other structs/unions:
 *       passed by pointer to caller-provided temporary storage.
 *
 * For the C types supported by chibicc this is sufficient.
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
 * Classification:
 *
 *   0 = GP register
 *   1 = XMM register
 *   2 = indirect
 *
 * Win64 uses positional registers. There is no independent GP/FP
 * register allocation like SysV AMD64.
 */
static int win64_classify_reg(Type *ty) {
  if ((ty->kind == TY_STRUCT || ty->kind == TY_UNION) &&
      win64_returns_by_reference(ty))
    return 2;

  if (is_flonum(ty))
    return 1;

  if (is_integer(ty) || ty->kind == TY_PTR)
    return 0;

  if (ty->kind == TY_STRUCT || ty->kind == TY_UNION)
    return 0;

  return 2;
}

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
 * Copy an arbitrary aggregate using the smallest useful number of
 * instructions. R10 = source, R11 = destination.
 *
 * Both registers are volatile under Win64.
 */
static void win64_copy_bytes(int size, FILE *out) {
  int i = 0;

  while (size - i >= 8) {
    println_abi(out, "  mov %d(%%r10), %%rax", i);
    println_abi(out, "  mov %%rax, %d(%%r11)", i);
    i += 8;
  }

  if (size - i >= 4) {
    println_abi(out, "  mov %d(%%r10), %%eax", i);
    println_abi(out, "  mov %%eax, %d(%%r11)", i);
    i += 4;
  }

  if (size - i >= 2) {
    println_abi(out, "  mov %d(%%r10), %%ax", i);
    println_abi(out, "  mov %%ax, %d(%%r11)", i);
    i += 2;
  }

  if (i < size) {
    println_abi(out, "  mov %d(%%r10), %%al", i);
    println_abi(out, "  mov %%al, %d(%%r11)", i);
  }
}

/*
 * Caller-owned temporaries for large aggregate arguments.
 *
 * Win64 requires these temporaries to be 16-byte aligned.
 */
static int win64_temp_bytes(const Node *args) {
  int size = 0;

  for (const Node *arg = args; arg; arg = arg->next) {
    if ((arg->ty->kind == TY_STRUCT || arg->ty->kind == TY_UNION) &&
        win64_returns_by_reference(arg->ty)) {
      size = align_to(size, 16);
      size += align_to(arg->ty->size, 16);
    }
  }

  return size;
}

static int win64_temp_offset(const Node *args, const Node *target) {
  int offset = 0;

  for (const Node *arg = args; arg; arg = arg->next) {
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
 * Evaluate an argument and put its representation on the temporary
 * argument stack.
 *
 * Arguments are evaluated right-to-left. This means that after all
 * arguments have been pushed:
 *
 *   [rsp + 32] = argument slot 0
 *   [rsp + 40] = argument slot 1
 *   ...
 *
 * The first four slots are subsequently copied into RCX/RDX/R8/R9 or
 * XMM0-XMM3.
 */
static void win64_push_arg_expr(
    Node *arg,
    Node *all_args,
    FILE *out,
    int *depth,
    int call_pad,
    int shadow_pad,
    int pushed)
{
  if (current_codegen && current_codegen->gen_expr)
    ((void (*)(void *, FILE *))current_codegen->gen_expr)(arg, out);

  /*
   * Small structs are represented by their value in RAX.
   * gen_expr() leaves a struct expression as an address.
   */
  if (arg->ty->kind == TY_STRUCT || arg->ty->kind == TY_UNION) {
    switch (arg->ty->size) {
    case 1:
      println_abi(out, "  movzbq (%%rax), %%rax");
      break;
    case 2:
      println_abi(out, "  movzwq (%%rax), %%rax");
      break;
    case 4:
      println_abi(out, "  movl (%%rax), %%eax");
      break;
    case 8:
      println_abi(out, "  movq (%%rax), %%rax");
      break;
    default:
      break;
    }
  }

  /*
   * Large aggregate:
   *
   * Make a 16-byte-aligned temporary, copy the value into it, then
   * push the address of that temporary as the actual argument.
   *
   * The temporary area was allocated before argument evaluation, so
   * it cannot overlap the argument slots.
   */
  if ((arg->ty->kind == TY_STRUCT || arg->ty->kind == TY_UNION) &&
      win64_returns_by_reference(arg->ty)) {
    int temp_off = win64_temp_offset(all_args, arg);

    /*
     * Current RSP is below the temporary area by:
     *
     *   shadow pad + alignment pad + already pushed arguments
     */
    int dst_off =
        shadow_pad * 8 +
        call_pad * 8 +
        pushed * 8 +
        temp_off;

    println_abi(out, "  mov %%rax, %%r10");
    println_abi(out, "  lea %d(%%rsp), %%r11", dst_off);
    win64_copy_bytes(arg->ty->size, out);

    println_abi(out, "  push %%r11");
    (*depth)++;
    return;
  }

  /*
   * Keep floating point arguments in an 8-byte stack slot.
   * This also makes the subsequent register loading uniform.
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
    int call_pad,
    int shadow_pad,
    int *pushed)
{
  if (!arg)
    return;

  win64_push_args_rev(
      arg->next,
      all_args,
      out,
      depth,
      call_pad,
      shadow_pad,
      pushed);

  win64_push_arg_expr(
      arg,
      all_args,
      out,
      depth,
      call_pad,
      shadow_pad,
      *pushed);

  (*pushed)++;
}

static int win64_count_params(Type *ty) {
  int count = 0;

  if (!ty)
    return 0;

  for (const Type *p = ty->params; p; p = p->next)
    count++;

  return count;
}

/*
 * Stack frame layout:
 *
 *   rbp +  0 : saved RBP
 *   rbp +  8 : return address
 *   rbp + 16 : home slot / first register argument
 *   rbp + 24 : second
 *   rbp + 32 : third
 *   rbp + 40 : fourth
 *   rbp + 48 : fifth argument
 *   ...
 *
 * Locals are below RBP.
 *
 * We save RDI and RSI because the generic x86 backend may use them.
 */
static void win64_assign_lvar_offsets(Obj *fn) {
  if (!fn->is_function)
    return;

  int top = 48;
  int bottom = 16;
  int param_idx = 0;

  for (Obj *var = fn->params; var; var = var->next) {
    /*
     * Large aggregates are passed as pointers, but chibicc's C
     * semantics require the parameter to behave as an actual object.
     * Make a local copy.
     */
    if ((var->ty->kind == TY_STRUCT || var->ty->kind == TY_UNION) &&
        win64_returns_by_reference(var->ty)) {
      bottom = align_to(bottom, var->align);
      bottom += var->ty->size;
      var->offset = -bottom;
    } else if (param_idx < WIN64_REG_MAX) {
      /*
       * Every argument consumes exactly one positional slot.
       */
      var->offset = 16 + param_idx * 8;
    } else {
      var->offset = top;
      top += 8;
    }

    param_idx++;
  }

  for (Obj *var = fn->locals; var; var = var->next) {
    if (var->offset)
      continue;

    bottom = align_to(bottom, var->align);
    bottom += var->ty->size;
    var->offset = -bottom;
  }

  /*
   * After:
   *
   *   push rbp
   *   push rdi
   *   push rsi
   *
   * RSP is at rbp - 16, which is 16-byte aligned (0 mod 16).
   *
   * Since bottom includes the 16 bytes for rdi/rsi below rbp,
   * the additional stack allocation must be align_to(bottom, 16) - 16
   * so that RSP remains 16-byte aligned after the prologue.
   *
   * We don't reserve shadow space here; it is allocated by the
   * caller around each call.
   */
  fn->stack_size = align_to(bottom, 16) - 16;
}

static int win64_get_spill_base(Obj *fn) {
  return fn ? (fn->stack_size + 16) : 16;
}

static void win64_finalize_stack(Obj *fn, int spill_offset) {
  if (fn)
    fn->stack_size = align_to(spill_offset, 16) - 16;
}

static int win64_push_args(Node *node, FILE *out, int *depth) {
  int arg_count = 0;

  for (const Node *arg = node->args; arg; arg = arg->next)
    arg_count++;

  bool ret_mem =
      node->ret_buffer &&
      win64_returns_by_reference(node->ty);

  /*
   * The hidden structure-return pointer occupies the first physical
   * argument slot.
   */
  int total_args = arg_count + (ret_mem ? 1 : 0);

  int temp_bytes = win64_temp_bytes(node->args);
  int temp_words = temp_bytes / 8;

  /*
   * Temporary storage is always a multiple of 16, so it does not
   * change stack alignment.
   */
  if (temp_bytes) {
    println_abi(out, "  sub $%d, %%rsp", temp_bytes);
    *depth += temp_words;
  }

  /*
   * Win64 requires at least 32 bytes of shadow space (4 slots) at the
   * call site. If total_args > 4, the 5th and subsequent arguments are
   * passed on the stack immediately above the 32-byte shadow space.
   */
  int stack_slots = total_args < WIN64_REG_MAX ? WIN64_REG_MAX : total_args;
  int shadow_pad = total_args < WIN64_REG_MAX ? (WIN64_REG_MAX - total_args) : 0;

  int call_pad =
      (*depth + stack_slots) & 1;

  if (call_pad) {
    println_abi(out, "  sub $8, %%rsp");
    (*depth)++;
  }

  if (shadow_pad) {
    println_abi(out, "  sub $%d, %%rsp", shadow_pad * 8);
    *depth += shadow_pad;
  }

  int pushed = 0;

  /*
   * Push user arguments right-to-left.
   */
  win64_push_args_rev(
      node->args,
      node->args,
      out,
      depth,
      call_pad,
      shadow_pad,
      &pushed);

  /*
   * Hidden sret pointer is physical argument slot zero, so it is
   * pushed last.
   */
  if (ret_mem) {
    println_abi(
        out,
        "  lea %d(%%rbp), %%rax",
        node->ret_buffer->offset);

    println_abi(out, "  push %%rax");
    (*depth)++;
  }

  bool is_variadic =
      node->func_ty &&
      node->func_ty->is_variadic;

  int named_args =
      node->func_ty ?
      win64_count_params(node->func_ty) : 0;

  /*
   * Copy the first four physical argument slots into their ABI
   * registers.
   */
  for (int slot = 0;
       slot < total_args && slot < WIN64_REG_MAX;
       slot++) {

    int off = slot * 8;

    /*
     * Physical slot zero is the hidden return-buffer pointer.
     * It is always an integer/pointer argument.
     */
    const Node *arg = NULL;

    if (ret_mem) {
      if (slot > 0) {
        arg = node->args;

        for (int i = 1; i < slot; i++)
          arg = arg->next;
      }
    } else {
      arg = node->args;

      for (int i = 0; i < slot; i++)
        arg = arg->next;
    }

    if (arg && is_flonum(arg->ty)) {
      if (arg->ty->size == 4) {
        println_abi(
            out,
            "  movss %d(%%rsp), %%xmm%d",
            off,
            slot);
      } else {
        println_abi(
            out,
            "  movsd %d(%%rsp), %%xmm%d",
            off,
            slot);
      }

      /*
       * For variadic/unprototyped calls, floating-point values must
       * also be copied into the corresponding GP register.
       */
      int source_index = slot - (ret_mem ? 1 : 0);

      if (is_variadic && source_index >= named_args) {
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
   * Return value is the number of 8-byte stack units added to the
   * current expression-stack depth.
   */
  return temp_words +
         call_pad +
         stack_slots;
}

static void win64_copy_ret_buffer(Obj *var, FILE *out) {
  switch (var->ty->size) {
  case 1:
    println_abi(out, "  mov %%al, %d(%%rbp)", var->offset);
    return;

  case 2:
    println_abi(out, "  mov %%ax, %d(%%rbp)", var->offset);
    return;

  case 4:
    println_abi(out, "  mov %%eax, %d(%%rbp)", var->offset);
    return;

  case 8:
    println_abi(out, "  mov %%rax, %d(%%rbp)", var->offset);
    return;

  default:
    unreachable();
  }
}

/*
 * Small struct return.
 *
 * RAX contains the integer representation. chibicc's normal return
 * machinery may expect the value to remain represented as an address,
 * so reconstruct the object in a temporary pointed to by RAX.
 *
 * R10 is volatile.
 */
static void win64_copy_struct_reg(Obj *fn, FILE *out) {
  const Type *ty = fn->ty->return_ty;

  switch (ty->size) {
  case 1:
    println_abi(out, "  movzbq (%%rax), %%rax");
    return;

  case 2:
    println_abi(out, "  movzwq (%%rax), %%rax");
    return;

  case 4:
    println_abi(out, "  movl (%%rax), %%eax");
    return;

  case 8:
    println_abi(out, "  movq (%%rax), %%rax");
    return;

  default:
    unreachable();
  }
}

/*
 * For a memory-return aggregate, RAX contains the address of the
 * generated temporary/result object. Copy it into the caller's
 * supplied return buffer.
 */
static void win64_copy_struct_mem(Obj *fn, FILE *out) {
  const Type *ty = fn->ty->return_ty;
  const Obj *var = fn->params;

  println_abi(
      out,
      "  mov %d(%%rbp), %%r11",
      var->offset);

  println_abi(out, "  mov %%rax, %%r10");

  win64_copy_bytes(ty->size, out);

  /*
   * Win64 indirect-return functions return the destination pointer
   * in RAX.
   */
  println_abi(out, "  mov %%r11, %%rax");
}

/*
 * alloca() receives its size in RDI in chibicc's generic x86 backend.
 */
static void win64_builtin_alloca(Obj *fn, FILE *out) {
  /*
   * Align size to 16 bytes.
   */
  println_abi(out, "  add $15, %%rdi");
  println_abi(out, "  and $-16, %%rdi");

  /*
   * Shift the temporary area by %rdi.
   */
  println_abi(out,
              "  mov %d(%%rbp), %%rcx",
              fn->alloca_bottom->offset);

  println_abi(out, "  sub %%rsp, %%rcx");
  println_abi(out, "  mov %%rsp, %%rax");
  println_abi(out, "  sub %%rdi, %%rsp");
  println_abi(out, "  mov %%rsp, %%rdx");

  println_abi(out, "1:");
  println_abi(out, "  test %%rcx, %%rcx");
  println_abi(out, "  je 2f");

  println_abi(out, "  mov (%%rax), %%r8b");
  println_abi(out, "  mov %%r8b, (%%rdx)");
  println_abi(out, "  inc %%rdx");
  println_abi(out, "  inc %%rax");
  println_abi(out, "  dec %%rcx");
  println_abi(out, "  jmp 1b");

  println_abi(out, "2:");

  /*
   * Move alloca_bottom pointer.
   */
  println_abi(out,
              "  subq %%rdi, %d(%%rbp)",
              fn->alloca_bottom->offset);

  println_abi(out,
              "  mov %d(%%rbp), %%rax",
              fn->alloca_bottom->offset);
}

static void win64_emit_prologue(Obj *fn, FILE *out) {
  println_abi(out, "  push %%rbp");
  println_abi(out, "  mov %%rsp, %%rbp");

  /*
   * RDI and RSI are nonvolatile on Win64 and are used by the generic
   * x86 backend.
   */
  println_abi(out, "  push %%rdi");
  println_abi(out, "  push %%rsi");

  if (fn->stack_size >= 4096) {
    /*
     * MinGW's __chkstk_ms expects the allocation size in RAX.
     */
    println_abi(
        out,
        "  mov $%d, %%rax",
        fn->stack_size);

    println_abi(out, "  call ___chkstk_ms");
    println_abi(out, "  sub %%rax, %%rsp");
  } else if (fn->stack_size > 0) {
    println_abi(
        out,
        "  sub $%d, %%rsp",
        fn->stack_size);
  }

  /*
   * Used by alloca.
   */
  if (fn->alloca_bottom) {
    println_abi(
        out,
        "  mov %%rsp, %d(%%rbp)",
        fn->alloca_bottom->offset);
  }

  /*
   * A variadic function must home all four register arguments because
   * va_arg may access arguments that were not named.
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
   * Materialize register parameters in their normal local-object
   * locations.
   */
  int idx = 0;

  for (const Obj *var = fn->params;
       var;
       var = var->next, idx++) {

    if (idx >= WIN64_REG_MAX)
      break;

    if ((var->ty->kind == TY_STRUCT ||
         var->ty->kind == TY_UNION) &&
        win64_returns_by_reference(var->ty)) {

      println_abi(
          out,
          "  mov %s, %%r10",
          win64_argreg64[idx]);

      println_abi(
          out,
          "  lea %d(%%rbp), %%r11",
          var->offset);

      win64_copy_bytes(var->ty->size, out);
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

    println_abi(
        out,
        "  mov %s, %d(%%rbp)",
        win64_gp_reg(idx, var->ty->size),
        var->offset);
  }

  /*
   * Stack-passed large aggregate parameters are pointers to caller
   * allocated objects. Make local copies for normal C parameter
   * semantics.
   */
  idx = 0;

  for (const Obj *var = fn->params;
       var;
       var = var->next, idx++) {

    if (idx < WIN64_REG_MAX)
      continue;

    if ((var->ty->kind != TY_STRUCT &&
         var->ty->kind != TY_UNION) ||
        !win64_returns_by_reference(var->ty))
      continue;

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

    win64_copy_bytes(var->ty->size, out);
  }

  /*
   * NOTE:
   *
   * This is intentionally left in terms of the ABI's va_area.
   * Win64 va_list is fundamentally different from SysV's:
   *
   *     char * -> next argument/home/stack slot
   *
   * If common code currently assumes the SysV 24-byte va_list
   * structure, that common code must be made ABI-aware.
   *
   * For a pointer-based implementation the useful initial value is
   * the first unnamed physical argument slot.
   */
  if (fn->va_area) {
    int named = 0;

    for (const Obj *var = fn->params; var; var = var->next)
      named++;

    /*
     * The parameter locations begin at RBP+16.
     * There are four register/home slots followed by stack args.
     *
     * The hidden sret parameter, if present, is already part of
     * fn->params, which is exactly what we want for the physical
     * argument position.
     */
    int first_off = 16 + named * 8;

    println_abi(
        out,
        "  lea %d(%%rbp), %%rax",
        first_off);

    /*
     * Keep the pointer at the beginning of va_area. This assumes
     * Win64-specific va_arg support uses the first pointer-sized
     * field of va_area.
     */
    println_abi(
        out,
        "  mov %%rax, %d(%%rbp)",
        fn->va_area->offset);
  }
}

static void win64_emit_epilogue(Obj *fn, FILE *out) {
  println_abi(out, ".L.return.%s:", fn->name);

  /*
   * Locals/fixed allocation are discarded without needing the exact
   * allocation size.
   */
  println_abi(out, "  lea -16(%%rbp), %%rsp");

  println_abi(out, "  pop %%rsi");
  println_abi(out, "  pop %%rdi");
  println_abi(out, "  pop %%rbp");
  println_abi(out, "  ret");
}

static void win64_emit_return(Obj *fn, Type *return_ty, FILE *out) {
  if (!return_ty && fn && fn->ty)
    return_ty = fn->ty->return_ty;
  if (!return_ty)
    return;

  if (return_ty->kind == TY_STRUCT || return_ty->kind == TY_UNION) {
    if (win64_returns_by_reference(return_ty)) {
      win64_copy_struct_mem(fn, out);
    } else {
      win64_copy_struct_reg(fn, out);
    }
  }
}

static const CallConv win64_callconv;

static const char *win64_reg32(const char *r64) {
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

static const char *win64_reg16(const char *r64) {
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

static const char *win64_reg8(const char *r64) {
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

static void win64_load_vreg(LLIRVReg *v, const char *reg, FILE *out) {
  if (!v) return;
  int offset = v->spill_offset ? v->spill_offset : -((v->id + 1) * 8);
  int sz = v->ty ? v->ty->size : 8;
  if (reg[1] == 'x') {
    println_abi(out, "  movq %d(%%rbp), %s", offset, reg);
  } else if (sz == 1) {
    if (v->ty && v->ty->is_unsigned)
      println_abi(out, "  movzbq %d(%%rbp), %s", offset, reg);
    else
      println_abi(out, "  movsbq %d(%%rbp), %s", offset, reg);
  } else if (sz == 2) {
    if (v->ty && v->ty->is_unsigned)
      println_abi(out, "  movzwq %d(%%rbp), %s", offset, reg);
    else
      println_abi(out, "  movswq %d(%%rbp), %s", offset, reg);
  } else if (sz == 4) {
    if (v->ty && v->ty->is_unsigned)
      println_abi(out, "  movl %d(%%rbp), %s", offset, win64_reg32(reg));
    else
      println_abi(out, "  movslq %d(%%rbp), %s", offset, reg);
  } else {
    println_abi(out, "  movq %d(%%rbp), %s", offset, reg);
  }
}

static void win64_store_vreg(const char *reg, LLIRVReg *v, FILE *out) {
  if (!v) return;
  int offset = v->spill_offset ? v->spill_offset : -((v->id + 1) * 8);
  if (reg[1] == 'x') {
    println_abi(out, "  movq %s, %d(%%rbp)", reg, offset);
  } else {
    println_abi(out, "  movq %s, %d(%%rbp)", reg, offset);
  }
}

static void win64_load_call_arg(LLIRVReg *arg, const char *reg, FILE *out) {
  if (!arg) return;
  if (arg->is_struct_val && (arg->struct_size == 1 || arg->struct_size == 2 || arg->struct_size == 4 || arg->struct_size == 8)) {
    win64_load_vreg(arg, "%rax", out);
    if (arg->struct_size == 1)
      println_abi(out, "  movzbq (%%rax), %%rax");
    else if (arg->struct_size == 2)
      println_abi(out, "  movzwq (%%rax), %%rax");
    else if (arg->struct_size == 4)
      println_abi(out, "  movl (%%rax), %%eax");
    else
      println_abi(out, "  movq (%%rax), %%rax");
    if (strcmp(reg, "%rax") != 0)
      println_abi(out, "  movq %%rax, %s", reg);
  } else {
    win64_load_vreg(arg, reg, out);
  }
}

static void win64_emit_call(LLIRInsn *insn, FILE *out) {
  const CallConv *cc = &win64_callconv;
  int shadow = cc->shadow_space;
  int extra_args = insn->var ? 1 : 0;
  int total_args = insn->num_args + extra_args;
  int stack_args = (total_args > 4) ? (total_args - 4) : 0;
  int total_alloc = align_to(shadow + stack_args * 8, 16);
  if (total_alloc > 0)
    println_abi(out, "  sub $%d, %%rsp", total_alloc);

  if (insn->var) {
    println_abi(out, "  lea %d(%%rbp), %%rcx", insn->var->offset);
  }

  for (int a = 0; a < insn->num_args; a++) {
    int pos = a + extra_args;
    LLIRVReg *arg = insn->args[a];
    if (pos < 4) {
      if (arg && arg->ty && is_flonum(arg->ty)) {
        win64_load_call_arg(arg, "%rax", out);
        println_abi(out, "  movq %%rax, %%xmm%d", pos);
        const char *r = (cc && cc->gp_regs64[pos]) ? cc->gp_regs64[pos] : "%rcx";
        println_abi(out, "  movq %%rax, %s", r);
      } else {
        const char *r = (cc && cc->gp_regs64[pos]) ? cc->gp_regs64[pos] : "%rcx";
        win64_load_call_arg(arg, r, out);
      }
    } else {
      win64_load_call_arg(arg, "%rax", out);
      println_abi(out, "  movq %%rax, %d(%%rsp)", shadow + (pos - 4) * 8);
    }
  }

  if (insn->src1) {
    win64_load_vreg(insn->src1, "%r11", out);
    println_abi(out, "  call *%%r11");
  } else if (insn->label) {
    println_abi(out, "  call %s", insn->label);
  }

  if (total_alloc > 0)
    println_abi(out, "  add $%d, %%rsp", total_alloc);

  if (insn->dst && !insn->var) {
    if (insn->dst->ty && is_flonum(insn->dst->ty)) {
      println_abi(out, "  movq %%xmm0, %%rax");
      win64_store_vreg("%rax", insn->dst, out);
    } else {
      win64_store_vreg("%rax", insn->dst, out);
    }
  }
}

static Node *win64_builtin_va_start(Node *ap, Node *last, Token *tok) {
  VarScope *sc = find_var(&(Token){.loc = "__va_area__", .len = 11});
  if (!sc || !sc->var)
    error_tok(tok, "__builtin_va_start used outside variadic function");
  Node *va_var = new_var_node(sc->var, tok);
  add_type(ap);
  if (ap->ty->kind == TY_PTR && (ap->ty->base->kind != TY_STRUCT && ap->ty->base->kind != TY_UNION)) {
    // Pointer va_list (e.g. char *va_list)
    Node *addr = new_unary(ND_ADDR, va_var, tok);
    Node *val = new_unary(ND_DEREF, new_cast(addr, pointer_to(ap->ty)), tok);
    return new_binary(ND_ASSIGN, ap, val, tok);
  } else {
    // Array of __va_elem, or pointer to __va_elem
    // In __va_elem, overflow_arg_area is at offset 8
    Node *va_addr = new_unary(ND_ADDR, va_var, tok);
    Node *val = new_unary(ND_DEREF, new_cast(va_addr, pointer_to(pointer_to(ty_void))), tok);
    Node *of_addr = new_add(new_cast(ap, pointer_to(ty_char)), new_num(8, tok), tok);
    Node *of_ptr = new_cast(of_addr, pointer_to(pointer_to(ty_void)));
    return new_binary(ND_ASSIGN, new_unary(ND_DEREF, of_ptr, tok), val, tok);
  }
}

static Node *win64_builtin_va_arg(Node *ap, Type *ty, Token *tok) {
  add_type(ap);
  bool is_ptr = (ap->ty->kind == TY_PTR && (ap->ty->base->kind != TY_STRUCT && ap->ty->base->kind != TY_UNION));

  Obj *old_p = new_lvar("", pointer_to(ty_void));
  Node head = {};
  Node *cur = &head;

  if (is_ptr) {
    cur = cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, new_var_node(old_p, tok), new_cast(ap, pointer_to(ty_void)), tok), tok);
    cur = cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, ap, new_cast(new_add(new_cast(ap, pointer_to(ty_char)), new_num(8, tok), tok), ap->ty), tok), tok);
  } else {
    Node *of_addr = new_add(new_cast(ap, pointer_to(ty_char)), new_num(8, tok), tok);
    Node *of_ptr = new_cast(of_addr, pointer_to(pointer_to(ty_void)));
    Node *load_of = new_unary(ND_DEREF, of_ptr, tok);
    cur = cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, new_var_node(old_p, tok), load_of, tok), tok);
    Node *new_of = new_add(new_cast(new_var_node(old_p, tok), pointer_to(ty_char)), new_num(8, tok), tok);
    cur = cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, new_unary(ND_DEREF, of_ptr, tok), new_cast(new_of, pointer_to(ty_void)), tok), tok);
  }

  cur = cur->next = new_unary(ND_EXPR_STMT,
    new_unary(ND_DEREF, new_cast(new_var_node(old_p, tok), pointer_to(ty)), tok), tok);

  Node *node = new_node(ND_STMT_EXPR, tok);
  node->body = head.next;
  return node;
}

static Node *win64_builtin_va_copy(Node *dest, Node *src, Token *tok) {
  add_type(dest);
  if (dest->ty->kind == TY_PTR && (dest->ty->base->kind != TY_STRUCT && dest->ty->base->kind != TY_UNION)) {
    return new_binary(ND_ASSIGN, dest, src, tok);
  } else {
    Node *deref_dest = new_unary(ND_DEREF, dest, tok);
    Node *deref_src = new_unary(ND_DEREF, src, tok);
    return new_binary(ND_ASSIGN, deref_dest, deref_src, tok);
  }
}

static Node *win64_builtin_va_end(Node *ap, Token *tok) {
  Node *node = new_node(ND_NULL_EXPR, tok);
  node->ty = ty_void;
  return node;
}

static void win64_define_macros(void) {
  define_macro("_WIN64", "1");
  define_macro("_WIN32", "1");

  define_macro("_M_X64", "100");
  define_macro("_M_AMD64", "100");

  define_macro("__x86_64", "1");
  define_macro("__x86_64__", "1");
  define_macro("__amd64", "1");
  define_macro("__amd64__", "1");

  define_macro("__SIZEOF_LONG__", "4");
  define_macro("__SIZEOF_POINTER__", "8");
  define_macro("__SIZEOF_PTRDIFF_T__", "8");
  define_macro("__SIZEOF_SIZE_T__", "8");

  define_macro("__SIZE_TYPE__", "unsigned long long");
  define_macro("__PTRDIFF_TYPE__", "long long");
  define_macro("__INTPTR_TYPE__", "long long");
  define_macro("__UINTPTR_TYPE__", "unsigned long long");
  define_macro("__INTMAX_TYPE__", "long long");
  define_macro("__UINTMAX_TYPE__", "unsigned long long");

  define_macro("__WCHAR_TYPE__", "unsigned short");
  define_macro("__WINT_TYPE__", "unsigned short");
}

static void win64_init_types(void) {
  ty_char->size = 1;
  ty_char->align = 1;

  ty_short->size = 2;
  ty_short->align = 2;

  ty_int->size = 4;
  ty_int->align = 4;

  ty_long->size = 4;
  ty_long->align = 4;

  ty_llong->size = 8;
  ty_llong->align = 8;

  ty_uchar->size = 1;
  ty_uchar->align = 1;

  ty_ushort->size = 2;
  ty_ushort->align = 2;

  ty_uint->size = 4;
  ty_uint->align = 4;

  ty_ulong->size = 4;
  ty_ulong->align = 4;

  ty_ullong->size = 8;
  ty_ullong->align = 8;

  ty_float->size = 4;
  ty_float->align = 4;

  ty_double->size = 8;
  ty_double->align = 8;

  /*
   * MSVC/Win64 treats long double as double.
   */
  ty_ldouble->size = 8;
  ty_ldouble->align = 8;
}

static void win64_declare_builtin_types(void) {
  push_scope("__builtin_va_list")->type_def = pointer_to(ty_char);
}

static const CallConv win64_callconv = {
  .name = "win64",

  .num_gp_regs = 4,
  .gp_regs64 = {
    "%rcx", "%rdx", "%r8", "%r9"
  },
  .gp_regs32 = {
    "%ecx", "%edx", "%r8d", "%r9d"
  },
  .gp_regs16 = {
    "%cx", "%dx", "%r8w", "%r9w"
  },
  .gp_regs8 = {
    "%cl", "%dl", "%r8b", "%r9b"
  },

  .num_fp_regs = 4,

  .shadow_space = 32,

  /*
   * A single positional slot is consumed regardless of whether
   * the argument goes into a GP or XMM register.
   */
  .paired_slots = true,

  .pass_struct_by_ref = true,

  .stack_align = 16,
};

ABI abi_win64 = {
  .name = "win64",
  .description = "Microsoft Windows x64 ABI",

  .callconv = &win64_callconv,
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
  .align_llong = 8,

  .size_ptr = 8,
  .align_ptr = 8,

  .size_float = 4,
  .align_float = 4,

  .size_double = 8,
  .align_double = 8,

  .size_ldouble = 8,
  .align_ldouble = 8,

  .align_stack = 16,

  /*
   * This is enough storage for the pointer-based Win64 va_list
   * representation used by the ABI-specific va_start/va_arg path.
   */
  .va_area_size = 8,
  .va_area_align = 8,

  .returns_by_reference = win64_returns_by_reference,
  .classify_reg = win64_classify_reg,

  .assign_lvar_offsets = win64_assign_lvar_offsets,
  .get_spill_base = win64_get_spill_base,
  .finalize_stack = win64_finalize_stack,
  .push_args = win64_push_args,

  .copy_ret_buffer = win64_copy_ret_buffer,
  .copy_struct_reg = win64_copy_struct_reg,
  .copy_struct_mem = win64_copy_struct_mem,

  .builtin_alloca = win64_builtin_alloca,

  .builtin_va_start = win64_builtin_va_start,
  .builtin_va_arg = win64_builtin_va_arg,
  .builtin_va_copy = win64_builtin_va_copy,
  .builtin_va_end = win64_builtin_va_end,

  .emit_prologue = win64_emit_prologue,
  .emit_epilogue = win64_emit_epilogue,
  .emit_return = win64_emit_return,
  .emit_call = win64_emit_call,

  .define_macros = win64_define_macros,
  .init_types = win64_init_types,
  .declare_builtin_types = win64_declare_builtin_types,
};
