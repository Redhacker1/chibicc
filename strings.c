#include "chibicc.h"

void strarray_push(StringArray *arr, char *s) {
  if (!arr->data) {
    arr->data = calloc(8, sizeof(char *));
    arr->capacity = 8;
  }

  if (arr->capacity == arr->len) {
    arr->data = realloc(arr->data, sizeof(char *) * arr->capacity * 2);
    arr->capacity *= 2;
    for (int i = arr->len; i < arr->capacity; i++)
      arr->data[i] = NULL;
  }

  arr->data[arr->len++] = s;
}

// Takes a printf-style format string and returns a formatted string.
#ifdef _WIN32
char *format(char *fmt, ...)
{
    va_list ap;

    // Determine required buffer size.
    va_start(ap, fmt);
    int len = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    if (len < 0)
        return NULL;

    char *buf = malloc((size_t)len + 1);
    if (!buf)
        return NULL;

    // Format the string into the allocated buffer.
    va_start(ap, fmt);
    vsnprintf(buf, (size_t)len + 1, fmt, ap);
    va_end(ap);

    return buf;
}
#else
char *format(char *fmt, ...) {
  char *buf;
  size_t buflen;
  FILE *out = open_memstream(&buf, &buflen);

  va_list ap;
  va_start(ap, fmt);
  vfprintf(out, fmt, ap);
  va_end(ap);
  fclose(out);
  return buf;
}
#endif
