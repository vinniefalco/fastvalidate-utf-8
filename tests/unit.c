#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "simdutf8check.h"
#include "simdasciicheck.h"


void test() {
  char *goodsequences[] = {"a", "\xc3\xb1", "\xe2\x82\xa1", "\xf0\x90\x8c\xbc", "안녕하세요, 세상"};
  char *badsequences[] = {"\xc3\x28",         // 0
                          "\xa0\xa1",         // 1
                          "\xe2\x28\xa1",     // 2
                          "\xe2\x82\x28",     // 3
                          "\xf0\x28\x8c\xbc", // 4
                          "\xf0\x90\x28\xbc", // 5
                          "\xf0\x28\x8c\x28", // 6
                          "\xc0\x9f",         // 7
                          "\xf5\xff\xff\xff", // 8
                          "\xed\xa0\x81", // 9
                          "\xf8\x90\x80\x80\x80", //10
                          "123456789012345\xed", //11
                          "123456789012345\xf1", //12
                          "123456789012345\xc2", //13
                        };
  for (size_t i = 0; i < 5; i++) {
    size_t len = strlen(goodsequences[i]);
    if(!validate_utf8_fast(goodsequences[i], len)) {
      printf("failing to validate good string %zu \n", i);
      for(size_t j = 0; j < len; j++) printf("0x%02x ", (unsigned char)goodsequences[i][j]);
      printf("\n");
      abort();
    }
#ifdef __AVX2__
    if(!avx_validate_utf8_fast(goodsequences[i], len)) {
      printf("(avx) failing to validate good string %zu \n", i);
      for(size_t j = 0; j < len; j++) printf("0x%02x ", (unsigned char)goodsequences[i][j]);
      printf("\n");
      abort();
    }
#endif
  }
  for (size_t i = 0; i < 14; i++) {
    size_t len = strlen(badsequences[i]);
    if(validate_utf8_fast(badsequences[i], len)) {
      printf("failing to invalidate bad string %zu \n", i);
      for(size_t j = 0; j < len; j++) printf("0x%02x ", (unsigned char)badsequences[i][j]);
      printf("\n");
      abort();
    }
#ifdef __AVX2__
    if(avx_validate_utf8_fast(badsequences[i], len)) {
      printf("(avx) failing to invalidate bad string %zu \n", i);
      for(size_t j = 0; j < len; j++) printf("0x%02x ", (unsigned char)badsequences[i][j]);
      printf("\n");
      abort();
    }
#endif
  }

  char ascii[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 0};
  char notascii[] = {128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 238, 255, 0};
  assert(validate_ascii_fast(ascii, strlen(ascii)));
  assert(!validate_ascii_fast(notascii, strlen(notascii)));

  __m128i cont = _mm_setr_epi8(4,0,0,0,3,0,0,2,0,1,2,0,3,0,0,1);
  __m128i has_error = _mm_setzero_si128();
  __m128i carries = carryContinuations(cont, _mm_set1_epi8(1));
  checkContinuations(cont, carries, &has_error);
  assert(_mm_test_all_zeros(has_error, has_error));
  assert(_mm_test_all_ones(_mm_cmpeq_epi8(carries, _mm_setr_epi8(4,3,2,1,3,2,1,2,1,1,2,1,3,2,1,1))));

  // overlap
  cont = _mm_setr_epi8(4,0,1,0,3,0,0,2,0,1,2,0,3,0,0,1);
  has_error = _mm_setzero_si128();
  carries = carryContinuations(cont, _mm_set1_epi8(1));
  checkContinuations(cont, carries, &has_error);
  assert(!_mm_test_all_zeros(has_error, has_error));

  // underlap
  cont = _mm_setr_epi8(4,0,0,0,0,0,0,2,0,1,2,0,3,0,0,1);
  has_error = _mm_setzero_si128();
  carries = carryContinuations(cont, _mm_set1_epi8(1));
  checkContinuations(cont, carries, &has_error);
  assert(!_mm_test_all_zeros(has_error, has_error));

    // register crossing
  cont = _mm_setr_epi8(0,2,0,3,0,0,2,0,1,2,0,3,0,0,1,1);
  __m128i prev = _mm_setr_epi8(3,2,1,3,2,1,2,1,1,2,1,3,2,4,3,2);
  has_error = _mm_setzero_si128();
  carries = carryContinuations(cont, prev);
  checkContinuations(cont, carries, &has_error);
  assert(_mm_test_all_zeros(has_error, has_error));

}

int main() {
  test();
  printf("Your code is ok.\n");
  return EXIT_SUCCESS;
}
