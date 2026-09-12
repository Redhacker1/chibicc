#include "ir/hlir.h"
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

static HLIRVal *gen_addr_hlir(HLIRFunction *fn, Node *node) {
  switch (node->kind) {
  case ND_VAR: {
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
  default:
    break;
  }
  HLIRVal *dst = hlir_new_val(fn, pointer_to(node->ty));
  HLIRInsn *insn = hlir_new_insn(HLIR_ADDR_VAR);
  insn->dst = dst;
  hlir_append_insn(fn, insn);
  return dst;
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
  case ND_VAR: {
    if (node->var->ty->kind == TY_ARRAY || node->var->ty->kind == TY_STRUCT || node->var->ty->kind == TY_UNION || node->var->ty->kind == TY_FUNC)
      return gen_addr_hlir(fn, node);
    HLIRVal *dst = hlir_new_val(fn, node->ty);
    HLIRInsn *insn = hlir_new_insn(HLIR_LOAD_VAR);
    insn->dst = dst;
    insn->var = node->var;
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
  case ND_DEREF: {
    HLIRVal *addr = gen_expr_hlir(fn, node->lhs);
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
    HLIRVal *base = gen_addr_hlir(fn, node->lhs);
    HLIRVal *dst = hlir_new_val(fn, node->ty);
    HLIRInsn *insn = hlir_new_insn(HLIR_LOAD_MEMBER);
    insn->dst = dst;
    insn->src1 = base;
    insn->imm = node->member->offset;
    insn->ty = node->ty;
    hlir_append_insn(fn, insn);
    return dst;
  }
  case ND_ASSIGN: {
    HLIRVal *val = gen_expr_hlir(fn, node->rhs);
    if (node->lhs->kind == ND_VAR) {
      HLIRInsn *insn = hlir_new_insn(HLIR_STORE_VAR);
      insn->var = node->lhs->var;
      insn->src1 = val;
      insn->ty = node->lhs->ty;
      hlir_append_insn(fn, insn);
    } else if (node->lhs->kind == ND_MEMBER) {
      HLIRVal *base = gen_addr_hlir(fn, node->lhs->lhs);
      HLIRInsn *insn = hlir_new_insn(HLIR_STORE_MEMBER);
      insn->src1 = base;
      insn->src2 = val;
      insn->imm = node->lhs->member->offset;
      insn->ty = node->lhs->ty;
      hlir_append_insn(fn, insn);
    } else {
      HLIRVal *addr = gen_addr_hlir(fn, node->lhs);
      HLIRInsn *insn = hlir_new_insn(HLIR_STORE_PTR);
      insn->src1 = addr;
      insn->src2 = val;
      insn->ty = node->lhs->ty;
      hlir_append_insn(fn, insn);
    }
    return val;
  }
  case ND_CAST: {
    HLIRVal *src = gen_expr_hlir(fn, node->lhs);
    HLIRVal *dst = hlir_new_val(fn, node->ty);
    HLIRInsn *insn = hlir_new_insn(HLIR_CAST);
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
    HLIRKind k = HLIR_ADD;
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
  case ND_FUNCALL: {
    HLIRVal **args = NULL;
    int num_args = 0;
    for (Node *arg = node->args; arg; arg = arg->next)
      num_args++;
    if (num_args > 0) {
      args = calloc(num_args, sizeof(HLIRVal *));
      int i = 0;
      for (Node *arg = node->args; arg; arg = arg->next)
        args[i++] = gen_expr_hlir(fn, arg);
    }
    HLIRVal *callee = gen_expr_hlir(fn, node->lhs);
    HLIRVal *dst = node->ty->kind == TY_VOID ? NULL : hlir_new_val(fn, node->ty);
    HLIRInsn *insn = hlir_new_insn(HLIR_CALL);
    insn->dst = dst;
    insn->src1 = callee;
    insn->num_args = num_args;
    insn->args = args;
    insn->ty = node->ty;
    hlir_append_insn(fn, insn);
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
  default:
    break;
  }
  return NULL;
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
  case ND_BLOCK:
    for (Node *n = node->body; n; n = n->next)
      gen_stmt_hlir(fn, n);
    return;
  case ND_GOTO: {
    HLIRInsn *jmp = hlir_new_insn(HLIR_JMP);
    jmp->label = node->unique_label;
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
  case ND_ASM: {
    HLIRInsn *insn = hlir_new_insn(HLIR_ASM);
    insn->asm_str = node->asm_str;
    hlir_append_insn(fn, insn);
    return;
  }
  default:
    break;
  }
}

HLIRProg *ast_to_hlir(Obj *prog) {
  HLIRProg *hlir_prog = calloc(1, sizeof(HLIRProg));
  hlir_prog->globals = prog;

  int fn_count = 0;
  for (const Obj *fn = prog; fn; fn = fn->next)
    if (fn->is_function && fn->is_definition && fn->is_live)
      fn_count++;

  hlir_prog->fns = calloc(fn_count, sizeof(HLIRFunction *));
  hlir_prog->num_fns = fn_count;

  int idx = 0;
  for (Obj *fn = prog; fn; fn = fn->next) {
    if (!fn->is_function || !fn->is_definition || !fn->is_live)
      continue;

    HLIRFunction *hlir_fn = calloc(1, sizeof(HLIRFunction));
    hlir_fn->fn_obj = fn;
    hlir_fn->name = fn->name;
    hlir_fn->func_ty = fn->ty;
    hlir_fn->locals = fn->locals;
    hlir_fn->params = fn->params;
    hlir_fn->stack_size = fn->stack_size;

    gen_stmt_hlir(hlir_fn, fn->body);
    hlir_prog->fns[idx++] = hlir_fn;
  }

  return hlir_prog;
}
