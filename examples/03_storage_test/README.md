# 03_storage_test

Read-only SD-card hardware-validation firmware for the reference **Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC / 230208** specimen.

**Current status:** `PASS — PHYSICALLY VALIDATED (MEDIA ANOMALY OBSERVED)`

Physical validation protocol:

- [`../../evidence/specimens/panlee-v15-230208-sample-a/03_storage_test/README.md`](../../evidence/specimens/panlee-v15-230208-sample-a/03_storage_test/README.md)

This test deliberately avoids the factory firmware's write/rename/readback cycle. Its purpose is to validate the physical SD/SDSPI read path without modifying card contents.

## Evidence basis and result

Factory-firmware reverse engineering recovered the SD interface as SDSPI:

| Signal | GPIO |
|---|---:|
| SD CLK | 39 |
| SD MOSI / DI | 40 |
| SD MISO / DO | 38 |
| SD CS | 41 |

`03_storage_test` then independently initialized and read a real SDHC card on the physical specimen at 10 MHz. It successfully read card metadata, sector 0, the MBR, and the first partition boot sector, and returned normally from `app_main()`.

The board-side result is therefore **PASS** for the SDSPI read path and this GPIO mapping.

The inserted card also exposed a separate media-consistency warning: its MBR partition extent exceeds the capacity reported by its CSD. The firmware reports this separately as `PASS WITH MEDIA ANOMALY`; this is not classified as a failure of the WT32-SC01-PLUS SD hardware path.

## What this test does

The firmware:

1. initializes the SPI bus on GPIO39/40/38;
2. initializes the SDSPI device with CS on GPIO41;
3. probes the SD card with `sdmmc_card_init()`;
4. prints card metadata with `sdmmc_card_print_info()`;
5. reads sector 0 with `sdmmc_read_sectors()`;
6. inspects the MBR partition entries if present;
7. compares MBR partition extents against `card.csd.capacity`;
8. if an MBR partition is found, reads only the first sector of the first partition;
9. prints simple FAT/exFAT boot-sector hints when identifiable;
10. deinitializes SDSPI and returns from `app_main()`.

The validated SDSPI test clock is **10 MHz**.

## Read-only safety boundary

This project contains no filesystem mount and no write API.

It does **not** call:

- `esp_vfs_fat_sdspi_mount()`;
- `fopen()`;
- `rename()`;
- `unlink()`;
- `esp_vfs_fat_sdcard_format()`;
- `sdmmc_write_sectors()`.

Therefore the intended storage access is limited to SD protocol initialization and raw sector reads.

## Physical run result

The tested card initialized as:

```text
Name: SD
Type: SDHC
Speed: 10.00 MHz (limit: 10.00 MHz)
Size: 52000MB
CSD: ver=2, sector_size=512, capacity=106496000
```

The first MBR partition was read as:

```text
Type          : 0x0C
Start LBA     : 2048
Sector count  : 125827072
End exclusive : 125829120
```

The first partition boot sector was read successfully and identified as FAT32-compatible:

```text
Signature           : 0x55AA
OEM / system field  : MSDOS5.0
Bytes per sector    : 512
Sectors per cluster : 64
Reserved sectors    : 38
FAT count           : 2
Filesystem hint     : FAT32
```

## Capacity consistency result

The automatic media audit reported:

```text
CSD addressable sectors : 106496000
MBR maximum end         : 125829120 (exclusive)
MBR vs CSD geometry     : WARNING - PARTITION EXTENT EXCEEDS CSD CAPACITY
```

Final runtime classification:

```text
SDSPI read path           : PASS
Card capacity consistency : WARNING - MBR EXCEEDS CSD CAPACITY
RESULT                    : PASS WITH MEDIA ANOMALY
```

This warning belongs to the inserted card / its partition geometry and remains a separate investigation item.

## Safety audit result

```text
FAT filesystem mounted : no
Files opened           : no
Files created/renamed  : no
Sectors written        : no
Card formatted         : no
```

## Build

From an activated ESP-IDF 6.0.2 shell:

```powershell
cd C:\Users\CHUWI\Documents\GitHub\WT32-SC01-PLUS-Lab
git switch agent/03-storage-test
cd .\examples\03_storage_test
idf.py build
```

## Flash and monitor

```powershell
idf.py -p COM10 flash monitor
```

Exit `idf_monitor` with:

```text
Ctrl+]
```

or `Ctrl+T`, then `Ctrl+X`.

## PASS criteria — satisfied

The real specimen demonstrated:

- SPI/SDSPI initialization succeeds;
- SD card initialization succeeds;
- card metadata can be read;
- sector 0 can be read correctly;
- MBR and first-partition boot sector can be read;
- the GPIO39/40/38/41 mapping works at 10 MHz;
- no panic, watchdog reset or read timeout occurred;
- the application reached `END 03_storage_test` and returned from `app_main()`.

## Claim boundary

This PASS validates the SD physical **read** path and GPIO39/40/38/41 SDSPI mapping for this specimen at 10 MHz.

It does **not** validate:

- FAT filesystem mounting;
- file creation or modification;
- SD-card write operations;
- filesystem integrity;
- true capacity/integrity of the tested SD card;
- hot-plug/card-detect behavior;
- write-protect behavior;
- maximum stable SDSPI speed;
- every SD-card model or capacity;
- all WT32-SC01-PLUS OEM revisions.

A later write-path test, if needed, should use a dedicated expendable/test SD card and a separately declared destructive-test protocol.
