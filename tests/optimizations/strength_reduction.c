#include "test.h"

static int opaque_val(int x) {
  return x;
}

// 1. Power of Two Multiplications with Various Integers
static int test_power_of_two_mult(int x) {
  ASSERT(x * 1, x);
  ASSERT(x * 2, x + x);
  ASSERT(x * 4, (x + x) + (x + x));
  ASSERT(x * 8, x << 3);
  ASSERT(x * 16, x << 4);
  ASSERT(x * 32, x << 5);
  ASSERT(x * 64, x << 6);
  ASSERT(x * 128, x << 7);
  ASSERT(x * 256, x << 8);
  ASSERT(x * 512, x << 9);
  ASSERT(x * 1024, x << 10);
  ASSERT(x * 2048, x << 11);
  ASSERT(x * 4096, x << 12);
  ASSERT(x * 8192, x << 13);
  ASSERT(x * 16384, x << 14);
  ASSERT(x * 32768, x << 15);
  ASSERT(x * 65536, x << 16);

  return 0;
}

// 2. Linear Expression Simplifications
static int test_linear_expressions(int x) {
  // (x * 2) + (x * 2) = x * 4
  int r1 = (x * 2) + (x * 2);
  ASSERT(x * 4, r1);

  // (x * 8) - (x * 4) = x * 4
  int r2 = (x * 8) - (x * 4);
  ASSERT(x * 4, r2);

  // (x * 16) / 2 = x * 8 (assuming no overflow)
  int r3 = (x * 16) / 2;
  ASSERT(x * 8, r3);

  return 0;
}

// 3. Bitwise Strength Reductions
static int test_bitwise_strength(int x) {
  // x & ~0 = x
  ASSERT(x, x & ~0);

  // x | ~0 = ~0
  ASSERT(~0, x | ~0);

  // x ^ ~0 = ~x
  ASSERT(~x, x ^ ~0);

  // (x << 2) >> 2 for non-negative
  if (x >= 0 && x < 0x10000000) {
    ASSERT(x, (x << 2) >> 2);
  }

  return 0;
}

int main(void) {
  int val = opaque_val(23);
  int zero = opaque_val(0);
  int neg = opaque_val(-17);

  test_power_of_two_mult(val);
  test_power_of_two_mult(zero);
  test_power_of_two_mult(neg);

  test_linear_expressions(val);
  test_linear_expressions(neg);

  test_bitwise_strength(val);
  test_bitwise_strength(neg);

  printf("OK\n");
  return 0;
}
