#include "test.h"

static int opaque_val(int x) {
  return x;
}

static long long opaque_ll(long long x) {
  return x;
}

// 1. Negative immediate sign-extension and pointer arithmetic
static void test_negative_imm_and_ptr_arith(void) {
  char buf[32] = "abcdefghijkl";
  char *p = buf + 5; // points to 'f'
  ASSERT('f', *p);

  p += -1; // must move back to 'e'
  ASSERT('e', *p);

  int offset = -2;
  p += offset; // must move back to 'c'
  ASSERT('c', *p);

  long long l_offset = -1LL;
  p += l_offset; // must move back to 'b'
  ASSERT('b', *p);

  // Test do-while pointer loop with *p++ and negative step
  char text[] = "hello\nworld\n";
  char *tp = text;
  int lines = 0;
  do {
    if (*tp == '\n')
      lines++;
  } while (*tp++);
  ASSERT(2, lines);
}

// 2. Memzero and Memcpy register cache invalidation
static void test_memzero_memcpy_clobber(void) {
  struct { int a; int b; int c; int d; } s = { 0 };
  int target = 42;
  s.a = opaque_val(1);
  s.b = opaque_val(2);
  s.c = opaque_val(3);
  s.d = opaque_val(4);
  ASSERT(1, s.a);
  ASSERT(2, s.b);
  ASSERT(3, s.c);
  ASSERT(4, s.d);

  // Zero out local struct then immediately store through a pointer
  struct { char data[16]; } z = { 0 };
  int *p = &target;
  *p = 999;
  ASSERT(999, target);
  ASSERT(0, z.data[0]);

  // Copy local struct then store
  struct { int x; int y; } src = { 10, 20 };
  struct { int x; int y; } dst = src;
  int val = 77;
  int *pval = &val;
  *pval = 88;
  ASSERT(88, val);
  ASSERT(10, dst.x);
  ASSERT(20, dst.y);
}

// 3. Floating-point return values and arithmetic
static float return_float(float x) {
  return x * 2.5f;
}

static double return_double(double x) {
  return x * 3.75;
}

static void test_flonum_returns(void) {
  float f = return_float(2.0f);
  ASSERT(1, f == 5.0f);

  double d = return_double(4.0);
  ASSERT(1, d == 15.0);

  float g1 = 1.5f;
  double g2 = 2.25;
  ASSERT(1, g1 == 1.5f);
  ASSERT(1, g2 == 2.25);
}

// 4. Spill slot reuse across multiple integer sizes and lifetimes
static void test_mixed_size_spill_reuse(void) {
  long long big = opaque_ll(-1LL);
  int sml = opaque_val(42);
  short w = (short)opaque_val(-5);
  char b = (char)opaque_val(-12);

  ASSERT(-1LL, big);
  ASSERT(42, sml);
  ASSERT(-5, w);
  ASSERT(-12, b);

  long long sum = big + sml + w + b;
  ASSERT(24LL, sum);

  // Large chain of calculations to exercise register allocator spill slot reuse
  int v0 = opaque_val(1);
  int v1 = v0 + 1;
  int v2 = v1 + 1;
  int v3 = v2 + 1;
  int v4 = v3 + 1;
  int v5 = v4 + 1;
  int v6 = v5 + 1;
  int v7 = v6 + 1;
  int v8 = v7 + 1;
  int v9 = v8 + 1;
  int v10 = v9 + 1;
  int v11 = v10 + 1;
  int v12 = v11 + 1;
  int v13 = v12 + 1;
  int v14 = v13 + 1;
  int v15 = v14 + 1;
  ASSERT(16, v15);
}

// 5. Direct named function calls
static int callee_one(int x) { return x + 10; }
static int callee_two(int x, int y) { return x * y; }

static void test_direct_calls(void) {
  int r1 = callee_one(opaque_val(5));
  ASSERT(15, r1);

  int r2 = callee_two(opaque_val(6), opaque_val(7));
  ASSERT(42, r2);
}

// 6. Pointer scaling and subtraction (char vs int pointers)
static void test_pointer_scaling(void) {
  char str[] = "hello world";
  char *p1 = str;
  char *p2 = str + 5;
  ASSERT(5, p2 - p1);

  p1 = p2 - 3;
  ASSERT('l', *p1);

  int arr[10] = { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90 };
  int *ip1 = &arr[2];
  int *ip2 = &arr[7];
  ASSERT(5, ip2 - ip1);
  ASSERT(50, *(ip2 - 2));
}

int main(void) {
  test_negative_imm_and_ptr_arith();
  test_memzero_memcpy_clobber();
  test_flonum_returns();
  test_mixed_size_spill_reuse();
  test_direct_calls();
  test_pointer_scaling();

  printf("OK\n");
  return 0;
}
