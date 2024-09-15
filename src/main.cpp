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

using namespace std;  // Use the entire std namespace for simplicity

// Global variables for video setup
static void *xfb = nullptr;  // Framebuffer pointer (where video memory is stored)
static GXRModeObj *rmode = nullptr;  // Structure to store the TV display mode

#define PI_DIGITS 16  // "3." followed by 14 decimal places which is the precision limit of double
#define TOTAL_LENGTH (PI_DIGITS + 2)  // '3.' + 15 decimal places

/**
 * Formats the Pi value into a string with up to 14 decimal places.
 * @param pi_value The Pi value to format.
 * @param pi_str The string buffer where the formatted Pi will be stored.
 */
void format_pi(double pi_value, char *pi_str)
{
  // Use 'lf' for double, limiting the precision to 14 decimal places.
  snprintf(pi_str, TOTAL_LENGTH, "%.14lf", pi_value);
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
  cout << "^\n";
  cout << "First mismatch at digit " << mismatch_index << endl;  // Adjusted for '3.'
}

/**
 * Compares the calculated Pi value with the actual Pi value (up to 14 decimal places).
 * This function prints the comparison result and identifies the first mismatched digit, if any.
 * @param calculated_pi The Pi value calculated by the program.
 */
void compare_pi_accuracy(double calculated_pi)
{
  // Handle invalid cases where the Pi value is not valid (this should not happen)
  if (calculated_pi <= 0.0)
  {
    cout << "Invalid input: Pi cannot be less than or equal to zero." << endl;
    return;
  }

  // Buffers for the calculated Pi and the actual Pi strings
  char calculated_str[TOTAL_LENGTH];
  char actual_pi_str[TOTAL_LENGTH];

  // Format the Pi values into strings
  format_pi(calculated_pi, calculated_str);
  format_pi(M_PI, actual_pi_str);  // M_PI from math.h is accurate to 15 decimal places for double

  cout << "\nComparing calculated Pi to the actual value of Pi (up to 14 decimal places)" << endl;

  // Check the '3.' prefix before comparing decimal places
  if (strncmp(calculated_str, "3.", 2) != 0)
  {
    print_mismatch(calculated_str, actual_pi_str, 2);
    cout << "None of the " << (PI_DIGITS - 1) << " digits are correct!" << endl;  // Minus 1 as a decimal point isn't a number
    return;
  }

  // Compare the digits after the decimal point
  int mismatch_index = -1;
  for (int i = 2; i < TOTAL_LENGTH - 1; ++i)  // Skip the first two characters '3.'
  {                                           // TOTAL_LENGTH - 1 avoids null terminator
    if (calculated_str[i] != actual_pi_str[i])
    {
      mismatch_index = i;  // First mismatch found
      break;
    }
  }

  // If there is no mismatch, print results and that all digits are correct
  if (mismatch_index == -1)
  {
    cout << "Actual Pi:     " << actual_pi_str << endl;
    cout << "Calculated Pi: " << calculated_str << endl;
    cout << "All " << (PI_DIGITS - 1) << " digits are correct!" << endl;  // Minus 1 as a decimal point isn't a number
  }
  else  // Print where the first mismatch occurred
  {
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
  VIDEO_Init();  // Initialize the video system
  PAD_Init();  // Initialize GameCube controller input
  WPAD_Init();  // Initialize Wii remote input

  // Detect the current TV mode (e.g., NTSC, PAL) and set the appropriate video mode
  switch (VIDEO_GetCurrentTvMode())
  {
    case VI_NTSC:  // NTSC is common in North America
      rmode = &TVNtsc480IntDf;
      break;
    case VI_PAL:  // PAL is used in Europe and other regions
      rmode = &TVPal528IntDf;
      break;
    case VI_MPAL:  // MPAL is a variation of PAL used in some regions
      rmode = &TVMpal480IntDf;
      break;
    default:  // Default to NTSC if TV mode detection fails
      rmode = &TVNtsc480IntDf;
      break;
  }

  // Allocate memory for the framebuffer, used to store video output
  // The framebuffer stores the pixels that will be shown on the screen
  xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

  // Check if framebuffer allocation was successful
  if (!xfb)
  {
    // If the framebuffer allocation failed, print an error message (if possible)
    // and exit the program since we can't continue without video output
    cout << "Failed to allocate framebuffer!" << endl;
    usleep(3000000);  // Wait for 3 seconds before exiting
    SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);  // Gracefully exit to Homebrew Channel
    exit(1);  // Fallback if the reset fails
  }

  // Initialize the console system to allow printing text (via printf)
  // Parameters: framebuffer, start x/y, width, height, and pitch (row size)
  console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);

  // Set up the video mode and tell the video system where the framebuffer is located
  VIDEO_Configure(rmode); // Configure the video mode using the chosen display mode (NTSC, PAL, etc.)
  VIDEO_SetNextFramebuffer(xfb); // Tell the video system where the framebuffer is located
  VIDEO_SetBlack(FALSE);  // Disable the black screen (make display visible)
  VIDEO_Flush();  // Flush changes to the video system to apply the video configuration
  VIDEO_WaitVSync();  // Wait for the video system to sync and apply changes

  // If the TV mode is non-interlaced (progressive scan), wait for another sync
  if (rmode->viTVMode & VI_NON_INTERLACE)
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
double arctan(double x)
{
  double result = 0.0;  // The result of the arctangent calculation
  double term = x;  // The first term in the series is x
  double x2 = x * x;  // Precompute x^2 to avoid repetitive multiplication
  int n = 1;  // The first term uses n = 1

  // Continue adding terms to the result while they are larger than a small threshold
  while (fabs(term) > 1e-15)  // Controls precision vs. performance: adjust this value to change the trade-off
  {
    result += term;  // Add the current term to the result
    n += 2;  // Increase n by 2 (since the series uses odd numbers)
    term *= -x2 * ((n - 2.0) / n);  // Compute the next term efficiently without recalculating powers
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
  double x, y;  // Variables for calculation
  double dx = 1.0;  // Small step size for integration

  // Loop through small intervals to sum up areas under the curve
  for (x = dx; x <= a - dx; x += dx)
  {
    y = 1.0 / ((a * a) + (x * x));  // Calculate the value of the function at point x
    sum += (y * dx);  // Approximate the area of small rectangles
  }

  // Approximate the remaining area and multiply by 4 to get Pi
  sum += ((((1.0 / (a * a)) + (1.0 / (2 * a * a))) / 2.0) * dx);
  return 4.0 * sum * a;  // Multiply by 4 * a to approximate Pi
}

/**
 * Calculates Pi using Machin's formula (the modern method).
 * Machin's formula is a more efficient method for calculating Pi using arctangents.
 * @return The calculated value of Pi using Machin's formula.
 */
double calculate_pi_modern()
{
  // Machin's formula: Pi = 16 * arctan(1/5) - 4 * arctan(1/239)
  return 16.0 * arctan(1.0 / 5.0) - 4.0 * arctan(1.0 / 239.0);
}

/**
 * Times the Pi calculation and prints both the calculated Pi and the time taken.
 * This function measures the time for Pi calculation and compares it to the known value of Pi.
 * @param method The method to use for Pi calculation: 0 for legacy, 1 for modern.
 */
void calculate_and_display_pi(int method)
{
  struct timeval start_time, end_time;  // Structs to store time (seconds and microseconds)
  double pi = 0.0;  // Variable to store the result of the Pi calculation

  // Start the timer
  gettimeofday(&start_time, nullptr);

  // Depending on the selected method, calculate Pi
  if (method == 0)
  {
    cout << "Calculating Pi using Numerical Integration (Legacy Method)..." << endl;
    pi = calculate_pi_legacy();  // Call the legacy numerical integration method
  }
  else
  {
    cout << "Calculating Pi using Machin's Formula (Modern Method)..." << endl;
    pi = calculate_pi_modern();  // Call the modern Machin's formula method
  }

  // Stop the timer
  gettimeofday(&end_time, nullptr);

  // Calculate the elapsed time in milliseconds
  double time_taken = (end_time.tv_sec - start_time.tv_sec) * 1000.0 +
                      (end_time.tv_usec - start_time.tv_usec) / 1000.0;

  // Display the result of the Pi calculation
  cout << "\nPi Calculation Complete!" << endl;

  // Handle unrealistic time values (negative or zero, which may occur in emulation)
  if (time_taken <= 0)
  {
    cout << "Time taken: unknown (possibly due to emulation)" << endl;
  }
  else
  {
    cout << "Time taken: " << time_taken << " millisecond(s)" << endl;
  }

  // Compare the calculated Pi value with the actual Pi value
  compare_pi_accuracy(pi);
}

/**
 * Displays the method selection screen for the user.
 * The user can choose between two methods for Pi calculation: legacy (numerical integration)
 * and modern (Machin's formula).
 * @return Returns 0 for the legacy method or 1 for the modern method.
 */
int display_selection_screen()
{
  // Clear the screen before displaying the selection menu
  cout << "\x1b[2J";  // ANSI escape code to clear the screen
  cout << "Select Pi Calculation Method:\n";
  cout << "Press Left on the D-pad for Numerical Integration (Legacy Method).\n";
  cout << "Press Right on the D-pad for Machin's Formula (Modern Method).\n";
  cout << "\nPress 'Home' on Wii Remote or 'Start' on GameCube controller to exit.\n";

  // Wait for the user to select a method or exit
  while (true)
  {
    PAD_ScanPads();  // Check GameCube controller inputs
    WPAD_ScanPads();  // Check Wii Remote inputs

    u32 gc_pressed = PAD_ButtonsDown(0);  // GameCube controller button state
    u32 wii_pressed = WPAD_ButtonsDown(0);  // Wii Remote button state

    // Check if the 'Start' button (GameCube) or 'Home' button (Wii Remote) is pressed
    if (gc_pressed & PAD_BUTTON_START || wii_pressed & WPAD_BUTTON_HOME)
    {
      // Print exit message and return to the Homebrew Channel
      cout << "\nExiting to Homebrew Channel..." << endl;
      usleep(2000000);  // Wait for 2 seconds before exiting
      SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);  // Exit to the Homebrew Channel
      exit(1);  // Fallback if the reset fails
    }

    // Return 0 if the user selects the Legacy method (Numerical Integration)
    if (wii_pressed & WPAD_BUTTON_LEFT || gc_pressed & PAD_BUTTON_LEFT)
    {
      return 0;
    }

    // Return 1 if the user selects the Modern method (Machin's Formula)
    if (wii_pressed & WPAD_BUTTON_RIGHT || gc_pressed & PAD_BUTTON_RIGHT)
    {
      return 1;
    }

    // Wait for video sync to handle input smoothly
    VIDEO_WaitVSync();
  }
}

