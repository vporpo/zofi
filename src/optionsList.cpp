//-*- C++ -*-
// List of command line options.
//
// Copyright (C) 2021 Vasileios Porpodas <v.porpodas at gmail.com>
//
// This file is part of ZOFI.

#include "optionsList.h"
#include <config.h>

// List of command line arguments.
Option<std::string> InjectTo(
    "-inject-to", "rwe",
    "Inject errors to: registers read(r) or written(w). "
    "You can select between explicitly(e) and/or implicitly(i) accessed "
    "registers and/or the instruction pointer (c and/or o). Using `c` accepts "
    "only control-flow instructions, while `o` accepts all others.");
Option<std::string>
    ForceInjectToReg("-force-inject-to-reg", "",
                  "Force fault injection to a specific register, regardless of "
                  "whether the current instruction accesses it or not. Pass "
                  "'help' for a list of available registers.");
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
