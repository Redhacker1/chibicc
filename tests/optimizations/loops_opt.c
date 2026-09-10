#include "test.h"

static int opaque_val(int x) {
  return x;
}

// 1. Loop with Induction Variables & Accumulator
static int test_loop_induction(int n) {
  int sum = 0;
  for (int i = 0; i < n; i++) {
    int term = i * 2;
    sum += term;
  }
  return sum;
}

// 2. Nested Loops
static int test_nested_loops(int outer, int inner) {
  int total = 0;
  for (int i = 0; i < outer; i++) {
    for (int j = 0; j < inner; j++) {
      total += (i + j);
    }
  }
  return total;
}

// 3. While Loop with Early Break
static int test_while_break(int limit) {
  int count = 0;
  int i = 0;
  while (1) {
    if (i >= limit)
      break;
    count += i;
    i++;
  }
  return count;
}

// 4. Do-While Loop with Continue
static int test_do_while_continue(int max_val) {
  int sum = 0;
  int i = 0;
  do {
    i++;
    if (i % 2 == 0)
      continue;
    sum += i;
  } while (i < max_val);
  return sum;
}

int main(void) {
  int n = opaque_val(10);
  ASSERT(90, test_loop_induction(n));   // 2 * (0+1+...+9) = 2 * 45 = 90
  ASSERT(0, test_loop_induction(0));

  ASSERT(70, test_nested_loops(4, 5)); // sum of (i+j) for i in 0..3, j in 0..4
  ASSERT(45, test_while_break(10));
  ASSERT(25, test_do_while_continue(10)); // 1 + 3 + 5 + 7 + 9 = 25

  printf("OK\n");
  return 0;
}
