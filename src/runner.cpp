// Classes for running the binaries.
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

#include "runner.h"
#include "debugstream.h"
#include "options.h"
#include "regManip.h"
#include "utils.h"
#include <algorithm>
#include <capstone/capstone.h>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>

extern char **Envp; // zofi.cpp

RunnerBase::RunnerBase(long Id) : Id(Id) {
  // argv[0]
  Argv.push_back(Binary.getValue());
  // argv[1...last]
  if (Args.isSet())
    for (const char **Arg = Args.getValue(); *Arg; ++Arg)
      Argv.push_back(*Arg);
  // argv[last+1] execve() needs a terminator NULL.
  Argv.push_back(NULL);

  // Push environmental variables.
  for (char **EnvVar = Envp; *EnvVar; ++EnvVar)
    Argp.push_back(*EnvVar);
  Argp.push_back(NULL);

  // Check if the arguments make sense, otherwise exit.
  sanityChecksOrExit();

  // Create the unique stdout/stderr files.
  ExState.initFiles(Id);
}

RunnerBase::~RunnerBase() {
  // Close the files.
  for (int Fd : {ExState.getStdoutFd(), ExState.getStderrFd()})
    if (Fd >= 0)
      closeSafe(Fd);

  closeOpenTerminal();

  // Forced to skip temporary file removal.
  if (NoCleanup)
    return;

  // Remove temporary files.
  for (const char *File : {ExState.getStdoutFile(), ExState.getStderrFile()}) {
    if (File[0] != '\0')
      removeSafe(File);
  }
}

void RunnerBase::sanityChecksOrExit() {
  // Check that the binary exists.
  struct stat StatData;
  if (!Binary || stat(Binary, &StatData) == -1)
    userDie("Error accessing file '", Binary, "'.");
}

ExitState RunnerBase::getWaitPidExitState(int Status) {
  // Exited normally by calling exit() or returning from main().
  if (WIFEXITED(Status)) {
    dbg(2) << "EXITED\n";
    return {ExitType::Exited, WEXITSTATUS(Status)};
  }
  // Terminated by a signal. Since the child is ptraced this can only happen if
  // we try to inject an error too early, before it has started.
  else if (WIFSIGNALED(Status)) {
    dbg(2) << "SIGNALED\n";
    return {ExitType::Signaled, WTERMSIG(Status)};
  }
  // Stopped by delivery of a signal.
  else if (WIFSTOPPED(Status)) {
    dbg(2) << "STOPPED\n";
    return {ExitType::Stopped, WSTOPSIG(Status)};
  }
  //
  else {
    die("getWaitPidExitState(): Unreachable");
    return {ExitType::Invalid, 666};
  }
}

bool RunnerBase::run(bool TimeoutAlarm) {
  // Cleanup from prior attempts to launch thread or inject error.
  closeOpenTerminal();

  // Get the parent's pid
  ParentPID = getpid();

  dbg(2) << "ParentPID " << ParentPID << "\n";

  if (!NoRedirect.getValue()) {
    // We are connecting the child child process to a new pty because some
    // faults from glibc are still printed on the parent's terminal even after
    // stdout and stderr redirection.
    std::tie(ChildPID, ChildTerminalFd) = forkptySafe();
  } else {
    // Don't create a new terminal if we are not redirecting the output.
    ChildPID = forkSafe();
  }

  if (ChildPID == 0) {
    // Duplicate File Descriptors
    int OldStdout = -1, OldStderr = -1;
    if (!NoRedirect.getValue()) {
      OldStdout = dup(1);
      OldStderr = dup(2);
    }

    // Infinite execution timeout.
    if (TimeoutAlarm) {
      assert(BinExecTime.isSet() && "Should have been set by now");
      // The timeout is "Base + BinTime * Mul".
      // The base is required for binaries that run very fast, about a few
      // milliseconds. Since the timing measurements in this timescale are
      // small,
      // we may easily go over the infinite loop limit and consider this an
      // infinite execution.
      double InfExecTimeout =
          InfExecTimeoutBase.getValue() +
          BinExecTime.getValue() * InfExecTimeoutMul.getValue();
      alarmSafe(InfExecTimeout);
    }

    // Redirect stdout and stderr to files
    if (!NoRedirect.getValue()) {
      ftruncate(ExState.getStdoutFd(), 0);
      ftruncate(ExState.getStderrFd(), 0);
      dup2(ExState.getStdoutFd(), 1);
      dup2(ExState.getStderrFd(), 2);
    }
    // Prepare self for being traced by parent.
    ptraceSafe(PTRACE_TRACEME, 0, 0, 0);

    // Run given command
    execve(Binary.getValue(), (char *const *)Argv.data(),
           (char *const *)Argp.data());
    perror("execve()");
    if (!NoRedirect.getValue())
      dprintf(OldStderr, "Error in execve(). Use %s to see the perror() output",
              NoRedirect.getFlag());
    die("execve error");

    exit(1);
  } else if (ChildPID > 0) {
    // Parent process
    dbg(2) << "ChildPID " << ChildPID << "\n";

    // Wait for the child to stop at execve, since it is being traced.
    waitpidSafe(ChildPID);
    // Let the child continue after execve.
    // This can fail if the child finishes immediately.
    if (ptrace(PTRACE_CONT, ChildPID, 0, 0) == -1)
      return false;
    return true;
  } else {
    // We should never reach here.
    die("fork() failed");
  }
  return true;
}

