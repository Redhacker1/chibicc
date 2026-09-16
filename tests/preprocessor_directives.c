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

// #ident and #sccs
#ident "ident directive test"
#sccs "sccs directive test"

// #pragma message
#pragma message("This is a message from pragma message")
#pragma message "This is a message without parentheses"

// #pragma push_macro and #pragma pop_macro
#define STACK_MACRO 10
#pragma push_macro("STACK_MACRO")
#define STACK_MACRO 20
int stack_val1 = STACK_MACRO;
#pragma push_macro("STACK_MACRO")
#define STACK_MACRO 30
int stack_val2 = STACK_MACRO;
#pragma pop_macro("STACK_MACRO")
int stack_val3 = STACK_MACRO;
#pragma pop_macro("STACK_MACRO")
int stack_val4 = STACK_MACRO;

// Pushing an undefined macro
#undef UNDEF_STACK_MACRO
#pragma push_macro("UNDEF_STACK_MACRO")
#define UNDEF_STACK_MACRO 555
int undef_stack_val1 = UNDEF_STACK_MACRO;
#pragma pop_macro("UNDEF_STACK_MACRO")
#ifdef UNDEF_STACK_MACRO
int undef_stack_val2 = 1;
#else
int undef_stack_val2 = 0;
#endif

// _Pragma operator
#define PRAGMA_MACRO 1000
_Pragma("push_macro(\"PRAGMA_MACRO\")")
#define PRAGMA_MACRO 2000
int pragma_op_val1 = PRAGMA_MACRO;
_Pragma("pop_macro(\"PRAGMA_MACRO\")")
int pragma_op_val2 = PRAGMA_MACRO;

int main() {
  ASSERT(3, val1);
  ASSERT(2, val2);
  ASSERT(42, NESTED_RESULT);
  ASSERT(3, mode);
  ASSERT(20, stack_val1);
  ASSERT(30, stack_val2);
  ASSERT(20, stack_val3);
  ASSERT(10, stack_val4);
  ASSERT(555, undef_stack_val1);
  ASSERT(0, undef_stack_val2);
  ASSERT(2000, pragma_op_val1);
  ASSERT(1000, pragma_op_val2);

  printf("OK\n");
  return 0;
}
