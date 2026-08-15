# 08_lvgl_demo

Integrated LVGL touch demo for the reference **Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC / 230208** specimen.

**Current status:** `PHYSICAL PASS`

## Physically validated hardware

This example integrates the display and touch paths previously established by the hardware tests and has now been validated on the real reference specimen:

```text
HW-02 display path       : PASS
HW-03 raw touch path     : PASS
HW-03 touch orientation  : PASS
08 LVGL + touch          : PASS
```

Validated display path:

```text
ST7796-compatible
480x320 landscape
8-bit I80
10 MHz
BL GPIO45
RST GPIO4
DC GPIO0
WR GPIO47
D0..D7 = 9,46,3,8,18,17,16,15
```

Validated touch path:

```text
FT6336U-compatible signature
I2C1
SDA GPIO6
SCL GPIO5
INT GPIO7
RST GPIO4 shared with LCD reset
400 kHz
address 0x38
reg 0xA0 = 0x02
```

Validated raw-to-LCD transform:

```c
lcd_x = raw_y;
lcd_y = 319 - raw_x;
```

Equivalent orientation flags:

```text
swap_xy  = true
mirror_x = false
mirror_y = true
```

## Purpose

This is the first integrated GUI test. It verifies that the separately validated LCD and touch paths work together through LVGL rather than only through standalone diagnostic code.

The display presents three large buttons:

```text
RED   GREEN   BLUE
```

A successful LVGL click updates two labels:

```text
Last button: <name>
LVGL touch events: <count>
```

## Physical validation result

**PASS — 2026-08-15.**

The demo was built, flashed and exercised on the real Panlee reference board. The LVGL interface rendered correctly in 480x320 landscape orientation and touch events were mapped to the displayed controls correctly.

Observed photographic evidence includes:

```text
Last button: BLUE
LVGL touch events: 6
```

This confirms the complete path:

```text
FT6336U-compatible touch
        -> raw coordinates
        -> validated orientation transform
        -> LVGL pointer input
        -> LVGL hit testing
        -> button event callback
        -> on-screen state update
```

The successful BLUE-button activation is especially useful because it verifies that the touch coordinates are not merely being received: they are mapped into the same coordinate system as the rendered LVGL objects.

## Software architecture

Display rendering uses Espressif's `esp_lvgl_port` on top of the existing `esp_lcd` ST7796/I80 path.

Touch input deliberately remains a small direct FT6336U-compatible read adapter rather than changing the already validated controller configuration. The LVGL pointer callback reads:

```text
TD_STATUS 0x02
P1_XH..P1_YL starting at 0x03
```

and supplies mapped pointer coordinates to LVGL.

No touch-controller configuration registers are written.

## Dependencies

The project declares managed components in `main/idf_component.yml`:

```text
espressif/esp_lcd_st7796 1.4.0
espressif/esp_lvgl_port  2.9.0
lvgl/lvgl                ^9.3
```

The first build may therefore spend additional time downloading managed components.

## Build and flash

From an activated ESP-IDF 6.0.2 shell:

```powershell
cd C:\Users\CHUWI\Documents\GitHub\WT32-SC01-PLUS-Lab
cd .\examples\08_lvgl_demo
idf.py fullclean
idf.py build
idf.py -p COM10 flash monitor
```

## Expected startup log

A successful startup should contain approximately:

```text
WT32-SC01-PLUS-Lab / 08_lvgl_demo
...
FT6336U-compatible touch signature confirmed: 0xA0=0x02
...
[READY]
Touch RED / GREEN / BLUE buttons on the display.
Each successful LVGL click increments the on-screen event counter.
RESULT: READY FOR PHYSICAL LVGL TOUCH VALIDATION
END 08_lvgl_demo startup
```

The final `READY FOR PHYSICAL LVGL TOUCH VALIDATION` message is a firmware startup message; the repository-level status of this example is now **PHYSICAL PASS**.

## Reproduction / validation procedure

1. confirm the LVGL interface is rendered correctly and fills the 480x320 landscape screen;
2. touch `RED` and confirm `Last button: RED`;
3. touch `GREEN` and confirm `Last button: GREEN`;
4. touch `BLUE` and confirm `Last button: BLUE`;
5. repeat touches at different points inside the buttons and confirm the counter increments as expected;
6. check that touching empty background does not falsely activate a button;
7. watch for resets, display corruption, stuck touch, I2C errors or watchdog events.

## Validated claim

The real reference specimen has demonstrated:

- LVGL startup without panic or watchdog reset;
- visually correct 480x320 landscape UI;
- working FT6336U-compatible touch path at I2C address `0x38`;
- correct raw-to-LCD orientation mapping;
- LVGL object hit testing at displayed button locations;
- button callback execution and on-screen event-count/state updates.

## Claim boundary

This PASS validates the integrated display + single-point touch + LVGL event path on the named specimen.

It does not yet validate:

- multi-touch;
- gestures;
- interrupt-driven touch input;
- high-precision edge calibration;
- long-duration GUI stability;
- animation/frame-rate performance;
- PSRAM-backed full-frame rendering;
- Wi-Fi/BLE running concurrently with LVGL;
- portability to every WT32-SC01-PLUS OEM revision.
