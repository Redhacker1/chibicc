#ifndef CHIBICC_OBJFMT_H
#define CHIBICC_OBJFMT_H

#include <stdio.h>

typedef struct Obj2 Obj2;
typedef Obj2 Obj;
typedef struct ObjFmt ObjFmt;

struct ObjFmt {
  const char *name;
  const char *description;

  void (*emit_var_decl)(Obj2 *var, FILE *out);
  void (*emit_var_type_size)(Obj2 *var, FILE *out);
  void (*emit_fn_decl)(Obj2 *fn, FILE *out);
  void (*emit_fn_type)(Obj2 *fn, FILE *out);
};

extern ObjFmt *current_objfmt;

extern ObjFmt objfmt_elf;
extern ObjFmt objfmt_coff;
extern ObjFmt objfmt_macho;
extern ObjFmt objfmt_flat;

void set_objfmt(const char *name);
ObjFmt *get_objfmt(const char *name);
void init_objfmts(void);

#endif // CHIBICC_OBJFMT_H
