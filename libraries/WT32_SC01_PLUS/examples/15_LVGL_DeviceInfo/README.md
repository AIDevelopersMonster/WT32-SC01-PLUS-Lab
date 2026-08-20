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

The navigation shell is already a physically validated module. This example does not silently expand that claim. Device-runtime reporting is introduced as its own example and carries its own physical validation record.

## Dependency

```text
LVGL 8.3.11
```

The included `build_opt.h` applies `-DLV_CONF_SKIP`, matching examples 13 and 14.

## Current status

```text
SOURCE COMPLETE
CI TARGET ADDED
PHYSICAL PASS
WEB FLASHER CATALOG TARGET
```

Physical validation was completed on the reference Panlee specimen on **2026-08-20**.

## Physical validation record

The completed hardware run confirmed:

- firmware booted without panic/reboot;
- `HOME / REMOTE / SETTINGS / INFO` navigation remained operational;
- the INFO page rendered the live data cards correctly;
- the runtime page reported the ESP32-S3 device information on the physical board;
- Flash reported the expected 16 MiB class configuration;
- PSRAM reported the expected 2 MiB configuration;
- heap and PSRAM runtime values were visible and nonzero;
- sketch/free-slot information was displayed;
- uptime updated while the firmware was running;
- touch-controller information was displayed;
- repeated navigation to and from INFO remained stable;
- the Settings backlight control and the rest of the navigation shell continued to operate.

Status: **PHYSICAL PASS** for the named Panlee `ZX3D50CE08S-V15-USRC / 230208` specimen.

Expected Serial markers include:

```text
WT32-SC01-PLUS 15_LVGL_DeviceInfo
DEVICE: ESP32-S3 ...
MEMORY: Flash=16 MiB PSRAM=2 MiB ...
READY: LVGL live device info initialized
```

## Video evidence

- [YouTube Shorts — WT32-SC01-PLUS + LVGL Device Info](https://youtube.com/shorts/vlxDE6bILbU)

## Web Flasher

Because the example is now physically validated and non-destructive, it is eligible for the repository Web Flasher catalog. The Web Flasher workflow already installs `lvgl@8.3.11`, compiles catalogued BSP examples, and generates the ESP Web Tools manifests automatically.

## Claim boundary

This physical PASS validates runtime Device Info reporting on the named specimen. It does not validate Wi-Fi, OTA, weather, QR onboarding, RS485/Modbus actions or filesystem-backed assets.
