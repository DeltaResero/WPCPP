// menu.cpp
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

#include "menu.hpp"
#include "utility.hpp"
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <iostream>
#include "input.hpp"

using namespace std;  // Use the entire std namespace for simplicity

/**
 * Displays a menu and allows the user to navigate and select options
 * The options are navigated using Left/Right on the D-pad, and 'A' selects the option
 * @return The index of the selected option
 */
int method_selection_menu()
{
  // Array of Pi calculation methods
  string pi_methods[] = {
    "Numerical Integration",
    "Machin's Formula",
    "Ramanujan's First Series",
    "Chudnovsky Algorithm",
    "Gauss-Legendre Algorithm"
  };

  int num_methods = sizeof(pi_methods) / sizeof(pi_methods[0]);
  int selected_index = 0;

  // Track the previous state of buttons to detect state changes
  bool button_right_last = false;
  bool button_left_last = false;
  bool button_a_last = false;

  // Clear the screen and display instructions
  cout << "\x1b[2J";
  cout << "Select Pi Calculation Method:\n";
  cout << "Use Left/Right on the D-pad to navigate.\n";
  cout << "Press 'A' to confirm.\n";
  cout << "Press 'Home' on Wii Remote or 'Start' on GameCube controller to exit.\n";

  // Loop until the user selects a method or exits
  while (true)
  {
    // Poll inputs once per loop iteration to update the global input states
    poll_inputs();

    // Check if navigation buttons are pressed
    bool button_right_down = is_button_just_pressed(PAD_BUTTON_RIGHT, WPAD_BUTTON_RIGHT);
    bool button_left_down = is_button_just_pressed(PAD_BUTTON_LEFT, WPAD_BUTTON_LEFT);
    bool button_a_down = is_button_just_pressed(PAD_BUTTON_A, WPAD_BUTTON_A);

    // Navigate to the right method
    if (button_right_down && !button_right_last)
    {
      if (selected_index < num_methods - 1)  // Ensure it doesn't go out of bounds
      {
        selected_index++;  // Move to the next method
      }
    }

    // Navigate to the left method
    if (button_left_down && !button_left_last)
    {
      if (selected_index > 0)  // Ensure it doesn't go below 0
      {
        selected_index--;  // Move to the previous method
      }
    }

    // Update last button states for the next iteration
    button_right_last = button_right_down;
    button_left_last = button_left_down;

    // Display the currently selected method
    cout << "\rCurrently Selected: " << pi_methods[selected_index] << "      \r";

    // Confirm selection when 'A' button is pressed
    if (button_a_down && !button_a_last)
    {
      return selected_index;  // Return the selected method index
    }

    // Update the last state of the 'A' button
    button_a_last = button_a_down;

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
 * Displays a precision selection screen to allow the user to choose the number
 * of decimal places for the Pi calculation
 * @return The selected precision (between 1 and 50 decimal places)
 */
int precision_selection_menu()
{
  int precision = 50;  // Start with maximum precision (50 decimal places)
  int step_size = 1;   // Initial step size for adjusting precision

  // Track the previous state of buttons to detect state changes
  bool button_a_last = false;
  bool button_l_last = false;
  bool button_r_last = false;
  bool button_left_last = false;
  bool button_right_last = false;

  // Clear the screen and display instructions
  cout << "\x1b[2J";  // Clear console screen
  cout << "Select Pi Precision (1-50 decimal places):\n";
  cout << "Use Left/Right on the D-pad to adjust.\n";
  cout << "Press 'L'/'R' or '-'/'+' to change the stepping size.\n";
  cout << "Press 'A' to confirm.\n";

  // Loop until the user confirms their precision selection
  while (true)
  {
    // Poll inputs once per loop iteration to update the global input states
    poll_inputs();

    // Check if specific buttons are pressed
    bool button_left_down = is_button_just_pressed(PAD_BUTTON_LEFT, WPAD_BUTTON_LEFT);
    bool button_right_down = is_button_just_pressed(PAD_BUTTON_RIGHT, WPAD_BUTTON_RIGHT);
    bool button_l_down = is_button_just_pressed(PAD_TRIGGER_L, WPAD_BUTTON_MINUS);
    bool button_r_down = is_button_just_pressed(PAD_TRIGGER_R, WPAD_BUTTON_PLUS);
    bool button_a_down = is_button_just_pressed(PAD_BUTTON_A, WPAD_BUTTON_A);

    // Decrease step size by a factor of 10 if the L triggers or "-" button is pressed
    if (button_l_down && !button_l_last)
    {
      if (step_size > 1)  // Ensure step size doesn't go below 1
      {
        step_size /= 10;  // Reduce step size
      }
    }

    // Increase step size by a factor of 10 if the R trigger or "+" button is pressed
    if (button_r_down && !button_r_last)
    {
      if (step_size < 10)  // Ensure step size doesn't exceed 10
      {
        step_size *= 10;  // Increase step size
      }
    }

    // Decrease precision if the left D-pad button is pressed, ensuring it stays >= 1
    if (button_left_down && !button_left_last)
    {
      if (precision - step_size >= 1)  // Ensure precision doesn't go below 1
      {
        precision -= step_size;  // Decrease precision
      }
    }

    // Increase precision if the right D-pad button is pressed, ensuring it stays <= 50
    if (button_right_down && !button_right_last)
    {
      if (precision + step_size <= 50)  // Ensure precision doesn't exceed 50
      {
        precision += step_size;  // Increase precision
      }
    }

    // Update last button states for the next iteration
    button_l_last = button_l_down;
    button_r_last = button_r_down;
    button_left_last = button_left_down;
    button_right_last = button_right_down;

    // Display the current precision and step size
    cout << "\rCurrent Precision: " << precision << " decimal places  (Step Size: " << step_size << ")     \r";

    // Confirm selection when 'A' button is pressed
    if (button_a_down && !button_a_last)
    {
      return precision;  // Return the selected precision
    }

    // Update the last state of the 'A' button
    button_a_last = button_a_down;

    // Wait for video sync to ensure smooth input handling
    VIDEO_WaitVSync();
  }
}

// EOF
