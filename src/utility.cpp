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
#include <iostream>
#include <unistd.h>  // For usleep() to pause the program
#include <cstdio>
#include <cstdlib>
#include <ogcsys.h>
#include <wiiuse/wpad.h>

using namespace std;  // Use the entire std namespace for simplicity

/**
 * Exits the program and attempts to return to the Homebrew Channel or system menu
 * Always waits for 3 seconds before exiting
 */
void exit_WPCPP()
{
  // Print exit message
  cout << "\nExiting to Homebrew Channel..." << endl;

  // Wait for 3 seconds (3000 milliseconds)
  usleep(3000000);  // 3 seconds in microseconds

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
        PAD_ScanPads();  // Update GameCube controller state
        WPAD_ScanPads();  // Update Wii Remote state
        u32 gc_pressed = PAD_ButtonsDown(0);  // Get GameCube Controller button state
        u32 wii_pressed = WPAD_ButtonsDown(0);  // Get Wii Remote button state
        if (gc_pressed || wii_pressed)  // Check if any button is pressed
        {
            break;
        }
        VIDEO_WaitVSync();  // Wait for video sync to ensure smooth input handling
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
  // Convert the mpf_class Pi to a string with the specified precision + 1 extra digit
  mp_exp_t exp;  // For storing the exponent
  string pi_str_raw = pi_value.get_str(exp, 10, precision + 1);

  // Insert the decimal point after the first digit (since Pi starts with '3')
  pi_str_raw.insert(1, ".");

  // Copy the string to the provided buffer
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

  // Format the calculated Pi string
  format_pi(calculated_pi, calculated_str, precision);

  // Pi with up to 100 decimal places (can be truncated to match user-selected precision)
  const char *pi_digits = "3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679";

  // Create a buffer to hold the truncated actual Pi
  char actual_pi_str[TOTAL_LENGTH];
  snprintf(actual_pi_str, precision + 3, "%s", pi_digits); // Truncate to 'precision + 2' characters (3. + precision digits)

  cout << "Comparing calculated Pi to the actual value of Pi (up to " << precision << " decimal places)" << endl;

  // Check if the first 2 characters are '3.' before comparing decimal places
  if (strncmp(calculated_str, "3.", 2) != 0)
  {
    // Should not normally end up here
    cout << "None of the digits are correct!" << endl;
    return;
  }

  // Compare the digits after the decimal point
  int mismatch_index = 2;
  while (mismatch_index < precision + 2 && calculated_str[mismatch_index] == actual_pi_str[mismatch_index])
  {
    ++mismatch_index;
  }

  // If there is no mismatch, print results and that all digits are correct
  if (mismatch_index == precision + 2)
  {
    // All digits match
    cout << "Actual Pi:     " << actual_pi_str << endl;
    cout << "Calculated Pi: " << calculated_str << endl;
    cout << "All " << precision << " digit(s) after the decimal are correct!" << endl;
  }
  else
  {
    // Found a mismatch so print where the first mismatch occurred
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
