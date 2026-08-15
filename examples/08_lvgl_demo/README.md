# 08_lvgl_demo

Integrated LVGL touch demo for the reference **Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC / 230208** specimen.

**Current status:** `READY FOR BUILD / PHYSICAL VALIDATION PENDING`

## Prerequisites already physically validated

This example intentionally reuses only hardware behavior already established by earlier tests:

```text
HW-02 display path       : PASS
HW-03 raw touch path     : PASS
HW-03 touch orientation  : PASS
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

The physical PASS criterion is therefore stronger than seeing pixels or raw coordinates separately: touching each displayed button must cause the expected LVGL object event.

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

## Build

From an activated ESP-IDF 6.0.2 shell:

```powershell
cd C:\Users\CHUWI\Documents\GitHub\WT32-SC01-PLUS-Lab
git fetch origin
git switch --track origin/agent/08-lvgl-demo
cd .\examples\08_lvgl_demo
idf.py fullclean
idf.py build
```

Do **not** flash until the build output has been reviewed.

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

## Physical validation procedure

After a successful reviewed build:

```powershell
idf.py -p COM10 flash monitor
```

On the display:

1. confirm the LVGL interface is rendered correctly and fills the 480x320 landscape screen;
2. touch `RED` and confirm `Last button: RED`;
3. touch `GREEN` and confirm `Last button: GREEN`;
4. touch `BLUE` and confirm `Last button: BLUE`;
5. repeat touches at different points inside the buttons and confirm the counter increments exactly as expected;
6. check that touching empty background does not increment the button-event counter;
7. watch for resets, display corruption, stuck touch, I2C errors or watchdog events.

## PASS criteria

Promote `08_lvgl_demo` to physical PASS only when the real specimen demonstrates all of the following:

- LVGL starts without panic or watchdog reset;
- the UI is visually correct in 480x320 landscape orientation;
- FT6336U-compatible signature check succeeds after the shared LCD/touch reset sequence;
- all three buttons respond at their displayed locations;
- the reported button name matches the object actually touched;
- the LVGL event counter increases on valid button clicks;
- empty-screen touches do not falsely activate a button;
- repeated touches remain stable.

## Claim boundary

A PASS here validates the integrated display + single-point touch + LVGL event path on the named specimen.

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
