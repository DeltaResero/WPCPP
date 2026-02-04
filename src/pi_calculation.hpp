// src/pi_calculation.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Wii Pi Calculator Project Plus (WPCPP)
// Copyright (C) 2024-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef PI_CALCULATION_HPP
#define PI_CALCULATION_HPP

#include <gmpxx.h>

mpf_class calculate_pi_machin();
mpf_class calculate_pi_numerical_integration();
mpf_class calculate_pi_ramanujan(int precision);
mpf_class calculate_pi_chudnovsky(int precision);
mpf_class calculate_pi_gauss_legendre(int precision);
mpf_class calculate_pi_spigot(int precision);
mpf_class calculate_pi_bbp(int precision);
void calculate_and_display_pi(int method, int precision);

#endif

// EOF
