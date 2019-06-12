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

#include <iostream>
#include "debugstream.h"
#include "options.h"

Dbg::Dbg(int MsgLevel, std::ostream &OS)
  : OS(OS), ProgramVerboseLevel(VerboseLevel), MsgLevel(MsgLevel) {}

Dbg Dbg::precision(int P) {
  std::cerr.precision(P);
  return *this;
}

void Dbg::dump() const {
  std::cerr << "ProgramVerboseLevel: " << ProgramVerboseLevel << "\n";
  std::cerr << "MsgLevel: " << MsgLevel << "\n";
}


std::mutex Dbg::Mtx;
