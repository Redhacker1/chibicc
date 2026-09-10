#include "test.h"

static int opaque_val(int x) {
  return x;
}

static int side_effect_counter = 0;

static int impure_fn(int val) {
  side_effect_counter += val;
  return side_effect_counter;
}

// 1. Pure Dead Variables & Computations
static int test_pure_dead_code(int x) {
  int dead1 = x * 100;
  int dead2 = dead1 + 500;
  int dead3 = dead2 / 3;
  int dead4 = dead3 ^ 0xAA;
  (void)dead1; (void)dead2; (void)dead3; (void)dead4;

  int live = x + 10;
  return live;
}

// 2. Side-Effect Preservation
static int test_side_effect_preservation(int x) {
  side_effect_counter = 0;

  // Unused return value from function with side effects MUST NOT be eliminated
  impure_fn(5);
  impure_fn(10);
  impure_fn(x);

  ASSERT(15 + x, side_effect_counter);
  return 0;
}

// 3. Cascading Dead Code Elimination
static int test_cascading_dce(int a, int b) {
  int u1 = a + b;
  int u2 = u1 * 2;
  int u3 = u2 - a;
  int u4 = u3 * u1;
  int u5 = u4 / (b ? b : 1);
  (void)u5;

  return a * 10 + b;
}

// 4. Dead Code in Conditional Branches
static int test_dead_code_in_branches(int flag, int x) {
  int res = 0;
  if (flag) {
    int dead_a = x * 10;
    (void)dead_a;
    res = x + 1;
  } else {
    int dead_b = x * 20;
    (void)dead_b;
    res = x + 2;
  }
  return res;
}

// 5. Memory Write Preservation
static int test_memory_write_preservation(void) {
  int buffer[4] = { 0 };
  buffer[0] = 10;
  buffer[1] = 20;
  buffer[2] = 30;
  buffer[3] = 40;

  // Read all to make sure all stores occurred
  ASSERT(10, buffer[0]);
  ASSERT(20, buffer[1]);
  ASSERT(30, buffer[2]);
  ASSERT(40, buffer[3]);

  return 0;
}

int main(void) {
  int x = opaque_val(7);

  ASSERT(17, test_pure_dead_code(x));
  test_side_effect_preservation(x);
  ASSERT(75, test_cascading_dce(7, 5));
  ASSERT(8, test_dead_code_in_branches(1, x));
  ASSERT(9, test_dead_code_in_branches(0, x));
  test_memory_write_preservation();

  printf("OK\n");
  return 0;
}
