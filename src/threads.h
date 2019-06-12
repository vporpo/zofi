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
#include <vector>

/// Starts a runner thread and tracks when it finishes.
template <typename RunnerT> class ThreadLauncher {
  /// The index of this run. Used for debugging.
  long Id = -1;
  /// True if the thread has joined.
  bool Finished = false;
  /// The runner that will be launched.
  RunnerT R;
  /// The thread being executed.
  std::thread T;

public:
  /// The \p OrigExState is the original run's exit state. \p Stats is used to
  /// collect the injection statistics across all threads.
  ThreadLauncher(long Id, const ExecutionExitState *OrigExState,
                 Statistics *Stats);
  ~ThreadLauncher();
  /// \Returns true if the thread has finished.
  bool hasFinished() const { return Finished; }
  /// \Returns the runner.
  const RunnerT &getRunner() const { return R; }
};

template <typename RunnerT> class ThreadScheduler {
  /// If this flag is set to true, it stops the thread data from being
  /// destroyed, until the scheduler is destroyed.
  bool KeepData = false;

  /// Vector of finished threads.
  std::vector<std::unique_ptr<ThreadLauncher<RunnerT>>> KeptThreads;

  /// Set of active threads.
  std::vector<std::unique_ptr<ThreadLauncher<RunnerT>>> ActiveThreads;

  /// Check if any of the active threads can join and if so remove it from the
  /// active set. \Returns the number of threads joined.
  unsigned tryJoinActiveThreads();

public:
  /// If \p KeepData is true, we keep the thread data until this object is
  /// destructed.
  ThreadScheduler(bool KeepData = false);
  /// Launch \p TotalNumThreads threads.
  void run(unsigned long TotalNumThreads,
           const ExecutionExitState *OrigExState = nullptr,
           Statistics *Stats = nullptr);

  /// \Returns the last runner in the kept data.
  const RunnerT *getRunnerBack() const;
};

template class ThreadLauncher<OrigRunner>;
template class ThreadScheduler<OrigRunner>;

template class ThreadLauncher<Runner>;
template class ThreadScheduler<Runner>;

#endif //__THREADS_H__
