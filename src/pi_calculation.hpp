// pi_calculation.hpp
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

#ifndef PI_CALCULATION_HPP
#define PI_CALCULATION_HPP

#include <gmpxx.h>

mpf_class calculate_pi_machin();
mpf_class calculate_pi_numerical_integration();
mpf_class calculate_pi_ramanujan();
mpf_class calculate_pi_chudnovsky();
mpf_class calculate_pi_gauss_legendre();
mpf_class calculate_pi_spigot(int precision);
mpf_class calculate_pi_bbp();
void calculate_and_display_pi(int method, int precision);

#endif

// EOF
