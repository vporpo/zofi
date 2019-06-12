// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -test-runs 1 -injections-per-run 0 -v 1 -no-progress-bar | %GET_OUTCOME Masked % | %EQUALS 100

// Checks if the application-triggered segfault is reported as such by zofi

int A[8];

int main () {
  // This should segfault
  A[1000000] = 1;
  return 0;
}
