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

template <typename RunnerT>
ThreadLauncher<RunnerT>::ThreadLauncher(long Id,
                                        const ExecutionExitState *OrigState,
                                        Statistics *Stats)
    : Id(Id), R(Id, OrigState), T(&RunnerT::runAndWait, &R, &Finished, Stats) {}

template <typename RunnerT> ThreadLauncher<RunnerT>::~ThreadLauncher() {
  T.join();
}

template <typename RunnerT>
ThreadScheduler<RunnerT>::ThreadScheduler(bool KeepData) : KeepData(KeepData) {}

template <typename RunnerT>
unsigned ThreadScheduler<RunnerT>::tryJoinActiveThreads() {
  unsigned CntJoined = 0;
  for (auto It = ActiveThreads.begin(); It != ActiveThreads.end();) {
    const auto &TPtr = *It;
    if (TPtr->hasFinished()) {
      if (KeepData)
        KeptThreads.push_back(std::move(*It));
      It = ActiveThreads.erase(It);
      CntJoined++;
      dbg(2) << "Thread joined\n";
    } else
      ++It;
  }
  return CntJoined;
}

template <typename RunnerT>
void ThreadScheduler<RunnerT>::run(unsigned long TotalNumThreads,
                                   const ExecutionExitState *OrigExState,
                                   Statistics *Stats) {
  ProgressBar Bar(TotalNumThreads, 30, std::cout);
  unsigned BarCnt = 0;
  bool ShowingBar = VerboseLevel.getValue() == 1 && !NoProgressBar.getValue();
  if (ShowingBar)
    Bar.init();

  for (unsigned Id = 0; Id != TotalNumThreads; ++Id) {
    // Busy loop until we can spawn a new thread.
    unsigned CntJoined = 0;
    while (ActiveThreads.size() == Jobs.getValue()) {
      usleep(1000); // 1ms
      CntJoined = tryJoinActiveThreads();
    }

    Dbg(2) << "-------------------------\n";
    dbg(2) << "Thread " << Id << " begin\n";

    // Update the progress bar. Don't display it for Verbose > 1 as it will mess
    // up the dumps.
    if (ShowingBar)
      for (unsigned Cnt = 0; Cnt != CntJoined; ++Cnt)
        Bar.display(++BarCnt);

    // Launch thread and insert the ThreadLauncher into the set.
    ActiveThreads.push_back(
        std::make_unique<ThreadLauncher<RunnerT>>(Id, OrigExState, Stats));
  }
  // Busy loop until all threads have joined.
  while (!ActiveThreads.empty()) {
    unsigned CntJoined = tryJoinActiveThreads();
    if (ShowingBar)
      for (unsigned Cnt = 0; Cnt != CntJoined; ++Cnt)
        Bar.display(++BarCnt);
    usleep(1000); // 1ms
  }
  if (ShowingBar) {
    Bar.display(TotalNumThreads);
    Bar.finalize();
  }
}

template <typename RunnerT>
const RunnerT *ThreadScheduler<RunnerT>::getRunnerBack() const {
  assert(KeepData && "This only works if we have kept the thread data");
  assert(!KeptThreads.empty() && "Nothing to return");
  return &KeptThreads.back()->getRunner();
}
