#include "chibicc.h"
#include "abi/abi.h"

ABI *current_abi = NULL;

static ABI *abis[32];
static int abi_count = 0;

void register_abi(ABI *abi) {
  for (int i = 0; i < abi_count; i++) {
    if (!strcmp(abis[i]->name, abi->name)) {
      abis[i] = abi;
      return;
    }
  }
  if (abi_count < 32)
    abis[abi_count++] = abi;
}

ABI *get_abi(const char *name) {
  if (!name)
    return NULL;
  for (int i = 0; i < abi_count; i++) {
    if (abis[i] && abis[i]->name && !strcmp(abis[i]->name, name))
      return abis[i];
  }
  return NULL;
}

void set_abi(const char *name) {
  if (!name)
    return;
  ABI *abi = get_abi(name);
  if (!abi)
    error("unknown ABI: %s", (char *)name);
  current_abi = abi;
  if (abi->default_objfmt)
    current_objfmt = abi->default_objfmt;
  if (abi->init_types)
    abi->init_types();
}

void init_abis(void) {
  init_objfmts();
  abi_count = 0;
  register_abi(&abi_sysv64);
  register_abi(&abi_win64);
  register_abi(&abi_win32);
  register_abi(&abi_sys6);
  register_abi(&abi_pascal);
  register_abi(&abi_z80);

  // Default ABI is SysV x86_64
  set_abi("sysv64");
}

void declare_abi_builtin_types(void) {
  for (int i = 0; i < abi_count; i++) {
    if (abis[i] && abis[i] != current_abi && abis[i]->declare_builtin_types)
      abis[i]->declare_builtin_types();
  }
  if (current_abi && current_abi->declare_builtin_types)
    current_abi->declare_builtin_types();
  else
    push_scope("__builtin_va_list")->type_def = pointer_to(ty_char);
}

const char *abi_x86_reg32(const char *r64) {
  if (!r64) return r64;
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

const char *abi_x86_reg16(const char *r64) {
  if (!r64) return r64;
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

const char *abi_x86_reg8(const char *r64) {
  if (!r64) return r64;
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

Node *abi_va_start_ptr(Node *ap, Node *last, Token *tok) {
  VarScope *sc = find_var(&(Token){.loc = "__va_area__", .len = 11});
  if (!sc || !sc->var)
    error_tok(tok, "__builtin_va_start used outside variadic function");
  Node *va_var = new_var_node(sc->var, tok);
  add_type(ap);
  if (ap->ty->kind == TY_PTR && (ap->ty->base->kind != TY_STRUCT && ap->ty->base->kind != TY_UNION)) {
    Node *addr = new_unary(ND_ADDR, va_var, tok);
    Node *val = new_unary(ND_DEREF, new_cast(addr, pointer_to(ap->ty)), tok);
    return new_binary(ND_ASSIGN, ap, val, tok);
  } else {
    Node *va_addr = new_unary(ND_ADDR, va_var, tok);
    Node *val = new_unary(ND_DEREF, new_cast(va_addr, pointer_to(pointer_to(ty_void))), tok);
    Node *of_addr = new_add(new_cast(ap, pointer_to(ty_char)), new_num(8, tok), tok);
    Node *of_ptr = new_cast(of_addr, pointer_to(pointer_to(ty_void)));
    return new_binary(ND_ASSIGN, new_unary(ND_DEREF, of_ptr, tok), val, tok);
  }
}

Node *abi_va_arg_ptr(Node *ap, Type *ty, int slot_size, Token *tok) {
  add_type(ap);
  bool is_ptr = (ap->ty->kind == TY_PTR && (ap->ty->base->kind != TY_STRUCT && ap->ty->base->kind != TY_UNION));

  Obj *old_p = new_lvar("", pointer_to(ty_void));
  Node head = {};
  Node *cur = &head;

  if (is_ptr) {
    cur = cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, new_var_node(old_p, tok), new_cast(ap, pointer_to(ty_void)), tok), tok);
    cur = cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, ap, new_cast(new_add(new_cast(ap, pointer_to(ty_char)), new_num(slot_size, tok), tok), ap->ty), tok), tok);
  } else {
    Node *of_addr = new_add(new_cast(ap, pointer_to(ty_char)), new_num(8, tok), tok);
    Node *of_ptr = new_cast(of_addr, pointer_to(pointer_to(ty_void)));
    Node *load_of = new_unary(ND_DEREF, of_ptr, tok);
    cur = cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, new_var_node(old_p, tok), load_of, tok), tok);
    Node *new_of = new_add(new_cast(new_var_node(old_p, tok), pointer_to(ty_char)), new_num(slot_size, tok), tok);
    cur = cur->next = new_unary(ND_EXPR_STMT,
      new_binary(ND_ASSIGN, new_unary(ND_DEREF, of_ptr, tok), new_cast(new_of, pointer_to(ty_void)), tok), tok);
  }

  cur = cur->next = new_unary(ND_EXPR_STMT,
    new_unary(ND_DEREF, new_cast(new_var_node(old_p, tok), pointer_to(ty)), tok), tok);

  Node *node = new_node(ND_STMT_EXPR, tok);
  node->body = head.next;
  return node;
}

Node *abi_va_copy_ptr(Node *dest, Node *src, Token *tok) {
  add_type(dest);
  if (dest->ty->kind == TY_PTR && (dest->ty->base->kind != TY_STRUCT && dest->ty->base->kind != TY_UNION)) {
    return new_binary(ND_ASSIGN, dest, src, tok);
  } else {
    Node *deref_dest = new_unary(ND_DEREF, dest, tok);
    Node *deref_src = new_unary(ND_DEREF, src, tok);
    return new_binary(ND_ASSIGN, deref_dest, deref_src, tok);
  }
}

Node *abi_va_end_nop(Node *ap, Token *tok) {
  Node *node = new_node(ND_NULL_EXPR, tok);
  node->ty = ty_void;
  return node;
}
