// RUN: %CC %THIS_FILE -mavx -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -test-runs 1 -v 10 -no-progress-bar -j 0 2>&1 | grep "ymm0:" | head -n 1 | grep "0xddddeeeeccccdddd bbbbccccaaaabbbb 4567defa3456cdef 2345bcde1234abcd"

// This test checks that we can import a YMM vector register correctly.

#ifdef __AVX__
#include <immintrin.h>
#else
#error Requires AVX
#endif

int main() {
  __m256i ymm0 =
      _mm256_setr_epi32(0x1234abcd, 0x2345bcde, 0x3456cdef, 0x4567defa,
                        0xaaaabbbb, 0xbbbbcccc, 0xccccdddd, 0xddddeeee);
  int i;
  for (i = 0; i != 140000000; ++i)
    ;
  return 0;
}
