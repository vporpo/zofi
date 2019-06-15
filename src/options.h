//-*- C++ -*-
// Support classes for the command line operands.
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

#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include "utils.h"
#include <iomanip>
#include <iostream>
#include <map>
#include <ostream>
#include <string>

#define ORIG_STDOUT "%ORIG_STDOUT"
#define ORIG_STDERR "%ORIG_STDERR"
#define TEST_STDOUT "%TEST_STDOUT"
#define TEST_STDERR "%TEST_STDERR"

// Base class
class OptionBase {
protected:
  const char *Flag;
  const char *Description;

public:
  OptionBase(const char *Flag, const char *Description);
  // This is true if this options requires an argument.
  // For example -num 123.
  // If not, then it should accept "1" as a default argument.
  // For example -enable-something.
  bool needsValue = false;
  virtual void setValueFromStr(const char *strVal){};
  /// Print the flag and its description.
  virtual void dumpDescr(std::ostream &OS) const {
    OS << std::left << std::setw(23) << Flag << " : " << Description << "\n";
  }
  virtual void dumpDescr() const { dump(std::cerr); }
  /// Print the flag and its value
  virtual void dump(std::ostream &OS) const = 0;
  virtual void dump() const { dump(std::cerr); };
  /// \Returns the flag.
  const char *getFlag() const { return Flag; }
  /// \Returns the description.
  const char *getDescription() const { return Description; }
};

template <typename T> static T getValForStr(const char *&str) {}
// Template specialization for parsing 'str' and returning the right value.
// NOTE: Each new option type supported needs a specialization.
template <> inline int getValForStr<int>(const char *&str) {
  return strtolSafe(str);
}
template <> inline unsigned getValForStr<unsigned>(const char *&str) {
  return (unsigned)strtoulSafe(str);
}
template <> inline bool getValForStr<bool>(const char *&str) {
  return strtolSafe(str);
}
template <> inline long getValForStr<long>(const char *&str) {
  return strtolSafe(str);
}
template <> inline unsigned long getValForStr<unsigned long>(const char *&str) {
  return strtoulSafe(str);
}
template <> inline const char *getValForStr<const char *>(const char *&str) {
  return str;
}
template <> inline std::string getValForStr<std::string>(const char *&str) {
  return std::string(str);
}
template <> inline float getValForStr<float>(const char *&str) {
  return strtodSafe(str);
}
template <> inline double getValForStr<double>(const char *&str) {
  return strtodSafe(str);
}
template <> inline const char **getValForStr<const char **>(const char *&str) {
  return &str;
}

template <typename T> static bool getNeedsValue() { return true; }
// Template specialization for getNeedsValue().
// NOTE: I think only bool should not require a value argument.
template <> inline bool getNeedsValue<bool>() { return false; }

// The option class holding an actual option value.
template <typename T> class Option : public OptionBase {
  T Val;
  bool IsSet = false;

public:
  Option(const char *Flg, T InitVal, const char *Descr)
      : OptionBase(Flg, Descr) {
    Val = InitVal;
    needsValue = getNeedsValue<T>();
  }
  void setValueFromStr(const char *StrVal) {
    Val = getValForStr<T>(StrVal);
    IsSet = true;
  }
  void setValue(T V) {
    Val = V;
    IsSet = true;
  }
  T getValue() const { return Val; }
  bool isSet() const { return IsSet; }
  void dump(std::ostream &OS) const override {
    if (IsSet)
      OS << std::left << " " << std::setw(22) << Flag << " = " << Val << "\n";
  }
  // Converting operator to avoid using getValue() in most cases.
  operator T() const { return Val; }
};

class OptionsParser {
  /// Maps the option flag to the option object.
  std::map<std::string, OptionBase *> OptionsMap;

  /// Check if the arguments good values, or die.
  void sanityChecks();

  /// Parses \p argc and \p argv and initializes Option objects.
  void parse(int argc, char **argv);

public:
  OptionsParser();
  static void usage();
  void init(int argc, char **argv);
  void addOption(OptionBase *Opt);
  // Prints all flags and their descriptions.
  void printAllOptions() const;
  /// Dumps all flags and their value.
  void dump(std::ostream &OS) const;
  void dump() const;
  /// \Returns a string with all flags and their values.
  std::string getValuesStr() const;
};

extern OptionsParser Options;

extern Option<std::string> InjectTo;
extern Option<const char *> Binary;
extern Option<const char **> Args;
extern Option<unsigned> InjectionsPerRun;
extern Option<double> UserInjectionTime;
extern Option<int> MaxInjectionAttempts;
extern Option<unsigned> TestRuns;
extern Option<unsigned> Jobs;
extern Option<double> BinExecTime;
extern Option<double> BinExecTimeOvershoot;
extern Option<unsigned long> InfExecTimeoutMul;
extern Option<double> InfExecTimeoutBase;
extern Option<std::string> DiffCmd;
extern Option<const char *> DiffShell;
extern Option<bool> DiffDisableRedirect;
extern Option<bool> HelpOption;
extern Option<bool> Version;
extern Option<bool> NoRedirect;
extern Option<std::string> Stdout;
extern Option<std::string> Stderr;
extern Option<int> VerboseLevel;
extern Option<bool> NoProgressBar;
extern Option<bool> DontInjectToLibs;
extern Option<bool> NoCleanup;
extern Option<int> DetectionExitCode;
extern Option<const char *> OutCsvFile;
extern Option<const char *> OutMoufoplotDir;
extern Option<const char *> SetOrigExitState;
extern Option<bool> DisableTimingRun;
#endif //__OPTIONS_H__
