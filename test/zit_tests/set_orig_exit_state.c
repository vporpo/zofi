// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -test-runs 1 -v 1 -no-progress-bar -injections-per-run 0 -set-orig-exit-state /dev/null,/dev/null,exited:66 2>&1 | %GET_OUTCOME Masked % | %EQUALS 100
// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -test-runs 1 -v 1 -no-progress-bar -injections-per-run 0 -set-orig-exit-state /dev/null,/dev/null,exited:0 2>&1 | %GET_OUTCOME Corrupted % | %EQUALS 100
// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -test-runs 1 -v 1 -no-progress-bar -injections-per-run 0 -set-orig-exit-state /dev/null,/dev/null,signaled:666 2>&1 | %GET_OUTCOME Corrupted % | %EQUALS 100

// Checks that overriding the exit state of the orignal run works properly.
// FIXME: this is not currently checking the stdout/stderr files.

int main() {
  return 66;
}
