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
SOURCE CREATED
CI TARGET TO BE ADDED
PHYSICAL VALIDATION REQUIRED
WEB FLASHER: NOT YET ELIGIBLE
```

## Physical validation checklist

The key qualification is not just changing settings while the firmware is running. The saved values must survive a restart.

On the reference Panlee specimen:

1. Boot the example and open `SETTINGS`.
2. Select `LIGHT`.
3. Set brightness to a clearly non-default value, for example approximately 35%.
4. Stop moving the slider and allow at least one second for the delayed NVS save.
5. Confirm Serial reports a brightness save.
6. Restart or power-cycle the board.
7. Confirm the board returns in `LIGHT` theme without manually switching it again.
8. Confirm physical backlight returns near the saved value rather than the default 80%.
9. Open `SETTINGS` and confirm the switch and slider reflect the restored values.
10. Change back to `DARK` and another brightness value, wait for save, and restart again.
11. Confirm the second pair of settings is restored.
12. Verify `HOME / REMOTE / SETTINGS / INFO` navigation still works after restoration.
13. Verify Device Info continues updating.
14. Verify no reset, panic, corrupt rendering or stuck touch occurs.

Useful Serial markers:

```text
WT32-SC01-PLUS 17_LVGL_SettingsPersistence
PREF LOAD: theme=LIGHT brightness=35
PREF SAVE: theme=DARK
PREF SAVE: brightness=60
READY: settings restored theme=LIGHT brightness=35
```

Exact brightness values depend on what the operator selects during the physical test.

## Claim boundary

Passing this example will certify persistence of the declared theme and brightness settings on the named specimen using ESP32 NVS/Preferences.

It will not certify:

- arbitrary application settings;
- NVS endurance limits;
- factory-reset behavior;
- settings migration between future schema versions;
- Wi-Fi credential persistence;
- OTA configuration;
- QR onboarding;
- RS485/Modbus settings.

After physical PASS the example becomes eligible for the main library README and Web Flasher catalog.
