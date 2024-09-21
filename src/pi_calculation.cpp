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
 * Calculates Pi using Machin's formula which approximates Pi using arctangents
 * @return The calculated value of Pi using Machin's formula
 */
mpf_class calculate_pi_machin()
{
  // Machin's formula: Pi = 16 * arctan(1/5) - 4 * arctan(1/239)
  return 16 * arctan(mpf_class(1) / mpf_class(5)) - 4 * arctan(mpf_class(1) / mpf_class(239));
}

/**
 * Calculates Pi using numerical integration which approximates Pi by summing up small areas under a curve multipled by 4
 * @return The calculated value of Pi using numerical integration
 */
double calculate_pi_numerical_integration()
{
  double sum = 0.0;  // Accumulates the area under the curve
  double a = 10000000.0;  // Large constant to ensure accuracy
  double a2 = a * a;  // Precompute a^2 to avoid redundant calculations
  double x, y;  // Variables for calculation
  double dx = 1.0;  // Small step size for integration

  // Loop through small intervals to sum up areas under the curve
  for (x = dx; x <= a - dx; x += dx)
  {
    y = 1.0 / (a2 + (x * x));  // Calculate the value of the function at point x
    sum += (y * dx);  // Approximate the area of small rectangles
  }

  // Approximate the remaining area and multiply by 4 to approximate Pi
  sum += ((((1.0 / a2) + (1.0 / (2 * a2))) / 2.0) * dx);
  return 4.0 * sum * a;
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
    pi = mpf_class(calculate_pi_numerical_integration());
  }
  else
  {
    cout << "Calculating Pi using Machin's Formula Method..." << endl;
    pi = calculate_pi_machin();
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
