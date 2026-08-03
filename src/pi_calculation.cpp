// src/pi_calculation.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Wii Pi Calculator Project Plus (WPCPP)
// Copyright (C) 2024-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "pi_calculation.hpp"
#include "utility.hpp"
#include "results.hpp"
#include "input.hpp"
#include <gmpxx.h>
#include <iostream>
#include <cmath>
#include <sys/time.h>
#include <gccore.h>
#include <vector>
#include <string>

using namespace std;  // Use the entire std namespace for simplicity

/**
 * Computes the arctangent using a Taylor series approximation
 * This function is crucial for the Machin's formula calculation of Pi
 * @param x The value to compute arctangent for
 * @param precision Number of decimal places the caller needs in the finished result
 * @return The computed arctangent of x
 */
// Places arctan works to beyond the ones asked for. Each term is far smaller than
// the one before it, so everything left when the terms fall below this is too small
// to reach the digits on display. The spare places hold the leftovers away from the
// last digit shown, which matters because Machin's formula scales the result up by
// sixteen. The count below needs this same figure, so it lives out here
static const int arctan_spare_digits = 10;

/**
 * Works out roughly how many terms arctan will need for a given value
 * @param x The value arctan will be given
 * @param precision Number of decimal places the caller needs in the finished result
 * @return The expected number of terms, used to size the progress line
 */
static int arctan_term_count(double x, int precision)
{
  // Every term is smaller than the one before it by a factor of x squared, so the
  // places gained per term is twice the log of one over x. For a fifth that is
  // about one and four tenths places a term, and for a two hundred and thirty
  // ninth it is nearer five
  const double places_per_term = 2.0 * log10(1.0 / x);
  return static_cast<int>((precision + arctan_spare_digits) / places_per_term) + 1;
}

mpf_class arctan(const mpf_class &x, int precision)
{
  mpf_class result = 0.0;  // The result of the arctangent calculation
  mpf_class term = x;  // The first term in the series is x
  mpf_class x2 = x * x;  // Precompute x^2 to avoid repetitive multiplication
  int n = 1;  // The first term uses n = 1

  mpf_class threshold("1e-" + to_string(precision + arctan_spare_digits));  // Stopping point for the loop below

  // Loop while the absolute value of the term is greater than the threshold
  while (term > threshold || term < -threshold)  // Equivalent to abs(term) > threshold
  {
    result += term;  // Add the current term to the result
    n += 2;  // Increase n by 2 (since the series uses odd numbers)
    term *= -x2 * (n - 2) / n;  // Compute the next term efficiently without recalculating powers
    if (!progress_step()) { break; }
  }

  return result;  // Return the final result of the arctangent
}

/**
 * Computes the factorial of a given integer using GMP for arbitrary precision
 * This function calculates the factorial (n!) of the integer n.
 * @param n The integer for which to compute the factorial
 * @return The factorial of n as an arbitrary precision GMP value
 */
mpf_class gmp_factorial(int n)
{
  // Build the factorial as a whole number rather than a floating point one. GMP has a
  // routine built for this, and it beats multiplying up one step at a time. A whole
  // number also carries every digit, so nothing is rounded away while it is being built
  mpz_class exact = 1;  // 0! and 1! are both 1, which is what n of zero or less returns

  if (n > 0)
  {
    mpz_fac_ui(exact.get_mpz_t(), static_cast<unsigned long>(n));
  }

  // Convert to floating point at the very end, so rounding happens once instead of n times
  mpf_class result;
  mpf_set_z(result.get_mpf_t(), exact.get_mpz_t());

  // Return the final result, which is n!
  return result;
}

/**
 * Calculates Pi using Machin's formula which approximates Pi using arctangents
 * @param precision Number of decimal places to calculate
 * @return The calculated value of Pi using Machin's formula
 */
