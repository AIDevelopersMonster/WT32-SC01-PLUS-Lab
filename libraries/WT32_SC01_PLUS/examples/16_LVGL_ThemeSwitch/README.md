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
SOURCE CREATED
CI TARGET TO BE ADDED
PHYSICAL VALIDATION REQUIRED
WEB FLASHER: NOT YET ELIGIBLE
```

## Physical validation checklist

On the reference Panlee specimen verify:

- firmware boots without panic/reboot;
- initial theme is Dark;
- navigation works in Dark mode;
- Settings shows `Dark / Light` theme control;
- switching to Light immediately redraws the current page;
- Light mode has readable text, cards, controls and navigation;
- switching back to Dark works repeatedly;
- active navigation highlighting remains correct in both themes;
- HOME, REMOTE, SETTINGS and INFO render correctly in both themes;
- Device Info continues updating after multiple theme changes;
- backlight slider still controls physical brightness;
- Remote command buttons remain touch-operable;
- no reset, panic, visual corruption or stuck touch occurs during repeated switching.

Useful Serial markers:

```text
WT32-SC01-PLUS 16_LVGL_ThemeSwitch
THEME: DARK
THEME: LIGHT
THEME: DARK
READY: LVGL theme switch initialized
```

## Claim boundary

Passing this example certifies runtime Light/Dark appearance switching on the named specimen. It does not certify theme persistence across reboot, Wi-Fi, OTA, QR onboarding, weather, RS485/Modbus actions or filesystem-backed assets.

After physical PASS the example becomes eligible for the main library README and repository Web Flasher catalog.
