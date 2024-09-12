#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>  // For usleep() to pause the program
#include <sys/time.h>  // For gettimeofday for time in milliseconds
#include <ogcsys.h>
#include <gccore.h>
#include <wiiuse/wpad.h>  // Wii remote input

// Global variables for video setup
static void *xfb = NULL;  // Framebuffer pointer (where video memory is stored)
static GXRModeObj *rmode = NULL;  // Structure to store the TV display mode

// Function to initialize the video system and set up the display
void initialize_video() {
    VIDEO_Init();  // Initialize the video system
    PAD_Init();  // Initialize GameCube controller
    WPAD_Init();  // Initialize Wii remotes

    // Detect the current TV mode and set the appropriate video mode
    switch (VIDEO_GetCurrentTvMode()) {
        case VI_NTSC:  // NTSC is common in North America
            rmode = &TVNtsc480IntDf;
            break;
        case VI_PAL:  // PAL is used in Europe and other regions
            rmode = &TVPal528IntDf;
            break;
        case VI_MPAL:  // MPAL is a variation of PAL used in some regions
            rmode = &TVMpal480IntDf;
            break;
        default:  // Default to NTSC if detection fails
            rmode = &TVNtsc480IntDf;
            break;
    }

    // Allocate memory for the framebuffer in uncached memory
    // The framebuffer stores the pixels that will be shown on the screen
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    // Check if framebuffer allocation failed
    if (!xfb) {
        // If the framebuffer allocation failed, print an error message (if possible)
        // and exit the program since we can't continue without video output
        printf("Failed to allocate framebuffer!\n");
        usleep(3000000);  // Pause for 3 seconds
        SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);  // Gracefully exit to Homebrew Channel
        exit(1);  // Fallback if the reset fails
    }

    // Initialize the console system, enabling us to print text using printf
    // Parameters: framebuffer, start x/y, width, height, and pitch (row size)
    console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);

    // Configure the video mode using the chosen display mode (NTSC, PAL, etc.)
    VIDEO_Configure(rmode);

    // Tell the video system where the framebuffer is located
    VIDEO_SetNextFramebuffer(xfb);

    // Make the display visible (disable black screen)
    VIDEO_SetBlack(FALSE);

    // Flush changes to the video system
    VIDEO_Flush();

    // Wait for the video system to sync and apply changes
    VIDEO_WaitVSync();

    // If the TV mode is non-interlaced (i.e., progressive scan), wait for another sync
    if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
}

// Function to compute arctangent using a series approximation (Taylor series)
double arctan(double x) {
    double result = 0.0;  // Variable to store the result of the arctan
    double term = x;  // The first term in the series is x
    int n = 1;  // The first term uses the odd index n=1

    // Continue adding terms to the result while they are larger than a small threshold
    while (fabs(term) > 1e-15) {
        result += term;  // Add the current term to the result
        n += 2;  // The series alternates with odd numbers (3, 5, 7, ...)
        // Compute the next term efficiently without recalculating powers
        term *= -x * x * ((n - 2.0) / n);
    }

    return result;  // Return the final result of the arctangent
}

// Legacy Pi calculation method from the original WPCP (numerical integration)
double calculate_pi_legacy() {
    double sum = 0.0, a = 10000000.0, x, y, dx = 1.0;
    // Loop through small intervals to sum up areas under the curve (numerical integration)
    for (x = dx; x <= a - dx; x += dx) {
        y = 1.0 / ((a * a) + (x * x));  // Calculate the function at point x
        sum += (y * dx);  // Approximate the area of small rectangles
    }
    sum += ((((1.0 / (a * a)) + (1.0 / (2 * a * a))) / 2.0) * dx);
    return 4.0 * sum * a;  // Multiply by 4 * a to approximate Pi
}

// Function to calculate Pi using Machin's formula
double calculate_pi_modern() {
    // Use Machin's formula for Pi:
    // Pi = 16 * arctan(1/5) - 4 * arctan(1/239)
    return 16.0 * arctan(1.0 / 5.0) - 4.0 * arctan(1.0 / 239.0);
}

