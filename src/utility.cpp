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
#include "verify.hpp"
#include "input.hpp"
#include <iostream>
#include <iomanip>
#include <time.h>
#include <sys/time.h>
#include <cmath>
#include <cstdlib>
#include <ogcsys.h>
#include <string>

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

// Room set aside on the progress line for the estimate. The line is redrawn over
// itself, so every field on it is padded to a fixed size and the estimate is cut
// to fit rather than allowed to push the line wider. A line that grew would be
// too long for the erase below to clear, and a line that wrapped would cost a row
static const size_t progress_estimate_width = 30;

// State of the progress line. Kept here rather than passed around so that the
// calculations themselves need no extra arguments to report what they are doing
static int progress_total = 1;
static const char *progress_unit = "step";
static long progress_done = 0;
static long progress_done_at_last_draw = 0;
static int progress_draws = 0;
static int progress_spin = 0;
static double progress_anchor_seconds = 0.0;
static long progress_anchor_done = 0;
static int progress_count_width = 1;
static size_t progress_line_width = 0;
static bool progress_running = false;
static bool progress_cancel_asked = false;
static struct timeval progress_start;
static struct timeval progress_last_draw;
static struct timeval progress_last_poll;

/**
 * Measures the gap between two readings of the clock
 * @param from The earlier reading
 * @param to The later reading
 * @return The gap in seconds
 */
static double progress_seconds_between(const struct timeval &from, const struct timeval &to)
{
  return (to.tv_sec - from.tv_sec) + (to.tv_usec - from.tv_usec) / 1000000.0;
}

/**
 * Describes how much time is left, in whichever unit suits and to a deliberately
 * coarse figure. Anyone reading this wants to know whether to wait or walk away,
 * not to be told a number that turns out to be wrong
 * @param optimistic Shortest the wait could reasonably be, in seconds
 * @param pessimistic Longest the wait could reasonably be, in seconds
 * @return The wait in words, as a single figure when both ends agree
 */
static long progress_round_coarse(double value)
{
  // Nobody waiting on this needs the odd second. Past ten of any unit the figure
  // is rounded to the nearest five, which is also what keeps the two ends of the
  // range landing on the same number when they are close enough not to matter
  if (value < 10.0)
  {
    return lround(value);
  }
  return lround(value / 5.0) * 5;
}

static string progress_estimate_words(double optimistic, double pessimistic)
{
  if (lround(pessimistic) < 1)
  {
    return "< 1 second left";
  }

  // Settle the rounding before choosing the unit, so a wait a shade under a
  // minute is not announced as sixty seconds
  const char *unit = "hour(s)";
  double divisor = 3600.0;
  if (lround(pessimistic) < 60)
  {
    unit = "second(s)";
    divisor = 1.0;
  }
  else if (lround(pessimistic / 60.0) < 60)
  {
    unit = "minute(s)";
    divisor = 60.0;
  }

  long high = progress_round_coarse(pessimistic / divisor);
  long low = progress_round_coarse(optimistic / divisor);
  if (high < 1) { high = 1; }
  if (low > high) { low = high; }

  // Both ends landed on the same figure, so there is no range worth showing
  if (low == high)
  {
    return "about " + to_string(high) + " " + unit + " left";
  }

  // The shortest end rounds away to nothing in this unit, leaving a ceiling
  // rather than a range
  if (low < 1)
  {
    return "under " + to_string(high) + " " + unit + " left";
  }

  return "about " + to_string(low) + " to " + to_string(high) + " " + unit + " left";
}

/**
 * Draws the progress line over the top of itself
 * @param estimate The wait in words, or empty while there is nothing to say yet
 */
static void progress_draw(const string &estimate)
{
  static const char spinner[] = { '|', '/', '-', '\\' };

  // Never claim to be further along than the total, since the count for Machin's
  // formula is worked out in advance and can be short by a term or two
  long shown = progress_done;
  if (shown > progress_total) { shown = progress_total; }

  string words = estimate;
  if (words.length() > progress_estimate_width)
  {
    words.resize(progress_estimate_width);
  }

  cout << "\r[" << spinner[progress_spin & 3] << "] "
       << left << setw(5) << progress_unit << " "
       << right << setw(progress_count_width) << shown
       << " of " << setw(progress_count_width) << progress_total << "  "
       << left << setw(static_cast<int>(progress_estimate_width)) << words
       << right;
  cout.flush();
}

/**
 * Reads the controllers a few times a second while work is going on, so that a
 * long calculation can be cancelled or exited. The calculations never reach the
 * input code themselves, so without this the controllers go unread for as long
 * as the work lasts and the only way out is the console's power switch
 * @param now The clock reading the caller has already taken
 */
static void progress_poll_buttons(const struct timeval &now)
{
  // A tenth of a second is often enough to catch a button pressed and let go of,
  // and rare enough to cost nothing. Reading the controllers on every step would
  // cost more than the arithmetic being reported, since a step can be one digit
  if (progress_seconds_between(progress_last_poll, now) < 0.1)
  {
    return;
  }

  progress_last_poll = now;
  poll_inputs();

  // Leaving the program outright, the same buttons that leave every other screen
  if (is_button_just_pressed(PAD_BUTTON_START, WPAD_BUTTON_HOME))
  {
    exit_WPCPP();
  }

  // Cancelling, the same button that goes back a screen everywhere else
  if (is_button_just_pressed(PAD_BUTTON_B, WPAD_BUTTON_B))
  {
    progress_cancel_asked = true;
  }
}

/**
 * Starts reporting progress. Draws straight away rather than waiting, so that
 * pressing 'A' is answered immediately even when the work turns out to be quick
 * @param total_steps How many steps the work is expected to take
 * @param unit What one step is, named on the line: "digit", "term", "pass", "step"
 */
