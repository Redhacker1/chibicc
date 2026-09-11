#include "test.h"

static int opaque_val(int x) {
  return x;
}

static unsigned int opaque_uval(unsigned int x) {
  return x;
}

// 1. High-level Constant Folding & Expression Simplification
static void test_hlir_const_folding(void) {
  int a = (10 + 20) * 3 - (40 / 2); // (30 * 3) - 20 = 90 - 20 = 70
  ASSERT(70, a);

  int b = (1 << 4) | (3 & 0xF); // 16 | 3 = 19
  ASSERT(19, b);

  int c = (100 % 7) + (50 >> 2); // 2 + 12 = 14
  ASSERT(14, c);

  int d = (10 == 10) + (20 != 30) + (5 < 10) + (15 >= 15); // 1 + 1 + 1 + 1 = 4
  ASSERT(4, d);

  int e = !0 + !(!42) - (-(-5)); // 1 + 1 - 5 = -3
  ASSERT(-3, e);
}

// 2. High-level Algebraic Identities
static void test_hlir_algebraic_identities(void) {
  int x = opaque_val(42);
  int y = opaque_val(100);

  ASSERT(42, x + 0);
  ASSERT(42, 0 + x);
  ASSERT(42, x - 0);
  ASSERT(0, x - x);
  ASSERT(42, x * 1);
  ASSERT(42, 1 * x);
  ASSERT(0, x * 0);
  ASSERT(0, 0 * x);
  ASSERT(42, x / 1);
  ASSERT(1, x / x);
  ASSERT(42, x | 0);
  ASSERT(42, 0 | x);
  ASSERT(42, x | x);
  ASSERT(0, x & 0);
  ASSERT(0, 0 & x);
  ASSERT(42, x & x);
  ASSERT(42, x ^ 0);
  ASSERT(42, 0 ^ x);
  ASSERT(0, x ^ x);

  // Advanced identities: negation, bitwise NOT, modulo by 1, and bitwise logic with -1
  ASSERT(-42, x * -1);
  ASSERT(-42, -1 * x);
  ASSERT(-42, 0 - x);
  ASSERT(0, x % 1);
  ASSERT(~42, x ^ -1);
  ASSERT(~42, -1 ^ x);
  ASSERT(42, x & -1);
  ASSERT(42, -1 & x);
  ASSERT(-1, x | -1);
  ASSERT(-1, -1 | x);
  ASSERT(42, x << 0);
  ASSERT(42, x >> 0);

  // Nested algebraic chain
  int r = ((x + 0) * 1 - 0) + ((y * 0) | (x ^ x));
  ASSERT(42, r);
}

// 3. High-level & Low-level Control Flow and Dead Code
static int test_hlir_control_flow(int val) {
  int result = 0;

  if (1) {
    result += val;
  } else {
    result += 9999;
  }

  if (0) {
    result += 8888;
  } else {
    result += 10;
  }

  return result;
}

// 4. Strength Reductions (Division and Modulo by power of 2)
static void test_strength_reductions(void) {
  unsigned int u = opaque_uval(64);
  ASSERT(16, u / 4);
  ASSERT(8, u / 8);
  ASSERT(2, u / 32);

  unsigned int u2 = opaque_uval(77);
  ASSERT(13, u2 % 64);
  ASSERT(5, u2 % 8);
  ASSERT(1, u2 % 2);

  int s = opaque_val(9);
  ASSERT(72, s * 8);
  ASSERT(144, s * 16);
  ASSERT(0, s * 0);
  ASSERT(9, s * 1);
}

// 5. Canonicalization of Commutative and Relational Expressions with Constants
static void test_canonicalization(void) {
  int x = opaque_val(15);
  ASSERT(1, 10 < x);
  ASSERT(0, 20 < x);
  ASSERT(1, 15 <= x);
  ASSERT(0, 16 <= x);
  ASSERT(1, 20 > x);
  ASSERT(0, 10 > x);
  ASSERT(1, 15 >= x);
  ASSERT(0, 14 >= x);
  ASSERT(1, 15 == x);
  ASSERT(0, 16 == x);
  ASSERT(1, 16 != x);
  ASSERT(0, 15 != x);
  ASSERT(25, 10 + x);
  ASSERT(150, 10 * x);
  ASSERT(15, 0xFF & x);
  ASSERT(15, 0 | x);
  ASSERT(15, 0 ^ x);
}

// 6. High-level Double Unary & Redundant Operations
static void test_hlir_double_unary_and_cse(void) {
  int x = opaque_val(42);
  ASSERT(42, -(-x));
  ASSERT(42, ~(~x));
  ASSERT(1, !(!x));

  // Local CSE test: repeated expressions in straight line code
  int a = x + 10;
  int b = x + 10;
  int c = a * b;
  ASSERT(2704, c); // (42 + 10) * (42 + 10) = 52 * 52 = 2704

  // Redundant store followed by store (DSE) & load-after-store
  int temp = 100;
  temp = x + 5; // overwrite dead store
  int read_back = temp;
  ASSERT(47, read_back);
}

int main(void) {
  test_hlir_const_folding();
  test_hlir_algebraic_identities();
  ASSERT(52, test_hlir_control_flow(42));
  test_strength_reductions();
  test_canonicalization();
  test_hlir_double_unary_and_cse();

  printf("OK\n");
  return 0;
}
