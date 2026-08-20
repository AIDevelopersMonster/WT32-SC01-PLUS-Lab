# Project 4 — ESP32-TUX / controlled Panlee adaptation

## Upstream

- Author: Sukesh Ashok Kumar (`sukesh-ak`)
- Repository: https://github.com/sukesh-ak/ESP32-TUX
- Upstream branch reviewed: `master`
- Upstream revision reviewed: `47639648a37ffc9ef9c2a748eeb9761894b9238a`
- Upstream revision date: 2024-02-27
- License: MIT
- Framework: ESP-IDF
- UI: LVGL 8.x
- Display/touch driver: LovyanGFX

ESP32-TUX is a reusable touch-HMI template rather than a single-purpose demo. Its documented screens and services include Home, Remote/App, Settings and OTA, plus Wi-Fi provisioning, display rotation, brightness, themes, SPIFFS/SD integration and task-safe LVGL access patterns.

## Why this is Project 4

The third-party series now progresses from a minimal board bring-up example to increasingly complete applications and reusable architecture:

1. Sukesh minimal LovyanGFX — direct Arduino display/touch bring-up.
2. SubCoderHUN Smart Desk Companion — application-level PlatformIO/LVGL integration.
3. BambuHelper — finished product/onboarding and browser-flashing reference.
4. **ESP32-TUX — reusable ESP-IDF/LVGL HMI architecture.**

ESP32-TUX is useful because it exposes a reusable device profile and a substantially larger UI/application architecture while still explicitly supporting WT32-SC01 Plus.

## Reference hardware

Our validation target is strictly:

```text
Panlee WT32-SC01-PLUS
ZX3D50CE08S-V15-USRC
marking 230208
ESP32-S3 rev v0.2
16 MiB SPI Flash
2 MiB embedded PSRAM
```

This does not establish compatibility with every board sold as WT32-SC01-PLUS.

## Static hardware compatibility audit

The upstream WT32-SC01-Plus LovyanGFX profile was compared with this lab's physically validated Panlee BSP profile.

| Function | ESP32-TUX upstream | Panlee lab profile | Static result |
|---|---:|---:|---|
| LCD WR | GPIO47 | GPIO47 | MATCH |
| LCD DC/RS | GPIO0 | GPIO0 | MATCH |
| LCD reset | GPIO4 | GPIO4 | MATCH |
| LCD backlight | GPIO45 | GPIO45 | MATCH |
| LCD D0..D7 | 9,46,3,8,18,17,16,15 | 9,46,3,8,18,17,16,15 | MATCH |
| Touch SDA | GPIO6 | GPIO6 | MATCH |
| Touch SCL | GPIO5 | GPIO5 | MATCH |
| Touch INT | GPIO7 | GPIO7 | MATCH |
| Touch I2C address | 0x38 | 0x38 | MATCH |
| SD MISO | GPIO38 | GPIO38 | MATCH |
| SD MOSI | GPIO40 | GPIO40 | MATCH |
| SD SCLK | GPIO39 | GPIO39 | MATCH |
| SD CS | GPIO41 | GPIO41 | MATCH |

This is a **source-level compatibility result**, not a physical PASS for ESP32-TUX itself.

### Important bus-speed difference

ESP32-TUX configures the 8-bit display bus for a 40 MHz write frequency. The lab's conservative Panlee BSP profile currently uses a 10 MHz LCD clock. The pin mapping therefore matches, but 40 MHz must be treated as an upstream assumption until it is physically exercised on this specimen.

## Flash-layout incompatibility that must be fixed first

The upstream defaults are explicitly configured for **8 MB Flash** and use `partitions/partition-8MB.csv`.

The upstream partition table contains:

```text
factory  2M
ota_0    2M
ota_1    2M
storage  512K
```

Our Panlee specimen has **16 MB physical Flash**. Therefore we will not use an upstream prebuilt 8 MB image as the canonical Panlee target and will not blindly transplant its flash assumptions.

A Panlee adaptation must explicitly select 16 MB Flash and use a reviewed partition layout. The first adaptation may retain the upstream partition sizes while merely declaring the real flash size; later work may intentionally use the additional space, but only after the baseline boots and validates correctly.

## Other configuration items requiring review

- Upstream documentation targets ESP-IDF 5.0; our current lab toolchain is newer, so API/build compatibility must be checked rather than assumed.
- Upstream `sdkconfig.defaults` uses a non-Moscow timezone default (`UTC-05:30`). Timezone configuration must be set deliberately for the test location/application.
- The example OTA URL points to a private LAN address and is not a production update service. OTA will not be treated as validated merely because the UI exposes it.
- Wi-Fi provisioning changes NVS/network state. Test evidence must distinguish UI visibility from successful provisioning and reconnection.
- SD access must be tested separately from display/touch even though the GPIO mapping matches our validated SDSPI profile.

## Validation gate

Current status:

| Item | Status |
|---|---|
| Upstream repository and license audited | PASS |
| Exact upstream revision recorded | PASS |
| WT32-SC01 Plus device profile found | PASS |
| Pin mapping compared with Panlee BSP | MATCH |
| 8 MB vs 16 MB flash mismatch identified | PASS |
| Panlee-specific build configuration | NOT YET CREATED |
| Build with selected ESP-IDF toolchain | NOT YET RUN |
| Upload to Panlee specimen | NOT YET RUN |
| Boot | NOT YET RUN |
| LCD/LVGL | NOT YET RUN |
| Touch | NOT YET RUN |
| Rotation/theme/brightness | NOT YET RUN |
| Wi-Fi provisioning | NOT YET RUN |
| SD/SPIFFS | NOT YET RUN |
| OTA | NOT YET RUN |
| Video evidence | NOT YET RECORDED |

No runtime or hardware PASS claim should be promoted until the corresponding physical test is performed.

## Planned test order

1. Preserve the existing factory-flash backup and specimen identity.
2. Prepare a Panlee-specific 16 MB build configuration without changing unrelated application behavior.
3. Build from source using the recorded upstream revision and resolved dependencies.
4. Review generated flash offsets, partition table and image sizes before upload.
5. Flash through the normal ESP32-S3 development path.
6. Verify boot and serial logs.
7. Verify display/LVGL output and touch.
8. Verify brightness, themes and orientation changes.
9. Verify Wi-Fi provisioning and reconnect behavior.
10. Verify SPIFFS and SD separately.
11. Treat OTA as an independent test; do not point it at the upstream author's example LAN URL.
12. Record logs and a physical video before updating this table to PASS.

## Directory layout

```text
esp32-tux/
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
