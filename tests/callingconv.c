#include "test.h"

int __callingconv(win64) win64_func(int a, int b, int c, int d) {
  return a + b + c + d;
}

int __callingconv(sysv64) sysv_func(int a, int b, int c, int d, int e, int f) {
  return a + b + c + d + e + f;
}

int __attribute__((ms_abi)) ms_func(int a, int b) {
  return a + b;
}

int main(void) {
  int r1 = win64_func(1, 2, 3, 4);
  int r2 = sysv_func(1, 2, 3, 4, 5, 6);
  int r3 = ms_func(10, 20);
  ASSERT(10, r1);
  ASSERT(21, r2);
  ASSERT(30, r3);
  printf("OK\n");
  return 0;
}
