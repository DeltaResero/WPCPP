# WPCPP (Wii Pi Calculator Project Plus)

## Description

> This repository is a continuation fork of the original version 1.1 release of WPCP (Wii Pi Calculator Project) by MadCatMk2 that calculates the digits of pi (π) on the Wii.
  It has since been rewritten in C++, modernized, and extended for some extra functionality. The program can display accurately up to 14 decimal places of Pi using Machin's formula,
  which is more accurate and allows for slightly more precision than the original numerical integration method.

## Limitations

> As the Wii’s floating-point hardware supports only **double precision**,
  the program can only display up to **14 decimal places of Pi**. To support higher precision, external
  libraries like GMP would be required to calculate more decimal places. Although this is possible, it has yet to be implemented.

## How to Build

### Prerequisites

* devkitPro
  * devkitPPC
  * libogc
  * wii-dev

### PowerPC devkitPro devkitPPC Toolchain and Build System Setup

To set up the devkitPro PowerPC devkitPPC toolchain and build system, follow the instructions on the official devkitPro wiki:

- [Getting Started with devkitPro](https://devkitpro.org/wiki/Getting_Started)
- [DevkitPro Pacman](https://devkitpro.org/wiki/devkitPro_pacman)

## Build Instructions

> To compile the project, install and configure devkitPPC with all prerequisites installed and run `make` in the project directory. If everything is set up correctly, this should generate the `.elf` and `.dol` files.

## How to Use


> - Rename the compiled dol to boot.dol and place into the apps/WPCPP folder
> - Copy the apps folder and it's contents to an SD card
> - Insert the SD card into the Wii and launch with the Homebrew Channel.
> - Wait for the calculations to finish and exit with either the reset or power button the Wii.

&nbsp;

![WPCPP 2.0 Screenshot](https://github.com/DeltaResero/WPCPP/blob/main/extras/wpcpp_screenshot.png?raw=true)

&nbsp;

## Disclaimer

> This application relies on the Homebrew Channel to run. It is not affiliated with, endorsed by, nor sponsored by the creators of the Wii console nor the Homebrew Channel.
  All trademarks and copyrights are the property of their respective owners.
