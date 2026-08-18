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
