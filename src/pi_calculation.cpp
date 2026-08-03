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
#include "input.hpp"
#include <gmpxx.h>
#include <iostream>
#include <cmath>
#include <sys/time.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
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
    progress_step();
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
      progress_step();
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
    progress_step();

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
    progress_step();

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
    progress_step();

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
    progress_step();

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
    progress_step();

    int q = 0;  // `q` will store the quotient for the current step

    // Process each element of array `A` to generate the next digit
    for (int i = len; i > 0; --i)
    {
      // Calculate new value for A[i-1] by shifting and adding the quotient from the previous step
      int x = 10 * A[i - 1] + q * i;
      A[i - 1] = x % (2 * i - 1);  // Store the remainder back in A[i-1]
      q = x / (2 * i - 1);  // Store the quotient to pass on to the next element
    }

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
      for (int k = 0; k < nines; ++k)
      {
        pi += 9 * multiplier;  // Add each 9 to Pi
        multiplier /= ten;  // Move the decimal place for each 9
      }

      predigit = q;  // Set predigit to the current digit
      nines = 0;  // Reset nines count
    }
  }

  progress_end();

  // Final step: Add the last digit and ensure the last one isn't missed
  pi += predigit * multiplier;

  // If there were trailing 9's that were skipped, handle them here
  if (nines > 0)
  {
    for (int k = 0; k < nines; ++k)
    {
      pi += 9 * multiplier;
      multiplier /= ten;
    }
  }

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
    progress_step();

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
 * Announces the chosen method and runs it
 * @param method The method to use for Pi calculation
 * @param precision The number of decimal places for the Pi calculation
 * @param pi Filled in with the calculated value
 * @return The short name for the results screen, or an empty string if the
 *         method number does not name anything
 */