void RunnerBase::closeOpenTerminal() const {
  // Close the child's terminal.
  if (ChildTerminalFd != -1)
    closeSafe(ChildTerminalFd);
}

OrigRunner::OrigRunner(long Id, const ExecutionExitState *OrigExState)
    : RunnerBase(-1) {}

void OrigRunner::runAndWait(bool *ThreadFinished, Statistics *Stats) {
  // Start a timing run. Note: This is non-blocking.
  bool Success = run();
  if (!Success)
    userDie("Workload runs for a very short time. Cannot inject errors to it.");
  // This is a timing run, so block until the child has finished.
  dbg(2) << "Waiting for ChildPID " << ChildPID << " to finish...";
  int Status = waitpidSafe(ChildPID);
  Dbg(2) << " Done.\n";

  ExState.setExitState(getWaitPidExitState(Status));
  dbg(2) << " ExitState=" << ExState.getExitState().getDumpStr() << "\n";

  *ThreadFinished = true;
}

Status Runner::waitChildAndGetStatus() {
  // Wait until child process has finished (or early exited)
  dbg(2) << "Before waitpid()\n";
  int WaitStatus = waitpidSafe(ChildPID);
  dbg(2) << "After waitpid()\n";
  ExState.setExitState(getWaitPidExitState(WaitStatus));

  // If we disable redirection to a file, then we should disable output checks.
  SkipCheck = NoRedirect.getValue() ? true : SkipCheck;
  if (SkipCheck) {
    dbg(2) << "CHECK: Skipped.\n";
    // If it is not exited, kill it.
    if (ExState.getExitState().Type != ExitType::Exited)
      ptraceSafe(PTRACE_KILL, ChildPID, 0, 0);
    return Status::Skipped;
  }

  switch (ExState.getExitState().Type) {
  // The child may have exited.
  case ExitType::Exited:
    // If the binary is protected by error detection, check if we got the exit
    // code that corresponds to "detected error".
    if (DetectionExitCode.getValue() > 0 &&
        ExState.getExitState().Val == DetectionExitCode.getValue()) {
      dbg(2) << "CHECK: Detected.\n";
      return Status::Detected;
    }
    // Else, we need to check its state.
    if (checkOutput(ExState.getExitState())) {
      dbg(2) << "CHECK: Masked.\n";
      return Status::Masked;
    } else {
      dbg(2) << "CHECK: Corrupted.\n";
      return Status::Corrupted;
    }
    break;
  // The infinite loop signal may have stopped the child.
  case ExitType::Stopped:
    if (ExState.getExitState().Val == SIGALRM) {
      dbg(2) << "CHECK: InfExec.\n";
      // Kill infinitely-looping child.
      ptraceSafe(PTRACE_KILL, ChildPID, 0, 0);
      return Status::InfExec;
    }
    // The original run may have stopped with a signal if the original
    // application is faulty. We therefore check the output here too.
    if (checkOutput(ExState.getExitState())) {
      dbg(2) << "CHECK: Masked.\n";
      return Status::Masked;
    } else {
      dbg(2) << "CHECK: Exception.\n";
      return Status::Exception;
    }
    break;
  // Since we attached to the child, it can't have terminated.
  case ExitType::Signaled:
    assert(0 && "Why terminated? Haven't we attached to it?");
    break;
  default:
    assert(0 && "Unreachable: bad ExitState.Type");
    return Status::None;
  }
  die("waitChild(): UNREACHABLE");
  return Status::None;
}

