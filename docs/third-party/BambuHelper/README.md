# BambuHelper on WT32-SC01-PLUS

BambuHelper is a specialized application for ESP32-S3 touchscreen boards that turns the display into a dedicated companion/control panel for Bambu Lab 3D printers.

Upstream project:

- https://github.com/Keralots/BambuHelper

## Video demonstration

- [YouTube Shorts — BambuHelper on WT32-SC01-PLUS: 3D printer, Web Flasher, Wi-Fi and project ideas](https://youtube.com/shorts/dWHqiyShObU)

## Why it is interesting for this lab

BambuHelper is valuable to WT32-SC01-PLUS-Lab not only as a 3D-printing application, but as an example of how to turn an ESP32-S3 touchscreen board into a complete user-facing product.

Interesting design directions include:

- a full touch UI rather than a minimal hardware demo;
- browser-based firmware installation with ESP Web Tools / Web Flasher;
- first-boot setup through Wi-Fi access-point onboarding;
- browser-based configuration after the device is on the network;
- a practical end-user workflow that does not require Arduino IDE, PlatformIO or esptool;
- reusable ideas for dashboards, control panels, clocks, appliance interfaces and other standalone devices.

## Relation to our Panlee reference board

The upstream project includes support for a Panlee WT32-SC01 Plus target and uses a hardware configuration very close to the physically validated lab specimen:

- ESP32-S3;
- 16 MB flash;
- 2 MB QSPI PSRAM in the build environment;
- ST7796-family display;
- 8-bit parallel display bus;
- touch over I2C.

This made BambuHelper a useful independent reference while developing and validating the clean-room Arduino BSP and browser-flashing workflow in this repository.

## Web Flasher as a reusable product pattern

One of the strongest ideas demonstrated by BambuHelper is the user path:

```text
Connect USB -> Open browser -> Select board/firmware -> Flash -> Configure Wi-Fi -> Use device
```

That pattern directly influenced the WT32-SC01-PLUS Lab Web Flasher:

- https://aidevelopersmonster.github.io/WT32-SC01-PLUS-Lab/

The lab flasher applies the same general browser-first idea to hardware-validation firmware for the physically verified Panlee reference specimen.

## Wi-Fi and web setup

A useful product-level lesson is that firmware installation is only the first step. A polished embedded device also needs a clear onboarding path.

BambuHelper demonstrates the broader pattern of:

```text
Flash -> First boot -> Wi-Fi/AP setup -> Browser configuration -> Normal operation
```

This is relevant to future WT32-SC01-PLUS projects where the board should be usable without a local development environment after initial installation.

## Ideas for future projects

The same hardware and interaction model can be reused far beyond 3D printing. Examples include:

- desktop status panels;
- retro digital clocks;
- smart-home dashboards;
- greenhouse/climate controllers;
- laboratory instruments;
- machine status terminals;
- network appliances;
- compact information displays.

## Retro-clock extraction idea

A separate follow-up task is to inspect BambuHelper's UI assets, fonts and large numeric/status widgets to determine whether any of them can be cleanly reused or adapted into a standalone retro electronic-clock demo for WT32-SC01-PLUS.

The goal would not be to copy the whole application, but to identify reusable visual patterns or components that can become an independent `RetroClock` / `DeskClock` example in this lab, subject to the upstream license and asset-redistribution terms.

## Scope and licensing

This repository documents BambuHelper as a third-party reference project. Upstream source code, graphics, fonts and other assets remain under their original license(s) and should not be copied into this repository without checking redistribution rights.
