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

#include "addrSpace.h"
#include "utils.h"
#include <climits>
#include <fstream>

void AddressSpace::parse() {
  // Parse /proc/<Pid>/maps to extract the address range and file path.
  std::string ProcFile("/proc/" + std::to_string(Pid) + "/maps");
  std::fstream FS(ProcFile, std::fstream::in);
  if (FS.fail())
    die(__FUNCTION__, "(): Failed to open ", ProcFile);

  std::string Line;
  while (std::getline(FS, Line) && !FS.eof()) {
    unsigned long From, To;
    char Permissions[5];
    unsigned long Num;
    char Dev[6];
    unsigned long Inode;
    char Path[PATH_MAX];
    sscanf(Line.c_str(), "%lx-%lx %4s %lx %5s %ld %s", &From, &To, Permissions,
           &Num, Dev, &Inode, Path);
    MappingsSet.insert(Mapping(From, To, Path));
  }
  FS.close();
}

AddressSpace::AddressSpace(pid_t Pid) : Pid(Pid) { parse(); }

bool AddressSpace::isInLibrary(unsigned long Addr) const {
  Mapping AddrMapping(Addr);
  auto It = MappingsSet.find(AddrMapping);
  if (It == MappingsSet.end()) {
    dump();
    die("Could not find addr ", Addr, " in address space");
  }
  const Mapping &FoundMapping = *It;
  const std::string &Path = FoundMapping.Path;
  // Return true if the paths ends in .so
  std::string So = ".so";
  bool Found = Path.rfind(So) == Path.size() - So.size();
  return Found;
}

void AddressSpace::dump() const {
  for (const auto &Entry : MappingsSet)
    Entry.dump();
}
