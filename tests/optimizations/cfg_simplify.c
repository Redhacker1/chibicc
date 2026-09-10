#include "test.h"

static int opaque_val(int x) {
  return x;
}

// 1. Unreachable Code After Return
static int test_unreachable_after_ret(int x) {
  if (x > 0) {
    return 100;
    // Dead code after return
    int dead = x * 10;
    (void)dead;
    return -1;
  }
  return 200;
}

// 2. Unreachable Code After Goto
static int test_unreachable_after_goto(int x) {
  int res = 0;
  if (x > 5) {
    goto target_a;
    // Dead instructions
    res = 999;
    res += 1000;
  } else {
    goto target_b;
    res = 888;
  }

target_a:
  return 10;

target_b:
  return 20;
}

// 3. Redundant Jump to Next Label
static int test_jump_to_next_label(int x) {
  int val = x;
  goto next_label;
next_label:
  val += 5;
  goto final_label;
final_label:
  return val;
}

// 4. Nested Branching and Falls-Through
static int test_complex_cfg(int a, int b) {
  int acc = 0;
  for (int i = 0; i < 5; i++) {
    if (a > b) {
      acc += a;
      if (acc > 50)
        break;
      continue;
    } else {
      acc += b;
      if (acc > 50)
        break;
      continue;
    }
  }
  return acc;
}

// 5. Short Circuit and Conditional Expressions
static int test_short_circuit_cfg(int a, int b) {
  int r = 0;
  if ((a > 0 && b > 0) || (a < 0 && b < 0)) {
    r = 1;
  } else {
    r = 2;
  }
  return r;
}

int main(void) {
  int pos = opaque_val(10);
  int neg = opaque_val(-10);

  ASSERT(100, test_unreachable_after_ret(pos));
  ASSERT(200, test_unreachable_after_ret(neg));

  ASSERT(10, test_unreachable_after_goto(10));
  ASSERT(20, test_unreachable_after_goto(2));

  ASSERT(15, test_jump_to_next_label(10));

  ASSERT(30, test_complex_cfg(6, 2));
  ASSERT(35, test_complex_cfg(2, 7));

  ASSERT(1, test_short_circuit_cfg(5, 10));
  ASSERT(1, test_short_circuit_cfg(-5, -10));
  ASSERT(2, test_short_circuit_cfg(5, -10));
  ASSERT(2, test_short_circuit_cfg(-5, 10));

  printf("OK\n");
  return 0;
}
