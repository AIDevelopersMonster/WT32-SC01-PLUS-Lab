# 04_SDDestructiveTest

Full-media destructive SD qualification for the validated Panlee V15 / 230208 SDSPI path.

## WARNING

This test permanently destroys every partition, filesystem and file on the inserted SD card.

It does not start writing automatically. After initialization it requires the operator to type exactly:

```text
ERASE-ALL-SD
```

Any other input aborts before the first raw write.

## Test sequence

The test uses the raw card-reported LBA range from `SD.numSectors()` and 512-byte sectors.

It performs three complete media passes:

1. write `0x00` to every sector;
2. read every sector and verify every byte is `0x00`;
3. write `0xAA` to every sector;
4. read every sector and verify every byte is `0xAA`;
5. write `0x55` to every sector;
6. read every sector and verify every byte is `0x55`.

A single read error, write error or byte mismatch stops the test and reports the first failing LBA and byte offset when available.

The final successful media state is all `0x55`; no FAT partition or filesystem remains.

## Hardware path

```text
SCK  = GPIO39
MOSI = GPIO40
MISO = GPIO38
CS   = GPIO41
SDSPI clock = 10 MHz
```

These pins are physically validated for the Panlee `ZX3D50CE08S-V15-USRC / 230208` specimen.

## Why this test exists

The read-only Arduino storage test passed, but the current card reports contradictory geometry: raw card capacity is about 52000 MiB while the FAT volume reports about 61423 MiB. A complete raw write/readback test can determine whether all LBAs claimed by the card are genuinely writable and independently readable.

This test validates the reported raw LBA range only. It cannot prove hidden capacity beyond the card-reported range.

## Important runtime implications

For the currently observed card:

```text
106,496,000 sectors x 512 bytes = about 54.5 GB raw
```

Three full writes plus three full reads transfer roughly 327 GB. At a 10 MHz SDSPI interface the run can take many hours. Keep the board and card powered and thermally stable.

## Expected successful result

```text
SD FULL-MEDIA DESTRUCTIVE QUALIFICATION PASS
0x00 WRITE+VERIFY : PASS
0xAA WRITE+VERIFY : PASS
0x55 WRITE+VERIFY : PASS
```

## After the test

The card will not contain a usable filesystem. To reuse it normally, repartition and format it on a PC or with a separately controlled formatting procedure.
