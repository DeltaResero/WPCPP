// utility.hpp
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

#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <gmpxx.h>
#include <string>
#include <vector>

#define MAX_PI_DIGITS 1000
#define TOTAL_LENGTH (MAX_PI_DIGITS + 3)  // '3.' + digits + null terminator

// A class to hold the results of the accuracy comparison.
// Encapsulates the report lines and mismatch index.
class AccuracyReport
{
public:
  // Constructor initializes mismatch_index to -1 (no mismatch)
  AccuracyReport() : mismatch_index(-1) {}

  // Adds a line of text to the report
  void add_line(const std::string& line) {
    report_lines.push_back(line);
  }

  // Returns the list of report lines (read-only)
  const std::vector<std::string>& get_lines() const {
    return report_lines;
  }

  // Sets the index where the mismatch occurred
  void set_mismatch_index(int index) {
    mismatch_index = index;
  }

  // Returns the mismatch index
  int get_mismatch_index() const {
    return mismatch_index;
  }

private:
  std::vector<std::string> report_lines;
  int mismatch_index; // Character index of the first mismatch, or -1 if none.
};

void exit_WPCPP();
void wait_for_user_input_to_return();
void format_pi(const mpf_class &pi_value, char *pi_str, int precision);
AccuracyReport compare_pi_accuracy(const mpf_class &calculated_pi, int precision);

#endif

// EOF
