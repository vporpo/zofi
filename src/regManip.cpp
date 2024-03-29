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

#include "regManip.h"
#include "debugstream.h"
#include "optionsList.h"
#include "utils.h"
#include <capstone/capstone.h>
#include <cstdio>
#include <cstring>
#include <sstream>
#if ! defined (__x86_64__)
#error Unsupported target. ZOFI currently supports only x86_64.
#endif

void RegDescr::dump(std::ostream &OS) const {
  OS << "<Reg: " << Name << " StartBit:" << StartBit << " Bits:" << Bits << " "
     << (Written ? "W" : "R") << ">";
}

std::string RegDescr::dumpStr() const {
  std::stringstream SS;
  dump(SS);
  return SS.str();
}

void RegDescr::dump() const {
  dump(std::cerr);
  std::cerr << "\n";
}

VecRegsState::VecRegsState() {
  memset(&xsave, 0, sizeof(xsave));
  memset(zmm, 0, sizeof(zmm));
}

void VecRegsState::importData() {
  for (uint32_t Reg = 0; Reg != NumZMMRegs; ++Reg)
    for (uint32_t Byte = 0; Byte != ZMMBytes; ++Byte)
      zmm[Reg][Byte] = xsave.getZMM(Reg, Byte);
}

void VecRegsState::exportData() {
  for (uint32_t Reg = 0; Reg != NumZMMRegs; ++Reg)
    for (uint32_t Byte = 0; Byte != ZMMBytes; ++Byte)
      xsave.setZMM(Reg, Byte, zmm[Reg][Byte]);
}

void RegisterManipulator::importRegistersTo(user_regs_struct &GpRegs,
                                            VecRegsState &VecRegs) {
  ptraceSafe(PTRACE_GETREGS, ChildPid, nullptr, &GpRegs);

  // PTRACE_GETFPREGS is limited to XMM vector regs.
  // For full access we need to use PTRACE_GETREGSET.
  struct iovec iov;
  iov.iov_base = (uint8_t *)&VecRegs.xsave;
  iov.iov_len = sizeof(VecRegs.xsave);
  ptrace(PTRACE_GETREGSET, ChildPid, NT_X86_XSTATE, &iov);
  VecRegs.importData();
}

bool RegisterManipulator::exportRegisters() const {
  if (ptrace(PTRACE_SETREGS, ChildPid, nullptr, &gpregs) == -1) {
    dbg(3) << "ptrace(PTRACE_SETREGS) failed\n";
    return false;
  }

  struct iovec iov;
  iov.iov_base = (uint8_t *)&vecregs.xsave;
  iov.iov_len = sizeof(vecregs.xsave);
  if (ptrace(PTRACE_SETREGSET, ChildPid, NT_X86_XSTATE, &iov) == -1) {
    dbg(3) << "ptrace(PTRACE_SETREGSET) failed\n";
    return false;
  }
  return true;
}

int RegisterManipulator::getStartBit(const std::string &R) const {
  const RegData &Data = getRegDataForStrSafe(R);
  return Data.getStartBit();
}

int RegisterManipulator::getBits(const std::string &R) const {
  const RegData &Data = getRegDataForStrSafe(R);
  return Data.getBits();
}

void *RegisterManipulator::getRegisterPtr(const std::string &R, int Bit) {
  const RegData &Data = getRegDataForStrSafe(R);
  auto *RegField = Data.getRegPtr();
  return (void *)((uint8_t *)RegField + Bit / 8);
}

template <typename T>
std::pair<T, bool>
RegisterManipulator::getRegisterContents(const std::string &Reg, int Bit) {
  T *ValPtr = (T *)getRegisterPtr(Reg, Bit);
  if (!ValPtr)
    return {0, false /*Failed*/};
  return {*ValPtr, true /*Success*/};
}

bool RegisterManipulator::setRegisterContents(const std::string &R, uint8_t Val,
                                              int Bit) {
  const RegData &Data = getRegDataForStrSafe(R);
  auto *RegField = Data.getRegPtr();
  if (!RegField)
    return false;
  *((uint8_t *)RegField + Bit / 8) = Val;
  return true;
}

const RegData &
RegisterManipulator::getRegDataForStrSafe(const std::string &RegStr,
                                          bool UserError) const {
  auto it = StrToRegMap.find(RegStr);
  if (it == StrToRegMap.end()) {
    if (UserError)
      userDie("Unknown register: ", RegStr, ".");
    else
      die("Unknown register: ", RegStr, ".");
  }
  return it->second;
}

