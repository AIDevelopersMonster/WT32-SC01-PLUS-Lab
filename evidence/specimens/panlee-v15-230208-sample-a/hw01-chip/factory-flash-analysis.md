# Factory flash analysis — panlee-v15-230208-sample-a

**Acceptance stage:** HW-01  
**Analysis date:** 2026-08-13  
**Specimen:** `panlee-v15-230208-sample-a`  
**Board markings:** Panlee / `ZX3D50CE08S-V15-USRC` / `230208`

## Status

**FACTORY_FLASH_BACKUP = VERIFIED**

Two complete 16 MiB reads were performed with `esptool v5.3.1` using ROM download mode with `--no-stub`. The two files produced the same SHA-256 digest, so the captured factory flash is verified bit-for-bit across two independent reads.

```text
Dump #1 SHA-256:
3772C1BF7D6D2B713973212DDF5C671E3C844A13A8464F675343D9AED4E7F044

Dump #2 SHA-256:
3772C1BF7D6D2B713973212DDF5C671E3C844A13A8464F675343D9AED4E7F044

Match: TRUE
Size: 16,777,216 bytes (0x01000000)
```

The full factory binary is intentionally **not committed to this public repository**. It is third-party/vendor firmware and no redistribution permission has been established. The repository records reproducible metadata, hashes and analysis instead.

## Acquisition command

The full flash was successfully read with:

```powershell
python -m esptool `
  --chip esp32s3 `
  --port COM10 `
  --baud 921600 `
  --no-stub `
  read-flash --flash-size 16MB `
  0 0x1000000 `
  board-info\factory-flash-16mb.bin
```

A second read was made to `factory-flash-16mb-verify.bin` and the SHA-256 hashes were compared.

The serial port and full MAC address are intentionally not treated as portable board-family properties; public evidence should avoid publishing private identifier inventories unnecessarily.

## Flash image integrity

Static analysis of the uploaded verified image produced the following checks.

| Check | Result |
|---|---|
| File size | `0x01000000` / 16 MiB |
| Full-file SHA-256 | `3772c1bf7d6d2b713973212ddf5c671e3c844a13a8464f675343d9aed4e7f044` |
| Partition table location | `0x8000` |
| Partition-table MD5 | PASS |
| Partition-table MD5 | `d57aaa4e8317ed5f61fab3df96817446` |
| Bootloader image checksum | PASS |
| Bootloader appended SHA-256 | PASS |
| Factory application checksum | PASS |
| Factory application appended SHA-256 | PASS |
| Last non-`0xFF` byte | `0x20147F` |
| First byte after application image | `0x201480` |

## Partition table

The image contains a small, valid ESP-IDF partition table:

| Label | Type / subtype | Offset | Size | Observation |
|---|---|---:|---:|---|
| `nvs` | data / nvs | `0x009000` | `0x006000` (24 KiB) | all `0xFF` in captured image |
| `phy_init` | data / phy | `0x00F000` | `0x001000` (4 KiB) | all `0xFF` in captured image |
| `factory` | app / factory | `0x010000` | `0x400000` (4 MiB) | factory application |

No OTA application slots and no separate SPIFFS/LittleFS data partition are present in this captured partition table.

Although the physical flash is 16 MiB, this factory partition layout only allocates the first part of the device. The application image ends at `0x201480`; all bytes from that address through `0x00FFFFFF` are `0xFF` in the verified dump.

## Bootloader image

The bootloader starts at flash offset `0x000000` and is a valid ESP image.

```text
Segments: 3
Entry:    0x403C9980
Image end: 0x000057A0
Appended image SHA-256:
7cf42dda75183bb977b6db546597ff5fa5ce0dd031fbb9d781ca8a5782c22cf6
```

The bootloader contains the string:

```text
ESP-IDF v4.4.4-dirty
```

## Factory application metadata

The factory application begins at `0x10000`. Its ESP application descriptor is valid and contains:

```text
Project name:   get-start
App version:    1
Compile time:   14:32:37
Compile date:   Feb 14 2023
ESP-IDF:        v4.4.4-dirty
Secure version: 0
ELF SHA-256:    77b58b48aabb0a2d59e4ab81b31de1d88bb866bcf6a690f7b5e3dcf1631a956e
```

The serialized application image itself has a valid checksum and valid appended image SHA-256:

```text
Application image end: 0x201480
Appended image SHA-256:
57f6b71db40f007a5607c37a508823b47870be92cf397b1484e6318068b087bc
```

## QMSD / 8MS provenance evidence

The binary contains repeated QMSD identifiers and retained build paths, including:

```text
QMSD_BOARD
qmsd_board_backlight_init
qmsd certificated!
http://hmi.8ms.xyz
/home/sorz/code_repository/esp32-8ms-v3/components/qmsd_board/
/home/sorz/code_repository/esp32-8ms-v3/components/qmsd_gui/
```

It also contains audio paths such as:

```text
components-ext/qmsd_audio/mp3player/mp3_player.c
mp3_player
mp3player_task
audio_mp3
```

**Interpretation:** the captured firmware was built from a QMSD/8MS-oriented ESP32-S3 software tree using ESP-IDF 4.4.4. This is a firmware-level provenance observation; it is not a statement about ownership or licensing of the source code.

## Display evidence inside the firmware

The application contains a compiled ST7796 driver and ESP32-S3 8080-LCD bus implementation. Retained strings include:

```text
components-third-party/screen/controller_driver/st7796/st7796.c
lcd st7796
ST7796
lcd_st7796_draw_bitmap
lcd_st7796_set_window
lcd_st7796_set_rotation
lcd_st7796_init
components-third-party/bus/8080_lcd_esp32s3.c
```

A PNG resource embedded in the application begins at flash offset `0x13424`. Its IHDR declares:

```text
Width:  480
Height: 320
```

**Evidence ceiling:** this proves that the factory firmware was built to drive a 480×320 ST7796/8080 display configuration. It strongly supports the expected display configuration, but physical controller identity still belongs to HW-02 hardware acceptance and should not be promoted to a specimen PASS from binary analysis alone.

## Touch evidence inside the firmware

The application contains the driver identifier:

```text
FT5x06
```

**Evidence ceiling:** the factory firmware includes an FT5x06-family touch driver. This does not by itself prove that the exact physical controller is FT6336U. Exact touch-controller identification remains an HW-03 task.

## Factory/demo test behaviour

Strings close to the embedded display resources strongly indicate that the captured image is a factory/demo test application rather than an ordinary end-user application:

```text
Enter test mode
Fine qmsd!
IO Test
SD Test
USB Con
USB Dis
```

The binary also contains SD-card and MP3/audio-related code. This is consistent with a board validation/demo firmware exercising multiple onboard features.

## NVS observation

The complete `nvs` partition (`0x9000`–`0xEFFF`) is `0xFF` in the captured image. The `phy_init` partition is also `0xFF`.

Therefore this particular captured factory state does not contain persisted NVS records in that partition. No Wi-Fi credentials were identified there.

## Simplified flash map

```text
0x000000
  bootloader
  valid ESP image, ESP-IDF v4.4.4-dirty
  image ends at 0x0057A0

