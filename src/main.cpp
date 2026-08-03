// src/main.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Wii Pi Calculator Project Plus (WPCPP)
// Copyright (C) 2024-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include <gmpxx.h>
#include <iostream>
#include "video.hpp"
#include "pi_calculation.hpp"
#include "menu.hpp"
#include "utility.hpp"
#include "input.hpp"

/**
 * Main function that runs the Pi calculation loop
 * Initializes the video system, displays menus for configuring, and performs
 * Pi calculations based on input until the user exits the program
 */
int main()
{
  // Initialize the video system and prepare the display
  initialize_video();

  // Initialize inputs for Wii Remote and GameCube Controllers
  initialize_inputs();

  // Spare bits carried on top of whatever precision the user asks for. Every multiply
  // and divide rounds its result off a little, and those roundings pile up over the
  // thousands of operations a run takes. The spare bits give that loss somewhere to
  // go besides the digits being displayed
  const mp_bitcnt_t guard_bits = 64;

  // Where the two selection screens open. Held out here rather than inside them so
  // that a screen left and returned to shows the last answer given to it, which is
  // what makes going back a screen worth doing
  int method = 0;
  int precision = 50;

  // Main loop to keep the program running until the user decides to exit
  while (true)
  {
    // Prompt the user to select a method for calculating Pi
    method = method_selection_menu(method);

    // Stay with that method for as long as B keeps being pressed. Someone who stops
    // a calculation, or who has read a result and wants another, has nearly always
    // asked the wrong number of digits rather than picked the wrong method
    while (true)
    {
      // Backing out of the precision screen is the way back to the method list. It
      // leaves the figure it was showing behind either way, so the digits dialled
      // in before changing method are still there on the way back through
      if (!precision_selection_menu(precision))
      {
        break;
      }

      // Dynamically set GMP precision (number of bits) based on user input of how many digits of pi they want to calculate
      // 3.32193 bits per decimal place is an approximation of log2(10)
      mpf_set_default_prec(static_cast<mp_bitcnt_t>(precision * 3.32193) + guard_bits);

      // Calculate Pi using the selected method and precision, then display the result
      const bool went_back = calculate_and_display_pi(method, precision);

      // Anything other than going back a screen is done with this method, so the
      // method list comes around again
      if (!went_back)
      {
        break;
      }
    }
  }

  return 0;  // This point should never be reached, as the loop is infinite
}

// EOF
