// src/results.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Wii Pi Calculator Project Plus (WPCPP)
// Copyright (C) 2024-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Puts a finished result on the television. None of this knows how the digits
// were arrived at, only how many there are and which of them held up, so the
// screen can be changed without going near the arithmetic and the other way
// about.
//
// A result runs to more digits than any TV mode can show at once, so it is
// paged. The page size is worked out from the console's own idea of its size
// rather than assumed, since the three TV modes differ in height.

#include "results.hpp"
#include "utility.hpp"
#include "input.hpp"
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <iostream>
#include <string>

using namespace std;  // Use the entire std namespace for simplicity

// Drawing one page is a job of its own, defined below the loop that drives it
static void draw_result_page(const string &pi_full_string,
                             const AccuracyReport &accuracy_info,
                             int current_page, int total_pages,
                             int digits_per_page);

/**
 * Works out how many digits of Pi one screenful holds
 * @return The number of digits a page carries
 */
static int digits_that_fit_one_page()
{
  // Rows a page of digits cannot use: up to three lines of accuracy report, the blank
  // line and separator above the digits, the closing rule and blank line below them,
  // the page counter, the scroll hint, the closing prompt, and one row of slack so
  // that the closing newline does not scroll the report off the top of the screen.
  // This counts each of those as a single row, which holds only while none of them
  // wrap. The widest are the closing prompt at 59 characters and the verdict line at
  // 62 once both digit counts reach eight figures, so a console narrower than 62
  // columns would need more rows reserved than this
  const int reserved_rows = 11;

  // A page that carries on to either side shows an ellipsis there, which needs room.
  // The first page spends two of these on the "3." it carries ahead of its digits
  const int ellipsis_room = 6;

  // Ask the console how big it is rather than assuming a size. The three TV modes give
  // three different heights, so any fixed page size is wrong on at least two of them.
  // A page larger than the screen scrolls the accuracy report away before it is read
  int console_cols = 0;
  int console_rows = 0;
  CON_GetMetrics(&console_cols, &console_rows);

  // Fall back to the smallest of the three TV modes if the console reports nothing
  // usable, so that the page size below can never come out as zero or negative
  if (console_cols < 20) { console_cols = 75; }
  if (console_rows <= reserved_rows) { console_rows = 27; }

  // Pages are counted in digits rather than characters so that the "3." at the front
  // cannot push the last digit of an otherwise exact page over onto a page of its own
  const int screen_digits = ((console_rows - reserved_rows) * console_cols) - ellipsis_room;

  // Hold pages to a round thousand wherever the screen allows it, so that the page
  // number says which digits are on it: page three starts at digit 2001 whatever the
  // TV mode. Letting the screen decide instead would split ten thousand digits into
  // nine pages on NTSC and eight on PAL, with no page starting on a round number
  return screen_digits < 1000 ? screen_digits : 1000;
}

/**
 * Shows the digits a screenful at a time, letting the user page through them
 * @param pi_full_string The full result, "3." and every digit asked for
 * @param precision The number of decimal places the result carries
 * @param accuracy_info The report shown above the digits, which also says where
 *        the first wrong digit is so the rest can be coloured
 * @return True when the user went back a screen rather than on to the method list
 */
bool display_pi_pages(const string &pi_full_string, int precision,
                             const AccuracyReport &accuracy_info)
{
  const int digits_per_page = digits_that_fit_one_page();

  int total_pages = (precision + digits_per_page - 1) / digits_per_page;
  if (total_pages == 0) { total_pages = 1; }
  int current_page = 0;
  bool needs_redraw = true;

  while (true)
  {
    // Read both controllers through the shared input module. Reading the Wii Remote
    // directly here would leave the module holding a stale idea of which buttons are
    // down, and the next menu would then mistake a button still being held for a fresh
    // press. It would also leave the GameCube controller unread and therefore dead
    poll_inputs();

    // Turn the page
    if (is_button_just_pressed(PAD_BUTTON_RIGHT, WPAD_BUTTON_RIGHT))
    {
      if (current_page < total_pages - 1)
      {
        current_page++;
        needs_redraw = true;
      }
    }
    if (is_button_just_pressed(PAD_BUTTON_LEFT, WPAD_BUTTON_LEFT))
    {
      if (current_page > 0)
      {
        current_page--;
        needs_redraw = true;
      }
    }

    // Leave the program, matching what the menus do with these same buttons
    if (is_button_just_pressed(PAD_BUTTON_START, WPAD_BUTTON_HOME))
    {
      exit_WPCPP();
    }

    // Done with this result altogether, so on to the method list
    if (is_button_just_pressed(PAD_BUTTON_A, WPAD_BUTTON_A))
    {
      return false;
    }

    // Back a screen, to the precision this result was worked out to
    if (is_button_just_pressed(PAD_BUTTON_B, WPAD_BUTTON_B))
    {
      return true;
    }

    if (needs_redraw)
    {
      draw_result_page(pi_full_string, accuracy_info, current_page, total_pages,
                       digits_per_page);
      needs_redraw = false;
    }

    // Wait for video sync to ensure smooth input handling
    VIDEO_WaitVSync();
  }
}

