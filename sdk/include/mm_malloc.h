#ifndef _MM_MALLOC_H_INCLUDED
#define _MM_MALLOC_H_INCLUDED

#include <stdlib.h>
#include <errno.h>

static __inline__ void *
_mm_malloc (size_t __size, size_t __align)
{
  void * __malloc_ptr;
  void * __aligned_ptr;

  if (__align & (__align - 1))
    {
      errno = EINVAL;
      return ((void *) 0);
    }

  if (__size == 0)
    return ((void *) 0);

  if (__align < 2 * sizeof (void *))
    __align = 2 * sizeof (void *);

  __malloc_ptr = malloc (__size + __align);
  if (!__malloc_ptr)
    return ((void *) 0);

  __aligned_ptr = (void *) (((size_t) __malloc_ptr + __align)
			    & ~((size_t) (__align) - 1));

  ((void **) __aligned_ptr)[-1] = __malloc_ptr;

  return __aligned_ptr;
}

static __inline__ void
_mm_free (void *__aligned_ptr)
{
  if (__aligned_ptr)
    free (((void **) __aligned_ptr)[-1]);
}

#endif /* _MM_MALLOC_H_INCLUDED */
