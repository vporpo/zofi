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

#include "options.h"
#include "debugstream.h"
#include "threads.h"
#include "utils.h"
#include <config.h>
#include <cstring>
#include <iomanip>
#include <sstream>

// Warning: Thish should be declared before the individual options.
OptionsParser Options;

// List of command line arguments.
Option<std::string> InjectTo("-inject-to", "rwe",
                             "Inject errors to: registers read(r), written(w), "
                             "explicitly(e) accessed or implicitly(i) accessed "
                             "registers, control-flow(c) or other(o) "
                             "instructions");
Option<const char *> Binary("-bin", 0, "The binary to inject faults into.");
Option<const char **> Args("-args", 0,
                           "The command line arguments for the binary. "
                           "Note: This should be placed last. Every "
                           "argument that follows is considered to be an "
                           "argument for the binary, not for " __BIN_NAME__
                           ".");
Option<unsigned>
    InjectionsPerRun("-injections-per-run", 1,
                     "The number of fault injections per program run.");
Option<double> UserInjectionTime("-injection-time", 0.0,
                                 "Fault injection should occur after this "
                                 "many seconds");
Option<int>
    MaxInjectionAttempts("-max-injection-attempts", 100,
                         "Stop trying to inject after this many attempts.");
Option<unsigned> TestRuns("-test-runs", 1, "The number of test runs.");
Option<unsigned> Jobs("-j", 1, "The number of parallel jobs. Use -j 0 for "
                               "automatically detecting the number of HW "
                               "threads supported by the CPU.");
Option<double>
    BinExecTime("-bin-exec-time", 0.0,
                "The execution time of the binary in microseconds. If not "
                "set, it will be measured with a test run.");
Option<double> BinExecTimeOvershoot("-bin-exec-time-overshoot", 1.1,
                                    "The original run may be slightly faster "
                                    "than the test runs. To accommodate for "
                                    "this, we multiply the original execution "
                                    "by this much.");
Option<unsigned long> InfExecTimeoutMul("-inf-exec-timeout-mul", 4,
                                        "The timeout for infinte execution is "
                                        "calculated as Base + Time * Mul. This "
                                        "is the 'Mul' part.");
Option<double> InfExecTimeoutBase("-inf-exec-timeout-base", 0.3,
                                  "The timeout for infinte execution is "
                                  "calculated as Base + Time * Mul. "
                                  "This is the 'Base' part (in seconds).");

Option<std::string> DiffCmd(
    "-diff-cmd", "",
    "Provide a custom diff command for comparing the outputs. The "
    "macros " STRFY(ORIG_STDOUT) ", " STRFY(ORIG_STDERR) ", " STRFY(
        TEST_STDOUT) ", " STRFY(TEST_STDERR) " will be replaced by the "
                                             "corresponding files. Note: you "
                                             "can specify a different shell "
                                             "with -diff-shell.");
Option<const char *>
    DiffShell("-diff-shell", "/bin/bash",
              "Specify a shell for -diff-cmd. The default is /bin/bash.");

Option<bool> DiffDisableRedirect("-diff-disable-redirect", false,
                                 "Don't redirect the stdout and stderr of the "
                                 "custom -diff-cmd. This is for debugging.");

Option<bool> HelpOption("-help", false, "Print the help message and exit.");
Option<bool> Version("-version", false, "Print the version and exit.");
Option<bool> NoRedirect("-no-redirect", false,
                        "Don't redirect the stdout and stderr");
Option<std::string> Stdout("-stdout-path", "/tmp/zofi.Stdout",
                           "The path prefix where the stdout will get dumped.");
Option<std::string> Stderr("-stderr-path", "/tmp/zofi.Stderr",
                           "The path prefix where the stderr will get dumped.");
Option<int> VerboseLevel("-v", 1, "The program's verbosity level.");
Option<bool> NoProgressBar("-no-progress-bar", false,
                           "Disable the progress bar.");
Option<bool>
    DontInjectToLibs("-no-inject-to-libs", false,
                     "Inject faults to external dynamically linked code.");
Option<bool> NoCleanup("-no-cleanup", false, "Do not remove temparary files.");
Option<int> DetectionExitCode("-detection-exit-code", 0,
                              "If the binary is protected by an error "
                              "detection technique, then this is the exit code "
                              "returned upon detection of an error. Please use "
                              "a positive value <= 125 to enable this "
                              "feature.");
