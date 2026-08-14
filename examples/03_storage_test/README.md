# 03_storage_test

Read-only SD-card hardware-validation firmware for the reference **Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC / 230208** specimen.

**Current status:** `READY FOR BUILD / PHYSICAL VALIDATION PENDING`

This test deliberately avoids the factory firmware's write/rename/readback cycle. Its purpose is to validate the physical SD/SDSPI path without modifying card contents.

## Evidence basis

Factory-firmware reverse engineering recovered the SD interface as SDSPI:

| Signal | GPIO |
|---|---:|
| SD CLK | 39 |
| SD MOSI / DI | 40 |
| SD MISO / DO | 38 |
| SD CS | 41 |

The factory firmware used mount point `/sdcard` and contained an intended `hello.txt -> foo.txt` write/rename/read sequence. We intentionally do **not** reproduce that write path in the first independent lab test.

## What this test does

The firmware:

1. initializes the SPI bus on GPIO39/40/38;
2. initializes the SDSPI device with CS on GPIO41;
3. probes the SD card with `sdmmc_card_init()`;
4. prints card metadata with `sdmmc_card_print_info()`;
5. reads sector 0 with `sdmmc_read_sectors()`;
6. inspects the MBR partition entries if present;
7. if an MBR partition is found, reads only the first sector of the first partition;
8. prints simple FAT/exFAT boot-sector hints when identifiable;
9. deinitializes SDSPI and returns from `app_main()`.

The initial SDSPI test clock is deliberately conservative at **10 MHz**.

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

This distinction is intentional. In ESP-IDF 6.0.2, the SD FAT mount configuration used by `esp_vfs_fat_sdspi_mount()` does not provide the newer runtime `read_only` mount flag, so the first validation remains below the filesystem layer.

## Software configuration

The project uses the centralized specimen profile:

```text
software/espressif/config/panlee-v15-230208-sample-a.idf6.0.2.sdkconfig.defaults
```

No project-local `sdkconfig.defaults` copy is maintained.

## Build

From an activated ESP-IDF 6.0.2 shell:

```powershell
cd C:\Users\CHUWI\Documents\GitHub\WT32-SC01-PLUS-Lab
git fetch origin
git switch --track origin/agent/03-storage-test
cd .\examples\03_storage_test
idf.py fullclean
idf.py build
```

Do not flash until the build completes successfully and the generated flash command has been inspected.

## Physical test prerequisites

Insert an SD card before running the test.

Because the probe is read-only, an existing card with data may be used, but public logs should be reviewed before publication. This test does not enumerate filenames or read file contents, reducing accidental disclosure of card data.

## Expected successful serial sequence

A successful run should include lines equivalent to:

```text
WT32-SC01-PLUS-Lab / 03_storage_test
READ-ONLY SD / SDSPI hardware validation
...
Probing and initializing SD card
[CARD INFORMATION]
...
Reading sector 0 (read-only)
[SECTOR 0 / PARTITION TABLE]
...
[SAFETY AUDIT]
  FAT filesystem mounted   : no
  Files opened             : no
  Files created/renamed    : no
  Sectors written          : no
  Card formatted           : no

RESULT: STORAGE READ PATH PASS CANDIDATE
END 03_storage_test
```

## PASS criteria

Promote `03_storage_test` to PASS only after the real specimen demonstrates:

- SPI/SDSPI initialization succeeds;
- SD card initialization succeeds;
- card metadata can be read;
- sector 0 can be read correctly;
- no panic, watchdog reset, CRC storm or repeated timeout occurs;
- the application reaches `END 03_storage_test`.

A missing card, unsupported card, contact problem, pull-up problem or failed read is an **INVESTIGATE** result, not an automatic board failure.

## Claim boundary

A PASS from this test validates the SD physical read path and the GPIO39/40/38/41 SDSPI mapping for this specimen at the tested clock.

It does **not** yet validate:

- FAT filesystem mounting;
- file creation or modification;
- filesystem integrity;
- hot-plug/card-detect behavior;
- write-protect behavior;
- maximum stable SDSPI speed;
- every SD-card model or capacity;
- all WT32-SC01-PLUS OEM revisions.

A later write-path test, if needed, should use a dedicated expendable/test SD card and a separately declared destructive-test protocol.
