#include "tests/test.h"

// Comprehensive Cross-ABI and Nested Multi-ABI Test Suite
// Testing mix-and-match and deep nesting between SysV AMD64, Win64, and Win32 (cdecl) ABIs.

// Calling convention macros
#define SYSV_FN __attribute__((sysv_abi))
#define WIN64_FN __attribute__((ms_abi))
#define WIN32_FN __attribute__((cdecl))

// ========================================================
// 1. Structures and Unions for Cross-ABI passing
// ========================================================

typedef struct {
  char a;
  char b;
  char c;
} CrossS3;

typedef struct {
  short x;
  short y;
} CrossS4;

typedef struct {
  char a, b, c, d, e;
} CrossS5;

typedef struct {
  int a;
  int b;
} CrossS8;

typedef struct {
  char a[9];
} CrossS9;

typedef struct {
  long long a;
  double b;
} CrossS16;

typedef struct {
  long long a;
  long long b;
  long long c;
} CrossS24;

typedef union {
  int i;
  float f;
} CrossU4;

typedef union {
  long long ll;
  double d;
} CrossU8;

typedef struct {
  CrossS3 s3;
  CrossS8 s8;
  CrossU4 u4;
} CrossNestedStruct;

// ========================================================
// 2. Direct Cross-ABI Calls (SysV <-> Win64 <-> Win32)
// ========================================================

static int WIN64_FN cross_target_win64(int a, int b, int c, int d, int e, int f) {
  return a * 1 + b * 2 + c * 3 + d * 4 + e * 5 + f * 6;
}

static int SYSV_FN cross_target_sysv(int a, int b, int c, int d, int e, int f, int g, int h) {
  return a + b + c + d + e + f + g + h;
}

static int WIN32_FN cross_target_win32(int a, int b, int c, int d, int e) {
  return a - b + c - d + e;
}

// SysV caller calling Win64 and Win32
static int SYSV_FN sysv_calling_win64_and_win32(int x) {
  int res1 = cross_target_win64(x, 2, 3, 4, 5, 6); // 1*1 + 2*2 + 3*3 + 4*4 + 5*5 + 6*6 = 1+4+9+16+25+36 = 91 (if x=1)
  int res2 = cross_target_win32(10, 5, 20, 10, 30); // 10 - 5 + 20 - 10 + 30 = 45
  return res1 + res2;
}

// Win64 caller calling SysV and Win32
static int WIN64_FN win64_calling_sysv_and_win32(int x) {
  int res1 = cross_target_sysv(x, 2, 3, 4, 5, 6, 7, 8); // 1+2+3+4+5+6+7+8 = 36 (if x=1)
  int res2 = cross_target_win32(50, 10, 20, 30, 40); // 50 - 10 + 20 - 30 + 40 = 70
  return res1 + res2;
}

// Win32 caller calling SysV and Win64
static int WIN32_FN win32_calling_sysv_and_win64(int x) {
  int res1 = cross_target_sysv(x, 1, 1, 1, 1, 1, 1, 1); // 1+7 = 8 (if x=1)
  int res2 = cross_target_win64(x, 1, 1, 1, 1, 1); // 1+2+3+4+5+6 = 21 (if x=1)
  return res1 + res2;
}

static void test_direct_cross_calls(void) {
  ASSERT(136, sysv_calling_win64_and_win32(1));
  ASSERT(106, win64_calling_sysv_and_win32(1));
  ASSERT(29, win32_calling_sysv_and_win64(1));
}

// ========================================================
// 3. Multi-Level Deep Nested Call Chains
// Chain: SysV -> Win64 -> Win32 -> SysV -> Win64 -> Win32
// ========================================================

static int WIN32_FN nest_leaf_win32(int a, int b) {
  return a * 10 + b;
}

static int SYSV_FN nest_level3_sysv(int a, int b, int c) {
  int sub = nest_leaf_win32(a, b);
  return sub + c * 100;
}

static int WIN32_FN nest_level2_win32(int a, int b, int c, int d) {
  int sub = nest_level3_sysv(a, b, c);
  return sub + d * 1000;
}

static int WIN64_FN nest_level1_win64(int a, int b, int c, int d, int e) {
  int sub = nest_level2_win32(a, b, c, d);
  return sub + e * 10000;
}

static int SYSV_FN nest_root_sysv(int a, int b, int c, int d, int e, int f) {
  int sub = nest_level1_win64(a, b, c, d, e);
  return sub + f * 100000;
}

static void test_nested_call_chains(void) {
  // a=1, b=2 => leaf=12
  // c=3 => lvl3=12 + 300 = 312
  // d=4 => lvl2=312 + 4000 = 4312
  // e=5 => lvl1=4312 + 50000 = 54312
  // f=6 => root=54312 + 600000 = 654312
  ASSERT(654312, nest_root_sysv(1, 2, 3, 4, 5, 6));
}

