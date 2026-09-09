#include "test/test.h"

// System V AMD64 ABI Test Suite - Ported from References/SysV/abitest

// ========================================================
// 1. Basic Type Sizes and Alignment Tests
// ========================================================

typedef struct { char dummy; } S_Char;
typedef struct { short dummy; } S_Short;
typedef struct { int dummy; } S_Int;
typedef struct { long dummy; } S_Long;
typedef struct { long long dummy; } S_LongLong;
typedef struct { float dummy; } S_Float;
typedef struct { double dummy; } S_Double;
typedef struct { void *dummy; } S_Pointer;

typedef union { char dummy; } U_Char;
typedef union { short dummy; } U_Short;
typedef union { int dummy; } U_Int;
typedef union { long dummy; } U_Long;
typedef union { long long dummy; } U_LongLong;
typedef union { float dummy; } U_Float;
typedef union { double dummy; } U_Double;
typedef union { void *dummy; } U_Pointer;

typedef struct { char c; short s; int i; } S_C_S_I;
typedef struct { int i; double d; char c; } S_I_D_C;
typedef union { int i; double d; char c; } U_I_D_C;

static void test_sysv_sizes_and_alignments(void) {
  // Basic scalar sizes
  ASSERT(1, sizeof(char));
  ASSERT(2, sizeof(short));
  ASSERT(4, sizeof(int));
  ASSERT(8, sizeof(long long));
  ASSERT(4, sizeof(float));
  ASSERT(8, sizeof(double));
  ASSERT(8, sizeof(void *));

  // Basic scalar alignments
  ASSERT(1, _Alignof(char));
  ASSERT(2, _Alignof(short));
  ASSERT(4, _Alignof(int));
  ASSERT(8, _Alignof(long long));
  ASSERT(4, _Alignof(float));
  ASSERT(8, _Alignof(double));
  ASSERT(8, _Alignof(void *));

  // Struct & union sizes and alignments
  ASSERT(1, sizeof(S_Char));
  ASSERT(1, _Alignof(S_Char));
  ASSERT(2, sizeof(S_Short));
  ASSERT(2, _Alignof(S_Short));
  ASSERT(4, sizeof(S_Int));
  ASSERT(4, _Alignof(S_Int));
  ASSERT(8, sizeof(S_LongLong));
  ASSERT(8, _Alignof(S_LongLong));
  ASSERT(4, sizeof(S_Float));
  ASSERT(4, _Alignof(S_Float));
  ASSERT(8, sizeof(S_Double));
  ASSERT(8, _Alignof(S_Double));
  ASSERT(8, sizeof(S_Pointer));
  ASSERT(8, _Alignof(S_Pointer));

  ASSERT(1, sizeof(U_Char));
  ASSERT(2, sizeof(U_Short));
  ASSERT(4, sizeof(U_Int));
  ASSERT(8, sizeof(U_LongLong));
  ASSERT(4, sizeof(U_Float));
  ASSERT(8, sizeof(U_Double));
  ASSERT(8, sizeof(U_Pointer));

  // 3-element compound structs and unions
  ASSERT(8, sizeof(S_C_S_I));
  ASSERT(4, _Alignof(S_C_S_I));
  ASSERT(24, sizeof(S_I_D_C));
  ASSERT(8, _Alignof(S_I_D_C));
  ASSERT(8, sizeof(U_I_D_C));
  ASSERT(8, _Alignof(U_I_D_C));
}

// ========================================================
// 2. Scalar Argument Passing (GP and FP)
// ========================================================

int __attribute__((sysv_abi)) sysv_gp6(int a, int b, int c, int d, int e, int f) {
  ASSERT(1, a);
  ASSERT(2, b);
  ASSERT(3, c);
  ASSERT(4, d);
  ASSERT(5, e);
  ASSERT(6, f);
  return a + b + c + d + e + f;
}

int __attribute__((sysv_abi)) sysv_gp8(int a, int b, int c, int d, int e, int f, int g, int h) {
  ASSERT(1, a);
  ASSERT(2, b);
  ASSERT(3, c);
  ASSERT(4, d);
  ASSERT(5, e);
  ASSERT(6, f);
  ASSERT(7, g);
  ASSERT(8, h);
  return a + b + c + d + e + f + g + h;
}

