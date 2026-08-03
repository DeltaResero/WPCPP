// src/menu.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Wii Pi Calculator Project Plus (WPCPP)
// Copyright (C) 2024-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "menu.hpp"
#include "utility.hpp"
#include "pi_calculation.hpp"
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <iostream>
#include <iomanip>
#include "input.hpp"

using namespace std;  // Use the entire std namespace for simplicity

/**
 * Displays a menu and allows the user to navigate and select options
 * The options are navigated using Left/Right on the D-pad, and 'A' selects the option
 * @param start_index Which method the list opens on, so that coming back to this
 *        screen lands where it was left rather than at the top every time
 * @return The index of the selected option
 */
int method_selection_menu(int start_index)
{
  const int num_methods = pi_method_count();
  int selected_index = start_index;

  // Clear the screen and display instructions
  cout << "\x1b[2J"; // ANSI escape code to clear the screen
  cout << "Select Pi Calculation Method:\n";
  cout << "Use Left/Right on the D-pad to navigate.\n";
  cout << "Press 'A' to confirm.\n";
  cout << "Press 'Home' on Wii Remote or 'Start' on GameCube controller to exit.\n";

  // Variable to hold the length of the longest string to clear
  int max_length = 50;  // Define a max length for clearing output

  // Loop until the user selects a method or exits
  while (true)
  {
    // Poll inputs once per loop iteration to update the global input states
    poll_inputs();

    // Check which buttons have just gone down. is_button_just_pressed reports only
    // the moment a button changes from released to held, so each of these is true for
    // a single pass of the loop no matter how long the button is held down
    bool button_right_down = is_button_just_pressed(PAD_BUTTON_RIGHT, WPAD_BUTTON_RIGHT);
    bool button_left_down = is_button_just_pressed(PAD_BUTTON_LEFT, WPAD_BUTTON_LEFT);
    bool button_a_down = is_button_just_pressed(PAD_BUTTON_A, WPAD_BUTTON_A);

    // Navigate to the right method
    if (button_right_down)
    {
      if (selected_index < num_methods - 1)  // Ensure it doesn't go out of bounds
      {
        selected_index++;  // Move to the next method
      }
    }

    // Navigate to the left method
    if (button_left_down)
    {
      if (selected_index > 0)  // Ensure it doesn't go below 0
      {
        selected_index--;  // Move to the previous method
      }
    }

    // Clear the previous line by overwriting it with spaces, then reprint the currently selected method
    cout << "\rCurrently Selected: " << string(max_length, ' ') << "\r";  // Clear previous line
    cout << "Currently Selected: " << pi_method_menu_name(selected_index) << "\r"; // Print new selection

    // Confirm selection when 'A' button is pressed
    if (button_a_down)
    {
      return selected_index;  // Return the selected method index
    }

    // Check if 'Home' button (Wii Remote) or 'Start' button (GameCube) is pressed to exit
    if (is_button_just_pressed(PAD_BUTTON_START, WPAD_BUTTON_HOME))
    {
      exit_WPCPP();  // Exit the program and return to the system menu
    }

    // Wait for video sync to ensure smooth input handling
    VIDEO_WaitVSync();
  }
}

/**
 * Moves the two figures on the precision screen by whatever the D-pad and the
 * triggers were just doing. Both are held inside their limits here, so the
 * screen around this never has to check what it is about to show
 * @param precision The figure being adjusted, kept between 1 and MAX_PI_DIGITS
 * @param step_size How far Left and Right move it, kept between 1 and the cap
 */
