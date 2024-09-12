#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ogcsys.h>
#include <gccore.h>
#include <wiiuse/wpad.h>  // Wii remote input

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

void initialize_video() {
    VIDEO_Init();
    PAD_Init();  // Initialize GameCube controller
    WPAD_Init();  // Initialize Wii remotes

    switch (VIDEO_GetCurrentTvMode()) {
        case VI_NTSC:
            rmode = &TVNtsc480IntDf;
            break;
        case VI_PAL:
            rmode = &TVPal528IntDf;
            break;
        case VI_MPAL:
            rmode = &TVMpal480IntDf;
            break;
        default:
            rmode = &TVNtsc480IntDf;
            break;
    }

    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    if (!xfb) {
        printf("Failed to allocate framebuffer!\n");
        exit(1);
    }

    console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);

    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
}

double arctan(double x) {
    double result = 0.0;
    double term = x;
    int n = 1;

    while (fabs(term) > 1e-15) {
        result += term;
        n += 2;
        term *= -x * x * ((n - 2.0) / n);  // Efficient term computation
    }

    return result;
}

void calculate_pi() {
    // Use Machin's formula for Pi:
    // Pi = 16 * arctan(1/5) - 4 * arctan(1/239)
    double pi = 16.0 * arctan(1.0 / 5.0) - 4.0 * arctan(1.0 / 239.0);
    printf("\nPi Calculation Complete!\n");
    printf("\nPi = %.50lf\n", pi);
}

void wait_for_exit() {
    printf("\nPress 'Home' on Wii Remote or 'Start' on GameCube controller to exit.\n");

    while (1) {
        PAD_ScanPads();
        WPAD_ScanPads();

        u32 gc_pressed = PAD_ButtonsDown(0);  // GameCube controller
        u32 wii_pressed = WPAD_ButtonsDown(0);  // Wii remote

        if (gc_pressed & PAD_BUTTON_START || wii_pressed & WPAD_BUTTON_HOME) {
            printf("\nExiting to Homebrew Channel...\n");
            VIDEO_WaitVSync();
            SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);  // Return to Homebrew Channel
        }

        VIDEO_WaitVSync();
    }
}

int main(int argc, char **argv) {
    initialize_video();

    calculate_pi();

    wait_for_exit();  // Wait for the user to press the exit button

    return 0;
}
