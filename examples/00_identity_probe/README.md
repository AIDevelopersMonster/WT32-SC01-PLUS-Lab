# 00_identity_probe

First executable firmware test for the reference **Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC / 230208** specimen.

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

Expected comparison result for the reference specimen:

```text
[REFERENCE COMPARISON]
  Chip model ESP32-S3      : MATCH
  Chip revision v0.2       : MATCH
  Flash ID 0x5E4018        : MATCH
  Flash size 16 MiB        : MATCH
  PSRAM size 2 MiB         : MATCH
  Identity baseline        : MATCH
```

A mismatch is an investigation result, not an automatic board failure. It may indicate another OEM/revision, a different memory population, a build/configuration problem, or an incorrect assumption in the baseline.

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

**Important:** uploading this firmware is still destructive to the currently installed factory firmware image because normal ESP-IDF flashing replaces boot/application/partition data. Use it only after a verified full factory-flash backup exists. The reference specimen already has two identical full 16 MiB reads recorded by the lab.

## Build

This example is a standalone ESP-IDF project.

From an ESP-IDF shell:

```powershell
cd examples\00_identity_probe
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
```

For the reference specimen, ensure PSRAM support is enabled for ESP32-S3 and configured conservatively for the known **Quad** PSRAM interface. Do not convert an unverified PSRAM clock into a board-level fact merely because a particular build setting works.

Then flash and monitor using the port that belongs to the board:

```powershell
idf.py -p COMx flash monitor
```

Exit the monitor with the normal ESP-IDF monitor shortcut.

## Evidence collection

Save the complete serial output from reset through the final `END 00_identity_probe` marker. Do not publish the base MAC address unless you intentionally want that hardware identifier public.

Recommended evidence destination after the physical run:

```text
evidence/specimens/panlee-v15-230208-sample-a/00_identity_probe/
```

Suggested files:

```text
serial-log.txt
build-environment.txt
README.md
```

Only after the real board has produced the expected runtime comparison should the example status be promoted from TODO to PASS.
