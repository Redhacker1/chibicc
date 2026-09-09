#include "abi/objfmt.h"
#include "chibicc.h"
#include <string.h>

#define println_fmt(out, ...) do { \
  if (out) fprintf(out, __VA_ARGS__); \
  if (out) fprintf(out, "\n"); \
} while (0)

ObjFmt *current_objfmt = &objfmt_elf;

// ELF object format
static void elf_emit_var_decl(Obj *var, FILE *out) {
  if (var->is_static)
    println_fmt(out, "  .local %s", var->name);
  else
    println_fmt(out, "  .globl %s", var->name);
}

static void elf_emit_var_type_size(Obj *var, FILE *out) {
  println_fmt(out, "  .type %s, @object", var->name);
  println_fmt(out, "  .size %s, %d", var->name, var->ty->size);
}

static void elf_emit_fn_decl(Obj *fn, FILE *out) {
  if (fn->is_static)
    println_fmt(out, "  .local %s", fn->name);
  else
    println_fmt(out, "  .globl %s", fn->name);
}

static void elf_emit_fn_type(Obj *fn, FILE *out) {
  println_fmt(out, "  .type %s, @function", fn->name);
}

ObjFmt objfmt_elf = {
  .name = "elf",
  .description = "ELF object file directives",
  .emit_var_decl = elf_emit_var_decl,
  .emit_var_type_size = elf_emit_var_type_size,
  .emit_fn_decl = elf_emit_fn_decl,
  .emit_fn_type = elf_emit_fn_type,
};

// COFF / Windows PE object format
static void coff_emit_var_decl(Obj *var, FILE *out) {
  if (!var->is_static)
    println_fmt(out, "  .globl %s", var->name);
}

static void coff_emit_fn_decl(Obj *fn, FILE *out) {
  if (!fn->is_static)
    println_fmt(out, "  .globl %s", fn->name);
}

ObjFmt objfmt_coff = {
  .name = "coff",
  .description = "COFF / PE object file directives",
  .emit_var_decl = coff_emit_var_decl,
  .emit_var_type_size = NULL,
  .emit_fn_decl = coff_emit_fn_decl,
  .emit_fn_type = NULL,
};

// Mach-O object format
static void macho_emit_var_decl(Obj *var, FILE *out) {
  if (!var->is_static)
    println_fmt(out, "  .globl _%s", var->name);
}

static void macho_emit_fn_decl(Obj *fn, FILE *out) {
  if (!fn->is_static)
    println_fmt(out, "  .globl _%s", fn->name);
}

ObjFmt objfmt_macho = {
  .name = "macho",
  .description = "Mach-O object file directives",
  .emit_var_decl = macho_emit_var_decl,
  .emit_var_type_size = NULL,
  .emit_fn_decl = macho_emit_fn_decl,
  .emit_fn_type = NULL,
};

// Flat binary / simple assembly
static void flat_emit_var_decl(Obj *var, FILE *out) {
  if (!var->is_static)
    println_fmt(out, "  .globl %s", var->name);
}

static void flat_emit_fn_decl(Obj *fn, FILE *out) {
  if (!fn->is_static)
    println_fmt(out, "  .globl %s", fn->name);
}

ObjFmt objfmt_flat = {
  .name = "flat",
  .description = "Flat / plain assembler directives",
  .emit_var_decl = flat_emit_var_decl,
  .emit_var_type_size = NULL,
  .emit_fn_decl = flat_emit_fn_decl,
  .emit_fn_type = NULL,
};

static ObjFmt *registered_fmts[16];
static int num_registered_fmts = 0;

void register_objfmt(ObjFmt *fmt) {
  if (num_registered_fmts < 16)
    registered_fmts[num_registered_fmts++] = fmt;
}

ObjFmt *get_objfmt(const char *name) {
  for (int i = 0; i < num_registered_fmts; i++) {
    if (!strcmp(registered_fmts[i]->name, name))
      return registered_fmts[i];
  }
  return NULL;
}

void set_objfmt(const char *name) {
  ObjFmt *f = get_objfmt(name);
  if (f)
    current_objfmt = f;
}

void init_objfmts(void) {
  num_registered_fmts = 0;
  register_objfmt(&objfmt_elf);
  register_objfmt(&objfmt_coff);
  register_objfmt(&objfmt_macho);
  register_objfmt(&objfmt_flat);
  current_objfmt = &objfmt_elf;
}
