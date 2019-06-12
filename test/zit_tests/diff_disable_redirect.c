// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -test-runs 1 -v 1 -no-progress-bar -injections-per-run 0 -diff-cmd 'echo Praise_Bob' -diff-disable-redirect | %GREP Praise_Bob > /dev/null 2> /dev/null
// RUN: %CC %THIS_FILE -o %UNIQUE_FILE && %ZOFI -bin %UNIQUE_FILE -test-runs 1 -v 1 -no-progress-bar -injections-per-run 0 -diff-cmd 'echo Praise_Bob' | %GREP -v Praise_Bob > /dev/null 2> /dev/null

// Tests if the custom -diff-disable-redirect works properly

int main() {
  return 0;
}
