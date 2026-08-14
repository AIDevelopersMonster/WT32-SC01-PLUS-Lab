# 00_identity_probe

First executable firmware test for the reference **Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC / 230208** specimen.

**Physical validation status: `PASS` (2026-08-14).**

Validation protocol:

- [`../../evidence/specimens/panlee-v15-230208-sample-a/00_identity_probe/README.md`](../../evidence/specimens/panlee-v15-230208-sample-a/00_identity_probe/README.md)

This example is deliberately narrow: it does **not** try to rediscover display, touch, audio, SD, RS-485 or expansion wiring that has already been recovered from the verified factory firmware. Instead, it independently re-measures the MCU/memory identity at runtime and compares the result with the established HW-01 / factory-reverse-engineering baseline.

## Why this test exists

The repository already has a much richer evidence trail than the original placeholder for this example. For the reference specimen, passive tooling and factory-firmware reverse engineering have already established:

- ESP32-S3 QFN56 revision v0.2;
- 16 MiB SPI flash;
- flash ID `0x5E4018`;
- 2 MiB embedded Quad PSRAM (`AP_3v3` in factory-tool evidence);
- factory application `get-start`, version `1`;
- factory build `Feb 14 2023 14:32:37`;
- factory ESP-IDF `v4.4.4-dirty`.

The canonical evidence remains outside this example:

- [`../../evidence/specimens/panlee-v15-230208-sample-a/hw01-chip/factory-flash-analysis.md`](../../evidence/specimens/panlee-v15-230208-sample-a/hw01-chip/factory-flash-analysis.md)
- [`../../evidence/specimens/panlee-v15-230208-sample-a/factory-mode-reverse-engineering.md`](../../evidence/specimens/panlee-v15-230208-sample-a/factory-mode-reverse-engineering.md)
- [`../../tools/factory-test/README.md`](../../tools/factory-test/README.md)

`main/reference_baseline.h` is only a small runtime-comparison snapshot. It is **not** a replacement for those evidence files.

## What this firmware measures

At startup it prints:

- current ESP-IDF and application metadata;
- ESP chip model, revision, core count and feature flags;
- base MAC address;
- previous reset reason;
- runtime SPI-flash JEDEC/device ID;
- runtime SPI-flash size;
- detected PSRAM size;
- SPIRAM heap totals;
- internal heap totals and largest free block;
- a field-by-field comparison with the reference specimen baseline.

The physical reference specimen produced:

```text
[REFERENCE COMPARISON]
  Chip model ESP32-S3      : MATCH
  Chip revision v0.2       : MATCH
  Flash ID 0x5E4018        : MATCH
  Flash size 16 MiB        : MATCH
  PSRAM size 2 MiB         : MATCH
  Identity baseline        : MATCH
```

A mismatch on another specimen is an investigation result, not an automatic board failure. It may indicate another OEM/revision, a different memory population, a build/configuration problem, or an incorrect assumption in the baseline.

## Safety boundary

The application itself intentionally performs no external peripheral initialization:

- no display initialization;
- no touch initialization;
- no explicit external GPIO writes;
- no SD/filesystem operations;
- no NVS writes;
- no Wi-Fi/BLE startup;
- no RS-485/UART1 setup.

This prevents `00_identity_probe` from depending on still-variant-sensitive peripheral pin maps.

**Important:** uploading this firmware is destructive to the currently installed factory firmware image because normal ESP-IDF flashing replaces boot/application/partition data. Use it only after a verified full factory-flash backup exists. The reference specimen has a verified full 16 MiB factory backup recorded by the lab.

## Reproducible configuration

The project keeps only the minimal board-specific configuration in `sdkconfig.defaults`:

```text
CONFIG_IDF_TARGET="esp32s3"
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_SPIRAM=y
```

A clean 2026-08-14 build from this Git-controlled defaults file succeeded without manual `menuconfig` changes. ESP-IDF resolved the Quad/Auto/40 MHz PSRAM settings for ESP32-S3 from its defaults after PSRAM support was enabled.

## Build and run

From an ESP-IDF shell:

```powershell
cd examples\00_identity_probe
idf.py build
idf.py -p COMx flash monitor
```

On the verified reference specimen, ESP-IDF reported during boot:

```text
esp_psram: Found 2MB PSRAM device
esp_psram: Speed: 40MHz
esp_psram: SPI SRAM memory test OK
```

The application is intentionally one-shot and returns from `app_main()` after printing the result. A final line such as:

```text
I (...) main_task: Returned from app_main()
```

is therefore normal completion, not a hang.

Exit `idf_monitor` with:

```text
Ctrl+]
```

ESP-IDF may also advertise `Ctrl+T`, then `Ctrl+X`. If the host monitor is instead interrupted after failed attempts to send characters to this non-interactive firmware, host-side `Writing to serial is timing out`, `KeyboardInterrupt`, or a nonzero `idf_monitor` exit code does **not** invalidate a test that already reached the final `END 00_identity_probe` marker.

## Evidence collection

The physical PASS protocol is stored at:

```text
evidence/specimens/panlee-v15-230208-sample-a/00_identity_probe/README.md
```

The complete local serial log should be retained for lab control. The public repository should not publish the base MAC address unless disclosure is intentional.
