# 02_touch_orientation_test

Display-assisted five-point orientation test for the reference **Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC / 230208** specimen.

**Status:** `PASS — SIMPLE RAW->480x320 ORIENTATION TRANSFORM`

Prerequisite:

```text
HW-02 display path = PASS
HW-03 raw touch path = PASS
```

This test does not replace `02_touch_test`. The raw touch probe remains the canonical proof that the FT6336U-compatible touch read path works. This second-stage test uses the already validated LCD and touch paths together to determine the coordinate transform.

## Physical PASS result

The operator touched the five displayed targets in the requested order and the firmware captured:

```text
TOP-LEFT     lcd=( 40, 40) raw=(282, 58)
TOP-RIGHT    lcd=(439, 40) raw=(272,426)
CENTER       lcd=(240,160) raw=(170,238)
BOTTOM-LEFT  lcd=( 40,279) raw=( 35, 37)
BOTTOM-RIGHT lcd=(439,279) raw=( 40,439)
```

The best of the eight simple swap/mirror candidates was:

```text
swap_xy  = true
mirror_x = false
mirror_y = true
RMS      = 11.92 px
```

The second-best candidate had RMS `214.31 px`, so the orientation result is strongly separated.

For the validated 320x480 native raw geometry and 480x320 LCD landscape geometry, this reduces to the simple integer mapping:

```text
LCD_X = raw_Y
LCD_Y = 319 - raw_X
```

Representative point checks from the physical run:

```text
raw=(282, 58) -> mapped=( 58, 37), target=( 40, 40)
raw=(272,426) -> mapped=(426, 47), target=(439, 40)
raw=(170,238) -> mapped=(238,149), target=(240,160)
raw=( 35, 37) -> mapped=( 37,284), target=( 40,279)
raw=( 40,439) -> mapped=(439,279), target=(439,279)
```

The residual error is dominated by manual touch placement and the deliberately simple no-scale/no-offset mapping. It is sufficiently small for orientation certification, but not a claim of precision calibration.

## Hardware configuration

Display:

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

Touch:

```text
FT6336U-compatible signature
I2C1
SDA GPIO6
SCL GPIO5
INT GPIO7
RST GPIO4 shared with LCD reset
400 kHz
address 0x38
```

## Shared reset handling

The LCD driver performs its normal reset through GPIO4. This same signal also resets/releases the touch controller. After LCD initialization, the firmware waits 250 ms before accessing the touch controller.

Touch startup during the physical orientation run succeeded immediately after the LCD reset sequence:

```text
[TOUCH STARTUP]
  direct read 0xA0 : 0x02
```

Thus the integrated LCD + touch startup sequence is validated for this specimen.

## Acceptance conclusion

The simple orientation transform is **PASS** for `panlee-v15-230208-sample-a`:

```text
swap_xy=true
mirror_x=false
mirror_y=true
```

Equivalent direct transform:

```c
lcd_x = raw_y;
lcd_y = 319 - raw_x;
```

This is the coordinate orientation to use as the baseline for an integrated LVGL touch callback on this specimen.

## Claim boundary

This PASS establishes axis swap and mirroring for the tested physical specimen. It does not yet establish:

- precision edge calibration;
- per-device scale/offset correction;
- nonlinear correction;
- multi-touch calibration;
- long-term drift;
- interrupt-driven latency/timing;
- portability to every WT32-SC01-PLUS OEM revision.

No calibration data was persisted and no touch-controller configuration registers were written.