RegisterManipulator::RegisterManipulator(int ChildPid) : ChildPid(ChildPid) {
// 1. Populate StrToRegMap
#undef DEF_REG
#define DEF_REG(REG, REG_FIELD, START_BIT, BITS)                               \
  StrToRegMap[#REG] = RegData((uint8_t *)REG_FIELD, START_BIT, BITS);
#include "regs.def"
  // 2. Initialize data
  memset(&gpregs, 0, sizeof(gpregs));
  // vecregs contents get initialized in its constructor.
}

// Return true if Instr
static bool isControlInstr(cs_insn &Instr) {
  cs_detail *InstrDetail = Instr.detail;
  assert(InstrDetail && "Disasm missing CS_OPT_ON=ON or CS_OP_SKIPDATA=OFF.");
  bool IsControl = false;
  for (int Idx = 0, E = InstrDetail->groups_count; Idx != E; ++Idx) {
    auto Group = InstrDetail->groups[Idx];
    switch (Group) {
    case CS_GRP_JUMP:
    case CS_GRP_CALL:
    case CS_GRP_RET:
    case CS_GRP_INT:
    case CS_GRP_IRET:
    case CS_GRP_PRIVILEGE:
    case CS_GRP_BRANCH_RELATIVE:
      IsControl = true;
      break;
    default:
      break;
    }
  }
  return IsControl;
}

using RegsVec = RegisterManipulator::RegsVec;

/// Debug function for
static void DUMP_METHOD dumpRegsVec(const RegsVec &RVec,
                                    const std::string &Name) {
  dbg(2) << Name << ": ";
  for (const auto &Reg : RVec)
    Dbg(2) << Reg.dumpStr() << ", ";
  Dbg(2) << "\n";
}

std::tuple<RegsVec, RegsVec, RegsVec>
RegisterManipulator::getInstrRegisters(uint8_t *ChildIP) {
  RegsVec WRegs, RRegs, AllRegs;

// Now we need to access the child's memory to get the current instruction.
// We use PEEKTEXT to copy data from the child to a temporary buffer.
#define MAX_INSTR_BYTES 16
  uint8_t ChildMem[MAX_INSTR_BYTES];
  for (int i = 0; i != MAX_INSTR_BYTES; ++i)
    ChildMem[i] = ptraceSafe(PTRACE_PEEKTEXT, ChildPid, ChildIP + i);

  // Now that we have copied the data to ChildMem, let's create a new IP.
  uint8_t *IP = ChildMem;

  // http://www.capstone-engine.org/lang_c.html
  csh handle;
  if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK)
    die("Error: capstone cs_open failed.");

  if (cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON) != CS_ERR_OK)
    die("Error: capstone cs_option CS_OPT_DETAIL failed.");

  if (cs_option(handle, CS_OPT_SKIPDATA, CS_OPT_OFF) != CS_ERR_OK)
    die("Error: capstone cs_option CS_OPT_SKIPDATA failed.");

  cs_insn *Instrs;
  size_t Cnt = cs_disasm(handle, IP, 8, 0x0, 1, &Instrs);
  if (Cnt == 0) {
    dbg(2) << "cs_disasm() return Cnt == 0\n";
    return std::make_tuple(WRegs, RRegs, AllRegs); // Empty
  }
  cs_insn &Instr = Instrs[0];

  assert(Cnt == 1 && "Disassembled too many instructions!");
  dbg(2) << "IP:" << (void *)ChildIP << ", Addr:" << (void *)Instr.address
         << ", Mnemonic:" << Instr.mnemonic << ", Operands:" << Instr.op_str
         << "\n";

  cs_detail *InstrDetail = Instr.detail;

  // We now have to collect the registers accessed by the instruction, based on
  // the -inject-to arguments. We collect them into vectors RRegs, WRegs,
  // AllRegs. Please note that a register could show up multiple times, as it
  // could be listed multiple times in the instruction. For example the
  // instruction R1 = R1 + 1 is accessing R1 twice, so R1 will show up twice in
  // the AllRegs vector, once as a 'R' and once as a 'W' register.

  // 1. Collect the instruction pointer.
  if ((isIn(InjectTo, "c") && isControlInstr(Instr)) ||
      (isIn(InjectTo, "o") && !isControlInstr(Instr))) {
    const char *IPStr = "rip";
    const RegData &RData = getRegDataForStrSafe(IPStr);
    RegDescr RDescr(IPStr, RData.getStartBit(), RData.getBits(),
                    false /*Written*/);
    WRegs.push_back(RDescr);
    RRegs.push_back(RDescr);
    AllRegs.push_back(RDescr);
  }

  // 2. Collect implicitly accessed registers, if enabled.
  if (isIn(InjectTo, "i")) {
    for (int Idx = 0, E = InstrDetail->regs_read_count; Idx != E; ++Idx) {
      auto RegId = InstrDetail->regs_read[Idx];
      std::string RegStr(cs_reg_name(handle, RegId));
      const RegData &RData = getRegDataForStrSafe(RegStr);
      RegDescr RDescr(RegStr, RData.getStartBit(), RData.getBits(),
                      false /*Read*/);
      RRegs.push_back(RDescr);
    }
    for (int Idx = 0, E = InstrDetail->regs_write_count; Idx != E; ++Idx) {
      auto RegId = InstrDetail->regs_write[Idx];
      std::string RegStr(cs_reg_name(handle, RegId));
      const RegData &RData = getRegDataForStrSafe(RegStr);
      RegDescr RDescr(RegStr, RData.getStartBit(), RData.getBits(),
                      true /*Written*/);
      WRegs.push_back(RDescr);
    }
  }

  // 3. Explicitly accessed registers.
  if (isIn(InjectTo, "e")) {
    const cs_x86 &X86Data = InstrDetail->x86;
    for (int Idx = 0, E = X86Data.op_count; Idx != E; ++Idx) {
      const cs_x86_op &Operand = X86Data.operands[Idx];
      switch (Operand.type) {
      case X86_OP_REG: {
        std::string RegStr(cs_reg_name(handle, Operand.reg));
        int StartBit = getStartBit(RegStr);
        RegDescr RDescr(RegStr, StartBit, getBits(RegStr));
        if (Operand.access & CS_AC_WRITE) {
          RDescr.Written = true;
          WRegs.push_back(RDescr);
        } else if (Operand.access & CS_AC_READ) {
          RDescr.Written = false;
          RRegs.push_back(RDescr);
        }
        AllRegs.push_back(RDescr);
        break;
      }
      case X86_OP_MEM: {
        for (const x86_reg &Reg :
             {Operand.mem.segment, Operand.mem.base, Operand.mem.index}) {
          if (Reg == X86_REG_INVALID)
            continue;
          std::string RegStr(cs_reg_name(handle, Reg));
          int StartBit = getStartBit(RegStr);
          RegDescr RDescr(RegStr, StartBit, getBits(RegStr),
                          false /* Written */);
          RRegs.push_back(RDescr);
          AllRegs.push_back(RDescr);
        }
        break;
      }
      default:
        break;
      }
    }
  }

  cs_free(Instrs, Cnt);
  cs_close(&handle);

  // Dump the registers we have collected.
  dumpRegsVec(WRegs, "WRegs");
  dumpRegsVec(RRegs, "RRegs");
  dumpRegsVec(AllRegs, "AllRegs");

  return std::make_tuple(WRegs, RRegs, AllRegs);
}