// ========================================================
// 4. Struct / Union Passing and Returning Across ABIs
// ========================================================

static CrossS3 WIN64_FN cross_s3_win64(CrossS3 s, int add) {
  CrossS3 res;
  res.a = s.a + add;
  res.b = s.b + add;
  res.c = s.c + add;
  return res;
}

static CrossS4 SYSV_FN cross_s4_sysv(CrossS4 s, int mult) {
  CrossS4 res;
  res.x = s.x * mult;
  res.y = s.y * mult;
  return res;
}

static CrossS5 WIN32_FN cross_s5_win32(CrossS5 s) {
  CrossS5 res;
  res.a = s.a + 1;
  res.b = s.b + 2;
  res.c = s.c + 3;
  res.d = s.d + 4;
  res.e = s.e + 5;
  return res;
}

static CrossS16 WIN64_FN cross_s16_win64(CrossS16 s) {
  CrossS16 res;
  res.a = s.a + 10;
  res.b = s.b + 20.5;
  return res;
}

static CrossS24 SYSV_FN cross_s24_sysv(CrossS24 s) {
  CrossS24 res;
  res.a = s.a + 100;
  res.b = s.b + 200;
  res.c = s.c + 300;
  return res;
}

static CrossNestedStruct SYSV_FN cross_nest_struct_sysv(CrossNestedStruct n) {
  // Mix calls to Win64 and Win32 inside SysV while transforming struct
  CrossS3 new_s3 = cross_s3_win64(n.s3, 5);
  CrossNestedStruct res;
  res.s3 = new_s3;
  res.s8.a = n.s8.a + 10;
  res.s8.b = n.s8.b + 20;
  res.u4.i = n.u4.i + 100;
  return res;
}

static void test_cross_struct_passing(void) {
  CrossS3 s3 = {1, 2, 3};
  CrossS3 r3 = cross_s3_win64(s3, 10);
  ASSERT(11, r3.a);
  ASSERT(12, r3.b);
  ASSERT(13, r3.c);

  CrossS4 s4 = {15, 25};
  CrossS4 r4 = cross_s4_sysv(s4, 3);
  ASSERT(45, r4.x);
  ASSERT(75, r4.y);

  CrossS5 s5 = {10, 20, 30, 40, 50};
  CrossS5 r5 = cross_s5_win32(s5);
  ASSERT(11, r5.a);
  ASSERT(22, r5.b);
  ASSERT(33, r5.c);
  ASSERT(44, r5.d);
  ASSERT(55, r5.e);

  CrossS16 s16 = {100, 50.0};
  CrossS16 r16 = cross_s16_win64(s16);
  ASSERT(110, r16.a);
  ASSERT(1, r16.b == 70.5);

  CrossS24 s24 = {1, 2, 3};
  CrossS24 r24 = cross_s24_sysv(s24);
  ASSERT(101, r24.a);
  ASSERT(202, r24.b);
  ASSERT(303, r24.c);

  CrossNestedStruct nest;
  nest.s3.a = 1; nest.s3.b = 2; nest.s3.c = 3;
  nest.s8.a = 5; nest.s8.b = 6;
  nest.u4.i = 50;

  CrossNestedStruct rnest = cross_nest_struct_sysv(nest);
  ASSERT(6, rnest.s3.a);
  ASSERT(7, rnest.s3.b);
  ASSERT(8, rnest.s3.c);
  ASSERT(15, rnest.s8.a);
  ASSERT(26, rnest.s8.b);
  ASSERT(150, rnest.u4.i);
}

// ========================================================
// 5. Mixed Floating-Point and Integer Across ABI Boundaries
// ========================================================

static double WIN64_FN mix_fp_int_win64(int a, double b, int c, double d, int e, double f, int g, double h) {
  return (a + c + e + g) + (b + d + f + h);
}

static double SYSV_FN mix_fp_int_sysv(double a, int b, double c, int d, double e, int f, double g, int h) {
  // Call Win64 from SysV passing mixed values
  double w = mix_fp_int_win64(b, a, d, c, f, e, h, g);
  return w * 2.0;
}

static void test_cross_mixed_fp_int(void) {
  double res = mix_fp_int_sysv(1.5, 10, 2.5, 20, 3.5, 30, 4.5, 40);
  // ints: 10 + 20 + 30 + 40 = 100
  // floats: 1.5 + 2.5 + 3.5 + 4.5 = 12.0
  // sum = 112.0, * 2.0 = 224.0
  ASSERT(1, res == 224.0);
}

// ========================================================
// 6. Function Pointers with Calling Conventions Across ABIs
// ========================================================

