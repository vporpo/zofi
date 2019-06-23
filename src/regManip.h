//-*- C++ -*-
// Classes that access and modify the CPU's registers.
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

#ifndef __REGMANIP_H__
#define __REGMANIP_H__

#include <map>
#include <vector>
#include <string>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <csignal>
#include <cassert>
#include <tuple>
#include "utils.h"

/// Data attributes for each register.
class RegData {
  /// Register in gpregs/fpregs data structure
  uint8_t *RegPtr = nullptr;
  /// The first bit being touched by the register.
  int StartBit = -1;
  /// The register size in bits.
  int Bits = -1;

public:
  RegData() = default;
  RegData(uint8_t *RegPtr, int StartBit, int Bits)
      : RegPtr(RegPtr), StartBit(StartBit), Bits(Bits) {}
  uint8_t *getRegPtr() const {
    assert(RegPtr && "Uninitialized?");
    return RegPtr;
  }
  int getStartBit() const {
    assert(StartBit >= 0 && "Uninitialized?");
    return StartBit;
  }
  int getBits() const {
    assert(Bits > 0 && "Uninitialized?");
    return Bits;
  }
};

/// Describes a register.
struct RegDescr {
  RegDescr() = default;
  RegDescr(const std::string &Name, unsigned StartBit, unsigned Bits)
      : Name(Name), StartBit(StartBit), Bits(Bits) {}
  RegDescr(const std::string &Name, unsigned StartBit, unsigned Bits,
           bool Written)
      : Name(Name), StartBit(StartBit), Bits(Bits), Written(Written) {}
  /// The name of the register, e.g. "rax", "eax" etc.
  std::string Name;

  /// The first bit in the register that is accessible. For example, this is 4
  /// for the x86 register ah and 0 for most registers, including al.
  unsigned StartBit = 0;

  /// Size of register in bits.
  unsigned Bits = 0;

  /// True if this register is written, false if it is read.
  bool Written = false;

  /// Debug print to \p OS.
  void dump(std::ostream &OS) const;

  /// Debug rpint returning an std::string.
  std::string dumpStr() const;

  /// Debug print to std::cerr.
  DUMP_METHOD void dump() const;
};

class RegisterManipulator {
public:
  using RegsVec = std::vector<RegDescr>;

private:
  /// Convert register strings to regs
  std::map<std::string, RegData> StrToRegMap;

  /// The PID of the child process
  int ChildPid;

  /// General Purpose Registers (/usr/include/sys/user.h)
  user_regs_struct gpregs;

  /// Floating Point Registers (/usr/include/sys/user.h)
  user_fpregs_struct fpregs;

  /// Read all registers from the machine into gpregs and fpregs.
  void importRegisters();

  /// Read all registers from the machine into \p GpRegs and \p FpRegs.
  void importRegistersTo(user_regs_struct &GpRegs, user_fpregs_struct &FpRegs);
  
  /// Write all the register values of gpregs and fpregs into the machine.
  /// This can fail if we modify an illegal register and will \return false.
  bool exportRegisters() const;

  /// \Returns the first accessible bit of register \p R. This is usually zero,
  /// except for some registers like x86 ah where this is 4.
  int getStartBit(const std::string &R) const;

  /// \Returns the size of register \p R in bits.
  int getBits(const std::string &R) const;

  /// Return the register that corresponds to REGSTR
  const RegData &getRegDataForStrSafe(const std::string &regStr) const;

  /// \Returns the registers and their size in bits accessed by the instruction
  /// at \p IP.
  std::tuple<RegsVec, RegsVec, RegsVec> getInstrRegisters(uint8_t *ChildIP);

  /// \Returns the pointer to the value of \p Reg around \p Bit. 
  /// Note: this only updates the internal gpregs and fpregs. You need to
  /// exportRegisters() to update the processor's state.
  void *getRegisterPtr(const std::string &Reg, int Bit = 0);

  /// \Returns the contents of \p Reg and true on success.
  template <typename T>
  std::pair<T, bool> getRegisterContents(const std::string &Reg, int Bit = 0);

  /// Set \p Reg to \p Val around \p Bit. This sets a single byte. \Returns
  /// false if the register is not accessible.
  /// Note: this only updates the internal gpregs and fpregs. You need to
  /// exportRegisters() to update the processor's state.
  bool setRegisterContents(const std::string &Reg, uint8_t Val, int Bit = 0);

public:
  /// Constructor. Initialize strToRegMap.
  RegisterManipulator(int ChildPid);

  /// Flip the \p Bit of \p Reg. \Returns true on success.
  bool tryBitFlip(const std::string &Reg, unsigned Bit);

  /// \Returns a random register in the instruction at \p IP and a randomb bit
  /// in it. Returns false on failure.
  std::tuple<RegDescr, unsigned, bool> getRandomRegAndBit(uint8_t *IP);

  /// Returns the program counter.
  uint8_t *getProgramCounter();

  /// Read all machine registers and print them.
  void dumpMachine();

  /// Dump the contents of this class (gpregs and fpregs).
  void dump();
};
#endif //__REGMANIP_H__
