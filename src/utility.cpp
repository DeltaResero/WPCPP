// utility.cpp
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

#include "utility.hpp"
#include "input.hpp"
#include <iostream>
#include <time.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <ogcsys.h>

using namespace std;  // Use the entire std namespace for simplicity

/**
 * Exits the program and attempts to return to the Homebrew Channel or system menu
 * Always waits for 3 seconds before exiting
 */
void exit_WPCPP()
{
  // Print exit message
  cout << "\nExiting to Homebrew Channel..." << endl;

  // Wait for 3 seconds before exiting
  struct timespec req = {3, 0};  // 3 seconds sleep
  nanosleep(&req, nullptr);

  // Reset the system and return to Homebrew Channel (or system menu if Homebrew isn't available)
  SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);

  // Fallback in case the system reset fails
  exit(1);
}

void wait_for_user_input_to_return()
{
    std::cout << "Press any button to return to the menu." << std::endl;
    while (true)
    {
      // Update the input states
      poll_inputs();

      // Check if any button on the GameCube controller or Wii Remote is pressed
      if (is_button_just_pressed(0xFFFFFFFF, 0xFFFFFFFF))
      {
        break;
      }

      // Wait for video sync to ensure smooth input handling
      VIDEO_WaitVSync();
    }
}

/**
 * Formats the Pi value into a string with a specified number of decimal places
 * @param pi_value The Pi value to format
 * @param pi_str The string buffer where the formatted Pi will be stored
 * @param precision Number of decimal places to format Pi
 */
void format_pi(const mpf_class &pi_value, char *pi_str, int precision)
{
  // Work with one extra digit of precision to handle rounding properly
  int working_precision = precision + 1;

  // Convert with extra precision
  mp_exp_t exp;
  string pi_str_raw = pi_value.get_str(exp, 10, working_precision + 1);

  // Insert the decimal point after the first digit
  pi_str_raw.insert(1, ".");

  // Truncate to exactly the precision we want (removing the extra digit)
  if (pi_str_raw.length() > static_cast<std::string::size_type>(precision + 2))  // +2 for "3."
  {
    pi_str_raw = pi_str_raw.substr(0, precision + 2);
  }

  // Copy to output buffer
  snprintf(pi_str, TOTAL_LENGTH, "%s", pi_str_raw.c_str());
}

/**
 * Compares the calculated Pi value with the actual Pi value (up to the specified precision).
 * This function prints the comparison result and identifies the first mismatched digit (if any)
 * @param calculated_pi The Pi value calculated by the program
 * @param precision The number of decimal places to compare
 */
void compare_pi_accuracy(const mpf_class &calculated_pi, int precision)
{
  if (calculated_pi <= 0)
  {
    cout << "Invalid input: Pi cannot be less than or equal to zero." << endl;
    return;
  }

  // Buffers for the calculated Pi string
  char calculated_str[TOTAL_LENGTH];

  // Format the calculated Pi string with truncation instead of rounding
  format_pi(calculated_pi, calculated_str, precision);

  // Pi with up to 100 decimal places
  const char *pi_digits = "3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679";

  // Create a buffer for the reference Pi truncated to exact precision
  char actual_pi_str[TOTAL_LENGTH];
  snprintf(actual_pi_str, precision + 3, "%s", pi_digits);  // +3 for null terminator after "3." + precision digits

  cout << "Comparing calculated Pi to the actual value of Pi (up to " << precision << " decimal places)" << endl;

  // Verify the basic format first
  if (strncmp(calculated_str, "3.", 2) != 0)
  {
    cout << "Actual Pi:     " << actual_pi_str << endl;
    cout << "Calculated Pi: " << calculated_str << endl;
    cout << "None of the digits are correct!" << endl;
    return;
  }

  // Compare digits after decimal point
  bool exact_match = true;
  int mismatch_index = 2;  // Start after "3."

  while (mismatch_index < precision + 2 && calculated_str[mismatch_index] && actual_pi_str[mismatch_index])
  {
    if (calculated_str[mismatch_index] != actual_pi_str[mismatch_index])
    {
      exact_match = false;
      break;
    }
    ++mismatch_index;
  }

  // Output results
  if (exact_match && mismatch_index == precision + 2)
  {
    cout << "Actual Pi:     " << actual_pi_str << endl;
    cout << "Calculated Pi: " << calculated_str << endl;
    cout << "All " << precision << " digit(s) after the decimal are correct!" << endl;
  }
  else
  {
    print_mismatch(calculated_str, actual_pi_str, mismatch_index);
  }
}

/**
 * Prints the location of the first mismatched digit between the calculated
 * Pi string and the actual Pi string
 * @param calculated_str The string of the calculated Pi value
 * @param actual_str The string representing the actual Pi value
 * @param mismatch_index The index where the mismatch occurs
 */
void print_mismatch(const char *calculated_str, const char *actual_str, int mismatch_index)
{
  cout << "Actual Pi:     " << actual_str << endl;
  cout << "Calculated Pi: " << calculated_str << endl;

  // Print an arrow pointing to the first mismatch
  cout << "               ";  // Aligns the arrow with the Pi values
  for (int i = 0; i < mismatch_index; ++i)
  {
    cout << " ";  // Create space for the arrow to point under the mismatched digit
  }
  // Output an arrow at the location of mismatch
  cout << "^\n";
  cout << "First mismatch at: " << (mismatch_index - 1) << " digit(s) after the decimal" << endl;
}

// EOF
