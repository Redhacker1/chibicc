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
