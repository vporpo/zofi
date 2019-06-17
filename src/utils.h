//-*- C++ -*-
// Utility functions.
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

#ifndef __UTILS_H__
#define __UTILS_H__

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <pty.h>
#include <signal.h>
#include <string>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

extern bool debugFlag;

#ifdef _DEBUG
#define DEBUG(...)                                                             \
  if (debugFlag) {                                                             \
    __VA_ARGS__                                                                \
  }
#else
#define DEBUG(...)
#endif

#ifdef _DEBUG
#define MSG(...) fprintf(stderr, __VA_ARGS__);
#else
#define MSG(...)
#endif

#define DUMP_METHOD __attribute__((noinline)) __attribute__((__used__))

template <typename T> static inline void dieBase(const T &Val) {
  std::cerr << Val << "\n";
  abort();
}

/// Print arguments and exit with exit code 1.
template <typename T, typename... Ts>
static inline void dieBase(const T &Val1, const Ts... Vals) {
  std::cerr << Val1;
  dieBase(Vals...);
}

/// Exit the program, reporting a bug.
#define die(...)                                                               \
  dieBase("Error in ", __FILE__, ":", __LINE__, " ", __FUNCTION__,             \
          "(): ", __VA_ARGS__, " Please submit a bug report.")

/// Program exit due to user error.
#define userDie(...) dieBase(__VA_ARGS__)

template <typename T> static inline void warningBase(const T &Val) {
  std::cerr << Val << "\n";
}

/// Print arguments and exit with exit code 1.
template <typename T, typename... Ts>
static inline void warningBase(const T &Val1, const Ts... Vals) {
  std::cerr << Val1;
  warningBase(Vals...);
}

/// Print a warning message.
#define warning(...) warningBase(__VA_ARGS__)

/// Replaces all instances of \p From in \p Str with \p To.
static inline void strReplace(std::string &Str, const std::string &From,
                              const std::string &To) {
  size_t Pos;
  while ((Pos = Str.find(From)) != std::string::npos) {
    Str.replace(Pos, From.length(), To);
  }
}

/// Safe mkstemp().
static inline int mkstempSafe(char *File) {
  int Fd = mkstemp(File);
  if (Fd == -1) {
    perror("mkstemp()");
    die("failed to open ", File);
  }
  return Fd;
}

/// A safe alarm() that can handle arbitrarily large inputs.
static inline void alarmSafe(double Secs) {
  // usecs() can only handle [0, 1000000]
  if (Secs <= 1) {
    unsigned long Usecs = (unsigned long)(Secs * 1000000);
    ualarm(Usecs, 0);
  }
  // It is safe for alarm() to go off after Secs, so using ceil() is fine here.
  else
    alarm(std::ceil(Secs));
}

/// Safe ptrace() wrapper that will die() if ptrace() fails.
static inline long ptraceSafe(enum __ptrace_request request, pid_t pid,
                              const void *addr = nullptr,
                              const void *data = nullptr) {
  long Ret = ptrace(request, pid, addr, data);
  if (Ret == -1) {
    perror("ptrace");
    die("ptrace() failed.");
  }
  return Ret;
}

/// Safe kill() wrapper that will die() on error.
static inline void killSafe(pid_t Pid, int Sig) {
  if (kill(Pid, Sig) != 0) {
    perror("killSafe");
    die("killSafe() failed to kill ", Pid, " with signal ", Sig);
  }
}

/// Safe fork() wrapper that will die() on error.
static inline pid_t forkSafe() {
  pid_t PID = fork();
  if (PID == -1) {
    perror("fork");
    die("Error: fork() failed.");
  }
  return PID;
}

/// Safe forkpty() wrapper that will die() on error.
static inline std::pair<pid_t, int> forkptySafe() {
  int amaster;
  pid_t PID = forkpty(&amaster, NULL, NULL, NULL);
  if (PID == -1) {
    perror("forkpty");
    die("Error: forkpty() failed.");
  }
  return std::make_pair(PID, amaster);
}

/// Safe waitpid() wrapper that will die() on error.
static inline int waitpidSafe(pid_t PID) {
  int Status;
  if (waitpid(PID, &Status, 0) == -1) {
    perror("waitpid()");
    die("waitpid() failed");
  }
  return Status;
}

/// Wrapper for open(). It will die() on failure.
static inline int openSafe(const char *Path, int Flags) {
  int Fd = open(Path, Flags);
  if (Fd == -1)
    die("Failed to open ", Path);
  return Fd;
}

/// Safe close() wrapper that will die() on error.
static inline void closeSafe(int Fd) {
  if (close(Fd) == -1) {
    perror("close");
    die("close() error");
  }
}

