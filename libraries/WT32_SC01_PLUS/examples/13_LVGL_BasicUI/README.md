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

## Physical validation status

### First run — 2026-08-20

Observed on the reference Panlee specimen:

| Item | Result |
|---|---|
| LVGL graphics/rendering | **PHYSICAL PASS** |
| Screen layout | **PHYSICAL PASS** |
| Touch interaction | **FAIL** |
| Button event | NOT TESTED because touch failed |
| Backlight slider | NOT TESTED because touch failed |

The failure was traced to the example initialization sequence, not to a newly observed hardware failure. `WT32_SC01_PLUS::begin()` initializes display and backlight only; touch is intentionally initialized separately by `board.touch().begin()`. The first revision of this example omitted that call even though previously validated touch examples such as `11_RainbowTouch` include it.

The example has now been corrected to initialize touch explicitly before registering the LVGL pointer input driver. It also prints the detected touch-controller identifiers to Serial before starting the UI.

**Current status:** graphics physically passed; corrected touch path requires one repeat physical run before the example can be promoted to full PHYSICAL PASS.

## Expected UI

The screen contains:

- title and BSP identification;
- `Tap me` button;
- live `Button presses` counter;
- `Backlight` slider from 10% to 100%.

A successful repeat physical run should confirm display rendering, touch-to-LVGL coordinate mapping, button events, slider interaction, and visible backlight level changes.

## Why this example exists

ESP32-TUX demonstrates a strong reusable HMI architecture on WT32-SC01 Plus using ESP-IDF, LovyanGFX and LVGL. This example extracts only the general architectural lesson: application UI should sit above reusable board support rather than repeat board-specific pin and controller configuration in every program.

The implementation here is independently integrated with this repository's own BSP and does not require ESP32-TUX source code at runtime.
