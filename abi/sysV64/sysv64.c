#include "chibicc.h"
#include "abi/abi.h"
#include "codegen/common/common.h"
#include "ir/ir.h"

#include <assert.h>

#define GP_MAX 6
#define FP_MAX 8

static char *argreg8[] = {
  "%dil", "%sil", "%dl", "%cl", "%r8b", "%r9b"
};

static char *argreg16[] = {
  "%di", "%si", "%dx", "%cx", "%r8w", "%r9w"
};

static char *argreg32[] = {
  "%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"
};

static char *argreg64[] = {
  "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"
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
 * SysV AMD64 eightbyte classes.
 */
typedef enum {
  SYSV_NO_CLASS,
  SYSV_MEMORY,
  SYSV_INTEGER,
  SYSV_SSE,
  SYSV_SSEUP,
  SYSV_X87,
  SYSV_X87UP,
  SYSV_COMPLEX_X87,
} SysVClass;

typedef struct {
  SysVClass cls[2];
  int n;
} SysVTypeClass;


/*
 * True for types which are classified as aggregates by the SysV ABI.
 */
static bool sysv64_is_aggregate(Type *ty) {
  return ty->kind == TY_STRUCT ||
         ty->kind == TY_UNION ||
         ty->kind == TY_ARRAY;
}


/*
 * Return the class contributed by a scalar type.
 */
static SysVClass classify_scalar(Type *ty) {
  switch (ty->kind) {
  case TY_FLOAT:
  case TY_DOUBLE:
    return SYSV_SSE;

  case TY_LDOUBLE:
    /*
     * SysV AMD64 long double is an 80-bit x87 value.
     *
     * It is passed on the stack, but is returned through ST(0).
     */
    return SYSV_X87;

  default:
    if (is_integer(ty) || ty->kind == TY_PTR)
      return SYSV_INTEGER;

    return SYSV_MEMORY;
  }
}


/*
 * Merge two SysV classes.
 */
static SysVClass merge_sysv_class(SysVClass a, SysVClass b) {
  if (a == b)
    return a;

  if (a == SYSV_NO_CLASS)
    return b;

  if (b == SYSV_NO_CLASS)
    return a;

  if (a == SYSV_MEMORY || b == SYSV_MEMORY)
    return SYSV_MEMORY;

  if (a == SYSV_INTEGER || b == SYSV_INTEGER)
    return SYSV_INTEGER;

  /*
   * X87 values cannot coexist with normal register classes in an
   * ordinary aggregate. Such an aggregate is passed in memory.
   */
  if (a == SYSV_X87 || a == SYSV_X87UP ||
      b == SYSV_X87 || b == SYSV_X87UP)
    return SYSV_MEMORY;

  if (a == SYSV_COMPLEX_X87 || b == SYSV_COMPLEX_X87)
    return SYSV_MEMORY;

  /*
   * SSE + SSEUP remains SSE for the subset of types currently handled
   * by chibicc.
   */
  if ((a == SYSV_SSE || a == SYSV_SSEUP) &&
      (b == SYSV_SSE || b == SYSV_SSEUP))
    return SYSV_SSE;

  return SYSV_MEMORY;
}


/*
 * Forward declaration.
 */
static void classify_sysv_type(Type *ty, SysVClass cls[2],
                               int base_offset);


/*
 * Classify a scalar contribution.
 */
static void classify_sysv_scalar(Type *ty, SysVClass cls[2],
                                 int base_offset) {
  SysVClass c = classify_scalar(ty);

  if (c == SYSV_MEMORY) {
    cls[0] = SYSV_MEMORY;
    cls[1] = SYSV_MEMORY;
    return;
  }

  int start = base_offset / 8;
  int end = (base_offset + ty->size + 7) / 8;

  /*
   * A scalar crossing an eightbyte boundary is only valid for the ABI
   * when its alignment permits it. chibicc does not currently have
   * vector types requiring more complicated handling here.
   */
  if (start < 0 || end > 2 || end <= start) {
    cls[0] = SYSV_MEMORY;
    cls[1] = SYSV_MEMORY;
    return;
  }

  for (int i = start; i < end; i++)
    cls[i] = merge_sysv_class(cls[i], c);
}


/*
 * Recursively classify an aggregate.
 */
static void classify_sysv_type(Type *ty, SysVClass cls[2],
                               int base_offset) {
  if (cls[0] == SYSV_MEMORY || cls[1] == SYSV_MEMORY)
    return;

  switch (ty->kind) {
  case TY_STRUCT:
  case TY_UNION:
    for (Member *mem = ty->members; mem; mem = mem->next)
      classify_sysv_type(mem->ty, cls, base_offset + mem->offset);
    return;

  case TY_ARRAY:
    for (int i = 0; i < ty->array_len; i++)
      classify_sysv_type(ty->base, cls,
                         base_offset + i * ty->base->size);
    return;

  default:
    classify_sysv_scalar(ty, cls, base_offset);
    return;
  }
}


/*
 * Check whether an aggregate contains an unaligned field.
 *
 * The SysV ABI classifies an aggregate containing an unaligned field
 * as MEMORY.
 */
static bool sysv64_has_unaligned_field(Type *ty, int base_offset) {
  switch (ty->kind) {
  case TY_STRUCT:
  case TY_UNION:
    for (Member *mem = ty->members; mem; mem = mem->next) {
      int offset = base_offset + mem->offset;

      if (mem->ty->align > 1 &&
          offset % mem->ty->align != 0)
        return true;

      if (sysv64_is_aggregate(mem->ty) &&
          sysv64_has_unaligned_field(mem->ty, offset))
        return true;
    }

    return false;

  case TY_ARRAY:
    for (int i = 0; i < ty->array_len; i++) {
      int offset = base_offset + i * ty->base->size;

      if (sysv64_is_aggregate(ty->base) &&
          sysv64_has_unaligned_field(ty->base, offset))
        return true;

      /*
       * Array elements themselves must satisfy their natural
       * alignment.
       */
      if (ty->base->align > 1 &&
          offset % ty->base->align != 0)
        return true;
    }

    return false;

  default:
    return false;
  }
}


/*
 * Full SysV AMD64 classification for the subset of types represented by
 * chibicc.
 */
static SysVTypeClass classify_sysv64(Type *ty) {
  SysVTypeClass result = {
    .cls = { SYSV_NO_CLASS, SYSV_NO_CLASS },
    .n = 0,
  };

  /*
   * Scalars.
   */
  if (!sysv64_is_aggregate(ty)) {
    SysVClass c = classify_scalar(ty);

    result.cls[0] = c;
    result.n = 1;
    return result;
  }

  /*
   * Aggregates larger than two eightbytes are MEMORY.
   */
  if (ty->size > 16) {
    result.cls[0] = SYSV_MEMORY;
    result.cls[1] = SYSV_MEMORY;
    return result;
  }

  /*
   * The ABI requires aggregates containing unaligned fields to be
   * passed in memory.
   */
  if (sysv64_has_unaligned_field(ty, 0)) {
    result.cls[0] = SYSV_MEMORY;
    result.cls[1] = SYSV_MEMORY;
    return result;
  }

  classify_sysv_type(ty, result.cls, 0);

  if (result.cls[0] == SYSV_MEMORY ||
      result.cls[1] == SYSV_MEMORY) {
    result.cls[0] = SYSV_MEMORY;
    result.cls[1] = SYSV_MEMORY;
    return result;
  }

  /*
   * An object occupying at most one eightbyte needs one class.
   *
   * A completely empty aggregate is represented as INTEGER by
   * chibicc's existing object model.
   */
  if (ty->size <= 8) {
    if (result.cls[0] == SYSV_NO_CLASS)
      result.cls[0] = SYSV_INTEGER;

    result.n = 1;
    return result;
  }

  if (result.cls[0] == SYSV_NO_CLASS)
    result.cls[0] = SYSV_INTEGER;

  if (result.cls[1] == SYSV_NO_CLASS)
    result.cls[1] = SYSV_INTEGER;

  result.n = 2;
  return result;
}


/*
 * Number of GP registers consumed by a classification.
 */
static int sysv64_num_gp(SysVTypeClass *cls) {
  int n = 0;

  for (int i = 0; i < cls->n; i++)
    if (cls->cls[i] == SYSV_INTEGER)
      n++;

  return n;
}


/*
 * Number of SSE registers consumed by a classification.
 */
static int sysv64_num_fp(SysVTypeClass *cls) {
  int n = 0;

  for (int i = 0; i < cls->n; i++)
    if (cls->cls[i] == SYSV_SSE ||
        cls->cls[i] == SYSV_SSEUP)
      n++;

  return n;
}


/*
 * Is an aggregate passed in memory?
 */
static bool sysv64_is_memory(Type *ty) {
  if (!sysv64_is_aggregate(ty))
    return false;

  SysVTypeClass cls = classify_sysv64(ty);
  return cls.n == 0 ||
         cls.cls[0] == SYSV_MEMORY ||
         cls.cls[1] == SYSV_MEMORY;
}


/*
 * Does an aggregate return through the hidden return buffer?
 */
static bool sysv64_returns_by_reference(Type *ty) {
  return sysv64_is_aggregate(ty) &&
         sysv64_is_memory(ty);
}


/*
 * Classify simple scalar arguments.
 *
 * long double deliberately does not count as an SSE argument.
 */
static int sysv64_classify_reg(Type *ty) {
  if (is_integer(ty) || ty->kind == TY_PTR)
    return 0;

  if (ty->kind == TY_FLOAT || ty->kind == TY_DOUBLE)
    return 1;

  return 2;
}


/*
 * Assign local-variable and stack-parameter offsets.
 */
static void sysv64_assign_lvar_offsets(Obj *fn) {
  if (!fn->is_function)
    return;

  int top = 16;
  int bottom = 0;

  int gp = 0;
  int fp = 0;

  /*
   * Determine which parameters are passed in registers.
   */
  for (Obj *var = fn->params; var; var = var->next) {
    Type *ty = var->ty;

    bool pass_stack = false;

    if (sysv64_is_aggregate(ty)) {
      SysVTypeClass cls = classify_sysv64(ty);

      int ngp = sysv64_num_gp(&cls);
      int nfp = sysv64_num_fp(&cls);

      if (cls.n == 0 ||
          gp + ngp > GP_MAX ||
          fp + nfp > FP_MAX) {
        pass_stack = true;
      } else {
        gp += ngp;
        fp += nfp;
      }

    } else if (ty->kind == TY_LDOUBLE) {
      /*
       * long double arguments are passed in memory.
       */
      pass_stack = true;

    } else if (ty->kind == TY_FLOAT ||
               ty->kind == TY_DOUBLE) {

      if (fp >= FP_MAX)
        pass_stack = true;
      else
        fp++;

    } else {
      if (gp >= GP_MAX)
        pass_stack = true;
      else
        gp++;
    }

    if (!pass_stack)
      continue;

    /*
     * Stack arguments occupy whole eightbyte slots, with alignment
     * respected.
     */
    int align = MAX(8, ty->align);
    int size = align_to(ty->size, 8);

    top = align_to(top, align);
    var->offset = top;
    top += size;
  }

  /*
   * Allocate locals.
   *
   * This is the same downward-growing layout convention used by
   * upstream chibicc.
   */
  for (Obj *var = fn->locals; var; var = var->next) {
    if (var->offset)
      continue;

    int align = var->align;

    bottom += var->ty->size;
    bottom = align_to(bottom, align);

    var->offset = -bottom;
  }

  fn->stack_size = align_to(bottom, 16);
}

static int sysv64_get_spill_base(Obj *fn) {
  return fn ? fn->stack_size : 0;
}

static void sysv64_finalize_stack(Obj *fn, int spill_offset) {
  if (fn)
    fn->stack_size = align_to(spill_offset, 16);
}


/*
 * Push an aggregate expression onto the stack.
 *
 * Keep this deliberately conservative. It avoids making assumptions
 * about alignment of the source aggregate.
 */
static void push_struct(Type *ty, FILE *out, int *depth) {
  /*
   * Stage the aggregate as individual eightbytes.
   *
   * This is important because the SysV ABI classifies each eightbyte
   * independently. A struct such as:
   *
   *     struct { int x; int y; }
   *
   * occupies one eightbyte and therefore must consume exactly one
   * INTEGER register.
   */
  int size = align_to(ty->size, 8);

  println_abi(out, "  sub $%d, %%rsp", size);
  *depth += size / 8;

  /*
   * %rax contains the address of the aggregate.
   */
  int i = 0;
  while (ty->size - i >= 8) {
    println_abi(out, "  mov %d(%%rax), %%r10", i);
    println_abi(out, "  mov %%r10, %d(%%rsp)", i);
    i += 8;
  }
  if (ty->size - i >= 4) {
    println_abi(out, "  mov %d(%%rax), %%r10d", i);
    println_abi(out, "  mov %%r10d, %d(%%rsp)", i);
    i += 4;
  }
  if (ty->size - i >= 2) {
    println_abi(out, "  mov %d(%%rax), %%r10w", i);
    println_abi(out, "  mov %%r10w, %d(%%rsp)", i);
    i += 2;
  }
  if (i < ty->size) {
    println_abi(out, "  mov %d(%%rax), %%r10b", i);
    println_abi(out, "  mov %%r10b, %d(%%rsp)", i);
  }
}


/*
 * Align the current stack position for a stack argument.
 *
 * depth is measured in eightbyte units.
 */
static void align_stack_arg(Type *ty, FILE *out, int *depth) {
  int align = MAX(8, ty->align);

  if (align <= 8)
    return;

  int current = *depth * 8;
  int misalign = current % align;

  if (misalign == 0)
    return;

  int pad = align - misalign;

  assert(pad % 8 == 0);

  println_abi(out,
              "  sub $%d, %%rsp",
              pad);

  *depth += pad / 8;
}


/*
 * Push arguments.
 *
 * Register arguments are staged first. Stack arguments are emitted in
 * right-to-left order.
 */
static void push_args2(Node *args, bool first_pass,
                       FILE *out, int *depth) {
  if (!args)
    return;

  push_args2(args->next, first_pass, out, depth);

  if ((first_pass && args->pass_by_stack) ||
      (!first_pass && !args->pass_by_stack))
    return;

  /*
   * Only stack arguments need individual alignment.
   */
  if (!first_pass)
    align_stack_arg(args->ty, out, depth);

  if (current_codegen && current_codegen->gen_expr)
    ((void (*)(void *, FILE *))current_codegen->gen_expr)(args, out);

  switch (args->ty->kind) {
  case TY_STRUCT:
  case TY_UNION:
  case TY_ARRAY:
    push_struct(args->ty, out, depth);
    break;

  case TY_FLOAT:
  case TY_DOUBLE:
    println_abi(out, "  sub $8, %%rsp");
    println_abi(out, "  movsd %%xmm0, (%%rsp)");
    (*depth)++;
    break;

  case TY_LDOUBLE:
    /*
     * x87 long double occupies a 16-byte stack slot.
     */
    println_abi(out, "  sub $16, %%rsp");
    println_abi(out, "  fstpt (%%rsp)");
    *depth += 2;
    break;

  default:
    println_abi(out, "  push %%rax");
    (*depth)++;
    break;
  }
}


static void pop_gp(int r, FILE *out, int *depth) {
  assert(r >= 0 && r < GP_MAX);

  println_abi(out, "  pop %s", argreg64[r]);
  (*depth)--;
}


static void pop_fp(int r, FILE *out, int *depth) {
  assert(r >= 0 && r < FP_MAX);

  println_abi(out, "  movsd (%%rsp), %%xmm%d", r);
  println_abi(out, "  add $8, %%rsp");
  (*depth)--;
}


/*
 * Push function arguments according to the SysV AMD64 ABI.
 */
static int sysv64_push_args(Node *node, FILE *out, int *depth) {
  int stack = 0;
  int gp = 0;
  int fp = 0;

  bool ret_mem =
    node->ret_buffer &&
    sysv64_returns_by_reference(node->ty);

  /*
   * The hidden structure-return argument consumes the first GP register.
   */
  if (ret_mem)
    gp++;

  /*
   * First classify every argument and determine whether it can fit
   * entirely in registers.
   */
  for (Node *arg = node->args; arg; arg = arg->next) {
    Type *ty = arg->ty;

    arg->pass_by_stack = false;

    if (sysv64_is_aggregate(ty)) {
      SysVTypeClass cls = classify_sysv64(ty);

      int ngp = sysv64_num_gp(&cls);
      int nfp = sysv64_num_fp(&cls);

      /*
       * SysV requires an aggregate to have all of its required
       * registers available. It cannot be partially split between
       * registers and memory.
       */
      if (cls.n == 0 ||
          gp + ngp > GP_MAX ||
          fp + nfp > FP_MAX) {

        arg->pass_by_stack = true;

        stack += align_to(ty->size, 8) / 8;

      } else {
        gp += ngp;
        fp += nfp;
      }

      continue;
    }

    if (ty->kind == TY_LDOUBLE) {
      arg->pass_by_stack = true;
      stack += 2;
      continue;
    }

    if (ty->kind == TY_FLOAT ||
        ty->kind == TY_DOUBLE) {

      if (fp >= FP_MAX) {
        arg->pass_by_stack = true;
        stack++;
      } else {
        fp++;
      }

      continue;
    }

    if (gp >= GP_MAX) {
      arg->pass_by_stack = true;
      stack++;
    } else {
      gp++;
    }
  }

  /*
   * Ensure the outgoing argument area starts with the alignment expected
   * by the callee.
   *
   * The staged register arguments are also part of depth, so account
   * for them below after staging.
   */
  if ((*depth + stack) % 2 == 1) {
    println_abi(out, "  sub $8, %%rsp");
    (*depth)++;
    stack++;
  }

  /*
   * Emit actual stack arguments.
   */
  push_args2(node->args, false, out, depth);

  /*
   * Stage register arguments.
   */
  push_args2(node->args, true, out, depth);

  /*
   * Hidden structure-return pointer is pushed after stack arguments so
   * that it becomes the first argument register after the final pops.
   */
  if (ret_mem) {
    println_abi(out,
                "  lea %d(%%rbp), %%rax",
                node->ret_buffer->offset);

    println_abi(out,
                "  push %%rax");

    (*depth)++;
  }

  /*
   * Transfer staged register arguments into their ABI registers.
   */
  int gp_pop = 0;
  int fp_pop = 0;

  if (ret_mem)
    pop_gp(gp_pop++, out, depth);

  for (Node *arg = node->args; arg; arg = arg->next) {
    if (arg->pass_by_stack)
      continue;

    Type *ty = arg->ty;

    if (sysv64_is_aggregate(ty)) {
      SysVTypeClass cls = classify_sysv64(ty);

      for (int i = 0; i < cls.n; i++) {
        switch (cls.cls[i]) {
        case SYSV_INTEGER:
          assert(gp_pop < GP_MAX);
          pop_gp(gp_pop++, out, depth);
          break;

        case SYSV_SSE:
        case SYSV_SSEUP:
          assert(fp_pop < FP_MAX);
          pop_fp(fp_pop++, out, depth);
          break;

        default:
          unreachable();
        }
      }

      continue;
    }

    if (ty->kind == TY_FLOAT ||
        ty->kind == TY_DOUBLE) {
      assert(fp_pop < FP_MAX);
      pop_fp(fp_pop++, out, depth);
      continue;
    }

    assert(gp_pop < GP_MAX);
    pop_gp(gp_pop++, out, depth);
  }

  return stack;
}


/*
 * Copy an aggregate returned in registers into a local return buffer.
 */
static void sysv64_copy_ret_buffer(Obj *var, FILE *out) {
  Type *ty = var->ty;
  SysVTypeClass cls = classify_sysv64(ty);

  int gp = 0;
  int fp = 0;

  for (int i = 0; i < cls.n; i++) {
    int offset = i * 8;
    int size = MIN(8, ty->size - offset);

    assert(size > 0);

    switch (cls.cls[i]) {
    case SYSV_INTEGER: {
      assert(gp < 2);

      const char *reg = (gp == 0) ? "%rax" : "%rdx";

      if (size == 8) {
        println_abi(out,
                    "  mov %s, %d(%%rbp)",
                    reg,
                    var->offset + offset);
      } else if (size == 4) {
        println_abi(out,
                    "  mov %s, %d(%%rbp)",
                    gp == 0 ? "%eax" : "%edx",
                    var->offset + offset);
      } else if (size == 2) {
        println_abi(out,
                    "  mov %s, %d(%%rbp)",
                    gp == 0 ? "%ax" : "%dx",
                    var->offset + offset);
      } else if (size == 1) {
        println_abi(out,
                    "  mov %s, %d(%%rbp)",
                    gp == 0 ? "%al" : "%dl",
                    var->offset + offset);
      } else {
        println_abi(out,
                    "  mov %s, %d(%%rbp)",
                    reg,
                    var->offset + offset);
      }

      gp++;
      break;
    }

    case SYSV_SSE:
    case SYSV_SSEUP:
      assert(fp < FP_MAX);

      if (size == 4) {
        println_abi(out,
                    "  movss %%xmm%d, %d(%%rbp)",
                    fp,
                    var->offset + offset);
      } else {
        println_abi(out,
                    "  movsd %%xmm%d, %d(%%rbp)",
                    fp,
                    var->offset + offset);
      }

      fp++;
      break;

    default:
      unreachable();
    }
  }
}


/*
 * Copy a local aggregate into the registers used for an aggregate return.
 *
 * The existing chibicc codegen convention gives us the address of the
 * aggregate in %rax.
 */
static void sysv64_copy_struct_reg(Obj *fn, FILE *out) {
  Type *ty = fn->ty->return_ty;
  SysVTypeClass cls = classify_sysv64(ty);

  /*
   * Preserve the aggregate address in a scratch volatile register (%r11)
   * while loading return registers.
   */
  println_abi(out, "  mov %%rax, %%r11");

  int gp = 0;
  int fp = 0;

  for (int i = 0; i < cls.n; i++) {
    int offset = i * 8;
    int size = MIN(8, ty->size - offset);

    assert(size > 0);

    switch (cls.cls[i]) {
    case SYSV_INTEGER: {
      assert(gp < 2);

      const char *reg = (gp == 0) ? "%rax" : "%rdx";

      if (size == 8) {
        println_abi(out,
                    "  mov %d(%%r11), %s",
                    offset,
                    reg);
      } else if (size == 4) {
        println_abi(out,
                    "  movl %d(%%r11), %s",
                    offset,
                    gp == 0 ? "%eax" : "%edx");
      } else if (size == 2) {
        println_abi(out,
                    "  movzwl %d(%%r11), %s",
                    offset,
                    gp == 0 ? "%eax" : "%edx");
      } else if (size == 1) {
        println_abi(out,
                    "  movzbl %d(%%r11), %s",
                    offset,
                    gp == 0 ? "%eax" : "%edx");
      } else {
        println_abi(out, "  xor %s, %s", reg, reg);
        for (int b = size - 1; b >= 0; b--) {
          if (b < size - 1)
            println_abi(out, "  shl $8, %s", reg);
          println_abi(out,
                      "  movb %d(%%r11), %s",
                      offset + b,
                      gp == 0 ? "%al" : "%dl");
        }
      }

      gp++;
      break;
    }

    case SYSV_SSE:
    case SYSV_SSEUP:
      assert(fp < FP_MAX);

      if (size == 4) {
        println_abi(out,
                    "  movss %d(%%r11), %%xmm%d",
                    offset,
                    fp);
      } else {
        println_abi(out,
                    "  movsd %d(%%r11), %%xmm%d",
                    offset,
                    fp);
      }

      fp++;
      break;

    default:
      unreachable();
    }
  }
}


/*
 * Copy a memory-returned aggregate into the hidden return buffer.
 */
static void sysv64_copy_struct_mem(Obj *fn, FILE *out) {
  Type *ty = fn->ty->return_ty;
  Obj *var = fn->params;

  /*
   * The hidden return buffer pointer is the first parameter.
   */
  println_abi(out,
              "  mov %d(%%rbp), %%r11",
              var->offset);

  int i = 0;
  while (ty->size - i >= 8) {
    println_abi(out, "  mov %d(%%rax), %%r10", i);
    println_abi(out, "  mov %%r10, %d(%%r11)", i);
    i += 8;
  }
  if (ty->size - i >= 4) {
    println_abi(out, "  mov %d(%%rax), %%r10d", i);
    println_abi(out, "  mov %%r10d, %d(%%r11)", i);
    i += 4;
  }
  if (ty->size - i >= 2) {
    println_abi(out, "  mov %d(%%rax), %%r10w", i);
    println_abi(out, "  mov %%r10w, %d(%%r11)", i);
    i += 2;
  }
  if (i < ty->size) {
    println_abi(out, "  mov %d(%%rax), %%r10b", i);
    println_abi(out, "  mov %%r10b, %d(%%r11)", i);
  }

  /*
   * SysV requires the hidden return-buffer pointer in RAX on return.
   */
  println_abi(out, "  mov %%r11, %%rax");
}


/*
 * Builtin alloca.
 */
static void sysv64_builtin_alloca(Obj *fn, FILE *out) {
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


static void store_fp(int r, int offset, int sz, FILE *out) {
  assert(r >= 0 && r < FP_MAX);

  switch (sz) {
  case 4:
    println_abi(out,
                "  movss %%xmm%d, %d(%%rbp)",
                r, offset);
    return;

  case 8:
    println_abi(out,
                "  movsd %%xmm%d, %d(%%rbp)",
                r, offset);
    return;

  default:
    unreachable();
  }
}


static void store_gp(int r, int offset, int sz, FILE *out) {
  assert(r >= 0 && r < GP_MAX);

  switch (sz) {
  case 1:
    println_abi(out,
                "  mov %s, %d(%%rbp)",
                argreg8[r], offset);
    return;

  case 2:
    println_abi(out,
                "  mov %s, %d(%%rbp)",
                argreg16[r], offset);
    return;

  case 4:
    println_abi(out,
                "  mov %s, %d(%%rbp)",
                argreg32[r], offset);
    return;

  case 8:
    println_abi(out,
                "  mov %s, %d(%%rbp)",
                argreg64[r], offset);
    return;

  default:
    /*
     * This path is only relevant to unusual aggregate handling.
     */
    for (int i = 0; i < sz; i++) {
      println_abi(out,
                  "  mov %s, %d(%%rbp)",
                  argreg8[r],
                  offset + i);

      if (i + 1 < sz)
        println_abi(out,
                    "  shr $8, %s",
                    argreg64[r]);
    }

    return;
  }
}


/*
 * Emit the SysV AMD64 function prologue.
 */
static void sysv64_emit_prologue(Obj *fn, FILE *out) {
  println_abi(out, "  push %%rbp");
  println_abi(out, "  mov %%rsp, %%rbp");
  if (fn->stack_size > 0)
    println_abi(out, "  sub $%d, %%rsp", fn->stack_size);

  if (fn->alloca_bottom) {
    println_abi(out,
                "  mov %%rsp, %d(%%rbp)",
                fn->alloca_bottom->offset);
  }

  /*
   * Save argument registers for variadic functions.
   */
  if (fn->va_area) {
    int gp = 0;
    int fp = 0;

    /*
     * Count the registers actually consumed by named parameters.
     */
    for (Obj *var = fn->params; var; var = var->next) {
      Type *ty = var->ty;

      if (var->offset > 0)
        continue;

      if (sysv64_is_aggregate(ty)) {
        SysVTypeClass cls = classify_sysv64(ty);

        gp += sysv64_num_gp(&cls);
        fp += sysv64_num_fp(&cls);

      } else if (ty->kind == TY_LDOUBLE) {
        /*
         * long double does not use GP/SSE registers.

         * It is already a stack argument.
         */

      } else if (ty->kind == TY_FLOAT ||
                 ty->kind == TY_DOUBLE) {
        fp++;

      } else {
        gp++;
      }
    }

    assert(gp <= GP_MAX);
    assert(fp <= FP_MAX);

    int off = fn->va_area->offset;

    /*
     * va_list:
     *
     *   +0   gp_offset
     *   +4   fp_offset
     *   +8   overflow_arg_area
     *   +16  reg_save_area
     *
     * The register save area consists of:
     *
     *   6 * 8  = 48 bytes GP
     *   8 * 16 = 128 bytes FP
     *
     * Total = 176 bytes.
     */
    println_abi(out,
                "  movl $%d, %d(%%rbp)",
                gp * 8,
                off);

    println_abi(out,
                "  movl $%d, %d(%%rbp)",
                fp * 16 + 48,
                off + 4);

    /*
     * overflow_arg_area points to the first unnamed stack argument.
     *
     * With the frame pointer established, the first incoming stack
     * argument is at RBP+16.
     */
    println_abi(out,
                "  lea 16(%%rbp), %%rax");

    println_abi(out,
                "  movq %%rax, %d(%%rbp)",
                off + 8);

    /*
     * reg_save_area starts immediately after va_area.
     */
    println_abi(out,
                "  lea %d(%%rbp), %%rax",
                off + 24);

    println_abi(out,
                "  movq %%rax, %d(%%rbp)",
                off + 16);

    /*
     * GP register save area.
     */
    println_abi(out,
                "  movq %%rdi, %d(%%rbp)",
                off + 24);

    println_abi(out,
                "  movq %%rsi, %d(%%rbp)",
                off + 32);

    println_abi(out,
                "  movq %%rdx, %d(%%rbp)",
                off + 40);

    println_abi(out,
                "  movq %%rcx, %d(%%rbp)",
                off + 48);

    println_abi(out,
                "  movq %%r8, %d(%%rbp)",
                off + 56);

    println_abi(out,
                "  movq %%r9, %d(%%rbp)",
                off + 64);

    /*
     * %al contains the number of XMM registers used by a variadic call.
     *
     * We can avoid touching the FP save area entirely when it is zero.
     */
    println_abi(out, "  test %%al, %%al");

    println_abi(out,
                "  je .L.va_skip_fp.%s",
                fn->name);

    println_abi(out,
                "  movups %%xmm0, %d(%%rbp)",
                off + 72);

    println_abi(out,
                "  movups %%xmm1, %d(%%rbp)",
                off + 88);

    println_abi(out,
                "  movups %%xmm2, %d(%%rbp)",
                off + 104);

    println_abi(out,
                "  movups %%xmm3, %d(%%rbp)",
                off + 120);

    println_abi(out,
                "  movups %%xmm4, %d(%%rbp)",
                off + 136);

    println_abi(out,
                "  movups %%xmm5, %d(%%rbp)",
                off + 152);

    println_abi(out,
                "  movups %%xmm6, %d(%%rbp)",
                off + 168);

    println_abi(out,
                "  movups %%xmm7, %d(%%rbp)",
                off + 184);

    println_abi(out,
                ".L.va_skip_fp.%s:",
                fn->name);
  }

  /*
   * Save register-passed parameters into their local stack slots.
   */
  int gp = 0;
  int fp = 0;

  for (Obj *var = fn->params; var; var = var->next) {
    if (var->offset > 0)
      continue;

    Type *ty = var->ty;

    if (sysv64_is_aggregate(ty)) {
      SysVTypeClass cls = classify_sysv64(ty);

      for (int i = 0; i < cls.n; i++) {
        int offset = i * 8;
        int size = MIN(8, ty->size - offset);

        assert(size > 0);

        switch (cls.cls[i]) {
        case SYSV_INTEGER:
          store_gp(gp++, var->offset + offset, size, out);
          break;

        case SYSV_SSE:
        case SYSV_SSEUP:
          store_fp(fp++, var->offset + offset, size, out);
          break;

        default:
          unreachable();
        }
      }

      continue;
    }

    if (ty->kind == TY_LDOUBLE)
      continue;

    if (ty->kind == TY_FLOAT ||
        ty->kind == TY_DOUBLE) {
      store_fp(fp++, var->offset, ty->size, out);
      continue;
    }

    store_gp(gp++, var->offset, ty->size, out);
  }
}


static void sysv64_emit_epilogue(Obj *fn, FILE *out) {
  println_abi(out, ".L.return.%s:", fn->name);
  println_abi(out, "  mov %%rbp, %%rsp");
  println_abi(out, "  pop %%rbp");
  println_abi(out, "  ret");
}

static void sysv64_emit_return(Obj *fn, Type *return_ty, FILE *out) {
  if (!return_ty && fn && fn->ty)
    return_ty = fn->ty->return_ty;
  if (!return_ty)
    return;

  if (return_ty->kind == TY_STRUCT || return_ty->kind == TY_UNION) {
    if (sysv64_returns_by_reference(return_ty)) {
      sysv64_copy_struct_mem(fn, out);
    } else {
      sysv64_copy_struct_reg(fn, out);
    }
  }
}


/*
 * For a variadic call, %al contains the number of XMM registers used
 * to pass floating-point arguments.
 */
static void sysv64_pre_call(Node *node, FILE *out) {
  if (!node->func_ty->is_variadic)
    return;

  int fp = 0;

  for (Node *arg = node->args; arg; arg = arg->next) {
    if (arg->pass_by_stack)
      continue;

    Type *ty = arg->ty;

    if (sysv64_is_aggregate(ty)) {
      SysVTypeClass cls = classify_sysv64(ty);
      fp += sysv64_num_fp(&cls);

    } else if (ty->kind == TY_FLOAT ||
               ty->kind == TY_DOUBLE) {
      fp++;
    }
  }

  println_abi(out,
              "  mov $%d, %%al",
              MIN(8, fp));
}


static const CallConv sysv64_callconv;

static const char *sysv64_reg32(const char *r64) {
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

static const char *sysv64_reg16(const char *r64) {
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

static const char *sysv64_reg8(const char *r64) {
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

static void sysv64_load_vreg(LLIRVReg *v, const char *reg, FILE *out) {
  if (!v) return;
  int offset = v->spill_offset ? v->spill_offset : -((v->id + 1) * 8);
  int sz = v->ty ? v->ty->size : 8;
  if (reg[1] == 'x') { // %xmm...
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
      println_abi(out, "  movl %d(%%rbp), %s", offset, sysv64_reg32(reg));
    else
      println_abi(out, "  movslq %d(%%rbp), %s", offset, reg);
  } else {
    println_abi(out, "  movq %d(%%rbp), %s", offset, reg);
  }
}

static void sysv64_store_vreg(const char *reg, LLIRVReg *v, FILE *out) {
  if (!v) return;
  int offset = v->spill_offset ? v->spill_offset : -((v->id + 1) * 8);
  if (reg[1] == 'x') {
    println_abi(out, "  movq %s, %d(%%rbp)", reg, offset);
  } else {
    println_abi(out, "  movq %s, %d(%%rbp)", reg, offset);
  }
}

static void sysv64_load_call_arg(LLIRVReg *arg, const char *reg, FILE *out) {
  if (!arg) return;
  if (arg->is_struct_val && arg->struct_size <= 8) {
    sysv64_load_vreg(arg, "%rax", out);
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
    sysv64_load_vreg(arg, reg, out);
  }
}

static void sysv64_emit_call(LLIRInsn *insn, FILE *out) {
  const CallConv *cc = &sysv64_callconv;

  /*
   * SysV AMD64:
   *
   *   GP: RDI, RSI, RDX, RCX, R8, R9
   *   FP: XMM0 .. XMM7
   *
   * A hidden structure-return pointer consumes RDI.
   *
   * We first classify every argument and determine whether it goes
   * in registers or memory.  Only after all stack arguments have been
   * materialized do we load the final register arguments.  This is
   * important because RDI/RSI/RCX/etc. are also useful scratch
   * registers when copying large arguments.
   */

  int gp_count = insn->var ? 1 : 0;
  int fp_count = 0;
  int stack_size = 0;

  /*
   * First pass:
   *
   * Determine the amount of outgoing stack argument space and consume
   * the GP/FP register sequences exactly as the second pass will.
   *
   * SysV requires an aggregate to be passed entirely in memory if:
   *
   *   - classification says MEMORY, or
   *   - it needs more registers of either class than remain.
   */
  for (int a = 0; a < insn->num_args; a++) {
    LLIRVReg *arg = insn->args[a];

    if (!arg) {
      if (gp_count < 6)
        gp_count++;
      else
        stack_size += 8;
      continue;
    }

    Type *ty = arg->struct_ty ? arg->struct_ty : arg->ty;

    if (arg->is_struct_val || (ty && sysv64_is_aggregate(ty))) {
      SysVTypeClass cls = classify_sysv64(ty);

      if (cls.n == 0 ||
          cls.cls[0] == SYSV_MEMORY ||
          cls.cls[1] == SYSV_MEMORY) {
        int sz = arg->struct_size ? arg->struct_size : ty->size;
        stack_size += align_to(sz, 8);
        continue;
      }

      int needed_gp = sysv64_num_gp(&cls);
      int needed_fp = sysv64_num_fp(&cls);

      /*
       * An aggregate must be passed wholly in registers or wholly
       * on the stack.  We may not put one eightbyte in a register
       * and the other on the stack.
       */
      if (gp_count + needed_gp <= 6 &&
          fp_count + needed_fp <= 8) {
        gp_count += needed_gp;
        fp_count += needed_fp;
      } else {
        int sz = arg->struct_size ? arg->struct_size : ty->size;
        stack_size += align_to(sz, 8);
      }

      continue;
    }

    if (ty && ty->kind == TY_LDOUBLE) {
      stack_size += 16;
      continue;
    }

    if (ty && (ty->kind == TY_FLOAT || ty->kind == TY_DOUBLE)) {
      if (fp_count < 8)
        fp_count++;
      else
        stack_size += 8;
      continue;
    }

    if (gp_count < 6)
      gp_count++;
    else
      stack_size += 8;
  }

  /*
   * Keep the outgoing argument area 16-byte aligned.
   */
  int total_alloc = align_to(stack_size, 16);

  if (total_alloc > 0)
    println_abi(out, "  sub $%d, %%rsp", total_alloc);

  /*
   * Second pass, part 1:
   *
   * Materialize all stack arguments first.
   */
  int gp_idx = insn->var ? 1 : 0;
  int fp_idx = 0;
  int stack_offset = 0;

  for (int a = 0; a < insn->num_args; a++) {
    LLIRVReg *arg = insn->args[a];

    if (!arg) {
      if (gp_idx >= 6) {
        println_abi(out, "  movq $0, %d(%%rsp)", stack_offset);
        stack_offset += 8;
      } else {
        gp_idx++;
      }
      continue;
    }

    Type *ty = arg->struct_ty ? arg->struct_ty : arg->ty;
    int sz = arg->struct_size ? arg->struct_size : (ty ? ty->size : 8);

    if (arg->is_struct_val || (ty && sysv64_is_aggregate(ty))) {
      SysVTypeClass cls = classify_sysv64(ty);

      bool in_regs = false;
      if (cls.n > 0 &&
          cls.cls[0] != SYSV_MEMORY &&
          cls.cls[1] != SYSV_MEMORY) {
        int needed_gp = sysv64_num_gp(&cls);
        int needed_fp = sysv64_num_fp(&cls);
        if (gp_idx + needed_gp <= 6 && fp_idx + needed_fp <= 8)
          in_regs = true;
      }

      if (!in_regs) {
        /*
         * The whole aggregate goes on the stack.
         */
        sysv64_load_vreg(arg, "%rsi", out);

        println_abi(out,
                    "  lea %d(%%rsp), %%rdi",
                    stack_offset);
        println_abi(out,
                    "  mov $%d, %%rcx",
                    sz);
        println_abi(out, "  rep movsb");

        stack_offset += align_to(sz, 8);
      } else {
        gp_idx += sysv64_num_gp(&cls);
        fp_idx += sysv64_num_fp(&cls);
      }

      continue;
    }

    if (ty && ty->kind == TY_LDOUBLE) {
      sysv64_load_vreg(arg, "%rsi", out);
      println_abi(out, "  lea %d(%%rsp), %%rdi", stack_offset);
      println_abi(out, "  mov $16, %%rcx");
      println_abi(out, "  rep movsb");
      stack_offset += 16;
      continue;
    }

    if (ty && (ty->kind == TY_FLOAT || ty->kind == TY_DOUBLE)) {
      if (fp_idx >= 8) {
        sysv64_load_call_arg(arg, "%rax", out);
        println_abi(out,
                    "  movq %%rax, %d(%%rsp)",
                    stack_offset);
        stack_offset += 8;
      } else {
        fp_idx++;
      }

      continue;
    }

    if (gp_idx >= 6) {
      sysv64_load_call_arg(arg, "%rax", out);
      println_abi(out,
                  "  movq %%rax, %d(%%rsp)",
                  stack_offset);
      stack_offset += 8;
    } else {
      gp_idx++;
    }
  }

  /*
   * Second pass, part 2:
   *
   * Load the final GP/FP argument registers.
   */
  gp_idx = 0;
  fp_idx = 0;

  if (insn->var) {
    /*
     * Hidden sret argument in RDI.
     */
    println_abi(out,
                "  lea %d(%%rbp), %%rdi",
                insn->var->offset);
    gp_idx = 1;
  }

  for (int a = 0; a < insn->num_args; a++) {
    LLIRVReg *arg = insn->args[a];

    if (!arg) {
      if (gp_idx < 6)
        gp_idx++;
      continue;
    }

    Type *ty = arg->struct_ty ? arg->struct_ty : arg->ty;
    int sz = arg->struct_size ? arg->struct_size : (ty ? ty->size : 8);

    if (arg->is_struct_val || (ty && sysv64_is_aggregate(ty))) {
      SysVTypeClass cls = classify_sysv64(ty);

      if (cls.n == 0 ||
          cls.cls[0] == SYSV_MEMORY ||
          cls.cls[1] == SYSV_MEMORY) {
        continue;
      }

      int needed_gp = sysv64_num_gp(&cls);
      int needed_fp = sysv64_num_fp(&cls);

      if (gp_idx + needed_gp > 6 ||
          fp_idx + needed_fp > 8) {
        continue;
      }

      sysv64_load_vreg(arg, "%rax", out);

      for (int i = 0; i < cls.n; i++) {
        int offset = i * 8;
        if (cls.cls[i] == SYSV_SSE || cls.cls[i] == SYSV_SSEUP) {
          println_abi(out,
                      "  movq %d(%%rax), %%xmm%d",
                      offset,
                      fp_idx++);
        } else if (cls.cls[i] == SYSV_INTEGER) {
          const char *r =
              (cc && cc->gp_regs64[gp_idx])
                  ? cc->gp_regs64[gp_idx]
                  : "%rdi";

          println_abi(out,
                      "  movq %d(%%rax), %s",
                      offset,
                      r);
          gp_idx++;
        }
      }

      continue;
    }

    if (ty && ty->kind == TY_LDOUBLE) {
      continue;
    }

    if (ty && (ty->kind == TY_FLOAT || ty->kind == TY_DOUBLE)) {
      if (fp_idx < 8) {
        sysv64_load_call_arg(arg, "%rax", out);
        println_abi(out,
                    "  movq %%rax, %%xmm%d",
                    fp_idx++);
      }

      continue;
    }

    if (gp_idx < 6) {
      const char *r =
          (cc && cc->gp_regs64[gp_idx])
              ? cc->gp_regs64[gp_idx]
              : "%rdi";

      sysv64_load_call_arg(arg, r, out);
      gp_idx++;
    }
  }

  /*
   * For SysV variadic calls, AL contains the number of vector registers used.
   */
  println_abi(out, "  mov $%d, %%al", fp_idx);

  if (insn->src1) {
    sysv64_load_vreg(insn->src1, "%r11", out);
    println_abi(out, "  call *%%r11");
  } else if (insn->label) {
    println_abi(out, "  call %s", insn->label);
  }

  if (total_alloc > 0)
    println_abi(out, "  add $%d, %%rsp", total_alloc);

  /*
   * Store the return value.
   *
   * A hidden-sret call has already written the result into insn->var,
   * so there is no register return value to save into dst.
   */
  if (insn->dst && !insn->var) {
    int offset =
        insn->dst->spill_offset
            ? insn->dst->spill_offset
            : -((insn->dst->id + 1) * 8);

    Type *ty = insn->dst->ty;

    if (ty && sysv64_is_aggregate(ty)) {
      SysVTypeClass cls = classify_sysv64(ty);

      if (cls.cls[0] != SYSV_MEMORY &&
          cls.cls[1] != SYSV_MEMORY &&
          cls.n > 0) {
        int gp = 0;
        int fp = 0;

        for (int i = 0; i < cls.n; i++) {
          int part_offset = i * 8;
          int part_size = MIN(8, ty->size - part_offset);

          if (cls.cls[i] == SYSV_INTEGER) {
            const char *r = (gp == 0) ? "%rax" : "%rdx";
            if (part_size == 8) {
              println_abi(out, "  movq %s, %d(%%rbp)", r, offset + part_offset);
            } else if (part_size == 4) {
              println_abi(out, "  movl %s, %d(%%rbp)", (gp == 0) ? "%eax" : "%edx", offset + part_offset);
            } else if (part_size == 2) {
              println_abi(out, "  movw %s, %d(%%rbp)", (gp == 0) ? "%ax" : "%dx", offset + part_offset);
            } else if (part_size == 1) {
              println_abi(out, "  movb %s, %d(%%rbp)", (gp == 0) ? "%al" : "%dl", offset + part_offset);
            } else {
              println_abi(out, "  movq %s, %d(%%rbp)", r, offset + part_offset);
            }
            gp++;
          } else if (cls.cls[i] == SYSV_SSE || cls.cls[i] == SYSV_SSEUP) {
            println_abi(out, "  movq %%xmm%d, %%r11", fp++);
            println_abi(out, "  movq %%r11, %d(%%rbp)", offset + part_offset);
          }
        }
      }
    } else if (ty && ty->kind == TY_LDOUBLE) {
      println_abi(out, "  fstpt %d(%%rbp)", offset);
    } else if (ty && (ty->kind == TY_FLOAT || ty->kind == TY_DOUBLE)) {
      println_abi(out, "  movq %%xmm0, %%rax");
      sysv64_store_vreg("%rax", insn->dst, out);
    } else {
      sysv64_store_vreg("%rax", insn->dst, out);
    }
  }
}

static Node *sysv64_builtin_va_start(Node *ap, Node *last, Token *tok) {
  VarScope *sc = find_var(&(Token){.loc = "__va_area__", .len = 11});
  if (!sc || !sc->var)
    error_tok(tok, "__builtin_va_start used outside variadic function");
  Node *va_var = new_var_node(sc->var, tok);
  VarScope *va_elem_sc = find_var(&(Token){.loc = "__va_elem", .len = 9});
  Type *va_elem_ty = (va_elem_sc && va_elem_sc->type_def) ? va_elem_sc->type_def : ty_void;
  Node *cast = new_cast(new_unary(ND_ADDR, va_var, tok), pointer_to(va_elem_ty));
  Node *deref_va = new_unary(ND_DEREF, cast, tok);
  Node *deref_ap = new_unary(ND_DEREF, ap, tok);
  return new_binary(ND_ASSIGN, deref_ap, deref_va, tok);
}

static Node *sysv64_builtin_va_arg(Node *ap, Type *ty, Token *tok) {
  add_type(ap);
  int klass = sysv64_classify_reg(ty);

  Obj *res = new_lvar("", pointer_to(ty_void));
  Node head = {};
  Node *cur = &head;

  Node *ap_char = new_cast(ap, pointer_to(ty_char));
  Node *of_addr = new_add(ap_char, new_num(8, tok), tok);
  Node *of_ptr = new_cast(of_addr, pointer_to(pointer_to(ty_void)));
  Node *reg_save_addr = new_add(ap_char, new_num(16, tok), tok);
  Node *reg_save_ptr = new_cast(reg_save_addr, pointer_to(pointer_to(ty_char)));

  if (klass == 0) {
    // INTEGER: gp_offset at offset 0
    Node *gp_ptr = new_cast(ap_char, pointer_to(ty_uint));
    Obj *gp = new_lvar("", ty_uint);
    cur = cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, new_var_node(gp, tok), new_unary(ND_DEREF, gp_ptr, tok), tok), tok);

    Node *if_node = new_node(ND_IF, tok);
    if_node->cond = new_binary(ND_LT, new_var_node(gp, tok), new_num(48, tok), tok);

    // then: res = reg_save_area + gp; gp_offset = gp + 8;
    Node then_head = {};
    Node *then_cur = &then_head;
    then_cur = then_cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, new_var_node(res, tok),
        new_cast(new_add(new_unary(ND_DEREF, reg_save_ptr, tok), new_var_node(gp, tok), tok), pointer_to(ty_void)), tok), tok);
    then_cur = then_cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, new_unary(ND_DEREF, gp_ptr, tok),
        new_binary(ND_ADD, new_var_node(gp, tok), new_num(8, tok), tok), tok), tok);
    if_node->then = new_node(ND_BLOCK, tok);
    if_node->then->body = then_head.next;

    // else: res = overflow_arg_area; overflow_arg_area = res + 8;
    Node else_head = {};
    Node *else_cur = &else_head;
    else_cur = else_cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, new_var_node(res, tok), new_unary(ND_DEREF, of_ptr, tok), tok), tok);
    else_cur = else_cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, new_unary(ND_DEREF, of_ptr, tok),
        new_cast(new_add(new_cast(new_var_node(res, tok), pointer_to(ty_char)), new_num(8, tok), tok), pointer_to(ty_void)), tok), tok);
    if_node->els = new_node(ND_BLOCK, tok);
    if_node->els->body = else_head.next;

    cur = cur->next = if_node;
  } else if (klass == 1) {
    // SSE: fp_offset at offset 4
    Node *fp_addr = new_add(ap_char, new_num(4, tok), tok);
    Node *fp_ptr = new_cast(fp_addr, pointer_to(ty_uint));
    Obj *fp = new_lvar("", ty_uint);
    cur = cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, new_var_node(fp, tok), new_unary(ND_DEREF, fp_ptr, tok), tok), tok);

    Node *if_node = new_node(ND_IF, tok);
    if_node->cond = new_binary(ND_LT, new_var_node(fp, tok), new_num(176, tok), tok);

    // then: res = reg_save_area + fp; fp_offset = fp + 16;
    Node then_head = {};
    Node *then_cur = &then_head;
    then_cur = then_cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, new_var_node(res, tok),
        new_cast(new_add(new_unary(ND_DEREF, reg_save_ptr, tok), new_var_node(fp, tok), tok), pointer_to(ty_void)), tok), tok);
    then_cur = then_cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, new_unary(ND_DEREF, fp_ptr, tok),
        new_binary(ND_ADD, new_var_node(fp, tok), new_num(16, tok), tok), tok), tok);
    if_node->then = new_node(ND_BLOCK, tok);
    if_node->then->body = then_head.next;

    // else: res = overflow_arg_area; overflow_arg_area = res + 8;
    Node else_head = {};
    Node *else_cur = &else_head;
    else_cur = else_cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, new_var_node(res, tok), new_unary(ND_DEREF, of_ptr, tok), tok), tok);
    else_cur = else_cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, new_unary(ND_DEREF, of_ptr, tok),
        new_cast(new_add(new_cast(new_var_node(res, tok), pointer_to(ty_char)), new_num(8, tok), tok), pointer_to(ty_void)), tok), tok);
    if_node->els = new_node(ND_BLOCK, tok);
    if_node->els->body = else_head.next;

    cur = cur->next = if_node;
  } else {
    // MEMORY
    cur = cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, new_var_node(res, tok), new_unary(ND_DEREF, of_ptr, tok), tok), tok);
    if (ty->align > 8) {
      Node *align_p = new_cast(new_binary(ND_BITAND,
        new_binary(ND_ADD, new_cast(new_var_node(res, tok), ty_ulong), new_num(15, tok), tok),
        new_num(-16, tok), tok), pointer_to(ty_void));
      cur = cur->next = new_unary(ND_EXPR_STMT,
        new_binary(ND_ASSIGN, new_var_node(res, tok), align_p, tok), tok);
    }
    int sz = align_to(ty->size, 8);
    cur = cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, new_unary(ND_DEREF, of_ptr, tok),
        new_cast(new_add(new_cast(new_var_node(res, tok), pointer_to(ty_char)), new_num(sz, tok), tok), pointer_to(ty_void)), tok), tok);
  }

  cur = cur->next = new_unary(ND_EXPR_STMT,
    new_unary(ND_DEREF, new_cast(new_var_node(res, tok), pointer_to(ty)), tok), tok);

  Node *node = new_node(ND_STMT_EXPR, tok);
  node->body = head.next;
  return node;
}