static void apply_precision_buttons(int &precision, int &step_size)
{
  bool button_left_down = is_button_just_pressed(PAD_BUTTON_LEFT, WPAD_BUTTON_LEFT);
  bool button_right_down = is_button_just_pressed(PAD_BUTTON_RIGHT, WPAD_BUTTON_RIGHT);
  bool button_l_down = is_button_just_pressed(PAD_TRIGGER_L, WPAD_BUTTON_MINUS);
  bool button_r_down = is_button_just_pressed(PAD_TRIGGER_R, WPAD_BUTTON_PLUS);

  // Cycle the step size down one power of ten
  if (button_l_down)
  {
    if (step_size > 1)
    {
      step_size /= 10;
    }
  }

  // Cycle the step size up one power of ten. The ladder stops at the cap itself,
  // since a step larger than the whole range could only ever land on the two ends.
  // Growing it with the cap is what keeps the top reachable in a few presses
  if (button_r_down)
  {
    if (step_size * 10 <= MAX_PI_DIGITS)
    {
      step_size *= 10;
    }
  }

  // Decrease precision, ensuring it stays >= 1
  if (button_left_down)
  {
    precision -= step_size;
    if (precision < 1)
    {
      precision = 1;
    }
  }

  // Increase precision, ensuring it stays within the cap
  if (button_right_down)
  {
    precision += step_size;
    if (precision > MAX_PI_DIGITS)
    {
      precision = MAX_PI_DIGITS;
    }
  }
}

/**
 * Displays a precision selection screen to allow the user to choose the number
 * of decimal places for the Pi calculation
 * @param precision Opens on this figure and is left holding whatever it was
 *        adjusted to, so that a screen backed out of still says where it got to
 * @return True when a precision was confirmed, false when the user went back to
 *         the method selection instead
 */
bool precision_selection_menu(int &precision)
{
  // The step always opens at one however large the figure above it is. A step
  // carried over from a previous visit would turn the first press of Left or
  // Right into a jump of hundreds, which is not what that press looks like
  int step_size = 1;

  // Digits in the largest number either field below can show. The status line is
  // redrawn over itself, so both numbers are padded to this width to keep the line
  // one constant length. Without that, dropping from a long number to a short one
  // would leave the tail of the old line sitting on screen
  const int field_width = static_cast<int>(to_string(MAX_PI_DIGITS).length());

  // Clear the screen and display instructions
  cout << "\x1b[2J";  // ANSI escape code to clear the screen
  cout << "Select Pi Precision (1-" << MAX_PI_DIGITS << " decimal places):\n";
  cout << "Use Left/Right on the D-pad to adjust.\n";
  cout << "Press 'L'/'R' or '-'/'+' to change the stepping size.\n";
  cout << "Press 'A' to confirm.\n";
  cout << "Press 'B' to go back to the method selection.\n";
  cout << "Press 'Home' on Wii Remote or 'Start' on GameCube controller to exit.\n";

  // Loop until the user confirms their precision selection
  while (true)
  {
    // Poll inputs once per loop iteration to update the global input states
    poll_inputs();

    // Check which buttons have just gone down. is_button_just_pressed reports only
    // the moment a button changes from released to held, so each of these is true for
    // a single pass of the loop no matter how long the button is held down
    bool button_a_down = is_button_just_pressed(PAD_BUTTON_A, WPAD_BUTTON_A);
    bool button_b_down = is_button_just_pressed(PAD_BUTTON_B, WPAD_BUTTON_B);

    apply_precision_buttons(precision, step_size);

    // Display the current precision and step size
    cout << "\rCurrent Precision: " << setw(field_width) << precision
         << " decimal places (Step Size: " << setw(field_width) << step_size << ")\r";

    // Confirm selection when 'A' button is pressed
    if (button_a_down)
    {
      return true;
    }

    // Go back a screen. The figure stays as the user left it either way, since
    // someone who backs out to change method rarely wants the digits forgotten
    if (button_b_down)
    {
      return false;
    }

    // Check if 'Home' button (Wii Remote) or 'Start' button (GameCube) is pressed to exit
    if (is_button_just_pressed(PAD_BUTTON_START, WPAD_BUTTON_HOME))
    {
      exit_WPCPP();  // Exit the program and return to the system menu
    }

    // Wait for video sync to ensure smooth input handling
    VIDEO_WaitVSync();
  }
}

// EOF