Option<const char *> OutCsvFile("-out-csv", nullptr,
                                "Output statistics to this CSV file.");
Option<const char *>
    OutMoufoplotDir("-out-moufoplot", nullptr,
                    "Output statistics in Moufoplot format using this dir.");
Option<const char *> SetOrigExitState("-set-orig-exit-state", nullptr,
                                      "Set the exit state of the original run, "
                                      "without running the workload. This "
                                      "should be in the form "
                                      "/path/to/stdout,/path/to/"
                                      "stderr,{exited:<EXIT_CODE>,signaled:<"
                                      "SIGNAL>}");
Option<bool> DisableTimingRun("-disable-timing-run", false,
                              "Skip the original timing run.");

template <> void Option<const char **>::dump(std::ostream &OS) const {
  if (IsSet) {
    OS << std::left << " " << std::setw(22) << Flag << " = ";
    for (const char **Ptr = Val; *Ptr; ++Ptr)
      OS << *Ptr << " ";
    OS << "\n";
  }
}

OptionBase::OptionBase(const char *Flag, const char *Description)
    : Flag(Flag), Description(Description) {
  Options.addOption(this);
}

void OptionsParser::usage() {
  std::cerr << "Usage:\n";
  std::cerr << __BIN_NAME__
            << " -binary <BIN> [-args <ARGS>] [Other Options] [...]\n";
  std::cerr << "\n";
  std::cerr << "Available Arguments:\n";
  Options.printAllOptions();
  std::cerr << "\n";
  std::cerr << "Example: \n"
            << "$ " << __BIN_NAME__
            << " -bin /usr/bin/seq -test-runs 50 -args -f '%g' 1 500000\n";
}

// Bootstrapping the command line options.
void OptionsParser::init(int argc, char **argv) {
  // Get command line options
  if (VerboseLevel.getValue() > 3)
    Options.dump();
  Options.parse(argc, argv);

  // Print version and exit.
  if (Version.isSet()) {
    std::cout << __BIN_NAME__ << " " << __PROJECT_VERSION__ << "\n";
    std::cout
        << "Copyright (C) 2019 Vasileios Porpodas <v.porpodas at gmail.com>\n";
    std::cout
        << "Licence GNU GPL Version 2: <http://gnu.org/licenses/gpl.html>\n";
    std::cout << "This is free software; see the source for copying "
                 "conditions.  There is NO\n";
    std::cout << "warranty; not even for MERCHANTABILITY or FITNESS FOR A "
                 "PARTICULAR PURPOSE.\n";
    exit(0);
  }

  // Print options and exit.
  if (HelpOption) {
    OptionsParser::usage();
    exit(0);
  }

  // Check if any of the required arguments are missing.
  if (!Binary) {
    OptionsParser::usage();
    userDie("Error: Missing " + std::string(Binary.getFlag()) +
            ". It is a required argument!");
  }

  // Sanity checks.
  if (DetectionExitCode.getValue() > 0 && DetectionExitCode.getValue() > 125)
    userDie("Please use a value in the range 0-125 for the exit code.");

  auto HWThreads = std::thread::hardware_concurrency();
  if (Jobs.getValue() == 0)
    Jobs.setValue(HWThreads);

  // Limit the number of jobs to the number of hardware threads supported by the
  // target.
  if (Jobs.getValue() > HWThreads) {
    warning("WARNING: The target supports ", HWThreads,
            ". We are limitng the number of jobs to ", HWThreads, ".");
    Jobs.setValue(HWThreads);
  }

  // Check if out file exists
  if (OutMoufoplotDir.isSet() && !fileExists(OutMoufoplotDir.getValue()))
    userDie("Directory ", OutMoufoplotDir.getValue(), " does not exist.");

  if (InjectionsPerRun.getValue() > 1)
    userDie("We only support 0 or 1 injections per run");

  if (!fileExists(Binary.getValue()))
    userDie("Cannot find ", Binary.getValue(), " file");
}

OptionsParser::OptionsParser() {}

void OptionsParser::sanityChecks() {
  // Check -inject-to
  for (char C : InjectTo.getValue()) {
    switch (C) {
    case 'r':
    case 'w':
    case 'e':
    case 'i':
    case 'c':
    case 'o':
      break;
    default:
      userDie("Illegal -inject-to character: ", C);
      break;
    }
  }
  if (!isIn(InjectTo, "r") && !isIn(InjectTo, "w"))
    userDie("No read (r) or write (w) registers selected for injection: ",
            InjectTo.getValue(), ".");
  if (!isIn(InjectTo, "e") && !isIn(InjectTo, "i"))
    userDie("No explicit (e) / implicit(i) registers selectied: ",
            InjectTo.getValue(), ".");
}