static Node *sysv64_builtin_va_copy(Node *dest, Node *src, Token *tok) {
  return new_binary(ND_ASSIGN, new_unary(ND_DEREF, dest, tok), new_unary(ND_DEREF, src, tok), tok);
}

static Node *sysv64_builtin_va_end(Node *ap, Token *tok) {
  Node *node = new_node(ND_NULL_EXPR, tok);
  node->ty = ty_void;
  return node;
}

static void sysv64_define_macros(void) {
  define_macro("__x86_64", "1");
  define_macro("__x86_64__", "1");
  define_macro("__amd64", "1");
  define_macro("__amd64__", "1");
  define_macro("__LP64__", "1");
  define_macro("_LP64", "1");
  define_macro("__ELF__", "1");
}


static void sysv64_init_types(void) {
  ty_bool->size = 1;
  ty_bool->align = 1;

  ty_char->size = 1;
  ty_char->align = 1;

  ty_short->size = 2;
  ty_short->align = 2;

  ty_int->size = 4;
  ty_int->align = 4;

  ty_long->size = 8;
  ty_long->align = 8;

  ty_llong->size = 8;
  ty_llong->align = 8;

  ty_uchar->size = 1;
  ty_uchar->align = 1;

  ty_ushort->size = 2;
  ty_ushort->align = 2;

  ty_uint->size = 4;
  ty_uint->align = 4;

  ty_ulong->size = 8;
  ty_ulong->align = 8;

  ty_ullong->size = 8;
  ty_ullong->align = 8;

  ty_float->size = 4;
  ty_float->align = 4;

  ty_double->size = 8;
  ty_double->align = 8;

  ty_ldouble->size = 16;
  ty_ldouble->align = 16;
}

