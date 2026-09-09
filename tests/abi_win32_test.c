#include "tests/test.h"

// Windows 32-bit (x86 cdecl) ABI Test Suite

// ========================================================
// 1. Basic Type Sizes and Alignment Tests for Win32
// ========================================================

typedef struct { char dummy; } Win32_S_Char;
typedef struct { short dummy; } Win32_S_Short;
typedef struct { int dummy; } Win32_S_Int;
typedef struct { long dummy; } Win32_S_Long;
typedef struct { long long dummy; } Win32_S_LongLong;
typedef struct { float dummy; } Win32_S_Float;
typedef struct { double dummy; } Win32_S_Double;

typedef struct { char c; short s; int i; } Win32_S_C_S_I;
typedef struct { int i; double d; char c; } Win32_S_I_D_C;

static void test_win32_sizes_and_alignments(void) {
  ASSERT(1, sizeof(char));
  ASSERT(2, sizeof(short));
  ASSERT(4, sizeof(int));
  ASSERT(4, sizeof(long));
  ASSERT(8, sizeof(long long));
  ASSERT(4, sizeof(float));
  ASSERT(8, sizeof(double));

  ASSERT(1, _Alignof(char));
  ASSERT(2, _Alignof(short));
  ASSERT(4, _Alignof(int));
  ASSERT(4, _Alignof(long));
  ASSERT(8, _Alignof(long long));
  ASSERT(4, _Alignof(float));
  ASSERT(8, _Alignof(double));

  ASSERT(8, sizeof(Win32_S_C_S_I));
  ASSERT(4, _Alignof(Win32_S_C_S_I));
  ASSERT(24, sizeof(Win32_S_I_D_C));
  ASSERT(8, _Alignof(Win32_S_I_D_C));
}

// ========================================================
// 2. Stack-passed arguments
// ========================================================

int __attribute__((cdecl)) win32_cdecl5(int a, int b, int c, int d, int e) {
  ASSERT(1, a);
  ASSERT(2, b);
  ASSERT(3, c);
  ASSERT(4, d);
  ASSERT(5, e);
  return a + b + c + d + e;
}

// ========================================================
// 3. 64-bit integer return value (EDX:EAX / RAX)
// ========================================================

long long __attribute__((cdecl)) win32_make_llong(int high, int low) {
  long long v = ((long long)high << 32) | (unsigned int)low;
  return v;
}

// ========================================================
// 4. Small struct return (<= 4 bytes in EAX)
// ========================================================

typedef struct {
  short x;
  short y;
} Win32SmallPoint;

Win32SmallPoint __attribute__((cdecl)) win32_make_point(short x, short y) {
  Win32SmallPoint p;
  p.x = x;
  p.y = y;
  return p;
}

// ========================================================
// 5. Large struct return (> 4 bytes via memory buffer)
// ========================================================

typedef struct {
  int a;
  int b;
  int c;
} Win32LargeStruct;

Win32LargeStruct __attribute__((cdecl)) win32_make_large(int a, int b, int c) {
  Win32LargeStruct s;
  s.a = a;
  s.b = b;
  s.c = c;
  return s;
}

// ========================================================
// 6. Variadic function under Win32 cdecl
// ========================================================

int __attribute__((cdecl)) win32_sum_varargs(int count, ...) {
  va_list ap;
  va_start(ap, count);
  int sum = 0;
  for (int i = 0; i < count; i++) {
    sum += va_arg(ap, int);
  }
  va_end(ap);
  return sum;
}

// ========================================================
// 7. Function pointer with cdecl calling convention
// ========================================================

typedef int (__attribute__((cdecl)) *Win32FuncPtr)(int, int, int, int, int);

// ========================================================
// Main Win32 ABI Test Runner
// ========================================================

int test_win32_abi(void) {
  printf("Testing Microsoft Windows x86 (Win32 cdecl) ABI...\n");

  // 1. Basic type sizes and alignment
  test_win32_sizes_and_alignments();

  // 2. Test stack arguments
  ASSERT(15, win32_cdecl5(1, 2, 3, 4, 5));

  // 3. Test 64-bit return value
  long long v = win32_make_llong(0x12345678, 0x9abcdef0);
  ASSERT(0x123456789abcdef0LL, v);

  // 4. Test small struct return
  Win32SmallPoint pt = win32_make_point(11, 22);
  ASSERT(11, pt.x);
  ASSERT(22, pt.y);

  // 5. Test large struct return
  Win32LargeStruct ls = win32_make_large(10, 20, 30);
  ASSERT(10, ls.a);
  ASSERT(20, ls.b);
  ASSERT(30, ls.c);

  // 6. Test variadic function
  ASSERT(100, win32_sum_varargs(4, 10, 20, 30, 40));

  // 7. Test function pointer call
  Win32FuncPtr fptr = win32_cdecl5;
  ASSERT(15, fptr(1, 2, 3, 4, 5));

  printf("Windows x86 (Win32 cdecl) ABI tests passed successfully!\n");
  return 0;
}

#ifndef RUN_ALL_ABI_TESTS
int main(void) {
  return test_win32_abi();
}
#endif
