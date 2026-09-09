#ifndef _STDINT_H
#define _STDINT_H

typedef signed char        int8_t;
typedef unsigned char      uint8_t;

typedef signed short       int16_t;
typedef unsigned short     uint16_t;

typedef signed int         int32_t;
typedef unsigned int       uint32_t;

typedef signed long long   int64_t;
typedef unsigned long long uint64_t;

typedef signed char        int_least8_t;
typedef unsigned char      uint_least8_t;

typedef signed short       int_least16_t;
typedef unsigned short     uint_least16_t;

typedef signed int         int_least32_t;
typedef unsigned int       uint_least32_t;

typedef signed long long   int_least64_t;
typedef unsigned long long uint_least64_t;

typedef signed char        int_fast8_t;
typedef unsigned char      uint_fast8_t;

typedef signed int         int_fast16_t;
typedef unsigned int       uint_fast16_t;

typedef signed int         int_fast32_t;
typedef unsigned long long uint_fast64_t;

typedef signed long long   intmax_t;
typedef unsigned long long uintmax_t;


typedef unsigned long long uintptr_t;
typedef signed long long   intptr_t;


#ifndef INT8_MIN
#define INT8_MIN    (-128)
#endif

#ifndef INT8_MAX
#define INT8_MAX    127
#endif

#ifndef UINT8_MAX
#define UINT8_MAX   255
#endif
#define INT8_MAX    127
#define UINT8_MAX   255

#ifndef INT16_MIN
#define INT16_MIN   (-32767 - 1)
#endif

#ifndef INT16_MAX
#define INT16_MAX   32767
#endif

#ifndef UINT16_MAX
#define UINT16_MAX  65535
#endif

#ifndef INT32_MIN
#define INT32_MIN   (-2147483647 - 1)
#endif

#ifndef INT32_MAX
#define INT32_MAX   2147483647
#endif

#ifndef UINT32_MAX
#define UINT32_MAX  4294967295U
#endif

#ifndef INT64_MIN
#define INT64_MIN   (-9223372036854775807LL - 1)
#endif

#ifndef INT64_MAX
#define INT64_MAX   9223372036854775807LL
#endif

#ifndef UINT64_MAX
#define UINT64_MAX  18446744073709551615ULL
#endif

#ifndef INTPTR_MIN
#define INTPTR_MIN  (-9223372036854775807LL - 1)
#endif

#ifndef INTPTR_MAX
#define INTPTR_MAX  9223372036854775807LL
#endif

#ifndef UINTPTR_MAX
#define UINTPTR_MAX 18446744073709551615ULL
#endif

#ifndef INTMAX_MIN
#define INTMAX_MIN  (-9223372036854775807LL - 1)
#endif

#ifndef INTMAX_MAX
#define INTMAX_MAX  9223372036854775807LL
#endif

#ifndef UINTMAX_MAX
#define UINTMAX_MAX 18446744073709551615ULL
#endif

#ifndef SIZE_MAX
#define SIZE_MAX    18446744073709551615ULL
#endif

#ifndef PTRDIFF_MIN
#define PTRDIFF_MIN (-9223372036854775807LL - 1)
#endif

#ifndef PTRDIFF_MAX
#define PTRDIFF_MAX 9223372036854775807LL
#endif

#endif