static void sysv64_declare_builtin_types(void) {
  Type *va_elem = struct_type();
  Member *m1 = calloc(1, sizeof(Member));
  m1->ty = ty_uint;
  m1->name = &(Token){.loc = "gp_offset", .len = 9};
  m1->offset = 0;

  Member *m2 = calloc(1, sizeof(Member));
  m2->ty = ty_uint;
  m2->name = &(Token){.loc = "fp_offset", .len = 9};
  m2->offset = 4;

  Member *m3 = calloc(1, sizeof(Member));
  m3->ty = pointer_to(ty_void);
  m3->name = &(Token){.loc = "overflow_arg_area", .len = 17};
  m3->offset = 8;

  Member *m4 = calloc(1, sizeof(Member));
  m4->ty = pointer_to(ty_void);
  m4->name = &(Token){.loc = "reg_save_area", .len = 13};
  m4->offset = 16;

  m1->next = m2;
  m2->next = m3;
  m3->next = m4;
  va_elem->members = m1;
  va_elem->size = 24;
  va_elem->align = 8;

  Type *va_list_ty = array_of(va_elem, 1);

  push_scope("__builtin_va_list")->type_def = va_list_ty;
  push_scope("__va_elem")->type_def = va_elem;
}


static const CallConv sysv64_callconv = {
  .name = "sysv64",

  .num_gp_regs = 6,

  .gp_regs64 = {
    "%rdi",
    "%rsi",
    "%rdx",
    "%rcx",
    "%r8",
    "%r9"
  },

  .gp_regs32 = {
    "%edi",
    "%esi",
    "%edx",
    "%ecx",
    "%r8d",
    "%r9d"
  },

  .gp_regs16 = {
    "%di",
    "%si",
    "%dx",
    "%cx",
    "%r8w",
    "%r9w"
  },

  .gp_regs8 = {
    "%dil",
    "%sil",
    "%dl",
    "%cl",
    "%r8b",
    "%r9b"
  },

  .num_fp_regs = 8,
  .shadow_space = 0,
  .paired_slots = false,
  .pass_struct_by_ref = false,
  .stack_align = 16,
};


