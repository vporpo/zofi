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

#include <mutex>
#include "utils.h"

unsigned randSafe(unsigned Max) {
  static std::mutex Mtx;
  std::lock_guard<std::mutex> Lock(Mtx);
  // Modulo should be good enough for low values of Max.
  if (Max)
    return rand() % Max;
  else
    return rand();
}
