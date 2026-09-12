#include "test.h"

struct Point {
  int x;
  int y;
};

static int test_binary_ops(int a, int b) {
  int r = 0;
  r += (a + b);
  r -= (a - b);
  r += (a * b);
  r += (a / (b != 0 ? b : 1));
  r += (a % (b != 0 ? b : 1));
  r += (a & b);
  r += (a | b);
  r += (a ^ b);
  r += (a << 1);
  r += (b >> 1);
  return r;
}

static int test_comparisons(int a, int b) {
  int c1 = (a == b);
  int c2 = (a != b);
  int c3 = (a < b);
  int c4 = (a <= b);
  int c5 = (a > b);
  int c6 = (a >= b);
  int c7 = (a && b);
  int c8 = (a || b);
  return c1 + c2 + c3 + c4 + c5 + c6 + c7 + c8;
}

static int test_unary_ops(int a) {
  int r = -a;
  r += +a;
  r += !a;
  r += ~a;
  return r;
}

static int test_control_flow(int val) {
  int sum = 0;

  // if-else
  if (val > 10) {
    sum += 100;
  } else if (val > 5) {
    sum += 50;
  } else {
    sum += 10;
  }

  // for loop
  for (int i = 0; i < 5; i++) {
    if (i == 3) continue;
    sum += i;
  }

  // while loop
  int w = 3;
  while (w > 0) {
    sum += w;
    w--;
  }

  // do-while loop
  int d = 2;
  do {
    sum += d;
    d--;
  } while (d > 0);

  // switch-case with range
  switch (val) {
  case 1 ... 5:
    sum += 1;
    break;
  case 6 ... 10:
    sum += 2;
    break;
  default:
    sum += 3;
    break;
  }

  return sum;
}

static int test_labels_and_gotos(int n) {
  int res = 0;
  if (n == 0) goto lbl_zero;
  if (n == 1) goto lbl_one;
  if (n == 2) goto lbl_two;
  goto end;

lbl_zero:
  res = 100;
  goto end;
lbl_one:
  res = 200;
  goto end;
lbl_two:
  res = 300;
  goto end;

//ASSERT(0 (-1));

end:
  return res;
}

static int test_stmt_expr_and_member(int x, int y) {
  struct Point pt = { x, y };
  int z = ({
    int tmp_x = pt.x;
    int tmp_y = pt.y;
    tmp_x * 10 + tmp_y;
  });
  return z;
}

static int test_compound_assignments(int x) {
  int a = x;
  a += 5;
  a -= 2;
  a *= 3;
  a /= 2;
  a %= 7;
  a &= 15;
  a |= 2;
  a ^= 1;
  a <<= 1;
  a >>= 1;
  return a;
}

static int test_ternary_and_comma(int a, int b) {
  int x = (a > b) ? (a, b + 10) : (b, a + 20);
  int y = a ?: 999;
  return x + y;
}

int main() {
  ASSERT(83, test_binary_ops(10, 3));
  ASSERT(5, test_comparisons(10, 3));
  ASSERT(-11, test_unary_ops(10));
  ASSERT(119, test_control_flow(12));
  ASSERT(69, test_control_flow(7));
  ASSERT(29, test_control_flow(2));

  ASSERT(100, test_labels_and_gotos(0));
  ASSERT(200, test_labels_and_gotos(1));
  ASSERT(300, test_labels_and_gotos(2));
  ASSERT(0, test_labels_and_gotos(-1));

  ASSERT(75, test_stmt_expr_and_member(7, 5));
  ASSERT(2, test_compound_assignments(4));
  ASSERT(23, test_ternary_and_comma(10, 3));
  ASSERT(1019, test_ternary_and_comma(0, 2));

  printf("OK\n");
  return 0;
}
