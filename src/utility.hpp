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

#define PI_DIGITS 50  // Number of decimal places of Pi
#define TOTAL_LENGTH (PI_DIGITS + 3)  // '3.' + digits + null terminator

void exit_WPCPP();
void wait_for_user_input_to_return();
void format_pi(const mpf_class &pi_value, char *pi_str, int precision);
void compare_pi_accuracy(const mpf_class &calculated_pi, int precision);
void print_mismatch(const char *calculated_str, const char *actual_str, int mismatch_index);

#endif

// EOF