/// Safe remove() wrapper that will die() on error.
static inline void removeSafe(const char *File) {
  if (remove(File) != 0) {
    perror("remove()");
    die("Failed to remove ", File);
  }
}

/// Compares the contents of the file descriptors \p Fd1 and \p Fd2. \Returns
/// true if equal.
static inline bool defaultDiff(int Fd1, int Fd2) {
  bool Diff = false;
  FILE *Fp1 = fdopen(Fd1, "r");
  if (!Fp1) {
    perror("fdopen");
    die("Failed to open Fd1 ", Fd1);
  }
  rewind(Fp1);
  FILE *Fp2 = fdopen(Fd2, "r");
  if (!Fp2) {
    perror("fdopen");
    die("Failed to open Fd2 ", Fd2);
  }
  rewind(Fp2);
  int N = 256;
  char Buf1[N], Buf2[N];
  do {
    size_t Sz1 = fread((void *)Buf1, 1, N, Fp1);
    size_t Sz2 = fread((void *)Buf2, 1, N, Fp2);
    // If either the size differs, or the contents differ we found a difference.
    if (Sz1 != Sz2 || memcmp(Buf1, Buf2, Sz1)) {
      Diff = true;
      break;
    }
  } while (!feof(Fp1) || !feof(Fp2));
  // Returns true if no difference found.
  return !Diff;
}

/// \Returns true if \p File exists.
static inline bool fileExists(const char *File) {
  struct stat StatData;
  return stat(File, &StatData) == 0;
}

// Macros for stringification.
#define STRFY(s) XSTRFY(s)
#define XSTRFY(s) #s

/// Save strtoul() with error reporting.
static inline unsigned long strtoulSafe(const char *Str) {
  char *EndPtr = nullptr;
  unsigned long Val = strtoul(Str, &EndPtr, 10);
  if (*Str == '\0' || *EndPtr != '\0')
    userDie("Bad number: ", Str);
  return Val;
}

/// Save strtol() with error reporting.
static inline unsigned long strtolSafe(const char *Str) {
  char *EndPtr = nullptr;
  long Val = strtol(Str, &EndPtr, 10);
  if (*Str == '\0' || *EndPtr != '\0')
    userDie("Bad number: ", Str);
  return Val;
}

/// Save strtod() with error reporting.
static inline double strtodSafe(const char *Str) {
  char *EndPtr = nullptr;
  double Val = strtod(Str, &EndPtr);
  if (*Str == '\0' || *EndPtr != '\0')
    userDie("Bad number: ", Str);
  return Val;
}

using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

/// \Returns the current time.
static inline TimePoint getTime() {
  return std::chrono::high_resolution_clock::now();
}

/// \Returns the time difference \p T2 - \p T1 in secs.
static inline double getTimeDiff(TimePoint T1, TimePoint T2) {
  return (double)std::chrono::duration<double>(T2 - T1).count();
}

/// \Returns true if \p Needle is found in \p Haystack.
static inline bool isIn(const std::string &Haystack,
                        const std::string &Needle) {
  return Haystack.find(Needle) != std::string::npos;
}

/// Return a random number in [ 0, \p Max ).
extern unsigned randSafe(unsigned Max = 0);

/// Sets the seed for the random function.
extern void setRandSeed(int Seed);

/// Initialize seed for rand().
extern void randInit();

/// Print \p Seconds in a human readable form (days, ours, minutes, seconds).
static inline void prettyPrintTime(unsigned long Seconds, std::ostream &OS) {
  const unsigned SecsInDay = 3600 * 24;
  const unsigned SecsInHour = 3600;
  const unsigned SecsInMin = 60;
  const unsigned SecsInSec = 1;
  bool PrintNext = false;
  for (auto Pair :
       {std::make_pair(SecsInDay, "d"), std::make_pair(SecsInHour, "h"),
        std::make_pair(SecsInMin, "m"), std::make_pair(SecsInSec, "s")}) {
    unsigned long SecondsInUnit = Pair.first;
    const char *UnitStr = Pair.second;
    if (Seconds > SecondsInUnit || PrintNext) {
      unsigned long Unit = Seconds / SecondsInUnit;
      OS << std::setw(2) << Unit << UnitStr;
      Seconds -= Unit * SecondsInUnit;
      PrintNext = true;
    }
  }
}

/// Safe wrapper for pipe.
static inline void pipeSafe(int PipeFd[2]) {
  if (pipe(PipeFd) != 0)
    die("Pipe failed");
}

/// \Returns a readable timestamp (in ms).
static inline unsigned long getTimestamp() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count() % 65536;
}
#endif // __UTILS_H__
