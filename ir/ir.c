#include "ir/ir.h"
#include "ir/opt.h"
#include <stdlib.h>
#include <string.h>

IRVReg *ir_new_vreg(IRFunction *fn, Type *ty) {
  if (fn->num_vregs >= fn->vreg_capacity) {
    fn->vreg_capacity = fn->vreg_capacity ? fn->vreg_capacity * 2 : 64;
    fn->vregs = realloc(fn->vregs, sizeof(IRVReg *) * fn->vreg_capacity);
  }

  IRVReg *v = calloc(1, sizeof(IRVReg));
  v->id = fn->num_vregs;
  v->ty = ty;
  v->phys_reg = -1;
  v->spill_offset = 0;
  v->def_pos = -1;
  v->last_use_pos = -1;
  v->is_float = ty ? is_flonum(ty) : false;
  v->is_spilled = false;
  v->is_pinned = false;

  fn->vregs[fn->num_vregs++] = v;
  return v;
}

IRInsn *ir_new_insn(IRKind kind) {
  IRInsn *insn = calloc(1, sizeof(IRInsn));
  insn->kind = kind;
  return insn;
}

void ir_append_insn(IRFunction *fn, IRInsn *insn) {
  insn->pos = fn->num_insns++;
  if (!fn->head) {
    fn->head = fn->tail = insn;
  } else {
    fn->tail->next = insn;
    insn->prev = fn->tail;
    fn->tail = insn;
  }
  if (insn->dst)
    insn->dst->def_insn = insn;
}

