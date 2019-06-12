// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -test-runs 1 -v 1 -no-progress-bar -injection-time 1 -injections-per-run 0 -no-redirect 2>&1 | %GREP -z 'argc:1.*argv0:%UNIQUE_FILE' 2>&1 > /dev/null
// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -test-runs 1 -v 1 -no-progress-bar -injection-time 1 -injections-per-run 0 -no-redirect -args test1 test2 test3 2>&1 | %GREP -z 'argc:4.*argv0:%UNIQUE_FILE.*argv1:test1.*argv2:test2.*argv3:test3' 2>&1 > /dev/null

// Tests that arguments are passed through the tool correctly.
// Should be tested with zero, and more arguments and with fault injection disabled.

#include <stdio.h>
int main(int argc, char **argv) {
  printf("argc:%d\n", argc);
  int i;
  for (i = 0; i != argc; ++i)
    printf("argv%d:%s\n", i, argv[i]);
  return 0;
}
