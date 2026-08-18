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
2. Compile/run the upstream example unchanged on compatible hardware when practical.
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
| Upstream compiled on our specimen in this comparison | NOT YET RECORDED |
| Our BSP hardware profile | PHYSICALLY VALIDATED |
| Our comparison sketch created | YES |
| Side-by-side video | PLANNED |

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
```
