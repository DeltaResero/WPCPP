# WPCPP (Wii Pi Calculator Project Plus)

## Description

This repository is a continuation fork of the original version 1.1 release of
WPCP (Wii Pi Calculator Project) by MadCatMk2. It calculates the digits of pi (π)
on the Wii. It has since been rewritten in C++, modernized, and extended with
additional functionality. The program can now display up to **50 decimal places**
of Pi using with various methods via the help of .

## Limitations

As the Wii’s floating-point hardware supports only **double precision**, this
program could initially only display up to **14 decimal places of Pi**. To support
higher precision, we now use the [GNU Multiple Precision Arithmetic Library (GMP)](https://gmplib.org) for higher precision to accurately calculate more decimal places of [Pi (π)](https://en.wikipedia.org/wiki/Pi). The current implementation handles up to **50 decimal places**.

<br>

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

**1. Clone the Repository**

You must clone the repository **recursively** to download the required GMP dependency.
```bash
git clone --recursive https://github.com/DeltaResero/WPCPP.git
```

**2. Build the Project**

Navigate into the project directory and run `make`.
```bash
cd WPCPP
make
```
The first time you run `make`, the build system will automatically compile the GMP library. This may take a few minutes. Subsequent builds will be much faster and will only recompile your project's source files if they have changed. To speed up compilation you can instead run `make -j$(nproc)` to run jobs in parallel where $(nproc) is the number of processors.

The `WPCPP.dol` file will be created in the project's root directory.

## How to Use

* Copy the `apps` folder from the repository to the root of your SD card. This will create a `/apps/WPCPP/` directory on your SD card.
* Copy the compiled `WPCPP.dol` file into the `SD:/apps/WPCPP/` folder, and rename it to `boot.dol`.
* Insert the SD card into the Wii and launch the program using the Homebrew Channel.
* Follow the onscreen prompts.

<br>

![WPCPP 2.0 Screenshot](https://github.com/DeltaResero/WPCPP/blob/main/extras/wpcpp_screenshot.png?raw=true)

<br>

## Cleaning and Rebuilding

Use the cleaning targets to control what gets removed before a rebuild. If you want to do a fresh rebuild of the project sources but keep already-built third-party libraries like GMP, remove the project's build artifacts (object files, intermediate files, and the generated WPCPP.dol) with:
```
make clean
```

make distclean
Removes the project's build artifacts and any third-party libraries the build system built (for example the locally compiled GMP). If you want to force a full rebuild of everything on the next make and memove the project's build artifacts and any third-party libraries then use:
```
make distclean
```

## Updating Submodule(s)

This project includes GMP as a Git submodule. Use these commands to initialize, clone, or update submodules:

Initialize and clone submodules (use this after a fresh clone if you forgot --recursive):
```
git submodule update --init --recursive
```

Update submodules to the commits referenced by the main repository (safe, reproducible):
```
git submodule update --recursive
```

Update submodules to the latest commit on their configured branch (careful — this moves submodules to newer commits):
```
git submodule update --remote --merge --recursive
```

<br>

## Third-Party Libraries

This project includes the GMP (GNU Multiple Precision Arithmetic Library) for arbitrary-precision arithmetic. GMP is licensed under **LGPL v3**.

The library is included as a Git submodule and is automatically configured and built from source by the `Makefile` the first time you run `make`. It is not necessary to install GMP or any other dependencies manually into your devkitPro environment.

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
