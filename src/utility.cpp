// src/utility.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Wii Pi Calculator Project Plus (WPCPP)
// Copyright (C) 2024-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "utility.hpp"
#include "input.hpp"
#include <iostream>
#include <time.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <ogcsys.h>
#include <string>
#include <vector>
#include <algorithm>

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
    pi_str_raw.resize(precision + 2);
  }

  // Copy to output buffer
  snprintf(pi_str, TOTAL_LENGTH, "%s", pi_str_raw.c_str());
}

/**
 * Compares the calculated Pi value with the actual Pi value and generates a detailed report.
 * @param calculated_pi The Pi value calculated by the program
 * @param precision The number of decimal places to compare
 * @return An AccuracyReport object containing formatted strings and the mismatch index.
 */
AccuracyReport compare_pi_accuracy(const mpf_class &calculated_pi, int precision)
{
  AccuracyReport result;
  // result.mismatch_index is initialized to -1 by the constructor

  if (calculated_pi <= 0)
  {
    result.add_line("Invalid input: Pi cannot be less than or equal to zero.");
    return result;
  }

  // Buffers for the string representations of Pi
  char calculated_str_c[TOTAL_LENGTH];
  format_pi(calculated_pi, calculated_str_c, precision);
  string calculated_str(calculated_str_c);

  // Pi with up to 1000 decimal places
  const char *pi_digits = "3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679"
    "8214808651328230664709384460955058223172535940812848111745028410270193852110555964462294895493038196"
    "4428810975665933446128475648233786783165271201909145648566923460348610454326648213393607260249141273"
    "7245870066063155881748815209209628292540917153643678925903600113305305488204665213841469519415116094"
    "3305727036575959195309218611738193261179310511854807446237996274956735188575272489122793818301194912"
    "9833673362440656643086021394946395224737190702179860943702770539217176293176752384674818467669405132"
    "0005681271452635608277857713427577896091736371787214684409012249534301465495853710507922796892589235"
    "4201995611212902196086403441815981362977477130996051870721134999999837297804995105973173281609631859"
    "5024459455346908302642522308253344685035261931188171010003137838752886587533208381420617177669147303"
    "5982534904287554687311595628638823537875937519577818577805321712268066130019278766111959092164201989";
  char actual_pi_str_c[TOTAL_LENGTH];
  snprintf(actual_pi_str_c, precision + 3, "%s", pi_digits);
  string actual_pi_str(actual_pi_str_c);

  // Find the first mismatch
  int mismatch_idx = -1;
  for (size_t i = 0; i < actual_pi_str.length(); ++i)
  {
    if (i >= calculated_str.length() || actual_pi_str[i] != calculated_str[i])
    {
      mismatch_idx = i;
      break;
    }
  }

  const string actual_pi_label = "Actual Pi:     ";
  const string calculated_pi_label = "Calculated Pi: ";
  const size_t max_line_width = 60;
  const size_t available_width = max_line_width - actual_pi_label.length();

  // If there is no mismatch, return a success report
  if (mismatch_idx == -1)
  {
    string actual_display;
    string calc_display;

    if (actual_pi_str.length() > available_width)
    {
      actual_display = actual_pi_str.substr(0, available_width - 3) + "...";
      calc_display = calculated_str.substr(0, available_width - 3) + "...";
    }
    else
    {
      actual_display = actual_pi_str;
      calc_display = calculated_str;
    }
    result.add_line(actual_pi_label + actual_display);
    result.add_line(calculated_pi_label + calc_display);
    result.add_line("All " + to_string(precision) + " digit(s) after the decimal are correct!");
    return result;
  }

  result.set_mismatch_index(mismatch_idx);
  string actual_display;
  string calc_display;
  string arrow_line;
  const string red = "\x1b[31m";
  const string reset_color = "\x1b[37m";

  // Case 1: The full string fits on one line without truncation.
  if (actual_pi_str.length() <= available_width)
  {
    actual_display = actual_pi_str;
    calc_display = calculated_str;
    arrow_line = string(calculated_pi_label.length() + mismatch_idx, ' ') + red + "^" + reset_color;
  }
  // Case 2: The string is long, but the mismatch is visible near the start.
  else if (mismatch_idx < static_cast<int>(available_width - 4))
  {
    actual_display = actual_pi_str.substr(0, available_width - 3) + "...";
    calc_display = calculated_str.substr(0, available_width - 3) + "...";
    arrow_line = string(calculated_pi_label.length() + mismatch_idx, ' ') + red + "^" + reset_color;
  }
  // Case 3: The string is long and the mismatch is far to the right.
  else
  {
    const int prefix_len = 8;
    const string ellipsis = "...";
    int context_len = available_width - prefix_len - ellipsis.length();

    int context_start = mismatch_idx - (context_len / 2);
    if (context_start + context_len >= static_cast<int>(actual_pi_str.length()))
    {
      context_start = actual_pi_str.length() - context_len;
    }

    string prefix = actual_pi_str.substr(0, prefix_len);
    string actual_context = actual_pi_str.substr(context_start, context_len);
    string calc_context = calculated_str.substr(context_start, context_len);

    actual_display = prefix + ellipsis + actual_context;
    calc_display = prefix + ellipsis + calc_context;

    int arrow_pos = calculated_pi_label.length() + prefix.length() + ellipsis.length() + (mismatch_idx - context_start);
    arrow_line = string(arrow_pos, ' ') + red + "^" + reset_color;
  }

  result.add_line(actual_pi_label + actual_display);
  result.add_line(calculated_pi_label + calc_display);
  result.add_line(arrow_line);
  result.add_line("First mismatch at: " + to_string(mismatch_idx - 1) + " digit(s) after the decimal");

  return result;
}

// EOF
