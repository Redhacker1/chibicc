#include "test.h"

int main() {
  ASSERT(0, ({ enum { zero, one, two }; zero; }));
  ASSERT(1, ({ enum { zero, one, two }; one; }));
  ASSERT(2, ({ enum { zero, one, two }; two; }));
  ASSERT(5, ({ enum { five=5, six, seven }; five; }));
  ASSERT(6, ({ enum { five=5, six, seven }; six; }));
  ASSERT(0, ({ enum { zero, five=5, three=3, four }; zero; }));
  ASSERT(5, ({ enum { zero, five=5, three=3, four }; five; }));
  ASSERT(3, ({ enum { zero, five=5, three=3, four }; three; }));
  ASSERT(4, ({ enum { zero, five=5, three=3, four }; four; }));
  ASSERT(4, ({ enum { zero, one, two } x; sizeof(x); }));
  ASSERT(4, ({ enum t { zero, one, two }; enum t y; sizeof(y); }));
  ASSERT(1, ({ enum __attribute__((__packed__)) { ep0, ep1 } x; ep1; }));
  ASSERT(1, ({ enum __attribute__((packed)) te1 { ea, eb }; eb; }));
  ASSERT(1, ({ enum te2 __attribute__((__packed__)) { ec, ed }; ed; }));
  ASSERT(1, ({ typedef enum __attribute__((__packed__)) { ee, ef } te3; ef; }));

  printf("OK\n");
  return 0;
}
