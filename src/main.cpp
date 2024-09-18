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

// Define a context to encapsulate video-related global state
struct VideoContext {
  void *xfb;  // Framebuffer pointer
  GXRModeObj *rmode;  // TV display mode
  bool initialized;  // Tracks whether the video system is initialized
};

// Initialize a video context globally
static VideoContext video_ctx = { nullptr, nullptr, false };  // Keep track of video state in one place

#define PI_DIGITS 16  // "3." followed by 14 decimal places which is the precision limit of double
#define TOTAL_LENGTH (PI_DIGITS + 2)  // '3.' + 15 decimal places

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
void format_pi(double pi_value, char *pi_str, int precision)
{
  // FIXME: Use 'lf' for double, limiting the precision to a max of 14 decimal places for now.
  snprintf(pi_str, TOTAL_LENGTH, "%.*lf", precision, pi_value); // Format Pi to 'precision' number of decimal places
}

/**
 * Prints the location of the first mismatched digit between the calculated
 * Pi string and the actual Pi string.
 * @param calculated_str The string of the calculated Pi value.
 * @param actual_str The string representing the actual Pi value.
 * @param mismatch_index The index where the mismatch occurs.
 */
void print_mismatch(const char *calculated_str, const char *actual_str, int mismatch_index)
{ // Method assumes "3." to be correct and handled elsewhere as special cases
  cout << "Actual Pi:     " << actual_str << endl;
  cout << "Calculated Pi: " << calculated_str << endl;

  // Print an arrow pointing to the first mismatch
  cout << "               ";  // Aligns the arrow with the Pi values
  for (int i = 0; i < mismatch_index; ++i) // offset for the decimal point
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
void compare_pi_accuracy(double calculated_pi, int precision)
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
  format_pi(calculated_pi, calculated_str, precision);
  format_pi(M_PI, actual_pi_str, precision);  // FIXME: M_PI from math.h is only accurate to 15 decimal places

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
    // No mismatches
    cout << "Actual Pi:     " << actual_pi_str << endl;
    cout << "Calculated Pi: " << calculated_str << endl;
    cout << "All " << precision << " digit(s) after the decimal are correct!" << endl;
  }
  else
  {
    // Print where the first mismatch occurred
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
double calculate_pi_modern()
{
  // Machin's formula: Pi = 16 * arctan(1/5) - 4 * arctan(1/239)
  return 16.0 * arctan(1.0 / 5.0) - 4.0 * arctan(1.0 / 239.0);
}

/**
 * Displays a menu and allows the user to navigate and select options.
 * The options are navigated using Left/Right on the D-pad, and 'A' selects the option.
 * @return The index of the selected option.
 */
int display_selection_screen()
{
  // Clear the screen before displaying the selection menu
  cout << "\x1b[2J";  // ANSI escape code to clear the screen

  string pi_methods[] = {
    "Numerical Integration (Legacy Method)",
    "Machin's Formula (Modern Method)"
  };
  int num_methods = sizeof(pi_methods) / sizeof(pi_methods[0]);
  int selected_index = 0;

  bool button_right_last = false;
  bool button_left_last = false;
  bool button_a_last = false;  // Track last state of 'A' button to handle debounce

  cout << "\x1b[2J";
  cout << "Select Pi Calculation Method:\n";
  cout << "Use Left/Right on the D-pad to navigate.\n";
  cout << "Press 'A' to confirm.\n";
  cout << "Press 'Home' on Wii Remote or 'Start' on GameCube controller to exit.\n";

  // Wait for the user to select a method or exit
  while (true)
  {
    PAD_ScanPads();  // GameCube controller
    WPAD_ScanPads(); // Wii Remote

    u32 gc_pressed = PAD_ButtonsDown(0);  // GameCube button state
    u32 wii_pressed = WPAD_ButtonsDown(0); // Wii button state
    VIDEO_WaitVSync();  // Prevent ghost inputs

    bool button_right_down = (wii_pressed & WPAD_BUTTON_RIGHT) || (gc_pressed & PAD_BUTTON_RIGHT);
    bool button_left_down = (wii_pressed & WPAD_BUTTON_LEFT) || (gc_pressed & PAD_BUTTON_LEFT);
    bool button_a_down = (gc_pressed & PAD_BUTTON_A) || (wii_pressed & WPAD_BUTTON_A);

    // Navigate Right
    if (button_right_down && !button_right_last)
    {
      if (selected_index < num_methods - 1)
      {
        selected_index++;
      }
    }

    // Navigate Left
    if (button_left_down && !button_left_last)
    {
      if (selected_index > 0)
      {
        selected_index--;
      }
    }

    button_right_last = button_right_down;
    button_left_last = button_left_down;

    cout << "\rCurrently Selected: " << pi_methods[selected_index] << "      \r";

    // A button is pressed to select method
    if (button_a_down && !button_a_last)
    {
      return selected_index;
    }

    button_a_last = button_a_down;

    // Check if 'Home' button (Wii Remote) or 'Start' button (GameCube) is pressed
    if (gc_pressed & PAD_BUTTON_START || wii_pressed & WPAD_BUTTON_HOME)
    {
      exit_WPCPP();  // Exit the program and return to the system menu
    }

    // Wait for video sync to handle input smoothly
    VIDEO_WaitVSync();
  }
}

/**
 * Displays a precision selection screen to allow the user to choose the number
 * of decimal places for the Pi calculation.
 * @return The selected precision (between 1 and 14 decimal places).
 */
int display_precision_selection_screen()
{
  int precision = 14;  // Start with maximum precision
  int step_size = 1;  // Default step size is 1 (i.e., move by 1 digit at a time)
  bool button_a_last = false;
  bool button_l_last = false;
  bool button_r_last = false;
  bool button_left_last = false;
  bool button_right_last = false;

  cout << "\x1b[2J";
  cout << "Select Pi Precision (1-14 decimal places):\n";
  cout << "Use Left/Right on the D-pad to adjust.\n";
  cout << "Press 'L'/'R' or '-'/'+' to change the stepping size.\n";
  cout << "Press 'A' to confirm.\n";

  while (true)
  {
    PAD_ScanPads();  // GameCube controller
    WPAD_ScanPads();  // Wii Remote
    u32 gc_pressed = PAD_ButtonsDown(0);  // GameCube button state
    u32 wii_pressed = WPAD_ButtonsDown(0);  // Wii button state
    VIDEO_WaitVSync();  // Prevent ghost inputs

    bool button_l_down = (gc_pressed & PAD_TRIGGER_L) || (wii_pressed & WPAD_BUTTON_MINUS);
    bool button_r_down = (gc_pressed & PAD_TRIGGER_R) || (wii_pressed & WPAD_BUTTON_PLUS);
    bool button_a_down = (gc_pressed & PAD_BUTTON_A) || (wii_pressed & WPAD_BUTTON_A);
    bool button_left_down = (gc_pressed & PAD_BUTTON_LEFT) || (wii_pressed & WPAD_BUTTON_LEFT);
    bool button_right_down = (gc_pressed & PAD_BUTTON_RIGHT) || (wii_pressed & WPAD_BUTTON_RIGHT);

    // Adjust stepping size with L triggers or - button
    if (button_l_down && !button_l_last)
    {
      if (step_size > 1)
      {
        step_size /= 10;  // Reduce step size (go to smaller step)
      }
    }

    // Adjust stepping size with R trigger or + button
    if (button_r_down && !button_r_last)
    {
      if (step_size < 10)
      {
        step_size *= 10;  // Increase step size (go to larger step)
      }
    }

    // Adjust precision using Left D-pad with current step size
    if (button_left_down && !button_left_last)
    {
      if (precision - step_size >= 1)
      {
        precision -= step_size;  // Decrease precision by the step size
      }
    }

    // Adjust precision using Right D-pad with current step size
    if (button_right_down && !button_right_last)
    {
      if (precision + step_size <= 14)
      {
        precision += step_size;  // Increase precision by the step size
      }
    }

    // Update last button states
    button_l_last = button_l_down;
    button_r_last = button_r_down;
    button_left_last = button_left_down;
    button_right_last = button_right_down;

    // Display current precision and step size
    cout << "\rCurrent Precision: " << precision << " decimal places  (Step Size: " << step_size << ")     \r";

    if (button_a_down && !button_a_last)
    {
      return precision;  // Confirm selection with 'A'
    }

    button_a_last = button_a_down;

    // Wait for video sync to handle input smoothly
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

  // Output precision level before starting
  cout << "Precision level set to: " << precision << " decimal place(s)" << endl;

  struct timeval start_time, end_time;  // Structs to store time (seconds and microseconds)
  double pi = 0.0;  // FIXME: Variable to store the result of the Pi calculation currently limited by double

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

  // Call function to display and compare calculated results to expected results
  compare_pi_accuracy(pi, precision);

  cout << "Press any button to return to the menu." << endl;
  while (true)
  {
    PAD_ScanPads();
    WPAD_ScanPads();
    u32 gc_pressed = PAD_ButtonsDown(0);
    u32 wii_pressed = WPAD_ButtonsDown(0);
    if (gc_pressed || wii_pressed)
    {
      break;
    }
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
    // Let the user select the Pi calculation method and level of precision
    int method = display_selection_screen();
    int precision = display_precision_selection_screen();

    calculate_and_display_pi(method, precision);
  }

  return 0;  // The program should never reach here
}

// EOF
