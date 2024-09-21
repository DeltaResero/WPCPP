// video.hpp
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

#ifndef VIDEO_HPP
#define VIDEO_HPP

#include <gccore.h>  // For GXRModeObj and video functions from libogc

// Define a context to encapsulate video-related global state
struct VideoContext{
  void *xfb;  // Framebuffer pointer
  GXRModeObj *rmode;  // TV display mode
  bool initialized;  // Tracks whether the video system is initialized
};

void initialize_video();

#endif

// EOF
