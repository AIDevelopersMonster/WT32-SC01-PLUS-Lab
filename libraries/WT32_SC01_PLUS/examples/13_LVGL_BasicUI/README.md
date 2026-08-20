# 13_LVGL_BasicUI

Minimal interactive LVGL 8 example for the physically validated Panlee WT32-SC01-PLUS BSP.

## Project credits

- **Project lead, hardware work and physical validation:** Alex Malachevsky
- **Engineering collaboration:** Commander Sol
- **Repository:** `AIDevelopersMonster/WT32-SC01-PLUS-Lab`
- **License:** MIT

## Video demonstration

Physical demonstration of the working LVGL UI on the reference Panlee board:

- [YouTube Shorts — WT32-SC01-PLUS + LVGL 8 Basic UI](https://youtube.com/shorts/1nZqa2jilpw)

The video demonstrates the already validated application path: LVGL rendering, touch input, button events with the live counter, and the backlight slider controlling real hardware.

## What it demonstrates

- LVGL 8 rendering through `WT32_SC01_PLUS_Display::drawRGB565()`;
- BSP touch input exposed to LVGL as a pointer device;
- a clickable LVGL button and live press counter;
- an LVGL slider controlling the BSP backlight API;
- application code with no LCD GPIO table, touch GPIO table, LovyanGFX configuration, or direct ST7796 commands.

## How the working sketch is structured

The application is intentionally split into four simple layers:

```text
Arduino application
       -> LVGL widgets and events
       -> WT32_SC01_PLUS BSP
       -> ST7796 display / FT6336U-compatible touch / backlight PWM
```

`flushDisplay()` receives an RGB565 rectangle rendered by LVGL and sends it to the display through the BSP `drawRGB565()` method.

`readTouch()` reads the already mapped BSP touch point and exposes it to LVGL as a normal pointer device.

`onButton()` reacts to `LV_EVENT_CLICKED`, increments application state, and updates the live counter label.

`onBrightness()` reads the LVGL slider value and passes it directly to:

```cpp
board.backlight().set(value);
```

That makes the example a minimal demonstration of a general HMI pattern: an LVGL widget can control real board hardware through a clean BSP API.

## What can be built from these elements

The same building blocks can be expanded into:

- device settings menus;
- industrial HMI pages;
- smart-home panels;
- greenhouse and climate controllers;
- RS485 / Modbus control panels;
- power, speed, temperature and actuator controls;
- Wi-Fi configuration screens;
- dashboards and status pages;
- embedded instruments and touch terminals.

Later examples in the library build on this same architecture rather than repeating board-specific display and touch setup.

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
