# WPCP (Wii Pi Calculator Project)

## Description

> This repository is a continuation fork of the original version 1.1 release of WPCP (Wii Pi Calculator Project) by MadCatMk2 that calculates the digits of pi (π) on the Wii.
  It has since been modernized and extended for some minor extra functionality. The program can display accurately up to 14 decimal places of Pi using Machin's formula,
  which is more accurate and allows for slightly more precision than the original numerical integration method.

## Limitations

> As the Wii’s floating-point hardware supports only **double precision**,
  the program can only display up to **14 decimal places of Pi**. To support higher precision, external
  libraries like GMP would be required to calculate more decimal places. Although this is possible, it has yet to be implemented.


## Build Instructions

> To compile the project, install and configure devkitPPC with wii-dev installed and run `make` in the project directory. If everything is set up correctly, this should generate the `.elf` and `.dol` files.

## How to Use


> - Rename the compiled dol to boot.dol and place into the apps/WPCP folder
> - Copy the apps folder and it's contents to an SD card
> - Insert the SD card into the Wii and launch with the Homebrew Channel.
> - Wait for the calculations to finish and exit with either the reset or power button the Wii.

&nbsp;

![WPCP 1.1 Screenshot](https://github.com/DeltaResero/WPCPP/blob/main/extras/wpcp_v1-4_screenshot.png?raw=true)

&nbsp;

## Disclaimer

> This application relies on the Homebrew Channel to run. It is not affiliated with, endorsed by, nor sponsored by the creators of the Wii console nor the Homebrew Channel.
  All trademarks and copyrights are the property of their respective owners.