static string run_selected_method(int method, int precision, mpf_class &pi)
{
  // Short name for the results screen, where the line it sits on has to leave room
  // for two digit counts as well. The longest of these is 21 characters, which is
  // what the width budget in compare_pi_accuracy is worked out from
  string method_name;

  // Determine the calculation method based on user selection and calculate Pi
  switch (method)
  {
    case 0:
      cout << "Calculating Pi using Numerical Integration Method..." << endl;
      method_name = "Numerical Integration";
      pi = calculate_pi_numerical_integration();
      break;
    case 1:
      cout << "Calculating Pi using Machin's Formula Method..." << endl;
      method_name = "Machin's Formula";
      pi = calculate_pi_machin(precision);
      break;
    case 2:
      cout << "Calculating Pi using Ramanujan's First Series..." << endl;
      method_name = "Ramanujan's Series";
      pi = calculate_pi_ramanujan(precision);
      break;
    case 3:
      cout << "Calculating Pi using Chudnovsky's Algorithm..." << endl;
      method_name = "Chudnovsky";
      pi = calculate_pi_chudnovsky(precision);
      break;
    case 4:
      cout << "Calculating Pi using Gauss-Legendre Algorithm..." << endl;
      method_name = "Gauss-Legendre";
      pi = calculate_pi_gauss_legendre(precision);
      break;
    case 5:
      cout << "Calculating Pi using Spigot Algorithm..." << endl;
      method_name = "Spigot";
      pi = calculate_pi_spigot(precision);
      break;
    case 6:
      cout << "Calculating Pi using Bailey-Borwein-Plouffe (BBP) formula..." << endl;
      method_name = "BBP";
      pi = calculate_pi_bbp(precision);
      break;
    case 7:
      cout << "Calculating Pi using Borwein's Quartic Algorithm..." << endl;
      method_name = "Borwein Quartic";
      pi = calculate_pi_borwein_quartic(precision);
      break;
    default:
      cout << "Invalid method selection." << endl;
      break;
    }

  return method_name;
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

// Drawing one page is a job of its own, defined below the loop that drives it
static void draw_result_page(const string &pi_full_string,
                             const AccuracyReport &accuracy_info,
                             int current_page, int total_pages,
                             int digits_per_page);

/**
 * Shows the digits a screenful at a time, letting the user page through them
 * @param pi_full_string The full result, "3." and every digit asked for
 * @param precision The number of decimal places the result carries
 * @param accuracy_info The report shown above the digits, which also says where
 *        the first wrong digit is so the rest can be coloured
 */
static void display_pi_pages(const string &pi_full_string, int precision,
                             const AccuracyReport &accuracy_info)
{
  // Rows a page of digits cannot use: up to three lines of accuracy report, the blank
  // line and separator above the digits, the closing rule and blank line below them,
  // the page counter, the scroll hint, the closing prompt, and one row of slack so
  // that the closing newline does not scroll the report off the top of the screen.
  // This counts each of those as a single row, which holds only while none of them
  // wrap. The widest are the closing prompt at 54 characters and the verdict line at
  // 62 once both digit counts reach eight figures, so a console narrower than 62
  // columns would need more rows reserved than this
  const int reserved_rows = 11;

  // A page that carries on to either side shows an ellipsis there, which needs room.
  // The first page spends two of these on the "3." it carries ahead of its digits
  const int ellipsis_room = 6;

  // Ask the console how big it is rather than assuming a size. The three TV modes give
  // three different heights, so any fixed page size is wrong on at least two of them.
  // A page larger than the screen scrolls the accuracy report away before it is read
  int console_cols = 0;
  int console_rows = 0;
  CON_GetMetrics(&console_cols, &console_rows);

  // Fall back to the smallest of the three TV modes if the console reports nothing
  // usable, so that the page size below can never come out as zero or negative
  if (console_cols < 20) { console_cols = 75; }
  if (console_rows <= reserved_rows) { console_rows = 27; }

  // How many digits one screenful holds. Pages are counted in digits rather than
  // characters so that the "3." at the front cannot push the last digit of an
  // otherwise exact page over onto a page of its own
  const int screen_digits = ((console_rows - reserved_rows) * console_cols) - ellipsis_room;

  // Hold pages to a round thousand wherever the screen allows it, so that the page
  // number says which digits are on it: page three starts at digit 2001 whatever the
  // TV mode. Letting the screen decide instead would split ten thousand digits into
  // nine pages on NTSC and eight on PAL, with no page starting on a round number
  const int digits_per_page = screen_digits < 1000 ? screen_digits : 1000;

  int total_pages = (precision + digits_per_page - 1) / digits_per_page;
  if (total_pages == 0) { total_pages = 1; }
  int current_page = 0;
  bool needs_redraw = true;

  while (true)
  {
    // Read both controllers through the shared input module. Reading the Wii Remote
    // directly here would leave the module holding a stale idea of which buttons are
    // down, and the next menu would then mistake a button still being held for a fresh
    // press. It would also leave the GameCube controller unread and therefore dead
    poll_inputs();

    // Turn the page
    if (is_button_just_pressed(PAD_BUTTON_RIGHT, WPAD_BUTTON_RIGHT))
    {
      if (current_page < total_pages - 1)
      {
        current_page++;
        needs_redraw = true;
      }
    }
    if (is_button_just_pressed(PAD_BUTTON_LEFT, WPAD_BUTTON_LEFT))
    {
      if (current_page > 0)
      {
        current_page--;
        needs_redraw = true;
      }
    }

    // Leave the program, matching what the menus do with these same buttons
    if (is_button_just_pressed(PAD_BUTTON_START, WPAD_BUTTON_HOME))
    {
      exit_WPCPP();
    }

    // Go back to the menu
    if (is_button_just_pressed(PAD_BUTTON_A | PAD_BUTTON_B, WPAD_BUTTON_A | WPAD_BUTTON_B))
    {
      break;
    }

    if (needs_redraw)
    {
      draw_result_page(pi_full_string, accuracy_info, current_page, total_pages,
                       digits_per_page);
      needs_redraw = false;
    }

    // Wait for video sync to ensure smooth input handling
    VIDEO_WaitVSync();
  }
}

/**
 * Draws one page of digits, with the accuracy report above and the controls
 * below. Everything on screen is rewritten, so the caller only calls this when
 * something has actually changed
 * @param pi_full_string The full result, "3." and every digit asked for
 * @param accuracy_info The report to print above the digits
 * @param current_page The page to draw, counted from zero
 * @param total_pages How many pages the result comes to
 * @param digits_per_page How many digits a page holds
 */
static void draw_result_page(const string &pi_full_string,
                             const AccuracyReport &accuracy_info,
                             int current_page, int total_pages,
                             int digits_per_page)
{
  cout << "\x1b[2J";

  // Print the Accuracy Report Header
  for (const auto& line : accuracy_info.get_lines())
  {
    cout << line << endl;
  }

  // Print a separator
  cout << endl << "--- Full Result ---" << endl;

  // Prepare the content for the current page
  // A digit sits two characters further along than its own position, since "3."
  // comes first. The opening page takes that prefix with it rather than counting
  // it against its own digit budget
  int start_pos = (current_page * digits_per_page) + 2;
  int page_length = digits_per_page;
  if (current_page == 0)
  {
    start_pos = 0;
    page_length = digits_per_page + 2;
  }

  string page_content_raw = pi_full_string.substr(start_pos, page_length);
  string page_content_full = page_content_raw;
  int mismatch_index = accuracy_info.get_mismatch_index();
  int page_mismatch_pos = mismatch_index - start_pos;

  // Add ellipses for continuation if there are multiple pages
  if (total_pages > 1)
  {
    if (current_page > 0)
    {
      page_content_full.insert(0, "...");
      if (mismatch_index != -1) { page_mismatch_pos += 3; }
    }
    if (current_page < total_pages - 1)
    {
      page_content_full += "...";
    }
  }

  const string red = "\x1b[31m";
  const string reset_color = "\x1b[37m";

  // Print the paginated body with color coding
  if (mismatch_index == -1)
  {
    cout << page_content_full << endl;
  }
  else
  {
    // Mismatch occurred before the start of this page's raw content
    if (mismatch_index < start_pos)
    {
      cout << red << page_content_full << reset_color << endl;
    }
    // Mismatch occurs after this page's raw content
    else if (mismatch_index >= start_pos + (int)page_content_raw.length())
    {
      cout << page_content_full << endl;
    }
    // Mismatch is on this page
    else
    {
      string correct_part = page_content_full.substr(0, page_mismatch_pos);
      string incorrect_part = page_content_full.substr(page_mismatch_pos);
      cout << correct_part << red << incorrect_part << reset_color << endl;
    }
  }

  // Close the digits off with a rule as wide as the header above them, then a
  // blank line, so the result does not run straight into the controls. Plain
  // dashes rather than an "end of result" marker, since on any page but the
  // last one the result carries on
  cout << "-------------------" << endl << endl;

  // Print the Footer
  if (total_pages > 1)
  {
    cout << "Page " << (current_page + 1) << " of " << total_pages << endl;
    cout << "Use D-Pad Left/Right to scroll." << endl;
  }

  cout << "Press A/B to return to menu. Press Home/Start to exit." << endl;
}

/**
 * Times the Pi calculation and displays a detailed, paginated report.
 * @param method The method to use for Pi calculation.
 * @param precision The number of decimal places for the Pi calculation.
 */
void calculate_and_display_pi(int method, int precision)
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
    return;  // Nothing was calculated, so there is nothing to report
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
  while (true)
  {
    poll_inputs();
    if (is_button_just_pressed(0xFFFFFFFF, 0xFFFFFFFF))
    {
      break;
    }
    VIDEO_WaitVSync();
  }

  display_pi_pages(format_pi(pi, precision), precision, accuracy_info);
}

// EOF
