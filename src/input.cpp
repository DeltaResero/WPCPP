// input.cpp
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

#include "input.hpp"
#include <gccore.h>
#include <wiiuse/wpad.h>

// Global variables to track the state of inputs
static u32 gc_last_state = 0;  // Store the previous state for GameCube controller
static u32 wii_last_state = 0;  // Store the previous state for Wii Remote

static u32 gc_state = 0;  // Store the current state for GameCube controller
static u32 wii_state = 0;  // Store the current state for Wii Remote

/**
 * Initialize the input system for both GameCube controllers and Wii Remotes
 */
void initialize_inputs()
{
    PAD_Init();  // Initialize GameCube controller input
    WPAD_Init();  // Initialize Wii remote input
}

/**
 * Poll the input state for GameCube and Wii Remote controllers
 * This should be called in each loop iteration to update the current input state
 */
void poll_inputs()
{
    // Store the previous states before polling
    gc_last_state = gc_state;
    wii_last_state = wii_state;

    PAD_ScanPads();  // Update GameCube controller state
    WPAD_ScanPads();  // Update Wii Remote state

    // Get the current state of the buttons
    gc_state = PAD_ButtonsHeld(0);  // Buttons currently held on GameCube controller
    wii_state = WPAD_ButtonsHeld(0);  // Buttons currently held on Wii Remote

    VIDEO_WaitVSync();  // Wait for video sync to avoid input ghosting
}

/**
 * Check if a specific button was just pressed (i.e., transitioned from unpressed to pressed)
 * @param gc_button GameCube button to check
 * @param wii_button Wii Remote button to check
 * @return True if either button was just pressed, false otherwise
 */
bool is_button_just_pressed(u32 gc_button, u32 wii_button)
{
    bool gc_just_pressed = (gc_state & gc_button) && !(gc_last_state & gc_button);
    bool wii_just_pressed = (wii_state & wii_button) && !(wii_last_state & wii_button);

    return gc_just_pressed || wii_just_pressed;
}

// EOF