mpf_class calculate_pi_machin(int precision)
{
  // One run of the progress line across both arctans, since counting up to the end
  // twice over would look as though the work had started again from scratch
  progress_begin(arctan_term_count(1.0 / 5.0, precision)
    + arctan_term_count(1.0 / 239.0, precision), "term");

  // Machin's formula: Pi = 16 * arctan(1/5) - 4 * arctan(1/239)
  mpf_class result = 16 * arctan(mpf_class(1) / mpf_class(5), precision)
    - 4 * arctan(mpf_class(1) / mpf_class(239), precision);

  progress_end();
  return result;
}

/**
 * Calculates Pi using numerical integration based on the rectangle rule (Riemann sum),
 * optimized with double precision inside the loop for speed, while periodically converting
 * results to GMP for enhanced precision. This method approximates Pi by summing small
 * areas under the curve and multiplying by 4. Although GMP can handle high precision,
 * the accuracy of this method is limited by numerical integration's inherent approximation
 * errors, which can accumulate. The accuracy typically reaches about 15-17 decimal places
 * depending on the chosen values for 'a', 'dx', and 'batch_size', representing a trade-off
 * between performance and accuracy.
 * @return The calculated value of Pi using numerical integration
 */
mpf_class calculate_pi_numerical_integration()
{
  double a = 27500000.0;  // Large constant for accuracy
  double a2 = a * a;  // Precompute a^2 for efficiency
  double dx = 1.00;  // Initial small step size for integration

  const int batch_size = 10000;  // Number of iterations per batch
  mpf_class sum_gmp = 0.0;  // GMP accumulator for final precise result
  double batch_sum = 0.0;  // Temporary double accumulator for each batch

  int batch_count = 0;  // Counter to track iterations in the current batch

  // Report a batch at a time rather than a step at a time. The loop below runs
  // over twenty seven million times, so checking the clock inside it would cost
  // more than the sum it is there to work out
  progress_begin(static_cast<int>((a - dx) / dx) / batch_size, "step");

  // Loop through intervals for area approximation with adaptive step size
  for (double x = dx; x <= a - dx; x += dx)
  {
    double x2 = x * x;  // Compute x^2
    batch_sum += (1.0 / (a2 + x2)) * dx;  // Accumulate area

    // Every batch_size iterations, convert the batch_sum to GMP and reset batch_sum
    if (++batch_count == batch_size)
    {
      sum_gmp += batch_sum;  // Accumulate in GMP
      batch_sum = 0.0;  // Reset batch sum for the next batch
      batch_count = 0;  // Reset counter
      if (!progress_step()) { break; }
    }
  }

  progress_end();

  // Add any remaining sum from the last batch, if present
  if (batch_sum != 0.0)
  {
    sum_gmp += batch_sum;
  }

  // Approximate the remaining area using the midpoint correction and add to GMP
  mpf_class remaining = (mpf_class(1.0) / a2 + mpf_class(1.0) / (2 * a2)) / 2.0 * dx;
  sum_gmp += remaining;

  // Multiply by 4 and 'a' to approximate Pi using GMP precision
  mpf_class a_gmp = a;
  return 4.0 * sum_gmp * a_gmp;
}

/**
 * Calculates Pi using Ramanujan's first series
 * Ramanujan's series is known for its rapid convergence to Pi, making it highly efficient
 * @param precision The number of decimal places of Pi to calculate
 * @return The calculated value of Pi using Ramanujan's series
 */
