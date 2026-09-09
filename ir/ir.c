#include "ir/ir.h"
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
  case ND_VAR: {
    IRVReg *dst = ir_new_vreg(fn, pointer_to(node->var->ty));
    IRInsn *insn = ir_new_insn(IR_ADDR);
    insn->dst = dst;
    insn->var = node->var;
    ir_append_insn(fn, insn);
    return dst;
  }
  case ND_DEREF:
    return gen_expr_ir(fn, node->lhs);
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
  case ND_VLA_PTR: {
    IRVReg *dst = ir_new_vreg(fn, pointer_to(node->var->ty));
    IRInsn *insn = ir_new_insn(IR_ADDR);
    insn->dst = dst;
    insn->var = node->var;
    ir_append_insn(fn, insn);

    IRVReg *deref = ir_new_vreg(fn, node->var->ty);
    IRInsn *load_insn = ir_new_insn(IR_LOAD);
    load_insn->dst = deref;
    load_insn->src1 = dst;
    load_insn->ty = node->var->ty;
    ir_append_insn(fn, load_insn);
    return deref;
  }
  default:
    error_tok(node->tok, "not an lvalue");
  }
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
  case ND_VAR:
  case ND_MEMBER: {
    IRVReg *addr = gen_addr_ir(fn, node);
    if (node->ty->kind == TY_ARRAY || node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION)
      return addr;
    IRVReg *dst = ir_new_vreg(fn, node->ty);
    IRInsn *insn = ir_new_insn(IR_LOAD);
    insn->dst = dst;
    insn->src1 = addr;
    insn->ty = node->ty;
    ir_append_insn(fn, insn);
    return dst;
  }
  case ND_DEREF: {
    IRVReg *addr = gen_expr_ir(fn, node->lhs);
    if (node->ty->kind == TY_ARRAY || node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION)
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
  case ND_ASSIGN: {
    if (node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION) {
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
    ABI *callee_abi = get_node_abi(node);

    // Count and evaluate arguments
    int argc = 0;
    for (Node *arg = node->args; arg; arg = arg->next)
      argc++;

    IRVReg **arg_vregs = calloc(argc, sizeof(IRVReg *));
    int i = 0;
    for (Node *arg = node->args; arg; arg = arg->next)
      arg_vregs[i++] = gen_expr_ir(fn, arg);

    IRVReg *callee = gen_expr_ir(fn, node->lhs);

    IRVReg *dst = (node->ty->kind != TY_VOID) ? ir_new_vreg(fn, node->ty) : NULL;
    IRInsn *call = ir_new_insn(IR_CALL);
    call->dst = dst;
    call->src1 = callee;
    call->num_args = argc;
    call->args = arg_vregs;
    call->call_abi = callee_abi;
    call->ty = node->ty;
    ir_append_insn(fn, call);

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
  case ND_GOTO: {
    IRInsn *jmp = ir_new_insn(IR_JMP);
    jmp->label = node->unique_label;
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
  for (Obj *fn = prog; fn; fn = fn->next)
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

void ir_dump(FILE *out, IRProg *prog) {
  if (!out || !prog)
    return;

  for (int i = 0; i < prog->num_fns; i++) {
    IRFunction *fn = prog->fns[i];
    fprintf(out, "function %s() [vregs: %d, insns: %d]:\n", fn->name, fn->num_vregs, fn->num_insns);
    for (IRInsn *insn = fn->head; insn; insn = insn->next) {
      fprintf(out, "  %4d: ", insn->pos);
      switch (insn->kind) {
      case IR_IMM:
        fprintf(out, "v%d = %lld\n", insn->dst->id, (long long)insn->imm);
        break;
      case IR_FIMM:
        fprintf(out, "v%d = %f\n", insn->dst->id, insn->fimm);
        break;
      case IR_ADDR:
        fprintf(out, "v%d = &%s\n", insn->dst->id, insn->var ? insn->var->name : "(anon)");
        break;
      case IR_LOAD:
        fprintf(out, "v%d = *v%d\n", insn->dst->id, insn->src1->id);
        break;
      case IR_STORE:
        fprintf(out, "*v%d = v%d\n", insn->src1->id, insn->src2->id);
        break;
      case IR_MOV:
        fprintf(out, "v%d = v%d\n", insn->dst->id, insn->src1->id);
        break;
      case IR_ADD:
        fprintf(out, "v%d = v%d + v%d\n", insn->dst->id, insn->src1->id, insn->src2->id);
        break;
      case IR_SUB:
        fprintf(out, "v%d = v%d - v%d\n", insn->dst->id, insn->src1->id, insn->src2->id);
        break;
      case IR_MUL:
        fprintf(out, "v%d = v%d * v%d\n", insn->dst->id, insn->src1->id, insn->src2->id);
        break;
      case IR_DIV:
        fprintf(out, "v%d = v%d / v%d\n", insn->dst->id, insn->src1->id, insn->src2->id);
        break;
      case IR_LABEL:
        fprintf(out, "%s:\n", insn->label);
        break;
      case IR_JMP:
        fprintf(out, "jmp %s\n", insn->label);
        break;
      case IR_BR:
        fprintf(out, "br v%d, %s, %s\n", insn->src1->id, insn->label_true ? insn->label_true : "", insn->label_false ? insn->label_false : "");
        break;
      case IR_RET:
        if (insn->src1)
          fprintf(out, "ret v%d\n", insn->src1->id);
        else
          fprintf(out, "ret\n");
        break;
      case IR_CALL:
        if (insn->dst)
          fprintf(out, "v%d = call v%d(%d args)\n", insn->dst->id, insn->src1->id, insn->num_args);
        else
          fprintf(out, "call v%d(%d args)\n", insn->src1->id, insn->num_args);
        break;
      default:
        fprintf(out, "insn kind %d\n", insn->kind);
        break;
      }
    }
    fprintf(out, "\n");
  }
}
