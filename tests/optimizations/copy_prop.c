#include "test.h"

static int opaque_val(int x) {
  return x;
}

static int add_three(int a, int b, int c) {
  return a + b + c;
}

// 1. Basic Single-Step Copy Propagation
static int test_basic_copy_prop(int x) {
  int a = x;
  int b = a;
  int c = b;
  ASSERT(x, c);

  int d = a + 5;
  int e = b + 10;
  ASSERT(x + 5, d);
  ASSERT(x + 10, e);

  return 0;
}

// 2. Long Multi-Hop Transitive Copy Chains
static int test_long_copy_chain(int val) {
  int v1 = val;
  int v2 = v1;
  int v3 = v2;
  int v4 = v3;
  int v5 = v4;
  int v6 = v5;
  int v7 = v6;
  int v8 = v7;
  int v9 = v8;
  int v10 = v9;

  ASSERT(val, v10);
  ASSERT(val * 10, v1 + v2 + v3 + v4 + v5 + v6 + v7 + v8 + v9 + v10);

  return 0;
}

// 3. Copy Propagation into Function Arguments
static int test_call_copy_prop(int x, int y, int z) {
  int c1 = x;
  int c2 = y;
  int c3 = z;

  int res = add_three(c1, c2, c3);
  ASSERT(x + y + z, res);

  int d1 = c1;
  int d2 = c2;
  int d3 = c3;
  int res2 = add_three(d1, d2, d3);
  ASSERT(x + y + z, res2);

  return 0;
}

// 4. Multiple Aliases Interleaved with Arithmetic
static int test_interleaved_copies(int base) {
  int a = base;
  int b = a;
  int c = b + 10;
  int d = c;
  int e = d;
  int f = e * 2;
  int g = f;

  ASSERT((base + 10) * 2, g);
  ASSERT(base, a);
  ASSERT(base, b);
  ASSERT(base + 10, c);
  ASSERT(base + 10, d);
  ASSERT(base + 10, e);

  return 0;
}

// 5. Pointer and Struct Copy Propagation
typedef struct {
  int x;
  int y;
  int z;
} Point3D;

static int test_pointer_struct_copies(void) {
  Point3D pt = { 11, 22, 33 };
  Point3D *p1 = &pt;
  Point3D *p2 = p1;
  Point3D *p3 = p2;

  ASSERT(11, p3->x);
  ASSERT(22, p3->y);
  ASSERT(33, p3->z);

  Point3D pt2 = pt;
  Point3D pt3 = pt2;
  ASSERT(11, pt3.x);
  ASSERT(22, pt3.y);
  ASSERT(33, pt3.z);

  return 0;
}

int main(void) {
  int x = opaque_val(42);
  int y = opaque_val(100);
  int z = opaque_val(250);

  test_basic_copy_prop(x);
  test_long_copy_chain(x);
  test_call_copy_prop(x, y, z);
  test_interleaved_copies(x);
  test_pointer_struct_copies();

  printf("OK\n");
  return 0;
}
