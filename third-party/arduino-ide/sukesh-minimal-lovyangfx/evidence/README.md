# Physical validation evidence

## Test date

2026-08-18

## Target hardware

```text
Panlee WT32-SC01-PLUS
ZX3D50CE08S-V15-USRC
230208
ESP32-S3 (QFN56), revision v0.2
```

## Upstream target

Sukesh Akhilesh minimal WT32-SC01-PLUS Arduino/LovyanGFX touch-drawing Gist:

https://gist.github.com/sukesh-ak/610508bc84779a26efdcf969bf51a2d1

The upstream source is not mirrored here because an explicit redistribution license was not identified during this review.

## Environment

- Arduino IDE
- Board selection: `ESP32S3 Dev Module`
- ESP32 Arduino core observed during upload: `3.3.11`
- `esptool` observed during upload: `5.3.1`
- Required external display/touch library: LovyanGFX

## Dependency check

Before LovyanGFX was installed, compilation failed as expected with:

```text
fatal error: LovyanGFX.hpp: No such file or directory
```

After installing LovyanGFX, the original sketch compiled and uploaded successfully. The resulting display and touch demo then ran on the physical board.

This establishes a practical compatibility result for the tested combination:

```text
Panlee WT32-SC01-PLUS
+ Arduino IDE / ESP32S3 Dev Module
+ LovyanGFX
+ Sukesh minimal touch-drawing sketch
= PHYSICAL PASS
```

## Compile/upload record

```text
Sketch uses 366715 bytes (27%) of program storage space.
Maximum is 1310720 bytes.
Global variables use 24484 bytes (7%) of dynamic memory,
leaving 303196 bytes for local variables.
Maximum is 327680 bytes.

Connected to ESP32-S3 on COM10:
Chip type:          ESP32-S3 (QFN56) (revision v0.2)
Features:           Wi-Fi, BT 5 (LE), Dual Core + LP Core, 240MHz,
                    Embedded PSRAM 2MB (AP_3v3)
Crystal frequency:  40MHz
USB mode:           USB-Serial/JTAG
MAC:                48:27:e2:1f:30:5c
```

The bootloader, partition table, `boot_app0`, and application image were written successfully. `esptool` verified the hash of each written image and completed with:

```text
Hard resetting via RTS pin...
```

## Physical observations

| Function | Result |
|---|---|
| Display initialization | **PASS** |
| ST7796 image output | **PASS** |
| Touch controller input | **PASS** |
| Live touch coordinates | **PASS** |
| Touch-drawing trace | **PASS** |
| 16 MB flash detection in sketch | **PASS** |
| 2 MB embedded PSRAM detected by esptool | **PASS** |
| PSRAM exposed to sketch in tested generic Arduino configuration | **NO — sketch displayed 0 bytes** |

The `0 bytes` runtime PSRAM result is recorded as a configuration observation, not as evidence that the physical board lacks PSRAM. The upload-time chip probe explicitly detected 2 MB embedded PSRAM.

## Video evidence

Physical demonstration:

https://youtube.com/shorts/5CkP_Jh4ofo

The video shows the real WT32-SC01-PLUS running the upstream touch-drawing demonstration, including active display output and touch response.

## Claim boundary

This evidence validates the named Panlee specimen and the tested software configuration. It does not certify every WT32-SC01-PLUS OEM revision, every LovyanGFX release, every Arduino ESP32 core version, or PSRAM configuration.
