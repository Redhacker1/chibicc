#include "test.h"

__callingconv(win64) int win64_func(int a, int b, int c, int d) {
  return a + b + c + d;
}

__callingconv(sysv64) int sysv_func(int a, int b, int c, int d, int e, int f) {
  return a + b + c + d + e + f;
}

__attribute__((ms_abi)) int ms_func(int a, int b) {
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
