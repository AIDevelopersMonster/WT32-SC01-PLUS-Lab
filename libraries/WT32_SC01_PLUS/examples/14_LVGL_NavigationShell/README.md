# 14_LVGL_NavigationShell

Reusable four-page LVGL navigation shell for the physically validated Panlee WT32-SC01-PLUS BSP.

Target specimen:

```text
Panlee WT32-SC01-PLUS
ZX3D50CE08S-V15-USRC
230208
ESP32-S3 + ST7796 480x320 + FT6336U-compatible touch
```

## Purpose

`13_LVGL_BasicUI` is complete and physically validated. It established the BSP-to-LVGL bridge: display flush, touch pointer input, LVGL events and hardware backlight control.

`14_LVGL_NavigationShell` is the next layer. It turns that validated bridge into a reusable multi-page HMI structure without yet adding network, OTA, weather or application-specific control logic.

The shell is intentionally small enough to test independently before later modules are added.

## Pages

The shell contains four fixed application pages:

- `HOME` — landing/dashboard area;
- `REMOTE` — six generic command slots with callback events;
- `SETTINGS` — first reusable setting, physical backlight control;
- `INFO` — board/application information area.

A persistent bottom navigation bar remains visible while the page content changes.

## Architecture

```text
Arduino application
       |
       +-- page builders
       +-- showPage(PageId)
       +-- navigation event handler
       |
       v
      LVGL 8
       |
       v
WT32_SC01_PLUS BSP
       |
       +-- ST7796 display
       +-- touch controller
       +-- backlight PWM
```

The shell keeps board-specific GPIO and controller details inside the BSP. No separate ST7796 or touch pin table is duplicated here.

## Why it is derived from Project 4 lessons

The physically validated ESP32-TUX Project 4 demonstrated the value of a stable application frame with persistent navigation and independently populated pages such as Home, Remote, Settings and Device Info.

This example does **not** copy ESP32-TUX as a new core. It independently implements the architectural lesson on top of the lab's own Arduino BSP and the already validated `13_LVGL_BasicUI` bridge.

This separation lets later features be added one by one and physically certified independently.

## Central navigation API

The core function is:

```cpp
void showPage(PageId page);
```

It:

1. records the new active page;
2. clears only the content area;
3. invokes the page-specific builder;
4. refreshes the active navigation-button style;
5. reports the page transition through Serial.

The navigation bar itself is not destroyed on every page switch.

## Extension model

Future modules should extend the shell rather than rebuild navigation from scratch.

Planned independent additions may include:

- richer Settings;
- Device Info runtime values;
- theme switching;
- rotation;
- Wi-Fi state/setup;
- timezone/NTP;
- QR lifecycle;
- Remote callbacks to GPIO/RS485/Modbus;
- filesystem-backed LVGL assets;
- OTA UI and controlled OTA test path.

Each should retain its own validation status. A working navigation shell must not be used as evidence that future networking or control features work.

## Dependency

```text
LVGL 8.3.11
```

`build_opt.h` applies:

```text
-DLV_CONF_SKIP
```

for the complete Arduino build, matching the already validated `13_LVGL_BasicUI` configuration approach.

## Current status

```text
SOURCE COMPLETE
CI TARGET ADDED
PHYSICAL PASS
WEB FLASHER CATALOG TARGET
```

Physical validation was completed on the reference Panlee specimen on **2026-08-20**.

## Physical validation record

The completed hardware run confirmed the intended navigation-shell behavior on the physical display and touch panel:

- firmware booted and rendered the LVGL shell correctly;
- `HOME` appeared as the initial page;
- the persistent bottom navigation bar rendered correctly at 480x320;
- `HOME`, `REMOTE`, `SETTINGS` and `INFO` were reachable by touch;
- active-page highlighting followed page changes;
- repeated navigation remained stable without visible redraw corruption;
- `REMOTE` command controls were touch-operable and generated their placeholder actions;
- the `SETTINGS` backlight slider remained interactive and controlled physical display brightness;
- touch mapping remained usable across the navigation bar and page controls;
- the resulting UI was visually suitable as the reusable base shell for later HMI modules.

Status: **PHYSICAL PASS** for the named Panlee `ZX3D50CE08S-V15-USRC / 230208` specimen.

Useful Serial markers include:

```text
READY: LVGL navigation shell initialized
PAGE: 0
PAGE: 1
PAGE: 2
PAGE: 3
REMOTE action placeholder: N
```

## Web Flasher

Because this example is now physically validated, it is eligible for the repository Web Flasher catalog. The Web Flasher build workflow installs `lvgl@8.3.11`, compiles catalogued sketches from the BSP examples directory, and generates the ESP Web Tools manifests automatically.

The destructive SD full-media test remains intentionally excluded from the Web Flasher; this navigation shell is a normal non-destructive interactive demo.

## Claim boundary

This physical PASS certifies the reusable LVGL navigation shell only. It does not certify Wi-Fi, OTA, weather, RS485/Modbus application actions, SD assets or QR onboarding until those are implemented and separately exercised.
