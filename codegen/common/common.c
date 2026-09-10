#include "chibicc.h"
#include "codegen/common/common.h"

int align_to(int n, int align) {
  if (align <= 0)
    return n;
  return (n + align - 1) / align * align;
}

int codegen_label_count(void) {
  static int i = 1;
  return i++;
}

static void (*print_hook)(const char *s) = NULL;

void codegen_set_print_hook(void (*hook)(const char *s)) {
  print_hook = hook;
}

void codegen_println(FILE *out, char *fmt, ...) {
  char buf[2048];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  if (print_hook) {
    print_hook(buf);
  } else if (out) {
    fprintf(out, "%s\n", buf);
  }
}