0x008000
  partition table
  MD5 PASS

0x009000 - 0x00EFFF
  nvs, 24 KiB
  all 0xFF

0x00F000 - 0x00FFFF
  phy_init, 4 KiB
  all 0xFF

0x010000
  factory application
  project: get-start
  ESP-IDF v4.4.4-dirty
  QMSD / 8MS components
  ST7796 + ESP32-S3 8080 display code
  FT5x06-family touch code
  SD/audio/test UI code
  application image ends at 0x201480

0x201480 - 0xFFFFFF
  all 0xFF
```

## Recovery value

Because this is a full physical-flash capture beginning at address `0x000000`, the archived binary can serve as a specimen-specific recovery image if the board is later erased or experimental firmware is installed.

Do not assume this image is appropriate for other WT32-SC01-PLUS/Panlee revisions. It belongs specifically to `panlee-v15-230208-sample-a`.

## Public/private storage policy

### Public repository

Keep:

- this analysis;
- SHA-256 hashes;
- sanitized chip/flash/eFuse summaries;
- commands required to reproduce acquisition;
- derived partition/application metadata.

### Private/local archive

Keep the actual vendor binary, for example:

```text
board-info/
└── factory-flash/
    └── panlee-v15-230208-sample-a/
        ├── factory-flash-16mb.bin
        ├── factory-flash-16mb-verify.bin   # optional after verification
        └── factory-flash-16mb.sha256.txt
```

`board-info/` and `*.bin` are already ignored by this repository's `.gitignore`.

For long-term recovery, keep at least one additional offline/private copy of the verified binary together with its SHA-256 hash.

If redistribution rights for the factory firmware are later established, a **GitHub Release asset** is preferable to committing a 16 MiB binary into normal Git history.
