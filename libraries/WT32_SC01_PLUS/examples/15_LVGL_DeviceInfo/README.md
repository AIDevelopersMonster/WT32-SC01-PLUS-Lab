# 15_LVGL_DeviceInfo

Live runtime Device Info page for the physically validated Panlee WT32-SC01-PLUS BSP.

Target specimen:

```text
Panlee WT32-SC01-PLUS
ZX3D50CE08S-V15-USRC
230208
ESP32-S3 + ST7796 480x320 + FT6336U-compatible touch
```

## Purpose

`14_LVGL_NavigationShell` established the reusable four-page HMI frame. `15_LVGL_DeviceInfo` keeps that shell and replaces the static INFO page with live values read from the running ESP32-S3.

This is the next reusable lesson carried forward from Project 4: Device Info should be useful both to the end user and as physical laboratory evidence.

## Live values

The INFO page displays:

- ESP32 chip model;
- silicon revision;
- CPU core count;
- CPU frequency;
- Arduino core version;
- ESP-IDF version;
- detected Flash size;
- detected PSRAM size;
- free/total heap;
- free PSRAM;
- sketch size;
- free application slot space;
- uptime;
- minimum free heap since boot;
- touch-controller chip and firmware IDs.

Dynamic values refresh once per second while preserving the navigation shell.

## Why this is separate from 14

The navigation shell is already a physically validated module. This example must not silently expand that claim. Device-runtime reporting is therefore introduced as its own example and gets its own physical validation status.

## Dependency

```text
LVGL 8.3.11
```

The included `build_opt.h` applies `-DLV_CONF_SKIP`, matching examples 13 and 14.

## Current status

```text
SOURCE CREATED
CI TARGET TO BE ADDED
PHYSICAL VALIDATION REQUIRED
WEB FLASHER: NOT YET ELIGIBLE
```

## Physical validation checklist

On the reference Panlee specimen verify:

- firmware boots without panic/reboot;
- HOME/REMOTE/SETTINGS/INFO navigation still works;
- INFO page renders all three data cards cleanly;
- chip model reports ESP32-S3;
- chip revision is consistent with the validated specimen;
- Flash reports 16 MiB;
- PSRAM reports 2 MiB;
- heap/free-heap values are nonzero and plausible;
- free PSRAM is nonzero;
- sketch/free-slot values are visible;
- uptime increments every second;
- minimum free heap is stable or decreases only as expected;
- touch chip/firmware IDs match the established BSP observations;
- repeated INFO page entry/exit does not corrupt the UI;
- no obvious memory leak, reset or rendering corruption appears during repeated navigation.

Expected Serial markers include:

```text
WT32-SC01-PLUS 15_LVGL_DeviceInfo
DEVICE: ESP32-S3 ...
MEMORY: Flash=16 MiB PSRAM=2 MiB ...
READY: LVGL live device info initialized
```

## Claim boundary

Passing this example will validate runtime Device Info reporting on the named specimen. It will not validate Wi-Fi, OTA, weather, QR onboarding, RS485/Modbus actions or filesystem-backed assets.

After physical PASS this example becomes eligible for the repository Web Flasher catalog.
