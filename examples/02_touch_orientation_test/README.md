# 02_touch_orientation_test

Display-assisted five-point orientation test for the reference **Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC / 230208** specimen.

**Status:** `READY FOR BUILD / PHYSICAL VALIDATION PENDING`

Prerequisite:

```text
HW-02 display path = PASS
HW-03 raw touch path = PASS
```

This test does not replace `02_touch_test`. The raw touch probe remains the canonical proof that the FT6336U-compatible touch read path works. This second-stage test uses the already validated LCD and touch paths together only to determine the coordinate transform.

## Purpose

The LCD is used as a logical 480x320 landscape display, while the touch controller produces coordinates consistent with a native approximately 320x480 coordinate system.

The test displays five known targets and records the median raw coordinate measured at each one:

```text
TOP-LEFT     lcd=( 40, 40)
TOP-RIGHT    lcd=(439, 40)
CENTER       lcd=(240,160)
BOTTOM-LEFT  lcd=( 40,279)
BOTTOM-RIGHT lcd=(439,279)
```

For each target, five consecutive raw samples are captured and the median X and median Y are stored.

The firmware then evaluates all eight simple axis-orientation candidates formed by:

```text
swap_xy   = false / true
mirror_x  = false / true
mirror_y  = false / true
```

and reports their RMS error against the five known LCD target positions.

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

Before starting the targets, the firmware requires a successful read of register `0xA0` and expects the already validated FT6336U-compatible value:

```text
0xA0 = 0x02
```

If that signature cannot be read after the display reset sequence, the orientation test aborts rather than collecting ambiguous calibration data.

## Operator procedure

For each displayed red target:

1. make sure the previous touch has been released;
2. touch the center of the red target;
3. hold briefly until the serial monitor prints `CAPTURED`;
4. release;
5. repeat for the next target.

The sequence is:

```text
TOP-LEFT
TOP-RIGHT
CENTER
BOTTOM-LEFT
BOTTOM-RIGHT
```

A target times out after 15 seconds if a stable capture is not obtained.

## Expected serial output

A successful capture will contain a block similar to:

```text
[FIVE-POINT CAPTURE]
  TOP-LEFT     lcd=( 40, 40) raw=(...,...)
  TOP-RIGHT    lcd=(439, 40) raw=(...,...)
  CENTER       lcd=(240,160) raw=(...,...)
  BOTTOM-LEFT  lcd=( 40,279) raw=(...,...)
  BOTTOM-RIGHT lcd=(439,279) raw=(...,...)

[TRANSFORM CANDIDATES]
  #1 swap_xy=... mirror_x=... mirror_y=... RMS=... px
  ...

[BEST TRANSFORM]
  swap_xy  : ...
  mirror_x : ...
  mirror_y : ...
  RMS error: ... px
```

The program labels the result `PASS CANDIDATE` only when the best candidate has RMS error <= 35 px and is separated from the second-best candidate by at least 25 px.

The operator must still confirm that the intended five targets were actually touched in the requested order before the result is promoted to a repository-level orientation PASS.

## No persistence / no controller configuration

The test:

- does not write calibration to NVS;
- does not write files;
- does not modify touch-controller registers;
- does not enter touch factory mode;
- does not calibrate by changing controller firmware parameters.

It only reads touch data and drives the already validated LCD.

## Build

From an activated ESP-IDF 6.0.2 shell:

```powershell
cd C:\Users\CHUWI\Documents\GitHub\WT32-SC01-PLUS-Lab
git switch agent/02-touch-test
git pull
cd .\examples\02_touch_orientation_test
idf.py build
```

After the build has been reviewed:

```powershell
idf.py -p COM10 flash monitor
```

## Claim boundary

A successful five-point run can establish the simple swap/mirror orientation required to map the native touch axes into the 480x320 LCD coordinate system.

It does not by itself establish high-precision edge calibration, nonlinear correction, multi-touch calibration, long-term drift, or portability to other WT32-SC01-PLUS OEM revisions.
