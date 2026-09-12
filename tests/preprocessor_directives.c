#include "test.h"

// Null directive
#
#

// Basic #define and #undef
#define TEST_MACRO_A 100
#if TEST_MACRO_A != 100
#error "TEST_MACRO_A should be 100"
#endif

#undef TEST_MACRO_A
#ifdef TEST_MACRO_A
#error "TEST_MACRO_A should be undefined"
#endif

// #warning directive
#warning "This is a preprocessor warning test"

// #elifdef directive
#define COND_A 1
#undef COND_B
#define COND_C 1

#ifdef COND_X
int val1 = 1;
#elifdef COND_B
int val1 = 2;
#elifdef COND_C
int val1 = 3;
#else
int val1 = 4;
#endif

// #elifndef directive
#ifdef COND_X
int val2 = 1;
#elifndef COND_B
int val2 = 2;
#elifdef COND_C
int val2 = 3;
#else
int val2 = 4;
#endif

// Nested conditional directives
#ifdef COND_A
  #ifndef COND_B
    #ifdef COND_C
      #define NESTED_RESULT 42
    #endif
  #endif
#endif

// Complex elif/elifdef/elifndef chain
#define TARGET_MODE 3

#if TARGET_MODE == 1
int mode = 1;
#elif TARGET_MODE == 2
int mode = 2;
#elifdef COND_B
int mode = 99;
#elifndef COND_B
int mode = 3;
#else
int mode = 4;
#endif

int main() {
  ASSERT(3, val1);
  ASSERT(2, val2);
  ASSERT(42, NESTED_RESULT);
  ASSERT(3, mode);

  printf("OK\n");
  return 0;
}
