//-*- C++ -*-
// Classes for running the binaries.
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

#ifndef __RUNNER_H__
#define __RUNNER_H__

#include "addrSpace.h"
#include "debugstream.h"
#include "exitState.h"
#include "statistics.h"
#include "utils.h"
#include <cassert>
#include <climits>
#include <string>
#include <unistd.h>
#include <vector>

/// The injection status of the process.
enum class FtStatus {
  None,      ///< Uninitialized.
  Masked,    ///< Fault was injected but was not detected in any way.
  Exception, ///< Triggered an exception.
  InfExec,   ///< Infinite execution.
  Corrupted, ///< The output of the program is corrupted.
  Detected,  ///< The error was detected by a fault-injection technique.
  InjFailed, ///< Failed to an inject a fault.
  Skipped,   ///< Skipped checking.
};

/// The base class for the orig/test runners.
class RunnerBase {
protected:
  /// The Id of the current run.
  unsigned long Id = 0;

  /// The Process ID of the child process.
  pid_t ChildPID = 0;

  /// The child's terminal fd, as returned by forkpty().
  int ChildTerminalFd = -1;

  /// The Process ID of the parent process.
  pid_t ParentPID = 0;

  /// Execution time of the binary.
  double ExecTime = 0.0;

  /// The exit status of this run. We keep track of the exit type (i.e., exited
  /// normally/signaled/stopped and the exit code/signal number).
  ExecutionExitState ExState;

  /// The fault injection status of this run (Corruption, Masked, etc.).
  FtStatus FaultInjectionStatus;

  /// Arguments for execve().
  std::vector<const char *> Argv;

  /// Env for execve().
  std::vector<const char *> Argp;

  /// Remove temporary files.
  bool DoCleanup = true;

  /// Check that the arguments are valid, or exit.
  void sanityChecksOrExit();

public:
  /// Inspects the waitpid() \p Status and \returns the exit state.
  static ExitState getWaitPidExitState(int Status);
protected:

  /// Close open terminal to avoid "too many open files" error.
  void closeOpenTerminal() const;

  /// Run the workload. Note: This is non-blocking. \Returns false on failure.
  bool run(bool TimeoutAlarm = false);

  ~RunnerBase();

public:
  RunnerBase(long Id, bool DoCleanup);

  /// \Returns the exit state.
  const ExecutionExitState &getExecutionExitState() const { return ExState; }

  /// \Returns the exit state.
  const FtStatus &getFtStatus() const { return FaultInjectionStatus; }

  /// Run the workload, wait for it to finish and set \p ThreadFinished to true.
  virtual void runAndWait() = 0;
};

/// Runner for the original program run. This collects the original (correct)
/// stdout, stderr and the original execution time.

// This class launches binaries and
class OrigRunner : public RunnerBase {
public:
  OrigRunner(long Id, bool DoCleanup);
  /// In the original run we just run and wait to finish. No injection takes
  /// place, therefore there is no \p Stats to update.
  void runAndWait() override;
};

/// Runner for the test program.
class Runner : public RunnerBase {
  /// The full exit state of the original runner, including stdout/stderr.
  const ExecutionExitState *OrigExState = nullptr;

  /// Control the output checking. Currently it is turned off if we skip output
  /// redirection.
  bool SkipCheck = false;

  /// Wait until the child has stopped and return its status.
  FtStatus waitChildAndGetStatus();

  /// Statistics collection.
  Statistics *Stats = nullptr;

  /// Similar to system(), run \p Cmd, but using a custom \p Shell. \Returns
  /// true on success.
  static bool systemCustom(const char *Cmd, const char *Shell);

public:
  /// Test runs need to access data from the original timed run in \p OrigR.
  Runner(long Id, const ExecutionExitState *OrigExState, Statistics *Stats);

  /// Set injection time provided by user.
  void setUserInjectionTime(long UserInjectionTime);

  /// Get a random injection time point.
  double getRandomInjectionTime();

  /// Start the child process, inject the faults and wait for completion. When
  /// finished update \p Stats.
  void runAndWait() override;

  /// Start the injection threads. This blocks until all threads have joined.
  void launchInjectionThreads();

  /// Wait for \p SleepTime seconds, and stop the child.
  bool stopChildAfter(double SleepTime);

  /// Stop the execution of the child, and inject a fault.
  bool doBitFlip();

  /// Inject a fault by stopping at \p InjectionTime the child and performaing a
  /// bit-flip. \Returns true on success.
  bool tryInjectFaultAt(Statistics &Stats, double InjectionTime);

  /// Compares the origingal stdout, stderr and exit status against the new
  /// ones.
  /// \Returns true if no corruptions were found.
  bool checkOutput(const ExitState &State);
};

#endif // __RUNNER_H__
