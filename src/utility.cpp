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
#include "verify.hpp"
#include <iostream>
#include <time.h>
#include <unistd.h>
#include <cmath>
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
 * @param precision Number of decimal places to format Pi
 * @return The formatted value, as long as the precision asks for
 */
string format_pi(const mpf_class &pi_value, int precision)
{
  // Extra decimal places fetched on top of the ones displayed, then thrown away below.
  // GMP rounds the last place it hands back, and rounding a 9 upward carries into the
  // place before it. One spare place is not enough, since the carry lands on the last
  // digit shown. These give the carry somewhere harmless to go
  const int spare_digits = 8;

  // GMP returns the digits with no decimal point in them, along with an exponent
  // saying how many of those digits belong in front of the point. A value of zero
  // is returned as an empty string with an exponent of zero
  mp_exp_t exp;
  string digits = pi_value.get_str(exp, 10, precision + 1 + spare_digits);

  // Separate any minus sign so that it is not mistaken for a digit below
  string sign;
  if (!digits.empty() && digits[0] == '-')
  {
    sign = "-";
    digits.erase(0, 1);
  }

  // Give a zero value a digit to work with, since GMP supplies none
  if (digits.empty())
  {
    digits = "0";
    exp = 1;
  }

  // Keep the exponent inside a sane range so that the padding below cannot ask for
  // an enormous string. Anything outside this range is trimmed off the result anyway
  if (exp < -static_cast<mp_exp_t>(precision))
  {
    exp = -static_cast<mp_exp_t>(precision);
  }
  if (exp > static_cast<mp_exp_t>(MAX_PI_DIGITS))
  {
    exp = static_cast<mp_exp_t>(MAX_PI_DIGITS);
  }

  // Place the decimal point where the exponent says it belongs
  string formatted;
  if (exp <= 0)
  {
    // The value is smaller than one, so it needs a leading zero and padding zeros
    formatted = "0." + string(static_cast<size_t>(-exp), '0') + digits;
  }
  else if (static_cast<size_t>(exp) >= digits.length())
  {
    // Every digit sits in front of the point, so pad out to the point itself
    formatted = digits + string(static_cast<size_t>(exp) - digits.length(), '0') + ".";
  }
  else
  {
    formatted = digits;
    formatted.insert(static_cast<size_t>(exp), ".");
  }

  formatted = sign + formatted;

  // Fix the length at exactly the precision we want, dropping the extra digit. GMP
  // leaves trailing zeros off its digits, so padding them back keeps the value the same
  size_t point = formatted.find('.');
  formatted.resize(point + 1 + static_cast<size_t>(precision), '0');

  return formatted;
}

/**
 * Checks the calculated Pi value by deriving digits of Pi independently and
 * generates a detailed report.
 * @param calculated_pi The Pi value calculated by the program
 * @param precision The number of decimal places to check
 * @param method_name The method that produced the value, named on the verdict line
 * @return An AccuracyReport object containing formatted strings and the mismatch index.
 */