/**
 * Prints the digits belonging to one page, wrong ones in red
 * @param pi_full_string The full result, "3." and every digit asked for
 * @param accuracy_info The report, which says where the first wrong digit is
 * @param current_page The page to print, counted from zero
 * @param total_pages How many pages the result comes to
 * @param digits_per_page How many digits a page holds
 */
static void print_page_digits(const string &pi_full_string,
                              const AccuracyReport &accuracy_info,
                              int current_page, int total_pages,
                              int digits_per_page)
{
  // A digit sits two characters further along than its own position, since "3."
  // comes first. The opening page takes that prefix with it rather than counting
  // it against its own digit budget
  int start_pos = (current_page * digits_per_page) + 2;
  int page_length = digits_per_page;
  if (current_page == 0)
  {
    start_pos = 0;
    page_length = digits_per_page + 2;
  }

  const string page_digits = pi_full_string.substr(start_pos, page_length);
  const int mismatch_index = accuracy_info.get_mismatch_index();
  string page_text = page_digits;
  int mismatch_on_page = mismatch_index - start_pos;

  // Mark a page that carries on to either side. An ellipsis going in front of the
  // digits pushes every one of them along, the mismatch included
  if (total_pages > 1)
  {
    if (current_page > 0)
    {
      page_text.insert(0, "...");
      mismatch_on_page += 3;
    }
    if (current_page < total_pages - 1)
    {
      page_text += "...";
    }
  }

  // Where the red starts. A mismatch that falls ahead of this page leaves every
  // digit on it wrong, and one past the end of the page leaves them all right
  int red_from = -1;
  if (mismatch_index >= 0
      && mismatch_index < start_pos + static_cast<int>(page_digits.length()))
  {
    red_from = mismatch_index < start_pos ? 0 : mismatch_on_page;
  }

  const string red = "\x1b[31m";
  const string reset_color = "\x1b[37m";

  if (red_from < 0)
  {
    cout << page_text << endl;
  }
  else
  {
    cout << page_text.substr(0, red_from) << red << page_text.substr(red_from)
         << reset_color << endl;
  }
}

/**
 * Draws one page of digits, with the accuracy report above and the controls
 * below. Everything on screen is rewritten, so the caller only calls this when
 * something has actually changed
 * @param pi_full_string The full result, "3." and every digit asked for
 * @param accuracy_info The report to print above the digits
 * @param current_page The page to draw, counted from zero
 * @param total_pages How many pages the result comes to
 * @param digits_per_page How many digits a page holds
 */
static void draw_result_page(const string &pi_full_string,
                             const AccuracyReport &accuracy_info,
                             int current_page, int total_pages,
                             int digits_per_page)
{
  cout << "\x1b[2J";

  // Print the Accuracy Report Header
  for (const auto& line : accuracy_info.get_lines())
  {
    cout << line << endl;
  }

  // Print a separator
  cout << endl << "--- Full Result ---" << endl;

  print_page_digits(pi_full_string, accuracy_info, current_page, total_pages,
                    digits_per_page);

  // Close the digits off with a rule as wide as the header above them, then a
  // blank line, so the result does not run straight into the controls. Plain
  // dashes rather than an "end of result" marker, since on any page but the
  // last one the result carries on
  cout << "-------------------" << endl << endl;

  // Print the Footer
  if (total_pages > 1)
  {
    cout << "Page " << (current_page + 1) << " of " << total_pages << endl;
    cout << "Use D-Pad Left/Right to scroll." << endl;
  }

  cout << "Press 'A' for methods, 'B' for precision, Home/Start exits." << endl;
}

// EOF
