#include "ir/hlir.h"
#include "abi/abi.h"
#include <stdlib.h>
#include <string.h>

HLIRVal *hlir_new_val(HLIRFunction *fn, Type *ty) {
  HLIRVal *val = calloc(1, sizeof(HLIRVal));
  val->id = fn->num_vals++;
  val->ty = ty;
  if (fn->num_vals > fn->val_cap) {
    fn->val_cap = fn->val_cap ? fn->val_cap * 2 : 16;
    fn->vals = realloc(fn->vals, sizeof(HLIRVal *) * fn->val_cap);
  }
  fn->vals[val->id] = val;
  return val;
}

HLIRInsn *hlir_new_insn(HLIRKind kind) {
  HLIRInsn *insn = calloc(1, sizeof(HLIRInsn));
  insn->kind = kind;
  return insn;
}

void hlir_append_insn(HLIRFunction *fn, HLIRInsn *insn) {
  if (!fn->head) {
    fn->head = fn->tail = insn;
  } else {
    fn->tail->next = insn;
    insn->prev = fn->tail;
    fn->tail = insn;
  }
  insn->pos = fn->num_insns++;
}

void hlir_insert_before(HLIRFunction *fn, HLIRInsn *target, HLIRInsn *new_insn) {
  if (!target) {
    hlir_append_insn(fn, new_insn);
    return;
  }
  new_insn->next = target;
  new_insn->prev = target->prev;
  if (target->prev)
    target->prev->next = new_insn;
  else
    fn->head = new_insn;
  target->prev = new_insn;
  fn->num_insns++;
}

void hlir_insert_after(HLIRFunction *fn, HLIRInsn *target, HLIRInsn *new_insn) {
  if (!target) {
    hlir_append_insn(fn, new_insn);
    return;
  }
  new_insn->prev = target;
  new_insn->next = target->next;
  if (target->next)
    target->next->prev = new_insn;
  else
    fn->tail = new_insn;
  target->next = new_insn;
  fn->num_insns++;
}

void hlir_remove_insn(HLIRFunction *fn, HLIRInsn *insn) {
  if (!insn || !fn)
    return;
  if (insn->prev)
    insn->prev->next = insn->next;
  else
    fn->head = insn->next;
  if (insn->next)
    insn->next->prev = insn->prev;
  else
    fn->tail = insn->prev;
  insn->prev = insn->next = NULL;
  fn->num_insns--;
}

void hlir_replace_insn(HLIRFunction *fn, HLIRInsn *old_insn, HLIRInsn *new_insn) {
  if (!old_insn || !new_insn || !fn)
    return;
  new_insn->prev = old_insn->prev;
  new_insn->next = old_insn->next;
  if (old_insn->prev)
    old_insn->prev->next = new_insn;
  else
    fn->head = new_insn;
  if (old_insn->next)
    old_insn->next->prev = new_insn;
  else
    fn->tail = new_insn;
  old_insn->prev = old_insn->next = NULL;
}

