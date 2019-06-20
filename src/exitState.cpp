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

#include <sstream>
#include "exitState.h"
#include "options.h"

std::ostream &ExitState::dump(std::ostream &OS) const {
  OS << "{";
  switch (Type) {
  case ExitType::Exited:
    OS << "Exited";
    break;
  case ExitType::Signaled:
    OS << "Signaled";
    break;
  case ExitType::Stopped:
    OS << "Stopped";
    break;
  case ExitType::Invalid:
    OS << "Invalid";
    break;
  }
  OS << ", " << Val << "}";
  return OS;
}

std::string ExitState::getDumpStr() const {
  std::stringstream SS;
  dump(SS);
  return SS.str();
}

ExecutionExitState::ExecutionExitState() {
  StdoutFile[0] = '\0';
  StderrFile[0] = '\0';
}

bool ExecutionExitState::isSet() const {
  return StdoutFd != -1 && StderrFd != -1 && State.isSet();
}

void ExecutionExitState::initFiles(long Id) {
  // Open the Stdout and Stderr file descriptors.
  snprintf(StdoutFile, FnameSz, "%s.%d.XXXXXX", Stdout.getValue().c_str(), Id);
  snprintf(StderrFile, FnameSz, "%s.%d.XXXXXX", Stderr.getValue().c_str(), Id);
  StdoutFd = mkstempSafe(StdoutFile);
  StderrFd = mkstempSafe(StderrFile);
}

void ExecutionExitState::import(const char *Str) {
// /path/to/stdout,/path/to/stderr,{exit:<EXIT_CODE>,signaled:<SIGNAL>}
#define MaxTySz 10
  char ExitTypeStr[MaxTySz];
  int Val;
  int Cnt = sscanf(Str, "%" STRFY(FnameSz) "[^,],%" STRFY(
                            FnameSz) "[^,],%" STRFY(MaxTySz) "[^:]:%d",
                   StdoutFile, StderrFile, ExitTypeStr, &Val);
  if (Cnt != 4 || errno != 0 || Cnt == EOF)
    userDie("Bad exit state argument: ", Str, ".");
  // Check if files exist.
  for (const char *File : {StdoutFile, StderrFile})
    if (!fileExists(File))
      userDie("File ", File, " does not exist.");
  State.import(ExitTypeStr, Val);
}

bool ExecutionExitState::operator==(const ExecutionExitState &OtherState) const {
  int OtherStdoutFd = openSafe(OtherState.getStdoutFile(), O_RDONLY);
  int OtherStderrFd = openSafe(OtherState.getStderrFile(), O_RDONLY);
  bool Same = State == OtherState.getExitState() &&
              defaultDiff(StdoutFd, OtherStdoutFd) &&
              defaultDiff(StderrFd, OtherStderrFd);

  closeSafe(OtherStdoutFd);
  closeSafe(OtherStderrFd);
  return Same;
}

