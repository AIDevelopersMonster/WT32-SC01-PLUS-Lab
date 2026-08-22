# 19_LVGL_Orientation

Four-orientation LVGL validation example for the WT32-SC01-PLUS BSP.

Target specimen:

```text
Panlee WT32-SC01-PLUS
ZX3D50CE08S-V15-USRC
230208
ESP32-S3 + ST7796 480x320 + FT6336U-compatible touch
```

## Goal

This example validates runtime orientation changes at:

```text
0 deg
90 deg
180 deg
270 deg
```

The important part is not merely rotating graphics. A PASS requires display geometry, LVGL layout and touch hit-testing to remain correct together.

The BSP continues to expose the physically validated native landscape touch mapping:

```text
LCD_X = raw_Y
LCD_Y = 319 - raw_X
```

`19_LVGL_Orientation` then uses LVGL 8.3.11 software rotation:

```cpp
displayDriver.sw_rotate = 1;
lv_disp_set_rotation(...);
```

LVGL performs the pointer-coordinate transformation after the BSP read. The example intentionally does not add a second application-level touch transform.

## UI

The example has four pages:

```text
TEST
QR
SET
INFO
```

### TEST

Five touch targets are shown in every orientation:

```text
TL        TR

   CENTER

BL        BR
```

Each successful press turns the target into the PASS color and increments the per-orientation counter.

A complete touch pass requires:

```text
0 deg   5/5
90 deg  5/5
180 deg 5/5
270 deg 5/5
```

This is intended to expose swapped axes, mirroring, offset errors and stale logical resolutions immediately.

### QR

Displays the project URL as an LVGL QR code. It is retained specifically to check a geometry-sensitive widget after rotation.

### SET

Contains:

- `0 / 90 / 180 / 270` orientation buttons;
- Light/Dark theme switch;
- physical backlight slider;
- touch-counter reset.

Orientation, theme and brightness are saved through `Preferences` and restored after reboot.

### INFO

Reports:

- active orientation;
- current LVGL logical resolution;
- native 480x320 panel resolution;
- software-rotation state;
- Flash / PSRAM / heap / uptime;
- the four touch-target counters;
- theme and brightness.

Expected logical geometry:

```text
0 deg   -> 480x320
90 deg  -> 320x480
180 deg -> 480x320
270 deg -> 320x480
```

## Dependencies

```text
Arduino-ESP32 3.3.8
LVGL 8.3.11
WT32_SC01_PLUS BSP
```

`build_opt.h`:

```text
-DLV_CONF_SKIP
-DLV_USE_QRCODE=1
```

## Current status

```text
SOURCE COMPLETE
CI TARGET ADDED
DISPLAY 0/90/180/270: PHYSICAL PASS
TOUCH 0/90/180/270: PHYSICAL PASS
NAVIGATION / QR / THEME / BACKLIGHT: PHYSICAL PASS
ORIENTATION NVS RESTORE: PHYSICAL PASS
WEB FLASHER: CATALOGUED
```

## Physical validation record

The example was physically exercised on the reference Panlee specimen on **2026-08-20**.

The successful demonstration confirmed:

- runtime display rotation through `0 / 90 / 180 / 270` degrees;
- expected LVGL logical geometry (`480x320` landscape and `320x480` portrait);
- touch hit-testing remained aligned with the rotated UI;
- the `TL / TR / CENTER / BL / BR` validation targets operated in all four orientations;
- no additional application-level touch transform was required beyond the native BSP mapping plus LVGL rotation;
- `TEST / QR / SET / INFO` navigation remained usable after rotation;
- the project QR retained correct square geometry and remained usable;
- Light/Dark theme switching continued to operate;
- the physical backlight slider remained touch-aligned;
- orientation, theme and brightness persistence through `Preferences` / NVS remained operational;
- the saved orientation was restored after restart;
- repeated orientation changes showed no observed panic, reset loop or rendering corruption during the successful run.

### Video evidence

- [YouTube Shorts — WT32-SC01-PLUS + LVGL four-orientation display/touch validation](https://youtube.com/shorts/ttZOVsNHwy4)

## Physical validation checklist

The validation gate used for this example is:

1. Confirm the whole UI is visible and fills the expected logical screen.
2. Confirm header reports the expected resolution.
3. Open `TEST`.
4. Press `TL` and confirm only the top-left target reacts.
5. Press `TR` and confirm only the top-right target reacts.
6. Press `CENTER` and confirm the center target reacts.
7. Press `BL` and confirm only the bottom-left target reacts.
8. Press `BR` and confirm only the bottom-right target reacts.
9. Confirm the counter reaches `5/5`.
10. Navigate through `TEST / QR / SET / INFO`.
11. Confirm the QR remains square and readable.
12. Confirm Light/Dark switching works.
13. Confirm the brightness slider remains touch-aligned and changes physical backlight.
14. Confirm INFO reports the correct logical resolution.
15. Repeat orientation changes in different orders and confirm no rendering corruption, panic or stuck touch.
16. Reboot and verify that the saved orientation, theme and brightness are restored correctly.

## PASS result

```text
DISPLAY 0/90/180/270          PASS
LOGICAL RESOLUTION            PASS
TOUCH TL/TR/CENTER/BL/BR      PASS IN ALL FOUR ORIENTATIONS
NAVIGATION                    PASS
QR GEOMETRY / DECODING        PASS
THEME                         PASS
BACKLIGHT SLIDER              PASS
ORIENTATION NVS RESTORE       PASS
NO PANIC / RESET / CORRUPTION PASS
```

The physical PASS is specimen-scoped to the named Panlee `ZX3D50CE08S-V15-USRC / 230208` board. It does not by itself certify other WT32-SC01-PLUS OEM revisions.

The example is now eligible for the main library README and Web Flasher catalog. Promotion of orientation behavior into a reusable BSP-level API remains a separate design step.