void hlir_dump_function(FILE *out, HLIRFunction *fn) {
  if (!out || !fn) return;
  fprintf(out, "hlir_function %s() [vals: %d, insns: %d]:\n", fn->name, fn->num_vals, fn->num_insns);
  for (HLIRInsn *insn = fn->head; insn; insn = insn->next) {
    fprintf(out, "  %4d: ", insn->pos);
    switch (insn->kind) {
    case HLIR_ICONST:
      fprintf(out, "%%%d = iconst %lld\n", insn->dst ? insn->dst->id : -1, (long long)insn->imm);
      break;
    case HLIR_FCONST:
      fprintf(out, "%%%d = fconst %f\n", insn->dst ? insn->dst->id : -1, insn->fimm);
      break;
    case HLIR_SCONST:
      fprintf(out, "%%%d = sconst &%s\n", insn->dst ? insn->dst->id : -1, insn->label ? insn->label : "");
      break;
    case HLIR_LOAD_VAR:
      fprintf(out, "%%%d = load_var %s\n", insn->dst ? insn->dst->id : -1, insn->var ? insn->var->name : "");
      break;
    case HLIR_STORE_VAR:
      fprintf(out, "store_var %s = %%%d\n", insn->var ? insn->var->name : "", insn->src1 ? insn->src1->id : -1);
      break;
    case HLIR_ADDR_VAR:
      fprintf(out, "%%%d = addr_var &%s\n", insn->dst ? insn->dst->id : -1, insn->var ? insn->var->name : "");
      break;
    case HLIR_LOAD_PTR:
      fprintf(out, "%%%d = load_ptr *%%%d\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1);
      break;
    case HLIR_STORE_PTR:
      fprintf(out, "store_ptr *%%%d = %%%d\n", insn->src1 ? insn->src1->id : -1, insn->src2 ? insn->src2->id : -1);
      break;
    case HLIR_ADD:
      fprintf(out, "%%%d = add %%%d, %%%d\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1, insn->src2 ? insn->src2->id : -1);
      break;
    case HLIR_SUB:
      fprintf(out, "%%%d = sub %%%d, %%%d\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1, insn->src2 ? insn->src2->id : -1);
      break;
    case HLIR_MUL:
      fprintf(out, "%%%d = mul %%%d, %%%d\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1, insn->src2 ? insn->src2->id : -1);
      break;
    case HLIR_DIV:
      fprintf(out, "%%%d = div %%%d, %%%d\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1, insn->src2 ? insn->src2->id : -1);
      break;
    case HLIR_LABEL:
      fprintf(out, "%s:\n", insn->label ? insn->label : "");
      break;
    case HLIR_JMP:
      fprintf(out, "jmp %s\n", insn->label ? insn->label : "");
      break;
    case HLIR_JMP_IF_ZERO:
      fprintf(out, "jmp_if_zero %%%d, %s\n", insn->src1 ? insn->src1->id : -1, insn->label ? insn->label : "");
      break;
    case HLIR_JMP_IF_NZ:
      fprintf(out, "jmp_if_nz %%%d, %s\n", insn->src1 ? insn->src1->id : -1, insn->label ? insn->label : "");
      break;
    case HLIR_RET:
      if (insn->src1)
        fprintf(out, "ret %%%d\n", insn->src1->id);
      else
        fprintf(out, "ret\n");
      break;
    case HLIR_CALL:
      if (insn->dst)
        fprintf(out, "%%%d = call %%%d(%d args)\n", insn->dst->id, insn->src1 ? insn->src1->id : -1, insn->num_args);
      else
        fprintf(out, "call %%%d(%d args)\n", insn->src1 ? insn->src1->id : -1, insn->num_args);
      break;
    default:
      fprintf(out, "hlir kind %d\n", insn->kind);
      break;
    }
  }
}

void hlir_dump(FILE *out, HLIRProg *prog) {
  if (!out || !prog) return;
  for (int i = 0; i < prog->num_fns; i++)
    hlir_dump_function(out, prog->fns[i]);
}

static HLIRVal *gen_expr_hlir(HLIRFunction *fn, Node *node);
static void gen_stmt_hlir(HLIRFunction *fn, Node *node);
static void gen_asm_hlir(HLIRFunction *fn, Node *node);

static HLIRVal *gen_addr_hlir(HLIRFunction *fn, Node *node) {
  switch (node->kind) {
  case ND_VAR:
  case ND_VLA_PTR: {
    HLIRVal *dst = hlir_new_val(fn, pointer_to(node->var->ty));
    HLIRInsn *insn = hlir_new_insn(HLIR_ADDR_VAR);
    insn->dst = dst;
    insn->var = node->var;
    hlir_append_insn(fn, insn);
    return dst;
  }
  case ND_DEREF:
    return gen_expr_hlir(fn, node->lhs);
  case ND_COMMA:
    gen_expr_hlir(fn, node->lhs);
    return gen_addr_hlir(fn, node->rhs);
  case ND_MEMBER: {
    HLIRVal *base = gen_addr_hlir(fn, node->lhs);
    if (node->member->offset == 0)
      return base;
    HLIRVal *offset = hlir_new_val(fn, ty_long);
    HLIRInsn *imm_insn = hlir_new_insn(HLIR_ICONST);
    imm_insn->dst = offset;
    imm_insn->imm = node->member->offset;
    hlir_append_insn(fn, imm_insn);

    HLIRVal *dst = hlir_new_val(fn, pointer_to(node->ty));
    HLIRInsn *add_insn = hlir_new_insn(HLIR_ADD);
    add_insn->dst = dst;
    add_insn->src1 = base;
    add_insn->src2 = offset;
    hlir_append_insn(fn, add_insn);
    return dst;
  }
  case ND_FUNCALL:
    if (node->ret_buffer)
      return gen_expr_hlir(fn, node);
    if (node->ty && (node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION)) {
      HLIRVal *val = gen_expr_hlir(fn, node);
      Obj2 *tmp = calloc(1, sizeof(Obj2));
      tmp->name = "";
      tmp->ty = node->ty;
      tmp->is_local = true;
      tmp->align = node->ty->align;
      if (fn->fn_obj) {
        tmp->next = fn->fn_obj->locals;
        fn->fn_obj->locals = tmp;
      }

      HLIRVal *tmp_addr = hlir_new_val(fn, pointer_to(node->ty));
      HLIRInsn *addr_insn = hlir_new_insn(HLIR_ADDR_VAR);
      addr_insn->dst = tmp_addr;
      addr_insn->var = tmp;
      hlir_append_insn(fn, addr_insn);

      HLIRInsn *store = hlir_new_insn(HLIR_STORE_PTR);
      store->src1 = tmp_addr;
      store->src2 = val;
      store->ty = node->ty;
      hlir_append_insn(fn, store);
      return tmp_addr;
    }
    break;
  case ND_STMT_EXPR:
    for (Node *n = node->body; n; n = n->next) {
      if (!n->next && n->kind == ND_EXPR_STMT)
        return gen_addr_hlir(fn, n->lhs);
      gen_stmt_hlir(fn, n);
    }
    return NULL;
  case ND_ASSIGN:
  case ND_COND:
    if (node->ty && (node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION))
      return gen_expr_hlir(fn, node);
    break;
  default:
    break;
  }
  error_tok(node->tok, "not an lvalue in HLIR generator");
  return NULL;
}

static HLIRVal *gen_expr_hlir(HLIRFunction *fn, Node *node) {
  if (!node)
    return NULL;

  switch (node->kind) {
  case ND_NULL_EXPR:
    return NULL;
  case ND_NUM: {
    HLIRVal *dst = hlir_new_val(fn, node->ty);
    if (is_flonum(node->ty)) {
      HLIRInsn *insn = hlir_new_insn(HLIR_FCONST);
      insn->dst = dst;
      insn->fimm = node->fval;
      insn->ty = node->ty;
      hlir_append_insn(fn, insn);
    } else {
      HLIRInsn *insn = hlir_new_insn(HLIR_ICONST);
      insn->dst = dst;
      insn->imm = node->val;
      insn->ty = node->ty;
      hlir_append_insn(fn, insn);
    }
    return dst;
  }
  case ND_VLA_PTR: {
    HLIRVal *addr = gen_addr_hlir(fn, node);
    HLIRVal *dst = hlir_new_val(fn, pointer_to(node->var->ty));
    HLIRInsn *insn = hlir_new_insn(HLIR_LOAD_PTR);
    insn->dst = dst;
    insn->src1 = addr;
    insn->ty = pointer_to(node->var->ty);
    hlir_append_insn(fn, insn);
    return dst;
  }
  case ND_VAR: {
    if (node->var->ty->kind == TY_VLA) {
      HLIRVal *addr = gen_addr_hlir(fn, node);
      HLIRVal *dst = hlir_new_val(fn, pointer_to(node->var->ty->base));
      HLIRInsn *insn = hlir_new_insn(HLIR_LOAD_PTR);
      insn->dst = dst;
      insn->src1 = addr;
      insn->ty = pointer_to(node->var->ty->base);
      hlir_append_insn(fn, insn);
      return dst;
    }
    HLIRVal *addr = gen_addr_hlir(fn, node);
    if (node->ty->kind == TY_ARRAY || node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION || node->ty->kind == TY_FUNC)
      return addr;
    HLIRVal *dst = hlir_new_val(fn, node->ty);
    HLIRInsn *insn = hlir_new_insn(HLIR_LOAD_PTR);
    insn->dst = dst;
    insn->src1 = addr;
    insn->ty = node->ty;
    hlir_append_insn(fn, insn);
    return dst;
  }
  case ND_MEMBER: {
    HLIRVal *addr = gen_addr_hlir(fn, node);
    if (node->ty->kind == TY_ARRAY || node->ty->kind == TY_VLA || node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION || node->ty->kind == TY_FUNC)
      return addr;
    HLIRVal *dst = hlir_new_val(fn, node->ty);
    HLIRInsn *insn = hlir_new_insn(HLIR_LOAD_PTR);
    insn->dst = dst;
    insn->src1 = addr;
    insn->ty = node->ty;
    hlir_append_insn(fn, insn);

    if (node->member && node->member->is_bitfield) {
      Member *mem = node->member;
      int total_bits = (mem->ty ? mem->ty->size : 8) * 8;
      if (mem->bit_width < total_bits) {
        HLIRVal *shl_dst = hlir_new_val(fn, node->ty);
        HLIRVal *shl_amt = hlir_new_val(fn, ty_int);
        HLIRInsn *imm1 = hlir_new_insn(HLIR_ICONST);
        imm1->dst = shl_amt;
        imm1->imm = total_bits - mem->bit_width - mem->bit_offset;
        imm1->ty = ty_int;
        hlir_append_insn(fn, imm1);

        HLIRInsn *shl = hlir_new_insn(HLIR_SHL);
        shl->dst = shl_dst;
        shl->src1 = dst;
        shl->src2 = shl_amt;
        shl->ty = node->ty;
        hlir_append_insn(fn, shl);

        HLIRVal *shr_dst = hlir_new_val(fn, node->ty);
        HLIRVal *shr_amt = hlir_new_val(fn, ty_int);
        HLIRInsn *imm2 = hlir_new_insn(HLIR_ICONST);
        imm2->dst = shr_amt;
        imm2->imm = total_bits - mem->bit_width;
        imm2->ty = ty_int;
        hlir_append_insn(fn, imm2);

        HLIRInsn *shr = hlir_new_insn(HLIR_SHR);
        shr->dst = shr_dst;
        shr->src1 = shl_dst;
        shr->src2 = shr_amt;
        shr->ty = node->ty;
        hlir_append_insn(fn, shr);
        return shr_dst;
      }
    }
    return dst;
  }
  case ND_DEREF: {
    HLIRVal *addr = gen_expr_hlir(fn, node->lhs);
    if (node->ty->kind == TY_ARRAY || node->ty->kind == TY_VLA || node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION || node->ty->kind == TY_FUNC)
      return addr;
    HLIRVal *dst = hlir_new_val(fn, node->ty);
    HLIRInsn *insn = hlir_new_insn(HLIR_LOAD_PTR);
    insn->dst = dst;
    insn->src1 = addr;
    insn->ty = node->ty;
    hlir_append_insn(fn, insn);
    return dst;
  }
  case ND_ADDR:
    return gen_addr_hlir(fn, node->lhs);
  case ND_LABEL_VAL: {
    HLIRVal *dst = hlir_new_val(fn, pointer_to(ty_void));
    HLIRInsn *insn = hlir_new_insn(HLIR_SCONST);
    insn->dst = dst;
    insn->label = node->unique_label;
    hlir_append_insn(fn, insn);
    return dst;
  }
  case ND_ASSIGN: {
    if (node->lhs && node->lhs->kind == ND_MEMBER && node->lhs->member && node->lhs->member->is_bitfield) {
      Member *mem = node->lhs->member;
      HLIRVal *rhs = gen_expr_hlir(fn, node->rhs);
      HLIRVal *addr = gen_addr_hlir(fn, node->lhs);

      HLIRVal *old_val = hlir_new_val(fn, mem->ty);
      HLIRInsn *load = hlir_new_insn(HLIR_LOAD_PTR);
      load->dst = old_val;
      load->src1 = addr;
      load->ty = mem->ty;
      hlir_append_insn(fn, load);

      uint64_t value_mask = mem->bit_width >= 64 ? UINT64_MAX : ((1ULL << mem->bit_width) - 1);
      uint64_t field_mask = mem->bit_width >= 64 ? UINT64_MAX : (value_mask << mem->bit_offset);

      HLIRVal *v_mask = hlir_new_val(fn, mem->ty);
      HLIRInsn *imm_vmask = hlir_new_insn(HLIR_ICONST);
      imm_vmask->dst = v_mask;
      imm_vmask->imm = value_mask;
      imm_vmask->ty = mem->ty;
      hlir_append_insn(fn, imm_vmask);

      HLIRVal *masked_rhs = hlir_new_val(fn, mem->ty);
      HLIRInsn *and_rhs = hlir_new_insn(HLIR_BITAND);
      and_rhs->dst = masked_rhs;
      and_rhs->src1 = rhs;
      and_rhs->src2 = v_mask;
      and_rhs->ty = mem->ty;
      hlir_append_insn(fn, and_rhs);

      HLIRVal *shifted_rhs = masked_rhs;
      if (mem->bit_offset > 0) {
        HLIRVal *sh_amt = hlir_new_val(fn, ty_int);
        HLIRInsn *imm_sh = hlir_new_insn(HLIR_ICONST);
        imm_sh->dst = sh_amt;
        imm_sh->imm = mem->bit_offset;
        imm_sh->ty = ty_int;
        hlir_append_insn(fn, imm_sh);

        shifted_rhs = hlir_new_val(fn, mem->ty);
        HLIRInsn *shl = hlir_new_insn(HLIR_SHL);
        shl->dst = shifted_rhs;
        shl->src1 = masked_rhs;
        shl->src2 = sh_amt;
        shl->ty = mem->ty;
        hlir_append_insn(fn, shl);
      }

      HLIRVal *f_mask = hlir_new_val(fn, mem->ty);
      HLIRInsn *imm_fmask = hlir_new_insn(HLIR_ICONST);
      imm_fmask->dst = f_mask;
      imm_fmask->imm = ~field_mask;
      imm_fmask->ty = mem->ty;
      hlir_append_insn(fn, imm_fmask);

      HLIRVal *cleared_old = hlir_new_val(fn, mem->ty);
      HLIRInsn *and_old = hlir_new_insn(HLIR_BITAND);
      and_old->dst = cleared_old;
      and_old->src1 = old_val;
      and_old->src2 = f_mask;
      and_old->ty = mem->ty;
      hlir_append_insn(fn, and_old);

      HLIRVal *combined = hlir_new_val(fn, mem->ty);
      HLIRInsn *or_insn = hlir_new_insn(HLIR_BITOR);
      or_insn->dst = combined;
      or_insn->src1 = cleared_old;
      or_insn->src2 = shifted_rhs;
      or_insn->ty = mem->ty;
      hlir_append_insn(fn, or_insn);

      HLIRInsn *store = hlir_new_insn(HLIR_STORE_PTR);
      store->src1 = addr;
      store->src2 = combined;
      store->ty = mem->ty;
      hlir_append_insn(fn, store);

      return rhs;
    }
    if (node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION) {
      if (node->rhs->kind == ND_FUNCALL && node->ty->size <= 16 && !node->rhs->ret_buffer) {
        HLIRVal *r = gen_expr_hlir(fn, node->rhs);
        HLIRVal *l = gen_addr_hlir(fn, node->lhs);
        HLIRInsn *insn = hlir_new_insn(HLIR_STORE_PTR);
        insn->src1 = l;
        insn->src2 = r;
        insn->ty = node->ty;
        hlir_append_insn(fn, insn);
        return l;
      }
      HLIRVal *r = gen_expr_hlir(fn, node->rhs);
      HLIRVal *l = gen_addr_hlir(fn, node->lhs);
      HLIRInsn *insn = hlir_new_insn(HLIR_MEMCPY);
      insn->src1 = l;
      insn->src2 = r;
      insn->imm = node->ty->size;
      hlir_append_insn(fn, insn);
      return l;
    }
    HLIRVal *r = gen_expr_hlir(fn, node->rhs);
    HLIRVal *l = gen_addr_hlir(fn, node->lhs);
    HLIRInsn *insn = hlir_new_insn(HLIR_STORE_PTR);
    insn->src1 = l;
    insn->src2 = r;
    insn->ty = node->ty;
    hlir_append_insn(fn, insn);
    return r;
  }
  case ND_CAST: {
    HLIRVal *src = gen_expr_hlir(fn, node->lhs);
    if (node->ty->kind == TY_VOID)
      return NULL;
    if (src && node->lhs && node->lhs->ty) {
      if (node->lhs->ty == node->ty)
        return src;
      if (node->lhs->ty->base && node->ty->base)
        return src;
      if (node->lhs->ty->size == node->ty->size &&
          node->lhs->ty->is_unsigned == node->ty->is_unsigned &&
          is_flonum(node->lhs->ty) == is_flonum(node->ty))
        return src;
    }
    HLIRVal *dst = hlir_new_val(fn, node->ty);
    HLIRInsn *insn = hlir_new_insn(HLIR_CAST);
    insn->dst = dst;
    insn->src1 = src;
    insn->ty = node->ty;
    hlir_append_insn(fn, insn);
    return dst;
  }
  case ND_NEG: {
    HLIRVal *src = gen_expr_hlir(fn, node->lhs);
    HLIRVal *dst = hlir_new_val(fn, node->ty);
    HLIRInsn *insn = hlir_new_insn(HLIR_NEG);
    insn->dst = dst;
    insn->src1 = src;
    insn->ty = node->ty;
    hlir_append_insn(fn, insn);
    return dst;
  }
  case ND_BITNOT: {
    HLIRVal *src = gen_expr_hlir(fn, node->lhs);
    HLIRVal *dst = hlir_new_val(fn, node->ty);
    HLIRInsn *insn = hlir_new_insn(HLIR_BITNOT);
    insn->dst = dst;
    insn->src1 = src;
    insn->ty = node->ty;
    hlir_append_insn(fn, insn);
    return dst;
  }
  case ND_NOT: {
    HLIRVal *src = gen_expr_hlir(fn, node->lhs);
    HLIRVal *dst = hlir_new_val(fn, node->ty);
    HLIRInsn *insn = hlir_new_insn(HLIR_LOGNOT);
    insn->dst = dst;
    insn->src1 = src;
    insn->ty = node->ty;
    hlir_append_insn(fn, insn);
    return dst;
  }
  case ND_ADD:
  case ND_SUB:
  case ND_MUL:
  case ND_DIV:
  case ND_MOD:
  case ND_BITAND:
  case ND_BITOR:
  case ND_BITXOR:
  case ND_SHL:
  case ND_SHR:
  case ND_EQ:
  case ND_NE:
  case ND_LT:
  case ND_LE: {
    HLIRVal *l = gen_expr_hlir(fn, node->lhs);
    HLIRVal *r = gen_expr_hlir(fn, node->rhs);
    HLIRVal *dst = hlir_new_val(fn, node->ty);
    HLIRKind k = HLIR_NOP;
    switch (node->kind) {
    case ND_ADD: k = HLIR_ADD; break;
    case ND_SUB: k = HLIR_SUB; break;
    case ND_MUL: k = HLIR_MUL; break;
    case ND_DIV: k = HLIR_DIV; break;
    case ND_MOD: k = HLIR_MOD; break;
    case ND_BITAND: k = HLIR_BITAND; break;
    case ND_BITOR: k = HLIR_BITOR; break;
    case ND_BITXOR: k = HLIR_BITXOR; break;
    case ND_SHL: k = HLIR_SHL; break;
    case ND_SHR: k = HLIR_SHR; break;
    case ND_EQ: k = HLIR_CMP_EQ; break;
    case ND_NE: k = HLIR_CMP_NE; break;
    case ND_LT: k = HLIR_CMP_LT; break;
    case ND_LE: k = HLIR_CMP_LE; break;
    default: break;
    }
    HLIRInsn *insn = hlir_new_insn(k);
    insn->dst = dst;
    insn->src1 = l;
    insn->src2 = r;
    insn->ty = node->ty;
    hlir_append_insn(fn, insn);
    return dst;
  }
  case ND_LOGAND: {
    static int id = 0;
    int c = id++;
    char *false_label = format(".L.hlir.logand.false.%d", c);
    char *end_label = format(".L.hlir.logand.end.%d", c);

    HLIRVal *dst = hlir_new_val(fn, ty_int);

    HLIRVal *l = gen_expr_hlir(fn, node->lhs);
    HLIRInsn *br1 = hlir_new_insn(HLIR_JMP_IF_ZERO);
    br1->src1 = l;
    br1->label = false_label;
    hlir_append_insn(fn, br1);

    HLIRVal *r = gen_expr_hlir(fn, node->rhs);
    HLIRInsn *br2 = hlir_new_insn(HLIR_JMP_IF_ZERO);
    br2->src1 = r;
    br2->label = false_label;
    hlir_append_insn(fn, br2);

    HLIRInsn *imm1 = hlir_new_insn(HLIR_ICONST);
    imm1->dst = dst;
    imm1->imm = 1;
    imm1->ty = ty_int;
    hlir_append_insn(fn, imm1);

    HLIRInsn *jmp_end = hlir_new_insn(HLIR_JMP);
    jmp_end->label = end_label;
    hlir_append_insn(fn, jmp_end);

    HLIRInsn *lbl_false = hlir_new_insn(HLIR_LABEL);
    lbl_false->label = false_label;
    hlir_append_insn(fn, lbl_false);

    HLIRInsn *imm0 = hlir_new_insn(HLIR_ICONST);
    imm0->dst = dst;
    imm0->imm = 0;
    imm0->ty = ty_int;
    hlir_append_insn(fn, imm0);

    HLIRInsn *lbl_end = hlir_new_insn(HLIR_LABEL);
    lbl_end->label = end_label;
    hlir_append_insn(fn, lbl_end);

    return dst;
  }
  case ND_LOGOR: {
    static int id = 0;
    int c = id++;
    char *true_label = format(".L.hlir.logor.true.%d", c);
    char *end_label = format(".L.hlir.logor.end.%d", c);

    HLIRVal *dst = hlir_new_val(fn, ty_int);

    HLIRVal *l = gen_expr_hlir(fn, node->lhs);
    HLIRInsn *br1 = hlir_new_insn(HLIR_JMP_IF_NZ);
    br1->src1 = l;
    br1->label = true_label;
    hlir_append_insn(fn, br1);

    HLIRVal *r = gen_expr_hlir(fn, node->rhs);
    HLIRInsn *br2 = hlir_new_insn(HLIR_JMP_IF_NZ);
    br2->src1 = r;
    br2->label = true_label;
    hlir_append_insn(fn, br2);

    HLIRInsn *imm0 = hlir_new_insn(HLIR_ICONST);
    imm0->dst = dst;
    imm0->imm = 0;
    imm0->ty = ty_int;
    hlir_append_insn(fn, imm0);

    HLIRInsn *jmp_end = hlir_new_insn(HLIR_JMP);
    jmp_end->label = end_label;
    hlir_append_insn(fn, jmp_end);

    HLIRInsn *lbl_true = hlir_new_insn(HLIR_LABEL);
    lbl_true->label = true_label;
    hlir_append_insn(fn, lbl_true);

    HLIRInsn *imm1 = hlir_new_insn(HLIR_ICONST);
    imm1->dst = dst;
    imm1->imm = 1;
    imm1->ty = ty_int;
    hlir_append_insn(fn, imm1);

    HLIRInsn *lbl_end = hlir_new_insn(HLIR_LABEL);
    lbl_end->label = end_label;
    hlir_append_insn(fn, lbl_end);

    return dst;
  }
  case ND_COND: {
    static int id = 0;
    int c = id++;
    char *else_lbl = format(".L.hlir.cond.else.%d", c);
    char *end_lbl = format(".L.hlir.cond.end.%d", c);

    HLIRVal *cond = gen_expr_hlir(fn, node->cond);
    HLIRInsn *br = hlir_new_insn(HLIR_JMP_IF_ZERO);
    br->src1 = cond;
    br->label = else_lbl;
    hlir_append_insn(fn, br);

    HLIRVal *dst = node->ty->kind == TY_VOID ? NULL : hlir_new_val(fn, node->ty);
    HLIRVal *then_val = gen_expr_hlir(fn, node->then);
    if (dst && then_val) {
      HLIRInsn *mov = hlir_new_insn(HLIR_CAST);
      mov->dst = dst;
      mov->src1 = then_val;
      mov->ty = node->ty;
      hlir_append_insn(fn, mov);
    }

    HLIRInsn *jmp = hlir_new_insn(HLIR_JMP);
    jmp->label = end_lbl;
    hlir_append_insn(fn, jmp);

    HLIRInsn *lbl_else = hlir_new_insn(HLIR_LABEL);
    lbl_else->label = else_lbl;
    hlir_append_insn(fn, lbl_else);

    HLIRVal *els_val = gen_expr_hlir(fn, node->els);
    if (dst && els_val) {
      HLIRInsn *mov = hlir_new_insn(HLIR_CAST);
      mov->dst = dst;
      mov->src1 = els_val;
      mov->ty = node->ty;
      hlir_append_insn(fn, mov);
    }

    HLIRInsn *lbl_end = hlir_new_insn(HLIR_LABEL);
    lbl_end->label = end_lbl;
    hlir_append_insn(fn, lbl_end);
    return dst;
  }
  case ND_COMMA:
    gen_expr_hlir(fn, node->lhs);
    return gen_expr_hlir(fn, node->rhs);
  case ND_STMT_EXPR:
    for (Node *n = node->body; n; n = n->next) {
      if (!n->next && n->kind == ND_EXPR_STMT)
        return gen_expr_hlir(fn, n->lhs);
      gen_stmt_hlir(fn, n);
    }
    return NULL;
  case ND_FUNCALL: {
    if (node->lhs->kind == ND_VAR && node->lhs->var && !strcmp(node->lhs->var->name, "alloca")) {
      HLIRVal *sz = gen_expr_hlir(fn, node->args);
      HLIRVal *dst = hlir_new_val(fn, pointer_to(ty_void));
      HLIRInsn *insn = hlir_new_insn(HLIR_ALLOCA);
      insn->dst = dst;
      insn->src1 = sz;
      insn->ty = pointer_to(ty_void);
      hlir_append_insn(fn, insn);
      return dst;
    }

    ABI *callee_abi = get_node_abi(node);

    int num_args = 0;
    for (Node *arg = node->args; arg; arg = arg->next)
      num_args++;

    HLIRVal **args = NULL;
    if (num_args > 0) {
      args = calloc(num_args, sizeof(HLIRVal *));
      int i = 0;
      for (Node *arg = node->args; arg; arg = arg->next) {
        HLIRVal *v = gen_expr_hlir(fn, arg);
        if (arg->ty && (arg->ty->kind == TY_STRUCT || arg->ty->kind == TY_UNION)) {
          v->is_struct_val = true;
          v->struct_size = arg->ty->size;
          v->struct_ty = arg->ty;
        }
        args[i++] = v;
      }
    }

    HLIRVal *callee = NULL;
    char *label = NULL;
    if (node->lhs->kind == ND_VAR && (node->lhs->var->is_function || (node->lhs->var->ty && node->lhs->var->ty->kind == TY_FUNC))) {
      label = node->lhs->var->name;
    } else {
      callee = gen_expr_hlir(fn, node->lhs);
    }

    HLIRVal *dst = (node->ty->kind != TY_VOID) ? hlir_new_val(fn, node->ty) : NULL;
    HLIRInsn *call = hlir_new_insn(HLIR_CALL);
    call->dst = dst;
    call->src1 = callee;
    call->label = label;
    call->num_args = num_args;
    call->args = args;
    call->call_abi = callee_abi;
    call->ty = node->ty;
    call->var = node->ret_buffer;
    hlir_append_insn(fn, call);

    if (node->ret_buffer) {
      HLIRVal *ret_buf_addr = hlir_new_val(fn, pointer_to(node->ret_buffer->ty));
      HLIRInsn *addr_insn = hlir_new_insn(HLIR_ADDR_VAR);
      addr_insn->dst = ret_buf_addr;
      addr_insn->var = node->ret_buffer;
      hlir_append_insn(fn, addr_insn);
      return ret_buf_addr;
    }

    return dst;
  }
  case ND_CAS: {
    HLIRVal *old_val = gen_expr_hlir(fn, node->cas_old);
    HLIRVal *new_val = gen_expr_hlir(fn, node->cas_new);
    HLIRVal *addr = gen_expr_hlir(fn, node->cas_addr);

    HLIRVal *dst = hlir_new_val(fn, ty_bool);
    HLIRInsn *insn = hlir_new_insn(HLIR_CAS);
    insn->dst = dst;
    insn->src1 = addr;
    insn->src2 = old_val;
    insn->src3 = new_val;
    insn->ty = node->cas_addr->ty->base;
    hlir_append_insn(fn, insn);
    return dst;
  }
  case ND_EXCH: {
    HLIRVal *val = gen_expr_hlir(fn, node->rhs);
    HLIRVal *addr = gen_expr_hlir(fn, node->lhs);

    HLIRVal *dst = hlir_new_val(fn, node->lhs->ty->base);
    HLIRInsn *insn = hlir_new_insn(HLIR_EXCH);
    insn->dst = dst;
    insn->src1 = addr;
    insn->src2 = val;
    insn->ty = node->lhs->ty->base;
    hlir_append_insn(fn, insn);
    return dst;
  }
  case ND_MEMZERO: {
    HLIRVal *addr = hlir_new_val(fn, pointer_to(node->var->ty));
    HLIRInsn *addr_insn = hlir_new_insn(HLIR_ADDR_VAR);
    addr_insn->dst = addr;
    addr_insn->var = node->var;
    hlir_append_insn(fn, addr_insn);

    HLIRInsn *zero_insn = hlir_new_insn(HLIR_MEMZERO);
    zero_insn->src1 = addr;
    zero_insn->imm = node->var->ty->size;
    hlir_append_insn(fn, zero_insn);
    return NULL;
  }
  case ND_ASM:
    gen_asm_hlir(fn, node);
    return NULL;
  default:
    error_tok(node->tok, "invalid expression node in HLIR generator");
  }
  return NULL;
}

static void gen_asm_hlir(HLIRFunction *fn, Node *node) {
  HLIRInsn *insn = hlir_new_insn(HLIR_ASM);
  insn->asm_str = node->asm_str;
  insn->asm_is_volatile = node->asm_is_volatile;
  insn->asm_is_goto = node->asm_is_goto;
  insn->asm_clobbers = node->asm_clobbers;
  insn->asm_labels = node->asm_labels;

  AsmOperand *out_head = NULL, *out_tail = NULL;
  for (AsmOperand *src_op = node->asm_outputs; src_op; src_op = src_op->next) {
    AsmOperand *op = calloc(1, sizeof(AsmOperand));
    *op = *src_op;
    op->next = NULL;

    bool is_rw = (strchr(op->constraint, '+') != NULL);
    bool is_mem = (strchr(op->constraint, 'm') != NULL || strchr(op->constraint, 'o') != NULL || strchr(op->constraint, 'v') != NULL) &&
                  (strchr(op->constraint, 'r') == NULL && strchr(op->constraint, 'g') == NULL &&
                   strchr(op->constraint, 'a') == NULL && strchr(op->constraint, 'b') == NULL &&
                   strchr(op->constraint, 'c') == NULL && strchr(op->constraint, 'd') == NULL &&
                   strchr(op->constraint, 'S') == NULL && strchr(op->constraint, 'D') == NULL &&
                   strchr(op->constraint, 'q') == NULL && strchr(op->constraint, 'Q') == NULL);

    if (op->expr->kind == ND_VAR) {
      op->var = op->expr->var;
    } else {
      op->addr_vreg = (void *)gen_addr_hlir(fn, op->expr);
    }

    if (!is_mem) {
      if (is_rw) {
        op->vreg = (void *)gen_expr_hlir(fn, op->expr);
      } else {
        op->vreg = (void *)hlir_new_val(fn, op->expr->ty);
      }
    }

    if (!out_head) out_head = out_tail = op;
    else out_tail = out_tail->next = op;
  }
  insn->asm_outputs = out_head;

  AsmOperand *in_head = NULL, *in_tail = NULL;
  for (AsmOperand *src_op = node->asm_inputs; src_op; src_op = src_op->next) {
    AsmOperand *op = calloc(1, sizeof(AsmOperand));
    *op = *src_op;
    op->next = NULL;

    bool is_mem = (strchr(op->constraint, 'm') != NULL || strchr(op->constraint, 'o') != NULL || strchr(op->constraint, 'v') != NULL) &&
                  (strchr(op->constraint, 'r') == NULL && strchr(op->constraint, 'g') == NULL &&
                   strchr(op->constraint, 'a') == NULL && strchr(op->constraint, 'b') == NULL &&
                   strchr(op->constraint, 'c') == NULL && strchr(op->constraint, 'd') == NULL &&
                   strchr(op->constraint, 'S') == NULL && strchr(op->constraint, 'D') == NULL &&
                   strchr(op->constraint, 'q') == NULL && strchr(op->constraint, 'Q') == NULL);
    bool is_imm = (strchr(op->constraint, 'i') != NULL || strchr(op->constraint, 'n') != NULL);

    if (is_imm && op->expr->kind == ND_NUM) {
      op->is_imm_val = true;
      op->imm_val = op->expr->val;
    } else if (is_mem) {
      if (op->expr->kind == ND_VAR)
        op->var = op->expr->var;
      else
        op->addr_vreg = (void *)gen_addr_hlir(fn, op->expr);
    } else {
      if (op->expr->kind == ND_NUM) {
        op->is_imm_val = true;
        op->imm_val = op->expr->val;
      }
      op->vreg = (void *)gen_expr_hlir(fn, op->expr);
    }

    if (!in_head) in_head = in_tail = op;
    else in_tail = in_tail->next = op;
  }
  insn->asm_inputs = in_head;

  hlir_append_insn(fn, insn);

  for (AsmOperand *op = insn->asm_outputs; op; op = op->next) {
    if (!op->vreg)
      continue;
    HLIRVal *v = (HLIRVal *)op->vreg;
    if (op->var) {
      HLIRVal *addr = hlir_new_val(fn, pointer_to(op->var->ty));
      HLIRInsn *addr_insn = hlir_new_insn(HLIR_ADDR_VAR);
      addr_insn->dst = addr;
      addr_insn->var = op->var;
      hlir_append_insn(fn, addr_insn);

      HLIRInsn *store = hlir_new_insn(HLIR_STORE_PTR);
      store->src1 = addr;
      store->src2 = v;
      store->ty = op->var->ty;
      hlir_append_insn(fn, store);
    } else if (op->addr_vreg) {
      HLIRVal *addr = (HLIRVal *)op->addr_vreg;
      HLIRInsn *store = hlir_new_insn(HLIR_STORE_PTR);
      store->src1 = addr;
      store->src2 = v;
      store->ty = op->expr->ty;
      hlir_append_insn(fn, store);
    }
  }
}

static void gen_stmt_hlir(HLIRFunction *fn, Node *node) {
  if (!node)
    return;

  switch (node->kind) {
  case ND_IF: {
    static int id = 0;
    int c = id++;
    char *else_lbl = node->els ? format(".L.hlir.if.else.%d", c) : format(".L.hlir.if.end.%d", c);
    char *end_lbl = format(".L.hlir.if.end.%d", c);

    HLIRVal *cond = gen_expr_hlir(fn, node->cond);
    HLIRInsn *br = hlir_new_insn(HLIR_JMP_IF_ZERO);
    br->src1 = cond;
    br->label = else_lbl;
    hlir_append_insn(fn, br);

    gen_stmt_hlir(fn, node->then);

    if (node->els) {
      HLIRInsn *jmp = hlir_new_insn(HLIR_JMP);
      jmp->label = end_lbl;
      hlir_append_insn(fn, jmp);

      HLIRInsn *lbl_else = hlir_new_insn(HLIR_LABEL);
      lbl_else->label = else_lbl;
      hlir_append_insn(fn, lbl_else);

      gen_stmt_hlir(fn, node->els);
    }

    HLIRInsn *lbl_end = hlir_new_insn(HLIR_LABEL);
    lbl_end->label = end_lbl;
    hlir_append_insn(fn, lbl_end);
    return;
  }
  case ND_FOR: {
    static int id = 0;
    int c = id++;
    char *begin_lbl = format(".L.hlir.for.begin.%d", c);
    char *end_lbl = node->brk_label ? node->brk_label : format(".L.hlir.for.end.%d", c);
    char *cont_lbl = node->cont_label ? node->cont_label : format(".L.hlir.for.cont.%d", c);

    if (node->init)
      gen_stmt_hlir(fn, node->init);

    HLIRInsn *lbl_begin = hlir_new_insn(HLIR_LABEL);
    lbl_begin->label = begin_lbl;
    hlir_append_insn(fn, lbl_begin);

    if (node->cond) {
      HLIRVal *cond = gen_expr_hlir(fn, node->cond);
      HLIRInsn *br = hlir_new_insn(HLIR_JMP_IF_ZERO);
      br->src1 = cond;
      br->label = end_lbl;
      hlir_append_insn(fn, br);
    }

    gen_stmt_hlir(fn, node->then);

    HLIRInsn *lbl_cont = hlir_new_insn(HLIR_LABEL);
    lbl_cont->label = cont_lbl;
    hlir_append_insn(fn, lbl_cont);

    if (node->inc)
      gen_expr_hlir(fn, node->inc);

    HLIRInsn *jmp = hlir_new_insn(HLIR_JMP);
    jmp->label = begin_lbl;
    hlir_append_insn(fn, jmp);

    HLIRInsn *lbl_end = hlir_new_insn(HLIR_LABEL);
    lbl_end->label = end_lbl;
    hlir_append_insn(fn, lbl_end);
    return;
  }
  case ND_DO: {
    static int id = 0;
    int c = id++;
    char *begin_lbl = format(".L.hlir.do.begin.%d", c);
    char *end_lbl = node->brk_label ? node->brk_label : format(".L.hlir.do.end.%d", c);
    char *cont_lbl = node->cont_label ? node->cont_label : format(".L.hlir.do.cont.%d", c);

    HLIRInsn *lbl_begin = hlir_new_insn(HLIR_LABEL);
    lbl_begin->label = begin_lbl;
    hlir_append_insn(fn, lbl_begin);

    gen_stmt_hlir(fn, node->then);

    HLIRInsn *lbl_cont = hlir_new_insn(HLIR_LABEL);
    lbl_cont->label = cont_lbl;
    hlir_append_insn(fn, lbl_cont);

    HLIRVal *cond = gen_expr_hlir(fn, node->cond);
    HLIRInsn *br = hlir_new_insn(HLIR_JMP_IF_NZ);
    br->src1 = cond;
    br->label = begin_lbl;
    hlir_append_insn(fn, br);

    HLIRInsn *lbl_end = hlir_new_insn(HLIR_LABEL);
    lbl_end->label = end_lbl;
    hlir_append_insn(fn, lbl_end);
    return;
  }
  case ND_SWITCH: {
    HLIRVal *val = gen_expr_hlir(fn, node->cond);

    for (Node *n = node->case_next; n; n = n->case_next) {
      char *case_lbl = n->label;
      HLIRVal *case_val = hlir_new_val(fn, val->ty);
      HLIRInsn *imm = hlir_new_insn(HLIR_ICONST);
      imm->dst = case_val;
      imm->imm = n->begin;
      imm->ty = val->ty;
      hlir_append_insn(fn, imm);

      HLIRVal *eq = hlir_new_val(fn, ty_bool);
      HLIRInsn *cmp = hlir_new_insn(HLIR_CMP_EQ);
      cmp->dst = eq;
      cmp->src1 = val;
      cmp->src2 = case_val;
      cmp->ty = val->ty;
      hlir_append_insn(fn, cmp);

      HLIRInsn *br = hlir_new_insn(HLIR_JMP_IF_NZ);
      br->src1 = eq;
      br->label = case_lbl;
      hlir_append_insn(fn, br);
    }

    if (node->default_case) {
      HLIRInsn *jmp = hlir_new_insn(HLIR_JMP);
      jmp->label = node->default_case->label;
      hlir_append_insn(fn, jmp);
    } else {
      HLIRInsn *jmp = hlir_new_insn(HLIR_JMP);
      jmp->label = node->brk_label;
      hlir_append_insn(fn, jmp);
    }

    gen_stmt_hlir(fn, node->then);

    HLIRInsn *lbl_end = hlir_new_insn(HLIR_LABEL);
    lbl_end->label = node->brk_label;
    hlir_append_insn(fn, lbl_end);
    return;
  }
  case ND_CASE: {
    HLIRInsn *lbl = hlir_new_insn(HLIR_LABEL);
    lbl->label = node->label;
    hlir_append_insn(fn, lbl);
    gen_stmt_hlir(fn, node->lhs);
    return;
  }
  case ND_BLOCK:
    for (Node *n = node->body; n; n = n->next)
      gen_stmt_hlir(fn, n);
    return;
  case ND_NULL_EXPR:
    return;
  case ND_GOTO: {
    HLIRInsn *jmp = hlir_new_insn(HLIR_JMP);
    jmp->label = node->unique_label;
    hlir_append_insn(fn, jmp);
    return;
  }
  case ND_GOTO_EXPR: {
    HLIRVal *addr = gen_expr_hlir(fn, node->lhs);
    HLIRInsn *jmp = hlir_new_insn(HLIR_JMP);
    jmp->src1 = addr;
    hlir_append_insn(fn, jmp);
    return;
  }
  case ND_LABEL: {
    HLIRInsn *lbl = hlir_new_insn(HLIR_LABEL);
    lbl->label = node->unique_label;
    hlir_append_insn(fn, lbl);
    gen_stmt_hlir(fn, node->lhs);
    return;
  }
  case ND_RETURN: {
    HLIRVal *ret_val = node->lhs ? gen_expr_hlir(fn, node->lhs) : NULL;
    HLIRInsn *ret = hlir_new_insn(HLIR_RET);
    ret->src1 = ret_val;
    ret->ty = node->lhs ? node->lhs->ty : ty_void;
    hlir_append_insn(fn, ret);
    return;
  }
  case ND_EXPR_STMT:
    gen_expr_hlir(fn, node->lhs);
    return;
  case ND_ASM:
    gen_asm_hlir(fn, node);
    return;
  default:
    error_tok(node->tok, "invalid statement node in HLIR generator");
  }
}

HLIRProg *ast_to_hlir(Obj2 *prog) {
  HLIRProg *hlir_prog = calloc(1, sizeof(HLIRProg));
  hlir_prog->globals = prog;

  int fn_count = 0;
  for (const Obj2 *fn = prog; fn; fn = fn->next)
    if (fn->is_function && fn->is_definition && fn->is_live)
      fn_count++;

  hlir_prog->fns = calloc(fn_count, sizeof(HLIRFunction *));
  hlir_prog->num_fns = fn_count;

  int idx = 0;
  for (Obj2 *fn = prog; fn; fn = fn->next) {
    if (!fn->is_function || !fn->is_definition || !fn->is_live)
      continue;

    HLIRFunction *hlir_fn = calloc(1, sizeof(HLIRFunction));
    hlir_fn->fn_obj = fn;
    hlir_fn->name = fn->name;
    hlir_fn->func_ty = fn->ty;
    hlir_fn->abi = get_fn_abi(fn);
    hlir_fn->locals = fn->locals;
    hlir_fn->params = fn->params;
    hlir_fn->stack_size = fn->stack_size;

    gen_stmt_hlir(hlir_fn, fn->body);
    hlir_prog->fns[idx++] = hlir_fn;
  }

  return hlir_prog;
}