mpf_class calculate_pi_ramanujan(int precision)
{
  mpf_class sum = 0.0;  // Initialize the sum to accumulate series terms
  mpf_class factor = 2 * sqrt(mpf_class(2)) / 9801;  // Precompute the constant factor in Ramanujan's formula

  // Each term of this series is worth log10(396^4 / 256) decimal places, which
  // works out at about 7.982 rather than the 8 this used to assume. The shortfall
  // per term is tiny but it adds up in step with the precision, so it ate through
  // the two spare terms once around seven thousand places were asked for and the
  // answer came up short of what was requested. Dividing by a little less than the
  // true figure leaves a margin that grows with the precision instead of shrinking
  int iterations = static_cast<int>(precision / 7.9) + 2;  // Number of iterations controls the precision of the result (precision vs. performance)

  progress_begin(iterations, "term");

  // Loop through each term in the series expansion
  for (int k = 0; k < iterations; ++k)
  {
    if (!progress_step()) { break; }

    // Calculate the numerator: (4k)! * (1103 + 26390k)
    mpf_class numerator = gmp_factorial(4 * k) * (1103 + 26390 * k);

    // Calculate the denominator, which is composed of two parts: (k!)^4 and (396)^(4 * k)
    mpf_class denominator = gmp_factorial(k);  // Start with k!

    mpf_class temp;  // Temporary variable for storing intermediate results

    // Raise (k!) to the power of 4 for the denominator
    mpf_pow_ui(temp.get_mpf_t(), denominator.get_mpf_t(), 4);  // Compute (k!)^4
    denominator = temp;  // Update denominator with (k!)^4

    // Raise 396 to the power of (4 * k) and multiply with the denominator
    mpf_class base396 = mpf_class(396);  // Set the base 396
    mpf_pow_ui(temp.get_mpf_t(), base396.get_mpf_t(), 4 * k);  // Compute (396)^(4 * k)
    denominator *= temp;  // Multiply denominator by (396)^(4 * k)

    // Add the current term (numerator / denominator) to the sum
    sum += numerator / denominator;
  }

  progress_end();

  // Final step: Pi is calculated as 1 / (factor * sum)
  return 1 / (factor * sum);
}

/**
 * Calculates Pi using the Chudnovsky algorithm
 * The Chudnovsky algorithm is extremely efficient for calculating Pi with high precision
 * @param precision The number of decimal places of Pi to calculate
 * @return The calculated value of Pi using the Chudnovsky algorithm
 */
mpf_class calculate_pi_chudnovsky(int precision)
{
  // Constant term in the Chudnovsky formula: C = 426880 * sqrt(10005)
  mpf_class C = 426880 * sqrt(mpf_class(10005));

  mpf_class sum = 0;  // Initialize the sum to accumulate series terms

  int iterations = (precision / 14) + 2;  // Number of iterations controls the precision of the result (precision vs. performance)

  progress_begin(iterations, "term");

  // Loop through each term in the series expansion
  for (int k = 0; k < iterations; ++k)
  {
    if (!progress_step()) { break; }

    // Calculate the numerator: (6k)! * (13591409 + 545140134k)
    // Work the multiplier out as a whole number rather than in plain ints. Once k
    // reaches four, 545140134 times k passes the largest value an int can hold.
    // Going past that limit has no defined meaning in C++, so the compiler is free
    // to do as it likes, and it did: on a 64 bit machine the full value survived in
    // a register and the answers looked right, while on the Wii it wrapped round to
    // a negative number and the series fell apart after a handful of terms
    mpz_class multiplier = 545140134;
    multiplier = multiplier * k + 13591409;

    mpf_class numerator;
    mpf_set_z(numerator.get_mpf_t(), multiplier.get_mpz_t());
    numerator *= gmp_factorial(6 * k);

    // Calculate the denominator, composed of three parts: (3k)!, (k!)^3, and (640320)^(3 * k)
    // First, compute (640320)^(3 * k)
    mpf_class power_neg640320;
    mpf_pow_ui(power_neg640320.get_mpf_t(), mpf_class(640320).get_mpf_t(), 3 * k);  // Compute (640320)^(3 * k)

    // Second, compute (k!)^3
    mpf_class factorial_k_cubed = gmp_factorial(k) * gmp_factorial(k) * gmp_factorial(k);  // Compute (k!)^3

    // Third, compute the full denominator: (3k)! * (k!)^3 * (640320)^(3 * k)
    mpf_class denominator = gmp_factorial(3 * k) * factorial_k_cubed * power_neg640320;

    // Alternate signs: add for even k, subtract for odd k
    if (k % 2 == 0)
    {
      sum += numerator / denominator;  // Add the current term to the sum
    }
    else
    {
      sum -= numerator / denominator;  // Subtract the current term from the sum
    }
  }

  progress_end();

  // Final step: Pi is calculated as C / sum
  return C / sum;
}