// Function to time the calculation and print Pi and time taken
void calculate_and_display_pi(int method) {
    struct timeval start_time, end_time;  // Structs to store time in seconds and microseconds
    double pi = 0.0;  // Variable to store the result of the Pi calculation

    // Start the timer
    gettimeofday(&start_time, NULL);  // Start timing

    // Depending on the method, calculate Pi using the appropriate method
    if (method == 0) {
        printf("Calculating Pi using Numerical Integration (Legacy Method)...\n");
        pi = calculate_pi_legacy();  // Call the legacy method: Numerical Integration
    } else {
        printf("Calculating Pi using Machin's Formula (Modern Method)...\n");
        pi = calculate_pi_modern();  // Call the modern method: Machin's Formula
    }

    // Stop the timer
    gettimeofday(&end_time, NULL);  // End timing

    // Calculate the elapsed time in milliseconds
    double time_taken = (end_time.tv_sec - start_time.tv_sec) * 1000.0 +
                        (end_time.tv_usec - start_time.tv_usec) / 1000.0;

    // Display the Pi result and handle unrealistic time values
    printf("\nPi Calculation Complete!\n");
    printf("Pi = %.50lf\n", pi);  // Display 50 digits of Pi

    if (time_taken <= 0) {
        printf("Time taken: unknown (possibly due to emulation)\n");
    } else {
        printf("Time taken: %.2f milliseconds\n", time_taken);  // Display time taken in milliseconds
    }
}

// Function to display the method selection screen with proper names
int display_selection_screen() {
    // Clear the screen before showing method selection
    printf("\x1b[2J");  // ANSI escape code to clear the screen
    printf("Select Pi Calculation Method:\n");
    printf("Press Left on the D-pad for Numerical Integration (Legacy Method).\n");
    printf("Press Right on the D-pad for Machin's Formula (Modern Method).\n");
    printf("\nPress 'Home' on Wii Remote or 'Start' on GameCube controller to exit.\n");

    // Wait for the user to select a method or exit
    while (1) {
        PAD_ScanPads();  // Check GameCube controller inputs
        WPAD_ScanPads();  // Check Wii Remote inputs

        u32 gc_pressed = PAD_ButtonsDown(0);  // GameCube controller button state
        u32 wii_pressed = WPAD_ButtonsDown(0);  // Wii Remote button state

        // Check if the 'Start' button (GameCube) or 'Home' button (Wii Remote) is pressed
        if (gc_pressed & PAD_BUTTON_START || wii_pressed & WPAD_BUTTON_HOME) {
            // Print exit message and return to the Homebrew Channel
            printf("\nExiting to Homebrew Channel...\n");
            usleep(2000000);  // Pause for 2 seconds
            SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);  // Gracefully exit to Homebrew Channel
            exit(1);  // Fallback if the reset fails
        }

        // Return 0 if the user selects the Legacy method (Numerical Integration)
        if (wii_pressed & WPAD_BUTTON_LEFT || gc_pressed & PAD_BUTTON_LEFT) {
            return 0;
        }

        // Return 1 if the user selects the Modern method (Machin's Formula)
        if (wii_pressed & WPAD_BUTTON_RIGHT || gc_pressed & PAD_BUTTON_RIGHT) {
            return 1;
        }

        VIDEO_WaitVSync();  // Wait for video sync to handle input smoothly
    }
}

// Function to wait for the user to press A to recalculate or exit
void wait_for_recalculate_or_exit() {
    printf("\nPress 'A' to calculate Pi again or 'Home'/'Start' to exit.\n");

    while (1) {
        PAD_ScanPads();  // Check GameCube controller inputs
        WPAD_ScanPads();  // Check Wii Remote inputs

        u32 gc_pressed = PAD_ButtonsDown(0);  // GameCube controller button state
        u32 wii_pressed = WPAD_ButtonsDown(0);  // Wii Remote button state

        // Check if the 'Start' button (GameCube) or 'Home' button (Wii Remote) is pressed
        if (gc_pressed & PAD_BUTTON_START || wii_pressed & WPAD_BUTTON_HOME) {
            // Print exit message and return to the Homebrew Channel
            printf("\nExiting to Homebrew Channel...\n");
            usleep(2000000);  // Pause for 2 seconds
            SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);  // Gracefully exit to Homebrew Channel
            exit(1);  // Fallback if the reset fails
        }

        // Recalculate Pi if 'A' is pressed
        if (wii_pressed & WPAD_BUTTON_A || gc_pressed & PAD_BUTTON_A) {
            return;
        }

        VIDEO_WaitVSync();  // Wait for video sync to handle input smoothly
    }
}

// Main function
int main(int argc, char **argv) {
    // Initialize the video system and set up the display
    initialize_video();

    while (1) {
        int method = display_selection_screen();  // Let the user select the Pi calculation method
        calculate_and_display_pi(method);  // Calculate and display Pi
        wait_for_recalculate_or_exit();  // Wait for user input to recalculate or exit
    }

    return 0;  // The program should never reach here
}