std::tuple<RegDescr, unsigned, bool>
RegisterManipulator::getSelectedRegAndBit(uint8_t *IP) {
  // 1. Get the registers accessed by the current instruction.
  RegsVec WRegs, RRegs, AllRegs;
  std::tie(WRegs, RRegs, AllRegs) = getInstrRegisters(IP);

  // 2. Pick a register. If forced, selecte the forced one, otherwise select a
  // random one written by the instruction.
  RegsVec RegsVec;
  if (ForceInjectToReg.isSet()) {
    const std::string &ForcedReg = ForceInjectToReg.getValue();
    const RegData &RData = getRegDataForStrSafe(ForcedReg, /*UserError=*/true);
    RegsVec = {RegDescr(ForcedReg, RData.getStartBit(), RData.getBits())};
  } else {
    assert((isIn(InjectTo, "r") || isIn(InjectTo, "w")) &&
           "At least one of 'r', 'w' should be enabled");
    RegsVec = (isIn(InjectTo, "r") && isIn(InjectTo, "w")) ? AllRegs
              : isIn(InjectTo, "r")                        ? RRegs
                                                           : WRegs;
  }

  if (RegsVec.empty())
    return std::make_tuple(RegDescr(), 0, false);

  unsigned SelectedReg = ForceInjectToReg.isSet() ? 0 : randSafe(RegsVec.size());
  const RegDescr &Reg = RegsVec[SelectedReg];

  // 3. Pick a random bit.
  unsigned StartBit = RegsVec[SelectedReg].StartBit;
  unsigned Bits = RegsVec[SelectedReg].Bits;
  unsigned SelectedBit;
  if (ForceInjectToBit.isSet()) {
    SelectedBit = strtolSafe(ForceInjectToBit.getValue().c_str());
    // If the user has not forced a register, then this may be out of bounds.
    // In this case we will have to retry.
    if (!(SelectedBit >= StartBit && SelectedBit < StartBit + Bits))
      return std::make_tuple(Reg, SelectedBit, false);
  } else {
    SelectedBit = StartBit + randSafe(Bits);
  }
  assert(SelectedBit >= StartBit && SelectedBit < StartBit + Bits &&
         "Bad RandBit");

  return std::make_tuple(Reg, SelectedBit, true);
}

