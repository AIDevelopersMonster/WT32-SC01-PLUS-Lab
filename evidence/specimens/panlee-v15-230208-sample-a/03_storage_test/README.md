# 03_storage_test — physical validation protocol

**Status:** `PASS WITH MEDIA ANOMALY`  
**Board hardware result:** `PASS`  
**Media consistency result:** `WARNING`  

**Acceptance stage:** `HW-04`  
**Specimen:** `panlee-v15-230208-sample-a`  
**Board marking:** Panlee / `ZX3D50CE08S-V15-USRC` / `230208`  
**Physical run date:** 2026-08-15  
**Test project:** [`examples/03_storage_test`](../../../../examples/03_storage_test/)  
**Application version:** `8874b2b`  
**ESP-IDF:** `v6.0.2`

## Purpose

This run validates the physical SD-card read path independently of the original factory firmware while deliberately avoiding all filesystem and sector writes.

The test exercises:

- ESP32-S3 SPI master;
- SDSPI card initialization;
- recovered SD pin mapping;
- SD card metadata readout;
- raw sector 0 read;
- MBR partition-table parsing;
- first-partition boot-sector read;
- consistency check between MBR partition extents and card-reported CSD capacity.

## Physically validated SDSPI mapping

| Signal | GPIO |
|---|---:|
| SD CLK | 39 |
| SD MOSI / DI | 40 |
| SD MISO / DO | 38 |
| SD CS | 41 |

These signals were previously recovered from the factory firmware. `03_storage_test` now provides direct physical confirmation that this mapping can initialize and read a real SD card on the named specimen.

## Test configuration

```text
SDSPI clock              : 10000 kHz
Filesystem mount         : NO
Sector writes            : NO
Format / create / rename : NO
Read scope               : metadata + sector 0 + first partition boot sector
```

The test uses only raw read operations for card data. It does not mount FAT and does not call `sdmmc_write_sectors()`.

## Boot / application result

The board booted normally under ESP-IDF 6.0.2:

```text
Project name: wt32_sc01_plus_storage_test
App version : 8874b2b
ESP-IDF     : v6.0.2
Chip rev    : v0.2
Flash       : 16MB
PSRAM       : 2MB, 40MHz, memory test OK
```

The application reached `app_main()`, completed the storage probe and returned normally.

## SD-card initialization result

The card initialized successfully over SDSPI:

```text
Name: SD
Type: SDHC
Speed: 10.00 MHz (limit: 10.00 MHz)
Size: 52000MB
CSD: ver=2, sector_size=512, capacity=106496000 read_bl_len=9
SSR: bus_width=1
SDSPI actual clock: 10000 kHz
```

During protocol probing, the log included unsupported responses for CMD52 and CMD5. They did not prevent SD memory-card initialization and are not classified as a board failure in this successful run.

## Raw sector read result

Sector 0 was read successfully and contained a valid `0x55AA` signature.

The first MBR partition entry was:

```text
Partition 1
Type          : 0x0C
Start LBA     : 2048
Sector count  : 125827072
End exclusive : 125829120
```

The first partition boot sector at LBA 2048 was also read successfully:

```text
Signature           : 0x55AA
OEM / system field  : MSDOS5.0
Bytes per sector    : 512
Sectors per cluster : 64
Reserved sectors    : 38
FAT count           : 2
Filesystem hint     : FAT32
```

This directly confirms that the board can perform actual data reads from the inserted card, not merely detect card presence.

## Capacity-consistency warning

The automatic geometry audit detected that the partition table declares an extent beyond the capacity reported by the card CSD:

```text
CSD addressable sectors : 106496000
MBR maximum end         : 125829120 (exclusive)
MBR vs CSD geometry     : WARNING - PARTITION EXTENT EXCEEDS CSD CAPACITY
```

Therefore the run is intentionally classified as:

```text
SDSPI read path           : PASS
Card capacity consistency : WARNING - MBR EXCEEDS CSD CAPACITY
RESULT                    : PASS WITH MEDIA ANOMALY
```

This warning is evidence about the inserted SD card / its partition geometry. It is **not** evidence of failure in the WT32-SC01-PLUS SDSPI hardware path.

Possible causes of the media anomaly are intentionally left unresolved by this test. The read-only board validation does not distinguish among stale/incorrect partitioning, unusual card behavior, incorrect capacity reporting, or other media-specific causes.

## Safety audit

The firmware reported:

```text
FAT filesystem mounted : no
Files opened           : no
Files created/renamed  : no
Sectors written        : no
Card formatted         : no
```

No write-path validation is claimed.

## Physical PASS conclusion

**HW-04 SD/SDSPI read path: PASS for `panlee-v15-230208-sample-a` at 10 MHz.**

Directly validated:

- GPIO39 CLK;
- GPIO40 MOSI;
- GPIO38 MISO;
- GPIO41 CS;
- SDSPI card initialization;
- SDHC communication;
- stable operation at the tested 10 MHz clock;
- sector 0 read;
- MBR read/parsing;
- first partition boot-sector read;
- normal application completion without panic, watchdog reset or read timeout.

## Claim boundary

This PASS does **not** validate:

- SD-card write operations;
- FAT filesystem mounting;
- file creation/modification;
- filesystem integrity;
- the integrity or true capacity of the tested SD card;
- hot-plug/card-detect behavior;
- write-protect behavior;
- maximum stable SDSPI clock;
- every SD-card model/capacity;
- all WT32-SC01-PLUS OEM revisions.

If a write-path test is added later, it should use a dedicated expendable SD card under a separately declared destructive-test protocol.
