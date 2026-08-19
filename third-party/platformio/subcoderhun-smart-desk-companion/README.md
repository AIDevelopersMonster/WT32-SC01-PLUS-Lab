# SubCoderHUN WT32-SC01 PLUS Smart Desk Companion

This is the second third-party validation target in the WT32-SC01-PLUS-Lab ecosystem and the first full application-level PlatformIO/LVGL project. This dossier tracks the upstream project, its Panlee adaptation, and validation evidence.

- [Upstream project record](upstream/README.md) — canonical source, snapshot, license, and original environment.
- [Panlee adaptation](our-version/README.md) — validated 16 MB build profile and operator notes.
- [Physical demo](https://youtube.com/shorts/4kxUpJS4kCk) — tested on Panlee `ZX3D50CE08S-V15-USRC / 230208`.
- [Evidence matrix](evidence/README.md) — claim-by-claim validation status and remaining checks.

## Attribution

Based on [SubCoderHUN/WT32-SC01-PLUS](https://github.com/SubCoderHUN/WT32-SC01-PLUS).
Original project licensed under the Apache License 2.0. Modified for Panlee
WT32-SC01-PLUS / `ZX3D50CE08S-V15-USRC`.

Validation snapshot: `df8c3f251ee2d9fe8ab0961343251661d1c10e40` (checked 2026-08-18).

## Application scope

The application combines clock/date display, Wi-Fi configuration, NTP synchronization, OpenWeatherMap weather, online radio over I2S, brightness control, EEPROM-backed settings, optional microSD logging, Wake-on-LAN, and an LVGL UI generated with SquareLine Studio.

## Hardware match

The upstream display, touch, audio, and SD pin assignments closely match those independently established for the named Panlee specimen. The adaptation uses a 16 MB flash profile instead of upstream's 8 MB profile.

The physically demonstrated adaptation covers boot, display/LVGL, touch, UI navigation, brightness, Wi-Fi, Moscow weather, and online radio/I2S. Moscow time remains approximately one hour behind, and PSRAM runtime use is not yet verified. EEPROM persistence and SD logging are not promoted by this demonstration.

## Development environment

The recorded adaptation uses PlatformIO, the Arduino framework, `platformio/espressif32@6.5.0`, LVGL 8.3.6, and vendored LovyanGFX. See the setup guide for the exact Panlee 16 MB overrides.

## Security and release discipline

The upstream source contained an OpenWeatherMap API key. That key is not reproduced here. Use a local ignored secrets header as described in the adaptation README. Do not publish build caches, credentials, private keys, or binaries without a provenance, license, and validation record.
