# 01_display_test

Second executable hardware-validation firmware for the reference **Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC / 230208** specimen.

**Current status:** `READY FOR BUILD / NOT YET PHYSICALLY VALIDATED`

This test is deliberately narrower than an LVGL demo. It validates only the LCD path: ESP32-S3 I80 bus, ST7796 initialization, backlight and visible RGB565 output. Touch, TE synchronization, SD, audio, RS-485, Wi-Fi and BLE are not initialized.

## Evidence basis

For this exact specimen, the verified factory firmware contains:

- ST7796 driver strings and code;
- an ESP32-S3 8080 LCD bus implementation;
- a 480x320 embedded display resource;
- a physical factory LCD test that successfully showed RGB/grayscale patterns and red/green/blue full-screen states;
- a physically observed `MADCTL=0x28`, consistent with BGR plus axis swap for 480x320 landscape operation.

The working pin hypothesis used here is independently published for the WT32-SC01-PLUS family and is now being tested directly on this specimen:

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
| CS | `-1` — treated as tied/always selected |

**Evidence ceiling:** until this firmware is run and visually confirmed on the reference specimen, the mapping above remains a test hypothesis supported by factory RE + family documentation, not a new direct HW-02 PASS.

## Software configuration

The project deliberately does **not** keep a separate `sdkconfig.defaults` copy. Its top-level CMake file loads the centralized, physically validated profile:

```text
software/espressif/config/panlee-v15-230208-sample-a.idf6.0.2.sdkconfig.defaults
```

That profile currently establishes:

```text
CONFIG_IDF_TARGET="esp32s3"
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_SPIRAM=y
```

The ST7796 driver is pulled from Espressif Component Registry as:

```yaml
espressif/esp_lcd_st7796: "1.4.0"
```

## First-run choices

These are conservative test settings, not yet board-family guarantees:

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

## Physical PASS criteria

Mark `01_display_test` PASS only when the reference specimen visibly demonstrates all of the following:

- the backlight turns on after LCD initialization;
- the full active area is used;
- black, white, red, green and blue are visually correct;
- the eight color bars are stable and distinct;
- grayscale progresses monotonically without gross color tinting;
- geometry orientation is correct: TL red, TR green, BL blue, BR white;
- cyan border reaches all four edges;
- no persistent missing bands, corrupted regions or obvious stuck data bits;
- no gross flicker or repeated controller resets.

A wrong rotation, swapped red/blue, missing data lines or unstable image is an **investigation result**, not an excuse to change several parameters simultaneously. Change one variable at a time and preserve the failed observation.

## Build

From the repository root in an activated ESP-IDF 6.0.2 shell:

```powershell
git switch agent/01-display-test
cd examples\01_display_test
idf.py fullclean
idf.py build
```

On the first build, ESP-IDF Component Manager may download the official ST7796 component.

Do not flash until the build completes successfully and the generated flash command has been inspected.

## Flash and monitor

After a successful build:

```powershell
idf.py -p COM10 flash monitor
```

Exit `idf_monitor` with:

```text
Ctrl+]
```

or `Ctrl+T`, then `Ctrl+X`.

## Evidence after the physical run

Store the validation protocol under:

```text
evidence/specimens/panlee-v15-230208-sample-a/01_display_test/
```

Record at minimum:

- application commit/version;
- ESP-IDF and ST7796 component versions;
- exact pin map and I80 clock;
- complete boot/serial log;
- visual observations for every pattern;
- photo/video if available;
- final PASS / INVESTIGATE result.

Do not promote HW-02 to PASS before the visible panel result is confirmed.
