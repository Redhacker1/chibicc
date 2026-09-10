#ifndef CHIBICC_CODEGEN_COMMON_H
#define CHIBICC_CODEGEN_COMMON_H

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

// Round up `n` to the nearest multiple of `align`.
int align_to(int n, int align);

// Unique label counter
int codegen_label_count(void);

// Instruction output stream hook
void codegen_set_print_hook(void (*hook)(const char *s));
void codegen_println(FILE *out, char *fmt, ...);

#endif // CHIBICC_CODEGEN_COMMON_H
