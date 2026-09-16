#ifndef TESTS_TEST_H
#define TESTS_TEST_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT(x, y) assert(x, y, #y)

void assert(int expected, int actual, char *code);

#endif
