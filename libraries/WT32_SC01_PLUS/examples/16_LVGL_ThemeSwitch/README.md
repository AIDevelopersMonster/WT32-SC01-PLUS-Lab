# 16_LVGL_ThemeSwitch

Light/Dark theme switching for the WT32-SC01-PLUS LVGL HMI shell.

Target specimen:

```text
Panlee WT32-SC01-PLUS
ZX3D50CE08S-V15-USRC
230208
ESP32-S3 + ST7796 480x320 + FT6336U-compatible touch
```

## Purpose

`16_LVGL_ThemeSwitch` extends the physically validated navigation and Device Info examples with a reusable appearance layer.

The example keeps:

- `HOME / REMOTE / SETTINGS / INFO` navigation;
- live Device Info;
- touch input;
- hardware backlight control;

and adds runtime switching between:

```text
DARK <-> LIGHT
```

No Wi-Fi, cloud service, external API or persistent storage is required.

## Theme architecture

All UI colors are centralized in a `ThemePalette` structure. The current palette is selected through `ThemeMode` and used by page builders, navigation buttons, cards, labels, sliders and command buttons.

The theme switch lives on the Settings page.

Because changing the theme requires rebuilding the current page, the switch callback does not destroy its own event source directly. It schedules the rebuild with:

```cpp
lv_async_call(applyThemeAsync, nullptr);
```

This avoids deleting the LVGL object while its event callback is still executing.

## Dependency

```text
LVGL 8.3.11
```

`build_opt.h` applies:

```text
-DLV_CONF_SKIP
```

## Current status

```text
SOURCE COMPLETE
CI TARGET ADDED
PHYSICAL PASS
WEB FLASHER: ELIGIBLE, NOT YET CATALOGUED
```

Physical validation was completed on the reference Panlee specimen on **2026-08-20**.

## Physical validation record

The completed hardware run confirmed that the intended theme-switching behavior works correctly on the physical board:

- the firmware boots and renders the LVGL shell correctly;
- the initial Dark theme is usable;
- the Settings page switches the interface between Dark and Light at runtime;
- switching back and forth works correctly;
- the four-page `HOME / REMOTE / SETTINGS / INFO` navigation remains operational;
- the active navigation state remains visually distinct in both themes;
- the live Device Info page remains available after theme changes;
- the physical backlight control remains operational;
- touch interaction remains operational across the themed UI;
- no visible rendering corruption or instability was observed during the successful physical test.

Status: **PHYSICAL PASS** for the named Panlee `ZX3D50CE08S-V15-USRC / 230208` specimen.

Useful Serial markers:

```text
WT32-SC01-PLUS 16_LVGL_ThemeSwitch
THEME: DARK
THEME: LIGHT
THEME: DARK
READY: LVGL theme switch initialized
```

## Web Flasher

The example is non-destructive and has now passed physical validation, so it is eligible for the repository Web Flasher catalog. It is intentionally not marked as catalogued until the Web Flasher manifest list is updated separately.

## Claim boundary

This physical PASS certifies runtime Light/Dark appearance switching on the named specimen. It does not certify theme persistence across reboot, Wi-Fi, OTA, QR onboarding, weather, RS485/Modbus actions or filesystem-backed assets.
