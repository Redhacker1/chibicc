#ifndef _ASSERT_H
#define _ASSERT_H

#undef assert

#ifdef NDEBUG
# define assert(expr) ((void)0)
#else
# ifdef __cplusplus
extern "C" {
# endif
void _assert(const char *expr, const char *file, int line, const char *func);
# ifdef __cplusplus
}
# endif

# if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#  define assert(expr) \
     ((expr) ? (void)0 : _assert(#expr, __FILE__, __LINE__, __func__))
# else
#  define assert(expr) \
     ((expr) ? (void)0 : _assert(#expr, __FILE__, __LINE__, (const char *)0))
# endif
#endif

#endif // _ASSERT_H
