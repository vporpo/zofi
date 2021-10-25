//-*- C++ -*-
// List of command line options.
//
// Copyright (C) 2021 Vasileios Porpodas <v.porpodas at gmail.com>
//
// This file is part of ZOFI.

#ifndef __OPTIONSLIST_H__
#define __OPTIONSLIST_H__

#include "options.h"

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

#endif // __OPTIONSLIST_H__
