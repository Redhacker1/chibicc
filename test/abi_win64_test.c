#include "test/test.h"

// Comprehensive Microsoft Windows x64 ABI Test Suite

// ========================================================
// 1. Basic Type Sizes and Alignment Tests for Win64
// ========================================================

typedef struct { char dummy; } Win64_S_Char;
typedef struct { short dummy; } Win64_S_Short;
typedef struct { int dummy; } Win64_S_Int;
typedef struct { long dummy; } Win64_S_Long;
typedef struct { long long dummy; } Win64_S_LongLong;
typedef struct { float dummy; } Win64_S_Float;
typedef struct { double dummy; } Win64_S_Double;
typedef struct { void *dummy; } Win64_S_Pointer;

typedef union { char dummy; } Win64_U_Char;
typedef union { short dummy; } Win64_U_Short;
typedef union { int dummy; } Win64_U_Int;
typedef union { long dummy; } Win64_U_Long;
typedef union { long long dummy; } Win64_U_LongLong;
typedef union { float dummy; } Win64_U_Float;
typedef union { double dummy; } Win64_U_Double;
typedef union { void *dummy; } Win64_U_Pointer;

typedef struct { char c; short s; int i; } Win64_S_C_S_I;
typedef struct { int i; double d; char c; } Win64_S_I_D_C;
typedef union { int i; double d; char c; } Win64_U_I_D_C;

typedef struct { char a, b, c; } Win64_S3;
typedef struct { char a, b, c, d, e; } Win64_S5;
typedef struct { short a, b, c; } Win64_S6;
typedef struct { char a[7]; } Win64_S7;
typedef struct { long long a, b; } Win64_S16;
typedef struct { long long a, b, c; } Win64_S24;

static void test_win64_sizes_and_alignments(void) {
  // In Win64 (LLP64), long is 32-bit (4 bytes), pointer is 64-bit (8 bytes)
  ASSERT(1, sizeof(char));
  ASSERT(2, sizeof(short));
  ASSERT(4, sizeof(int));
  ASSERT(4, sizeof(long));
  ASSERT(8, sizeof(long long));
  ASSERT(4, sizeof(float));
  ASSERT(8, sizeof(double));
  ASSERT(8, sizeof(void *));

  ASSERT(1, _Alignof(char));
  ASSERT(2, _Alignof(short));
  ASSERT(4, _Alignof(int));
  ASSERT(4, _Alignof(long));
  ASSERT(8, _Alignof(long long));
  ASSERT(4, _Alignof(float));
  ASSERT(8, _Alignof(double));
  ASSERT(8, _Alignof(void *));

  // Struct sizes & alignments
  ASSERT(1, sizeof(Win64_S_Char));
  ASSERT(1, _Alignof(Win64_S_Char));
  ASSERT(2, sizeof(Win64_S_Short));
  ASSERT(2, _Alignof(Win64_S_Short));
  ASSERT(4, sizeof(Win64_S_Int));
  ASSERT(4, _Alignof(Win64_S_Int));
  ASSERT(4, sizeof(Win64_S_Long));
  ASSERT(4, _Alignof(Win64_S_Long));
  ASSERT(8, sizeof(Win64_S_LongLong));
  ASSERT(8, _Alignof(Win64_S_LongLong));
  ASSERT(4, sizeof(Win64_S_Float));
  ASSERT(4, _Alignof(Win64_S_Float));
  ASSERT(8, sizeof(Win64_S_Double));
  ASSERT(8, _Alignof(Win64_S_Double));
  ASSERT(8, sizeof(Win64_S_Pointer));
  ASSERT(8, _Alignof(Win64_S_Pointer));

  // Union sizes & alignments
  ASSERT(1, sizeof(Win64_U_Char));
  ASSERT(2, sizeof(Win64_U_Short));
  ASSERT(4, sizeof(Win64_U_Int));
  ASSERT(4, sizeof(Win64_U_Long));
  ASSERT(8, sizeof(Win64_U_LongLong));
  ASSERT(4, sizeof(Win64_U_Float));
  ASSERT(8, sizeof(Win64_U_Double));
  ASSERT(8, sizeof(Win64_U_Pointer));

  // Compound structs & unions
  ASSERT(8, sizeof(Win64_S_C_S_I));
  ASSERT(4, _Alignof(Win64_S_C_S_I));
  ASSERT(24, sizeof(Win64_S_I_D_C));
  ASSERT(8, _Alignof(Win64_S_I_D_C));
  ASSERT(8, sizeof(Win64_U_I_D_C));
  ASSERT(8, _Alignof(Win64_U_I_D_C));

  // Irregular size structs
  ASSERT(3, sizeof(Win64_S3));
  ASSERT(1, _Alignof(Win64_S3));
  ASSERT(5, sizeof(Win64_S5));
  ASSERT(1, _Alignof(Win64_S5));
  ASSERT(6, sizeof(Win64_S6));
  ASSERT(2, _Alignof(Win64_S6));
  ASSERT(7, sizeof(Win64_S7));
  ASSERT(1, _Alignof(Win64_S7));
  ASSERT(16, sizeof(Win64_S16));
  ASSERT(8, _Alignof(Win64_S16));
  ASSERT(24, sizeof(Win64_S24));
  ASSERT(8, _Alignof(Win64_S24));
}