void ir_insert_before(IRFunction *fn, IRInsn *target, IRInsn *new_insn) {
  if (!target) {
    ir_append_insn(fn, new_insn);
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
  if (new_insn->dst)
    new_insn->dst->def_insn = new_insn;
}

void ir_insert_after(IRFunction *fn, IRInsn *target, IRInsn *new_insn) {
  if (!target) {
    ir_append_insn(fn, new_insn);
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
  if (new_insn->dst)
    new_insn->dst->def_insn = new_insn;
}

void ir_remove_insn(IRFunction *fn, IRInsn *insn) {
  if (!insn)
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

void ir_replace_insn(IRFunction *fn, IRInsn *old_insn, IRInsn *new_insn) {
  if (!old_insn || !new_insn)
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
  if (new_insn->dst)
    new_insn->dst->def_insn = new_insn;
}

void ir_renumber_insns(IRFunction *fn) {
  int pos = 0;
  int *def_count = (fn->num_vregs > 0) ? calloc(fn->num_vregs, sizeof(int)) : NULL;
  for (int i = 0; i < fn->num_vregs; i++)
    fn->vregs[i]->def_insn = NULL;

  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    insn->pos = pos++;
    if (insn->dst) {
      if (def_count)
        def_count[insn->dst->id]++;
      insn->dst->def_insn = insn;
    }
  }
  if (def_count) {
    for (int i = 0; i < fn->num_vregs; i++) {
      if (def_count[i] != 1)
        fn->vregs[i]->def_insn = NULL;
    }
    free(def_count);
  }
  fn->num_insns = pos;
}

bool ir_insn_has_side_effects(IRInsn *insn) {
  if (!insn)
    return false;
  switch (insn->kind) {
  case IR_STORE:
  case IR_MEMCPY:
  case IR_MEMZERO:
  case IR_CALL:
  case IR_RET:
  case IR_BR:
  case IR_JMP:
  case IR_LABEL:
  case IR_ASM:
  case IR_ALLOCA:
  case IR_CAS:
  case IR_EXCH:
    return true;
  default:
    return false;
  }
}

bool ir_insn_is_terminator(IRInsn *insn) {
  if (!insn)
    return false;
  return insn->kind == IR_JMP || insn->kind == IR_BR || insn->kind == IR_RET;
}

bool ir_insn_is_branch(IRInsn *insn) {
  if (!insn)
    return false;
  return insn->kind == IR_JMP || insn->kind == IR_BR;
}

bool ir_insn_is_commutative(IRKind kind) {
  switch (kind) {
  case IR_ADD:
  case IR_MUL:
  case IR_BITAND:
  case IR_BITOR:
  case IR_BITXOR:
  case IR_EQ:
  case IR_NE:
    return true;
  default:
    return false;
  }
}

IRFunction *ir_new_function(Obj *fn_obj) {
  IRFunction *fn = calloc(1, sizeof(IRFunction));
  fn->fn_obj = fn_obj;
  fn->name = fn_obj->name;
  fn->func_ty = fn_obj->ty;
  fn->abi = get_fn_abi(fn_obj);
  fn->locals = fn_obj->locals;
  fn->params = fn_obj->params;
  fn->stack_size = fn_obj->stack_size;
  return fn;
}

static IRVReg *gen_expr_ir(IRFunction *fn, Node *node);
static void gen_stmt_ir(IRFunction *fn, Node *node);

static IRVReg *gen_addr_ir(IRFunction *fn, Node *node) {
  switch (node->kind) {
  case ND_VAR:
  case ND_VLA_PTR: {
    IRVReg *dst = ir_new_vreg(fn, pointer_to(node->var->ty));
    IRInsn *insn = ir_new_insn(IR_ADDR);
    insn->dst = dst;
    insn->var = node->var;
    ir_append_insn(fn, insn);
    return dst;
  }
  case ND_DEREF:
    return gen_expr_ir(fn, node->lhs);
  case ND_COMMA:
    gen_expr_ir(fn, node->lhs);
    return gen_addr_ir(fn, node->rhs);
  case ND_MEMBER: {
    IRVReg *base = gen_addr_ir(fn, node->lhs);
    if (node->member->offset == 0)
      return base;
    IRVReg *offset = ir_new_vreg(fn, ty_long);
    IRInsn *imm_insn = ir_new_insn(IR_IMM);
    imm_insn->dst = offset;
    imm_insn->imm = node->member->offset;
    ir_append_insn(fn, imm_insn);

    IRVReg *dst = ir_new_vreg(fn, pointer_to(node->ty));
    IRInsn *add_insn = ir_new_insn(IR_ADD);
    add_insn->dst = dst;
    add_insn->src1 = base;
    add_insn->src2 = offset;
    ir_append_insn(fn, add_insn);
    return dst;
  }
  case ND_FUNCALL:
    if (node->ret_buffer)
      return gen_expr_ir(fn, node);
    if (node->ty && (node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION)) {
      IRVReg *val = gen_expr_ir(fn, node);
      Obj *tmp = calloc(1, sizeof(Obj));
      tmp->name = "";
      tmp->ty = node->ty;
      tmp->is_local = true;
      tmp->align = node->ty->align;
      if (fn->fn_obj) {
        tmp->next = fn->fn_obj->locals;
        fn->fn_obj->locals = tmp;
      }

      IRVReg *tmp_addr = ir_new_vreg(fn, pointer_to(node->ty));
      IRInsn *addr_insn = ir_new_insn(IR_ADDR);
      addr_insn->dst = tmp_addr;
      addr_insn->var = tmp;
      ir_append_insn(fn, addr_insn);

      IRInsn *store = ir_new_insn(IR_STORE);
      store->src1 = tmp_addr;
      store->src2 = val;
      store->ty = node->ty;
      ir_append_insn(fn, store);
      return tmp_addr;
    }
    break;
  case ND_STMT_EXPR:
    for (Node *n = node->body; n; n = n->next) {
      if (!n->next && n->kind == ND_EXPR_STMT)
        return gen_addr_ir(fn, n->lhs);
      gen_stmt_ir(fn, n);
    }
    return NULL;
  case ND_ASSIGN:
  case ND_COND:
    if (node->ty && (node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION))
      return gen_expr_ir(fn, node);
    break;
  default:
    break;
  }
  error_tok(node->tok, "not an lvalue");
  return NULL;
}

static IRVReg *gen_expr_ir(IRFunction *fn, Node *node) {
  if (!node)
    return NULL;

  switch (node->kind) {
  case ND_NULL_EXPR:
    return NULL;
  case ND_NUM: {
    IRVReg *dst = ir_new_vreg(fn, node->ty);
    if (is_flonum(node->ty)) {
      IRInsn *insn = ir_new_insn(IR_FIMM);
      insn->dst = dst;
      insn->fimm = node->fval;
      insn->ty = node->ty;
      ir_append_insn(fn, insn);
    } else {
      IRInsn *insn = ir_new_insn(IR_IMM);
      insn->dst = dst;
      insn->imm = node->val;
      insn->ty = node->ty;
      ir_append_insn(fn, insn);
    }
    return dst;
  }
  case ND_VLA_PTR: {
    IRVReg *addr = gen_addr_ir(fn, node);
    IRVReg *dst = ir_new_vreg(fn, pointer_to(node->var->ty));
    IRInsn *insn = ir_new_insn(IR_LOAD);
    insn->dst = dst;
    insn->src1 = addr;
    insn->ty = pointer_to(node->var->ty);
    ir_append_insn(fn, insn);
    return dst;
  }
  case ND_VAR: {
    if (node->var->ty->kind == TY_VLA) {
      IRVReg *addr = gen_addr_ir(fn, node);
      IRVReg *dst = ir_new_vreg(fn, pointer_to(node->var->ty->base));
      IRInsn *insn = ir_new_insn(IR_LOAD);
      insn->dst = dst;
      insn->src1 = addr;
      insn->ty = pointer_to(node->var->ty->base);
      ir_append_insn(fn, insn);
      return dst;
    }
    IRVReg *addr = gen_addr_ir(fn, node);
    if (node->ty->kind == TY_ARRAY || node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION || node->ty->kind == TY_FUNC)
      return addr;
    IRVReg *dst = ir_new_vreg(fn, node->ty);
    IRInsn *insn = ir_new_insn(IR_LOAD);
    insn->dst = dst;
    insn->src1 = addr;
    insn->ty = node->ty;
    ir_append_insn(fn, insn);
    return dst;
  }
  case ND_MEMBER: {
    IRVReg *addr = gen_addr_ir(fn, node);
    if (node->ty->kind == TY_ARRAY || node->ty->kind == TY_VLA || node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION || node->ty->kind == TY_FUNC)
      return addr;
    IRVReg *dst = ir_new_vreg(fn, node->ty);
    IRInsn *insn = ir_new_insn(IR_LOAD);
    insn->dst = dst;
    insn->src1 = addr;
    insn->ty = node->ty;
    ir_append_insn(fn, insn);

    if (node->member && node->member->is_bitfield) {
      Member *mem = node->member;
      int total_bits = (mem->ty ? mem->ty->size : 8) * 8;
      if (mem->bit_width < total_bits) {
        IRVReg *shl_dst = ir_new_vreg(fn, node->ty);
        IRVReg *shl_amt = ir_new_vreg(fn, ty_int);
        IRInsn *imm1 = ir_new_insn(IR_IMM);
        imm1->dst = shl_amt;
        imm1->imm = total_bits - mem->bit_width - mem->bit_offset;
        ir_append_insn(fn, imm1);

        IRInsn *shl = ir_new_insn(IR_SHL);
        shl->dst = shl_dst;
        shl->src1 = dst;
        shl->src2 = shl_amt;
        shl->ty = node->ty;
        ir_append_insn(fn, shl);

        IRVReg *shr_dst = ir_new_vreg(fn, node->ty);
        IRVReg *shr_amt = ir_new_vreg(fn, ty_int);
        IRInsn *imm2 = ir_new_insn(IR_IMM);
        imm2->dst = shr_amt;
        imm2->imm = total_bits - mem->bit_width;
        ir_append_insn(fn, imm2);

        IRInsn *shr = ir_new_insn(IR_SHR);
        shr->dst = shr_dst;
        shr->src1 = shl_dst;
        shr->src2 = shr_amt;
        shr->ty = node->ty;
        ir_append_insn(fn, shr);
        return shr_dst;
      }
    }
    return dst;
  }
  case ND_DEREF: {
    IRVReg *addr = gen_expr_ir(fn, node->lhs);
    if (node->ty->kind == TY_ARRAY || node->ty->kind == TY_VLA || node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION || node->ty->kind == TY_FUNC)
      return addr;
    IRVReg *dst = ir_new_vreg(fn, node->ty);
    IRInsn *insn = ir_new_insn(IR_LOAD);
    insn->dst = dst;
    insn->src1 = addr;
    insn->ty = node->ty;
    ir_append_insn(fn, insn);
    return dst;
  }
  case ND_ADDR:
    return gen_addr_ir(fn, node->lhs);
  case ND_LABEL_VAL: {
    IRVReg *dst = ir_new_vreg(fn, pointer_to(ty_void));
    IRInsn *insn = ir_new_insn(IR_ADDR);
    insn->dst = dst;
    insn->label = node->unique_label;
    ir_append_insn(fn, insn);
    return dst;
  }
  case ND_ASSIGN: {
    if (node->lhs && node->lhs->kind == ND_MEMBER && node->lhs->member && node->lhs->member->is_bitfield) {
      Member *mem = node->lhs->member;
      IRVReg *rhs = gen_expr_ir(fn, node->rhs);
      IRVReg *addr = gen_addr_ir(fn, node->lhs);

      // Load old value
      IRVReg *old_val = ir_new_vreg(fn, mem->ty);
      IRInsn *load = ir_new_insn(IR_LOAD);
      load->dst = old_val;
      load->src1 = addr;
      load->ty = mem->ty;
      ir_append_insn(fn, load);

      uint64_t value_mask = mem->bit_width >= 64 ? UINT64_MAX : ((1ULL << mem->bit_width) - 1);
      uint64_t field_mask = mem->bit_width >= 64 ? UINT64_MAX : (value_mask << mem->bit_offset);

      // Mask new value: (rhs & value_mask) << bit_offset
      IRVReg *v_mask = ir_new_vreg(fn, mem->ty);
      IRInsn *imm_vmask = ir_new_insn(IR_IMM);
      imm_vmask->dst = v_mask;
      imm_vmask->imm = value_mask;
      ir_append_insn(fn, imm_vmask);

      IRVReg *masked_rhs = ir_new_vreg(fn, mem->ty);
      IRInsn *and_rhs = ir_new_insn(IR_BITAND);
      and_rhs->dst = masked_rhs;
      and_rhs->src1 = rhs;
      and_rhs->src2 = v_mask;
      and_rhs->ty = mem->ty;
      ir_append_insn(fn, and_rhs);

      IRVReg *shifted_rhs = masked_rhs;
      if (mem->bit_offset > 0) {
        IRVReg *sh_amt = ir_new_vreg(fn, ty_int);
        IRInsn *imm_sh = ir_new_insn(IR_IMM);
        imm_sh->dst = sh_amt;
        imm_sh->imm = mem->bit_offset;
        ir_append_insn(fn, imm_sh);

        shifted_rhs = ir_new_vreg(fn, mem->ty);
        IRInsn *shl = ir_new_insn(IR_SHL);
        shl->dst = shifted_rhs;
        shl->src1 = masked_rhs;
        shl->src2 = sh_amt;
        shl->ty = mem->ty;
        ir_append_insn(fn, shl);
      }

      // Clear old field: old_val & ~field_mask
      IRVReg *f_mask = ir_new_vreg(fn, mem->ty);
      IRInsn *imm_fmask = ir_new_insn(IR_IMM);
      imm_fmask->dst = f_mask;
      imm_fmask->imm = ~field_mask;
      ir_append_insn(fn, imm_fmask);

      IRVReg *cleared_old = ir_new_vreg(fn, mem->ty);
      IRInsn *and_old = ir_new_insn(IR_BITAND);
      and_old->dst = cleared_old;
      and_old->src1 = old_val;
      and_old->src2 = f_mask;
      and_old->ty = mem->ty;
      ir_append_insn(fn, and_old);

      // Combine: cleared_old | shifted_rhs
      IRVReg *combined = ir_new_vreg(fn, mem->ty);
      IRInsn *or_insn = ir_new_insn(IR_BITOR);
      or_insn->dst = combined;
      or_insn->src1 = cleared_old;
      or_insn->src2 = shifted_rhs;
      or_insn->ty = mem->ty;
      ir_append_insn(fn, or_insn);

      // Store combined back
      IRInsn *store = ir_new_insn(IR_STORE);
      store->src1 = addr;
      store->src2 = combined;
      store->ty = mem->ty;
      ir_append_insn(fn, store);

      return rhs;
    }
    if (node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION) {
      if (node->rhs->kind == ND_FUNCALL && node->ty->size <= 16 && !node->rhs->ret_buffer) {
        IRVReg *r = gen_expr_ir(fn, node->rhs);
        IRVReg *l = gen_addr_ir(fn, node->lhs);
        IRInsn *insn = ir_new_insn(IR_STORE);
        insn->src1 = l;
        insn->src2 = r;
        insn->ty = node->ty;
        ir_append_insn(fn, insn);
        return l;
      }
      IRVReg *r = gen_expr_ir(fn, node->rhs);
      IRVReg *l = gen_addr_ir(fn, node->lhs);
      IRInsn *insn = ir_new_insn(IR_MEMCPY);
      insn->src1 = l;
      insn->src2 = r;
      insn->imm = node->ty->size;
      ir_append_insn(fn, insn);
      return l;
    }
    IRVReg *r = gen_expr_ir(fn, node->rhs);
    IRVReg *l = gen_addr_ir(fn, node->lhs);
    IRInsn *insn = ir_new_insn(IR_STORE);
    insn->src1 = l;
    insn->src2 = r;
    insn->ty = node->ty;
    ir_append_insn(fn, insn);
    return r;
  }
  case ND_CAST: {
    IRVReg *src = gen_expr_ir(fn, node->lhs);
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
    IRVReg *dst = ir_new_vreg(fn, node->ty);
    IRInsn *insn = ir_new_insn(IR_CAST);
    insn->dst = dst;
    insn->src1 = src;
    insn->ty = node->ty;
    ir_append_insn(fn, insn);
    return dst;
  }
  case ND_NEG: {
    IRVReg *src = gen_expr_ir(fn, node->lhs);
    IRVReg *dst = ir_new_vreg(fn, node->ty);
    IRInsn *insn = ir_new_insn(IR_NEG);
    insn->dst = dst;
    insn->src1 = src;
    insn->ty = node->ty;
    ir_append_insn(fn, insn);
    return dst;
  }
  case ND_BITNOT: {
    IRVReg *src = gen_expr_ir(fn, node->lhs);
    IRVReg *dst = ir_new_vreg(fn, node->ty);
    IRInsn *insn = ir_new_insn(IR_BITNOT);
    insn->dst = dst;
    insn->src1 = src;
    insn->ty = node->ty;
    ir_append_insn(fn, insn);
    return dst;
  }
  case ND_NOT: {
    IRVReg *src = gen_expr_ir(fn, node->lhs);
    IRVReg *dst = ir_new_vreg(fn, node->ty);
    IRInsn *insn = ir_new_insn(IR_LOGNOT);
    insn->dst = dst;
    insn->src1 = src;
    insn->ty = node->ty;
    ir_append_insn(fn, insn);
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
    IRVReg *l = gen_expr_ir(fn, node->lhs);
    IRVReg *r = gen_expr_ir(fn, node->rhs);
    IRVReg *dst = ir_new_vreg(fn, node->ty);

    IRKind k = IR_NOP;
    switch (node->kind) {
    case ND_ADD: k = IR_ADD; break;
    case ND_SUB: k = IR_SUB; break;
    case ND_MUL: k = IR_MUL; break;
    case ND_DIV: k = IR_DIV; break;
    case ND_MOD: k = IR_MOD; break;
    case ND_BITAND: k = IR_BITAND; break;
    case ND_BITOR: k = IR_BITOR; break;
    case ND_BITXOR: k = IR_BITXOR; break;
    case ND_SHL: k = IR_SHL; break;
    case ND_SHR: k = IR_SHR; break;
    case ND_EQ: k = IR_EQ; break;
    case ND_NE: k = IR_NE; break;
    case ND_LT: k = IR_LT; break;
    case ND_LE: k = IR_LE; break;
    default: break;
    }

    IRInsn *insn = ir_new_insn(k);
    insn->dst = dst;
    insn->src1 = l;
    insn->src2 = r;
    insn->ty = node->ty;
    ir_append_insn(fn, insn);
    return dst;
  }
  case ND_LOGAND: {
    static int id = 0;
    int c = id++;
    char *false_label = format(".L.logand.false.%d", c);
    char *end_label = format(".L.logand.end.%d", c);

    IRVReg *dst = ir_new_vreg(fn, ty_int);

    IRVReg *l = gen_expr_ir(fn, node->lhs);
    IRInsn *br1 = ir_new_insn(IR_BR);
    br1->src1 = l;
    br1->label_false = false_label;
    ir_append_insn(fn, br1);

    IRVReg *r = gen_expr_ir(fn, node->rhs);
    IRInsn *br2 = ir_new_insn(IR_BR);
    br2->src1 = r;
    br2->label_false = false_label;
    ir_append_insn(fn, br2);

    IRInsn *imm1 = ir_new_insn(IR_IMM);
    imm1->dst = dst;
    imm1->imm = 1;
    imm1->ty = ty_int;
    ir_append_insn(fn, imm1);

    IRInsn *jmp_end = ir_new_insn(IR_JMP);
    jmp_end->label = end_label;
    ir_append_insn(fn, jmp_end);

    IRInsn *lbl_false = ir_new_insn(IR_LABEL);
    lbl_false->label = false_label;
    ir_append_insn(fn, lbl_false);

    IRInsn *imm0 = ir_new_insn(IR_IMM);
    imm0->dst = dst;
    imm0->imm = 0;
    imm0->ty = ty_int;
    ir_append_insn(fn, imm0);

    IRInsn *lbl_end = ir_new_insn(IR_LABEL);
    lbl_end->label = end_label;
    ir_append_insn(fn, lbl_end);

    return dst;
  }
  case ND_LOGOR: {
    static int id = 0;
    int c = id++;
    char *true_label = format(".L.logor.true.%d", c);
    char *end_label = format(".L.logor.end.%d", c);

    IRVReg *dst = ir_new_vreg(fn, ty_int);

    IRVReg *l = gen_expr_ir(fn, node->lhs);
    IRInsn *br1 = ir_new_insn(IR_BR);
    br1->src1 = l;
    br1->label_true = true_label;
    ir_append_insn(fn, br1);

    IRVReg *r = gen_expr_ir(fn, node->rhs);
    IRInsn *br2 = ir_new_insn(IR_BR);
    br2->src1 = r;
    br2->label_true = true_label;
    ir_append_insn(fn, br2);

    IRInsn *imm0 = ir_new_insn(IR_IMM);
    imm0->dst = dst;
    imm0->imm = 0;
    imm0->ty = ty_int;
    ir_append_insn(fn, imm0);

    IRInsn *jmp_end = ir_new_insn(IR_JMP);
    jmp_end->label = end_label;
    ir_append_insn(fn, jmp_end);

    IRInsn *lbl_true = ir_new_insn(IR_LABEL);
    lbl_true->label = true_label;
    ir_append_insn(fn, lbl_true);

    IRInsn *imm1 = ir_new_insn(IR_IMM);
    imm1->dst = dst;
    imm1->imm = 1;
    imm1->ty = ty_int;
    ir_append_insn(fn, imm1);

    IRInsn *lbl_end = ir_new_insn(IR_LABEL);
    lbl_end->label = end_label;
    ir_append_insn(fn, lbl_end);

    return dst;
  }
  case ND_COND: {
    static int id = 0;
    int c = id++;
    char *else_label = format(".L.cond.else.%d", c);
    char *end_label = format(".L.cond.end.%d", c);

    IRVReg *cond = gen_expr_ir(fn, node->cond);
    IRInsn *br = ir_new_insn(IR_BR);
    br->src1 = cond;
    br->label_false = else_label;
    ir_append_insn(fn, br);

    IRVReg *dst = ir_new_vreg(fn, node->ty);
    IRVReg *then_val = gen_expr_ir(fn, node->then);
    if (then_val) {
      IRInsn *mov = ir_new_insn(IR_MOV);
      mov->dst = dst;
      mov->src1 = then_val;
      mov->ty = node->ty;
      ir_append_insn(fn, mov);
    }

    IRInsn *jmp_end = ir_new_insn(IR_JMP);
    jmp_end->label = end_label;
    ir_append_insn(fn, jmp_end);

    IRInsn *lbl_else = ir_new_insn(IR_LABEL);
    lbl_else->label = else_label;
    ir_append_insn(fn, lbl_else);

    IRVReg *els_val = gen_expr_ir(fn, node->els);
    if (els_val) {
      IRInsn *mov = ir_new_insn(IR_MOV);
      mov->dst = dst;
      mov->src1 = els_val;
      mov->ty = node->ty;
      ir_append_insn(fn, mov);
    }

    IRInsn *lbl_end = ir_new_insn(IR_LABEL);
    lbl_end->label = end_label;
    ir_append_insn(fn, lbl_end);

    return dst;
  }
  case ND_COMMA:
    gen_expr_ir(fn, node->lhs);
    return gen_expr_ir(fn, node->rhs);
  case ND_STMT_EXPR:
    for (Node *n = node->body; n; n = n->next) {
      if (!n->next && n->kind == ND_EXPR_STMT)
        return gen_expr_ir(fn, n->lhs);
      gen_stmt_ir(fn, n);
    }
    return NULL;
  case ND_FUNCALL: {
    if (node->lhs->kind == ND_VAR && node->lhs->var && !strcmp(node->lhs->var->name, "alloca")) {
      IRVReg *sz = gen_expr_ir(fn, node->args);
      IRVReg *dst = ir_new_vreg(fn, pointer_to(ty_void));
      IRInsn *insn = ir_new_insn(IR_ALLOCA);
      insn->dst = dst;
      insn->src1 = sz;
      insn->ty = pointer_to(ty_void);
      ir_append_insn(fn, insn);
      return dst;
    }

    ABI *callee_abi = get_node_abi(node);

    // Count and evaluate arguments
    int argc = 0;
    for (Node *arg = node->args; arg; arg = arg->next)
      argc++;

    IRVReg **arg_vregs = calloc(argc, sizeof(IRVReg *));
    int i = 0;
    for (Node *arg = node->args; arg; arg = arg->next) {
      IRVReg *v = gen_expr_ir(fn, arg);
      if (arg->ty && (arg->ty->kind == TY_STRUCT || arg->ty->kind == TY_UNION)) {
        v->is_struct_val = true;
        v->struct_size = arg->ty->size;
        v->struct_ty = arg->ty;
      }
      arg_vregs[i++] = v;
    }

    IRVReg *callee = NULL;
    char *label = NULL;
    if (node->lhs->kind == ND_VAR && (node->lhs->var->is_function || (node->lhs->var->ty && node->lhs->var->ty->kind == TY_FUNC))) {
      label = node->lhs->var->name;
    } else {
      callee = gen_expr_ir(fn, node->lhs);
    }

    IRVReg *dst = (node->ty->kind != TY_VOID) ? ir_new_vreg(fn, node->ty) : NULL;
    IRInsn *call = ir_new_insn(IR_CALL);
    call->dst = dst;
    call->src1 = callee;
    call->label = label;
    call->num_args = argc;
    call->args = arg_vregs;
    call->call_abi = callee_abi;
    call->ty = node->ty;
    call->var = node->ret_buffer;
    ir_append_insn(fn, call);

    if (node->ret_buffer) {
      IRVReg *ret_buf_addr = ir_new_vreg(fn, pointer_to(node->ret_buffer->ty));
      IRInsn *addr_insn = ir_new_insn(IR_ADDR);
      addr_insn->dst = ret_buf_addr;
      addr_insn->var = node->ret_buffer;
      ir_append_insn(fn, addr_insn);
      return ret_buf_addr;
    }

    return dst;
  }
  case ND_CAS: {
    IRVReg *old_val = gen_expr_ir(fn, node->cas_old);
    IRVReg *new_val = gen_expr_ir(fn, node->cas_new);
    IRVReg *addr = gen_expr_ir(fn, node->cas_addr);

    IRVReg *dst = ir_new_vreg(fn, ty_bool);
    IRInsn *insn = ir_new_insn(IR_CAS);
    insn->dst = dst;
    insn->src1 = addr;
    insn->src2 = old_val;
    insn->src3 = new_val;
    insn->ty = node->cas_addr->ty->base;
    ir_append_insn(fn, insn);
    return dst;
  }
  case ND_EXCH: {
    IRVReg *val = gen_expr_ir(fn, node->rhs);
    IRVReg *addr = gen_expr_ir(fn, node->lhs);

    IRVReg *dst = ir_new_vreg(fn, node->lhs->ty->base);
    IRInsn *insn = ir_new_insn(IR_EXCH);
    insn->dst = dst;
    insn->src1 = addr;
    insn->src2 = val;
    insn->ty = node->lhs->ty->base;
    ir_append_insn(fn, insn);
    return dst;
  }
  case ND_MEMZERO: {
    IRVReg *addr = ir_new_vreg(fn, pointer_to(node->var->ty));
    IRInsn *addr_insn = ir_new_insn(IR_ADDR);
    addr_insn->dst = addr;
    addr_insn->var = node->var;
    ir_append_insn(fn, addr_insn);

    IRInsn *zero_insn = ir_new_insn(IR_MEMZERO);
    zero_insn->src1 = addr;
    zero_insn->imm = node->var->ty->size;
    ir_append_insn(fn, zero_insn);
    return NULL;
  }
  case ND_ASM: {
    IRInsn *insn = ir_new_insn(IR_ASM);
    insn->asm_str = node->asm_str;
    ir_append_insn(fn, insn);
    return NULL;
  }
  default:
    error_tok(node->tok, "invalid expression node in IR generator");
  }
  return NULL;
}

static void gen_stmt_ir(IRFunction *fn, Node *node) {
  if (!node)
    return;

  switch (node->kind) {
  case ND_IF: {
    static int id = 0;
    int c = id++;
    char *else_label = node->els ? format(".L.if.else.%d", c) : format(".L.if.end.%d", c);
    char *end_label = format(".L.if.end.%d", c);

    IRVReg *cond = gen_expr_ir(fn, node->cond);
    IRInsn *br = ir_new_insn(IR_BR);
    br->src1 = cond;
    br->label_false = else_label;
    ir_append_insn(fn, br);

    gen_stmt_ir(fn, node->then);

    if (node->els) {
      IRInsn *jmp = ir_new_insn(IR_JMP);
      jmp->label = end_label;
      ir_append_insn(fn, jmp);

      IRInsn *lbl_else = ir_new_insn(IR_LABEL);
      lbl_else->label = else_label;
      ir_append_insn(fn, lbl_else);

      gen_stmt_ir(fn, node->els);
    }

    IRInsn *lbl_end = ir_new_insn(IR_LABEL);
    lbl_end->label = end_label;
    ir_append_insn(fn, lbl_end);
    return;
  }
  case ND_FOR: {
    static int id = 0;
    int c = id++;
    char *begin_label = format(".L.for.begin.%d", c);
    char *end_label = node->brk_label ? node->brk_label : format(".L.for.end.%d", c);
    char *cont_label = node->cont_label ? node->cont_label : format(".L.for.cont.%d", c);

    if (node->init)
      gen_stmt_ir(fn, node->init);

    IRInsn *lbl_begin = ir_new_insn(IR_LABEL);
    lbl_begin->label = begin_label;
    ir_append_insn(fn, lbl_begin);

    if (node->cond) {
      IRVReg *cond = gen_expr_ir(fn, node->cond);
      IRInsn *br = ir_new_insn(IR_BR);
      br->src1 = cond;
      br->label_false = end_label;
      ir_append_insn(fn, br);
    }

    gen_stmt_ir(fn, node->then);

    IRInsn *lbl_cont = ir_new_insn(IR_LABEL);
    lbl_cont->label = cont_label;
    ir_append_insn(fn, lbl_cont);

    if (node->inc)
      gen_expr_ir(fn, node->inc);

    IRInsn *jmp = ir_new_insn(IR_JMP);
    jmp->label = begin_label;
    ir_append_insn(fn, jmp);

    IRInsn *lbl_end = ir_new_insn(IR_LABEL);
    lbl_end->label = end_label;
    ir_append_insn(fn, lbl_end);
    return;
  }
  case ND_DO: {
    static int id = 0;
    int c = id++;
    char *begin_label = format(".L.do.begin.%d", c);
    char *end_label = node->brk_label ? node->brk_label : format(".L.do.end.%d", c);
    char *cont_label = node->cont_label ? node->cont_label : format(".L.do.cont.%d", c);

    IRInsn *lbl_begin = ir_new_insn(IR_LABEL);
    lbl_begin->label = begin_label;
    ir_append_insn(fn, lbl_begin);

    gen_stmt_ir(fn, node->then);

    IRInsn *lbl_cont = ir_new_insn(IR_LABEL);
    lbl_cont->label = cont_label;
    ir_append_insn(fn, lbl_cont);

    IRVReg *cond = gen_expr_ir(fn, node->cond);
    IRInsn *br = ir_new_insn(IR_BR);
    br->src1 = cond;
    br->label_true = begin_label;
    br->label_false = end_label;
    ir_append_insn(fn, br);

    IRInsn *lbl_end = ir_new_insn(IR_LABEL);
    lbl_end->label = end_label;
    ir_append_insn(fn, lbl_end);
    return;
  }
  case ND_SWITCH: {
    IRVReg *val = gen_expr_ir(fn, node->cond);

    for (Node *n = node->case_next; n; n = n->case_next) {
      char *case_lbl = n->label;
      IRVReg *case_val = ir_new_vreg(fn, val->ty);
      IRInsn *imm = ir_new_insn(IR_IMM);
      imm->dst = case_val;
      imm->imm = n->begin;
      imm->ty = val->ty;
      ir_append_insn(fn, imm);

      IRVReg *eq = ir_new_vreg(fn, ty_bool);
      IRInsn *cmp = ir_new_insn(IR_EQ);
      cmp->dst = eq;
      cmp->src1 = val;
      cmp->src2 = case_val;
      cmp->ty = val->ty;
      ir_append_insn(fn, cmp);

      IRInsn *br = ir_new_insn(IR_BR);
      br->src1 = eq;
      br->label_true = case_lbl;
      ir_append_insn(fn, br);
    }

    if (node->default_case) {
      IRInsn *jmp = ir_new_insn(IR_JMP);
      jmp->label = node->default_case->label;
      ir_append_insn(fn, jmp);
    } else {
      IRInsn *jmp = ir_new_insn(IR_JMP);
      jmp->label = node->brk_label;
      ir_append_insn(fn, jmp);
    }

    gen_stmt_ir(fn, node->then);

    IRInsn *lbl_end = ir_new_insn(IR_LABEL);
    lbl_end->label = node->brk_label;
    ir_append_insn(fn, lbl_end);
    return;
  }
  case ND_CASE: {
    IRInsn *lbl = ir_new_insn(IR_LABEL);
    lbl->label = node->label;
    ir_append_insn(fn, lbl);
    gen_stmt_ir(fn, node->lhs);
    return;
  }
  case ND_BLOCK:
    for (Node *n = node->body; n; n = n->next)
      gen_stmt_ir(fn, n);
    return;
  case ND_NULL_EXPR:
    return;
  case ND_GOTO: {
    IRInsn *jmp = ir_new_insn(IR_JMP);
    jmp->label = node->unique_label;
    ir_append_insn(fn, jmp);
    return;
  }
  case ND_GOTO_EXPR: {
    IRVReg *addr = gen_expr_ir(fn, node->lhs);
    IRInsn *jmp = ir_new_insn(IR_JMP);
    jmp->src1 = addr;
    ir_append_insn(fn, jmp);
    return;
  }
  case ND_LABEL: {
    IRInsn *lbl = ir_new_insn(IR_LABEL);
    lbl->label = node->unique_label;
    ir_append_insn(fn, lbl);
    gen_stmt_ir(fn, node->lhs);
    return;
  }
  case ND_RETURN: {
    IRVReg *ret_val = node->lhs ? gen_expr_ir(fn, node->lhs) : NULL;
    IRInsn *ret = ir_new_insn(IR_RET);
    ret->src1 = ret_val;
    ret->ty = node->lhs ? node->lhs->ty : ty_void;
    ir_append_insn(fn, ret);
    return;
  }
  case ND_EXPR_STMT:
    gen_expr_ir(fn, node->lhs);
    return;
  case ND_ASM: {
    IRInsn *insn = ir_new_insn(IR_ASM);
    insn->asm_str = node->asm_str;
    ir_append_insn(fn, insn);
    return;
  }
  default:
    error_tok(node->tok, "invalid statement node in IR generator");
  }
}

IRProg *ast_to_ir(Obj *prog) {
  IRProg *ir_prog = calloc(1, sizeof(IRProg));
  ir_prog->globals = prog;

  int fn_count = 0;
  for (const Obj *fn = prog; fn; fn = fn->next)
    if (fn->is_function && fn->is_definition && fn->is_live)
      fn_count++;

  ir_prog->fns = calloc(fn_count, sizeof(IRFunction *));
  ir_prog->num_fns = fn_count;

  int idx = 0;
  for (Obj *fn = prog; fn; fn = fn->next) {
    if (!fn->is_function || !fn->is_definition || !fn->is_live)
      continue;

    IRFunction *ir_fn = ir_new_function(fn);
    gen_stmt_ir(ir_fn, fn->body);
    ir_prog->fns[idx++] = ir_fn;
  }

  return ir_prog;
}

void ir_dump_function(FILE *out, IRFunction *fn) {
  if (!out || !fn)
    return;

  fprintf(out, "function %s() [vregs: %d, insns: %d]:\n", fn->name, fn->num_vregs, fn->num_insns);
  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    fprintf(out, "  %4d: ", insn->pos);
    switch (insn->kind) {
    case IR_PHI:
      fprintf(out, "v%d = phi(", insn->dst ? insn->dst->id : -1);
      for (int a = 0; a < insn->num_args; a++) {
        fprintf(out, "%sv%d", a > 0 ? ", " : "", insn->args[a] ? insn->args[a]->id : -1);
      }
      fprintf(out, ")\n");
      break;
    case IR_IMM:
      fprintf(out, "v%d = %lld\n", insn->dst ? insn->dst->id : -1, (long long)insn->imm);
      break;
    case IR_FIMM:
      fprintf(out, "v%d = %f\n", insn->dst ? insn->dst->id : -1, insn->fimm);
      break;
    case IR_ADDR:
      if (insn->imm != 0)
        fprintf(out, "v%d = &%s + %lld\n", insn->dst ? insn->dst->id : -1, insn->var ? insn->var->name : (insn->label ? insn->label : "anon"), (long long)insn->imm);
      else
        fprintf(out, "v%d = &%s\n", insn->dst ? insn->dst->id : -1, insn->var ? insn->var->name : (insn->label ? insn->label : "anon"));
      break;
    case IR_LOAD:
      fprintf(out, "v%d = *v%d\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1);
      break;
    case IR_STORE:
      fprintf(out, "*v%d = v%d\n", insn->src1 ? insn->src1->id : -1, insn->src2 ? insn->src2->id : -1);
      break;
    case IR_MOV:
      fprintf(out, "v%d = v%d\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1);
      break;
    case IR_CAST:
      fprintf(out, "v%d = cast v%d\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1);
      break;
    case IR_ADD:
      fprintf(out, "v%d = v%d + v%d\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1, insn->src2 ? insn->src2->id : -1);
      break;
    case IR_SUB:
      fprintf(out, "v%d = v%d - v%d\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1, insn->src2 ? insn->src2->id : -1);
      break;
    case IR_MUL:
      fprintf(out, "v%d = v%d * v%d\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1, insn->src2 ? insn->src2->id : -1);
      break;
    case IR_DIV:
      fprintf(out, "v%d = v%d / v%d\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1, insn->src2 ? insn->src2->id : -1);
      break;
    case IR_MOD:
      fprintf(out, "v%d = v%d %% v%d\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1, insn->src2 ? insn->src2->id : -1);
      break;
    case IR_BITAND:
      fprintf(out, "v%d = v%d & v%d\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1, insn->src2 ? insn->src2->id : -1);
      break;
    case IR_BITOR:
      fprintf(out, "v%d = v%d | v%d\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1, insn->src2 ? insn->src2->id : -1);
      break;
    case IR_BITXOR:
      fprintf(out, "v%d = v%d ^ v%d\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1, insn->src2 ? insn->src2->id : -1);
      break;
    case IR_SHL:
      fprintf(out, "v%d = v%d << v%d\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1, insn->src2 ? insn->src2->id : -1);
      break;
    case IR_SHR:
      fprintf(out, "v%d = v%d >> v%d\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1, insn->src2 ? insn->src2->id : -1);
      break;
    case IR_NEG:
      fprintf(out, "v%d = -v%d\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1);
      break;
    case IR_BITNOT:
      fprintf(out, "v%d = ~v%d\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1);
      break;
    case IR_LOGNOT:
      fprintf(out, "v%d = !v%d\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1);
      break;
    case IR_EQ:
      fprintf(out, "v%d = (v%d == v%d)\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1, insn->src2 ? insn->src2->id : -1);
      break;
    case IR_NE:
      fprintf(out, "v%d = (v%d != v%d)\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1, insn->src2 ? insn->src2->id : -1);
      break;
    case IR_LT:
      fprintf(out, "v%d = (v%d < v%d)\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1, insn->src2 ? insn->src2->id : -1);
      break;
    case IR_LE:
      fprintf(out, "v%d = (v%d <= v%d)\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1, insn->src2 ? insn->src2->id : -1);
      break;
    case IR_GT:
      fprintf(out, "v%d = (v%d > v%d)\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1, insn->src2 ? insn->src2->id : -1);
      break;
    case IR_GE:
      fprintf(out, "v%d = (v%d >= v%d)\n", insn->dst ? insn->dst->id : -1, insn->src1 ? insn->src1->id : -1, insn->src2 ? insn->src2->id : -1);
      break;
    case IR_LABEL:
      fprintf(out, "%s:\n", insn->label ? insn->label : "(anon label)");
      break;
    case IR_JMP:
      fprintf(out, "jmp %s\n", insn->label ? insn->label : "(anon label)");
      break;
    case IR_BR:
      fprintf(out, "br v%d, %s, %s\n", insn->src1 ? insn->src1->id : -1, insn->label_true ? insn->label_true : "", insn->label_false ? insn->label_false : "");
      break;
    case IR_RET:
      if (insn->src1)
        fprintf(out, "ret v%d\n", insn->src1->id);
      else
        fprintf(out, "ret\n");
      break;
    case IR_CALL:
      if (insn->dst)
        fprintf(out, "v%d = call v%d(%d args)\n", insn->dst->id, insn->src1 ? insn->src1->id : -1, insn->num_args);
      else
        fprintf(out, "call v%d(%d args)\n", insn->src1 ? insn->src1->id : -1, insn->num_args);
      break;
    default:
      fprintf(out, "insn kind %d\n", insn->kind);
      break;
    }
  }
  fprintf(out, "\n");
}

void ir_dump(FILE *out, IRProg *prog) {
  if (!out || !prog)
    return;

  for (int i = 0; i < prog->num_fns; i++)
    ir_dump_function(out, prog->fns[i]);
}

void llir_dump_function(FILE *out, LLIRFunction *fn) {
  ir_dump_function(out, fn);
}

void llir_dump(FILE *out, LLIRProg *prog) {
  ir_dump(out, prog);
}

LLIRProg *hlir_to_llir(HLIRProg *hlir) {
  if (!hlir)
    return NULL;
  return ast_to_ir(hlir->globals);
}