/**
 * Calculates Pi using the Gauss-Legendre algorithm
 * This algorithm iteratively refines estimates of Pi, converging rapidly
 * @param precision The number of decimal places of Pi to calculate
 * @return The calculated value of Pi using the Gauss-Legendre algorithm
 */
mpf_class calculate_pi_gauss_legendre(int precision)
{
  // Initialize values for the algorithm
  mpf_class a = 1;  // Initial value of a
  mpf_class b = 1 / sqrt(mpf_class(2));  // Initial value of b
  mpf_class t = 0.25;  // Initial value of t
  mpf_class p = 1;  // Initial value of p, representing powers of 2

  int iterations = static_cast<int>(ceil(log2(precision))) + 2;  // Number of iterations controls the precision of the result (precision vs. performance)

  progress_begin(iterations, "pass");

  // Loop through the iterative process to refine a, b, t, and p
  for (int i = 0; i < iterations; ++i)
  {
    if (!progress_step()) { break; }

    // Calculate the next value of a as the average of a and b
    mpf_class a_next = (a + b) / 2;

    // Calculate the next value of b as the square root of the product of a and b
    mpf_class b_next = sqrt(a * b);

    // Calculate the next value of t based on the difference between a and a_next
    mpf_class t_next = t - p * (a - a_next) * (a - a_next);

    // Double the value of p for the next iteration
    p *= 2;

    // Update a, b, and t for the next iteration
    a = a_next;
    b = b_next;
    t = t_next;
  }

  progress_end();

  // Final step: Pi is calculated as (a + b)^2 / (4 * t)
  return (a + b) * (a + b) / (4 * t);
}

/**
 * Calculates Pi using Borwein's quartic algorithm
 * Each pass roughly quadruples the number of correct digits, where the
 * Gauss-Legendre algorithm above only doubles them, so this reaches a given
 * precision in about half as many passes
 * @param precision The number of decimal places of Pi to calculate
 * @return The calculated value of Pi using Borwein's quartic algorithm
 */
mpf_class calculate_pi_borwein_quartic(int precision)
{
  // Starting values for the algorithm
  mpf_class a = mpf_class(6) - 4 * sqrt(mpf_class(2));
  mpf_class y = sqrt(mpf_class(2)) - 1;

  // Correct digits roughly quadruple each pass, so the count grows with the
  // logarithm of the precision to base four. One spare pass covers the fact
  // that the first pass starts from a single correct digit rather than four.
  // Spare passes are not cheap here, since each one costs two square roots at
  // the full working precision
  int iterations = static_cast<int>(ceil(log2(precision) / 2.0)) + 1;
  if (iterations < 2)
  {
    iterations = 2;  // The smallest precisions still need a pass to refine
  }

  progress_begin(iterations, "pass");

  for (int i = 0; i < iterations; ++i)
  {
    if (!progress_step()) { break; }

    mpf_class y_squared = y * y;
    mpf_class y_fourth = y_squared * y_squared;

    // The fourth root of 1 - y^4, taken as two square roots. This is the step
    // the algorithm is named for. Taking a single square root here still looks
    // plausible and still converges, just to the wrong number
    mpf_class root = sqrt(1 - y_fourth);
    root = sqrt(root);

    mpf_class y_next = (1 - root) / (1 + root);

    mpf_class one_plus_y = 1 + y_next;
    mpf_class one_plus_y_pow_4;
    mpf_pow_ui(one_plus_y_pow_4.get_mpf_t(), one_plus_y.get_mpf_t(), 4);

    // The exponent is 2i + 3, so the first pass uses 8. Starting it at 4
    // instead leaves the result wrong by an amount no extra precision fixes
    mpf_class two_power;
    mpf_pow_ui(two_power.get_mpf_t(), mpf_class(2).get_mpf_t(),
               (2UL * static_cast<unsigned long>(i)) + 3);

    mpf_class correction = two_power * y_next * (1 + y_next + (y_next * y_next));

    a = (a * one_plus_y_pow_4) - correction;
    y = y_next;
  }

  progress_end();

  // Final step: Pi is the reciprocal of a
  return 1 / a;
}

