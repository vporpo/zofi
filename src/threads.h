//-*- C++ -*-
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

#ifndef __THREADS_H__
#define __THREADS_H__

#include "options.h"
#include "runner.h"
#include "statistics.h"
#include <thread>
#include <set>

/// Data for tracking the active jobs.
struct JobData {
  pid_t ChildPID = 0;
  int Id = -1;
  int Pipe[2] = {0, 0};
  JobData(int Id, int Pipe2[2]) : Id(Id) {
    Pipe[0] = Pipe2[0];
    Pipe[1] = Pipe2[1];
  }
};

/// Base class for scheduler classes.
class JobSchedulerBase {
protected:
  /// ActiveJobs and their communication pipe.
  std::vector<JobData> ActiveJobs;

  /// Check if any of the active threads can join and if so remove it from the
  /// active set. \Returns the number of threads joined.
  unsigned tryJoinActiveThreads();

  /// Communication pipe from child to parent.
  int Pipe[2];

  /// Wait for a job to finish and cleanup.
  void waitForJob();

  /// The code run right after the fork.
  virtual void childJobCode(unsigned Id) = 0;

  /// The code run by the parent.
  virtual void parentJobCode(unsigned Id) = 0;

  /// Parent code once job has finished.
  virtual void jobFinishedParentCode(const JobData &Data) = 0;

public:
  JobSchedulerBase() = default;

  /// Launch \p TotalNumThreads threads.
  void run(unsigned long TotalNumThreads);
};

/// Scheduler for the original runs.
class OrigJobScheduler : public JobSchedulerBase {
  /// The exit state of the original run.
  ExecutionExitState OrigExitState;

  /// The child code run right after the fork.
  void childJobCode(unsigned Id);

  /// The parent code run after the fork.
  void parentJobCode(unsigned Id);

  void jobFinishedParentCode(const JobData &Data);

public:
  OrigJobScheduler() = default;

  /// \Returns the exit state of the original run.
  const ExecutionExitState &getOrigExitState() const { return OrigExitState; }
};

/// Scheduler for test runs.
class TestJobScheduler : public JobSchedulerBase {
  const ExecutionExitState *OrigExState = nullptr;

  /// Statistics.
  Statistics *Stats = nullptr;

  /// The child code run right after the fork.
  void childJobCode(unsigned Id);

  /// The parent code run after the fork.
  void parentJobCode(unsigned Id);

  void jobFinishedParentCode(const JobData &Data);

public:
  TestJobScheduler(const ExecutionExitState *OrigExState, Statistics *Stats)
      : OrigExState(OrigExState), Stats(Stats) {}
};

#endif //__THREADS_H__
