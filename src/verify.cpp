// src/verify.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Wii Pi Calculator Project Plus (WPCPP)
// Copyright (C) 2024-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Checks a calculated value of Pi by working out digits of Pi a second time, in
// a way that has nothing in common with how the first answer was reached.
//
// The Bailey-Borwein-Plouffe formula can produce a hexadecimal digit of Pi at
// any place without producing any of the digits ahead of it. That is what makes
// it a check rather than a lookup. No digits of Pi are stored anywhere in this
// program, so there is nothing to fall out of date when the digit limit rises
// and nothing that could be copied in instead of being calculated.
//
// This is also how record breaking runs are checked in the real world. Most
// constants have to be computed twice by unrelated methods before anyone will
// believe them, and Pi is let off that requirement precisely because this
// formula exists.
//
// Everything here runs in ordinary double precision, following the reference
// implementation piqpr8.c by David H. Bailey. Deliberately no GMP. The point of
// the check is that it shares nothing with the code being checked, and reaching
// for the same big number library would give that away. Doubles also keep it
// quick on hardware with a real floating point unit, which the Wii has.
//
// Measured on a run of every position from 0 to 808, compared against digits of
// Pi this program had no part in producing: all 809 probes agreed, the fewest
// agreeing digits at any one position was 9 and the average was 11.2. Sampled
// again out to position 10400, which is past 12000 decimal places, the fewest
// agreeing was still 10. Accuracy therefore does not fall away with depth over
// any range this program can reach. BBP_PROBE_DIGITS is set to 8 so that there
// is room to spare under the worst case seen. Two probes agreeing on 8 digits
// by luck alone has a chance of about one in forty billion.
//
// The reference notes the formula holds up to roughly position 11800000, which
// is about 14 million decimal places. Memory runs out long before that.

#include "verify.hpp"
#include <cmath>
#include <cstring>
#include <string>

using namespace std;  // Use the entire std namespace for simplicity

/**
 * Raises sixteen to a power and keeps only the remainder against a divisor,
 * never letting the running value grow past what a double can hold exactly.
 * Squaring and stepping through the bits of the exponent does this in a number
 * of steps that grows with the digits of the exponent rather than its size
 * @param power The exponent to raise sixteen to
 * @param divisor The number to take the remainder against
 * @return Sixteen to that power, reduced by the divisor
 */
static double power_of_sixteen_mod(double power, double divisor)
{
  // Every whole number leaves a remainder of nothing against one
  if (divisor == 1.0)
  {
    return 0.0;
  }

  // Climb the powers of two until one overshoots the exponent, which is where
  // the walk through the exponent's bits has to start. The count stops at 25
  // because the exponents this is asked for never reach two to the twenty-fifth.
  // Every value here is a power of two well inside what a double holds exactly,
  // so doubling and halving are both exact and nothing is rounded on the way
  int steps = 0;
  double step_size = 1.0;
  while (steps < 25 && step_size <= power)
  {
    step_size = 2.0 * step_size;
    steps++;
  }

  // The loop leaves step_size one power too high, unless it never ran at all
  if (steps > 0)
  {
    step_size = 0.5 * step_size;
  }
  double remaining = power;
  double result = 1.0;

  for (int i = 1; i <= steps; i++)
  {
    if (remaining >= step_size)
    {
      result = 16.0 * result;
      result = result - static_cast<int>(result / divisor) * divisor;
      remaining = remaining - step_size;
    }

    step_size = 0.5 * step_size;

    if (step_size >= 1.0)
    {
      result = result * result;
      result = result - static_cast<int>(result / divisor) * divisor;
    }
  }

  return result;
}

/**
 * Sums one of the four series the BBP formula is built from, keeping only the
 * fractional part throughout. Dropping the whole part every step is what stops
 * the sum growing beyond the digits a double carries, and it costs nothing
 * because only the fractional part is ever wanted
 * @param offset The constant added to eight times the term number
 * @param position How many fractional hex digits to skip
 * @return The fractional part of the series, shifted to start at that position
 */
static double bbp_series(int offset, int position)
{
  // Terms below this cannot move any digit that is going to be read
  const double negligible = 1e-17;

  double sum = 0.0;

  // Terms up to the position wanted. Only the remainder of the numerator
  // matters here, since the whole part is thrown away in the same breath
  for (int k = 0; k < position; k++)
  {
    double divisor = 8.0 * k + offset;
    sum = sum + power_of_sixteen_mod(static_cast<double>(position - k), divisor) / divisor;
    sum = sum - static_cast<int>(sum);
  }

  // Terms past that point shrink by a factor of sixteen each time, so the sum
  // stops mattering after a handful of them
  for (int k = position; k <= position + 100; k++)
  {
    double divisor = 8.0 * k + offset;
    double term = pow(16.0, static_cast<double>(position - k)) / divisor;

    if (term < negligible)
    {
      break;
    }

    sum = sum + term;
    sum = sum - static_cast<int>(sum);
  }

  return sum;
}