/**
 * Advances the working array one place and hands back what falls off the top
 * @param A The working array, updated in place
 * @param len How many places the array holds
 * @return The quotient carried out, which the next digit is drawn from
 */
static int spigot_next_quotient(std::vector<int> &A, int len)
{
  int q = 0;  // `q` will store the quotient for the current step

  for (int i = len; i > 0; --i)
  {
    // Calculate new value for A[i-1] by shifting and adding the quotient from the previous step
    int x = 10 * A[i - 1] + q * i;
    A[i - 1] = x % (2 * i - 1);  // Store the remainder back in A[i-1]
    q = x / (2 * i - 1);  // Store the quotient to pass on to the next element
  }

  return q;
}

/**
 * Writes out a run of nines that was held back to see whether a carry would
 * turn them all to zeros, and moves the place value along past them
 * @param pi The value being built up
 * @param multiplier Place value of the next digit, moved along once per nine
 * @param ten Ten at the working precision, which the place value divides by
 * @param nines How many nines were held back, which may be none
 */
static void add_pending_nines(mpf_class &pi, mpf_class &multiplier,
                              const mpf_class &ten, int nines)
{
  for (int k = 0; k < nines; ++k)
  {
    pi += 9 * multiplier;  // Add each 9 to Pi
    multiplier /= ten;  // Move the decimal place for each 9
  }
}

/**
 * Calculates Pi using the Spigot algorithm
 * The Spigot algorithm calculates Pi one digit at a time using a specific sequence of operations,
 * and it is known for its ability to output the digits of Pi without needing high memory or large precision for intermediate results
 * @param precision The number of decimal places of Pi to calculate
 * @return The calculated value of Pi using the Spigot algorithm
 */