void progress_begin(int total_steps, const char *unit)
{
  progress_total = (total_steps > 0) ? total_steps : 1;
  progress_unit = (unit != nullptr) ? unit : "step";
  progress_done = 0;
  progress_done_at_last_draw = 0;
  progress_spin = 0;
  progress_count_width = static_cast<int>(to_string(progress_total).length());

  // Fixed part of the line, then both counts, then the estimate. Kept so that
  // ending the line can wipe exactly as much as was written
  progress_line_width = 16 + (2 * static_cast<size_t>(progress_count_width))
    + progress_estimate_width;

  gettimeofday(&progress_start, nullptr);
  progress_last_draw = progress_start;
  progress_last_poll = progress_start;
  progress_running = true;
  progress_cancel_asked = false;

  progress_draw("");
  progress_draws = 1;
}

/**
 * Records one step of work and redraws the line about once a second. Called from
 * inside the calculation loops, so it does as little as possible: one reading of
 * the clock, which comes straight off the processor's own counter
 * @return False once the user has cancelled, which the loop that called this is
 *         expected to answer by stopping
 */
bool progress_step()
{
  if (!progress_running)
  {
    return true;
  }

  progress_done++;

  struct timeval now;
  gettimeofday(&now, nullptr);

  progress_poll_buttons(now);
  if (progress_cancel_asked)
  {
    return false;
  }

  const double since_draw = progress_seconds_between(progress_last_draw, now);
  if (since_draw < 1.0)
  {
    return true;
  }

  string estimate;

  const double since_start = progress_seconds_between(progress_start, now);
  const long steps_since_draw = progress_done - progress_done_at_last_draw;
  const long remaining = progress_total - progress_done;

  // Keep the first drawn line as a fixed point to measure against. Two readings
  // taken a while apart are what makes the shape of the work visible
  if (progress_draws == 1)
  {
    progress_anchor_seconds = since_start;
    progress_anchor_done = progress_done;
  }
  else if (steps_since_draw > 0 && remaining > 0
           && progress_done > progress_anchor_done
           && since_start > progress_anchor_seconds && progress_anchor_seconds > 0)
  {
    // Some methods cost the same on every step while others get steadily dearer,
    // and guessing which is which was wrong often enough to matter. So measure it
    // instead. Comparing how much longer the work has run against how many more
    // steps it has managed says how sharply the cost is climbing, and that one
    // figure describes both sorts: it comes out at one for steady work and higher
    // for work that is slowing down. Nothing here is told which method it is in
    double climb = log(since_start / progress_anchor_seconds)
      / log(static_cast<double>(progress_done) / static_cast<double>(progress_anchor_done));
    if (climb < 1.0) { climb = 1.0; }
    if (climb > 4.0) { climb = 4.0; }

    // Steady work would simply finish in proportion to the steps left. Work that
    // is climbing has to allow for the steps left being dearer than the ones done
    const double share_left = static_cast<double>(progress_total) / static_cast<double>(progress_done);
    const double allowing_for_climb = since_start * (pow(share_left, climb) - 1.0);

    // The nearer end assumes the current speed simply holds, which is right for
    // steady work and hopeful for anything else
    const double at_current_speed = remaining / (steps_since_draw / since_draw);

    estimate = progress_estimate_words(
      (at_current_speed < allowing_for_climb) ? at_current_speed : allowing_for_climb,
      (at_current_speed < allowing_for_climb) ? allowing_for_climb : at_current_speed);
  }

  progress_draw(estimate);

  progress_spin++;
  progress_draws++;
  progress_last_draw = now;
  progress_done_at_last_draw = progress_done;

  return true;
}

/**
 * Says whether the calculation that has just ended was cancelled or finished
 * @return True when the user stopped it part way through
 */
bool progress_cancelled()
{
  return progress_cancel_asked;
}

/**
 * Wipes the progress line and stops reporting. Nothing is left on screen
 * suggesting work is still going on once it has finished
 */
void progress_end()
{
  if (!progress_running)
  {
    return;
  }

  progress_running = false;
  cout << "\r" << string(progress_line_width, ' ') << "\r";
  cout.flush();
}

/**
 * Turns a length of time into words, choosing units to suit its size
 * @param milliseconds The length of time to describe
 * @return The time written out, from thousandths of a second up to hours
 */
string format_duration(double milliseconds)
{
  if (milliseconds < 0)
  {
    milliseconds = 0;
  }

  const long whole_ms = lround(milliseconds);

  // The quickest methods finish in a fraction of a millisecond, and a whole
  // number would report those as no time at all
  if (whole_ms < 10)
  {
    const long tenths = lround(milliseconds * 10.0);
    return to_string(tenths / 10) + "." + to_string(tenths % 10) + " millisecond(s)";
  }

  if (whole_ms < 1000)
  {
    return to_string(whole_ms) + " millisecond(s)";
  }

  // Round to a tenth of a second before deciding whether this is still seconds.
  // Deciding first would report a hair under a minute as sixty seconds
  const long tenths = lround(milliseconds / 100.0);
  if (tenths < 600)
  {
    return to_string(tenths / 10) + "." + to_string(tenths % 10) + " second(s)";
  }

  const long total_seconds = lround(milliseconds / 1000.0);
  const long hours = total_seconds / 3600;
  const long minutes = (total_seconds % 3600) / 60;
  const long seconds = total_seconds % 60;

  const string minutes_and_seconds = to_string(minutes) + " minute(s) "
    + to_string(seconds) + " second(s)";

  // Hours are here so the scale never needs revisiting, not because anything
  // reaches them today
  if (hours == 0)
  {
    return minutes_and_seconds;
  }

  return to_string(hours) + " hour(s) " + minutes_and_seconds;
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
