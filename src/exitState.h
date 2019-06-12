//-*- C++ -*-
// Classes describing the exit state of the binary.
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

#ifndef __EXIT_STATE_H__
#define __EXIT_STATE_H__

#include <linux/limits.h>
#include "utils.h"

// The maximum file name size.
#define FnameSz PATH_MAX

enum class ExitType {
  Invalid,
  Exited,
  Signaled,
  Stopped,
};

/// Describes the exit state of a program, that is the type of exit (exited,
/// stopped, signaled) and the exit code or signal number.
struct ExitState {
  ExitState() : Type(ExitType::Invalid), Val(666) {}
  /// Whether the program exited with a signal or normally.
  ExitType Type;
  /// The exit code if exited normally, or the signal number.
  int Val;
  ExitState(ExitType Type, int Val) : Type(Type), Val(Val) {}
  bool operator==(const ExitState &S2) const {
    return Type == S2.Type && Val == S2.Val;
  }
  /// Import the Type and Val from \p TypeStr and \p V.
  void import(const std::string &TypeStr, int V) {
    if (TypeStr == "exited" || TypeStr == "EXITED" || TypeStr == "Exited")
      Type = ExitType::Exited;
    else if (TypeStr == "signaled" || TypeStr == "SIGNALED" ||
             TypeStr == "Signaled")
      Type = ExitType::Signaled;
    else
      die("Invalid Exit Type:", TypeStr);

    Val = V;
  }
  /// \Returns true if the state has been initialized.
  bool isSet() const { return Type != ExitType::Invalid; }
  std::ostream &dump(std::ostream &OS) const;
  std::string getDumpStr() const;
};

/// Describes the exit state, including the stdout and stderr. This is used for
/// comparing the output of a test run against the original run.
class ExecutionExitState {
  /// The exit type and exit code / signal number.
  ExitState State;

  /// The file descriptor for the Stdout.
  int StdoutFd = -1;

  /// The file descriptor for the Stderr.
  int StderrFd = -1;

  /// The unique name of the file containing the dump of Stdout.
  char StdoutFile[FnameSz];

  /// The unique name of the file containing the dump of Stderr.
  char StderrFile[FnameSz];

public:
  ExecutionExitState();
  /// Creates the stdout/stderr files. We use \p the Id of this run as part of
  /// the file for being able to track the outputs when debugging.
  void initFiles(long Id);

  /// Return true if the state has been initialized.
  bool isSet() const;

  /// Getters and setters for the class data.
  const ExitState &getExitState() const { return State; }
  void setExitState(const ExitState &ExState) { State = ExState; }
  const char *getStdoutFile() const { return StdoutFile; }
  const char *getStderrFile() const { return StderrFile; }
  int getStdoutFd() const { return StdoutFd; }
  int getStderrFd() const { return StderrFd; }
  /// Parse \p Str and import the execution state from it.
  void import(const char *Str);
  /// Comparison using the defaultDiff().
  bool operator==(const ExecutionExitState &State2) const;
};

#endif //__EXIT_STATE_H__

