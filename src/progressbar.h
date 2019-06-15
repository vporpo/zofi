//-*- C++ -*-
// A fancy progress bar.
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

#ifndef __PROGRESSBAR_H__
#define __PROGRESSBAR_H__

#include "utils.h"
#include <iostream>

/// An ascii progress bar.
class ProgressBar {
  /// The maximum value of the value to be represetned by the progress bar.
  unsigned long Max = 0;

  /// The progress bar width in number of characters.
  unsigned Size = 0;

  /// The padding of the rate counter.
  const unsigned RateSize = 6;

  /// The padding of the counter.
  const unsigned CounterSize = 8;

  unsigned P0 = 0;
  unsigned P25 = 0;
  unsigned P50 = 0;
  unsigned P75 = 0;
  unsigned P100 = 0;

  /// The output stream.
  std::ostream &OS;

  /// Remember the fill size of last draw().
  unsigned LastFillSize = 0;

  /// The start time.
  TimePoint StartTime;

  /// Clears the current line and moves cursor to the beginning.
  void clearLineAndCR() const;

  /// Draw the bar until \p End using \p C.
  void draw(unsigned End, const char *C = nullptr);

  /// Draw the counter.
  void drawCounter(unsigned long Cnt) const;

public:
  /// \p Max is the maximum value represented by the counter. \p Size is the
  /// width of the progress bar on the screen (number of characters).
  ProgressBar(unsigned long Max, unsigned Size, std::ostream &OS = std::cerr);

  /// Print the initial bar filled with blank spaces.
  void init();

  /// Update the progress bar to reflect \p Val.
  void display(unsigned long Val);

  /// Finalize the progress bar and print a new line character.
  void finalize() const;
};

#endif //__PROGRESSBAR_H__