Runner::Runner(long Id, const ExecutionExitState *OrigExState)
    : RunnerBase(Id), OrigExState(OrigExState) {}

double Runner::getRandomInjectionTime() {
  assert(BinExecTime.isSet() && "Execution time not set");
  // Get a random number in [0, 1].
  double Rand = (double)randSafe() / RAND_MAX;
  double BinExecTimeWithOvershoot =
      BinExecTime.getValue() * BinExecTimeOvershoot.getValue();
  return Rand * BinExecTimeWithOvershoot;
}

/// Helper function for incrementing statistics for \p S.
static void incrStatsCounter(Statistics *Stats, Status S) {
  switch (S) {
  case Status::None:
    assert(0 && "Unreachable");
    break;
  case Status::Masked:
    Stats->incr(Type::Masked);
    break;
  case Status::Exception:
    Stats->incr(Type::Exception);
    break;
  case Status::InfExec:
    Stats->incr(Type::InfExec);
    break;
  case Status::Corrupted:
    Stats->incr(Type::Corrupted);
    break;
  case Status::Detected:
    Stats->incr(Type::Detected);
    break;
  case Status::InjFailed:
    Stats->incr(Type::InjFailed);
    break;
  case Status::Skipped:
    Stats->incr(Type::Skipped);
    break;
  }
}

void Runner::runAndWait(bool *ThreadFinished, Statistics *Stats) {
  unsigned Attempts = MaxInjectionAttempts;
  bool InjectOK = false;
  do {
    if (--Attempts == 0) {
      Stats->dump();
      userDie("Error: Failed to inject after " +
              std::to_string(MaxInjectionAttempts) +
              " attempts. Is the execution time of the binary too short?");
    }
    // Start an injection run. Note: This is non-blocking.
    bool Success = run(true /* Timeout Alarm */);
    if (!Success)
      continue;

    if (InjectionsPerRun.getValue() > 0) {
      // If the user has not set the injection time, set it to a random value.
      double InjectionTime = UserInjectionTime.isSet()
                                 ? UserInjectionTime.getValue()
                                 : getRandomInjectionTime();
      InjectOK = tryInjectFaultAt(*Stats, InjectionTime);
    }
  } while (!InjectOK && InjectionsPerRun.getValue() > 0);

  // Wait until the child has finished.
  Status S = waitChildAndGetStatus();
  incrStatsCounter(Stats, S);

  // dbg(2) << getStatusStr(Status) << "\n";
  *ThreadFinished = true;
}

bool Runner::stopChildAfter(double SleepTime) {
  // Sleep until the time comes to inject the fault.
  dbg(2) << "Sleeping " << SleepTime << "secs...";
  std::this_thread::sleep_for(
      std::chrono::milliseconds((unsigned long)(SleepTime * 1000)));
  Dbg(2) << "Done\n";

  // Stop the child.
  killSafe(ChildPID, SIGTRAP);

  int Status = waitpidSafe(ChildPID);
  ExitState ChildState = getWaitPidExitState(Status);
  if (ChildState.Type == ExitType::Exited) {
    dbg(2) << "Waited too long? Child has already exited\n";
    return false;
  }
  if (ChildState.Type == ExitType::Signaled) {
    dbg(2) << "Injected error too early\n";
    return false;
  }

  // The child must have stopped either by us, or by alarm().
  if (ChildState.Type != ExitType::Stopped)
    die("Child Process should be in ptrace-stopped\n");

  dbg(2) << "We just interrupted " << ChildPID << "\n";
  return true;
}

