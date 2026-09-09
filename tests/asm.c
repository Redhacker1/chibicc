#include "test.h"

char *asm_fn1(void) {
  asm("mov $50, %rax\n\t"
      "mov %rbp, %rsp\n\t"
      "pop %rbp\n\t"
      "ret");
}

char *asm_fn2(void) {
  asm inline volatile("mov $55, %rax\n\t"
                      "mov %rbp, %rsp\n\t"
                      "pop %rbp\n\t"
                      "ret");
}

char *asm_fn3(void) {
  __asm__ __volatile__("mov {$}60, %rax\n\t"
                       "mov %rbp, %rsp\n\t"
                       "pop %rbp\n\t"
                       "ret");
}

unsigned char _unused_decl(const long *Base, long Offset);
extern __inline__ __attribute__((__gnu_inline__, __always_inline__))
unsigned char _unused_decl(const long *Base, long Offset) {
  __asm__ ("btl %[Offset],%[Base]\n\tsetc %[old]"
           : [old] "=qm" (Offset)
           : [Offset] "r" (Offset), [Base] "m" (*Base));
  return 0;
}

static inline void _unused_static_inline(int x) {
  __asm__ ("mov %[val], %%eax" : : [val] "r" (x));
}

static inline int _used_inline(int x) {
  return x + 10;
}

int main() {
  ASSERT(50, asm_fn1());
  ASSERT(55, asm_fn2());
  ASSERT(60, asm_fn3());
  ASSERT(15, _used_inline(5));

  printf("OK\n");
  return 0;
}
