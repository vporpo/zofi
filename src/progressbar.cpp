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

#include "progressbar.h"
#include <cassert>
#include <iomanip>

ProgressBar::ProgressBar(unsigned long Max, unsigned Size, std::ostream &OS)
    : Max(Max), Size(Size), OS(OS), StartTime(getTime()) {
  P0 = 0;
  P25 = (Size * 25) / 100;
  P50 = (Size * 50) / 100;
  P75 = (Size * 75) / 100;
  P100 = Size - 1;
}

void ProgressBar::drawCounter(unsigned long Cnt) const {
  double Seconds = getTimeDiff(StartTime, getTime());
  double Rate = (Seconds > 0) ? ((float)Cnt / Seconds) : 0.0;
  OS << std::right << std::setw(RateSize) << std::setprecision(2) << std::fixed
     << Rate << "r/s ";

  // ETA
  prettyPrintTime((Max - Cnt) * Seconds / Cnt, OS);

  OS << std::right << std::setw(CounterSize) << Cnt << "/" << std::left
     << std::setw(CounterSize) << Max << ": ";
  OS.flush();
}

void ProgressBar::clearLineAndCR() const {
  // This is a VT100 escape code, so it should work in most terminals.
  OS << "\33[2K";
  // Carriage Return.
  OS << "\r";
  OS.flush();
}

void ProgressBar::draw(unsigned End, const char *C) {
  for (unsigned Cnt = 0; Cnt < End; ++Cnt) {
    // Print percentage every 25%.
    if (Cnt == P0)
      OS << "0%";
    else if (Cnt == P25)
      OS << "25%";
    else if (Cnt == P50)
      OS << "50%";
    else if (Cnt == P75)
      OS << "75%";
    else if (Cnt == P100)
      OS << "100%";
    else
      OS << C;
  }
  OS.flush();
  // Remember the number of characters drawn on screen.
  LastFillSize = End;
}

void ProgressBar::init() {
  drawCounter(0);
  draw(Size, " ");
}

void ProgressBar::display(unsigned long Val) {
  // First clear the bar.
  clearLineAndCR();
  // Next draw the new bar.
  drawCounter(Val);
  unsigned FillSize = (Val * Size) / Max;
  draw(FillSize, "=");
}

void ProgressBar::finalize() const {
  OS << "\n";
  OS.flush();
}