void OptionsParser::parse(int argc, char **argv) {
  for (int i = 1; i < argc; ++i) {
    char *arg = argv[i];
    dbg(3) << "parse() : " << arg << "\n";
    auto it = OptionsMap.find(arg);
    // Argument found in map.
    if (it != OptionsMap.end()) {
      OptionBase *Opt = it->second;
      // Option '-args' is the last arg, everything that follows is part of it.
      if (Opt == &Args) {
        assert(Opt->needsValue && "Expected");
        static_cast<Option<char **> *>(Opt)->setValue(&argv[i + 1]);
        break;
      }
      // All other options.
      else {
        if (Opt->needsValue) {
          if (i + 1 >= argc)
            userDie("Error: " + std::string(Opt->getFlag()) +
                    " requires a value argument.");
          Opt->setValueFromStr(argv[i + 1]);
          i++;
        } else {
          Opt->setValueFromStr("1");
        }
        dbg(3) << "set val : " << Opt->getFlag() << " : " << argv[i + 1]
               << "\n";
      }
    }
    // Argument not found.
    else {
      usage();
      userDie("\nError: Argument " + std::string(arg) + " not supported.");
    }
  }

  sanityChecks();
}

void OptionsParser::addOption(OptionBase *Opt) {
  OptionsMap[Opt->getFlag()] = Opt;
  dbg(3) << "addOption() " << Opt->getFlag() << "\n";
}

/// \Returns true if \p C is a good canditate for line splitting.
static bool isLineSplitCandidate(char C) {
  switch (C) {
  case ' ':
  case '\0':
  case ',':
    return true;
  default:
    return false;
  }
}

void OptionsParser::printAllOptions() const {
  size_t maxFlagW = 0;
  for (auto &it : OptionsMap)
    maxFlagW = std::max(maxFlagW, it.first.size());

  char DescrSubstr[80];
  for (auto &it : OptionsMap) {
    const std::string &flag = it.first;
    OptionBase *Opt = it.second;
    // Pretty print the options: the description does not wrap around.
    unsigned MaxSubstrLen = 80 - (maxFlagW + strlen(" : ") + 1);
    assert(MaxSubstrLen > 40 && "Some flag is too long! We need at least 40 "
                                "columns for the description.");
    unsigned DescrLen = strlen(Opt->getDescription());
    unsigned PrintedSubstrLen = 0;
    while (PrintedSubstrLen < DescrLen) {
      std::cerr << " " << std::left << std::setw(maxFlagW);
      // Only the first line prints the flag, the rest print spaces.
      std::cerr << (PrintedSubstrLen == 0 ? flag.c_str() : " ");
      std::cerr << (PrintedSubstrLen == 0 ? " : " : "   ");

      // Now print part of the description.
      // For pretty-printing we split the description at the last space.
      int LastSpaceIdx = (DescrLen - PrintedSubstrLen >= MaxSubstrLen - 1)
                             ? MaxSubstrLen - 1
                             : DescrLen - PrintedSubstrLen;
      assert(LastSpaceIdx >= 0 && "Please fix the option description. It "
                                  "cannot be split to fit 80-columns.");
      for (; LastSpaceIdx >= 0; LastSpaceIdx--)
        if (isLineSplitCandidate(
                Opt->getDescription()[PrintedSubstrLen + LastSpaceIdx]))
          break;

      unsigned SubstrLen = LastSpaceIdx + 1;
      memcpy((void *)DescrSubstr,
             (void *)&Opt->getDescription()[PrintedSubstrLen], SubstrLen);
      DescrSubstr[SubstrLen] = '\0';
      std::cerr << DescrSubstr << "\n";
      PrintedSubstrLen += SubstrLen;
    }
  }
  std::cerr << "\n";
}

void OptionsParser::dump(std::ostream &OS) const {
  for (auto &it : OptionsMap) {
    // const std::string &flag = it.first;
    OptionBase *Opt = it.second;
    Opt->dump(OS);
  }
}

void OptionsParser::dump() const { dump(std::cout); }

std::string OptionsParser::getValuesStr() const {
  std::stringstream SS;
  dump(SS);
  return SS.str();
}
