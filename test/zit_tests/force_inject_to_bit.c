// RUN: %CC %THIS_FILE -mavx -o %UNIQUE_FILE && %ZOFI -force-inject-to-reg ymm0 -force-inject-to-bit 255 -bin %UNIQUE_FILE -test-runs 1 -v 10 -no-progress-bar -j 0 2>&1 | grep "ymm0:" | tail -n 1 | grep "0x5dddeeeeccccdddd bbbbccccaaaabbbb 4567defa3456cdef 2345bcde1234abcd"
// RUN: %CC %THIS_FILE -mavx -o %UNIQUE_FILE && %ZOFI -force-inject-to-reg ymm0 -force-inject-to-bit 255 -bin %UNIQUE_FILE -test-runs 20 -no-progress-bar -j 0 | %GET_OUTCOME Masked % | %EQUALS 100

// The first test checks that -force-inject-to-bit works. We are forcing
// injetion to ymm0 bit 255. The byte should be converted from 0xdd to 0x5d.

// The second check tests that we only get masked errors for this workload.

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