double __attribute__((sysv_abi)) sysv_mixed(int a, double b, int c, double d, int e, double f, int g) {
  ASSERT(10, a);
  ASSERT(30, c);
  ASSERT(50, e);
  ASSERT(70, g);
  return a + b + c + d + e + f + g;
}

double __attribute__((sysv_abi)) sysv_fp10(double a1, double a2, double a3, double a4,
                                           double a5, double a6, double a7, double a8,
                                           double a9, double a10) {
  return a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9 + a10;
}

// ========================================================
// 3. Struct & Union Passing by Value (Ported from test_passing_structs.c / unions)
// ========================================================

typedef struct { int i; } SysV_IntStruct;
typedef struct { long l; } SysV_LongStruct;
typedef struct { long l1, l2; } SysV_Long2Struct;
typedef struct { long l1, l2, l3; } SysV_Long3Struct;

int __attribute__((sysv_abi)) sysv_pass_int_struct(SysV_IntStruct s) {
  return s.i;
}

long __attribute__((sysv_abi)) sysv_pass_long_struct(SysV_LongStruct s) {
  return s.l;
}

long __attribute__((sysv_abi)) sysv_pass_long2_struct(SysV_Long2Struct s) {
  return s.l1 + s.l2;
}

long __attribute__((sysv_abi)) sysv_pass_long3_struct(SysV_Long3Struct s) {
  return s.l1 + s.l2 + s.l3;
}

typedef union {
  int i;
  float f;
} SysV_IntFloatUnion;

typedef union {
  long l[2];
  double d[2];
} SysV_LargeUnion;

int __attribute__((sysv_abi)) sysv_pass_union1(SysV_IntFloatUnion u) {
  return u.i;
}

long __attribute__((sysv_abi)) sysv_pass_union2(SysV_LargeUnion u) {
  return u.l[0] + u.l[1];
}

typedef struct {
  int x;
  int y;
} SysVPoint;

typedef struct {
  long long a;
  long long b;
} SysVLargePair;

typedef struct {
  double d;
  int i;
} SysVMixedPair;

typedef struct {
  float a;
  float b;
  float c;
} SysVFloat3;

typedef struct {
  long long a;
  long long b;
  long long c;
} SysVBigStruct;

int __attribute__((sysv_abi)) sysv_pass_point(SysVPoint p, int extra) {
  return p.x + p.y + extra;
}

double __attribute__((sysv_abi)) sysv_pass_mixed(SysVMixedPair p, double extra) {
  return p.d + p.i + extra;
}

float __attribute__((sysv_abi)) sysv_pass_float3(SysVFloat3 p) {
  return p.a + p.b + p.c;
}

long long __attribute__((sysv_abi)) sysv_pass_big(SysVBigStruct s) {
  return s.a + s.b + s.c;
}

// ========================================================
// 4. Struct Returning (Ported from test_struct_returning.c)
// ========================================================

SysVPoint __attribute__((sysv_abi)) sysv_make_point(int x, int y) {
  SysVPoint p;
  p.x = x;
  p.y = y;
  return p;
}

SysVLargePair __attribute__((sysv_abi)) sysv_make_large_pair(long long a, long long b) {
  SysVLargePair p;
  p.a = a;
  p.b = b;
  return p;
}

SysVMixedPair __attribute__((sysv_abi)) sysv_make_mixed(double d, int i) {
  SysVMixedPair p;
  p.d = d;
  p.i = i;
  return p;
}

SysVFloat3 __attribute__((sysv_abi)) sysv_make_float3(float a, float b, float c) {
  SysVFloat3 p;
  p.a = a;
  p.b = b;
  p.c = c;
  return p;
}

SysVBigStruct __attribute__((sysv_abi)) sysv_make_big(long long a, long long b, long long c) {
  SysVBigStruct s;
  s.a = a;
  s.b = b;
  s.c = c;
  return s;
}

// Struct returning in INT
typedef struct { char m1; } SysV_S1;
typedef struct { short m1; } SysV_S2;
typedef struct { int m1; } SysV_S3;
typedef struct { long m1; } SysV_S4;
typedef struct { char m1; short s; } SysV_S6;
typedef struct { char m1; int i; } SysV_S7;
typedef struct { char m1; long l; } SysV_S8;
typedef struct { int m1[4]; } SysV_S12;

