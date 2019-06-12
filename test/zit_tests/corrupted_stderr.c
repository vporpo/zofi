// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -test-runs 20 -v 1 -no-progress-bar -injections-per-run 0 | %GET_OUTCOME Corrupted % | %EQUALS 100

// Tests if a different output in stdout is caught as a corruption.

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
  struct timeval tv;
  struct timezone tz;
  tz.tz_minuteswest = 0;
  tz.tz_dsttime = 0;
  gettimeofday(&tv, &tz);

  pid_t pid = getpid();

  fprintf(stderr, "%ld:%ld:%ld\n", tv.tv_sec, tv.tv_usec, (long) pid);
  return 0;
}
