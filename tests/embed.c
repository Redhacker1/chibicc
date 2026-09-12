#include "test.h"

// Test __has_embed in #if directives
#if !defined(__STDC_EMBED_FOUND__) || __STDC_EMBED_FOUND__ != 1
#error "__STDC_EMBED_FOUND__ should be 1"
#endif

#if !defined(__STDC_EMBED_EMPTY__) || __STDC_EMBED_EMPTY__ != 2
#error "__STDC_EMBED_EMPTY__ should be 2"
#endif

#if !defined(__STDC_EMBED_NOT_FOUND__) || __STDC_EMBED_NOT_FOUND__ != 0
#error "__STDC_EMBED_NOT_FOUND__ should be 0"
#endif

#if __has_embed("tests/embed_data.bin") != __STDC_EMBED_FOUND__
#error "__has_embed tests/embed_data.bin should be FOUND"
#endif

#if __has_embed("tests/embed_empty.bin") != __STDC_EMBED_EMPTY__
#error "__has_embed tests/embed_empty.bin should be EMPTY"
#endif

#if __has_embed("tests/nonexistent_file_123.bin") != __STDC_EMBED_NOT_FOUND__
#error "__has_embed nonexistent should be NOT_FOUND"
#endif

#if __has_embed("tests/embed_data.bin" limit(0)) != __STDC_EMBED_EMPTY__
#error "__has_embed with limit(0) should be EMPTY"
#endif

#if __has_embed("tests/embed_data.bin" limit(4)) != __STDC_EMBED_FOUND__
#error "__has_embed with limit(4) should be FOUND"
#endif

#define DATA_FILE "tests/embed_data.bin"
#if __has_embed(DATA_FILE) != __STDC_EMBED_FOUND__
#error "__has_embed with macro should be FOUND"
#endif

int main() {
  // Basic #embed
  const unsigned char data1[] = {
#embed "tests/embed_data.bin"
  };
  ASSERT(8, sizeof(data1));
  ASSERT('A', data1[0]);
  ASSERT('B', data1[1]);
  ASSERT('C', data1[2]);
  ASSERT('D', data1[3]);
  ASSERT('E', data1[4]);
  ASSERT('F', data1[5]);
  ASSERT('G', data1[6]);
  ASSERT('H', data1[7]);

  // #embed with limit
  const unsigned char data2[] = {
#embed "tests/embed_data.bin" limit(3)
  };
  ASSERT(3, sizeof(data2));
  ASSERT('A', data2[0]);
  ASSERT('B', data2[1]);
  ASSERT('C', data2[2]);

  // #embed with limit expression
  const unsigned char data2_expr[] = {
#embed "tests/embed_data.bin" limit(2 + 2)
  };
  ASSERT(4, sizeof(data2_expr));
  ASSERT('A', data2_expr[0]);
  ASSERT('D', data2_expr[3]);

  // #embed with prefix and suffix
  const int data3[] = {
#embed "tests/embed_data.bin" limit(2) prefix(10) suffix(20)
  };
  ASSERT(4, sizeof(data3) / sizeof(data3[0]));
  ASSERT(10, data3[0]);
  ASSERT('A', data3[1]);
  ASSERT('B', data3[2]);
  ASSERT(20, data3[3]);

  // #embed with empty file and if_empty
  const int data4[] = {
#embed "tests/embed_empty.bin" if_empty(42, 43)
  };
  ASSERT(2, sizeof(data4) / sizeof(data4[0]));
  ASSERT(42, data4[0]);
  ASSERT(43, data4[1]);

  // #embed with limit(0) and if_empty
  const int data5[] = {
#embed "tests/embed_data.bin" limit(0) if_empty(99)
  };
  ASSERT(1, sizeof(data5) / sizeof(data5[0]));
  ASSERT(99, data5[0]);

  // Macro expansion for filename
  const unsigned char data6[] = {
#embed DATA_FILE limit(2)
  };
  ASSERT(2, sizeof(data6));
  ASSERT('A', data6[0]);
  ASSERT('B', data6[1]);

  printf("OK\n");
  return 0;
}
