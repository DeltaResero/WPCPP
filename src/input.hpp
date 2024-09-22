// input.hpp
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

#ifndef INPUT_HPP
#define INPUT_HPP

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <utility>

void initialize_inputs();
void poll_inputs();
std::pair<u32, u32> scan_inputs();
bool is_button_just_pressed(u32 gc_button, u32 wii_button);

#endif

// EOF
