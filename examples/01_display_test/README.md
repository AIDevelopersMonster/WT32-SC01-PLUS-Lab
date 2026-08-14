# 01_display_test

Second executable hardware-validation firmware for the reference **Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC / 230208** specimen.

**Current status:** `PASS — PHYSICALLY VALIDATED`

This test is deliberately narrower than an LVGL demo. It validates only the LCD path: ESP32-S3 I80 bus, ST7796 initialization, backlight and visible RGB565 output. Touch, TE synchronization, SD, audio, RS-485, Wi-Fi and BLE are not initialized.

Physical validation protocol:

- [`../../evidence/specimens/panlee-v15-230208-sample-a/01_display_test/README.md`](../../evidence/specimens/panlee-v15-230208-sample-a/01_display_test/README.md)

## Evidence basis and result

For this exact specimen, the verified factory firmware had already established strong evidence for:

- ST7796 driver strings and code;
- an ESP32-S3 8080 LCD bus implementation;
- a 480x320 embedded display resource;
- a physical factory LCD test;
- a physically observed `MADCTL=0x28`, consistent with BGR plus axis swap for 480x320 landscape operation.

`01_display_test` then independently initialized and drove the real display from ESP-IDF 6.0.2. The operator visually confirmed correct repeated pattern operation, so HW-02 is now PASS for this specimen.

## Physically validated display pin map

| Signal | GPIO |
|---|---:|
| BL | 45 |
| RESET | 4 |
| DC / RS | 0 |
| WR | 47 |
| TE | 48 — documented but unused in this test |
| D0 | 9 |
| D1 | 46 |
| D2 | 3 |
| D3 | 8 |
| D4 | 18 |
| D5 | 17 |
| D6 | 16 |
| D7 | 15 |
| CS | `-1` — tied/always selected in this test model |

The actively used LCD mapping above is now direct physical evidence for `panlee-v15-230208-sample-a`. TE itself remains unvalidated because this test does not use it.

## Software configuration

The project deliberately does **not** keep a separate `sdkconfig.defaults` copy. Its top-level CMake file loads the centralized, physically validated profile:

```text
software/espressif/config/panlee-v15-230208-sample-a.idf6.0.2.sdkconfig.defaults
```

That profile establishes:

```text
CONFIG_IDF_TARGET="esp32s3"
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_SPIRAM=y
```

The ST7796 driver is pinned through ESP-IDF Component Manager as:

```yaml
espressif/esp_lcd_st7796: "1.4.0"
```

## Validated first-run settings

The following settings worked on the reference specimen during this test:

- I80 width: 8 bit;
- I80 pixel clock: **10 MHz**;
- RGB565, 16 bits/pixel;
- color order: BGR;
- `swap_xy=true`;
- no mirror;
- backlight active high;
- TE unused;
- draw buffer: 20 lines in internal DMA-capable RAM.

BGR (`0x08`) plus ST7796 MV/axis-swap (`0x20`) is intended to reproduce the physically observed `MADCTL=0x28` state.

These are validated operating settings for this specimen, not a claim about maximum panel speed or every OEM revision.

## Visual test sequence

The firmware continuously cycles through:

1. solid black;
2. solid white;
3. solid red;
4. solid green;
5. solid blue;
6. eight vertical color bars;
7. left-to-right grayscale;
8. orientation/geometry screen for four seconds.

The final geometry screen should show:

```text
+------------------------------------------------+
| RED                                       GREEN |
|                                                |
|                 yellow cross                   |
|                                                |
| BLUE                                      WHITE |
+------------------------------------------------+
```

It also contains a cyan outer border and a gray 40-pixel grid.

## Physical PASS criteria — satisfied

The reference specimen visibly demonstrated the intended recurring test screens, while the serial log repeatedly completed all eight patterns without LCD DMA timeout, panic, watchdog reset or controller initialization error.

PASS therefore covers:

- backlight operation after LCD initialization;
- active display output over the full intended 480x320 landscape area;
- black, white, red, green and blue output;
- color bars and grayscale pattern output;
- orientation/geometry diagnostic output;
- repeated stable I80 transfers at 10 MHz during the observed run.

## Build

From the repository root in an activated ESP-IDF 6.0.2 shell:

```powershell
git switch agent/01-display-test
cd examples\01_display_test
idf.py fullclean
idf.py build
```

On the first build, ESP-IDF Component Manager may download the pinned ST7796 component and its dependencies.

## Flash and monitor

```powershell
idf.py -p COM10 flash monitor
```

The firmware intentionally loops forever through the visual patterns.

To exit only `idf_monitor`:

```text
Ctrl+]
```

or `Ctrl+T`, then `Ctrl+X`.

Exiting the monitor does **not** stop the display loop. To stop the firmware itself, remove power or flash another application. RESET simply starts this same test again from the beginning.

## Evidence

Canonical physical-run protocol:

```text
evidence/specimens/panlee-v15-230208-sample-a/01_display_test/README.md
```

The protocol records application/tool versions, pin map, I80 settings, build/boot results, visual sequence, operator confirmation, stop behavior and claim boundaries.

## Claim boundary

This PASS belongs specifically to `panlee-v15-230208-sample-a`. It does not yet validate:

- touch-controller identity or coordinates;
- TE synchronization;
- maximum stable I80 clock;
- LVGL integration;
- all WT32-SC01-PLUS OEM or PCB revisions.
