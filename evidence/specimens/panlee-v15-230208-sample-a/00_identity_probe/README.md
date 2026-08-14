# 00_identity_probe — hardware validation protocol

**Status:** `PASS`

**Specimen:** `panlee-v15-230208-sample-a`  
**Board marking:** Panlee / ZX3D50CE08S-V15-USRC / 230208  
**Physical run date:** 2026-08-14  
**Test project:** [`examples/00_identity_probe`](../../../../examples/00_identity_probe/)  
**Application version:** `8e964eb`  
**ESP-IDF:** `v6.0.2`  
**Transport:** built-in USB Serial/JTAG, `COM10` during this run

## Purpose

This protocol records the first physical execution of the repository's own `00_identity_probe` firmware on the reference specimen. The test is an independent runtime cross-check of the already established HW-01/factory-reverse-engineering baseline; it is not a rediscovery test for display, touch, audio, SD, RS-485, or expansion wiring.

## Reproducibility check

Before flashing, the local generated `sdkconfig`, `sdkconfig.old`, and earlier local `sdkconfig.defaults` were removed. The branch copy of `sdkconfig.defaults` was then pulled from GitHub and a clean build was performed.

The configuration used for the clean build is intentionally minimal:

```text
CONFIG_IDF_TARGET="esp32s3"
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_SPIRAM=y
```

ESP-IDF generated the runtime configuration from this file and completed the clean build successfully. The generated application image size was `0x2b630` bytes; the 1 MiB application partition retained about 83% free space.

## Flashing procedure

The physical run used:

```powershell
idf.py -p COM10 flash monitor
```

The flasher connected to an ESP32-S3 QFN56 revision v0.2 and reported 40 MHz crystal, USB Serial/JTAG, and 2 MiB embedded PSRAM. Bootloader, partition table, and application writes all completed with verification of written data.

## Boot-time memory validation

ESP-IDF reported:

```text
esp_psram: Found 2MB PSRAM device
esp_psram: Speed: 40MHz
esp_psram: SPI SRAM memory test OK
esp_psram: Adding pool of 2048K of PSRAM memory to heap allocator
```

This is a direct physical validation that the selected ESP-IDF configuration can initialize and test the board's 2 MiB PSRAM.

## Runtime measurements

| Item | Measured result | Expected baseline | Result |
|---|---:|---:|---|
| MCU | ESP32-S3 | ESP32-S3 | PASS |
| Silicon revision | v0.2 (`raw=2`) | v0.2 | PASS |
| CPU cores | 2 | 2 | PASS |
| Flash JEDEC/device ID | `0x5E4018` | `0x5E4018` | PASS |
| Flash manufacturer byte | `0x5E` | `0x5E` | PASS |
| Flash device bytes | `0x4018` | `0x4018` | PASS |
| Runtime Flash size | 16 MiB | 16 MiB | PASS |
| PSRAM size | 2 MiB | 2 MiB | PASS |
| PSRAM startup test | OK | operational | PASS |
| PSRAM runtime speed | 40 MHz | build/runtime observation | INFO |
| Wi-Fi capability | present | present | PASS |
| Bluetooth LE capability | present | present | PASS |
| Bluetooth Classic | absent | not expected for ESP32-S3 | PASS |

The application printed:

```text
[REFERENCE COMPARISON]
  Chip model ESP32-S3      : MATCH
  Chip revision v0.2       : MATCH
  Flash ID 0x5E4018        : MATCH
  Flash size 16 MiB        : MATCH
  PSRAM size 2 MiB         : MATCH
  Identity baseline        : MATCH
```

Final acceptance result: **PASS**.

## Memory snapshot

Runtime heap data from this run:

```text
PSRAM size             : 2097152 bytes
Heap SPIRAM total      : 2097152 bytes
Heap SPIRAM free       : 2094964 bytes
Internal 8-bit total   : 444971 bytes
Internal 8-bit free    : 394927 bytes
Largest internal block : 294912 bytes
Minimum free heap      : 2457860 bytes
```

`Minimum free heap` is a global heap metric and includes the configured external PSRAM; it must not be interpreted as internal DRAM alone.

## Feature-flag note

The generic `esp_chip_info()` feature flags printed `Embedded flash: no` and `Embedded PSRAM: no`, while direct memory probes and boot-time initialization independently confirmed external SPI Flash and 2 MiB PSRAM. These two generic feature-flag lines are therefore informational only and are **not** used as the memory acceptance criteria for this board.

## Safety boundary observed

The test application performed no display/touch initialization, no application-level external GPIO writes, no SD/filesystem or NVS writes, and no Wi-Fi/BLE startup. Normal ESP-IDF bootloader/system initialization still occurs before `app_main()`.

Flashing the test necessarily replaced the bootloader, partition table, and application regions of the previously installed factory image. A verified full 16 MiB factory backup exists separately in the lab evidence.

## End-of-test behavior

The firmware is deliberately one-shot. After printing the final marker it returns from `app_main()`:

```text
END 00_identity_probe - save the complete serial log as evidence
I (...) main_task: Returned from app_main()
```

This is normal ESP-IDF behavior and is not a crash or hang.

During this run the monitor was later interrupted from the keyboard after attempts to exit/write while the application provided no interactive console. The resulting messages such as `Writing to serial is timing out`, `KeyboardInterrupt`, and the nonzero `idf_monitor` exit code belong to the **host-side monitor shutdown**, after the firmware had already completed successfully. They do not change the hardware test result.

Preferred monitor exit is:

```text
Ctrl+]
```

or the ESP-IDF alternative shown by the monitor (`Ctrl+T`, then `Ctrl+X`).

## Evidence policy

The full local log contains a board-unique base MAC address. The public repository protocol records the technical results but intentionally does not reproduce that identifier. If a raw serial log is published later, redact board-unique identifiers unless intentional disclosure is required.

## Related evidence

- [`../hw01-chip/factory-flash-analysis.md`](../hw01-chip/factory-flash-analysis.md)
- [`../factory-mode-reverse-engineering.md`](../factory-mode-reverse-engineering.md)
- [`../../../../tools/factory-test/README.md`](../../../../tools/factory-test/README.md)
- [`../../../../examples/00_identity_probe/README.md`](../../../../examples/00_identity_probe/README.md)
