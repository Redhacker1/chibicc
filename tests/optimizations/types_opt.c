#include "test.h"

static int opaque_val(int x) {
  return x;
}

// 1. Character & Short Types
static int test_small_int_types(void) {
  char c1 = (char)opaque_val(10);
  char c2 = (char)opaque_val(20);
  char c3 = c1 + c2;
  ASSERT(30, (int)c3);

  short s1 = (short)opaque_val(1000);
  short s2 = (short)opaque_val(2000);
  short s3 = s1 + s2;
  ASSERT(3000, (int)s3);

  return 0;
}

// 2. Unsigned Integer Types & Bitwise Ops
static int test_unsigned_types(void) {
  unsigned int u1 = 0x80000000U;
  unsigned int u2 = u1 >> 1;
  ASSERT(0x40000000U, u2);

  unsigned int u3 = 0xFFFFFFFFU;
  unsigned int u4 = u3 / 2U;
  ASSERT(0x7FFFFFFFU, u4);

  return 0;
}

// 3. Floating Point Types
static int test_floating_types(void) {
  float f1 = 1.5f;
  float f2 = 2.5f;
  float f3 = f1 + f2;
  ASSERT(1, f3 == 4.0f);

  double d1 = 10.25;
  double d2 = 20.75;
  double d3 = d1 + d2;
  ASSERT(1, d3 == 31.0);

  return 0;
}

// 4. Pointer Arithmetic
static int test_pointer_arithmetic(void) {
  int arr[5] = { 10, 20, 30, 40, 50 };
  int *p = arr;
  int *p2 = p + 3;
  ASSERT(40, *p2);
  ASSERT(3, p2 - p);

  return 0;
}

// 5. Structs & Compound Types
typedef struct {
  char a;
  short b;
  int c;
  long long d;
} MixedStruct;

static int test_mixed_struct(void) {
  MixedStruct s;
  s.a = 5;
  s.b = 15;
  s.c = 25;
  s.d = 35;

  MixedStruct s2 = s;
  ASSERT(5, s2.a);
  ASSERT(15, s2.b);
  ASSERT(25, s2.c);
  ASSERT(1, s2.d == 35LL);

  return 0;
}

int main(void) {
  test_small_int_types();
  test_unsigned_types();
  test_floating_types();
  test_pointer_arithmetic();
  test_mixed_struct();

  printf("OK\n");
  return 0;
}
