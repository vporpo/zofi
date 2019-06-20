// RUN: %CC %THIS_FILE -lpthread -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -test-runs 8 -v 1 -no-progress-bar -j 0 | %GET_OUTCOME Masked % | %GREATER_THAN 70

// Check that we we support multiple threads.

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>


void *foo(void *Arg) {
  fprintf(stderr, "Child:\n");
  sleep(1);
  int RetVal = 0;
  pthread_exit(&RetVal);
}

int main() {
  fprintf(stderr, "Parent\n");
  pthread_t Thread1, Thread2;
  int pid1 = pthread_create(&Thread1, NULL, foo, 0);
  int pid2 = pthread_create(&Thread2, NULL, foo, 0);
  void *RetVal[2];
  pthread_join(Thread1, &RetVal[0]);
  pthread_join(Thread2, &RetVal[0]);
  fprintf(stderr, "Exiting\n");
  return 0;
}