/**
 * Waits for the user to press 'A' to recalculate Pi or 'Home/Start' to exit.
 */
void wait_for_recalculate_or_exit()
{
  cout << "\nPress 'A' to calculate Pi again or 'Home'/'Start' to exit." << endl;

  while (true)
  {
    PAD_ScanPads();  // Check GameCube controller inputs
    WPAD_ScanPads();  // Check Wii Remote inputs

    u32 gc_pressed = PAD_ButtonsDown(0);  // GameCube controller button state
    u32 wii_pressed = WPAD_ButtonsDown(0);  // Wii Remote button state

    // If 'Start' or 'Home' is pressed, exit to the Homebrew Channel
    if (gc_pressed & PAD_BUTTON_START || wii_pressed & WPAD_BUTTON_HOME)
    {
      // Print exit message and return to the Homebrew Channel
      cout << "\nExiting to Homebrew Channel..." << endl;
      usleep(2000000);  // Wait for 2 seconds before exiting
      SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);  // Exit to the Homebrew Channel
      exit(1);  // Fallback if the reset fails
    }

    // Recalculate Pi if 'A' is pressed
    if (wii_pressed & WPAD_BUTTON_A || gc_pressed & PAD_BUTTON_A)
    {
      return;
    }

    // Wait for video sync to handle input smoothly
    VIDEO_WaitVSync();
  }
}

/**
 * Main function that runs the Pi calculation loop.
 * It initializes the video system, displays the method selection screen, and performs
 * the Pi calculation. The loop continues until the user exits the program.
 */
int main(int argc, char **argv)
{
  // Initialize the video system and set up the display
  initialize_video();

  // Loop until the user exits the program
  while (true)
  {
    // Let the user select the Pi calculation method
    int method = display_selection_screen();

    // Calculate and display Pi using the selected method
    calculate_and_display_pi(method);

    // Wait for the user to choose to recalculate or exit
    wait_for_recalculate_or_exit();
  }

  return 0;  // The program should never reach here
}

// EOF
