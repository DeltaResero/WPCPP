// src/verify.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Wii Pi Calculator Project Plus (WPCPP)
// Copyright (C) 2024-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef VERIFY_HPP
#define VERIFY_HPP

#include <string>

// Hexadecimal digits fetched and compared per probe. A probe is reliable for
// more than this, so the spare ones are margin rather than an estimate. See
// the measurements written up at the top of verify.cpp
#define BBP_PROBE_DIGITS 8

// Decimal places one hexadecimal digit is worth, which is log10(16). Used to
// translate between the two scales in both directions
#define DECIMALS_PER_HEX_DIGIT 1.2041200

/**
 * Writes hexadecimal digits of Pi starting at a chosen place, without needing
 * any of the digits before it
 * @param position How many fractional hex digits to skip before the first one written
 * @param count How many digits to write
 * @param out Buffer receiving the digits, needing room for count plus a terminator
 */
void bbp_hex_digits(int position, int count, char *out);

/**
 * Checks the digit generator against places where the answer is already known,
 * so that a broken generator cannot quietly pass every result it is given
 * @return True when the generator reproduces the known digits
 */
bool bbp_self_test();

/**
 * Counts how many leading fractional hex digits of a value the BBP formula
 * agrees with, deriving each one it checks rather than looking it up
 * @param pi_hex Fractional hexadecimal digits of the value being checked
 * @param hex_length How many of those digits to account for
 * @return The count agreed with, which equals hex_length when all of them are
 */
int bbp_confirmed_hex_digits(const std::string &pi_hex, int hex_length);

#endif

// EOF
