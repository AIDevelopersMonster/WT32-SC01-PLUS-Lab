# WT32_SC01_PLUS Arduino BSP

Experimental Arduino board-support library for the WT32-SC01-PLUS family.

## v0.1 scope

The first bring-up target is the physically investigated specimen:

- Manufacturer marking: **Panlee**
- PCB marking: **ZX3D50CE08S-V15-USRC**
- Lot/date marking: **230208**
- MCU family: ESP32-S3
- LCD path: ST7796-class, 480x320 landscape, 8-bit Intel 8080

This is intentionally **not** a copy of the factory firmware. Factory reverse engineering and laboratory tests are used only as hardware evidence for a clean Arduino BSP.

The current v0.1-dev slice implements:

- board profile identity;
- BoardInfo reporting;
- LCD I80 initialization and RGB565 drawing;
- PWM backlight control;
- `01_DisplayTest` visual diagnostic.

Touch, SD, audio, RS485 and the combined SelfTest are planned as subsequent physically validated increments.

## Arduino IDE

Copy `libraries/WT32_SC01_PLUS` into your Arduino libraries directory, restart Arduino IDE, and open:

`File -> Examples -> WT32_SC01_PLUS -> 01_DisplayTest`

Select an ESP32-S3 target compatible with the board, enable PSRAM where appropriate, compile, upload, and open Serial Monitor at 115200 baud.

## Display PASS criteria

A physical PASS requires all of the following:

1. black, white, red, green and blue screens are correct;
2. the combined pattern shows clean color bars and a smooth grayscale;
3. geometry is stable with no persistent missing region or gross flicker;
4. backlight visibly changes through 100% -> 10% -> 50% -> 100%;
5. serial output reports successful initialization.

A compile PASS is not a hardware PASS.

## Hardware profile warning

The pin mapping in `WT32_SC01_PLUS_Pins.h` is for the Panlee `ZX3D50CE08S-V15-USRC / 230208` profile under investigation. Do not assume that every OEM board sold as WT32-SC01-PLUS uses the same mapping.

## Safety boundary

The Arduino BSP does not reproduce factory-only destructive or fixture-oriented operations. In particular, the factory USB connect/disconnect test that manipulates GPIO19/20 is not part of the normal SelfTest design.
