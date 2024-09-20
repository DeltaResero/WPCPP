//  main.cpp
//
//  Wii Pi Calculator Project Plus (WPCPP)
//  Copyright (C) 2023 DeltaResero
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

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <unistd.h>  // For usleep() to pause the program
#include <sys/time.h>  // For gettimeofday for time in milliseconds
#include <ogcsys.h>
#include <gccore.h>
#include <wiiuse/wpad.h>  // Wii remote input
#include <gmpxx.h>  // GMP for arbitrary precision arithmetic

using namespace std;  // Use the entire std namespace for simplicity

// Define a context to encapsulate video-related global state
struct VideoContext {
  void *xfb;  // Framebuffer pointer
  GXRModeObj *rmode;  // TV display mode
  bool initialized;  // Tracks whether the video system is initialized
};

// Initialize a video context globally
static VideoContext video_ctx = { nullptr, nullptr, false };  // Keep track of video state in one place

#define PI_DIGITS 50  // Number of decimal places of Pi
#define TOTAL_LENGTH (PI_DIGITS + 3)  // '3.' + digits + null terminator

/**
 * Exits the program and attempts to return to the Homebrew Channel or system menu.
 * Always waits for 3 seconds before exiting.
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

/**
 * Formats the Pi value into a string with a specified number of decimal places.
 * @param pi_value The Pi value to format.
 * @param pi_str The string buffer where the formatted Pi will be stored.
 * @param precision Number of decimal places to format Pi.
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
 * Prints the location of the first mismatched digit between the calculated
 * Pi string and the actual Pi string.
 * @param calculated_str The string of the calculated Pi value.
 * @param actual_str The string representing the actual Pi value.
 * @param mismatch_index The index where the mismatch occurs.
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

/**
 * Compares the calculated Pi value with the actual Pi value (up to the specified precision).
 * This function prints the comparison result and identifies the first mismatched digit, if any.
 * @param calculated_pi The Pi value calculated by the program.
 * @param precision The number of decimal places to compare.
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
 * Initializes the video system and sets up the display for the Wii.
 * This function configures the video mode based on the TV format (NTSC, PAL, etc.)
 * and initializes the framebuffer for video output.
 */
void initialize_video()
{
  if (video_ctx.initialized) return;  // Early return if already initialized
  video_ctx.initialized = true;  // Mark as initialized

  VIDEO_Init();  // Initialize the video system
  PAD_Init();  // Initialize GameCube controller input
  WPAD_Init();  // Initialize Wii remote input

  // Detect the current TV mode (e.g., NTSC, PAL) and set the appropriate video mode
  switch (VIDEO_GetCurrentTvMode())
  {
    case VI_NTSC:  // NTSC is common in North America
      video_ctx.rmode = &TVNtsc480IntDf;
      break;
    case VI_PAL:  // PAL is used in Europe and other regions
      video_ctx.rmode = &TVPal528IntDf;
      break;
    case VI_MPAL:  // MPAL is a variation of PAL used in some regions
      video_ctx.rmode = &TVMpal480IntDf;
      break;
    default:  // Default to NTSC if TV mode detection fails
      video_ctx.rmode = &TVNtsc480IntDf;
      break;
  }

  // Allocate memory for the framebuffer, used to store video output
  // The framebuffer stores the pixels that will be shown on the screen
  video_ctx.xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(video_ctx.rmode));

  // If framebuffer allocation fails, handle the error
  if (!video_ctx.xfb)
  {
    // If the framebuffer allocation failed, print an error message (if possible)
    // and exit the program since we can't continue without video output
    cout << "Failed to allocate framebuffer!" << endl;
    usleep(2000000);  // Wait for 2 seconds before calling exit
    exit_WPCPP();  // Call to exit and reset system
  }

  // Initialize the console system to allow printing text
  // Parameters: framebuffer, start x/y, width, height, and pitch (row size)
  console_init(video_ctx.xfb, 20, 20, video_ctx.rmode->fbWidth, video_ctx.rmode->xfbHeight, video_ctx.rmode->fbWidth * VI_DISPLAY_PIX_SZ);

  // Set up the video mode and tell the video system where the framebuffer is located
  VIDEO_Configure(video_ctx.rmode);  // Configure the video mode using the chosen display mode (NTSC, PAL, etc.)
  VIDEO_SetNextFramebuffer(video_ctx.xfb);  // Tell the video system where the framebuffer is located
  VIDEO_SetBlack(FALSE);  // Disable the black screen (make display visible)
  VIDEO_Flush();  // Flush changes to the video system to apply the video configuration
  VIDEO_WaitVSync();  // Wait for the video system to sync and apply changes

  // If the TV mode is non-interlaced (progressive scan), wait for another sync
  if (video_ctx.rmode->viTVMode & VI_NON_INTERLACE)
  {
    VIDEO_WaitVSync();
  }
}

