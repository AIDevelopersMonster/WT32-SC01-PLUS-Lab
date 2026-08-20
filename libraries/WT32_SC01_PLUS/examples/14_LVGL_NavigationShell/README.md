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

`13_LVGL_BasicUI` is already complete and physically validated. It established the BSP-to-LVGL bridge: display flush, touch pointer input, LVGL events and hardware backlight control.

`14_LVGL_NavigationShell` is the next layer. It turns that validated bridge into a reusable multi-page HMI structure without yet adding network, OTA, weather or application-specific control logic.

The shell is intentionally small enough to test independently before later modules are added.

## Pages

The first shell contains four fixed application pages:

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
SOURCE CREATED
CI TARGET ADDED
PHYSICAL VALIDATION REQUIRED
```

No `PHYSICAL PASS` is claimed yet for this example.

## Physical validation checklist

On the reference Panlee specimen verify:

- firmware boots without panic/reboot;
- HOME is the initial page;
- bottom navigation is fully visible at 480x320;
- HOME -> REMOTE -> SETTINGS -> INFO transitions work by touch;
- active button highlighting follows the selected page;
- repeated page switching does not corrupt the UI;
- REMOTE command buttons produce the expected Serial events;
- SETTINGS backlight slider changes visible brightness;
- touch remains correctly mapped near all four navigation buttons;
- no unwanted scrolling or page displacement occurs;
- no obvious redraw corruption appears during rapid switching.

Useful Serial markers:

```text
READY: LVGL navigation shell initialized
PAGE: 0
PAGE: 1
PAGE: 2
PAGE: 3
REMOTE action placeholder: N
```

## Claim boundary

Passing this example will certify the reusable LVGL navigation shell only. It will not certify Wi-Fi, OTA, weather, RS485/Modbus application actions, SD assets or QR onboarding until those are implemented and separately exercised.
