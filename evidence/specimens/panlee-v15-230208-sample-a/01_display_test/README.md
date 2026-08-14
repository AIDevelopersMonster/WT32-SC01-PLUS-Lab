# 01_display_test — physical validation protocol

**Status:** `PASS`

**Acceptance stage:** `HW-02`  
**Specimen:** `panlee-v15-230208-sample-a`  
**Board marking:** Panlee / `ZX3D50CE08S-V15-USRC` / `230208`  
**Physical run date:** 2026-08-14  
**Test project:** [`examples/01_display_test`](../../../../examples/01_display_test/)  
**Application version:** `69c1325`  
**ESP-IDF:** `v6.0.2`  
**ST7796 component:** `espressif/esp_lcd_st7796 1.4.0`

## Purpose

This run validates the display path independently of the original factory application. The test intentionally excludes touch, LVGL, TE synchronization, SD, audio, RS-485, Wi-Fi and BLE.

The test exercises:

- ESP32-S3 8-bit I80 LCD bus;
- ST7796-compatible panel initialization;
- panel reset and backlight control;
- RGB565 pixel transfer;
- full-screen fills;
- color ordering;
- grayscale progression;
- orientation and visible active-area geometry.

## Configuration used

```text
Logical resolution : 480 x 320 landscape
I80 bus width      : 8 bit
I80 pixel clock    : 10 MHz
Pixel format       : RGB565 / 16 bpp
Color order        : BGR
Axis swap          : enabled
Mirror             : disabled
CS                 : tied/unused (-1)
TE                 : GPIO48, intentionally unused
Touch              : not initialized
```

### Physically validated display pin map

| Signal | GPIO |
|---|---:|
| BL | 45 |
| RESET | 4 |
| DC / RS | 0 |
| WR | 47 |
| D0 | 9 |
| D1 | 46 |
| D2 | 3 |
| D3 | 8 |
| D4 | 18 |
| D5 | 17 |
| D6 | 16 |
| D7 | 15 |
| TE | 48 — not used by this test |
| CS | tied / `-1` |

This mapping was previously supported by factory-firmware reverse engineering and family documentation. This run promotes the actively used LCD signals above to direct physical evidence for this specimen.

## Build result

The standalone ESP-IDF project built successfully using the centralized specimen profile:

```text
software/espressif/config/
panlee-v15-230208-sample-a.idf6.0.2.sdkconfig.defaults
```

Generated application image:

```text
wt32_sc01_plus_display_test.bin
size: 0x32680 bytes
1 MiB app partition free: approximately 80%
```

The build resolved these principal dependencies:

```text
idf                         6.0.2
espressif/esp_lcd_st7796    1.4.0
espressif/cmake_utilities   0.5.3
```

## Flash / boot result

The firmware was written through the ESP32-S3 USB Serial/JTAG connection and verified by esptool after programming.

Boot completed normally. Relevant runtime observations:

```text
ESP-IDF: v6.0.2
SPI Flash Size: 16MB
PSRAM: Found 2MB
PSRAM speed: 40MHz
SPI SRAM memory test OK
Project: wt32_sc01_plus_display_test
App version: 69c1325
```

The public protocol intentionally omits the specimen MAC address.

## LCD initialization result

The LCD component initialized without error:

```text
st7796: version: 1.4.0
st7796_general: LCD panel create success, version: 1.4.0
display_test: Resetting and initializing ST7796
display_test: LCD initialized; starting visual pattern loop
```

No LCD DMA timeout, panic, watchdog reset or controller reinitialization error was observed in the supplied run log.

## Visual test sequence

The program continuously cycles through eight patterns:

1. solid black;
2. solid white;
3. solid red;
4. solid green;
5. solid blue;
6. eight vertical color bars;
7. left-to-right grayscale;
8. orientation / geometry pattern.

The orientation pattern is designed to expose rotation, mirroring, active-area and color-path errors:

```text
+------------------------------------------------+
| RED                                       GREEN |
|                                                |
|                 yellow cross                   |
|                                                |
| BLUE                                      WHITE |
+------------------------------------------------+
```

It additionally contains a cyan outer border and a gray 40-pixel grid.

## Physical observation

The operator visually confirmed that the real panel was working correctly and cycling through the intended test screens. The run log shows repeated complete pattern cycles; more than ten cycles were observed without a reported display failure.

The serial log repeatedly reached:

```text
PATTERN 0: SOLID BLACK
PATTERN 1: SOLID WHITE
PATTERN 2: SOLID RED
PATTERN 3: SOLID GREEN
PATTERN 4: SOLID BLUE
PATTERN 5: 8 COLOR BARS
PATTERN 6: HORIZONTAL GRAYSCALE
PATTERN 7: ORIENTATION / GEOMETRY
Cycle complete. Physical PASS requires operator visual confirmation.
```

Operator visual confirmation was subsequently provided, so the test is promoted to `PASS`.

## Runtime behavior / how to stop

`01_display_test` is intentionally a continuous visual burn-in/diagnostic loop. It does **not** return from `app_main()` while running normally.

To leave only the ESP-IDF serial monitor:

```text
Ctrl+]
```

or use:

```text
Ctrl+T, then Ctrl+X
```

Leaving the monitor does not stop the firmware; the display loop continues on the board. To stop the running firmware itself, remove/reset power or flash another application. Pressing RESET starts the same display test again from the beginning.

## Conclusion

**HW-02 display path: PASS for this physical specimen.**

Directly validated at the current test settings:

- 480x320 landscape operation;
- ESP32-S3 8-bit I80 data path;
- GPIO mapping listed above;
- reset and active-high backlight path;
- ST7796-compatible initialization using `esp_lcd_st7796 1.4.0`;
- RGB565 full-screen transfers and repeated visual pattern operation;
- stable operation at 10 MHz I80 pixel clock during the observed run.

### Claim boundary

This PASS belongs specifically to `panlee-v15-230208-sample-a`. It must not be generalized automatically to all WT32-SC01-PLUS OEM revisions.

This test does not validate:

- touch-controller identity or touch coordinates;
- TE synchronization on GPIO48;
- maximum stable I80 clock;
- LVGL integration;
- display behavior under long-duration thermal/stress testing.
