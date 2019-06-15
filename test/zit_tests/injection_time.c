// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -test-runs 1 -no-progress-bar -injection-time 0.5 | %GET_OUTCOME Masked N | %EQUALS 1

// Checks if -injection-time works with doubles

#include <unistd.h>
int main() {
  sleep(1);
  return 0;
}
