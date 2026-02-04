// src/video.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Wii Pi Calculator Project Plus (WPCPP)
// Copyright (C) 2024-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "video.hpp"
#include "utility.hpp"
#include <gccore.h>
#include <ogcsys.h>
#include <iostream>

using namespace std;  // Use the entire std namespace for simplicity

// Define a context to encapsulate video-related global state
struct VideoContext {
  void *xfb;          // Framebuffer pointer
  GXRModeObj *rmode;  // TV display mode
  bool initialized;   // Tracks whether the video system is initialized
};

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

  // Allocate memory for the framebuffer, which stores the pixels that will be shown on the screen
  video_ctx.xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(video_ctx.rmode));

  // If framebuffer allocation fails, handle the critical error gracefully
  // This is a catastrophic failure that shouldn't happen normally a real Wii
  if (!video_ctx.xfb)
  {
    cout << "Failed to allocate framebuffer!" << endl;
    exit_WPCPP();  // Call to exit and reset system
  }

  // Set up the video mode and tell the video system where the framebuffer is located
  VIDEO_Configure(video_ctx.rmode);
  VIDEO_SetNextFramebuffer(video_ctx.xfb);

  // Explicitly clear the framebuffer to black.
  // This prevents random "garbage" data from being on the screen when the program starts.
  VIDEO_ClearFrameBuffer(video_ctx.rmode, video_ctx.xfb, COLOR_BLACK);

  // Define the console's drawable area, leaving a 20px margin on all sides.
  // This prevents a buffer overflow where the console would try to draw outside the screen's memory.
  int console_x = 20;
  int console_y = 20;
  int console_width = video_ctx.rmode->fbWidth - (2 * console_x);
  int console_height = video_ctx.rmode->xfbHeight - (2 * console_y);

  // Initialize the console system to allow printing text within the safe, calculated dimensions.
  // Parameters: framebuffer, start x/y, width, height, and pitch (row size)
  console_init(video_ctx.xfb, console_x, console_y, console_width, console_height, video_ctx.rmode->fbWidth * VI_DISPLAY_PIX_SZ);

  // Disable the black screen (make display visible) and apply all video changes.
  VIDEO_SetBlack(FALSE);
  VIDEO_Flush();
  VIDEO_WaitVSync();

  // If the TV mode is non-interlaced (progressive scan), wait for another sync for stability.
  if (video_ctx.rmode->viTVMode & VI_NON_INTERLACE)
  {
    VIDEO_WaitVSync();
  }
}

// EOF
