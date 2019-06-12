// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -test-runs 4 -v 1 -no-progress-bar -bin-exec-time 0.1 -injections-per-run 0 -disable-timing-run | %GET_OUTCOME InfExec % | %EQUALS 100

// This checks whether we can capture an infinite execution correctly.

#include <stdio.h>
int main() {
  /* Infinite loop */
  while (1)
    ;
  return 0;
}
