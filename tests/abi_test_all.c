#define RUN_ALL_ABI_TESTS 1
#include "tests/test.h"
#include <stdio.h>
#include <stdlib.h>

void assert(int expected, int actual, char *code) {
  if (expected == actual) {
    printf("%s => %d\n", code, actual);
    fflush((void *)0);
  } else {
    printf("%s => %d expected but got %d\n", code, expected, actual);
    fflush((void *)0);
  }
  if (expected != actual) {
    exit(1);
  }
}

int test_sysv_abi(void);
int test_win64_abi(void);
int test_win32_abi(void);
int test_cross_abi(void);

#include "tests/abi_sysv_test.c"
#include "tests/abi_win64_test.c"
#include "tests/abi_win32_test.c"
#include "tests/abi_cross_test.c"

int main(void) {
  printf("========================================\n");
  printf("RUNNING COMPREHENSIVE ABI TEST SUITES\n");
  printf("========================================\n\n");

  test_sysv_abi();
  printf("\n");

  test_win64_abi();
  printf("\n");

  test_win32_abi();
  printf("\n");

  test_cross_abi();
  printf("\n");

  printf("========================================\n");
  printf("ALL ABI TESTS PASSED SUCCESSFULLY! (OK)\n");
  printf("========================================\n");
  exit(0);
  return 0;
}
