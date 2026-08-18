# Our version

This directory contains independently written comparison implementations using the board-specific Arduino BSP maintained in this repository. They do not copy the upstream Gist source.

## Arduino IDE

Install the local `WT32_SC01_PLUS` library (or its generated ZIP release), select `ESP32S3 Dev Module`, and open one of the examples below.

## Minimal comparison

`WT32_SC01_PLUS_Minimal/WT32_SC01_PLUS_Minimal.ino`

A small board-level example that initializes the validated BSP, shows a display pattern, reads touch coordinates, and draws markers.

## Rainbow Touch Draw

`WT32_SC01_PLUS_RainbowTouch/WT32_SC01_PLUS_RainbowTouch.ino`

This is the direct visual analogue of the upstream Sukesh touch-drawing demo, but it uses only the `WT32_SC01_PLUS` BSP API at application level.

The user drags a finger over the display and leaves a colored trail. Color is computed without HSV conversion, palettes, or lookup tables:

```text
R = map(X,       left -> right, 0 -> 255)
G = map(Y,       top -> bottom, 0 -> 255)
B = map(X + Y,   near -> far,   255 -> 0)
```

The components are packed directly into RGB565. The brush is drawn with `board.display().fillRect()`, while touch coordinates are obtained from `board.touch().read()`.

The application contains no ST7796 initialization sequence, Parallel8 GPIO table, LovyanGFX panel/bus classes, FT5x06/FT6336U pin mapping, or landscape coordinate transform. Those board details are encapsulated in the BSP.

## Physical validation

Both sides of the comparison were physically executed on the same Panlee reference specimen (`ZX3D50CE08S-V15-USRC / 230208`) on 2026-08-18.

- Upstream Sukesh/LovyanGFX touch drawing: **PHYSICAL PASS**.
- `WT32_SC01_PLUS_RainbowTouch`: **PHYSICAL PASS**.

Observed for the BSP version:

- successful compile and upload;
- display initialized correctly;
- touch input worked across the screen;
- finger motion produced a persistent trail;
- RGB565 trail color changed continuously with touch position;
- no LovyanGFX dependency or application-level LCD/touch pin configuration was required.

## Video evidence

Upstream Sukesh/LovyanGFX demonstration:

https://youtube.com/shorts/5CkP_Jh4ofo

Our BSP `RainbowTouch` demonstration:

https://youtube.com/shorts/Rl50IJfhhrM

The second video shows compilation/upload and the real board running the coordinate-mapped rainbow trail.

## Library inclusion

The reusable version is included in the Arduino BSP as:

`libraries/WT32_SC01_PLUS/examples/11_RainbowTouch/11_RainbowTouch.ino`

Because it is part of the library `examples/` tree, it is intended to be packaged in the next `WT32_SC01_PLUS` Arduino release ZIP.

## Claim boundary

The comparison demonstrates an architectural difference, not that LovyanGFX or the upstream sketch is inferior. The upstream Gist is a useful compact hardware reference; the BSP approach is useful when many applications reuse the same validated board profile.
