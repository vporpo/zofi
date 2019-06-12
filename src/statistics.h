//-*- C++ -*-
// Data structures for the fault injection statistics.
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

#ifndef __STATISTICS_H__
#define __STATISTICS_H__

#include <map>
#include <mutex>

enum class Type {
  Masked,    ///< Fault was injected but was not detected in any way.
  Exception, ///< Triggered an exception.
  InfExec,   ///< Infinite execution.
  Corrupted, ///< The output of the program is corrupted.
  Detected,  ///< The error was detected by a fault-injection technique.
  InjFailed, ///< Failed to an inject a fault.
  Skipped,   ///< Skipped checking.

  OrigExecTime,  ///< The execution time of the original run in msecs.
  TestsExecTime, ///< The execution time of all the test runs in msecs.
  NumTests,      ///< The total number of tests.

  Binary,   ///< The binary being tested.
  InjectTo, ///< Where we are injecting errors.
  TestRuns, ///< Number of program runs.
};

static inline const char *getTypeStr(Type S) {
  switch (S) {
  case Type::Masked:
    return "Masked";
  case Type::Exception:
    return "Exception";
  case Type::InfExec:
    return "InfExec";
  case Type::Corrupted:
    return "Corrupted";
  case Type::Detected:
    return "Detected";
  case Type::InjFailed:
    return "InjectionFailed";
  case Type::Skipped:
    return "Skipped";
  case Type::OrigExecTime:
    return "OrigExecTime";
  case Type::TestsExecTime:
    return "TestsExecTime";
  case Type::NumTests:
    return "NumTests";

  case Type::Binary:
    return "Binary";
  case Type::InjectTo:
    return "InjectTo";
  case Type::TestRuns:
    return "TestRuns";
  }
  return "Bad State";
}

/// Summarizes data about the injection outcomes.
class Statistics {
  /// Mutex for thread safety.
  std::mutex Mtx;

  std::map<Type, unsigned long> ULongMap;
  std::map<Type, std::string> StringMap;
  std::map<Type, double> DoubleMap;

  /// Total count of runs where injection did not fail.
  unsigned long TotalInjOK = 0;
  /// The grand total of all runs.
  unsigned long GrandTotal = 0;

public:
  Statistics();
  /// Zero out all counters.
  void zero();
  /// Increment counter for \p S. Note: this is thread safe.
  void incr(Type S);
  /// Set statistic \p S to \p Val.
  template <typename T> void set(Type S, T Val);
  /// Get string form of statistic \p S.
  std::string getStr(Type S);
  /// Debug print.
  void dump();
  /// Dump csv to file.
  void dumpToCSV();
  /// Dump csv to a Moufoplot compatible file.
  void dumpToMoufoplot();
};

#endif //__STATISTICS_H__
