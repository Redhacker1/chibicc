#include "test.h"

static int opaque_int(int x) { return x; }
static long long opaque_ll(long long x) { return x; }
static double opaque_dbl(double x) { return x; }

// 1. Constant Folding & Arithmetic Identities
static void test_const_fold_and_identities(void) {
  // Constant arithmetic
  int c1 = 10 + 20 * 3 - 5;
  ASSERT(65, c1);

  int c2 = (1 << 5) | (3 << 2) & 0x0F;
  ASSERT(44, c2);

  int c3 = -(-42);
  ASSERT(42, c3);

  int c4 = ~0;
  ASSERT(-1, c4);

  int c5 = !0;
  ASSERT(1, c5);

  int c6 = !123;
  ASSERT(0, c6);

  // Dynamic variable identities with 0 and 1
  int x = opaque_int(77);
  ASSERT(77, x + 0);
  ASSERT(77, 0 + x);
  ASSERT(77, x - 0);
  ASSERT(0, x - x);
  ASSERT(77, x * 1);
  ASSERT(77, 1 * x);
  ASSERT(0, x * 0);
  ASSERT(0, 0 * x);
  ASSERT(77, x / 1);
  ASSERT(1, x / x);
  ASSERT(77, x | 0);
  ASSERT(77, 0 | x);
  ASSERT(77, x | x);
  ASSERT(0, x & 0);
  ASSERT(0, 0 & x);
  ASSERT(77, x & x);
  ASSERT(77, x ^ 0);
  ASSERT(0, x ^ x);
  ASSERT(77, x << 0);
  ASSERT(77, x >> 0);

  // Reflexive comparisons
  ASSERT(1, x == x);
  ASSERT(0, x != x);
  ASSERT(1, x <= x);
  ASSERT(1, x >= x);
  ASSERT(0, x < x);
  ASSERT(0, x > x);
}

// 2. Copy Propagation & Local CSE
static void test_copy_prop_and_local_cse(void) {
  int a = opaque_int(12);
  int b = a;
  int c = b;
  int d = c + 5;
  ASSERT(17, d);

  // Common subexpression elimination (both commutative and direct)
  int x = opaque_int(30);
  int y = opaque_int(40);
  int e1 = (x * y) + (y * x);
  ASSERT(2400, e1);

  int e2 = (x + y) * 2 - (y + x);
  ASSERT(70, e2);

  // CSE with struct accesses
  struct Point { int x; int y; } pt = { opaque_int(5), opaque_int(10) };
  int sum = pt.x + pt.y + pt.x + pt.y;
  ASSERT(30, sum);
}

// 3. Dead Code & Dead Store Elimination
static int test_dead_code_and_dead_store(void) {
  int unused_var = opaque_int(999);
  (void)unused_var;

  int target = opaque_int(1);
  target = 2; // overwritten
  target = 3; // overwritten
  target = opaque_int(4);
  ASSERT(4, target);

  // Control flow dead code
  int res = 0;
  if (0) {
    res = 100;
  } else {
    res = 200;
  }
  ASSERT(200, res);

  while (0) {
    res += 50;
  }
  ASSERT(200, res);

  return 0;
}

// 4. Peephole & Strength Reduction
static void test_peephole_and_strength_reduction(void) {
  int val = opaque_int(7);

  // Multiplication by power of 2
  ASSERT(14, val * 2);
  ASSERT(28, val * 4);
  ASSERT(56, val * 8);
  ASSERT(112, val * 16);
  ASSERT(224, val * 32);
  ASSERT(448, val * 64);

  // Division / Modulo
  ASSERT(3, val / 2);
  ASSERT(1, val % 2);

  // Chained bitwise operations
  int b1 = (val & 0xFF) & 0xFF;
  ASSERT(7, b1);

  int b2 = (val | 0) | 0;
  ASSERT(7, b2);
}

// 5. CFG Simplification & Branch Folding
static void test_cfg_simplification(void) {
  int flag = opaque_int(1);
  int count = 0;

  for (int i = 0; i < 10; i++) {
    if (1) {
      count++;
    }
  }
  ASSERT(10, count);

  // Ternary folding with constant condition
  int choice = (1 ? 42 : 99);
  ASSERT(42, choice);

  int choice2 = (0 ? 42 : 99);
  ASSERT(99, choice2);

  // Branch condition folding
  int path = 0;
  if (flag && 1) {
    path = 1;
  }
  ASSERT(1, path);

  if (!flag || 0) {
    path = 2;
  }
  ASSERT(1, path);
}

// 6. Memory Load/Store & Spill Invalidation
static void test_memory_transformations(void) {
  int arr[4] = { 10, 20, 30, 40 };
  int *p = arr;

  // Consecutive writes and reads
  p[0] = opaque_int(100);
  p[1] = opaque_int(200);
  ASSERT(100, p[0]);
  ASSERT(200, p[1]);

  // Aggregate copy and mutation
  struct Large {
    int data[8];
  } s1 = { { 1, 2, 3, 4, 5, 6, 7, 8 } };

  struct Large s2 = s1;
  s2.data[0] = opaque_int(99);
  ASSERT(1, s1.data[0]);
  ASSERT(99, s2.data[0]);
  ASSERT(8, s2.data[7]);
}

int main(void) {
  test_const_fold_and_identities();
  test_copy_prop_and_local_cse();
  test_dead_code_and_dead_store();
  test_peephole_and_strength_reduction();
  test_cfg_simplification();
  test_memory_transformations();

  printf("OK\n");
  return 0;
}