SysV_S1 __attribute__((sysv_abi)) sysv_ret_s1(void) { SysV_S1 s; s.m1 = 42; return s; }
SysV_S2 __attribute__((sysv_abi)) sysv_ret_s2(void) { SysV_S2 s; s.m1 = 42; return s; }
SysV_S3 __attribute__((sysv_abi)) sysv_ret_s3(void) { SysV_S3 s; s.m1 = 42; return s; }
SysV_S4 __attribute__((sysv_abi)) sysv_ret_s4(void) { SysV_S4 s; s.m1 = 42; return s; }
SysV_S6 __attribute__((sysv_abi)) sysv_ret_s6(void) { SysV_S6 s; s.m1 = 42; s.s = 84; return s; }
SysV_S7 __attribute__((sysv_abi)) sysv_ret_s7(void) { SysV_S7 s; s.m1 = 42; s.i = 84; return s; }
SysV_S8 __attribute__((sysv_abi)) sysv_ret_s8(void) { SysV_S8 s; s.m1 = 42; s.l = 84; return s; }
SysV_S12 __attribute__((sysv_abi)) sysv_ret_s12(void) { SysV_S12 s; s.m1[0] = 1; s.m1[1] = 2; s.m1[2] = 3; s.m1[3] = 4; return s; }

// Struct returning in SSE
typedef struct { float f; } SysV_S100;
typedef struct { double d; } SysV_S101;
typedef struct { float f1; float f2; } SysV_S102;
typedef struct { double d1; double d2; } SysV_S105;

SysV_S100 __attribute__((sysv_abi)) sysv_ret_s100(void) { SysV_S100 s; s.f = 42.0f; return s; }
SysV_S101 __attribute__((sysv_abi)) sysv_ret_s101(void) { SysV_S101 s; s.d = 42.0; return s; }
SysV_S102 __attribute__((sysv_abi)) sysv_ret_s102(void) { SysV_S102 s; s.f1 = 42.0f; s.f2 = 84.0f; return s; }
SysV_S105 __attribute__((sysv_abi)) sysv_ret_s105(void) { SysV_S105 s; s.d1 = 42.0; s.d2 = 84.0; return s; }

// Struct returning in INT + SSE
typedef struct { int m1; float m2; } SysV_S304;
typedef struct { long m1; double m2; } SysV_S307;

SysV_S304 __attribute__((sysv_abi)) sysv_ret_s304(void) { SysV_S304 s; s.m1 = 42; s.m2 = 43.0f; return s; }
SysV_S307 __attribute__((sysv_abi)) sysv_ret_s307(void) { SysV_S307 s; s.m1 = 42; s.m2 = 43.0; return s; }

// ========================================================
// 5. Variadic Functions (Ported from test_varargs.c)
// ========================================================

int __attribute__((sysv_abi)) sysv_sum_varargs(int count, ...) {
  va_list ap;
  va_start(ap, count);
  int sum = 0;
  for (int i = 0; i < count; i++) {
    sum += va_arg(ap, int);
  }
  va_end(ap);
  return sum;
}

double __attribute__((sysv_abi)) sysv_sum_fp_varargs(int count, ...) {
  va_list ap;
  va_start(ap, count);
  double sum = 0;
  for (int i = 0; i < count; i++) {
    sum += va_arg(ap, double);
  }
  va_end(ap);
  return sum;
}

// ========================================================
// 6. Function Pointer Calls
// ========================================================

typedef int (__attribute__((sysv_abi)) *SysVFuncPtr)(int, int, int, int, int, int);

// ========================================================
// Main SysV ABI Test Runner
// ========================================================

