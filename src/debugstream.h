//-*- C++ -*-
// A debugging stream with multiple levels of output, controlled by a verbosity
// level.
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

#ifndef _DEBUGSTREAM_H__
#define _DEBUGSTREAM_H__

#include <iostream>
#include <mutex>

/// Prefix the debug output with "function(): "
#define dbg(LEVEL) (Dbg(LEVEL) << __FUNCTION__ << "(): ")

/// Dbg is the debug output stream. We can control its verbosity by specifying
/// the verbosity level in its constructor. For example:
/// \verbatim
/// Dbg(1) << "Super important, on by default!\n";
/// Dbg(9) << "Less important, needs verbosity 9\n";
/// \endverbatim
/// Dbg is also thread safe, so mutliple threads can safely use it at once.
class Dbg {
  std::ostream &OS;
  int ProgramVerboseLevel = 0;
  int MsgLevel = 0;
  /// Mutex for thread safety.
  static std::mutex Mtx;

public:
  Dbg(int MsgLevel = 1, std::ostream &OS = std::cerr);

  /// Operator<<
  template <typename T> Dbg &operator<<(const T &str) {
    if (ProgramVerboseLevel >= MsgLevel) {
      // OS is std::cerr by default, so use a mutex for thread safety.
      Mtx.lock();
      OS << str;
      Mtx.unlock();
    }
    return *this;
  }

  /// Set precision for floating point printing.
  Dbg precision(int P);

  /// Debug print.
  void dump() const;
};

#endif // _DEBUGSTREAM_H__