// ========================================================
// 2. Scalar and Floating-Point Argument Passing
// ========================================================

int __attribute__((ms_abi)) win64_gp4(int a, int b, int c, int d) {
  ASSERT(10, a);
  ASSERT(20, b);
  ASSERT(30, c);
  ASSERT(40, d);
  return a + b + c + d;
}

int __attribute__((ms_abi)) win64_gp8(int a, int b, int c, int d, int e, int f, int g, int h) {
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

float __attribute__((ms_abi)) win64_fp4(float a, float b, float c, float d) {
  ASSERT(1, (int)a);
  ASSERT(2, (int)b);
  ASSERT(3, (int)c);
  ASSERT(4, (int)d);
  return a + b + c + d;
}

double __attribute__((ms_abi)) win64_double8(double a, double b, double c, double d,
                                              double e, double f, double g, double h) {
  ASSERT(1, (int)a);
  ASSERT(2, (int)b);
  ASSERT(3, (int)c);
  ASSERT(4, (int)d);
  ASSERT(5, (int)e);
  ASSERT(6, (int)f);
  ASSERT(7, (int)g);
  ASSERT(8, (int)h);
  return a + b + c + d + e + f + g + h;
}

double __attribute__((ms_abi)) win64_mixed(int a, double b, int c, double d, int e) {
  ASSERT(10, a);
  ASSERT(30, c);
  ASSERT(50, e);
  return a + b + c + d + e;
}

double __attribute__((ms_abi)) win64_mixed_10(int a, float b, double c, void *d,
                                              int e, float f, double g, void *h,
                                              int i, int j) {
  ASSERT(1, a);
  ASSERT(2, (int)b);
  ASSERT(3, (int)c);
  ASSERT(4, (int)(long long)d);
  ASSERT(5, e);
  ASSERT(6, (int)f);
  ASSERT(7, (int)g);
  ASSERT(8, (int)(long long)h);
  ASSERT(9, i);
  ASSERT(10, j);
  return a + b + c + (long long)d + e + f + g + (long long)h + i + j;
}

// ========================================================
// 3. Small Struct (1, 2, 4, 8 bytes) Pass & Return in Registers
// ========================================================

typedef struct { char c; } Win64_S1;
typedef struct { short x, y; } Win64SmallPoint;
typedef struct { char a, b, c, d; } Win64Char4;
typedef struct { int x, y; } Win64IntPair;
typedef struct { long long v; } Win64_S8;

typedef union { char c; } Win64_U1;
typedef union { short s; } Win64_U2;
typedef union { int i; } Win64_U4;
typedef union { long long l; } Win64_U8;

Win64_S1 __attribute__((ms_abi)) win64_make_s1(char c) {
  Win64_S1 s;
  s.c = c;
  return s;
}

int __attribute__((ms_abi)) win64_pass_s1(Win64_S1 s) {
  return s.c;
}

Win64SmallPoint __attribute__((ms_abi)) win64_make_small_point(short x, short y) {
  Win64SmallPoint p;
  p.x = x;
  p.y = y;
  return p;
}

short __attribute__((ms_abi)) win64_pass_small_point(Win64SmallPoint p) {
  ASSERT(15, p.x);
  ASSERT(25, p.y);
  return p.x + p.y;
}

Win64Char4 __attribute__((ms_abi)) win64_make_char4(char a, char b, char c, char d) {
  Win64Char4 r;
  r.a = a; r.b = b; r.c = c; r.d = d;
  return r;
}

int __attribute__((ms_abi)) win64_pass_char4(Win64Char4 r) {
  return r.a + r.b + r.c + r.d;
}

Win64IntPair __attribute__((ms_abi)) win64_make_int_pair(int x, int y) {
  Win64IntPair r;
  r.x = x; r.y = y;
  return r;
}

int __attribute__((ms_abi)) win64_pass_int_pair(Win64IntPair r) {
  return r.x + r.y;
}

Win64_S8 __attribute__((ms_abi)) win64_make_s8(long long v) {
  Win64_S8 s;
  s.v = v;
  return s;
}

long long __attribute__((ms_abi)) win64_pass_s8(Win64_S8 s) {
  return s.v;
}

Win64_U1 __attribute__((ms_abi)) win64_make_u1(char c) {
  Win64_U1 u;
  u.c = c;
  return u;
}

Win64_U2 __attribute__((ms_abi)) win64_make_u2(short s) {
  Win64_U2 u;
  u.s = s;
  return u;
}

Win64_U4 __attribute__((ms_abi)) win64_make_u4(int i) {
  Win64_U4 u;
  u.i = i;
  return u;
}

Win64_U8 __attribute__((ms_abi)) win64_make_u8(long long l) {
  Win64_U8 u;
  u.l = l;
  return u;
}

int __attribute__((ms_abi)) win64_pass_multiple_small_structs(
    Win64_S1 s1, Win64SmallPoint sp, Win64Char4 c4, Win64IntPair ip, Win64_S8 s8) {
  return s1.c + sp.x + sp.y + c4.a + c4.b + c4.c + c4.d + ip.x + ip.y + (int)s8.v;
}

// ========================================================
// 4. Non-power-of-2 and Large Structs (> 8 bytes)
//    (Passed by Reference, Returned via Memory Pointer in RCX/RAX)
// ========================================================

Win64_S3 __attribute__((ms_abi)) win64_make_s3(char a, char b, char c) {
  Win64_S3 s;
  s.a = a; s.b = b; s.c = c;
  return s;
}

int __attribute__((ms_abi)) win64_pass_s3(Win64_S3 s) {
  return s.a + s.b + s.c;
}

Win64_S5 __attribute__((ms_abi)) win64_make_s5(char a, char b, char c, char d, char e) {
  Win64_S5 s;
  s.a = a; s.b = b; s.c = c; s.d = d; s.e = e;
  return s;
}

int __attribute__((ms_abi)) win64_pass_s5(Win64_S5 s) {
  return s.a + s.b + s.c + s.d + s.e;
}

Win64_S6 __attribute__((ms_abi)) win64_make_s6(short a, short b, short c) {
  Win64_S6 s;
  s.a = a; s.b = b; s.c = c;
  return s;
}

int __attribute__((ms_abi)) win64_pass_s6(Win64_S6 s) {
  return s.a + s.b + s.c;
}

Win64_S7 __attribute__((ms_abi)) win64_make_s7(char a, char b, char c, char d, char e, char f, char g) {
  Win64_S7 s;
  s.a[0] = a; s.a[1] = b; s.a[2] = c; s.a[3] = d; s.a[4] = e; s.a[5] = f; s.a[6] = g;
  return s;
}

int __attribute__((ms_abi)) win64_pass_s7(Win64_S7 s) {
  return s.a[0] + s.a[1] + s.a[2] + s.a[3] + s.a[4] + s.a[5] + s.a[6];
}

Win64_S16 __attribute__((ms_abi)) win64_make_s16(long long a, long long b) {
  Win64_S16 s;
  s.a = a; s.b = b;
  return s;
}

long long __attribute__((ms_abi)) win64_pass_s16(Win64_S16 s) {
  return s.a + s.b;
}

Win64_S24 __attribute__((ms_abi)) win64_make_s24(long long a, long long b, long long c) {
  Win64_S24 s;
  s.a = a; s.b = b; s.c = c;
  return s;
}

long long __attribute__((ms_abi)) win64_pass_s24(Win64_S24 s) {
  ASSERT(100, (int)s.a);
  ASSERT(200, (int)s.b);
  ASSERT(300, (int)s.c);
  return s.a + s.b + s.c;
}

long long __attribute__((ms_abi)) win64_pass_mixed_large_structs(
    int a, Win64_S24 s1, double b, Win64_S16 s2, int c, Win64_S24 s3) {
  ASSERT(10, a);
  ASSERT(20, (int)b);
  ASSERT(30, c);
  return a + s1.a + s1.b + s1.c + (long long)b + s2.a + s2.b + c + s3.a + s3.b + s3.c;
}

// ========================================================
// 5. Variadic Functions under Windows x64 ABI
// ========================================================

int __attribute__((ms_abi)) win64_sum_int_varargs(int count, ...) {
  va_list ap;
  va_start(ap, count);
  int sum = 0;
  for (int i = 0; i < count; i++) {
    sum += va_arg(ap, int);
  }
  va_end(ap);
  return sum;
}

double __attribute__((ms_abi)) win64_sum_double_varargs(int count, ...) {
  va_list ap;
  va_start(ap, count);
  double sum = 0;
  for (int i = 0; i < count; i++) {
    sum += va_arg(ap, double);
  }
  va_end(ap);
  return sum;
}

long long __attribute__((ms_abi)) win64_sum_mixed_varargs(int count, ...) {
  va_list ap;
  va_start(ap, count);
  long long sum = 0;
  // expects alternating int and double
  for (int i = 0; i < count; i++) {
    if (i % 2 == 0) {
      sum += va_arg(ap, int);
    } else {
      sum += (long long)va_arg(ap, double);
    }
  }
  va_end(ap);
  return sum;
}

// ========================================================
// 6. Function Pointers and Nested Calls
// ========================================================

typedef int (__attribute__((ms_abi)) *Win64FuncPtr4)(int, int, int, int);
typedef int (__attribute__((ms_abi)) *Win64FuncPtr8)(int, int, int, int, int, int, int, int);

int __attribute__((ms_abi)) win64_fibonacci(int n) {
  if (n <= 1)
    return n;
  return win64_fibonacci(n - 1) + win64_fibonacci(n - 2);
}

// ========================================================
// Main Win64 ABI Test Runner
// ========================================================

int test_win64_abi(void) {
  printf("Testing Microsoft Windows x64 ABI...\n");

  // 1. Basic type sizes and alignment
  test_win64_sizes_and_alignments();

  // 2. Scalar and Floating-Point Arguments
  ASSERT(100, win64_gp4(10, 20, 30, 40));
  ASSERT(36, win64_gp8(1, 2, 3, 4, 5, 6, 7, 8));
  ASSERT(10, (int)win64_fp4(1.0f, 2.0f, 3.0f, 4.0f));
  ASSERT(36, (int)win64_double8(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0));
  ASSERT(150, (int)win64_mixed(10, 20.0, 30, 40.0, 50));
  ASSERT(55, (int)win64_mixed_10(1, 2.0f, 3.0, (void *)4, 5, 6.0f, 7.0, (void *)8, 9, 10));

  // 3. Small Struct & Union Return and Passing
  Win64_S1 s1 = win64_make_s1(42);
  ASSERT(42, s1.c);
  ASSERT(42, win64_pass_s1(s1));

  Win64SmallPoint sp = win64_make_small_point(15, 25);
  ASSERT(15, sp.x);
  ASSERT(25, sp.y);
  ASSERT(40, win64_pass_small_point(sp));

  Win64Char4 c4 = win64_make_char4(1, 2, 3, 4);
  ASSERT(1, c4.a);
  ASSERT(2, c4.b);
  ASSERT(3, c4.c);
  ASSERT(4, c4.d);
  ASSERT(10, win64_pass_char4(c4));

  Win64IntPair ip = win64_make_int_pair(50, 60);
  ASSERT(50, ip.x);
  ASSERT(60, ip.y);
  ASSERT(110, win64_pass_int_pair(ip));

  Win64_S8 s8 = win64_make_s8(1000);
  ASSERT(1000, (int)s8.v);
  ASSERT(1000, (int)win64_pass_s8(s8));

  Win64_U1 u1 = win64_make_u1(7);
  ASSERT(7, u1.c);
  Win64_U2 u2 = win64_make_u2(77);
  ASSERT(77, u2.s);
  Win64_U4 u4 = win64_make_u4(777);
  ASSERT(777, u4.i);
  Win64_U8 u8 = win64_make_u8(7777);
  ASSERT(7777, (int)u8.l);

  ASSERT(1202, win64_pass_multiple_small_structs(s1, sp, c4, ip, s8));

  // 4. Non-power-of-2 and Large Structs (Pass-by-ref, Sret)
  Win64_S3 s3 = win64_make_s3(1, 2, 3);
  ASSERT(1, s3.a);
  ASSERT(2, s3.b);
  ASSERT(3, s3.c);
  ASSERT(6, win64_pass_s3(s3));

  Win64_S5 s5 = win64_make_s5(1, 2, 3, 4, 5);
  ASSERT(1, s5.a);
  ASSERT(5, s5.e);
  ASSERT(15, win64_pass_s5(s5));

  Win64_S6 s6 = win64_make_s6(10, 20, 30);
  ASSERT(10, s6.a);
  ASSERT(20, s6.b);
  ASSERT(30, s6.c);
  ASSERT(60, win64_pass_s6(s6));

  Win64_S7 s7 = win64_make_s7(1, 2, 3, 4, 5, 6, 7);
  ASSERT(1, s7.a[0]);
  ASSERT(7, s7.a[6]);
  ASSERT(28, win64_pass_s7(s7));

  Win64_S16 s16 = win64_make_s16(500, 600);
  ASSERT(500, (int)s16.a);
  ASSERT(600, (int)s16.b);
  ASSERT(1100, (int)win64_pass_s16(s16));

  Win64_S24 s24 = win64_make_s24(100, 200, 300);
  ASSERT(100, (int)s24.a);
  ASSERT(200, (int)s24.b);
  ASSERT(300, (int)s24.c);
  ASSERT(600, (int)win64_pass_s24(s24));

  Win64_S24 s24_1 = win64_make_s24(1, 2, 3);
  Win64_S16 s16_1 = win64_make_s16(4, 5);
  Win64_S24 s24_2 = win64_make_s24(6, 7, 8);
  long long mixed_struct_total = win64_pass_mixed_large_structs(10, s24_1, 20.0, s16_1, 30, s24_2);
  ASSERT(96, (int)mixed_struct_total);

  // 5. Variadic Functions
  ASSERT(60, win64_sum_int_varargs(3, 10, 20, 30));
  ASSERT(150, win64_sum_int_varargs(5, 10, 20, 30, 40, 50));
  ASSERT(45, (int)win64_sum_double_varargs(5, 1.0, 2.0, 3.0, 4.0, 35.0));
  ASSERT(100, (int)win64_sum_mixed_varargs(4, 10, 20.0, 30, 40.0));
  ASSERT(210, (int)win64_sum_mixed_varargs(6, 10, 20.0, 30, 40.0, 50, 60.0));

  // 6. Function Pointers & Nested Calls
  Win64FuncPtr4 fptr4 = win64_gp4;
  ASSERT(100, fptr4(10, 20, 30, 40));

  Win64FuncPtr8 fptr8 = win64_gp8;
  ASSERT(36, fptr8(1, 2, 3, 4, 5, 6, 7, 8));

  ASSERT(55, win64_fibonacci(10));

  printf("Windows x64 ABI tests passed successfully!\n");
  return 0;
}

#ifndef RUN_ALL_ABI_TESTS
int main(void) {
  return test_win64_abi();
}
#endif
