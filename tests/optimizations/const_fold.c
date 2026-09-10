#include "test.h"

// Prevent compiler from evaluating at AST constant-folding time where desired
static int opaque_val(int x) {
  return x;
}

// 1. Binary Constant Folding
static int test_binary_const_folding(void) {
  int a = 100 + 200;
  ASSERT(300, a);

  int b = 500 - 150;
  ASSERT(350, b);

  int c = 25 * 4;
  ASSERT(100, c);

  int d = 1000 / 8;
  ASSERT(125, d);

  int e = 100 % 30;
  ASSERT(10, e);

  int f = 0xF0 & 0x3C;
  ASSERT(0x30, f);

  int g = 0xF0 | 0x0F;
  ASSERT(0xFF, g);

  int h = 0xFF ^ 0x0F;
  ASSERT(0xF0, h);

  int i = 1 << 5;
  ASSERT(32, i);

  int j = 64 >> 2;
  ASSERT(16, j);

  // Relational & comparison folding
  ASSERT(1, 10 == 10);
  ASSERT(0, 10 == 20);
  ASSERT(1, 10 != 20);
  ASSERT(0, 10 != 10);
  ASSERT(1, 5 < 10);
  ASSERT(0, 10 < 5);
  ASSERT(1, 10 <= 10);
  ASSERT(1, 5 <= 10);
  ASSERT(0, 10 <= 5);
  ASSERT(1, 10 > 5);
  ASSERT(0, 5 > 10);
  ASSERT(1, 10 >= 10);
  ASSERT(1, 10 >= 5);
  ASSERT(0, 5 >= 10);

  return 0;
}

// 2. Unary Constant Folding
static int test_unary_const_folding(void) {
  int a = -(-42);
  ASSERT(42, a);

  int b = ~0;
  ASSERT(-1, b);

  int c = !0;
  ASSERT(1, c);

  int d = !42;
  ASSERT(0, d);

  int e = !(!5);
  ASSERT(1, e);

  return 0;
}

// 3. Algebraic Identities with Dynamic Variables
static int test_algebraic_identities(int x, int y) {
  // x + 0 = x, 0 + x = x
  ASSERT(x, x + 0);
  ASSERT(x, 0 + x);

  // x - 0 = x, x - x = 0
  ASSERT(x, x - 0);
  ASSERT(0, x - x);
  ASSERT(0, y - y);

  // x * 1 = x, 1 * x = x
  ASSERT(x, x * 1);
  ASSERT(x, 1 * x);

  // x * 0 = 0, 0 * x = 0
  ASSERT(0, x * 0);
  ASSERT(0, 0 * x);

  // x / 1 = x
  ASSERT(x, x / 1);
  ASSERT(y, y / 1);

  // x ^ x = 0
  ASSERT(0, x ^ x);
  ASSERT(0, y ^ y);

  // x ^ 0 = x, 0 ^ x = x
  ASSERT(x, x ^ 0);
  ASSERT(x, 0 ^ x);

  // x & 0 = 0, 0 & x = 0
  ASSERT(0, x & 0);
  ASSERT(0, 0 & x);

  // x | 0 = x, 0 | x = x
  ASSERT(x, x | 0);
  ASSERT(x, 0 | x);

  return 0;
}

// 4. Combined Algebraic Expressions
static int test_combined_algebraic(int x, int y, int z) {
  // (x + 0) * 1 - 0 = x
  int r1 = (x + 0) * 1 - 0;
  ASSERT(x, r1);

  // (x * 0) + (y * 1) + (z - z) = y
  int r2 = (x * 0) + (y * 1) + (z - z);
  ASSERT(y, r2);

  // (x ^ x) | (y & 0) | (z ^ 0) = z
  int r3 = (x ^ x) | (y & 0) | (z ^ 0);
  ASSERT(z, r3);

  // ((x * 1) + (y * 0)) / 1 = x
  int r4 = ((x * 1) + (y * 0)) / 1;
  ASSERT(x, r4);

  return 0;
}

// 5. 64-bit Integer Constant Folding
static int test_int64_const_folding(void) {
  long long a = 1000000000LL + 2000000000LL;
  ASSERT(1, a == 3000000000LL);

  long long b = 5000000000LL - 2000000000LL;
  ASSERT(1, b == 3000000000LL);

  long long c = 1000000LL * 2000LL;
  ASSERT(1, c == 2000000000LL);

  long long d = 9000000000LL / 3LL;
  ASSERT(1, d == 3000000000LL);

  long long e = 1LL << 40;
  ASSERT(1, (e >> 40) == 1LL);

  return 0;
}

int main(void) {
  test_binary_const_folding();
  test_unary_const_folding();

  int x = opaque_val(42);
  int y = opaque_val(123);
  int z = opaque_val(777);

  test_algebraic_identities(x, y);
  test_combined_algebraic(x, y, z);
  test_int64_const_folding();

  // Test negative values
  test_algebraic_identities(opaque_val(-50), opaque_val(-999));
  test_combined_algebraic(opaque_val(-10), opaque_val(20), opaque_val(-30));

  printf("OK\n");
  return 0;
}