ABI abi_sysv64 = {
  .name = "sysv64",
  .description = "System V AMD64 psABI (x86_64 Linux/BSD/macOS)",

  .callconv = &sysv64_callconv,

  .default_objfmt = &objfmt_elf,

  .size_bool = 1,
  .align_bool = 1,

  .size_char = 1,
  .align_char = 1,

  .size_short = 2,
  .align_short = 2,

  .size_int = 4,
  .align_int = 4,

  .size_long = 8,
  .align_long = 8,

  .size_llong = 8,
  .align_llong = 8,

  .size_ptr = 8,
  .align_ptr = 8,

  .size_float = 4,
  .align_float = 4,

  .size_double = 8,
  .align_double = 8,

  .size_ldouble = 16,
  .align_ldouble = 16,

  .align_stack = 16,

  .va_area_size = 208,
  .va_area_align = 16,

  .returns_by_reference = sysv64_returns_by_reference,
  .classify_reg = sysv64_classify_reg,
  .assign_lvar_offsets = sysv64_assign_lvar_offsets,
  .get_spill_base = sysv64_get_spill_base,
  .finalize_stack = sysv64_finalize_stack,
  .push_args = sysv64_push_args,
  .copy_ret_buffer = sysv64_copy_ret_buffer,
  .copy_struct_reg = sysv64_copy_struct_reg,
  .copy_struct_mem = sysv64_copy_struct_mem,
  .builtin_alloca = sysv64_builtin_alloca,
  .pre_call = sysv64_pre_call,
  .builtin_va_start = sysv64_builtin_va_start,
  .builtin_va_arg = sysv64_builtin_va_arg,
  .builtin_va_copy = sysv64_builtin_va_copy,
  .builtin_va_end = sysv64_builtin_va_end,
  .emit_prologue = sysv64_emit_prologue,
  .emit_epilogue = sysv64_emit_epilogue,
  .emit_return = sysv64_emit_return,
  .emit_call = sysv64_emit_call,
  .define_macros = sysv64_define_macros,
  .init_types = sysv64_init_types,
  .declare_builtin_types = sysv64_declare_builtin_types,
};

