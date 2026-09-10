#include "test.h"

// 1. Fixed-Point Optimization Interaction (Const Fold + Copy Prop + Peephole + DCE)
static int test_multi_pass_interaction(int input) {
  int a = input;
  int b = a + 0;       // Algebraic identity -> b = a
  int c = b * 1;       // Algebraic identity -> c = b -> c = a
  int d = c * 8;       // Strength reduction -> d = a << 3
  int dead = d * 100;  // DCE candidate
  (void)dead;
  int e = d - 0;       // Algebraic identity -> e = d
  return e;
}

// 2. Fast Exponentiation Algorithm
static long long power(long long base, int exp) {
  long long result = 1;
  while (exp > 0) {
    if (exp & 1)
      result *= base;
    base *= base;
    exp >>= 1;
  }
  return result;
}

// 3. Recursive and Iterative Fibonacci
static int fib_iter(int n) {
  if (n <= 1) return n;
  int a = 0, b = 1;
  for (int i = 2; i <= n; i++) {
    int c = a + b;
    a = b;
    b = c;
  }
  return b;
}

// 4. Matrix Multiplication
static void matrix_mul_2x2(const int a[2][2], const int b[2][2], int out[2][2]) {
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      out[i][j] = 0;
      for (int k = 0; k < 2; k++) {
        out[i][j] += a[i][k] * b[k][j];
      }
    }
  }
}

// 5. Binary Search
static int binary_search(const int *arr, int size, int target) {
  int low = 0;
  int high = size - 1;
  while (low <= high) {
    int mid = low + (high - low) / 2;
    if (arr[mid] == target)
      return mid;
    if (arr[mid] < target)
      low = mid + 1;
    else
      high = mid - 1;
  }
  return -1;
}

// 6. Popcount / Bit Manipulation
static int count_set_bits(unsigned int n) {
  int count = 0;
  while (n > 0) {
    count += (n & 1);
    n >>= 1;
  }
  return count;
}

int main(void) {
  // Test multi-pass interaction
  ASSERT(80, test_multi_pass_interaction(10));
  ASSERT(0, test_multi_pass_interaction(0));
  ASSERT(-64, test_multi_pass_interaction(-8));

  // Test power
  ASSERT(1024, power(2, 10));
  ASSERT(243, power(3, 5));
  ASSERT(1, power(5, 0));

  // Test fib
  ASSERT(0, fib_iter(0));
  ASSERT(1, fib_iter(1));
  ASSERT(1, fib_iter(2));
  ASSERT(2, fib_iter(3));
  ASSERT(3, fib_iter(4));
  ASSERT(55, fib_iter(10));

  // Test matrix mul
  int matA[2][2] = { {1, 2}, {3, 4} };
  int matB[2][2] = { {2, 0}, {1, 2} };
  int matC[2][2];
  matrix_mul_2x2(matA, matB, matC);
  ASSERT(4, matC[0][0]);
  ASSERT(4, matC[0][1]);
  ASSERT(10, matC[1][0]);
  ASSERT(8, matC[1][1]);

  // Test binary search
  int sorted[] = { 2, 5, 8, 12, 16, 23, 38, 56, 72, 91 };
  ASSERT(0, binary_search(sorted, 10, 2));
  ASSERT(5, binary_search(sorted, 10, 23));
  ASSERT(9, binary_search(sorted, 10, 91));
  ASSERT(-1, binary_search(sorted, 10, 99));

  // Test bit counting
  ASSERT(0, count_set_bits(0));
  ASSERT(1, count_set_bits(1));
  ASSERT(8, count_set_bits(0xFF));
  ASSERT(16, count_set_bits(0x55555555));

  printf("OK\n");
  return 0;
}
