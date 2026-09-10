#include "test.h"

static int opaque_val(int x) {
  return x;
}

// 1. Identical Binary Expressions
static int test_identical_cse(int a, int b) {
  int t1 = a + b;
  int t2 = a + b;
  int t3 = a + b;
  ASSERT(t1, t2);
  ASSERT(t2, t3);
  return t1 + t2 + t3;
}

// 2. Commutative Expression Matching (x+y == y+x, x*y == y*x, x&y == y&x, etc.)
static int test_commutative_cse(int a, int b) {
  // Addition
  int add1 = a + b;
  int add2 = b + a;
  ASSERT(add1, add2);

  // Multiplication
  int mul1 = a * b;
  int mul2 = b * a;
  ASSERT(mul1, mul2);

  // Bitwise AND
  int and1 = a & b;
  int and2 = b & a;
  ASSERT(and1, and2);

  // Bitwise OR
  int or1 = a | b;
  int or2 = b | a;
  ASSERT(or1, or2);

  // Bitwise XOR
  int xor1 = a ^ b;
  int xor2 = b ^ a;
  ASSERT(xor1, xor2);

  return add1 + mul1 + and1 + or1 + xor1;
}

// 3. Repeated Subexpressions in Math Expressions
static int test_complex_subexpr(int x, int y, int z) {
  int term1 = (x * y) + (y * z);
  int term2 = (y * x) + (z * y);
  ASSERT(term1, term2);

  int quad1 = (x + y) * (x + y);
  int quad2 = (y + x) * (x + y);
  ASSERT(quad1, quad2);

  return term1 + quad1;
}

// 4. Memory Store Barrier for Load CSE
static int test_cse_memory_barrier(void) {
  int arr[4] = { 10, 20, 30, 40 };

  int v1 = arr[1];
  arr[1] = 99; // Barrier: must not reuse previous load
  int v2 = arr[1];

  ASSERT(10, arr[0]);
  ASSERT(99, arr[1]);
  ASSERT(20, v1);
  ASSERT(99, v2);

  return 0;
}

int main(void) {
  int a = opaque_val(15);
  int b = opaque_val(25);
  int c = opaque_val(35);

  ASSERT(120, test_identical_cse(a, b));
  test_commutative_cse(a, b);
  test_complex_subexpr(a, b, c);
  test_cse_memory_barrier();

  printf("OK\n");
  return 0;
}