bool Runner::doBitFlip() {
  // This is where the actual fault injection takes place.
  RegisterManipulator RM(ChildPID);

  uint8_t *IP = RM.getProgramCounter();
  dbg(2) << "IP: " << (void *)IP << "\n";

  // Parse the address space of the child.
  AddressSpace ChildAS(ChildPID);

  // Check if we are in library
  if (DontInjectToLibs && ChildAS.isInLibrary((uint64_t)IP)) {
    dbg(2) << "We are in a library, skipping\n";
    return false;
  }

  RegDescr Reg;
  unsigned Bit;
  bool Success;
  std::tie(Reg, Bit, Success) = RM.getRandomRegAndBit(IP);
  // This can fail for instructions accessing no registers, like jne.
  if (!Success) {
    dbg(2) << "failed to get random reg and bit\n";
    return false;
  }

  // If we are injecting the fault into a register that gets written, then step
  // to the next instruction before inject it, otherwise the bitflip will be
  // overwritten by the output of the current instruction.
  dbg(3) << "Reg to be modified: " << Reg.dumpStr() << "\n";
  if (Reg.Written) {
    // Step to next instruction.
    dbg(2) << "about to SINGLESTEP\n";
    ptraceSafe(PTRACE_SINGLESTEP, ChildPID, 0, 0);
    int Status = waitpidSafe(ChildPID);
    ExitState State = getWaitPidExitState(Status);
    // If we are *exteremely* unlucky, the alarm might have gone off.
    // We will just restart test run.
    if (State.Type == ExitType::Stopped && State.Val == SIGALRM)
      return false;
    assert(State.Type == ExitType::Stopped && State.Val == SIGTRAP &&
           "SingleStep should stop the program with a SIGTRAP.");
  }

  // Now try to flip the bit.
  if (!RM.tryBitFlip(Reg.Name, Bit))
    return false;

  // Continue the execution.
  ptraceSafe(PTRACE_CONT, ChildPID, 0, 0);

  dbg(2) << "PTRACE_CONT\n";
  return true;
}

// Stop and inject the fault. Upon failure make sure that the child is killed.
bool Runner::tryInjectFaultAt(Statistics &Stats, double InjectionTime) {
  // Try to stop the child. This fails if the binary has already stopped, so no
  // need to kill it.
  if (!stopChildAfter(InjectionTime)) {
    Stats.incr(Type::InjFailed);
    dbg(2) << "stopChildAfter() failed. Child has already stopped?\n";
    return false;
  }

  // Now that the child has stopped, try to inject a fault.
  // Fault injection could fail for various reasons (e.g., instruction
  // accessing no registers, unimplemented features, etc.) so keep trying.
  if (!doBitFlip()) {
    Stats.incr(Type::InjFailed);
    dbg(2) << "doBitFlip() failed\n";
    // We failed to inject a bit-flip, so kill the child.
    killSafe(ChildPID, SIGKILL);
    return false;
  }
  return true;
}

bool Runner::systemCustom(const char *Cmd, const char *Shell) {
  pid_t ChildPID = forkSafe();
  if (ChildPID == 0) {
    // Child process.
    if (!DiffDisableRedirect.getValue()) {
      int DevNull = open("/dev/null", O_WRONLY);
      dup2(DevNull, 1);
      dup2(DevNull, 2);
    }
    if (execl(Shell, Shell, "-c", Cmd, (char *)0) == -1)
      die("execl() failed");
  } else {
    // Parent process.
    int Status = waitpidSafe(ChildPID);
    ExitState ExState = getWaitPidExitState(Status);
    return ExState.Type == ExitType::Exited && ExState.Val == 0;
  }
  assert(0 && "Unreachable under normal operation.");
  return false;
}

bool Runner::checkOutput(const ExitState &TestExitState) {
  // Sanity check. Make sure the states are initialized.
  if (!OrigExState->isSet())
    userDie("The exit state of the timing run is not set.");
  if (!TestExitState.isSet())
    userDie("The exit state of the test run is not set.");

  // If user has provided a command line for comparing
  if (DiffCmd.isSet()) {
    // Create the evaluated version of the diff cmd.
    // Replace %ORIG_STDOUT with OrigStdout, %ORIG_STDERR with OrigStderr,
    // %NEW_STDOUT TestStdout, %TEST_STDERR with TestStderr.
    std::string EvalDiffCmd = DiffCmd.getValue();
    strReplace(EvalDiffCmd, ORIG_STDOUT, OrigExState->getStdoutFile());
    strReplace(EvalDiffCmd, ORIG_STDERR, OrigExState->getStderrFile());
    strReplace(EvalDiffCmd, TEST_STDOUT, ExState.getStdoutFile());
    strReplace(EvalDiffCmd, TEST_STDERR, ExState.getStderrFile());

    dbg(2) << "User Command: " << EvalDiffCmd << "\n";
    bool OK = systemCustom(EvalDiffCmd.c_str(), DiffShell.getValue());
    return OK;
  }
  // The default diff does not allow for any discrepancies
  else {
    dbg(2) << "defaultDiff()\n";
    return ExState == *OrigExState;
  }
}
