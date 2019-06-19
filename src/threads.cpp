// Thread launcher for parallel runs.
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

#include "threads.h"
#include "options.h"
#include "progressbar.h"
#include "runner.h"
#include "statistics.h"
#include <algorithm>

/// Helper function for incrementing statistics for \p S.
static void incrStatsCounter(Statistics *Stats, FtStatus S) {
  switch (S) {
  case FtStatus::None:
    assert(0 && "Unreachable");
    break;
  case FtStatus::Masked:
    Stats->incr(Type::Masked);
    break;
  case FtStatus::Exception:
    Stats->incr(Type::Exception);
    break;
  case FtStatus::InfExec:
    Stats->incr(Type::InfExec);
    break;
  case FtStatus::Corrupted:
    Stats->incr(Type::Corrupted);
    break;
  case FtStatus::Detected:
    Stats->incr(Type::Detected);
    break;
  case FtStatus::InjFailed:
    Stats->incr(Type::InjFailed);
    break;
  case FtStatus::Skipped:
    Stats->incr(Type::Skipped);
    break;
  }
}

void OrigJobScheduler::jobFinishedParentCode(const JobData &Data) {
  // The first run sets the OrigExitState to be used by the test runs.
  if (Data.Id == 0)
    read(Data.Pipe[0], &OrigExitState, sizeof(OrigExitState));
}

void TestJobScheduler::jobFinishedParentCode(const JobData &Data) {
  // Get the fault injection status from child process.
  FtStatus FaultStatus;
  read(Data.Pipe[0], &FaultStatus, sizeof(FaultStatus));
  incrStatsCounter(Stats, FaultStatus);
}

void JobSchedulerBase::waitForJob() {
  int Status;
  pid_t Pid = waitpid(-1, &Status, 0);
  auto State = Runner::getWaitPidExitState(Status);
  assert(WIFEXITED(Status) && "Expected child to have exited.");
  auto HasPid = [&](const JobData &Data) { return Data.ChildPID == Pid; };
  auto it = std::find_if(ActiveJobs.begin(), ActiveJobs.end(), HasPid);
  assert(it != ActiveJobs.end() && "Pid not found in Active Jobs!");
  int ChildPipe[2] = {it->Pipe[0], it->Pipe[1]};

  jobFinishedParentCode(*it);

  // Cleanup
  close(ChildPipe[0]);
  ActiveJobs.erase(it);
}

void JobSchedulerBase::run(unsigned long TotalNumJobs) {
  ProgressBar Bar(TotalNumJobs, 30, std::cout);
  unsigned BarCnt = 0;
  bool ShowingBar = VerboseLevel.getValue() == 1 && !NoProgressBar.getValue();
  if (ShowingBar)
    Bar.init();

  for (unsigned Id = 0; Id != TotalNumJobs; ++Id) {
    // Block until we can spawn a new process.
    while (ActiveJobs.size() == Jobs.getValue())
      waitForJob();

    Dbg(2) << "-------------------------\n";
    dbg(2) << "Job " << Id << " begin\n";
    Dbg(2) << "-------------------------\n";

    // Update the progress bar. Don't display it for Verbose > 1 as it will mess
    // up the dumps.
    if (ShowingBar)
      Bar.display(BarCnt++);

    // Set up a pipe for communication from child to parent.
    pipeSafe(Pipe);
    ActiveJobs.push_back(JobData(Id, Pipe));

    // Get a random seed for the child process.
    unsigned SeedForChild = randSafe();

    // Launch thread and insert the ThreadLauncher into the set.
    pid_t ChildJobPID = forkSafe();
    if (ChildJobPID == 0) {
      // Child process
      close(Pipe[0]);

      // Initialize the rand() seed for this process with a random value.
      setRandSeed(SeedForChild);

      childJobCode(Id);

      close(Pipe[1]);
      exit(0);
    } else if (ChildJobPID > 0) {
      // Parent process.
      close(Pipe[1]);
      parentJobCode(Id);

      ActiveJobs.back().ChildPID = ChildJobPID;
    } else
      die("Job fork() failed");
  }
  // Busy loop until all threads have joined.
  while (ActiveJobs.size() > 0) {
    waitForJob();
    if (ShowingBar)
      Bar.display(BarCnt++);
  }
  if (ShowingBar)
    Bar.finalize();
}

void OrigJobScheduler::childJobCode(unsigned Id) {
  // Child process.
  OrigRunner OR(Id, NoCleanup);
  OR.runAndWait();
  // Send exit state to parent process.
  auto ExState = OR.getExecutionExitState();
  write(Pipe[1], &ExState, sizeof(ExState));
}

void OrigJobScheduler::parentJobCode(unsigned Id) {
}

void TestJobScheduler::childJobCode(unsigned Id) {
  Runner TR(Id, OrigExState, Stats);
  TR.runAndWait();
  // Send this run's fault injection status to parent.
  auto FaultStatus = TR.getFtStatus();
  write(Pipe[1], &FaultStatus, sizeof(FaultStatus));
}

void TestJobScheduler::parentJobCode(unsigned Id) {
}