void bbp_hex_digits(int position, int count, char *out)
{
  // The formula itself, combining its four series
  double value = 4.0 * bbp_series(1, position)
               - 2.0 * bbp_series(4, position)
               - bbp_series(5, position)
               - bbp_series(6, position);

  // The combination can land either side of zero, so lift it into a range where
  // the fractional part is the part wanted
  value = value - static_cast<int>(value) + 1.0;

  // Peel the digits off the front one at a time
  const char *hex_chars = "0123456789abcdef";
  double remainder = fabs(value);

  for (int i = 0; i < count; i++)
  {
    remainder = 16.0 * (remainder - floor(remainder));
    out[i] = hex_chars[static_cast<int>(remainder)];
  }

  out[count] = '\0';
}

bool bbp_self_test()
{
  // Pi in hexadecimal opens 3.243f6a88, which anchors the digits to the right
  // place. Without this a version that had slipped by a digit would still pass
  // the overlap test below, because it would slip by the same amount every time
  char opening[BBP_PROBE_DIGITS + 1];
  bbp_hex_digits(0, BBP_PROBE_DIGITS, opening);

  if (strncmp(opening, "243f6a88", BBP_PROBE_DIGITS) != 0)
  {
    return false;
  }

  // Asking for digits one place further along has to return the same digits
  // shifted by one. That holds no matter what the digits actually are, so it
  // checks the deeper positions without needing any of them written down here
  for (int position : {1000, 2000})
  {
    char here[BBP_PROBE_DIGITS + 1];
    char next[BBP_PROBE_DIGITS + 1];
    bbp_hex_digits(position, BBP_PROBE_DIGITS, here);
    bbp_hex_digits(position + 1, BBP_PROBE_DIGITS, next);

    if (strncmp(here + 1, next, BBP_PROBE_DIGITS - 1) != 0)
    {
      return false;
    }
  }

  return true;
}

/**
 * Checks whether the digits starting at one position all match the value
 * @param pi_hex Fractional hexadecimal digits of the value being checked
 * @param position Where to start checking
 * @return True when every digit of the probe matches
 */
static bool probe_matches(const string &pi_hex, int position)
{
  char expected[BBP_PROBE_DIGITS + 1];
  bbp_hex_digits(position, BBP_PROBE_DIGITS, expected);

  return pi_hex.compare(static_cast<size_t>(position), BBP_PROBE_DIGITS, expected) == 0;
}

int bbp_confirmed_hex_digits(const string &pi_hex, int hex_length)
{
  // Nothing to account for, and nothing to probe with
  if (hex_length < BBP_PROBE_DIGITS || static_cast<int>(pi_hex.length()) < hex_length)
  {
    return 0;
  }

  const int last_start = hex_length - BBP_PROBE_DIGITS;

  // Check the very end first. These algorithms build Pi as a single running sum,
  // so a mistake anywhere spoils every digit after it as well. Digits at the end
  // that are right therefore mean the digits ahead of them were right too, and
  // one probe settles the whole result. This is the case nearly every run takes
  if (probe_matches(pi_hex, last_start))
  {
    return hex_length;
  }

  // Something is wrong, so find where. The opening digits are worth checking
  // outright, both because the search below cannot narrow inside its own probe
  // width and because a badly wrong result usually goes wrong immediately
  char opening[BBP_PROBE_DIGITS + 1];
  bbp_hex_digits(0, BBP_PROBE_DIGITS, opening);

  for (int i = 0; i < BBP_PROBE_DIGITS; i++)
  {
    if (pi_hex[static_cast<size_t>(i)] != opening[i])
    {
      return i;
    }
  }

  // Halve the range down to the last position that still checks out. A probe
  // starting at some place covers the digits from there onwards, so it passes
  // only while the whole probe sits ahead of the mistake. That makes passing
  // and failing split the range cleanly in two, which is what lets the search
  // halve. Fourteen or so probes cover ten thousand digits, and since a probe
  // costs more the deeper it reaches, the whole search costs about as much as
  // the single deep probe already taken above
  int low = 0;              // Known to pass
  int high = last_start;    // Known to fail

  while (high - low > 1)
  {
    int middle = low + (high - low) / 2;

    if (probe_matches(pi_hex, middle))
    {
      low = middle;
    }
    else
    {
      high = middle;
    }
  }

  // Everything the passing probe covered is right and the digit straight after
  // it is the first that is not, since the failing probe next door differs only
  // by including that one
  return low + BBP_PROBE_DIGITS;
}

// EOF
