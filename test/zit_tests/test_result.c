// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -test-runs 40 -v 1 -no-progress-bar -j 0 | %GET_OUTCOME Exception N | %GREATER_THAN 10
// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -test-runs 40 -v 1 -no-progress-bar -j 0 | %GET_OUTCOME Corrupted N | %GREATER_THAN 2

// Note: This test may occasionally fail, as the checks are based on fault counters.

#include <stdio.h>
int A[16];
int main() {
  int i, j;
  for (j = 0; j != 2000000; ++j)
    for (i = 0; i != 16; ++i)
      A[i] += i;

  for (i = 0; i != 16; ++i)
    printf("%d,", A[i]);
  printf("\n");
  return 0;
}
