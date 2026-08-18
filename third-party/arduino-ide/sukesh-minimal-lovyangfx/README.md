# Sukesh minimal LovyanGFX example — Arduino IDE comparison

## Why this project

This is the first third-party comparison target for the WT32-SC01-PLUS video series because it is small enough to understand on camera and directly exposes the amount of board-specific configuration normally required.

Upstream Gist:

- Author: Sukesh Akhilesh (`sukesh-ak`)
- Project: `WT32-SC01-Plus_ESP32-S3.ino`
- URL: https://gist.github.com/sukesh-ak/610508bc84779a26efdcf969bf51a2d1
- Purpose: simple touch-drawing test for WT32-SC01-PLUS
- Arduino board selection: `ESP32S3 Dev Module`
- Display library: LovyanGFX
- Display controller: ST7796
- Bus: 8-bit MCU8080 / `Bus_Parallel8`
- Touch: FT5x06-family LovyanGFX driver
- Snapshot checked: 2026-08-18
- Gist activity observed: last active 2026-06-10; 16 stars and 2 forks at the checked snapshot

## License status

No explicit license was identified on the Gist page during this review.

Therefore this repository does **not** copy the upstream sketch. The upstream directory contains documentation and links only. Our comparison sketch in `our-version/` is independently written against this repository's own Arduino BSP.

If the upstream author later publishes an explicit license, this note can be updated and a properly attributed snapshot may be added if useful.

## What the upstream sketch demonstrates

The upstream example configures the board directly inside the sketch. In particular, it constructs LovyanGFX objects for:

- `Panel_ST7796`
- `Bus_Parallel8`
- PWM backlight
- `Touch_FT5x06`

It also embeds the board's LCD data/control pins and touch configuration in the application.

That is a useful reference implementation because the hardware configuration is visible and inspectable, but it means the application sketch carries board-support details that every new project would otherwise have to repeat or factor out manually.

## Physical validation — 2026-08-18

The original upstream sketch was compiled and flashed on the reference Panlee specimen and was observed working on real hardware.

Tested specimen:

```text
Panlee WT32-SC01-PLUS
ZX3D50CE08S-V15-USRC
230208
ESP32-S3 revision v0.2
```

Observed result:

| Item | Result |
|---|---|
| Arduino compilation | **PASS** |
| Upload via USB Serial/JTAG | **PASS** |
| ST7796 display output | **PHYSICAL PASS** |
| 8-bit parallel display path | **PHYSICAL PASS** |
| Capacitive touch input | **PHYSICAL PASS** |
| Touch coordinates | **PHYSICAL PASS** |
| Touch drawing demo | **PHYSICAL PASS** |
| Flash detected | **16 MB** |
| Hardware PSRAM detection by esptool | **2 MB embedded PSRAM** |
| PSRAM reported by the sketch/runtime | **0 bytes in the tested Arduino configuration** |

The PSRAM discrepancy is retained as an environment/configuration observation. `esptool` identifies the ESP32-S3 as having `Embedded PSRAM 2MB (AP_3v3)`, while the running sketch reported `PSRAM Size: 0 bytes`. This does **not** establish absence of PSRAM on the board; it indicates that PSRAM was not exposed/initialized in the tested generic Arduino board configuration.

### Upload evidence

The successful Arduino upload reported:

```text
Sketch uses 366715 bytes (27%) of program storage space.
Maximum is 1310720 bytes.
Global variables use 24484 bytes (7%) of dynamic memory.
Maximum is 327680 bytes.

Chip type: ESP32-S3 (QFN56) (revision v0.2)
Features: Wi-Fi, BT 5 (LE), Dual Core + LP Core, 240MHz,
          Embedded PSRAM 2MB (AP_3v3)
Crystal frequency: 40MHz
USB mode: USB-Serial/JTAG
```

All written images were hash-verified by `esptool`, followed by a hard reset.

## LovyanGFX dependency validation

The upstream sketch requires LovyanGFX and includes:

```cpp
#include <LovyanGFX.hpp>
```

On a clean Arduino IDE setup the first compile failed with:

```text
fatal error: LovyanGFX.hpp: No such file or directory
```

After installing the LovyanGFX library, the same upstream sketch compiled successfully, flashed successfully, and produced working display and touch output on the reference Panlee board.

Therefore LovyanGFX is not only a documented upstream dependency here; its use with this sketch has been **physically validated on the tested specimen**.

## Video evidence

YouTube Shorts — physical demonstration of the original Sukesh sketch on the Panlee WT32-SC01-PLUS:

https://youtube.com/shorts/5CkP_Jh4ofo

The video demonstrates the running display and touch-drawing behavior on the real board.

Detailed validation notes are also recorded in [`evidence/README.md`](evidence/README.md).

## Our comparison

The local project already contains a physically validated Arduino BSP for the Panlee specimen:

```text
libraries/WT32_SC01_PLUS
```

Validated hardware profile:

```text
Panlee
ZX3D50CE08S-V15-USRC
230208
ESP32-S3
ST7796 480x320
8-bit I80
```

The comparison therefore asks a simple question:

> How much board-specific setup does an Arduino user need in the application when the hardware knowledge is moved into a reusable BSP?

Our independent example is in:

```text
our-version/WT32_SC01_PLUS_Minimal/WT32_SC01_PLUS_Minimal.ino
```

It intentionally keeps application code small and leaves display, backlight and touch initialization to `WT32_SC01_PLUS`.

## Video concept

Working title:

**WT32-SC01 Plus в Arduino IDE: действительно ли всё так сложно?**

Suggested sequence:

1. Show the original Gist and explain why it is valuable.
2. Show the physically validated upstream example running on the Panlee board.
3. Highlight the board-specific LovyanGFX configuration embedded in the sketch.
4. Run our BSP-based sketch.
5. Compare application-level code size and readability.
6. Explain that the goal is not to claim the upstream approach is wrong: the Gist is a useful hardware reference, while a BSP is better when many applications reuse the same board.
7. Link both the upstream Gist and this repository in the video description.

## Test status

| Item | Status |
|---|---|
| Upstream source reviewed | YES |
| Upstream source mirrored here | NO — license not identified |
| LovyanGFX dependency installed and tested | **YES — PHYSICAL PASS** |
| Upstream compiled on our Panlee specimen | **YES — PASS** |
| Upstream flashed on our Panlee specimen | **YES — PASS** |
| Upstream LCD/touch demo | **YES — PHYSICAL PASS** |
| Video evidence | **YES** |
| Our BSP hardware profile | PHYSICALLY VALIDATED |
| Our comparison sketch created | YES |
| Our comparison sketch physically tested in this comparison | NOT YET RECORDED |
| Side-by-side comparison video | PLANNED |

## Hardware-variant warning

WT32-SC01-PLUS is an OEM family rather than a guarantee that every board marked with the product name is electrically identical. Our BSP profile is specifically validated on the Panlee `ZX3D50CE08S-V15-USRC / 230208` specimen. Any upstream sketch should be checked against the actual board revision before assuming full compatibility.

## Directory layout

```text
sukesh-minimal-lovyangfx/
  README.md
  upstream/
    README.md
  our-version/
    README.md
    WT32_SC01_PLUS_Minimal/
      WT32_SC01_PLUS_Minimal.ino
  releases/
    README.md
  evidence/
    README.md
```
