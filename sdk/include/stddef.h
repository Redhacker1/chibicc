#ifndef __STDDEF_H
#define __STDDEF_H

#define NULL ((void *)0)

#ifdef __SIZE_TYPE__
typedef __SIZE_TYPE__ size_t;
#elif defined(_WIN64) || defined(__x86_64__) || defined(__LP64__) || defined(_LP64)
typedef unsigned long long size_t;
#else
typedef unsigned long size_t;
#endif

#ifdef __PTRDIFF_TYPE__
typedef __PTRDIFF_TYPE__ ptrdiff_t;
#elif defined(_WIN64) || defined(__x86_64__) || defined(__LP64__) || defined(_LP64)
typedef long long ptrdiff_t;
#else
typedef long ptrdiff_t;
#endif

#if defined(_WIN32)
typedef unsigned short wchar_t;
#else
typedef unsigned int wchar_t;
#endif
typedef unsigned int wint_t;

typedef long double max_align_t;

#define offsetof(type, member) ((size_t)&(((type *)0)->member))

#endif
