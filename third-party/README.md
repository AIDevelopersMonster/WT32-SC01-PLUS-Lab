# Third-party WT32-SC01-PLUS projects

This directory tracks external WT32-SC01-PLUS projects that are useful for testing, comparison, video demonstrations, ports, and compatibility work.

The repository does **not** assume that public source code may be copied. Each project directory must record the upstream source and license status before any code or assets are imported.

## Layout

```text
third-party/
  arduino-ide/
  platformio/
  esp-idf/
  micropython/
  esphome/
  openhasp/
  meshtastic/
```

Each external project gets its own directory. A project directory may contain:

```text
README.md        upstream, environment, license, validation, video plan
upstream/        metadata/links only unless redistribution is clearly licensed
our-version/     our independent port, wrapper, adaptation, or improvement
releases/        notes/manifests for tested release artifacts when applicable
evidence/        logs, photos, screenshots, measurements when needed
```

Empty directories are created only when material exists; the structure above is a convention rather than mandatory boilerplate.

## Required metadata for every project

- Upstream author/project and canonical link
- Environment/toolchain
- Board variant used upstream
- License and redistribution status
- Upstream revision/date used for the test
- Our tested hardware revision
- What was reproduced unchanged
- What we changed or reimplemented
- Known incompatibilities or uncertainty
- Related video and test evidence

## License rule

- MIT / BSD / Apache-style projects may be adapted when their notice and attribution requirements are preserved.
- GPL projects may be modified and redistributed only under the applicable copyleft obligations.
- If no explicit license is found, keep only links, metadata, observations, and independently written code. Do not mirror the upstream source or assets without permission.

## Project 1 — Arduino IDE / Sukesh minimal LovyanGFX

The first comparison is the minimal Arduino IDE / LovyanGFX example by Sukesh Akhilesh versus this repository's own `WT32_SC01_PLUS` Arduino BSP.

See [`arduino-ide/sukesh-minimal-lovyangfx/`](arduino-ide/sukesh-minimal-lovyangfx/).

## Project 2 — PlatformIO / SubCoderHUN Smart Desk Companion

The second target is a full application-level PlatformIO/LVGL project by SubCoderHUN: clock, weather, online radio, EEPROM settings, optional SD logging, and a SquareLine Studio UI.

Upstream is Apache-2.0 licensed, so a properly attributed adaptation may be maintained after the upstream baseline is physically reproduced.

See [`platformio/subcoderhun-smart-desk-companion/`](platformio/subcoderhun-smart-desk-companion/).

## Project 3 — BambuHelper

The third target is BambuHelper, a finished ESP32-S3/WT32-SC01-PLUS companion application for Bambu Lab printers. It is particularly useful as a reference for product-level UI, browser flashing, first-boot onboarding and browser-based configuration.

See [`BambuHelper/`](BambuHelper/).

## Project 4 — ESP-IDF / ESP32-TUX

The fourth target is Sukesh Akhilesh's ESP32-TUX reusable HMI template: ESP-IDF, LVGL 8.x, LovyanGFX, Wi-Fi provisioning, OTA, SD/SPIFFS, themes, brightness and rotation.

**Status: `PROJECT_4_REFERENCE_WORKLOAD_COMPLETE`.** The project has been migrated to ESP-IDF 6.0.2, adapted for the physically verified Panlee 16 MiB Flash / 2 MiB PSRAM profile, flashed to the real board and physically exercised across the major HMI, storage, memory, provisioning and time-service paths.

Project 4 is now treated as a validated architectural reference rather than the repository's future application core. Reusable ideas are being extracted incrementally above the existing `WT32_SC01_PLUS` BSP.

See:

- [`esp-idf/esp32-tux/`](esp-idf/esp32-tux/) — Project 4 overview and closure status;
- [`esp-idf/esp32-tux/our-version/README.md`](esp-idf/esp32-tux/our-version/README.md) — detailed physical validation;
- [`esp-idf/esp32-tux/TRANSFER-PLAN.md`](esp-idf/esp32-tux/TRANSFER-PLAN.md) — transfer/capability matrix and limits.

## Series rule going forward

Before starting each new numbered third-party project:

1. record the exact upstream source/revision and license;
2. identify hardware/toolchain assumptions that differ from the Panlee specimen;
3. separate source/build status from physical PASS;
4. after validation, record reusable lessons and what should **not** be adopted;
5. close the project with a concise transfer/capability note so the next project does not depend on conversational memory.
