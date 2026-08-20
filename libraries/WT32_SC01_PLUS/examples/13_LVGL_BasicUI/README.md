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

## Physical validation — 2026-08-20

The corrected example was compiled, uploaded and exercised on the reference Panlee specimen.

Observed result:

| Item | Result |
|---|---|
| LVGL graphics/rendering | **PHYSICAL PASS** |
| Screen layout | **PHYSICAL PASS** |
| Touch-to-LVGL pointer input | **PHYSICAL PASS** |
| `Tap me` button events | **PHYSICAL PASS** |
| Live button counter | **PHYSICAL PASS** |
| Backlight slider interaction | **PHYSICAL PASS** |
| Visible brightness change | **PHYSICAL PASS** |

Physical observations included repeated button activation with the on-screen counter advancing from 7 to 8 and successful slider movement across multiple positions with corresponding backlight-level changes.

### Initial touch failure and fix

The first physical run rendered the LVGL UI correctly but touch did not respond. The failure was traced to the example initialization sequence rather than the hardware or BSP touch driver.

`WT32_SC01_PLUS::begin()` initializes display and backlight only. Touch is intentionally initialized separately by:

```cpp
board.touch().begin();
```

The first revision of `13_LVGL_BasicUI` omitted that call. Previously validated examples such as `11_RainbowTouch` already initialized touch explicitly. After adding `board.touch().begin()` before registering the LVGL pointer driver, the same physical specimen passed touch, button and slider interaction.

The corrected example also prints detected touch-controller identifiers to Serial before starting the UI.

**Final status: PHYSICAL PASS** for the named Panlee `ZX3D50CE08S-V15-USRC / 230208` specimen.

## Expected UI

The screen contains:

- title and BSP identification;
- `Tap me` button;
- live `Button presses` counter;
- `Backlight` slider from 10% to 100%.

## Why this example exists

ESP32-TUX demonstrates a strong reusable HMI architecture on WT32-SC01 Plus using ESP-IDF, LovyanGFX and LVGL. This example extracts only the general architectural lesson: application UI should sit above reusable board support rather than repeat board-specific pin and controller configuration in every program.

The implementation here is independently integrated with this repository's own BSP and does not require ESP32-TUX source code at runtime.
