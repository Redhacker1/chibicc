#ifndef __STDLIB_H
#define __STDLIB_H

#include <stddef.h>

void exit(int status);
void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);
int abs(int j);
int atoi(const char *nptr);

#endif // __STDLIB_H
