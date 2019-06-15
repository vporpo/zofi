// The entry point for the ZOFI tool.
//
// Copyright (C) 2019 Vasileios Porpodas <v.porpodas at gmail.com>
//
// This file is part of ZOFI.
//
// ZOFI is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2, or (at your option) any later
// version.
// GCC is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
// You should have received a copy of the GNU General Public License
// along with GCC; see the file LICENSE.  If not see
// <http://www.gnu.org/licenses/>.

#define _DEBUG
#include "config.h"
#include "debugstream.h"
#include "options.h"
#include "runner.h"
#include "threads.h"
#include "utils.h"

// Environmental variables.
char **Envp = nullptr;

int main(int argc, char **argv, char **envp) {
  Envp = envp;
  // Parse command line arguments.
  Options.init(argc, argv);

  // Initialize the random number generator.
  randInit();

  // Print them on screen.
  Dbg(1) << "------ Options ------\n";
  Dbg(1) << Options.getValuesStr();
  Dbg(1) << "---------------------\n";

  // If the user has not set the execution time of the binary, run once to
  // measure time and collect stdout, stderr.
  // This run blocks until the execution has finished.
  ExecutionExitState OrigState;

  // Note: This holds the exit state of the original runs. So its lifetime
  // should reach the execution of the test runs.
  ThreadScheduler<OrigRunner> OrigTS(true /* Keep Data */);
  // We run the original if we do not override either of: i. the bin execution
  // time, or ii. the exit state.
  if (!DisableTimingRun.getValue() &&
      (!BinExecTime.isSet() || !SetOrigExitState.isSet())) {
    Dbg(1) << "-- Original (Timing) Run --\n";

    auto OrigStart = getTime();
    // We spawn multiple threads even for the timing run, because modern
    // processors will turbo-boost when running on a single thread.
    unsigned NumOrigThreads = std::min(Jobs.getValue(), TestRuns.getValue());

    OrigTS.run(NumOrigThreads);

    double Duration = getTimeDiff(OrigStart, getTime());
    Dbg(1).precision(3) << "Original Duration: " << Duration << "s\n\n";
    // Don't override bin execution time if provided by user
    if (!BinExecTime.isSet())
      BinExecTime.setValue(Duration);
    else
      warning("Warning: binary execution time overrided by user: ",
              BinExecTime.getValue(), " s.");

    Dbg(2) << " Time: " << BinExecTime.getValue() << "s.\n";

    const OrigRunner *OrigR = OrigTS.getRunnerBack();
    OrigState = OrigR->getExecutionExitState();
  }

  // Sanity check.
  if (!BinExecTime.isSet() && TestRuns.getValue() != 0)
    userDie("No orig execution time available. Either remove "
            "-disable-timing-run, or set the -bin-exec-time\n");

  // Print a warning message if the execution time is too low.
  if (BinExecTime.getValue() < 0.05)
    warning("WARNING: Binary execution time ", BinExecTime.getValue(),
            "s is very low. The results may not be accurate, and " __BIN_NAME__
            " may fail.");

  // Set the exit state throught the command line. This will override the state
  // collected from the original run.
  if (SetOrigExitState.isSet())
    OrigState.import(SetOrigExitState.getValue());

  Statistics Stats;
  assert(BinExecTime.isSet() && "Expected orig exec time.");
  Stats.set<double>(Type::OrigExecTime, BinExecTime.getValue());

  // Run all tests.
  Dbg(1) << "-- Test Runs --\n";
  auto TimeBeginTests = getTime();
  ThreadScheduler<Runner> TS;
  TS.run(TestRuns.getValue(), &OrigState, &Stats);
  auto TimeEndTests = getTime();
  Stats.set<double>(Type::TestsExecTime,
                    getTimeDiff(TimeBeginTests, TimeEndTests));

  // Statistics
  Stats.set<unsigned long>(Type::NumTests, TestRuns.getValue());
  if (VerboseLevel.getValue() >= 1)
    Stats.dump();
  // Dump statistics to file.
  if (OutCsvFile.isSet())
    Stats.dumpToCSV();
  if (OutMoufoplotDir.isSet())
    Stats.dumpToMoufoplot();

  return 0;
}