mpf_class calculate_pi_spigot(int precision)
{
  // Calculate several digits beyond the ones asked for. A digit that has already been
  // produced can still be changed by a carry arriving from a later position. Positions
  // that are never computed can never send that carry, so the last few digits of any
  // spigot run are unreliable. The extra digits take that damage and are then discarded
  const int N = precision + 10;  // Set the number of digits of Pi we want to calculate based on the precision parameter
  int len = static_cast<int>(floor(10 * N / 3) + 1);  // Calculate array size based on the number of digits to process

  // Initialize the array 'A' to store intermediate values, starting with 2's
  std::vector<int> A(len, 2);

  // Track how many 9's and pre-digits occur for rounding
  int nines = 0;
  int predigit = 0;

  mpf_class pi = 0.0;  // `pi` will store the accumulated value of Pi as we calculate it
  mpf_class ten = 10.0;  // We use this constant to handle decimal places

  // Digits are emitted one step behind the loop, so the very first value written out is
  // the initial `predigit` placeholder rather than a real digit of Pi. That placeholder
  // occupies the tens column, which puts the leading 3 in the units column after it
  mpf_class multiplier = 10.0;  // Place value of the next digit to be emitted

  // This loop works out one digit of Pi per turn, so the count on the progress
  // line is not a stand in for progress, it is the digit being worked on
  progress_begin(N, "digit");

  // Loop through each digit position to calculate the digits of Pi
  for (int j = 1; j <= N; ++j)
  {
    if (!progress_step()) { break; }

    int q = spigot_next_quotient(A, len);

    A[0] = q % 10;  // Extract the first digit of the new quotient
    q = q / 10;  // Prepare for the next step by shifting `q`

    // Handle rounding and carry depending on the value of `q`
    if (q == 9)  // If `q` is 9, we might need to round up later
    {
      ++nines;  // Count how many 9's we have in a row
    }
    else if (q == 10)  // If `q` is 10, we need to round up and correct earlier digits
    {
      // Add 1 to the previous digit and round all the stored 9's to zeros
      pi += (predigit + 1) * multiplier;  // Adjust Pi with the corrected digit
      multiplier /= ten;  // Move the decimal place to the next position

      // Set any earlier 9's to zero in Pi. The carry has already turned each of them
      // into a 0, so nothing is added here and only the place value moves along
      for (int k = 0; k < nines; ++k)
      {
        multiplier /= ten;  // Move the decimal place to the next position
      }

      predigit = 0;  // Reset predigit
      nines = 0;  // Reset the count of consecutive 9's
    }
    else
    {
      // Store the current digit in Pi and handle any earlier 9's
      pi += predigit * multiplier;  // Add the predigit to Pi
      multiplier /= ten;  // Move the decimal place to the next position

      // Handle rounding if there were any earlier 9's
      add_pending_nines(pi, multiplier, ten, nines);

      predigit = q;  // Set predigit to the current digit
      nines = 0;  // Reset nines count
    }
  }

  progress_end();

  // Final step: Add the last digit and ensure the last one isn't missed
  pi += predigit * multiplier;

  // If there were trailing 9's that were skipped, handle them here
  add_pending_nines(pi, multiplier, ten, nines);

  return pi;  // Return the calculated value of Pi
}

/**
 * Calculates Pi using the Bailey-Borwein-Plouffe (BBP) formula
 * The BBP formula is a series that rapidly converges to Pi, allowing it to calculate Pi to many decimal places quickly
 * It is one of the fastest algorithms for calculating Pi and can be used to directly calculate the nth digit of Pi in hexadecimal
 * @param precision The number of decimal places of Pi to calculate
 * @return The calculated value of Pi using the BBP formula
 */
mpf_class calculate_pi_bbp(int precision)
{
  mpf_class pi = 0.0;  // Initialize the result `pi` to store the value of Pi as it is calculated
  mpf_class scaled;  // The current term after it has been scaled down by 16^k
  int iterations = static_cast<int>(precision / 1.2) + 2;  // Number of iterations (terms) to calculate. More terms yield higher precision

  progress_begin(iterations, "term");

  // Loop through each term in the BBP series to accumulate the value of Pi
  for (int k = 0; k < iterations; ++k)
  {
    if (!progress_step()) { break; }

    // Compute the current term of the BBP series.
    mpf_class term = (mpf_class(4) / (8 * k + 1))  // The first part of the BBP term
                   - (mpf_class(2) / (8 * k + 4))  // The second part
                   - (mpf_class(1) / (8 * k + 5))  // The third part
                   - (mpf_class(1) / (8 * k + 6));  // The fourth part

    // Divide the term by 16^k. Sixteen is two to the fourth, so dividing by 16^k
    // means shifting the binary point along by 4k places and nothing else. GMP
    // stores the exponent separately from the digits, so this only adjusts the
    // exponent and leaves the digits untouched. Building 16^k as a number and
    // dividing by it instead would cost a pile of full length multiplications per
    // term, and would be less accurate as well: 16^k needs 4k bits to write down
    // exactly, which outgrows the working precision partway through the series,
    // so every later term would be divided by a slightly rounded number
    mpf_div_2exp(scaled.get_mpf_t(), term.get_mpf_t(),
                 4UL * static_cast<mp_bitcnt_t>(k));

    pi += scaled;  // Add the scaled term to the running total
  }

  progress_end();
  return pi;  // Return the calculated value of Pi
}

