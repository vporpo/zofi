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
  /// Register in gpregs/vectoreg data structure
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

//
// | 112 |  96 |  80 |  64 |  48 |  32 |  16 |   0 | bits
// +-----+-----+-----+-----+-----+-----+-----+-----+-----   -      =======
// |          FIP          | FOP | FTW | FSW | FCW |    0   |         |
// +-----+-----+-----+-----+-----+-----+-----+-----+-----   |         |
// | MXCSR_MASK|   MXCSR   |         FDP           |  128   |         |
// +-----------------+-----------------------------+-----   |         |
// |                 |        ST(0)/MM0            | 256   160bytes   |
// |                 |           ...               |        |         |
// |                 |        ST(7)/MM7            | 1152   |         |
// +-----------------+-----------------------------+-----   -    Legacy  512b
// |                    XMM0                       | 1280   |       area
// |                     ...                       |       256bytes   |
// |                    XMM15                      | 3200   |         |
// +-----------------------------------------------+-----   -         |
// |                                  reserved     | 3328   |         |
// |                                    ...        |       96bytes    |
// |                                   unused      | 3968   |         |
// +-----------------------+-----------------------+-----   -      =======
// |       XCOMP_BV        |     XSTATE_BV         | 4096  16bytes    |
// +-----------------------+-----------------------+-----   -         |
// |                    reserved                   |       48bytes XSAVE hdr 64b
// |                                               |        |         |
// +-----------------------------------------------+-----   -      =======
// |               YMM_Hi128 YMM0_Hi               | 4608   |         |
// |                      ...                      |    256bytes YMM_Hi128 256b
// |                    YMM15_Hi                   |        |         |
// +-----------------------+-----------------------+-----   -      =======
// |   AVX512 opmask k1    |   opmask k0           | 6656   |         |   AVX-512
// |                       |       ...             |       64bytes   opmask 64b
// |                 k7    |          k6           |        |         |
// +-----------------------+-----------------------+-----   -         -
// |                  ZMM0_Hi256                   | 7168   |         |
// |                      ...                      |     512bytes  ZMM_Hi256 512b
// |                 ZMM15_Hi256                   |        |         |
// +-----------------------------------------------+-----   -         -
// |               ZMM16 full 512                  | 11264  |         |
// |                      ...                      |    1024bytes  Hi16_ZMM 1024b
// |               ZMM31 full 512                  |        |         |
// +-----------------------------------------------+-----   -      =======

static constexpr const uint32_t XMMBytes = 16;   // Full: 0-128bits
static constexpr const uint32_t YMMHiBytes = 16; // Hi: 128-256bits
static constexpr const uint32_t YMMBytes = 32;   // Full: 0-256bits
static constexpr const uint32_t OpmaskBytes = 8; // Full: 0-64bits
static constexpr const uint32_t ZMMHiBytes = 32; // Hi: 256-512bits
static constexpr const uint32_t ZMMBytes = 64;   // Full: 0-512bits

static constexpr const uint32_t MMBytes = 16;    // Full: 0-80bits (+ padding)

/// Holds the buffer returned by PTRACE_GETREGSET NT_X86_STATE.
struct XSave {
  uint8_t legacy_fsave_fcw[2];
  uint8_t legacy_fsave_fsw[2];
  uint8_t legacy_fsave_ftw[2];
  uint8_t legacy_fsave_fop[2];
  uint8_t legacy_fsave_fip[8];
  uint8_t legacy_fsave_fdp[8];
  uint8_t legacy_fsave_mxcsr[4];
  uint8_t legacy_fsave_mxcsr_mask[4];

  uint8_t legacy_mm_0to7[8][MMBytes];
  uint8_t legacy_xmm_0to15[16][XMMBytes];
  uint8_t legacy_reserved[96];
  uint8_t header_xstate_bv[8];
  uint8_t header_xcomp_bv[8];
  uint8_t header_reserved[48];
  uint8_t ymm_hi128_0to15[16][YMMHiBytes];
  uint8_t opmask_0to7[8][OpmaskBytes];
  uint8_t zmm_hi256_0to15[16][ZMMHiBytes];
  uint8_t zmm_full_16to31[16][ZMMBytes];

  uint8_t &getZMMInternal(unsigned Reg, unsigned Byte) {
    if (Reg <= 15) {
      if (Byte <= XMMBytes)
        return legacy_xmm_0to15[Reg][Byte];
      else if (Byte <= YMMBytes)
        return ymm_hi128_0to15[Reg][Byte - YMMHiBytes];
      else if (Byte <= ZMMBytes)
        return zmm_hi256_0to15[Reg][Byte - ZMMHiBytes];
      else
        die("Byte out of bounds");
    } else if (Reg <= 31) {
      return zmm_full_16to31[Reg][Byte];
    } else {
      die("Register out of bounds");
    }
  }

public:
  const uint8_t &getZMM(uint32_t Reg, uint32_t Byte) const {
    return const_cast<XSave *>(this)->getZMMInternal(Reg, Byte);
  }
  void setZMM(uint32_t Reg, uint32_t Byte, uint8_t Val) {
    getZMMInternal(Reg, Byte) = Val;
  }
};

/// The vector register values in XSave are not contiguous in memory. This makes
/// it harder to inject a bit-flip at a random bit when dealing with vector
/// registers XMM to ZMM. We therefore use this wrapper class that serializes
/// the data into the `zmm` array. It provides import/export functions to
/// read/write data back to the raw xsave buffer.
struct VecRegsState {
  /// The raw data buffer handled by ptrace.
  XSave xsave;

  //    64 byte                0 byte
  //     |                       |
  //     v                       v
  //    <-----------ZMM----------->
  //    .            <-----YMM---->
  //    .            .      <-XMM->
  //Reg +------------+------+-----+
  //  0 |            |      |     | 0   bytes
  //  1 |            |      |     | 64
  //  ...                         ...
  // 31 |            |      |     |
  //    +------------+------+-----+ 2048
  /// XMM/YMM/ZMM registers.
  static constexpr const uint32_t NumZMMRegs = 32;
  uint8_t zmm[NumZMMRegs][ZMMBytes];    // =2048 bytes

  VecRegsState();
  /// Copy the raw data from buf into xsave.
  void importData();
  /// Export data to xsave.
  void exportData();
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

  /// Vector Registers.
  VecRegsState vecregs;

  /// Read all registers into \p GpRegs and \p VecRegs.
  void importRegistersTo(user_regs_struct &GpRegs, VecRegsState &VecRegs);

  /// Write all the register values of gpregs and vecregs into the machine.
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
  /// Note: this only updates the internal gpregs and vecregs. You need to
  /// exportRegisters() to update the processor's state.
  void *getRegisterPtr(const std::string &Reg, int Bit = 0);

  /// \Returns the contents of \p Reg and true on success.
  template <typename T>
  std::pair<T, bool> getRegisterContents(const std::string &Reg, int Bit = 0);

  /// Set \p Reg to \p Val around \p Bit. This sets a single byte. \Returns
  /// false if the register is not accessible.
  /// Note: this only updates the internal gpregs and vecregs. You need to
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

  /// Dump the contents of this class (gpregs and vecregs).
  void dump();
};
#endif //__REGMANIP_H__
