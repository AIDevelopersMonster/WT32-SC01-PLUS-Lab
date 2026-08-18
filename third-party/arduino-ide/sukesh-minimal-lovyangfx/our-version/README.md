# Our version

This directory contains an independently written comparison implementation using the board-specific Arduino BSP already maintained in this repository.

It does not copy the upstream Gist source.

## Arduino IDE

Install the local `WT32_SC01_PLUS` library (or its generated ZIP release), then open:

```text
WT32_SC01_PLUS_Minimal/WT32_SC01_PLUS_Minimal.ino
```

Select:

```text
ESP32S3 Dev Module
```

The application intentionally contains no ST7796 bus pin table, LovyanGFX panel class, backlight PWM configuration, or FT5x06 I2C configuration. Those board details belong to the reusable BSP.

## What this comparison proves — and what it does not

It demonstrates an architectural difference:

- upstream minimal sketch: application owns the board/display/touch configuration;
- our version: application consumes a board-level API whose hardware profile has already been validated.

It does **not** claim that LovyanGFX or the upstream sketch is inferior. LovyanGFX is a powerful general display framework, and the upstream Gist is a useful compact hardware reference. The BSP approach becomes advantageous when many Arduino applications target the same validated board profile.