/**
 * Runs numerical integration through the same signature as the others, which
 * work to a precision it has no use for
 * @return The calculated value of Pi
 */
static mpf_class numerical_integration_ignoring_precision(int)
{
  return calculate_pi_numerical_integration();
}

// Every method, in menu order. The three names it goes by:
//   menu_name     what the selection menu offers
//   announcement  what goes on screen while the work runs
//   short_name    what the results screen shows, on a line that also has to hold
//                 two digit counts. The longest of these is 21 characters, which
//                 is what the width budget in compare_pi_accuracy is worked from
// They read close enough to look interchangeable but they are not, so leave the
// wording of each alone unless the screen it belongs to is being changed
static const struct
{
  const char *menu_name;
  const char *announcement;
  const char *short_name;
  mpf_class (*calculate)(int precision);
}
methods[] =
{
  { "Numerical Integration",                "Numerical Integration Method",         "Numerical Integration", numerical_integration_ignoring_precision },
  { "Machin's Formula",                     "Machin's Formula Method",              "Machin's Formula",      calculate_pi_machin },
  { "Ramanujan's First Series",             "Ramanujan's First Series",             "Ramanujan's Series",    calculate_pi_ramanujan },
  { "Chudnovsky Algorithm",                 "Chudnovsky's Algorithm",               "Chudnovsky",            calculate_pi_chudnovsky },
  { "Gauss-Legendre Algorithm",             "Gauss-Legendre Algorithm",             "Gauss-Legendre",        calculate_pi_gauss_legendre },
  { "Spigot Algorithm",                     "Spigot Algorithm",                     "Spigot",                calculate_pi_spigot },
  { "Bailey-Borwein-Plouffe (BBP) Formula", "Bailey-Borwein-Plouffe (BBP) formula", "BBP",                   calculate_pi_bbp },
  { "Borwein's Quartic Algorithm",          "Borwein's Quartic Algorithm",          "Borwein Quartic",       calculate_pi_borwein_quartic }
};

/**
 * How many calculation methods there are
 * @return The number of methods the menu can offer
 */
int pi_method_count()
{
  return static_cast<int>(sizeof(methods) / sizeof(methods[0]));
}

/**
 * The name the selection menu shows for a method
 * @param method The method number, counted from zero
 * @return The menu name, or an empty string if the number names nothing
 */
const char *pi_method_menu_name(int method)
{
  if (method < 0 || method >= pi_method_count())
  {
    return "";
  }

  return methods[method].menu_name;
}

/**
 * Announces the chosen method and runs it
 * @param method The method to use for Pi calculation
 * @param precision The number of decimal places for the Pi calculation
 * @param pi Filled in with the calculated value
 * @return The short name for the results screen, or an empty string if the
 *         method number does not name anything
 */
static string run_selected_method(int method, int precision, mpf_class &pi)
{
  if (method < 0 || method >= pi_method_count())
  {
    cout << "Invalid method selection." << endl;
    return "";
  }

  cout << "Calculating Pi using " << methods[method].announcement << "..." << endl;
  cout << "Press 'B' to cancel." << endl;
  pi = methods[method].calculate(precision);

  return methods[method].short_name;
}

/**
 * Works Pi out a second time by an unrelated route and compares, for the one
 * method the usual check cannot vouch for on its own
 * @param pi The value to check
 * @param precision The number of decimal places that were asked for
 */
static void cross_check_against_gauss_legendre(const mpf_class &pi, int precision)
{
  // The BBP method and the check that follows it are the same formula worked two
  // different ways. Agreement between them shows the arithmetic was carried out
  // properly, but it could not catch the formula itself being written down
  // wrongly, since the same mistake would sit on both sides. For that one method
  // only, work Pi out again by a route with no formula in common and compare.
  // Gauss-Legendre is hundreds of times quicker than the method it is checking
  // here, so this costs almost nothing
  mpf_class independent = calculate_pi_gauss_legendre(precision);
  mpf_class difference = pi - independent;

  if (difference < 0)
  {
    difference = -difference;
  }

  if (difference < mpf_class("1e-" + to_string(precision)))
  {
    cout << "Cross-check against Gauss-Legendre: PASS" << endl;
  }
  else
  {
    cout << "Cross-check against Gauss-Legendre: FAIL" << endl;
  }
}

