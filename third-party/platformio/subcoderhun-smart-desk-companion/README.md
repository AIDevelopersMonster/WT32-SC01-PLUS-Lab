# SubCoderHUN WT32-SC01 PLUS Smart Desk Companion

Quick links:

- [Upstream project record](upstream/README.md)
- [Panlee adaptation](our-version/README.md)
- [Physical demo](https://youtube.com/shorts/4kxUpJS4kCk)
- [Evidence matrix](evidence/README.md)

## Why this project

This is the second third-party validation target in the WT32-SC01-PLUS-Lab ecosystem and the first full application-level PlatformIO/LVGL project.

Upstream:

- Author: SubCoderHUN
- Repository: https://github.com/SubCoderHUN/WT32-SC01-PLUS
- Application: WT32-SC01 PLUS Smart Desk Companion
- Snapshot commit selected for validation: `df8c3f251ee2d9fe8ab0961343251661d1c10e40`
- Upstream main last pushed at checked snapshot: 2025-10-26
- License: Apache-2.0
- Checked: 2026-08-18

## Upstream capabilities

The application combines:

- clock/date display;
- Wi-Fi configuration;
- NTP time synchronization;
- current weather from OpenWeatherMap;
- online radio over I2S;
- brightness control and scheduled dimming;
- EEPROM-backed settings;
- optional microSD logging;
- Wake-on-LAN;
- LVGL UI generated from a SquareLine Studio project.

## Development environment

Primary environment:

```text
Visual Studio Code
PlatformIO
Arduino framework
ESP32-S3 DevKitC-1 target
LVGL 8.3.6
LovyanGFX 1.1.7
SquareLine Studio project included upstream
```

Upstream `platformio.ini` currently declares an 8 MB flash profile and `BOARD_HAS_PSRAM`.

## Hardware compatibility hypothesis

The upstream source uses the same major pin assignments independently established on our Panlee specimen:

```text
LCD WR       GPIO47
LCD RS/DC    GPIO0
LCD D0..D7   9,46,3,8,18,17,16,15
LCD RST      GPIO4
LCD BL       GPIO45
Touch SDA    GPIO6
Touch SCL    GPIO5
Touch addr   0x38

Audio BCLK   GPIO36
Audio LRC    GPIO35
Audio DOUT   GPIO37

SD CS        GPIO41
SD CLK       GPIO39
SD CMD/MOSI  GPIO40
SD D0/MISO   GPIO38
```

That makes the project a strong cross-implementation validation candidate for the Panlee `ZX3D50CE08S-V15-USRC / 230208` specimen.

The Panlee adaptation is now physically validated for the explicitly recorded display, touch, UI, brightness, Wi-Fi, Moscow weather, and online-radio/I2S paths. This does not certify untested OEM revisions or subsystems not listed as passed.

## Important configuration differences

Our reference specimen has been independently observed as:

```text
Flash: 16 MB
Embedded PSRAM: 2 MB
```

The upstream PlatformIO environment currently requests:

```text
board_build.flash_size = 8MB
board_build.partitions = default_8MB.csv
BOARD_HAS_PSRAM
```

The first reproduction should preserve the upstream configuration as much as practical. A separate Panlee-specific 16 MB / 2 MB environment belongs in `our-version/` only after the upstream baseline is recorded.

## Credential/security note

The checked upstream weather source contains an OpenWeatherMap API credential directly in source code. We will not reproduce that credential in this repository or documentation.

For physical network validation, use a user-controlled API key and document that local substitution as an environment/secret change rather than treating it as upstream code behavior.

A future adaptation should move credentials into a non-committed secrets/configuration mechanism.

## Validation plan

1. **UPSTREAM SOURCE AUDIT** — complete enough to start reproduction.
2. **UPSTREAM BUILD** — clone/open the inner `WT32-SC01-PLUS` PlatformIO project and build without functional modifications.
3. **UI PHYSICAL PASS** — boot, display, LVGL, touch, navigation, brightness.
4. **NETWORK PASS** — Wi-Fi, NTP, then weather with a user-controlled API key.
5. **PERSISTENCE PASS** — EEPROM-backed settings/reboot behavior.
6. **SD PASS** — optional logging using the already known physical SD path.
7. **AUDIO/RADIO PASS** — online stream through the I2S path when external audio hardware is connected.
8. **CROSS-IMPLEMENTATION REVIEW** — compare observations with our Arduino BSP validation.
9. **OUR VERSION** — only after the upstream baseline is stable.

## Candidate improvements after baseline

Potential changes for `our-version/` include:

- Panlee V15 16 MB / 2 MB PlatformIO environment;
- removal of hard-coded serial-port assumptions;
- secrets handling for weather credentials;
- dependency/toolchain refresh only when regression-tested;
- localization and UI improvements;
- documented use of our independently validated hardware profile;
- possible integration points with `WT32_SC01_PLUS` where technically useful;
- separate LCD clock-frequency qualification before adopting the upstream 40 MHz write rate in our BSP.

## Status

| Item | Status |
|---|---|
| Upstream repository reviewed | YES |
| Apache-2.0 license confirmed | YES |
| Snapshot commit recorded | YES |
| Pin mapping compared with Panlee specimen | HIGH MATCH |
| PlatformIO build | PASS |
| Upload | PASS |
| Boot | PHYSICAL PASS |
| ST7796 / LVGL | PHYSICAL PASS |
| Touch / UI navigation / brightness | PHYSICAL PASS |
| Wi-Fi / Weather for Moscow | PHYSICAL PASS |
| Online radio / I2S | PHYSICAL PASS |
| 16 MB build profile | PASS |
| Moscow timezone | NEEDS REVIEW (approximately 1 hour offset) |
| PSRAM runtime usage | NOT YET VERIFIED |
| Video evidence | [PHYSICAL DEMO](https://youtube.com/shorts/4kxUpJS4kCk) |

## Directory layout

```text
subcoderhun-smart-desk-companion/
  README.md
  upstream/
    README.md
  our-version/
    README.md
  evidence/
    README.md
  releases/
    README.md
```
