// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -test-runs 4 -v 1 -no-progress-bar -injections-per-run 0 -diff-cmd '/bin/ls %ORIG_STDOUT' | %GET_OUTCOME Masked % | %EQUALS 100

// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -test-runs 4 -v 1 -no-progress-bar -injections-per-run 0 -diff-cmd "/usr/bin/diff %ORIG_STDOUT %TEST_STDERR" | %GET_OUTCOME Corrupted % | %EQUALS 100

// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -test-runs 4 -v 1 -no-progress-bar -injections-per-run 0 -diff-cmd "/usr/bin/diff <(%GREP -v '^Variable' %ORIG_STDOUT) <(%GREP -v '^Variable' %TEST_STDOUT)" | %GET_OUTCOME Masked % | %EQUALS 100

// Tests if the custom -diff-cmd works properly

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

  printf("Consistent output\n");

  printf("Variable output: %ld:%ld:%ld\n", tv.tv_sec, tv.tv_usec, (long) pid);
  return 0;
}