/**
 * Computes the arctangent using a Taylor series approximation.
 * This function is crucial for the Machin's formula calculation of Pi.
 * @param x The value to compute arctangent for.
 * @return The computed arctangent of x.
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
 * Calculates Pi using numerical integration (the legacy method).
 * This method approximates Pi by summing up small areas under a curve.
 * @return The calculated value of Pi using numerical integration.
 */
double calculate_pi_legacy()
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
 * Calculates Pi using Machin's formula (the modern method).
 * Machin's formula is a more efficient method for calculating Pi using arctangents.
 * @return The calculated value of Pi using Machin's formula.
 */
mpf_class calculate_pi_modern()
{
  // Machin's formula: Pi = 16 * arctan(1/5) - 4 * arctan(1/239)
  mpf_class pi;
  pi = 16 * arctan(mpf_class(1) / mpf_class(5)) - 4 * arctan(mpf_class(1) / mpf_class(239));
  return pi;
}

/**
 * Displays a menu and allows the user to navigate and select options.
 * The options are navigated using Left/Right on the D-pad, and 'A' selects the option.
 * @return The index of the selected option.
 */
int display_selection_screen()
{
  // Array of Pi calculation methods
  string pi_methods[] = {
    "Numerical Integration (Legacy Method)",
    "Machin's Formula (Modern Method)"
  };
  int num_methods = sizeof(pi_methods) / sizeof(pi_methods[0]);
  int selected_index = 0;

  // Track the previous state of buttons to detect state changes
  bool button_right_last = false;
  bool button_left_last = false;
  bool button_a_last = false;

  // Clear the screen and display instructions
  cout << "\x1b[2J";
  cout << "Select Pi Calculation Method:\n";
  cout << "Use Left/Right on the D-pad to navigate.\n";
  cout << "Press 'A' to confirm.\n";
  cout << "Press 'Home' on Wii Remote or 'Start' on GameCube controller to exit.\n";

  // Loop until the user selects a method or exits
  while (true)
  {
    PAD_ScanPads();   // Update GameCube controller state
    WPAD_ScanPads();  // Update Wii Remote state

    u32 gc_pressed = PAD_ButtonsDown(0);  // Get GameCube Controller button state
    u32 wii_pressed = WPAD_ButtonsDown(0);  // Get Wii Remote button state
    VIDEO_WaitVSync();  // Sync video to prevent ghost inputs

    // Check if navigation buttons are pressed
    bool button_right_down = (wii_pressed & WPAD_BUTTON_RIGHT) || (gc_pressed & PAD_BUTTON_RIGHT);
    bool button_left_down = (wii_pressed & WPAD_BUTTON_LEFT) || (gc_pressed & PAD_BUTTON_LEFT);
    bool button_a_down = (gc_pressed & PAD_BUTTON_A) || (wii_pressed & WPAD_BUTTON_A);

    // Navigate to the right method
    if (button_right_down && !button_right_last)
    {
      if (selected_index < num_methods - 1)  // Ensure it doesn't go out of bounds
      {
        selected_index++;  // Move to the next method
      }
    }

    // Navigate to the left method
    if (button_left_down && !button_left_last)
    {
      if (selected_index > 0)  // Ensure it doesn't go below 0
      {
        selected_index--;  // Move to the previous method
      }
    }

    // Update last button states for the next iteration
    button_right_last = button_right_down;
    button_left_last = button_left_down;

    // Display the currently selected method
    cout << "\rCurrently Selected: " << pi_methods[selected_index] << "      \r";

    // Confirm selection when 'A' button is pressed
    if (button_a_down && !button_a_last)
    {
      return selected_index;  // Return the selected method index
    }

    // Update the last state of the 'A' button
    button_a_last = button_a_down;

    // Check if 'Home' button (Wii Remote) or 'Start' button (GameCube) is pressed to exit
    if (gc_pressed & PAD_BUTTON_START || wii_pressed & WPAD_BUTTON_HOME)
    {
      exit_WPCPP();  // Exit the program and return to the system menu
    }

    // Wait for video sync to ensure smooth input handling
    VIDEO_WaitVSync();
  }
}

/**
 * Displays a precision selection screen to allow the user to choose the number
 * of decimal places for the Pi calculation.
 * @return The selected precision (between 1 and 50 decimal places).
 */
