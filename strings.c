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
char *format(char *fmt, ...) {
#if defined(_MSC_VER)
  va_list ap;
  va_start(ap, fmt);
  int len = _vscprintf(fmt, ap);
  va_end(ap);
  if (len < 0) return NULL;
  char *buf = malloc((size_t)len + 1);
  if (!buf) return NULL;
  va_start(ap, fmt);
  vsprintf_s(buf, (size_t)len + 1, fmt, ap);
  va_end(ap);
  return buf;
#else
  char *buf = NULL;
  va_list ap;
  va_start(ap, fmt);
  vasprintf(&buf, fmt, ap);
  va_end(ap);
  return buf;
#endif
}
