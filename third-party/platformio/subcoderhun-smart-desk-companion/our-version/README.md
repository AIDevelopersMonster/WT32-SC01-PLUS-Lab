# Panlee adaptation

Based on [SubCoderHUN/WT32-SC01-PLUS](https://github.com/SubCoderHUN/WT32-SC01-PLUS), originally licensed under Apache License 2.0. Modified for Panlee WT32-SC01-PLUS / `ZX3D50CE08S-V15-USRC`.

See [PANLEE_16MB_SETUP.md](PANLEE_16MB_SETUP.md) for the reproducible build/upload procedure and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for dependency-license notes.

## Physical demo video

YouTube Shorts:

[https://youtube.com/shorts/4kxUpJS4kCk](https://youtube.com/shorts/4kxUpJS4kCk)

The video physically demonstrates the named Panlee board running the adapted 16 MB flash build configuration, including LCD/LVGL, touch, UI navigation, brightness control, Wi-Fi, OpenWeatherMap weather for `Moscow`, and online radio playback through I2S.

## Validation status

| Item | Status |
|---|---|
| PlatformIO build | PASS |
| Upload | PASS |
| Boot | PHYSICAL PASS |
| ST7796 / LVGL | PHYSICAL PASS |
| Touch | PHYSICAL PASS |
| UI navigation | PHYSICAL PASS |
| Brightness | PHYSICAL PASS |
| Wi-Fi | PHYSICAL PASS |
| Weather / Moscow | PHYSICAL PASS |
| Online radio / I2S | PHYSICAL PASS |
| 16 MB build profile | PASS |
| Moscow timezone | NEEDS REVIEW (approximately 1 hour offset) |
| PSRAM runtime usage | NOT YET VERIFIED |

`-DBOARD_HAS_PSRAM` is a build definition, not evidence that this application used PSRAM at runtime.

## LovyanGFX dependency

A clean PlatformIO resolution of `lovyan03/LovyanGFX@1.1.7` failed with `UnknownPackageError`. The successful build used the vendored library at `lib/LovyanGFX/`; when that directory is present, remove the registry dependency from `lib_deps` to avoid resolving the same library twice.

Vendoring must retain LovyanGFX's `license.txt`, credits, and the notices for its embedded third-party components.

## LVGL and SquareLine

`lib/lv_conf.h` must contain:

```c
#define LV_COLOR_16_SWAP 0
```

Otherwise `src/ui.c` stops with:

```text
LV_COLOR_16_SWAP should be 0 to match SquareLine Studio's settings
```

Treat SquareLine-generated UI files as generated artifacts: a later export may overwrite manual edits. Change them only with a clear regeneration plan.

## Locations and EEPROM

Upstream hard-coded `Rackeve`, `Budapest`, and `Kiskunlachaza`. The Panlee adaptation replaces the first city with `Moscow`. Find all coupled locations from PowerShell:

```powershell
Get-ChildItem .\src -Recurse -File -Include *.c,*.cpp,*.h |
  Select-String -Pattern "Moscow|Rackeve|Budapest|Kiskunlachaza|Location"
```

Check the LVGL/SquareLine dropdown options, city-index mapping in application logic, and EEPROM restore logic together. Because the location index is persisted, select the city again in the UI after changing or reordering the list.

A future refactor should use one location record containing city name, latitude, longitude, timezone, and country code. Prefer latitude/longitude for Weather API requests.

## Moscow timezone: open issue

Weather for Moscow physically works, but the clock is approximately one hour behind. This is not fixed or passed. Inspect the current implementation before changing it:

```powershell
Get-ChildItem .\src\Managers\TimeManager -Recurse -File |
  Select-String -Pattern "configTime|configTzTime|gmt|offset|daylight|3600|7200|10800"
```

The target is Moscow `UTC+3` with DST `0`.

## Radio stations and EEPROM

Locate station names, stream URLs, and selection logic with:

```powershell
Get-ChildItem .\src\Features\radio -Recurse -File |
  Select-String -Pattern "http\://|https\://|connecttohost|station|dropdown|SetupRadio"
```

Change each display name and stream URL as one pair. If EEPROM stores a station index, select the station again after reordering the list.

A future refactor should replace scattered `switch`/`case` lists with one array:

```cpp
struct RadioStation {
  const char *name;
  const char *url;
};
```

## Weather secrets

Never copy the OpenWeatherMap API key found in upstream source. A maintained source adaptation should use:

- ignored `include/weather_secrets.h` containing the real local key;
- tracked `include/weather_secrets.example.h` containing placeholders only.

Add the exact secret path to the adaptation's `.gitignore` before creating the real file.
