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

#include "utils.h"
#include <fcntl.h>
#include <mutex>
#include <sys/stat.h>
#include <sys/types.h>

unsigned randSafe(unsigned Max) {
  static std::mutex Mtx;
  std::lock_guard<std::mutex> Lock(Mtx);
  // Modulo should be good enough for low values of Max.
  return Max ? rand() % Max : rand();
}

/// \Returns an unsigned int from /dev/random.
static unsigned int getUrandom() {
  unsigned Num = 0;
  int Fd = open("/dev/urandom", O_RDONLY);
  if (Fd == -1)
    die("Failed to open /dev/urandom");
  if (read(Fd, &Num, 4) != 4)
    die("Failed to read 4 bytes from /dev/urandom");
  return Num;
}

void randInit() {
#ifdef USE_TIME_PID_SEED
  // Use time and pid.
  time_t Secs;
  time(&Secs);
  srand((unsigned int)Secs ^ (unsigned int)getpid());
#else
  // Use /dev/urandom.
  srand(getUrandom());
#endif
}
