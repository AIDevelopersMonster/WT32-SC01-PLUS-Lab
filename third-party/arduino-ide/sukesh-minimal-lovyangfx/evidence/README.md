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

## Upstream Sukesh/LovyanGFX validation

Canonical source:

https://gist.github.com/sukesh-ak/610508bc84779a26efdcf969bf51a2d1

The upstream source is not mirrored here because an explicit redistribution license was not identified during this review.

Environment observed during the test:

- Arduino IDE
- Board: `ESP32S3 Dev Module`
- ESP32 Arduino core: `3.3.11`
- esptool: `5.3.1`
- external dependency: LovyanGFX

Before LovyanGFX was installed, compilation failed with `LovyanGFX.hpp: No such file or directory`. After installing LovyanGFX, the original sketch compiled, uploaded, and ran successfully.

Observed upstream results:

| Function | Result |
|---|---|
| Compile/upload | **PASS** |
| ST7796 output | **PASS** |
| Touch controller input | **PASS** |
| Live touch coordinates | **PASS** |
| Touch-drawing trace | **PASS** |
| 16 MB flash detection | **PASS** |
| 2 MB embedded PSRAM detected by esptool | **PASS** |

The upstream runtime displayed PSRAM as `0 bytes` in the tested generic Arduino configuration. This is recorded as a configuration observation, not evidence that the physical board lacks PSRAM.

Video evidence — upstream Sukesh/LovyanGFX:

https://youtube.com/shorts/5CkP_Jh4ofo

## Our WT32_SC01_PLUS RainbowTouch validation

An independently written analogue was then implemented using this repository's board-specific Arduino BSP:

`libraries/WT32_SC01_PLUS/examples/11_RainbowTouch/11_RainbowTouch.ino`

It deliberately keeps all board-specific ST7796, Parallel8, touch-I2C, GPIO, and landscape-coordinate knowledge inside the BSP. Application code reads touch coordinates and draws through the BSP API.

The rainbow trail uses a simple coordinate field:

```text
R = map(X, left -> right, 0 -> 255)
G = map(Y, top -> bottom, 0 -> 255)
B = map(X + Y, near -> far, 255 -> 0)
```

Observed BSP results:

| Function | Result |
|---|---|
| Compile/upload | **PASS** |
| BSP display initialization | **PASS** |
| BSP touch initialization | **PASS** |
| Finger tracking | **PASS** |
| Persistent drawing trail | **PASS** |
| Coordinate-dependent RGB565 color field | **PASS** |
| LovyanGFX required by application | **NO** |
| Application-level LCD/touch GPIO configuration required | **NO** |

Video evidence — our BSP RainbowTouch:

https://youtube.com/shorts/Rl50IJfhhrM

The video shows upload and the real Panlee board running the rainbow touch-trail demonstration.

## Comparison result

The experiment now has physical evidence for both implementations on the same specimen:

```text
Sukesh/LovyanGFX touch draw       = PHYSICAL PASS
WT32_SC01_PLUS BSP RainbowTouch   = PHYSICAL PASS
```

The purpose is not to claim that the upstream implementation is inferior. Studying a working third-party implementation provided an independent compatibility reference and inspired a reusable application-level demonstration that is now part of our BSP examples.

## Release status

`11_RainbowTouch` is included under `libraries/WT32_SC01_PLUS/examples/` and is intended to be packaged in the next Arduino library release ZIP.

## Claim boundary

These results validate the named Panlee specimen and tested software configuration only. They do not certify every WT32-SC01-PLUS OEM revision or every Arduino/LovyanGFX version.
