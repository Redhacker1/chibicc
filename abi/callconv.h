#ifndef CHIBICC_CALLCONV_H
#define CHIBICC_CALLCONV_H

#include <stdbool.h>

typedef struct CallConv CallConv;

struct CallConv {
  const char *name;
  int num_gp_regs;
  const char *gp_regs64[8];
  const char *gp_regs32[8];
  const char *gp_regs16[8];
  const char *gp_regs8[8];
  int num_fp_regs;
  int shadow_space;       // e.g. 32 on Win64, 0 on SysV
  bool paired_slots;      // true on Win64 (float in XMMi consumes slot i)
  bool pass_struct_by_ref;// true on Win64 for struct > 8 bytes
  int stack_align;
};

#endif // CHIBICC_CALLCONV_H
