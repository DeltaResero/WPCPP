// pi_calculation.cpp
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

#include "pi_calculation.hpp"
#include "utility.hpp"
#include <gmpxx.h>
#include <iostream>
#include <cmath>
#include <sys/time.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

using namespace std;  // Use the entire std namespace for simplicity

/**
 * Computes the arctangent using a Taylor series approximation
 * This function is crucial for the Machin's formula calculation of Pi
 * @param x The value to compute arctangent for
 * @return The computed arctangent of x
 */
mpf_class arctan(const mpf_class &x)
{
  mpf_class result = 0.0;  // The result of the arctangent calculation
  mpf_class term = x;  // The first term in the series is x
  mpf_class x2 = x * x;  // Precompute x^2 to avoid repetitive multiplication
  int n = 1;  // The first term uses n = 1

  // NOTE: In the future, threshold should not be hardcoded
  // Threshold for stopping the iteration (precision set to 1e-50)
  mpf_class threshold("1e-50");  // Controls precision vs. performance: adjust this value to change the trade-off

  // Loop while the absolute value of the term is greater than the threshold
  while (term > threshold || term < -threshold)  // Equivalent to abs(term) > threshold
  {
    result += term;  // Add the current term to the result
    n += 2;  // Increase n by 2 (since the series uses odd numbers)
    term *= -x2 * (n - 2) / n;  // Compute the next term efficiently without recalculating powers
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
  mpf_class result = 1;  // Initialize result to 1 (as 0! = 1 and 1! = 1)

  // Loop to multiply result by each integer from 1 to n
  for (int i = 1; i <= n; ++i)
  {
    result *= i;  // Multiply result by the current value of i
  }

  // Return the final result, which is n!
  return result;
}

/**
 * Calculates Pi using Machin's formula which approximates Pi using arctangents
 * @return The calculated value of Pi using Machin's formula
 */
mpf_class calculate_pi_machin()
{
  // Machin's formula: Pi = 16 * arctan(1/5) - 4 * arctan(1/239)
  return 16 * arctan(mpf_class(1) / mpf_class(5)) - 4 * arctan(mpf_class(1) / mpf_class(239));
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
    }
  }

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
 * @return The calculated value of Pi using Ramanujan's series
 */
mpf_class calculate_pi_ramanujan()
{
  mpf_class sum = 0.0;  // Initialize the sum to accumulate series terms
  mpf_class factor = 2 * sqrt(mpf_class(2)) / 9801;  // Precompute the constant factor in Ramanujan's formula

  // NOTE: In the future iterations should not be hardcoded
  int iterations = 8;  // Number of iterations controls the precision of the result (precision vs. performance)

  // Loop through each term in the series expansion
  for (int k = 0; k < iterations; ++k)
  {
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

  // Final step: Pi is calculated as 1 / (factor * sum)
  return 1 / (factor * sum);
}

/**
 * Calculates Pi using the Chudnovsky algorithm
 * The Chudnovsky algorithm is extremely efficient for calculating Pi with high precision
 * @return The calculated value of Pi using the Chudnovsky algorithm
 */
mpf_class calculate_pi_chudnovsky()
{
  // Constant term in the Chudnovsky formula: C = 426880 * sqrt(10005)
  mpf_class C = 426880 * sqrt(mpf_class(10005));

  mpf_class sum = 0;  // Initialize the sum to accumulate series terms

  // NOTE: In the future iterations should not be hardcoded
  int iterations = 4;  // Number of iterations controls the precision of the result (precision vs. performance)

  // Loop through each term in the series expansion
  for (int k = 0; k < iterations; ++k)
  {
    // Calculate the numerator: (6k)! * (13591409 + 545140134k)
    mpf_class numerator = gmp_factorial(6 * k) * (13591409 + 545140134 * k);

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

  // Final step: Pi is calculated as C / sum
  return C / sum;
}

/**
 * Calculates Pi using the Gauss-Legendre algorithm
 * This algorithm iteratively refines estimates of Pi, converging rapidly
 * @return The calculated value of Pi using the Gauss-Legendre algorithm
 */
mpf_class calculate_pi_gauss_legendre()
{
  // Initialize values for the algorithm
  mpf_class a = 1;  // Initial value of a
  mpf_class b = 1 / sqrt(mpf_class(2));  // Initial value of b
  mpf_class t = 0.25;  // Initial value of t
  mpf_class p = 1;  // Initial value of p, representing powers of 2

  // NOTE: In the future iterations should not be hardcoded
  int iterations = 5;  // Number of iterations controls the precision of the result (precision vs. performance)

  // Loop through the iterative process to refine a, b, t, and p
  for (int i = 0; i < iterations; ++i)
  {
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

  // Final step: Pi is calculated as (a + b)^2 / (4 * t)
  return (a + b) * (a + b) / (4 * t);
}

/**
 * Times the Pi calculation and prints both the calculated Pi and the time taken
 * This function measures the time for Pi calculation and compares it to the known value of Pi
 * @param method The method to use for Pi calculation
 * @param precision The number of decimal places for the Pi calculation
 */
void calculate_and_display_pi(int method, int precision)
{
  // Clear the screen before displaying the results
  cout << "\x1b[2J";  // ANSI escape code to clear the screen

  // Display the selected precision level
  cout << "Precision level set to: " << precision << " decimal place(s)" << endl;

  struct timeval start_time, end_time;  // To measure elapsed time
  mpf_class pi;  // Variable to hold the calculated value of Pi

  // Start the timer to measure calculation duration
  gettimeofday(&start_time, nullptr);

  // Determine the calculation method based on user selection and calculate Pi
  if (method == 0)
  {
    cout << "Calculating Pi using Numerical Integration Method..." << endl;
    pi = calculate_pi_numerical_integration();
  }
  else if (method == 1)
  {
    cout << "Calculating Pi using Machin's Formula Method..." << endl;
    pi = calculate_pi_machin();
  }
  else if (method == 2)
  {
    cout << "Calculating Pi using Ramanujan's First Series..." << endl;
    pi = calculate_pi_ramanujan();
  }
  else if (method == 3)
  {
    cout << "Calculating Pi using Chudnovsky's Algorithm..." << endl;
    pi = calculate_pi_chudnovsky();
  }
  else // (method == 4)
  {
    cout << "Calculating Pi using Gauss-Legendre Algorithm..." << endl;
    pi = calculate_pi_gauss_legendre();
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
    cout << "Time taken: " << time_taken << " millisecond(s)" << endl;
  }

  // Call function to display and compare calculated results to expected results
  compare_pi_accuracy(pi, precision);

  // Call the utility function to handle returning to the menu
  wait_for_user_input_to_return();
}

// EOF
