//-*- C++ -*-
// An interface for inspecting the address space of a process.
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

#ifndef __ADDRSPACE_H__
#define __ADDRSPACE_H__

#include <cassert>
#include <set>
#include <string>

/// Holds the information about the address space of a process.
class AddressSpace {
  /// Holds an address mapping entry.
  struct Mapping {
    /// Address space start.
    unsigned long From = 0;
    /// Address space end.
    unsigned long To = 0;
    /// The path mapped to this address space.
    std::string Path;
    /// Normal entry constructor.
    Mapping(unsigned long From, unsigned long To, const char *Path)
        : From(From), To(To), Path(Path) {
      assert(From <= To && "Expected range.");
    }
    /// Query constructor for checking if \p Addr is in the address space.
    Mapping(unsigned long Addr) : Mapping(Addr, Addr, "Query") {}
    /// Methods to support std::set.
    bool operator<(const Mapping &M2) const {
      // Equivalence is checked by comparing reflexively, so query mappings
      // should return false in A<B && !B<A.
      return From < M2.From && To < M2.To;
    }
    /// Debug print.
    void dump() const {
      fprintf(stderr, "0x%lx-0x%lx %s\n", From, To, Path.c_str());
    }
  };

  /// The process id.
  pid_t Pid = 0;

  /// Map start address to end address and path.
  std::set<Mapping> MappingsSet;

  /// Parse /proc/<Pid>/maps to extract the complete address mapping.
  void parse();

public:
  /// Initialize the address space for process \p Pid.
  AddressSpace(pid_t Pid);

  /// \Returns true if \p Addr is in the address space mapped to a library.
  bool isInLibrary(unsigned long Addr) const;

  /// Debug print.
  void dump() const;
};

#endif //__ADDRSPACE_H__
