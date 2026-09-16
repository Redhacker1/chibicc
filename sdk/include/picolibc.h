#ifndef _PICOLIBC_H_
#define _PICOLIBC_H_

#define __PICOLIBC__ 1
#define __PICOLIBC_MINOR__ 8
#define __PICOLIBC_PATCHLEVEL__ 6
#define _NEWLIB_VERSION "4.3.0"
#define _PICOLIBC_VERSION "1.8.6"

#define _IEEE_LIBM 1
#define _PICOLIBC_CTYPE_SMALL 1
#define __IO_C99_FORMATS 1

#ifndef __SCHAR_MAX__
#define __SCHAR_MAX__ 127
#endif
#ifndef __SHRT_MAX__
#define __SHRT_MAX__ 32767
#endif
#ifndef __INT_MAX__
#define __INT_MAX__ 2147483647
#endif
#ifndef __LONG_MAX__
#define __LONG_MAX__ 2147483647L
#endif
#ifndef __LONG_LONG_MAX__
#define __LONG_LONG_MAX__ 9223372036854775807LL
#endif
#ifndef __SIZE_MAX__
#define __SIZE_MAX__ 18446744073709551615ULL
#endif

#ifndef __FLT_MAX_EXP__
#define __FLT_MAX_EXP__ 128
#endif
#ifndef __FLT_MAX_10_EXP__
#define __FLT_MAX_10_EXP__ 38
#endif
#ifndef __FLT_MIN_EXP__
#define __FLT_MIN_EXP__ (-125)
#endif
#ifndef __FLT_MIN_10_EXP__
#define __FLT_MIN_10_EXP__ (-37)
#endif
#ifndef __FLT_MANT_DIG__
#define __FLT_MANT_DIG__ 24
#endif
#ifndef __FLT_DIG__
#define __FLT_DIG__ 6
#endif
#ifndef __FLT_RADIX__
#define __FLT_RADIX__ 2
#endif

#ifndef __DBL_MAX_EXP__
#define __DBL_MAX_EXP__ 1024
#endif
#ifndef __DBL_MAX_10_EXP__
#define __DBL_MAX_10_EXP__ 308
#endif
#ifndef __DBL_MIN_EXP__
#define __DBL_MIN_EXP__ (-1021)
#endif
#ifndef __DBL_MIN_10_EXP__
#define __DBL_MIN_10_EXP__ (-307)
#endif
#ifndef __DBL_MANT_DIG__
#define __DBL_MANT_DIG__ 53
#endif
#ifndef __DBL_DIG__
#define __DBL_DIG__ 15
#endif

#ifndef __LDBL_MAX_EXP__
#define __LDBL_MAX_EXP__ 1024
#endif
#ifndef __LDBL_MAX_10_EXP__
#define __LDBL_MAX_10_EXP__ 308
#endif
#ifndef __LDBL_MIN_EXP__
#define __LDBL_MIN_EXP__ (-1021)
#endif
#ifndef __LDBL_MIN_10_EXP__
#define __LDBL_MIN_10_EXP__ (-307)
#endif
#ifndef __LDBL_MANT_DIG__
#define __LDBL_MANT_DIG__ 53
#endif
#ifndef __LDBL_DIG__
#define __LDBL_DIG__ 15
#endif

#ifndef __SIZEOF_WCHAR_T__
#define __SIZEOF_WCHAR_T__ 2
#endif

#ifndef __WCHAR_WIDTH__
#define __WCHAR_WIDTH__ 16
#endif

#ifndef __FLT_MIN__
#define __FLT_MIN__ 1.17549435082228750796873653722224568e-38F
#endif
#ifndef __FLT_MAX__
#define __FLT_MAX__ 3.40282346638528859811704183484516925e+38F
#endif
#ifndef __FLT_EPSILON__
#define __FLT_EPSILON__ 1.19209289550781250000000000000000000e-7F
#endif
#ifndef __FLT_DENORM_MIN__
#define __FLT_DENORM_MIN__ 1.40129846432481707092372958328991613e-45F
#endif

#ifndef __DBL_MIN__
#define __DBL_MIN__ 2.22507385850720138309023271733240406e-308
#endif
#ifndef __DBL_MAX__
#define __DBL_MAX__ 1.79769313486231570814527423731704357e+308
#endif
#ifndef __DBL_EPSILON__
#define __DBL_EPSILON__ 2.22044604925031308084726333618164062e-16
#endif
#ifndef __DBL_DENORM_MIN__
#define __DBL_DENORM_MIN__ 4.94065645841246544176568792868221372e-324
#endif

#ifndef __LDBL_MIN__
#define __LDBL_MIN__ 2.22507385850720138309023271733240406e-308L
#endif
#ifndef __LDBL_MAX__
#define __LDBL_MAX__ 1.79769313486231570814527423731704357e+308L
#endif
#ifndef __LDBL_EPSILON__
#define __LDBL_EPSILON__ 2.22044604925031308084726333618164062e-16L
#endif
#ifndef __LDBL_DENORM_MIN__
#define __LDBL_DENORM_MIN__ 4.94065645841246544176568792868221372e-324L
#endif

#ifndef __weak_reference
#define __weak_reference(sym, aliassym) extern __typeof(sym) aliassym __attribute__((__weak__, __alias__(#sym)))
#endif

#ifndef _WINT_T
#define _WINT_T
typedef unsigned int wint_t;
#endif

#endif /* _PICOLIBC_H_ */