AccuracyReport compare_pi_accuracy(const mpf_class &calculated_pi, int precision,
                                   const string &method_name)
{
  AccuracyReport result;
  // result.mismatch_index is initialized to -1 by the constructor

  // The verdict line carries the method name so the results screen says which one
  // produced the digits below it. Worst case is the longest name at 21 characters
  // followed by two counts, and room is left for those counts to reach eight
  // figures: "Numerical Integration: 99999999 of 99999999 digit(s) confirmed" is
  // 62 characters, which leaves 13 spare on the narrowest console at 75 columns.
  // Keep any new wording inside that budget, since a line that wraps costs a row
  // the pagination below has not reserved
  const string verdict_label = method_name + ": ";

  if (calculated_pi <= 0)
  {
    result.set_summary("Not checked: the result is not a positive number.");
    result.add_line(verdict_label + "not checked, the value is not above zero");
    return result;
  }

  const string calculated_str = format_pi(calculated_pi, precision);

  const string calculated_pi_label = "Calculated Pi: ";
  const size_t max_line_width = 60;
  const size_t available_width = max_line_width - calculated_pi_label.length();
  const string red = "\x1b[31m";
  const string reset_color = "\x1b[37m";

  // Prove the checker still works before trusting anything it says. A checker
  // that had quietly broken would otherwise wave every result through
  if (!bbp_self_test())
  {
    result.set_summary("Not checked: the checker failed its own self test.");
    result.add_line(verdict_label + "not checked, BBP failed its own self test");
    return result;
  }

  // The check works in hexadecimal, so ask for enough hex digits to cover every
  // decimal place wanted, plus a couple so the last one is fully accounted for.
  // The value carries 64 spare bits beyond the places asked for, which is about
  // sixteen hex digits, so there is room for this.
  //
  // A single probe is a fixed width, so asking for fewer digits than one probe
  // covers would leave nothing to check against. Low precisions therefore check
  // a whole probe's worth regardless. Every method here works to a few places
  // beyond the ones requested, so those extra digits are there to be checked,
  // and the count reported is capped at what was asked for either way
  int hex_needed = static_cast<int>(ceil((precision + 2) / DECIMALS_PER_HEX_DIGIT));
  if (hex_needed < BBP_PROBE_DIGITS)
  {
    hex_needed = BBP_PROBE_DIGITS;
  }

  // Ask for the whole expansion rather than a set number of digits. Asking for a
  // set number makes GMP round the last one it hands back, and rounding upwards
  // can leave zeros on the end which GMP then strips, so the string comes back
  // both altered and shorter than requested. Taking everything and ignoring the
  // tail below keeps that rounding away from any digit being checked
  mp_exp_t hex_exp;
  string hex_all = calculated_pi.get_str(hex_exp, 16, 0);

  // Pi has exactly one digit ahead of the point in any base, and in hexadecimal
  // that digit is 3. Anything else is the wrong size altogether, not merely
  // imprecise, so there is nothing further worth checking
  if (hex_exp != 1 || hex_all.empty() || hex_all[0] != '3')
  {
    result.set_summary("Not confirmed: the result is not Pi.");
    result.set_mismatch_index(0);
    result.add_line(calculated_pi_label + calculated_str.substr(0, available_width));
    result.add_line(verdict_label + "not confirmed before the decimal point");
    return result;
  }

  // Drop the leading 3 so that position zero means the first digit after the point
  string pi_hex = hex_all.substr(1);

  // The last digit of the expansion sits where the stored value runs out, so it
  // is a rounding of what follows rather than a digit in its own right. Leave it
  // and its neighbour out, since checking either would fail a perfectly good
  // result and paint correct digits red
  int hex_checked = hex_needed;
  const int hex_usable = static_cast<int>(pi_hex.length()) - 2;
  if (hex_checked > hex_usable)
  {
    hex_checked = hex_usable;
  }

  const int confirmed_hex = bbp_confirmed_hex_digits(pi_hex, hex_checked);

  // Turn a count of hex digits back into decimal places. One less than the exact
  // conversion is claimed, so that a boundary landing between two digits is
  // always reported as the earlier one. Claiming too little is harmless whereas
  // claiming too much is the failure this whole check exists to prevent
  int confirmed_decimals = static_cast<int>(floor(confirmed_hex * DECIMALS_PER_HEX_DIGIT)) - 1;

  if (confirmed_decimals < 0)
  {
    confirmed_decimals = 0;
  }
  if (confirmed_decimals > precision)
  {
    confirmed_decimals = precision;
  }

  // Every place asked for held up, so there is no mismatch to point at
  if (confirmed_decimals >= precision)
  {
    string calc_display = calculated_str;
    if (calculated_str.length() > available_width)
    {
      calc_display = calculated_str.substr(0, available_width - 3) + "...";
    }

    result.set_summary("Verified: all " + to_string(precision) + " digit(s) confirmed.");
    result.add_line(calculated_pi_label + calc_display);
    result.add_line(verdict_label + "all " + to_string(precision) + " digit(s) confirmed");
    return result;
  }

  // Position 0 holds the leading 3 and position 1 holds the decimal point, so
  // the first unconfirmed decimal place sits two characters further along
  const int mismatch_idx = confirmed_decimals + 2;
  result.set_mismatch_index(mismatch_idx);
  result.set_summary("Not confirmed past " + to_string(confirmed_decimals)
    + " of " + to_string(precision) + " digit(s).");

  string calc_display;
  string arrow_line;

  // Case 1: The full string fits on one line without truncation.
  if (calculated_str.length() <= available_width)
  {
    calc_display = calculated_str;
    arrow_line = string(calculated_pi_label.length() + mismatch_idx, ' ') + red + "^" + reset_color;
  }
  // Case 2: The string is long, but the boundary is visible near the start.
  else if (mismatch_idx < static_cast<int>(available_width - 4))
  {
    calc_display = calculated_str.substr(0, available_width - 3) + "...";
    arrow_line = string(calculated_pi_label.length() + mismatch_idx, ' ') + red + "^" + reset_color;
  }
  // Case 3: The string is long and the boundary is far to the right.
  else
  {
    const int prefix_len = 8;
    const string ellipsis = "...";
    int context_len = available_width - prefix_len - ellipsis.length();

    int context_start = mismatch_idx - (context_len / 2);
    if (context_start + context_len >= static_cast<int>(calculated_str.length()))
    {
      context_start = calculated_str.length() - context_len;
    }

    calc_display = calculated_str.substr(0, prefix_len) + ellipsis
      + calculated_str.substr(context_start, context_len);

    int arrow_pos = calculated_pi_label.length() + prefix_len + ellipsis.length()
      + (mismatch_idx - context_start);
    arrow_line = string(arrow_pos, ' ') + red + "^" + reset_color;
  }

  result.add_line(calculated_pi_label + calc_display);
  result.add_line(arrow_line);
  result.add_line(verdict_label + to_string(confirmed_decimals) + " of "
    + to_string(precision) + " digit(s) confirmed");

  return result;
}

// EOF