typedef int (SYSV_FN *SysVCallback)(int, int, int);
typedef int (WIN64_FN *Win64Callback)(int, int, int);
typedef int (WIN32_FN *Win32Callback)(int, int, int);

static int SYSV_FN callback_sysv_impl(int a, int b, int c) {
  return a * 100 + b * 10 + c;
}

static int WIN64_FN callback_win64_impl(int a, int b, int c) {
  return a * 1000 + b * 100 + c;
}

static int WIN32_FN callback_win32_impl(int a, int b, int c) {
  return a + b + c;
}

static int WIN64_FN win64_invoke_sysv_cb(SysVCallback cb, int a, int b, int c) {
  return cb(a, b, c) + 5;
}

static int SYSV_FN sysv_invoke_win64_cb(Win64Callback cb, int a, int b, int c) {
  return cb(a, b, c) + 7;
}

static int WIN32_FN win32_invoke_all_cbs(SysVCallback c1, Win64Callback c2, Win32Callback c3) {
  int r1 = c1(1, 2, 3); // 123
  int r2 = c2(4, 5, 6); // 4506
  int r3 = c3(7, 8, 9); // 24
  return r1 + r2 + r3;
}

static void test_cross_function_pointers(void) {
  ASSERT(128, win64_invoke_sysv_cb(callback_sysv_impl, 1, 2, 3));
  ASSERT(1211, sysv_invoke_win64_cb(callback_win64_impl, 1, 2, 4));
  ASSERT(4653, win32_invoke_all_cbs(callback_sysv_impl, callback_win64_impl, callback_win32_impl));
}

// ========================================================
// 7. Variadic Functions Invoked Across ABIs
// ========================================================

static int WIN64_FN win64_vararg(int count, ...) {
  va_list ap;
  va_start(ap, count);
  int sum = 0;
  for (int i = 0; i < count; i++) {
    sum += va_arg(ap, int);
  }
  va_end(ap);
  return sum;
}

static int SYSV_FN sysv_vararg(int count, ...) {
  __va_elem ap[1];
  va_start(ap, count);
  int sum = 0;
  for (int i = 0; i < count; i++) {
    sum += va_arg(ap, int);
  }
  va_end(ap);
  return sum;
}

static int SYSV_FN sysv_call_varargs(void) {
  int r1 = win64_vararg(5, 10, 20, 30, 40, 50); // 150
  int r2 = win64_vararg(8, 1, 2, 3, 4, 5, 6, 7, 8); // 36
  return r1 + r2;
}

static int WIN64_FN win64_call_varargs(void) {
  int r1 = sysv_vararg(5, 10, 20, 30, 40, 50); // 150
  int r2 = sysv_vararg(8, 1, 2, 3, 4, 5, 6, 7, 8); // 36
  return r1 + r2;
}

static void test_cross_varargs(void) {
  ASSERT(186, sysv_call_varargs());
  ASSERT(186, win64_call_varargs());
}

// ========================================================
// 8. Mutual Recursion Alternating Between ABIs
// ========================================================

static int WIN64_FN rec_win64(int n, int acc);
static int WIN32_FN rec_win32(int n, int acc);

static int SYSV_FN rec_sysv(int n, int acc) {
  if (n <= 0) return acc;
  return rec_win64(n - 1, acc + n);
}

static int WIN64_FN rec_win64(int n, int acc) {
  if (n <= 0) return acc;
  return rec_win32(n - 1, acc + n * 2);
}

static int WIN32_FN rec_win32(int n, int acc) {
  if (n <= 0) return acc;
  return rec_sysv(n - 1, acc + n * 3);
}

static void test_cross_recursion(void) {
  // n=6, acc=0:
  // rec_sysv(6, 0) -> rec_win64(5, 6) -> rec_win32(4, 6 + 10=16)
  // -> rec_sysv(3, 16 + 12=28) -> rec_win64(2, 28 + 3=31) -> rec_win32(1, 31 + 4=35)
  // -> rec_sysv(0, 35 + 3=38) -> 38
  ASSERT(38, rec_sysv(6, 0));
}

// ========================================================
// 9. Main Cross-ABI Test Runner
// ========================================================

int test_cross_abi(void) {
  printf("Testing Cross-ABI Interoperability and Nesting (SysV64 / Win64 / Win32)...\n");

  test_direct_cross_calls();
  test_nested_call_chains();
  test_cross_struct_passing();
  test_cross_mixed_fp_int();
  test_cross_function_pointers();
  test_cross_varargs();
  test_cross_recursion();

  printf("Cross-ABI tests passed successfully!\n");
  return 0;
}

#ifndef RUN_ALL_ABI_TESTS
int main(void) {
  return test_cross_abi();
}
#endif
