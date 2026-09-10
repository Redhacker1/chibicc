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
