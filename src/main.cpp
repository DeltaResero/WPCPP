//  main.cpp
//
//  Wii Pi Calculator Project Plus (WPCPP)
//  Copyright (C) 2024 DeltaResero
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.

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

  // Main loop to keep the program running until the user decides to exit
  while (true)
  {
    // Prompt the user to select a method for calculating Pi and a desired precision level
    int method = method_selection_menu();
    int precision = precision_selection_menu();

    // Dynamically set GMP precision (number of bits) based on user input of how many digits of pi they want to calculate
    // 3.32 bits per decimal place is an approximation
    mpf_set_default_prec(precision * 3.32193);

    // Calculate Pi using the selected method and precision, then display the result
    calculate_and_display_pi(method, precision);
  }

  return 0;  // This point should never be reached, as the loop is infinite
}

// EOF