/**
 * Waits for any button at all, so that what is already on screen can be read
 * before the next thing takes it over. Every button counts here rather than a
 * chosen one, since there is nothing to choose between at this point
 */
static void wait_for_any_button()
{
  while (true)
  {
    poll_inputs();

    if (is_button_just_pressed(0xFFFFFFFF, 0xFFFFFFFF))
    {
      return;
    }

    VIDEO_WaitVSync();
  }
}

/**
 * Times the Pi calculation and displays a detailed, paginated report.
 * @param method The method to use for Pi calculation.
 * @param precision The number of decimal places for the Pi calculation.
 * @return True when the user ended up going back a screen, either by cancelling
 *         the calculation or by leaving the results with B. False when they are
 *         finished with this method altogether
 */
bool calculate_and_display_pi(int method, int precision)
{
  // Clear the screen before displaying the results
  cout << "\x1b[2J";  // ANSI escape code to clear the screen

  // Display the selected precision level
  cout << "Precision level set to: " << precision << " decimal place(s)" << endl;

  struct timeval start_time;
  struct timeval end_time;  // To measure elapsed time
  mpf_class pi;  // Variable to hold the calculated value of Pi

  // Start the timer to measure calculation duration
  gettimeofday(&start_time, nullptr);

  const string method_name = run_selected_method(method, precision, pi);
  if (method_name.empty())
  {
    return false;  // Nothing was calculated, so there is nothing to report
  }

  // A cancelled calculation leaves a partial figure that is not an answer to
  // anything, so it is dropped without being timed, checked or shown
  if (progress_cancelled())
  {
    return true;
  }

  // Stop the timer now that calculation is complete
  gettimeofday(&end_time, nullptr);

  // Calculate the elapsed time in milliseconds
  double time_taken = (end_time.tv_sec - start_time.tv_sec) * 1000.0 + (end_time.tv_usec - start_time.tv_usec) / 1000.0;

  // Indicate that the Pi calculation has completed
  cout << "\nPi Calculation Complete!" << endl;

  // Handle unrealistic time values (negative or zero), which may occur in emulation
  if (time_taken <= 0)
  {
    cout << "Time taken: unknown (possibly due to emulation)" << endl;
  }
  else
  {
    cout << "Time taken: " << format_duration(time_taken) << endl;
  }

  // Check the answer before it is shown. The check works the digits out a second
  // time by a route with nothing in common with the method above, so it can tell
  // a right answer from a wrong one without any digits of Pi being kept in the
  // program to compare against
  cout << "\nVerifying against BBP..." << endl;
  AccuracyReport accuracy_info = compare_pi_accuracy(pi, precision, method_name);
  cout << accuracy_info.get_summary() << endl;

  // The BBP method checks itself against its own formula, so it gets a second
  // opinion from a method with nothing in common with it
  if (method == 6)
  {
    cross_check_against_gauss_legendre(pi, precision);
  }

  // Numerical integration works to a fixed step size rather than to the number
  // of places asked for, so it runs out of accuracy around fifteen decimal
  // places however many are requested. Hitting that wall is the method working
  // as designed, so say so rather than leave it reading like a fault
  if (method == 0 && accuracy_info.get_mismatch_index() != -1)
  {
    cout << "This method is limited to about 15 decimal place(s) by design." << endl;
  }

  // Wait for user to press a button before showing the detailed results
  cout << "\nPress any button to view results..." << endl;
  wait_for_any_button();

  return display_pi_pages(format_pi(pi, precision), precision, accuracy_info);
}

// EOF
