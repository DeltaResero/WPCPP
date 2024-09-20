# WPCPP (Wii Pi Calculator Project Plus)

## Description

This repository is a continuation fork of the original version 1.1 release of
WPCP (Wii Pi Calculator Project) by MadCatMk2. It calculates the digits of pi (π)
on the Wii. It has since been rewritten in C++, modernized, and extended with
additional functionality. The program can now display up to **50 decimal places**
of Pi using Machin's formula with the help of GMP for higher precision, which
is more accurate than the original numerical integration method.

## Limitations

As the Wii’s floating-point hardware supports only **double precision**, this
program could initially only display up to **14 decimal places of Pi**. To support
higher precision, we now use the GMP (GNU Multiple Precision Arithmetic Library)
to calculate more decimal places, and the current implementation handles
up to **50 decimal places**.

## How to Build

### Prerequisites

* devkitPro
  * devkitPPC
  * libogc
  * wii-dev

### PowerPC devkitPro devkitPPC Toolchain and Build System Setup

To set up the devkitPro PowerPC devkitPPC toolchain and build system, follow the
instructions on the official devkitPro wiki:

* [Getting Started with devkitPro](https://devkitpro.org/wiki/Getting_Started)
* [DevkitPro Pacman](https://devkitpro.org/wiki/devkitPro_pacman)

## Build Instructions

To compile the project, install and configure `devkitPPC` with all prerequisites
installed and run `make` in the project directory. If everything is set up correctly,
this should generate the `.elf` and `.dol` files.

## How to Use

* Rename the compiled `.dol` file to `boot.dol` and place it into the `apps/WPCPP` folder.
* Copy the `apps` folder and its contents to an SD card.
* Insert the SD card into the Wii and launch the program using the Homebrew Channel.
* Wait for the calculations to finish and exit using either the reset or power button
  on the Wii.

&nbsp;

![WPCPP 2.0 Screenshot](https://github.com/DeltaResero/WPCPP/blob/main/extras/wpcpp_screenshot.png?raw=true)

&nbsp;

## Third-Party Libraries

This project includes the GMP (GNU Multiple Precision Arithmetic Library) `v6.3.0`
for arbitrary-precision arithmetic. GMP is licensed under **LGPL v3**.

The precompiled static libraries and header files for GMP can be found in the
`third_party/gmp/lib` and `third_party/gmp/include` directories, respectively.

For more information about GMP, visit the official website: <https://gmplib.org/>.

## Disclaimer

This project is an application that runs on the Wii via the Homebrew Channel. It is not
affiliated with, endorsed by, nor sponsored by the creators of the Wii console nor the
Homebrew Channel. All trademarks and copyrights are the property of their respective owners.

This program is distributed under the terms of the GNU General Public License version 3 (GPLv3). You can
redistribute it and/or modify it under the terms of GPLv3 as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but **WITHOUT ANY WARRANTY**; without
even the implied warranty of **MERCHANTABILITY** or **FITNESS FOR A PARTICULAR PURPOSE**. See the
[GNU General Public License](https://www.gnu.org/licenses/gpl-3.0.en.html) for more details.
