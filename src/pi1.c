#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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
        exit(1);
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

// Function to calculate Pi using Machin's formula
void calculate_pi() {
    // Use Machin's formula for Pi:
    // Pi = 16 * arctan(1/5) - 4 * arctan(1/239)
    double pi = 16.0 * arctan(1.0 / 5.0) - 4.0 * arctan(1.0 / 239.0);

    // Print the result of Pi with high precision (50 decimal places)
    printf("\nPi Calculation Complete!\n");
    printf("\nPi = %.50lf\n", pi);
}

// Function to wait for the user to press 'Home' on the Wii Remote or 'Start' on the GameCube controller
void wait_for_exit() {
    printf("\nPress 'Home' on Wii Remote or 'Start' on GameCube controller to exit.\n");

    // Loop indefinitely, waiting for user input to exit the program
    while (1) {
        PAD_ScanPads();  // Check GameCube controller inputs
        WPAD_ScanPads();  // Check Wii remote inputs

        u32 gc_pressed = PAD_ButtonsDown(0);  // GameCube controller button state
        u32 wii_pressed = WPAD_ButtonsDown(0);  // Wii remote button state

        // Check if the 'Start' button (GameCube) or 'Home' button (Wii Remote) is pressed
        if (gc_pressed & PAD_BUTTON_START || wii_pressed & WPAD_BUTTON_HOME) {
            // Print exit message and return to the Homebrew Channel
            printf("\nExiting to Homebrew Channel...\n");
            VIDEO_WaitVSync();  // Wait for a sync to ensure the message is shown
            SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);  // Return to Homebrew Channel
        }

        // Sync the video after each frame
        VIDEO_WaitVSync();
    }
}

// Main function
int main(int argc, char **argv) {
    // Initialize the video system and set up the display
    initialize_video();

    // Perform the Pi calculation using Machin's formula
    calculate_pi();

    // Wait for the user to press the 'Home' or 'Start' button to exit
    wait_for_exit();

    return 0;
}
