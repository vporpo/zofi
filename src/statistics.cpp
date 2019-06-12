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

#include "statistics.h"
#include "debugstream.h"
#include "options.h"
#include "utils.h"
#include <cassert>
#include <fstream>
#include <iomanip>
#include <vector>

Statistics::Statistics() {
  // Initialize with zero.
  zero();
  // Initialize values that come from options.
  StringMap[Type::Binary] = Binary.getValue();
  StringMap[Type::InjectTo] = InjectTo.getValue();
  ULongMap[Type::TestRuns] = TestRuns.getValue();
}

void Statistics::zero() {
  for (const auto &S :
       {Type::Masked, Type::Exception, Type::InfExec, Type::Corrupted,
        Type::Detected, Type::InjFailed, Type::Skipped})
    ULongMap[S] = 0;
}

void Statistics::incr(Type S) {
  std::lock_guard<std::mutex> Lock(Mtx);

  switch (S) {
  case Type::Masked:
  case Type::Exception:
  case Type::InfExec:
  case Type::Corrupted:
  case Type::Detected:
  case Type::InjFailed:
  case Type::Skipped:
    ULongMap[S]++;
    break;
  default:
    assert(0 && "Bad type");
  }

  // Increment the counter only for the non-failed runs.
  switch (S) {
  case Type::InjFailed:
  case Type::Skipped:
    break;
  default:
    TotalInjOK++;
  }

  GrandTotal++;
}

template <> void Statistics::set<unsigned long>(Type S, unsigned long Val) {
  std::lock_guard<std::mutex> Lock(Mtx);
  // Note: we don't implement set() for fault counters because incr() should be
  // used instead.
  switch (S) {
  case Type::NumTests:
  case Type::TestRuns:
    ULongMap[S] = Val;
    break;
  default:
    assert(0 && "Bad type");
  }
}

template <> void Statistics::set<double>(Type S, double Val) {
  std::lock_guard<std::mutex> Lock(Mtx);
  switch (S) {
  case Type::OrigExecTime:
  case Type::TestsExecTime:
    DoubleMap[S] = Val;
    break;
  default:
    assert(0 && "Bad type");
  }
}

std::string Statistics::getStr(Type S) {
  std::lock_guard<std::mutex> Lock(Mtx);

  switch (S) {
  case Type::Masked:
  case Type::Exception:
  case Type::InfExec:
  case Type::Corrupted:
  case Type::Detected:
  case Type::InjFailed:
  case Type::Skipped:
  case Type::NumTests:
  case Type::TestRuns:
    return std::to_string(ULongMap.at(S));
  case Type::OrigExecTime:
  case Type::TestsExecTime:
    return std::to_string(DoubleMap.at(S));
  case Type::Binary:
  case Type::InjectTo:
    return StringMap.at(S);
  default:
    assert(0 && "Bad type");
  }
}

void Statistics::dump() {
  if (TotalInjOK == 0) {
    std::cout << "No Results.\n";
    return;
  }
  int Col0 = 16, Col1 = 3, Col2 = 5, Col3 = 7;
  std::cout << "\n";
  std::cout << "-------------------------------\n";
  std::cout << std::setw(Col0) << std::left << "Outcome"
            << ": " << std::setw(Col1) << "Cnt"
            << ", " << std::right << std::setw(Col2) << "%"
            << "\n";
  std::cout << "-------------------------------\n";

  std::vector<Type> StatsToPrint = {Type::Masked, Type::Exception,
                                    Type::InfExec, Type::Corrupted};

  // Enable "Detected" only if the user has used the detection flag.
  if (DetectionExitCode.getValue())
    StatsToPrint.push_back(Type::Detected);

  // If we skipped, print the skipped ones.
  if (ULongMap.at(Type::Skipped))
    StatsToPrint.push_back(Type::Skipped);

  // Iterate and print.
  for (const auto &S : StatsToPrint) {
    std::cout.precision(3);
    std::cout << std::setw(Col0) << std::left << getTypeStr(S) << ": "
              << std::right << std::setw(Col1) << ULongMap.at(S) << ", "
              << std::right << std::setw(Col2) << std::right << std::setw(Col3)
              << (float)(ULongMap.at(S) * 100) / TotalInjOK << "%\n";
  }
}

void Statistics::dumpToCSV() {
  bool FileExists = fileExists(OutCsvFile.getValue());

  std::fstream File(OutCsvFile.getValue(), std::fstream::ios_base::app);
  if ((File.rdstate() & std::fstream::failbit) != 0)
    userDie("Error opening file ", OutCsvFile.getValue());

  std::vector<Type> StatsToPrint = {
      Type::Binary,        Type::InjectTo, Type::TestRuns,  Type::Masked,
      Type::Exception,     Type::InfExec,  Type::Corrupted, Type::OrigExecTime,
      Type::TestsExecTime, Type::NumTests};
  const unsigned FieldWidth = 10;
  const char *Delim = ", ";

  // Print header on new files.
  if (!FileExists) {
    for (const auto &S : StatsToPrint)
      File << std::setw(FieldWidth) << getTypeStr(S) << Delim;
    File << "\n";
  }

  // Print counters.
  for (const auto &Stat : StatsToPrint)
    File << std::setw(FieldWidth) << getStr(Stat) << Delim;
  File << "\n";

  File.close();
}

void Statistics::dumpToMoufoplot() {
  std::vector<Type> StatsToPrint = {
      Type::Masked,       Type::Exception,     Type::InfExec, Type::Corrupted,
      Type::OrigExecTime, Type::TestsExecTime, Type::NumTests};

  std::string FilePrefix = std::string(Binary.getValue()) + "_" + "InjectTo-" +
                           InjectTo.getValue() + "_" + "TestRuns-" +
                           std::to_string(TestRuns.getValue());

  for (const auto &Stat : StatsToPrint) {
    // File: Dir/Bin_Stat
    std::string FilePath = std::string(OutMoufoplotDir.getValue()) + "/" +
                           FilePrefix + "_" + getTypeStr(Stat);
    std::fstream File(FilePath, std::fstream::ios_base::out);
    if ((File.rdstate() & std::fstream::failbit) != 0)
      userDie("Error opening file ", FilePath);
    File << getStr(Stat);
    File.close();
  }
}
