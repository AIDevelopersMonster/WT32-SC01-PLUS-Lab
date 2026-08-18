# Our version

This directory contains independently written comparison implementations using the board-specific Arduino BSP already maintained in this repository.

They do not copy the upstream Gist source.

## Arduino IDE

Install the local `WT32_SC01_PLUS` library (or its generated ZIP release), then open one of the examples below and select:

```text
ESP32S3 Dev Module
```

## Minimal comparison

```text
WT32_SC01_PLUS_Minimal/WT32_SC01_PLUS_Minimal.ino
```

This is the smallest board-level comparison. It initializes the validated BSP, displays a test pattern, reads touch coordinates, and draws simple markers.

## Rainbow Touch Draw

```text
WT32_SC01_PLUS_RainbowTouch/WT32_SC01_PLUS_RainbowTouch.ino
```

This is the direct visual analogue of the upstream touch-drawing demo, but it uses only the `WT32_SC01_PLUS` BSP API at application level.

The user can drag a finger over the display and leave a colored trail. The color is deliberately computed without HSV conversion, palettes, or lookup tables. A simple RGB field is stretched across the touchscreen using Arduino `map()`:

```text
R = map(X,       left  -> right, 0   -> 255)
G = map(Y,       top   -> bottom, 0  -> 255)
B = map(X + Y,   near  -> far,   255 -> 0)
```

The three 8-bit components are then packed directly into RGB565 for the ST7796 display.

This produces a continuous position-dependent rainbow-like color field while keeping the sketch easy to explain on camera.

The brush is a small filled square drawn with `board.display().fillRect()`. Coordinate clipping at screen edges is handled by the BSP display implementation.

## Why this is useful for the comparison

The application intentionally contains no:

- ST7796 initialization sequence;
- 8-bit I80 GPIO table;
- LovyanGFX panel or bus classes;
- backlight pin/PWM configuration;
- FT5x06/FT6336U I2C pin mapping;
- landscape touch-coordinate transform.

Those board-specific details are already encapsulated in `WT32_SC01_PLUS`.

At application level the essential loop becomes conceptually:

```cpp
WT32_SC01_PLUS_TouchPoint point;

if (board.touch().read(point) && point.touched) {
    uint16_t color = colorFromTouch(point.x, point.y);
    board.display().fillRect(point.x - 4, point.y - 4, 9, 9, color);
}
```

That makes this example suitable for a side-by-side video comparison with the original Sukesh LovyanGFX sketch: similar visible behavior, but board configuration is no longer repeated in the application.

## Validation status

Both sides of the comparison have now been physically executed on the same reference Panlee specimen:

- Upstream Sukesh/LovyanGFX touch-drawing example: **PHYSICAL PASS**.
- `WT32_SC01_PLUS_RainbowTouch`: **PHYSICAL PASS**.

Observed for the BSP version on 2026-08-18:

- sketch compiled and uploaded successfully;
- display initialized correctly;
- touch input worked across the screen;
- finger motion produced a persistent trail;
- RGB565 trail color changed continuously with touch position according to the simple `map()` field;
- no LovyanGFX dependency or application-level LCD/touch pin configuration was required.

The reusable library example is also available as:

```text
libraries/WT32_SC01_PLUS/examples/11_RainbowTouch/11_RainbowTouch.ino
```

## What this comparison proves — and what it does not

It demonstrates an architectural difference:

- upstream minimal sketch: application owns the board/display/touch configuration;
- our version: application consumes a board-level API whose hardware profile has already been validated.

It does **not** claim that LovyanGFX or the upstream sketch is inferior. LovyanGFX is a powerful general display framework, and the upstream Gist is a useful compact hardware reference. The BSP approach becomes advantageous when many Arduino applications target the same validated board profile.
