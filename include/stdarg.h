#ifndef __STDARG_H
#define __STDARG_H

typedef __builtin_va_list va_list;

#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_end(ap) __builtin_va_end(ap)
#define va_copy(dest, src) __builtin_va_copy(dest, src)
#define va_arg(ap, ty) __builtin_va_arg(ap, ty)

#define _VA_LIST_DEFINED
#define __GNUC_VA_LIST 1
typedef va_list __gnuc_va_list;

#endif
