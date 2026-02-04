// src/input.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Wii Pi Calculator Project Plus (WPCPP)
// Copyright (C) 2024-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef INPUT_HPP
#define INPUT_HPP

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <utility>

void initialize_inputs();
void poll_inputs();
bool is_button_just_pressed(u32 gc_button, u32 wii_button);

#endif

// EOF
