#ifndef _LIMITS_H
#define _LIMITS_H

/* char */
#define CHAR_BIT   8

#define SCHAR_MIN  (-128)
#define SCHAR_MAX  127
#define UCHAR_MAX  255

/* char is signed on our target for now */
#define CHAR_MIN   SCHAR_MIN
#define CHAR_MAX   SCHAR_MAX

/* short */
#define SHRT_MIN   (-32768)
#define SHRT_MAX   32767
#define USHRT_MAX  65535

/* int */
#define INT_MIN    (-2147483647 - 1)
#define INT_MAX    2147483647
#define UINT_MAX   4294967295U

/* long -- 32-bit on Win64 (LLP64) */
#define LONG_MIN   (-2147483647L - 1)
#define LONG_MAX   2147483647L
#define ULONG_MAX  4294967295UL

/* long long */
#define LLONG_MIN  (-9223372036854775807LL - 1)
#define LLONG_MAX  9223372036854775807LL
#define ULLONG_MAX 18446744073709551615ULL

/* wchar_t */
#define WCHAR_MIN  0
#define WCHAR_MAX  65535

/* Floating point */
#define FLT_RADIX       2
#define FLT_MANT_DIG    24
#define FLT_DIG         6
#define FLT_MIN_EXP     (-125)
#define FLT_MAX_EXP     128
#define FLT_MIN_10_EXP  (-37)
#define FLT_MAX_10_EXP  38
#define FLT_MIN         1.17549435e-38F
#define FLT_MAX         3.40282347e+38F
#define FLT_EPSILON     1.19209290e-7F

#define DBL_MANT_DIG    53
#define DBL_DIG         15
#define DBL_MIN_EXP     (-1021)
#define DBL_MAX_EXP     1024
#define DBL_MIN_10_EXP  (-307)
#define DBL_MAX_10_EXP  308
#define DBL_MIN         2.2250738585072014e-308
#define DBL_MAX         1.7976931348623157e+308
#define DBL_EPSILON     2.2204460492503131e-16

#define LDBL_MANT_DIG   53
#define LDBL_DIG        15
#define LDBL_MIN_EXP    (-1021)
#define LDBL_MAX_EXP    1024
#define LDBL_MIN_10_EXP (-307)
#define LDBL_MAX_10_EXP 308
#define LDBL_MIN        2.2250738585072014e-308L
#define LDBL_MAX        1.7976931348623157e+308L
#define LDBL_EPSILON    2.2204460492503131e-16L

#define UINTPTR_MAX 18446744073709551615ULL
#define INTPTR_MAX  9223372036854775807LL
#define INTPTR_MIN  (-9223372036854775807LL - 1)

#endif