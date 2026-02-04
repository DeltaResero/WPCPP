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
#include "input.hpp"
#include <gmpxx.h>
#include <iostream>
#include <cmath>
#include <sys/time.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <random>
#include <vector>
#include <string>

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
 * @param precision The number of decimal places of Pi to calculate
 * @return The calculated value of Pi using Ramanujan's series
 */
mpf_class calculate_pi_ramanujan(int precision)
{
  mpf_class sum = 0.0;  // Initialize the sum to accumulate series terms
  mpf_class factor = 2 * sqrt(mpf_class(2)) / 9801;  // Precompute the constant factor in Ramanujan's formula

  int iterations = (precision / 8) + 2;  // Number of iterations controls the precision of the result (precision vs. performance)

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
 * @param precision The number of decimal places of Pi to calculate
 * @return The calculated value of Pi using the Chudnovsky algorithm
 */
mpf_class calculate_pi_chudnovsky(int precision)
{
  // Constant term in the Chudnovsky formula: C = 426880 * sqrt(10005)
  mpf_class C = 426880 * sqrt(mpf_class(10005));

  mpf_class sum = 0;  // Initialize the sum to accumulate series terms

  int iterations = (precision / 14) + 2;  // Number of iterations controls the precision of the result (precision vs. performance)

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
 * Calculates Pi using the Spigot algorithm
 * The Spigot algorithm calculates Pi one digit at a time using a specific sequence of operations,
 * and it is known for its ability to output the digits of Pi without needing high memory or large precision for intermediate results
 * @param precision The number of decimal places of Pi to calculate
 * @return The calculated value of Pi using the Spigot algorithm
 */
mpf_class calculate_pi_spigot(int precision)
{
  // Calculate one extra digit for proper rounding/truncation handling
  const int N = precision + 2;  // Set the number of digits of Pi we want to calculate based on the precision parameter
  int len = static_cast<int>(floor(10 * N / 3) + 1);  // Calculate array size based on the number of digits to process

  // Initialize the array 'A' to store intermediate values, starting with 2's
  std::vector<int> A(len, 2);

  // Track how many 9's and pre-digits occur for rounding
  int nines = 0;
  int predigit = 0;

  mpf_class pi = 0.0;  // `pi` will store the accumulated value of Pi as we calculate it
  mpf_class ten = 10.0;  // We use this constant to handle decimal places
  mpf_class multiplier = 1.0;  // The multiplier helps us keep track of the place value (like tenths, hundredths, etc.)

  // Loop through each digit position to calculate the digits of Pi
  for (int j = 1; j <= N; ++j)
  {
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

      // Set any earlier 9's to zero in Pi
      for (int k = 0; k < nines; ++k)
      {
        pi += 9 * multiplier;  // Add 9 at the appropriate place value
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
  mpf_class sixteen = 16.0;  // The base (16) used in the BBP formula
  mpf_class temp;  // Temporary variable to store intermediate results of 16^(-k)
  int iterations = static_cast<int>(precision / 1.2) + 2;  // Number of iterations (terms) to calculate. More terms yield higher precision

  // Loop through each term in the BBP series to accumulate the value of Pi
  for (int k = 0; k < iterations; ++k)
  {
    // Compute the current term of the BBP series.
    mpf_class term = (mpf_class(4) / (8 * k + 1))  // The first part of the BBP term
                   - (mpf_class(2) / (8 * k + 4))  // The second part
                   - (mpf_class(1) / (8 * k + 5))  // The third part
                   - (mpf_class(1) / (8 * k + 6));  // The fourth part

    // Compute 16^(-k) using GMP's `mpf_pow_ui`.
    mpf_pow_ui(temp.get_mpf_t(), sixteen.get_mpf_t(), k);  // Calculate 16^k and store it in `temp`

    // Add the current term, divided by 16^k, to Pi
    pi += term / temp;  // Add the term divided by 16^k to the running total
  }

  return pi;  // Return the calculated value of Pi
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

  // Determine the calculation method based on user selection and calculate Pi
  switch (method)
  {
    case 0:
      cout << "Calculating Pi using Numerical Integration Method..." << endl;
      pi = calculate_pi_numerical_integration();
      break;
    case 1:
      cout << "Calculating Pi using Machin's Formula Method..." << endl;
      pi = calculate_pi_machin();
      break;
    case 2:
      cout << "Calculating Pi using Ramanujan's First Series..." << endl;
      pi = calculate_pi_ramanujan(precision);
      break;
    case 3:
      cout << "Calculating Pi using Chudnovsky's Algorithm..." << endl;
      pi = calculate_pi_chudnovsky(precision);
      break;
    case 4:
      cout << "Calculating Pi using Gauss-Legendre Algorithm..." << endl;
      pi = calculate_pi_gauss_legendre(precision);
      break;
    case 5:
      cout << "Calculating Pi using Spigot Algorithm..." << endl;
      pi = calculate_pi_spigot(precision);
      break;
    case 6:
      cout << "Calculating Pi using Bailey-Borwein-Plouffe (BBP) formula..." << endl;
      pi = calculate_pi_bbp(precision);
      break;
    default:
      cout << "Invalid method selection." << endl;
      return;
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

  // Get the full string representation of Pi for pagination
  char pi_full_str_c[TOTAL_LENGTH];
  format_pi(pi, pi_full_str_c, precision);
  string pi_full_string(pi_full_str_c);

  // Get the accuracy report
  AccuracyReport accuracy_info = compare_pi_accuracy(pi, precision);

  // --- New Pagination and Display Loop ---
  const int page_size = 1200; // A larger page size to better fill the screen
  int total_pages = (pi_full_string.length() + page_size - 1) / page_size;
  if (total_pages == 0) { total_pages = 1; }
  int current_page = 0;
  bool needs_redraw = true;

  while (true)
  {
    WPAD_ScanPads();
    u32 pressed = WPAD_ButtonsDown(0);

    if (pressed & WPAD_BUTTON_RIGHT)
    {
      if (current_page < total_pages - 1)
      {
        current_page++;
        needs_redraw = true;
      }
    }
    if (pressed & WPAD_BUTTON_LEFT)
    {
      if (current_page > 0)
      {
        current_page--;
        needs_redraw = true;
      }
    }
    if (pressed & (WPAD_BUTTON_A | WPAD_BUTTON_B | WPAD_BUTTON_HOME))
    {
      break;
    }

    if (needs_redraw)
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
      int start_pos = current_page * page_size;
      string page_content_raw = pi_full_string.substr(start_pos, page_size);
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

      // Print the Footer
      if (total_pages > 1)
      {
        cout << "\nPage " << (current_page + 1) << " of " << total_pages << endl;
        cout << "Use D-Pad Left/Right to scroll." << endl;
      }

      cout << "Press A/B to return to menu. Press Home/Start to exit." << endl;

      needs_redraw = false;
    }

    VIDEO_WaitVSync();
  }
}

// EOF