int display_precision_selection_screen()
{
  int precision = 50;  // Start with maximum precision (50 decimal places)
  int step_size = 1;   // Initial step size for adjusting precision

  // Track the previous state of buttons to detect state changes
  bool button_a_last = false;
  bool button_l_last = false;
  bool button_r_last = false;
  bool button_left_last = false;
  bool button_right_last = false;

  // Clear the screen and display instructions
  cout << "\x1b[2J";  // Clear console screen
  cout << "Select Pi Precision (1-50 decimal places):\n";
  cout << "Use Left/Right on the D-pad to adjust.\n";
  cout << "Press 'L'/'R' or '-'/'+' to change the stepping size.\n";
  cout << "Press 'A' to confirm.\n";

  // Loop until the user confirms their precision selection
  while (true)
  {
    PAD_ScanPads();   // Update the state of GameCube controller
    WPAD_ScanPads();  // Update the state of Wii Remote
    u32 gc_pressed = PAD_ButtonsDown(0);  // Get GameCube Controller button state
    u32 wii_pressed = WPAD_ButtonsDown(0);  // Get Wii Remote button state
    VIDEO_WaitVSync();  // Sync video to avoid input ghosting

    // Check if specific buttons are pressed
    bool button_l_down = (gc_pressed & PAD_TRIGGER_L) || (wii_pressed & WPAD_BUTTON_MINUS);
    bool button_r_down = (gc_pressed & PAD_TRIGGER_R) || (wii_pressed & WPAD_BUTTON_PLUS);
    bool button_a_down = (gc_pressed & PAD_BUTTON_A) || (wii_pressed & WPAD_BUTTON_A);
    bool button_left_down = (gc_pressed & PAD_BUTTON_LEFT) || (wii_pressed & WPAD_BUTTON_LEFT);
    bool button_right_down = (gc_pressed & PAD_BUTTON_RIGHT) || (wii_pressed & WPAD_BUTTON_RIGHT);

    // Decrease step size by a factor of 10 if the L triggers or "-" button is pressed
    if (button_l_down && !button_l_last)
    {
      if (step_size > 1)  // Ensure step size doesn't go below 1
      {
        step_size /= 10;  // Reduce step size
      }
    }

    // Increase step size by a factor of 10 if the R trigger or "+" button is pressed
    if (button_r_down && !button_r_last)
    {
      if (step_size < 10)  // Ensure step size doesn't exceed 10
      {
        step_size *= 10;  // Increase step size
      }
    }

    // Decrease precision if the left D-pad button is pressed, ensuring it stays >= 1
    if (button_left_down && !button_left_last)
    {
      if (precision - step_size >= 1)  // Ensure precision doesn't go below 1
      {
        precision -= step_size;  // Decrease precision
      }
    }

    // Increase precision if the right D-pad button is pressed, ensuring it stays <= 50
    if (button_right_down && !button_right_last)
    {
      if (precision + step_size <= 50)  // Ensure precision doesn't exceed 50
      {
        precision += step_size;  // Increase precision
      }
    }

    // Update last button states for the next iteration
    button_l_last = button_l_down;
    button_r_last = button_r_down;
    button_left_last = button_left_down;
    button_right_last = button_right_down;

    // Display the current precision and step size
    cout << "\rCurrent Precision: " << precision << " decimal places  (Step Size: " << step_size << ")     \r";

    // Confirm selection when 'A' button is pressed
    if (button_a_down && !button_a_last)
    {
      return precision;  // Return the selected precision
    }

    // Update the last state of the 'A' button
    button_a_last = button_a_down;

    // Wait for video sync to ensure smooth input handling
    VIDEO_WaitVSync();
  }
}

/**
 * Times the Pi calculation and prints both the calculated Pi and the time taken.
 * This function measures the time for Pi calculation and compares it to the known value of Pi.
 * @param method The method to use for Pi calculation: 0 for legacy, 1 for modern.
 * @param precision The number of decimal places for the Pi calculation.
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
    cout << "Calculating Pi using Numerical Integration (Legacy Method)..." << endl;
    pi = mpf_class(calculate_pi_legacy());  // Call the legacy numerical integration method
  }
  else
  {
    cout << "Calculating Pi using Machin's Formula (Modern Method)..." << endl;
    pi = calculate_pi_modern();  // Call the modern Machin's formula method
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

  // Prompt user to return to menu
  cout << "Press any button to return to the menu." << endl;
  while (true)
  {
    PAD_ScanPads();  // Update GameCube controller state
    WPAD_ScanPads();  // Update Wii Remote state
    u32 gc_pressed = PAD_ButtonsDown(0);  // Get GameCube Controller button state
    u32 wii_pressed = WPAD_ButtonsDown(0);  // Get Wii Remote button state
    if (gc_pressed || wii_pressed)  // Check if any button is pressed
    {
      break;  // Exit the loop to return to the menu
    }
    VIDEO_WaitVSync();  // Wait for video sync to ensure smooth input handling
  }
}

/**
 * Main function that runs the Pi calculation loop.
 * It initializes the video system, displays the method selection screen, and performs
 * the Pi calculation. The loop continues until the user exits the program.
 */
int main(int argc, char **argv)
{
  // Set the default GMP precision to handle up to 50 decimal places
  mpf_set_default_prec(167);  // 50 decimal places requires approximately 167 bits

  // Initialize the video system and prepare the display
  initialize_video();

  // Main loop to keep the program running until the user decides to exit
  while (true)
  {
    // Prompt the user to select a method for calculating Pi and a desired precision level
    int method = display_selection_screen();
    int precision = display_precision_selection_screen();

    // Calculate Pi using the selected method and precision, then display the result
    calculate_and_display_pi(method, precision);
  }

  return 0;  // This point should never be reached, as the loop is infinite
}

// EOF
