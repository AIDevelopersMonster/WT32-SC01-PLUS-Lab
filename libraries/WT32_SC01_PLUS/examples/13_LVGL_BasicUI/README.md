# 13_LVGL_BasicUI

Minimal interactive LVGL 8 example for the physically validated Panlee WT32-SC01-PLUS BSP.

## What it demonstrates

- LVGL 8 rendering through `WT32_SC01_PLUS_Display::drawRGB565()`;
- BSP touch input exposed to LVGL as a pointer device;
- a clickable LVGL button and live press counter;
- an LVGL slider controlling the BSP backlight API;
- application code with no LCD GPIO table, touch GPIO table, LovyanGFX configuration, or direct ST7796 commands.

## Dependency

Install **LVGL 8.3.11** before compiling this example locally.

Arduino CLI:

```text
arduino-cli lib install "lvgl@8.3.11"
```

The repository CI and Web Flasher workflows install this pinned LVGL version automatically.

## Hardware scope

Validated BSP target:

```text
Panlee WT32-SC01-PLUS
ZX3D50CE08S-V15-USRC
230208
ESP32-S3
ST7796 480x320 I80
FT6336U-compatible touch
```

The example source is available before physical validation, but its hardware status must remain **NOT YET PHYSICALLY VALIDATED** until it is actually flashed and exercised on the reference specimen.

## Expected UI

The screen contains:

- title and BSP identification;
- `Tap me` button;
- live `Button presses` counter;
- `Backlight` slider from 10% to 100%.

A successful physical run should confirm display rendering, touch-to-LVGL coordinate mapping, button events, slider interaction, and visible backlight level changes.

## Why this example exists

ESP32-TUX demonstrates a strong reusable HMI architecture on WT32-SC01 Plus using ESP-IDF, LovyanGFX and LVGL. This example extracts only the general architectural lesson: application UI should sit above reusable board support rather than repeat board-specific pin and controller configuration in every program.

The implementation here is independently integrated with this repository's own BSP and does not require ESP32-TUX source code at runtime.