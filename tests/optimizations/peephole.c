#include "test.h"

static int opaque_val(int x) {
  return x;
}

// 1. Strength Reduction: Multiplication by Powers of 2
static int test_strength_reduction_pow2(int x) {
  ASSERT(x * 2, x * 2);
  ASSERT(x * 4, x * 4);
  ASSERT(x * 8, x * 8);
  ASSERT(x * 16, x * 16);
  ASSERT(x * 32, x * 32);
  ASSERT(x * 64, x * 64);
  ASSERT(x * 128, x * 128);
  ASSERT(x * 256, x * 256);
  ASSERT(x * 512, x * 512);
  ASSERT(x * 1024, x * 1024);
  ASSERT(x * 2048, x * 2048);
  ASSERT(x * 4096, x * 4096);
  ASSERT(x * 8192, x * 8192);
  ASSERT(x * 16384, x * 16384);
  ASSERT(x * 32768, x * 32768);
  ASSERT(x * 65536, x * 65536);

  // Shifts equivalent to multiplications
  ASSERT(x << 1, x * 2);
  ASSERT(x << 3, x * 8);
  ASSERT(x << 10, x * 1024);
  ASSERT(x << 16, x * 65536);

  return 0;
}

// 2. Self-Cancellation (sub x, x -> 0, xor x, x -> 0)
static int test_self_cancellation(int x, int y) {
  int r1 = x - x;
  ASSERT(0, r1);

  int r2 = y - y;
  ASSERT(0, r2);

  int r3 = x ^ x;
  ASSERT(0, r3);

  int r4 = y ^ y;
  ASSERT(0, r4);

  int r5 = (x + y) - (x + y);
  ASSERT(0, r5);

  return 0;
}

// 3. Redundant Load-After-Store Elimination
static int test_load_after_store(int val) {
  int slot = 0;
  int *p = &slot;

  *p = val;
  int read_back = *p;
  ASSERT(val, read_back);

  *p = val * 3;
  int read_back2 = *p;
  ASSERT(val * 3, read_back2);

  return 0;
}

// 4. Memory Barrier Safety (Intervening Store or Call Must Not Allow Stale Load)
static int g_val = 0;
static void write_g_val(int v) {
  g_val = v;
}

static int test_load_after_store_barrier(void) {
  g_val = 100;
  write_g_val(200); // Call barrier
  ASSERT(200, g_val);

  int arr[2] = { 10, 20 };
  int *p0 = &arr[0];
  int *p1 = &arr[1];

  *p0 = 50;
  *p1 = 60; // Store to different location
  ASSERT(50, *p0);
  ASSERT(60, *p1);

  return 0;
}

int main(void) {
  int x = opaque_val(13);
  int y = opaque_val(47);

  test_strength_reduction_pow2(x);
  test_strength_reduction_pow2(y);
  test_strength_reduction_pow2(-5);

  test_self_cancellation(x, y);
  test_load_after_store(x);
  test_load_after_store_barrier();

  printf("OK\n");
  return 0;
}
