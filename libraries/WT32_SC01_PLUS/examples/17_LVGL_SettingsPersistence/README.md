# 17_LVGL_SettingsPersistence

Persistent Light/Dark theme and backlight settings for the WT32-SC01-PLUS LVGL HMI shell.

Target specimen:

```text
Panlee WT32-SC01-PLUS
ZX3D50CE08S-V15-USRC
230208
ESP32-S3 + ST7796 480x320 + FT6336U-compatible touch
```

## Purpose

`17_LVGL_SettingsPersistence` extends the physically validated `16_LVGL_ThemeSwitch` example with non-volatile UI state.

The example stores two user settings in ESP32 NVS through Arduino `Preferences`:

```text
theme       DARK / LIGHT
brightness  10..100 %
```

After restart or power cycle, the saved theme and physical backlight level are restored before the LVGL shell is presented.

No Wi-Fi, cloud service or external API is required.

## Persistence model

Preferences namespace:

```text
wt32ui
```

Keys:

```text
theme
bright
```

Theme changes are infrequent and are committed immediately.

Brightness changes can produce many slider events, so the example deliberately delays the NVS write for 900 ms after the last change. This prevents a Flash write for every slider step while still persisting the final user-selected value.

## Retained HMI features

The example keeps the validated architecture from examples 14-16:

- `HOME / REMOTE / SETTINGS / INFO` navigation;
- Light/Dark appearance switching;
- live Device Info;
- touch input;
- physical backlight control;
- Remote placeholder actions.

The INFO page additionally reports the current persisted UI state.

## Dependency

```text
LVGL 8.3.11
```

`build_opt.h` applies:

```text
-DLV_CONF_SKIP
```

`Preferences` is provided by the ESP32 Arduino core; no extra library install is required for it.

## Current status

```text
SOURCE COMPLETE
CI TARGET ADDED
PHYSICAL PASS
WEB FLASHER CATALOGUED
```

Physical validation was completed on the reference Panlee specimen on **2026-08-20**.

## Physical validation record

The physical test confirmed the persistence requirement, not merely runtime UI changes:

- the LVGL shell booted and remained touch-operable;
- Light/Dark theme changes worked as in example 16;
- a non-default brightness value was applied through the physical backlight control;
- the changed brightness was committed after the delayed NVS save interval;
- the board was restarted/power-cycled after changing settings;
- the previously selected theme was restored automatically after boot;
- the previously selected brightness was restored automatically after boot;
- the Settings controls reflected the restored state;
- a second settings combination could be stored and restored on a subsequent restart;
- `HOME / REMOTE / SETTINGS / INFO` navigation remained operational after restoration;
- live Device Info continued updating;
- no reset loop, panic, corrupt rendering or stuck touch was observed during the successful test.

Status: **PHYSICAL PASS** for persistence of the declared theme and brightness settings on the named Panlee `ZX3D50CE08S-V15-USRC / 230208` specimen.

Useful Serial markers include:

```text
WT32-SC01-PLUS 17_LVGL_SettingsPersistence
PREF LOAD: theme=LIGHT brightness=35
PREF SAVE: theme=DARK
PREF SAVE: brightness=60
READY: settings restored theme=LIGHT brightness=35
```

Exact brightness values depend on what the operator selects during the physical test.

## Web Flasher

The example is physically validated, non-destructive and included in the repository Web Flasher catalog. The Web Flasher workflow builds the sketch and generates the ESP Web Tools manifest automatically.

Installing/flashing the example does not itself certify arbitrary NVS migration behavior; the validated claim is the declared theme/brightness persistence behavior exercised on the named specimen.

## Claim boundary

This physical PASS certifies persistence of the declared theme and brightness settings on the named specimen using ESP32 NVS/Preferences.

It does not certify:

- arbitrary application settings;
- NVS endurance limits;
- factory-reset behavior;
- settings migration between future schema versions;
- Wi-Fi credential persistence;
- OTA configuration;
- QR onboarding;
- RS485/Modbus settings.
