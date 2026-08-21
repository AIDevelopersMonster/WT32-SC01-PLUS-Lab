# Project 4 — ESP32-TUX / Panlee ESP-IDF 6 adaptation

## Status

`PROJECT_4_REFERENCE_WORKLOAD_COMPLETE`

Project 4 is no longer only a static compatibility study. ESP32-TUX has been migrated from its ESP-IDF 5-era upstream baseline to **ESP-IDF 6.0.2**, adapted for the physically verified Panlee memory profile, flashed to the real board, and exercised across display, LVGL, touch, storage, PSRAM, Wi-Fi provisioning, time synchronization and several UI/system functions.

Detailed implementation and physical evidence are maintained in:

- [`our-version/README.md`](our-version/README.md) — ESP-IDF 6.0.2 migration and physical validation;
- [`our-version/PANLEE-BUILD.md`](our-version/PANLEE-BUILD.md) — controlled Panlee build procedure;
- [`TRANSFER-PLAN.md`](TRANSFER-PLAN.md) — reusable ideas to carry into our own WT32_SC01_PLUS ecosystem, plus capability boundaries;
- [`evidence/`](evidence/) — project-specific evidence records;
- [`upstream/`](upstream/) — upstream provenance and metadata.

## Upstream

- Author: Sukesh Ashok Kumar (`sukesh-ak`)
- Repository: https://github.com/sukesh-ak/ESP32-TUX
- Upstream branch reviewed: `master`
- Upstream revision reviewed: `47639648a37ffc9ef9c2a748eeb9761894b9238a`
- Upstream revision date: 2024-02-27
- License: MIT
- Original framework generation: ESP-IDF 5-era
- UI: LVGL 8.x
- Display/touch driver: LovyanGFX

ESP32-TUX is a reusable touch-HMI template with Home, Remote/App, Settings and OTA/Device Info pages, plus Wi-Fi provisioning, themes, brightness, rotation, SPIFFS/SD integration and task-safe UI patterns.

## Why this is Project 4

The third-party series progresses from minimal board bring-up toward reusable product architecture:

1. Sukesh minimal LovyanGFX — direct Arduino display/touch bring-up.
2. SubCoderHUN Smart Desk Companion — application-level PlatformIO/LVGL integration.
3. BambuHelper — finished-product onboarding and browser-flashing reference.
4. **ESP32-TUX — reusable ESP-IDF/LVGL HMI architecture.**

Project 4 proved that the Panlee board can run a substantially complete network-connected touchscreen workload on a current ESP-IDF toolchain.

## Reference hardware

Validation target:

```text
Panlee WT32-SC01-PLUS
ZX3D50CE08S-V15-USRC
marking 230208
ESP32-S3 rev v0.2
16 MiB SPI Flash
2 MiB embedded PSRAM
```

This remains specimen-specific evidence and does not prove compatibility with every WT32-SC01-PLUS OEM variant.

## Hardware compatibility result

The upstream WT32-SC01 Plus profile matches the lab's physically validated Panlee GPIO mapping for LCD, touch and SD:

| Function | ESP32-TUX upstream | Panlee lab profile | Result |
|---|---:|---:|---|
| LCD WR | GPIO47 | GPIO47 | MATCH |
| LCD DC/RS | GPIO0 | GPIO0 | MATCH |
| LCD reset | GPIO4 | GPIO4 | MATCH |
| LCD backlight | GPIO45 | GPIO45 | MATCH |
| LCD D0..D7 | 9,46,3,8,18,17,16,15 | 9,46,3,8,18,17,16,15 | MATCH |
| Touch SDA/SCL/INT | 6/5/7 | 6/5/7 | MATCH |
| Touch address | 0x38 | 0x38 | MATCH |
| SD MISO/MOSI/SCLK/CS | 38/40/39/41 | 38/40/39/41 | MATCH |

The original upstream display bus uses a 40 MHz write frequency, while the lab BSP uses a more conservative 10 MHz profile. The successful Project 4 physical run is evidence for the ESP32-TUX workload on this specimen; it does not automatically change the conservative BSP default for all applications.

## Flash-layout adaptation

Upstream defaults assume 8 MB Flash. The Panlee specimen physically has 16 MiB, so the maintained adaptation explicitly declares 16 MB while initially preserving the original application/OTA/storage geometry.

Validated baseline layout:

```text
NVS       0x009000   16 KiB
OTA data  0x00D000    8 KiB
PHY init  0x00F000    4 KiB
factory   0x010000    2 MiB
ota_0     0x210000    2 MiB
ota_1     0x410000    2 MiB
storage   0x610000  512 KiB
```

Unused physical flash is intentionally left unallocated in this baseline rather than redesigned before validation.

## Current physical validation summary

| Function | Status |
|---|---|
| ESP-IDF 6.0.2 build | **PASS** |
| Flash write / verification | **PASS** |
| Physical boot | **PASS** |
| 16 MiB Flash | **PASS** |
| 2 MiB PSRAM detection | **PASS** |
| PSRAM memory test / heap integration | **PASS** |
| Display / LVGL pages | **PASS** |
| Touch / page navigation | **PASS** |
| SPIFFS | **PASS** |
| SD detection + filesystem mount | **PASS** |
| SoftAP provisioning | **PASS** |
| Protocomm Security 1 | **PASS** |
| Wi-Fi credentials + connection | **PASS** |
| Credential persistence after reboot | **PASS** |
| SNTP / NTP | **PASS** |
| Moscow UTC+3 / DST=0 | **PASS** |
| Light/Dark theme switching | **PASS** |
| Brightness control UI | **PASS** |
| Remote page UI | **PASS** |
| QR provisioning/info screen | **PASS** |
| OTA UI / event / task startup | **PASS** |
| OTA image download/install/reboot | **NOT YET VALIDATED** |
| Weather UI | **PASS** |
| Live weather data | **NOT CONFIGURED** |
| Remote application actions | **NOT TESTED** |
| SD sample `readme.txt` file read | **NOT TESTED** — file absent on tested card |

The authoritative detailed evidence and logs are in [`our-version/README.md`](our-version/README.md).

## What we are taking from Project 4

Project 4 is now primarily an architectural reference. The reusable targets are:

- LVGL navigation/page shell;
- Settings and Device Info pages;
- brightness, themes and orientation patterns;
- controlled/task-safe LVGL event access;
- Wi-Fi status/configuration patterns;
- persistent credentials;
- dual-purpose QR lifecycle: provisioning before configuration, documentation/info afterward;
- stable redirect-ready QR destination instead of hard-coding changing URLs;
- SPIFFS/SD asset abstraction;
- generic Remote command surface;
- OTA UI/event architecture;
- NTP/timezone service patterns.

The lab will **not** adopt the complete ESP32-TUX source tree as its permanent core framework. Those ideas should be extracted incrementally above the existing `WT32_SC01_PLUS` BSP.

See [`TRANSFER-PLAN.md`](TRANSFER-PLAN.md) for the implementation/capability matrix, including what can be completed in code and what still requires real hardware, network services or external credentials.

## Relationship to the next work

`13_LVGL_BasicUI` is already the first small BSP-centered extraction step: it demonstrates LVGL rendering, touch events and brightness control without importing the full ESP32-TUX application architecture.

Project 4 itself is therefore considered closed as a reference workload. Future reusable HMI work belongs to our own modules/examples, while unfinished external-service tests (full OTA, live weather, etc.) remain optional Project 4 follow-ups rather than blockers for starting Project 5.
