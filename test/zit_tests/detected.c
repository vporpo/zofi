// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -test-runs 20 -v 1 -no-progress-bar -injections-per-run 0 -detection-exit-code 66 | %GET_OUTCOME Detected % | %EQUALS 100

// This test checks whether error detection is measured properly by ZOFI.

#include <stdio.h>
int main() {
  return 66;
}
