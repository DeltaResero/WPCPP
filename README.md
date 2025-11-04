# WPCPP (Wii Pi Calculator Project Plus)

## Description

This repository is a continuation fork of the original version 1.1 release of
WPCP (Wii Pi Calculator Project) by MadCatMk2. It calculates the digits of pi (π)
on the Wii. It has since been rewritten in C++, modernized, and extended with
additional functionality.

## Limitations

As the Wii’s floating-point hardware supports only **double precision**, this
program could initially only display up to **14 decimal places of Pi**. To support
higher precision, we now use the [GNU Multiple Precision Arithmetic Library (GMP)](https://gmplib.org)
to accurately calculate up to **50 decimal places** of [Pi (π)](https://en.wikipedia.org/wiki/Pi).

<br>

## How to Build

### Prerequisites

You must have a functional devkitPro environment set up with `devkitPPC`, `wii-dev`,
and `libogc`. For instructions, please follow the official [devkitPro Getting Started guide](https://devkitpro.org/wiki/Getting_Started).

### 1. Clone the Repository

To clone the project and its GMP dependency, run the following command. The `--recursive` 
flag is essential as it tells Git to download the required GMP submodule automatically.

```bash
git clone --recursive https://github.com/DeltaResero/WPCPP.git
```

> **Note:** If you cloned without the `--recursive` flag, navigate into the project
> directory and run `git submodule update --init --recursive` to fetch the dependency.

### 2. Build the Project

Navigate into the project directory and run `make`.

```bash
cd WPCPP
make
```

The first time you run `make`, the build system will automatically compile the GMP library. This may take several minutes. Subsequent builds will be much faster. To speed up compilation, you can run jobs in parallel by replacing `make` with `make -j$(nproc)` (on Linux/macOS).

The final `WPCPP.dol` file will be created in the project's root directory.

## How to Use

1.  Copy the `apps` folder from this repository to the root of your SD card. This will create a `/apps/WPCPP/` directory.
2.  Copy the compiled `WPCPP.dol` file into the `SD:/apps/WPCPP/` folder and **rename it to `boot.dol`**.
3.  Insert the SD card into your Wii and launch the program from the Homebrew Channel.
4.  Follow the onscreen prompts.

<br>

![WPCPP 2.0 Screenshot](https://github.com/DeltaResero/WPCPP/blob/main/extras/wpcpp_screenshot.png?raw=true)

<br>

## Project Maintenance

### Cleaning Build Files

To clean the project for a fresh rebuild, the `Makefile` provides two targets:

*   **`make clean`**
    Removes only the project's build files (`.o`, `.dol`). This is useful for a quick
    rebuild of the main application without touching the GMP library.

*   **`make distclean`**
    Removes all project build files **and** the compiled GMP library. Use this command
    to force a complete, fresh rebuild of everything from scratch.

### Updating Your Local Copy

If you have already cloned the project and want to pull the latest changes from this repository, you must run two commands. The first pulls the main project updates, and the second ensures the GMP submodule is updated to match what the project specifies.

```
git pull
git submodule update --init --recursive
```

<br>

## Third-Party Libraries

This project includes the GMP (GNU Multiple Precision Arithmetic Library)
for arbitrary-precision arithmetic. GMP is licensed under **LGPL v3**.

The library is included as a Git submodule and is automatically configured and
built from source by the `Makefile` the first time you run `make`. It's not
necessary to install GMP or any other dependencies manually into your
devkitPro environment.

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
