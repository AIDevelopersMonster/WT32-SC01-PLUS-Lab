# Our version

This directory is reserved for a maintained adaptation of SubCoderHUN's WT32-SC01 PLUS Smart Desk Companion **after** the upstream baseline has been built and physically validated on our reference Panlee specimen.

Upstream license: Apache-2.0.

Any adapted source placed here must preserve required upstream notices and clearly identify our modifications.

## Gate before adaptation

Do not begin functional refactoring until the following baseline has been recorded:

- upstream PlatformIO build result;
- display/LVGL/touch physical result;
- network/NTP result;
- weather result using a user-controlled API key;
- EEPROM persistence result;
- optional SD result;
- optional audio/radio result.

## Planned adaptation candidates

- dedicated Panlee `ZX3D50CE08S-V15-USRC / 230208` PlatformIO environment;
- 16 MB flash / 2 MB embedded PSRAM profile;
- portable serial-port handling;
- secrets/configuration file excluded from version control;
- dependency refresh under regression tests;
- localization/UI improvements;
- comparison or integration with the local `WT32_SC01_PLUS` BSP where useful;
- explicit validation of LCD write clock before any performance change is promoted.

## Current status

```text
UPSTREAM_BASELINE_REQUIRED
OUR_VERSION_NOT_STARTED
```
