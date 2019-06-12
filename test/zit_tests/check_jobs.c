// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -test-runs 1 -v 1 -no-progress-bar -injections-per-run 0 -j 0
// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -test-runs 1 -v 1 -no-progress-bar -injections-per-run 0 -j -1
// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -test-runs 1 -v 1 -no-progress-bar -injections-per-run 0 -j 66666

// Just check if the -j options do not cause any weird failures.

int main() {
  return 0;
}
