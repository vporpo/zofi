// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -inject-to 'rwc' -test-runs 1 -v 1 -no-progress-bar -injection-time 1 -injections-per-run 0 -no-redirect 2>&1 > /dev/null
// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -inject-to 'rwco' -test-runs 1 -v 1 -no-progress-bar -injection-time 1 -injections-per-run 0 -no-redirect 2>&1 > /dev/null
// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -inject-to 'rweico' -test-runs 1 -v 1 -no-progress-bar -injection-time 1 -injections-per-run 0 -no-redirect 2>&1 > /dev/null

// Check that -inject-to 'rwc', 'rwco', 'rweico' work.
int main() {
  return 0;
}
