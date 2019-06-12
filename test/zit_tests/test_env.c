// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && PraiseBob=1 %ZOFI -bin %UNIQUE_FILE -test-runs 1 -v 1 -no-progress-bar -injection-time 1 -injections-per-run 0 -no-redirect 2>&1 | %GREP 'PraiseBob=1' 2>&1 > /dev/null

// Tests that environmental variables are passed through to the binary.

#include <stdio.h>
int main(int argc, char **argv, char**argp) {
  char **Env;
  for (Env = argp; *Env; ++Env)
    printf("Env:%s\n", *Env);
  return 0;
}