uint8_t *RegisterManipulator::getProgramCounter() {
  // We get the Child's Instruction Pointer by looking at its rip register.
  importRegistersTo(gpregs, vecregs);
  uint8_t *ChildIP;
  bool Success;
  std::tie(ChildIP, Success) = getRegisterContents<uint8_t *>("rip");
  if (!Success)
    die("Could not read 'rip' register.");
  return ChildIP;
}

bool RegisterManipulator::tryBitFlip(const std::string &Reg, unsigned Bit) {
  // We need to convert something like xmm2, bit 46 to xmm_space[5] bit 14
  //                                or zmm2, bit 46 to xmm_space[17] bit 14
  assert(StrToRegMap.count(Reg) && "Missing register from map");

  importRegistersTo(gpregs, vecregs);

  if (VerboseLevel >= 10) {
    dbg(10) << "=== Before bitflip ===\n";
    dump();
  }

  uint8_t OldByte;
  bool Success;
  std::tie(OldByte, Success) = getRegisterContents<uint8_t>(Reg, Bit);
  if (!Success)
    die("getRegisterContents() should never fail");

  int BitInByte = Bit % 8;
  uint8_t NewByte = OldByte ^ (1 << BitInByte);
  bool Legal = setRegisterContents(Reg, NewByte, Bit);
  if (!Legal)
    return false;
  dbg(2) << "Flip reg: " << Reg << ", bit: " << Bit << ", (=" << BitInByte
         << " in Byte). Byte before: 0x" << std::setfill('0') << std::setw(2)
         << std::hex << (uint32_t)OldByte << ", after: 0x" << std::setfill('0')
         << std::setw(2) << std::hex << (uint32_t)NewByte << "\n";
  // Writing to illegal registers can fail.
  bool ExportSuccess = exportRegisters();
  if (VerboseLevel >= 10) {
    dbg(10) << "=== After bitflip ===\n";
    dump();
  }
  return ExportSuccess;
}


void RegisterManipulator::dumpMachine() {
  // Read registers into gpregs2 and vecregs2, so that we do not destroy the
  // state of the class.
  user_regs_struct gpregs2;
  VecRegsState vecregs2;
  memset(&gpregs2, 0, sizeof(gpregs2));
  importRegistersTo(gpregs2, vecregs2);

#undef DEF_REG
#define DEF_REG(REG, REG_FIELD, START_BIT, BITS)                               \
  if (REG_FIELD)                                                               \
    fprintf(stderr, "%12s: 0x%016lx\n", #REG, *(unsigned long *)REG_FIELD);    \
  else                                                                         \
    fprintf(stderr, "%12s: Unsupported\n", #REG);
#include "regs.def"
}

void RegisterManipulator::dump() {
#undef DEF_REG
#define DEF_REG(REG, REG_FIELD, START_BIT, BITS)                               \
  fprintf(stderr, "%12s: ", #REG);                                             \
  if (REG_FIELD) {                                                             \
    fprintf(stderr, "0x");                                                     \
    for (int Byte = BITS / 8 - 1; Byte >= START_BIT / 8; --Byte) {             \
      fprintf(stderr, "%02x", ((uint8_t *)REG_FIELD)[Byte]);                   \
      if (Byte % 8 == 0)                                                       \
        fprintf(stderr, " ");                                                  \
    }                                                                          \
  } else {                                                                     \
    fprintf(stderr, "Unsupported");                                            \
  }                                                                            \
  fprintf(stderr, "\n");
#include "regs.def"
}
