// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -test-runs 1 -v 1 -no-progress-bar -injections-per-run 0 | %GET_OUTCOME Corrupted % | %EQUALS 100

// Tests if a different exit code is caught as a corruption.
// NOTE: this is not guaranteed to fail, as the exit code is the pid.

#include <unistd.h>

int main() {

  pid_t pid = getpid();

  return pid;
}