int test_sysv_abi(void) {
  printf("Testing System V AMD64 ABI...\n");

  // 1. Basic type sizes and alignment
  test_sysv_sizes_and_alignments();

  // 2. Scalar arguments
  ASSERT(21, sysv_gp6(1, 2, 3, 4, 5, 6));
  ASSERT(36, sysv_gp8(1, 2, 3, 4, 5, 6, 7, 8));
  double m = sysv_mixed(10, 20.0, 30, 40.0, 50, 60.0, 70);
  ASSERT(280, (int)m);
  double fptot = sysv_fp10(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0);
  ASSERT(55, (int)fptot);

  // 3. Struct & Union passing
  SysV_IntStruct is = { 48 };
  ASSERT(48, sysv_pass_int_struct(is));
  SysV_LongStruct ls = { 49 };
  ASSERT(49, sysv_pass_long_struct(ls));
  SysV_Long2Struct l2s = { 50, 51 };
  ASSERT(101, sysv_pass_long2_struct(l2s));
  SysV_Long3Struct l3s = { 52, 53, 54 };
  ASSERT(159, sysv_pass_long3_struct(l3s));

  SysV_IntFloatUnion u1;
  u1.i = 12345;
  ASSERT(12345, sysv_pass_union1(u1));
  SysV_LargeUnion u2;
  u2.l[0] = 1000;
  u2.l[1] = 2000;
  ASSERT(3000, sysv_pass_union2(u2));

  SysVPoint pt = sysv_make_point(123, 456);
  ASSERT(123, pt.x);
  ASSERT(456, pt.y);
  ASSERT(589, sysv_pass_point(pt, 10));

  SysVLargePair lp = sysv_make_large_pair(1000000000LL, 2000000000LL);
  ASSERT(1000000000LL, lp.a);
  ASSERT(2000000000LL, lp.b);

  SysVMixedPair mp = sysv_make_mixed(12.5, 30);
  ASSERT(12, (int)mp.d);
  ASSERT(30, mp.i);
  ASSERT(52, (int)sysv_pass_mixed(mp, 10.0));

  SysVFloat3 f3 = sysv_make_float3(1.0f, 2.0f, 3.0f);
  ASSERT(1, (int)f3.a);
  ASSERT(2, (int)f3.b);
  ASSERT(3, (int)f3.c);
  ASSERT(6, (int)sysv_pass_float3(f3));

  SysVBigStruct big = sysv_make_big(100, 200, 300);
  ASSERT(100, big.a);
  ASSERT(200, big.b);
  ASSERT(300, big.c);
  ASSERT(600, (int)sysv_pass_big(big));

  // 4. Struct returning
  ASSERT(42, sysv_ret_s1().m1);
  ASSERT(42, sysv_ret_s2().m1);
  ASSERT(42, sysv_ret_s3().m1);
  ASSERT(42, sysv_ret_s4().m1);
  SysV_S6 s6 = sysv_ret_s6();
  ASSERT(42, s6.m1);
  ASSERT(84, s6.s);
  SysV_S7 s7 = sysv_ret_s7();
  ASSERT(42, s7.m1);
  ASSERT(84, s7.i);
  SysV_S8 s8 = sysv_ret_s8();
  ASSERT(42, s8.m1);
  ASSERT(84, s8.l);
  SysV_S12 s12 = sysv_ret_s12();
  ASSERT(1, s12.m1[0]);
  ASSERT(2, s12.m1[1]);
  ASSERT(3, s12.m1[2]);
  ASSERT(4, s12.m1[3]);

  ASSERT(42, (int)sysv_ret_s100().f);
  ASSERT(42, (int)sysv_ret_s101().d);
  SysV_S102 s102 = sysv_ret_s102();
  ASSERT(42, (int)s102.f1);
  ASSERT(84, (int)s102.f2);
  SysV_S105 s105 = sysv_ret_s105();
  ASSERT(42, (int)s105.d1);
  ASSERT(84, (int)s105.d2);

  SysV_S304 s304 = sysv_ret_s304();
  ASSERT(42, s304.m1);
  ASSERT(43, (int)s304.m2);
  SysV_S307 s307 = sysv_ret_s307();
  ASSERT(42, s307.m1);
  ASSERT(43, (int)s307.m2);

  // 5. Variadic arguments
#ifndef _WIN32
  ASSERT(150, sysv_sum_varargs(5, 10, 20, 30, 40, 50));
  ASSERT(45, (int)sysv_sum_fp_varargs(5, 1.0, 2.0, 3.0, 4.0, 35.0));
#endif

  // 6. Function pointer calls
  SysVFuncPtr fptr = sysv_gp6;
  ASSERT(21, fptr(1, 2, 3, 4, 5, 6));

  printf("System V AMD64 ABI tests passed successfully!\n");
  return 0;
}

#ifndef RUN_ALL_ABI_TESTS
int main(void) {
  return test_sysv_abi();
}
#endif
