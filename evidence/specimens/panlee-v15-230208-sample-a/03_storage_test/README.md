# 03_storage_test — physical validation protocol

**Subsystem status:** `PHYSICAL PASS`  
**Read-path status:** `PASS`  
**Full-media write/verify status:** `PASS`  
**Reference specimen:** `panlee-v15-230208-sample-a`  
**Board marking:** Panlee / `ZX3D50CE08S-V15-USRC` / `230208`

This evidence directory now contains two distinct SD validation runs:

1. the original read-only ESP-IDF storage probe on a card with contradictory partition geometry;
2. the later autonomous Arduino full-media destructive qualification on a separate 8 GB card.

The two cards and conclusions must not be conflated.

## Physically validated SDSPI mapping

| Signal | GPIO |
|---|---:|
| SD CLK | 39 |
| SD MOSI / DI | 40 |
| SD MISO / DO | 38 |
| SD CS | 41 |

Validated clock for the documented runs: **10 MHz**.

---

## Run A — read-only board-path validation

**Date:** 2026-08-15  
**Project:** [`examples/03_storage_test`](../../../../examples/03_storage_test/)  
**ESP-IDF:** `v6.0.2`  
**Result:** `PASS WITH MEDIA ANOMALY`

This run deliberately avoided all writes and validated the physical SDSPI read path independently of the factory firmware.

Observed card data:

```text
Name: SD
Type: SDHC
Speed: 10.00 MHz
Size: 52000MB
Sector size: 512
CSD capacity: 106496000 sectors
```

Sector 0 and the first partition boot sector were read successfully. The MBR declared:

```text
Partition 1 type       : 0x0C
Start LBA              : 2048
Sector count           : 125827072
End exclusive          : 125829120
```

The partition boot sector was FAT32-compatible:

```text
Signature              : 0x55AA
OEM/system field       : MSDOS5.0
Bytes per sector       : 512
Sectors per cluster    : 64
Reserved sectors       : 38
FAT count              : 2
Filesystem hint        : FAT32
```

The geometry audit found:

```text
CSD addressable sectors : 106496000
MBR maximum end         : 125829120
MBR vs CSD geometry     : WARNING - PARTITION EXTENT EXCEEDS CSD CAPACITY
```

Therefore this specific media was classified as anomalous, while the board-side read path was PASS.

No write-path claim was made from Run A.

---

## Run B — autonomous full-media destructive qualification

**Date:** 2026-08-16  
**Arduino example:** [`libraries/WT32_SC01_PLUS/examples/04_SDDestructiveTest`](../../../../libraries/WT32_SC01_PLUS/examples/04_SDDestructiveTest/)  
**Interface:** on-board LCD + touch; Serial diagnostic only  
**Test card:** separate 8 GB-class SD card  
**Detected capacity:** `15728640` sectors × 512 bytes = `7680 MiB`  
**Result:** `PHYSICAL PASS`

The test was intentionally destructive and operated across the entire card-reported LBA range using 32 KiB multi-sector transfers:

```text
64 sectors × 512 bytes = 32768 bytes per transfer
```

Qualification sequence:

```text
1/7  WRITE  0x00 over full media
2/7  VERIFY 0x00 by full readback and byte comparison
3/7  WRITE  0xAA over full media
4/7  VERIFY 0xAA by full readback and byte comparison
5/7  WRITE  0x55 over full media
6/7  VERIFY 0x55 by full readback and byte comparison
7/7  restore FAT + probe-file write/read/delete
```

Final on-device result:

```text
PASS
00 AA 55 VERIFIED
FAT RESTORED
CARD EMPTY AND READY
7680 MiB / 15728640 SECTORS
```

The final PASS photograph is stored as:

[`arduino-sd-destructive-full-pass-8gb.jpg`](./arduino-sd-destructive-full-pass-8gb.jpg)

### What Run B physically validates

- SDSPI writes through GPIO39/40/38/41 at 10 MHz;
- multi-sector 32 KiB write transfers;
- multi-sector 32 KiB read transfers;
- full-card `0x00` write + byte-for-byte readback;
- full-card `0xAA` write + byte-for-byte readback;
- full-card `0x55` write + byte-for-byte readback;
- successful FAT restoration after destructive raw testing;
- successful probe-file write;
- successful probe-file readback/verification;
- successful probe-file deletion;
- autonomous LCD/touch operator flow through completion.

### Final media state

After PASS, the 8 GB test card is **not** left filled with `0x55`. The filesystem is restored and the temporary probe file is removed, leaving the card empty and ready for normal use.

---

## Physical conclusion

For the named Panlee V15 / 230208 specimen:

```text
SDSPI physical mapping           : PASS
Read path @ 10 MHz               : PASS
Write path @ 10 MHz              : PASS
Full-media 00/AA/55 verification : PASS (8 GB test card)
FAT restoration                  : PASS
Autonomous LCD/touch workflow    : PASS
```

The earlier 52 GB-class card retains its documented geometry warning. The later 8 GB card was used specifically for the full destructive qualification and completed the entire test successfully.

## Claim boundary

These results do **not** establish:

- integrity of the earlier anomalous 52 GB-class card;
- maximum stable SDSPI clock;
- hot-plug/card-detect behavior;
- write-protect behavior;
- correctness of every SD-card make/model/capacity;
- identical pinout or behavior across all WT32-SC01-PLUS OEM revisions.

The PASS is hardware- and specimen-specific to the declared Panlee `ZX3D50CE08S-V15-USRC / 230208` reference board and the explicitly tested cards.