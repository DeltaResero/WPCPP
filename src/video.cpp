// video.cpp
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

#include "video.hpp"
#include "utility.hpp"
#include <gccore.h>
#include <ogcsys.h>
#include <iostream>

using namespace std;  // Use the entire std namespace for simplicity

// Initialize a video context globally
static VideoContext video_ctx = {nullptr, nullptr, false};  // Keep track of video state in one place

/**
 * Initializes the video system and sets up the display for the Wii
 * This function configures the video mode based on the TV format (NTSC, PAL, etc.)
 * and initializes the framebuffer for video output
 */
void initialize_video()
{
  if (video_ctx.initialized) return;  // Early return if already initialized
  video_ctx.initialized = true;  // Mark as initialized

  VIDEO_Init();  // Initialize the video system

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
    // This is reminiscent of the old keyboard error asking the user to press a key to continue
    cout << "Failed to allocate framebuffer!" << endl;
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

// EOF
