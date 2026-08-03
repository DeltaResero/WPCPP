// src/results.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Wii Pi Calculator Project Plus (WPCPP)
// Copyright (C) 2024-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef RESULTS_HPP
#define RESULTS_HPP

#include "utility.hpp"
#include <string>

bool display_pi_pages(const std::string &pi_full_string, int precision,
                      const AccuracyReport &accuracy_info);

#endif

// EOF
